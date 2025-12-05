package com.sundtrack.robotarm.serial.service;

import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortEvent;
import com.fazecast.jSerialComm.SerialPortMessageListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.CommandLineRunner;
import org.springframework.scheduling.TaskScheduler;
import org.springframework.messaging.simp.SimpMessagingTemplate;
import org.springframework.stereotype.Service;

import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.concurrent.ScheduledFuture;

@Service
public class SerialPortService implements CommandLineRunner, SerialPortMessageListener {

    private static final Logger logger = LoggerFactory.getLogger(SerialPortService.class);

    private final SimpMessagingTemplate messagingTemplate;
    private final TaskScheduler taskScheduler;
    private ScheduledFuture<?> reconnectionTask;

    private SerialPort commPort;

    @Autowired
    public SerialPortService(SimpMessagingTemplate messagingTemplate, TaskScheduler taskScheduler) {
        this.messagingTemplate = messagingTemplate;
        this.taskScheduler = taskScheduler;
    }

    @Override
    public void run(String... args) throws Exception {
        // Initial connection attempt on startup
        connect();
    }
    

    /**
     * Writes data to the serial port.
     * @param data The string to send. A newline character will be added.
     */
    public void writeToSerial(String data) {
        if (commPort == null || !commPort.isOpen()) {
            String portName = (commPort != null ? commPort.getSystemPortName() : "not initialized");
            logger.warn("Attempted to write to serial, but port [{}] is not open.", portName);
            scheduleReconnection(); // Attempt to reconnect if we try to write while disconnected
            throw new IllegalStateException("Serial port is not open or available.");
        }

        // Add the newline delimiter, since your device is listening for it
        String dataToSend = data + "\n";
        byte[] bytesToSend = dataToSend.getBytes(StandardCharsets.UTF_8);

        logger.info(">> Writing to serial: {}", data);
        commPort.writeBytes(bytesToSend, bytesToSend.length);
    }

    // --- NEW METHODS FOR SerialPortMessageListener ---

    @Override
    public int getListeningEvents() {
        // This tells the listener to fire for two events:
        // 1. Data has been received.
        // 2. The port has been disconnected (e.g., USB cable unplugged).
        return SerialPort.LISTENING_EVENT_DATA_RECEIVED |
               SerialPort.LISTENING_EVENT_PORT_DISCONNECTED;
    }

    @Override
    public byte[] getMessageDelimiter() {
        // We want to read until we see a newline character
        // This matches the ESP32's "println()"
        return new byte[]{'\n'};
    }

    @Override
    public boolean delimiterIndicatesEndOfMessage() {
        return true;
    }

    // --- This method is now called when a full message (ending in '\n') arrives ---

    @Override
    public void serialEvent(SerialPortEvent event) {
        // Dispatch the event to the appropriate handler method.
        switch (event.getEventType()) {
            case SerialPort.LISTENING_EVENT_DATA_RECEIVED:
                handleDataReceived(event);
                break;
            case SerialPort.LISTENING_EVENT_PORT_DISCONNECTED:
                handlePortDisconnected();
                break;
        }
    }

    /**
     * Handles an incoming message from the serial port.
     * @param event The event containing the received data.
     */
    private void handleDataReceived(SerialPortEvent event) {
        // Get the complete message data, convert to a string, and trim whitespace.
        byte[] messageData = event.getReceivedData();
        String message = new String(messageData, StandardCharsets.UTF_8).trim();

        logger.info("Read from serial: {}", message);
        messagingTemplate.convertAndSend("/topic/serial/logs", message);
    }

    /**
     * Handles the event when the serial port is disconnected.
     */
    private void handlePortDisconnected() {
        logger.warn("Serial port [{}] disconnected.", commPort.getSystemPortName());
        closeAndScheduleReconnection();
    }

    /**
     * Encapsulates the logic to find, configure, and open the serial port.
     */
    private void connect() {
        if (commPort != null && commPort.isOpen()) {
            logger.info("Connect call ignored, port is already open.");
            return;
        }

        // *** SET YOUR PORT HERE ***
        commPort = SerialPort.getCommPort("COM6");

        // Set port parameters
        commPort.setBaudRate(115200);
        commPort.setNumDataBits(8);
        commPort.setNumStopBits(1);
        commPort.setParity(SerialPort.NO_PARITY);

        if (commPort.openPort()) {
            logger.info("Successfully opened port: {}", commPort.getSystemPortName());
            cancelReconnectionTask(); // Connection is successful, cancel the retry task
            commPort.addDataListener(this);
        } else {
            logger.warn("Failed to open port: {}. Will retry automatically.", commPort.getSystemPortName());
            scheduleReconnection(); // If the initial connection fails, start the retry task
        }
    }

    private void closeAndScheduleReconnection() {
        if (commPort != null && commPort.isOpen()) {
            commPort.closePort();
        }
        scheduleReconnection();
    }

    /**
     * Schedules a recurring task to attempt reconnection if one isn't already running.
     */
    private synchronized void scheduleReconnection() {
        if (reconnectionTask == null || reconnectionTask.isDone()) {
            logger.info("Scheduling reconnection task to run every 5 seconds.");
            // The task will run every 5 seconds until it is cancelled.
            reconnectionTask = taskScheduler.scheduleAtFixedRate(this::connect, 5000);
        }
    }

    /**
     * Cancels the reconnection task if it is currently active.
     */
    private synchronized void cancelReconnectionTask() {
        if (reconnectionTask != null && !reconnectionTask.isDone()) {
            logger.info("Serial connection established. Cancelling reconnection task.");
            reconnectionTask.cancel(false); // false: don't interrupt if already running
        }
    }
}