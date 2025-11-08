package com.sundtrack.robotarm;

import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortDataListener;
import com.fazecast.jSerialComm.SerialPortEvent;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.CommandLineRunner;
import org.springframework.messaging.simp.SimpMessagingTemplate;
import org.springframework.stereotype.Service;

import java.nio.charset.StandardCharsets;

@Service
public class SerialPortService implements CommandLineRunner, SerialPortDataListener {

    private static final Logger logger = LoggerFactory.getLogger(SerialPortService.class);

    // This template is our gateway to send messages to WebSocket clients
    @Autowired
    private SimpMessagingTemplate messagingTemplate;

    private SerialPort commPort;

    @Override
    public void run(String... args) throws Exception {
        logger.info("Starting SerialPortService...");

        // Find the correct port
        // You'll need to change "COM3" to your ESP32's port
        // On Mac/Linux, it will be something like "/dev/tty.usbserial-XXXX"
        commPort = SerialPort.getCommPort("COM3"); // <--- IMPORTANT: SET YOUR PORT HERE

        // Set port parameters (must match your ESP32's Serial.begin())
        commPort.setBaudRate(9600);
        commPort.setNumDataBits(8);
        commPort.setNumStopBits(1);
        commPort.setParity(SerialPort.NO_PARITY);

        // Try to open the port
        if (commPort.openPort()) {
            logger.info("Successfully opened port: {}", commPort.getSystemPortName());

            // Set a timeout for reading
            commPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_BLOCKING, 1000, 0);

            // Add this class as a listener
            commPort.addDataListener(this);
        } else {
            logger.error("Failed to open port: {}", commPort.getSystemPortName());
            logger.warn("Please ensure no other program (like Arduino IDE) is using the port.");
        }
    }

    @Override
    public int getListeningEvents() {
        // We want to be notified when new data is available
        return SerialPort.LISTENING_EVENT_DATA_AVAILABLE;
    }

    @Override
    public void serialEvent(SerialPortEvent event) {
        if (event.getEventType() != SerialPort.LISTENING_EVENT_DATA_AVAILABLE) {
            return; // Ignore other event types
        }

        // Read the available data
        byte[] readBuffer = new byte[commPort.bytesAvailable()];
        int numRead = commPort.readBytes(readBuffer, readBuffer.length);

        if (numRead > 0) {
            // Convert the bytes to a String, trimming any whitespace (like newlines)
            String message = new String(readBuffer, StandardCharsets.UTF_8).trim();
            logger.info("Read from serial: {}", message);

            // Send the message to the WebSocket topic "/topic/serial"
            // The web client will be subscribed to this topic
            messagingTemplate.convertAndSend("/topic/serial", message);
        }
    }
}
