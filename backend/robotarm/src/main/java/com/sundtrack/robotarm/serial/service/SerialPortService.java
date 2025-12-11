package com.sundtrack.robotarm.serial.service;

import com.fasterxml.jackson.databind.ObjectMapper; // Import added
import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortEvent;
import com.fazecast.jSerialComm.SerialPortMessageListener;
import com.sundtrack.robotarm.serial.dto.BasePayloadDto;
import com.sundtrack.robotarm.serial.dto.IncomingMessageDto;
import com.sundtrack.robotarm.serial.dto.LogPayloadDto;
import com.sundtrack.robotarm.serial.dto.TelemetryPayloadDto;
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
import java.util.concurrent.ScheduledFuture;

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

    // --- PORT CONFIG ADDED HERE ---
    @Value("${serial.port.name:COM6}") // Defaults to COM6 if not found in properties
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

    public void writeToSerial(String data) {
        if (commPort == null || !commPort.isOpen()) {
            String pName = (commPort != null ? commPort.getSystemPortName() : "not initialized");
            logger.warn("Attempted to write to serial, but port [{}] is not open.", pName);
            scheduleReconnection();
            throw new IllegalStateException("Serial port is not open or available.");
        }
        String dataToSend = data + "\n";
        byte[] bytesToSend = dataToSend.getBytes(StandardCharsets.UTF_8);
        logger.info(">> Writing to serial: {}", data);
        commPort.writeBytes(bytesToSend, bytesToSend.length);
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
            }
            logger.info("Message received from serial: {}", message);

        } catch (Exception e) {
            logger.error("Failed to parse JSON from serial: '{}'", message, e);
            messagingTemplate.convertAndSend("/topic/serial/raw", message);
        }
    } // <--- PREVIOUSLY, THE CLASS CLOSED HERE. THIS WAS THE ERROR.

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
            cancelReconnectionTask();
            commPort.addDataListener(this);
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
} // <--- CLASS CORRECTLY CLOSES HERE