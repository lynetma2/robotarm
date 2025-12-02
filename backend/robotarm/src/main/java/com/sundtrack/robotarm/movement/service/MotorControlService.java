package com.sundtrack.robotarm.movement.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.sundtrack.robotarm.movement.dto.JogEventDto;
import com.sundtrack.robotarm.movement.config.MovementConfig;
import com.sundtrack.robotarm.movement.dto.DriveSegmentDTO;
import com.sundtrack.robotarm.movement.dto.StepCommandDto;
import com.sundtrack.robotarm.sequence.dto.SequenceDto;
import com.sundtrack.robotarm.sequence.dto.StepDto;
import com.sundtrack.robotarm.sequence.service.SequenceService;
import com.sundtrack.robotarm.serial.service.SerialPortService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.concurrent.atomic.AtomicBoolean;

@Service
public class MotorControlService {

    private static final Logger logger = LoggerFactory.getLogger(MotorControlService.class);

    private final SerialPortService serialPortService;
    private final SequenceService sequenceService;
    private final ObjectMapper objectMapper;
    private final MathService mathService;

    // Use AtomicBoolean for thread-safe state management across different requests
    private final AtomicBoolean isPlaying = new AtomicBoolean(false);

    @Autowired
    public MotorControlService(SerialPortService serialPortService, SequenceService sequenceService, ObjectMapper objectMapper, MathService mathService, MovementConfig movementConfig) {
        this.serialPortService = serialPortService;
        this.sequenceService = sequenceService;
        this.objectMapper = objectMapper;
        this.mathService = mathService;
    }

    /**
     * Handles a real-time jog event by converting it to a hardware-specific command.
     * NOTE: This is a placeholder for your specific G-code or command protocol.
     */
    public void handleJog(JogEventDto jogEvent) {
        // Delegate command creation to MathService for consistency
        String command = mathService.createJogCommand(jogEvent);
        serialPortService.writeToSerial(command);
    }

    /**
     * Initiates the playback of a given sequence by its ID.
     * This method runs the sequence in a new thread to avoid blocking.
     */
    public void playSequence(Long sequenceId) {
        // Atomically check and set the isPlaying flag to prevent concurrent playback.
        if (!isPlaying.compareAndSet(false, true)) {
            logger.warn("Cannot play sequence {}, another is already in progress.", sequenceId);
            return;
        }

        // Execute in a new thread so the @MessageMapping method can return immediately.
        new Thread(() -> {
            try {
                SequenceDto sequence = sequenceService.getSequenceById(sequenceId);
                logger.info("Starting execution of sequence: {}", sequence.title());

                // We need at least two steps to perform a move between them.
                if (sequence.steps().size() < 2) {
                    logger.warn("Sequence has fewer than 2 steps, nothing to play.");
                    return;
                }

                for (int i = 0; i < sequence.steps().size() - 1; i++) {
                    if (!isPlaying.get()) { // Check if a 'stop' command was received
                        logger.info("Sequence playback stopped by user command.");
                        break;
                    }

                    StepDto startStep = sequence.steps().get(i);
                    StepDto endStep = sequence.steps().get(i + 1);

                    // 1. Delegate all complex calculations to the MathService
                    DriveSegmentDTO[] segments = mathService.calculateDriveSegments(startStep.pose(), endStep.pose(), endStep.speed());

                    // 2. Create the command DTO with the calculated segments
                    var command = new StepCommandDto();
                    command.setData(new StepCommandDto.StepData(endStep.id(), endStep.name(), segments));

                    // 3. Serialize and send the command
                    try {
                        String commandJson = objectMapper.writeValueAsString(command);
                        serialPortService.writeToSerial(commandJson);
                        waitForMoveCompletion(1000); // Placeholder for hardware feedback
                    } catch (JsonProcessingException e) {
                        logger.error("Failed to serialize StepCommandDto for step '{}'", endStep.name(), e);
                    }
                }
            } catch (Exception e) {
                logger.error("Error during sequence playback for ID {}: {}", sequenceId, e.getMessage());
            } finally {
                isPlaying.set(false); // Ensure the flag is always reset
                logger.info("Finished execution for sequence ID: {}", sequenceId);
            }
        }).start();
    }

    /**
     * Stops any sequence that is currently playing.
     */
    public void stopPlayback() {
        if (isPlaying.get()) {
            logger.info("Stopping sequence playback...");
            isPlaying.set(false);
            // Optionally, send a hardware-specific stop command (e.g., M0 or feed hold)
            // serialPortService.writeToSerial("M0");
        }
    }

    private void waitForMoveCompletion(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            logger.warn("Move delay was interrupted.");
        }
    }
}