package com.sundtrack.robotarm.movement.service;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.sundtrack.robotarm.movement.config.MovementConfig;
import com.sundtrack.robotarm.movement.dto.DriveSegmentDTO;
import com.sundtrack.robotarm.movement.dto.JogEventDto;
import com.sundtrack.robotarm.sequence.dto.PoseDto;
import com.sundtrack.robotarm.state.RobotStateService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class MathService {

    private static final Logger logger = LoggerFactory.getLogger(MathService.class);

    private final MovementConfig movementConfig;
    private final RobotStateService robotStateService;

    @Autowired
    public MathService(MovementConfig movementConfig, RobotStateService robotStateService) {
        this.movementConfig = movementConfig;
        this.robotStateService = robotStateService;
    }

    /**
     * Calculates the motor drive segments required to move from a start to an end pose.
     * This is where the robot's inverse kinematics and motion profiling logic resides.
     *
     * @param endPose        The ending pose in Cartesian coordinates.
     * @param requestedSpeed The desired speed for the move, as a percentage (0-100).
     * @return An array of DriveSegmentDTOs, one for each motor.
     */
    public DriveSegmentDTO[] calculateDriveSegments(PoseDto endPose, long requestedSpeed) {
        // Get the CURRENT motor positions from the state service. This is the true starting point.
        long[] startMotorPositionsInSteps = robotStateService.getCurrentMotorPositions();
        logger.info("Found these current motorPositions: {}", startMotorPositionsInSteps);

        // --- 2. Calculate total steps for each motor ---
        long[] totalSteps = new long[3];
        int[] directions = new int[3];
        long[] endMotorPositionsCartesian = endPose.toArray();
        for (int i = 0; i < 3; i++) {
            long delta = coordinateToSteps(endMotorPositionsCartesian[i]) - startMotorPositionsInSteps[i];
            logger.info("Calculated this delta: {}, from this end step: {} and this start step: {}", delta, coordinateToSteps(endMotorPositionsCartesian[i]), startMotorPositionsInSteps[i]);
            directions[i] = delta >= 0 ? 0 : 1;
            totalSteps[i] = Math.abs(delta);
        }

        // --- 3. Motion Profiling (Trapezoidal Speed Profile) ---
        // For simplicity, we'll use a 20% accel, 60% uniform, 20% decel profile.
        DriveSegmentDTO[] segments = new DriveSegmentDTO[2];
        for (int i = 0; i < 2; i++) {
            long accelSteps = (long) (totalSteps[i] * 0.2);
            long decelSteps = (long) (totalSteps[i] * 0.2);
            long uniformSteps = totalSteps[i] - accelSteps - decelSteps;

            // --- 4. Speed/Frequency Calculation ---
            // This is a simplified frequency calculation. A real implementation would be more complex.
            int startFreq = 500;
            int uniformFreq = (int) (movementConfig.getMaxSpeedFrequency() * (requestedSpeed / 100.0));
            int endFreq = 500;

            if (totalSteps[i] == 0) {
                // If a motor doesn't move, create a segment with zero steps.
                segments[i] = new DriveSegmentDTO(i, 0, 0, 0, 0, 0, 0, 0);
            } else {
                segments[i] = new DriveSegmentDTO(
                        i,
                        directions[i],
                        accelSteps,
                        uniformSteps,
                        decelSteps,
                        startFreq,
                        uniformFreq,
                        endFreq
                );
            }
        }
        return segments;
    }

    public String createJogCommand(JogEventDto jogEvent) {
        String command = String.format("JOG M%d D%d S%.2f",
                jogEvent.motorId(),
                jogEvent.direction().getValue(),
                jogEvent.speed()
        );
        logger.info("Generated jog command: {}", command);
        return command;
    }

    public long coordinateToSteps(long steps) {
        return Math.round(steps * 40.0f / 9.0f);
    }

    public long stepsToCoordinate(long coordinate) {
        return Math.round(coordinate * 9.0f / 40.0f);
    }
}