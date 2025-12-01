package com.sundtrack.robotarm.movement.controller;

import com.sundtrack.robotarm.movement.dto.JogEventDto;
import com.sundtrack.robotarm.movement.service.MotorControlService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.messaging.handler.annotation.DestinationVariable;
import org.springframework.messaging.handler.annotation.MessageMapping;
import org.springframework.messaging.handler.annotation.Payload;
import org.springframework.stereotype.Controller;

@Controller
public class MotorControlController {

    private static final Logger logger = LoggerFactory.getLogger(MotorControlController.class);

    private final MotorControlService motorControlService;

    @Autowired
    public MotorControlController(MotorControlService motorControlService) {
        this.motorControlService = motorControlService;
    }

    @MessageMapping("/motors/jog")
    public void handleJogEvent(@Payload JogEventDto jogEvent) {
        // This method doesn't use @SendTo because jogging is a one-way command.
        // Feedback (like current position) would be sent from the SerialPortService.
        motorControlService.handleJog(jogEvent);
    }

    @MessageMapping("/sequences/{id}/play")
    public void playSequence(@DestinationVariable Long id) {
        logger.info("Received request to play sequence {}", id);
        motorControlService.playSequence(id);
    }

    @MessageMapping("/sequences/stop")
    public void stopSequence() {
        logger.info("Received request to stop all playback");
        motorControlService.stopPlayback();
    }
}