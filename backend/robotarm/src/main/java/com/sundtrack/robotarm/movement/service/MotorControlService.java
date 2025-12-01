package com.sundtrack.robotarm.movement.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.sundtrack.robotarm.movement.dto.JogEventDto;
import com.sundtrack.robotarm.movement.config.MovementConfig;
import com.sundtrack.robotarm.movement.dto.StepCommandDto;
import com.sundtrack.robotarm.sequence.dto.PoseDto;
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
    private final MovementConfig movementConfig;

    // Use AtomicBoolean for thread-safe state management across different requests
    private final AtomicBoolean isPlaying = new AtomicBoolean(false);

    @Autowired
    public MotorControlService(SerialPortService serialPortService, SequenceService sequenceService, ObjectMapper objectMapper, MovementConfig movementConfig) {
        this.serialPortService = serialPortService;
        this.sequenceService = sequenceService;
        this.objectMapper = objectMapper;
        this.movementConfig = movementConfig;
    }

    /**
     * Handles a real-time jog event by converting it to a hardware-specific command.
     * NOTE: This is a placeholder for your specific G-code or command protocol.
     */
    public void handleJog(JogEventDto jogEvent) {
        // Example: "JOG M<motorId> D<direction> S<speed>"
        String command = String.format("JOG M%d D%d S%.2f",
                jogEvent.motorId(),
                jogEvent.direction().getValue(),
                jogEvent.speed()
        );
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

                    logger.info("Moving from '{}' to '{}'", startStep.name(), endStep.name());
                    performInterpolatedMove(startStep.pose(), endStep.pose(), endStep.speed());
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

    /**
     * Performs a move from a start to an end pose using linear interpolation.
     * It breaks the move into small micro-steps and sends them sequentially.
     */
    private void performInterpolatedMove(PoseDto startPose, PoseDto endPose, double requestedSpeed) {
        // 1. Calculate total distance for each axis
        double dx = endPose.x() - startPose.x();
        double dy = endPose.y() - startPose.y();
        double dz = endPose.z() - startPose.z();
        // ... add dRoll, dPitch, dYaw if you want to interpolate orientation

        double totalDistance = Math.sqrt(dx*dx + dy*dy + dz*dz);

        // 2. Apply speed wrapper/scaling
        // The speed from the step is a percentage (0-100) of the configured max speed.
        double actualSpeed = movementConfig.getMaxSpeed() * (Math.min(100.0, Math.max(0.0, requestedSpeed)) / 100.0);

        if (actualSpeed <= 0) {
            logger.warn("Speed is zero or negative, skipping move.");
            return;
        }

        // 3. Calculate move duration and number of steps
        double durationSeconds = totalDistance / actualSpeed;
        int numSteps = (int) Math.ceil((durationSeconds * 1000) / movementConfig.getInterpolationStepMillis());

        if (numSteps <= 0) return;

        // 4. Loop and send interpolated micro-steps
        for (int i = 1; i <= numSteps; i++) {
            if (!isPlaying.get()) break;

            double fraction = (double) i / numSteps;

            // Calculate the interpolated pose for this micro-step
            double[] interpolatedPoseArray = {
                    startPose.x() + dx * fraction,
                    startPose.y() + dy * fraction,
                    startPose.z() + dz * fraction,
                    // ... interpolate roll, pitch, yaw here
                    startPose.roll(), // For now, keep orientation constant
                    startPose.pitch(),
                    startPose.yaw()
            };

            // Create and send the command
            StepCommandDto microStepCommand = createStepCommand("Interpolated Step", interpolatedPoseArray, actualSpeed);
            try {
                String commandJson = objectMapper.writeValueAsString(microStepCommand);
                serialPortService.writeToSerial(commandJson);
            } catch (JsonProcessingException e) {
                logger.error("Failed to serialize interpolated step command", e);
            }

            waitForMoveCompletion(movementConfig.getInterpolationStepMillis());
        }
    }

    /**
     * Converts a StepDto from a sequence into a StepCommandDto ready for serialization.
     */
    private com.sundtrack.robotarm.movement.dto.StepCommandDto convertToStepCommand(StepDto stepDto) {
        var command = new com.sundtrack.robotarm.movement.dto.StepCommandDto();
        var poseDto = stepDto.pose();
        double[] poseArray = {poseDto.x(), poseDto.y(), poseDto.z(), poseDto.roll(), poseDto.pitch(), poseDto.yaw()};

        var stepData = new com.sundtrack.robotarm.movement.dto.StepCommandDto.StepData(
                stepDto.id(),
                stepDto.name(),
                poseArray,
                stepDto.interpolation(),
                stepDto.speed()
        );
        command.setData(stepData);
        return command;
    }

    /**
     * Helper to create a StepCommandDto for interpolated micro-steps.
     */
    private StepCommandDto createStepCommand(String name, double[] poseArray, double speed) {
        var command = new StepCommandDto();
        var stepData = new StepCommandDto.StepData(
                java.util.Optional.empty(), // No persistent ID for micro-steps
                name,
                poseArray,
                com.sundtrack.robotarm.sequence.model.Interpolation.LINEAR, // Micro-steps are always linear
                speed
        );
        command.setData(stepData);
        return command;
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