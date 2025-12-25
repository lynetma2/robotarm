package com.sundtrack.robotarm.serial.service;

import com.fasterxml.jackson.databind.ObjectMapper; // Import added
import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortEvent;
import com.fazecast.jSerialComm.SerialPortMessageListener;
import com.sundtrack.robotarm.serial.dto.*;
import com.sundtrack.robotarm.state.RobotStateService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value; // Import added
import org.springframework.boot.CommandLineRunner;
import org.springframework.messaging.simp.SimpMessagingTemplate;
import org.springframework.scheduling.TaskScheduler;
import org.springframework.stereotype.Service;

import java.nio.charset.StandardCharsets;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.atomic.AtomicLong;

@Service
public class SerialPortService implements CommandLineRunner, SerialPortMessageListener {

    private static final Logger logger = LoggerFactory.getLogger(SerialPortService.class);

    private final SimpMessagingTemplate messagingTemplate;
    private final TaskScheduler taskScheduler;

    // --- MISSING DEPENDENCIES ADDED HERE ---
    private final ObjectMapper objectMapper;
    private final RobotStateService robotStateService;

    private ScheduledFuture<?> reconnectionTask;
    private SerialPort commPort;

    // --- SLIDING WINDOW PROTOCOL FIELDS ---
    private final AtomicLong sequenceGenerator = new AtomicLong(0);
    private final BlockingQueue<QueuedCommand> sendQueue = new LinkedBlockingQueue<>();
    private final ConcurrentHashMap<Long, QueuedCommand> pendingAcks = new ConcurrentHashMap<>();
    private static final int WINDOW_SIZE = 5; // Klipper-like window: max 5 un-ACKed commands

    // --- PORT CONFIG ADDED HERE ---
    @Value("${serial.port.name:COM4}") // Defaults to COM6 if not found in properties
    private String portName;

    @Autowired
    public SerialPortService(SimpMessagingTemplate messagingTemplate,
                             TaskScheduler taskScheduler,
                             ObjectMapper objectMapper,
                             RobotStateService robotStateService) {
        this.messagingTemplate = messagingTemplate;
        this.taskScheduler = taskScheduler;
        this.objectMapper = objectMapper;
        this.robotStateService = robotStateService;
    }

    @Override
    public void run(String... args) throws Exception {
        connect();
    }

    /**
     * Enqueues data to be sent. The data is wrapped with a sequence number and managed
     * by a sliding window protocol.
     * @param data The JSON string (or raw string) to send.
     */
    public void writeToSerial(String data) {
        if (commPort == null || !commPort.isOpen()) {
            String pName = (commPort != null ? commPort.getSystemPortName() : "not initialized");
            logger.warn("Attempted to write to serial, but port [{}] is not open.", pName);
            scheduleReconnection();
            throw new IllegalStateException("Serial port is not open or available.");
        }

        // 1. Assign Sequence ID
        long seqId = sequenceGenerator.incrementAndGet();

        // 2. Queue the command
        sendQueue.offer(new QueuedCommand(seqId, data, System.currentTimeMillis()));

        // 3. Attempt to process the queue immediately
        processSendQueue();
    }

    private synchronized void processSendQueue() {
        // Only send if the window is not full and we have commands waiting
        while (pendingAcks.size() < WINDOW_SIZE && !sendQueue.isEmpty()) {
            QueuedCommand cmd = sendQueue.poll();
            if (cmd != null) {
                sendInternal(cmd);
                // Add to pending ACKs map to block the window until ACK received
                pendingAcks.put(cmd.id(), cmd);
            }
        }
    }

    private void sendInternal(QueuedCommand cmd) {
        try {
            // Try to parse as JSON map to inject 'seq', otherwise wrap it
            String finalJson;
            try {
                Map<String, Object> map = objectMapper.readValue(cmd.originalPayload(), Map.class);
                map.put("seq", cmd.id());
                finalJson = objectMapper.writeValueAsString(map);
            } catch (Exception e) {
                // Fallback for non-JSON strings (like raw JOG commands if not updated yet)
                // We wrap it in a generic envelope so the firmware can still read the 'seq'
                Map<String, Object> wrapper = Map.of("type", "RAW", "cmd", cmd.originalPayload(), "seq", cmd.id());
                finalJson = objectMapper.writeValueAsString(wrapper);
            }

            String dataToSend = finalJson + "\n";
            byte[] bytesToSend = dataToSend.getBytes(StandardCharsets.UTF_8);
            
            logger.info(">> Sending [SEQ:{}]: {}", cmd.id(), finalJson);
            commPort.writeBytes(bytesToSend, bytesToSend.length);
        } catch (Exception e) {
            logger.error("Failed to send command sequence {}", cmd.id(), e);
        }
    }

    // --- LISTENER CONFIGURATION ---

    @Override
    public int getListeningEvents() {
        return SerialPort.LISTENING_EVENT_DATA_RECEIVED | SerialPort.LISTENING_EVENT_PORT_DISCONNECTED;
    }

    @Override
    public byte[] getMessageDelimiter() {
        return new byte[]{'\n'};
    }

    @Override
    public boolean delimiterIndicatesEndOfMessage() {
        return true;
    }

    @Override
    public void serialEvent(SerialPortEvent event) {
        switch (event.getEventType()) {
            case SerialPort.LISTENING_EVENT_DATA_RECEIVED:
                handleDataReceived(event);
                break;
            case SerialPort.LISTENING_EVENT_PORT_DISCONNECTED:
                handlePortDisconnected();
                break;
        }
    }

    private void handleDataReceived(SerialPortEvent event) {
        byte[] messageData = event.getReceivedData();
        String message = new String(messageData, StandardCharsets.UTF_8).trim();

        try {
            // ObjectMapper is now available
            IncomingMessageDto incomingMessage = objectMapper.readValue(message, IncomingMessageDto.class);
            BasePayloadDto payload = incomingMessage.data();

            if (payload instanceof TelemetryPayloadDto telemetry) {
                // RobotStateService is now available
                robotStateService.updateMotorPositions(telemetry.motorPositions());
                messagingTemplate.convertAndSend("/topic/serial/telemetry", telemetry);

            } else if (payload instanceof LogPayloadDto log) {
                logger.info("Read LOG from serial: [{}] {}", log.level(), log.message());
                messagingTemplate.convertAndSend("/topic/serial/logs", log);
            } else if (payload instanceof MotorMovingPayloadDto motorMoving) {
                logger.info("Read MotorMoving from serial: [{}] {}", motorMoving.motorId(), motorMoving.isMoving());
                robotStateService.updateMotorMovingState(motorMoving.motorId(), motorMoving.isMoving());
            } else if (payload instanceof AckPayloadDto ack) {
                // --- ACK HANDLING ---
                if (pendingAcks.containsKey(ack.seq())) {
                    logger.debug("Received ACK for sequence {}", ack.seq());
                    pendingAcks.remove(ack.seq());
                    // Window opened up, try to send next command
                    processSendQueue();
                }
            }
            logger.debug("Message received from serial: {}", message);

        } catch (Exception e) {
            logger.error("Failed to parse JSON from serial: '{}'", message, e);
            messagingTemplate.convertAndSend("/topic/serial/raw", message);
        }
    } // <--- PREVIOUSLY, THE CLASS CLOSED HERE. THIS WAS THE ERROR.

    // --- RETRANSMISSION TASK ---
    // Checks every 500ms for commands that haven't been ACKed in 2 seconds
    @org.springframework.scheduling.annotation.Scheduled(fixedRate = 500)
    public void checkTimeouts() {
        if (pendingAcks.isEmpty()) return;

        long now = System.currentTimeMillis();
        for (QueuedCommand cmd : pendingAcks.values()) {
            if (now - cmd.timestamp() > 2000) {
                logger.warn("Command [SEQ:{}] timed out waiting for ACK. Retransmitting...", cmd.id());
                sendInternal(cmd);
                // Update timestamp to prevent spamming retransmits
                pendingAcks.put(cmd.id(), new QueuedCommand(cmd.id(), cmd.originalPayload(), now));
            }
        }
    }

    // --- PRIVATE HELPER METHODS (NOW CORRECTLY INSIDE THE CLASS) ---

    private void handlePortDisconnected() {
        // Check for null to avoid NullPointerException if called oddly
        String name = (commPort != null) ? commPort.getSystemPortName() : "Unknown";
        logger.warn("Serial port [{}] disconnected.", name);
        closeAndScheduleReconnection();
    }

    private void connect() {
        if (commPort != null && commPort.isOpen()) {
            logger.info("Connect call ignored, port is already open.");
            return;
        }

        // Use the variable injected from properties
        commPort = SerialPort.getCommPort(portName);

        commPort.setBaudRate(115200);
        commPort.setNumDataBits(8);
        commPort.setNumStopBits(1);
        commPort.setParity(SerialPort.NO_PARITY);

        if (commPort.openPort()) {
            logger.info("Successfully opened port: {}", commPort.getSystemPortName());
            commPort.setComPortTimeouts(SerialPort.TIMEOUT_WRITE_BLOCKING, 0, 0);
            cancelReconnectionTask();
            commPort.addDataListener(this);
            // Reset protocol state on new connection
            sequenceGenerator.set(0);
            pendingAcks.clear();
            sendQueue.clear();
        } else {
            logger.warn("Failed to open port: {}. Will retry automatically.", commPort.getSystemPortName());
            scheduleReconnection();
        }
    }

    private void closeAndScheduleReconnection() {
        if (commPort != null && commPort.isOpen()) {
            commPort.removeDataListener(); // Good practice to remove listener before closing
            commPort.closePort();
        }
        scheduleReconnection();
    }

    private synchronized void scheduleReconnection() {
        if (reconnectionTask == null || reconnectionTask.isDone()) {
            logger.info("Scheduling reconnection task to run every 5 seconds.");
            reconnectionTask = taskScheduler.scheduleAtFixedRate(this::connect, 5000);
        }
    }

    private synchronized void cancelReconnectionTask() {
        if (reconnectionTask != null && !reconnectionTask.isDone()) {
            logger.info("Serial connection established. Cancelling reconnection task.");
            reconnectionTask.cancel(false);
        }
    }

    // Internal record to track commands
    private record QueuedCommand(long id, String originalPayload, long timestamp) {}
} // <--- CLASS CORRECTLY CLOSES HERE