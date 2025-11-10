package com.sundtrack.robotarm;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.messaging.handler.annotation.MessageMapping; // Import this
import org.springframework.messaging.handler.annotation.Payload;     // Import this
import org.springframework.stereotype.Controller;                 // Import this
import org.springframework.web.bind.annotation.*;

@RestController // @RestController includes @Controller, so @MessageMapping will work
@RequestMapping("/api/serial")
public class SerialController {

    private static final Logger logger = LoggerFactory.getLogger(SerialController.class);

    @Autowired
    private SerialPortService serialPortService;

    // --- (NEW) WebSocket Message Handler ---

    /**
     * Handles incoming WebSocket messages sent to the "/app/serial/write" destination.
     * The payload is expected to be a simple string (the command to send).
     *
     * @param command The raw string message from the WebSocket client.
     */
    @MessageMapping("/serial/write") // Client must send to "/app/serial/write"
    public void handleSerialWrite(@Payload String command) {
        try {
            logger.info("Received WebSocket command: {}", command);
            // Call the *exact same* service method
            serialPortService.writeToSerial(command);
        } catch (Exception e) {
            logger.error("Error writing to serial from WebSocket: {}", e.getMessage(), e);
            // Optionally, you could send an error back to this specific user
            // using @SendToUser or SimpMessagingTemplate
        }
    }


    // --- (Existing) REST API Handler ---

    public static class SerialWriteRequest {
        private String data;
        public String getData() { return data; }
        public void setData(String data) { this.data = data; }
    }

    @PostMapping("/write")
    public ResponseEntity<?> writeToSerial(@RequestBody SerialWriteRequest request) {
        if (request == null || request.getData() == null) {
            return ResponseEntity.badRequest().body("Invalid request. 'data' field is required.");
        }

        try {
            serialPortService.writeToSerial(request.getData());
            return ResponseEntity.ok().body("Data successfully sent: " + request.getData());
        } catch (IllegalStateException e) {
            return ResponseEntity.status(503).body(e.getMessage());
        } catch (Exception e) {
            return ResponseEntity.status(500).body("Error writing to serial: " + e.getMessage());
        }
    }
}