package com.sundtrack.robotarm.movement.service;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.sundtrack.robotarm.movement.config.MovementConfig;
import com.sundtrack.robotarm.movement.dto.DriveSegmentDTO;
import com.sundtrack.robotarm.movement.dto.JogEventDto;
import com.sundtrack.robotarm.sequence.dto.PoseDto;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class MathService {

    private static final Logger logger = LoggerFactory.getLogger(MathService.class);

    private final ObjectMapper objectMapper;
    private final MovementConfig movementConfig;

    @Autowired
    public MathService(ObjectMapper objectMapper, MovementConfig movementConfig) {
        this.objectMapper = objectMapper;
        this.movementConfig = movementConfig;
    }

    /**
     * Calculates the motor drive segments required to move from a start to an end pose.
     * This is where the robot's inverse kinematics and motion profiling logic resides.
     *
     * @param startPose      The starting pose in Cartesian coordinates.
     * @param endPose        The ending pose in Cartesian coordinates.
     * @param requestedSpeed The desired speed for the move, as a percentage (0-100).
     * @return An array of DriveSegmentDTOs, one for each motor.
     */
    public DriveSegmentDTO[] calculateDriveSegments(PoseDto startPose, PoseDto endPose, double requestedSpeed) {
        // --- 1. Inverse Kinematics: Convert Cartesian poses to motor positions (angles/steps) ---
        // This is a placeholder for your robot's specific inverse kinematics.
        // For this example, we'll assume a simple mapping:
        // X -> motor 0, Y -> motor 1, Z -> motor 2
        double[] startMotorPositions = {startPose.x(), startPose.y(), startPose.z()};
        double[] endMotorPositions = {endPose.x(), endPose.y(), endPose.z()};

        // --- 2. Calculate total steps for each motor ---
        long[] totalSteps = new long[3];
        int[] directions = new int[3];
        for (int i = 0; i < 3; i++) {
            double delta = endMotorPositions[i] - startMotorPositions[i];
            directions[i] = delta >= 0 ? 1 : -1;
            // This is where you'd convert units (e.g., mm or degrees) to motor steps
            totalSteps[i] = Math.round(Math.abs(delta) * 40/9); // Example: 1 degree = 1600/360 steps
        }

        // --- 3. Motion Profiling (Trapezoidal Speed Profile) ---
        // For simplicity, we'll use a 20% accel, 60% uniform, 20% decel profile.
        DriveSegmentDTO[] segments = new DriveSegmentDTO[3];
        for (int i = 0; i < 3; i++) {
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
}