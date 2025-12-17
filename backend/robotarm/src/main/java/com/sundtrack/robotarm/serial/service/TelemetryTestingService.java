package com.sundtrack.robotarm.serial.service;

import com.sundtrack.robotarm.sequence.controller.SequenceController;
import com.sundtrack.robotarm.serial.dto.TelemetryPayloadDto;
import com.sundtrack.robotarm.state.RobotStateService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.messaging.simp.SimpMessagingTemplate;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;

import java.util.Random;

@Service
@ConditionalOnProperty(name = "robot.telemetry.simulation.enabled", havingValue = "true", matchIfMissing = false)
public class TelemetryTestingService {

    private final SimpMessagingTemplate messagingTemplate;
    private final RobotStateService robotStateService;
    private final Random random = new Random();
    private static final Logger logger = LoggerFactory.getLogger(TelemetryTestingService.class);


    // Internal state to simulate continuous movement (Random Walk)
    private final long[] simulatedPositions = new long[6];

    @Autowired
    public TelemetryTestingService(SimpMessagingTemplate messagingTemplate, RobotStateService robotStateService) {
        this.messagingTemplate = messagingTemplate;
        this.robotStateService = robotStateService;
    }

    // Run every 100ms (10Hz)
    @Scheduled(fixedRate = 10000)
    public void sendRandomTelemetry() {
        // Update positions with a small random change to simulate movement
        for (int i = 0; i < simulatedPositions.length; i++) {
            simulatedPositions[i] += (random.nextInt(21) - 10); // Change by -10 to +10 steps
        }

        // Create a copy for the DTO to ensure thread safety/immutability during serialization
        long[] positionsCopy = simulatedPositions.clone();
        TelemetryPayloadDto telemetry = new TelemetryPayloadDto(positionsCopy, positionsCopy.length);

        // Update the backend state service so other services (like MathService) see the simulated position
        robotStateService.updateMotorPositions(positionsCopy);

        // Send to the frontend via the existing WebSocket topic
        logger.info("Telemetry information sent with the following: {}", telemetry);
        messagingTemplate.convertAndSend("/topic/serial/telemetry", telemetry);
    }
}
