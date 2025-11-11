package com.sundtrack.robotarm;

import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortEvent;
import com.fazecast.jSerialComm.SerialPortMessageListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.CommandLineRunner;
import org.springframework.messaging.simp.SimpMessagingTemplate;
import org.springframework.stereotype.Service;

import java.nio.charset.StandardCharsets;

@Service
public class SerialPortService implements CommandLineRunner, SerialPortMessageListener {

    private static final Logger logger = LoggerFactory.getLogger(SerialPortService.class);

    @Autowired
    private SimpMessagingTemplate messagingTemplate;

    private SerialPort commPort;

    @Override
    public void run(String... args) throws Exception {
        logger.info("Starting SerialPortService...");

        // *** SET YOUR PORT HERE ***
        // (e.g., "COM3" on Windows, "/dev/ttyUSB0" on Linux)
        commPort = SerialPort.getCommPort("COM6");

        // Set port parameters
        commPort.setBaudRate(115200);
        commPort.setNumDataBits(8);
        commPort.setNumStopBits(1);
        commPort.setParity(SerialPort.NO_PARITY);

        if (commPort.openPort()) {
            logger.info("Successfully opened port: {}", commPort.getSystemPortName());

            // --- THIS IS THE IMPORTANT PART ---
            // Remove the setComPortTimeouts line completely
            // Add this class as a message listener
            commPort.addDataListener(this);
        } else {
            logger.error("Failed to open port: {}", commPort.getSystemPortName());
            logger.warn("Please ensure no other program is using the port.");
        }
    }

    /**
     * Writes data to the serial port.
     * @param data The string to send. A newline character will be added.
     */
    public void writeToSerial(String data) {
        if (commPort == null || !commPort.isOpen()) {
            logger.warn("Attempted to write to serial, but port [{}] is not open.",
                    (commPort != null ? commPort.getSystemPortName() : "null"));
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
        // This tells the listener to fire when data is received
        return SerialPort.LISTENING_EVENT_DATA_RECEIVED;
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
        if (event.getEventType() == SerialPort.LISTENING_EVENT_DATA_RECEIVED) {

            // Get the complete message data
            byte[] messageData = event.getReceivedData();

            // Convert to a string and trim whitespace (like the \n)
            String message = new String(messageData, StandardCharsets.UTF_8).trim();

            logger.info("Read from serial: {}", message);
            messagingTemplate.convertAndSend("/topic/serial", message);
        }
    }
}