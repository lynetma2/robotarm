package com.sundtrack.robotarm.serial.controller;

import com.sundtrack.robotarm.serial.service.SerialPortService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.messaging.handler.annotation.MessageMapping; // Import this
import org.springframework.messaging.handler.annotation.Payload;     // Import this
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.*;

@Controller
public class SerialController {

    private static final Logger logger = LoggerFactory.getLogger(SerialController.class);

    @Autowired
    private SerialPortService serialPortService;

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
}