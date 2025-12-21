package com.sundtrack.robotarm.state;

import org.springframework.stereotype.Service;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

/**
 * A thread-safe service that acts as the single source of truth for the robot's current physical state.
 */
@Service
public class RobotStateService {

    // Assuming a max of 6 motors.
    // 'volatile' ensures that writes from the serial thread are visible to other threads.
    private volatile long[] currentMotorPositions = new long[6];
    private volatile boolean[] motorIsMoving = new boolean[6];
    private final Object lock = new Object();

    /**
     * Updates the current known positions of the motors.
     * This method is intended to be called by the SerialPortService.
     */
    public void updateMotorPositions(long[] newPositions) {
        // System.arraycopy is generally faster for bulk copies.
        System.arraycopy(newPositions, 0, this.currentMotorPositions, 0, Math.min(newPositions.length, this.currentMotorPositions.length));
    }

    /**
     * Gets a copy of the last known motor positions.
     * @return A copy of the motor positions array to prevent external modification.
     */
    public long[] getCurrentMotorPositions() {
        return Arrays.copyOf(this.currentMotorPositions, this.currentMotorPositions.length);
    }

    public void updateMotorMovingState(int motorId, boolean isMoving) {
        synchronized (lock) {
            motorIsMoving[motorId] = isMoving;
            // If this update means all motors are now stopped, notify waiting threads
            if (!isAnyMotorMoving()) {
                lock.notifyAll();
            }
        }
    }

    public boolean getIsMotorMoving(int motorId) {
        synchronized (lock) {
            return motorIsMoving[motorId];
        }
    }

    public void waitForAllMotorsToStop() {
        synchronized (lock) {
            while (isAnyMotorMoving()) {
                try {
                    lock.wait();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
        }
    }

    private boolean isAnyMotorMoving() {
        for (boolean moving : motorIsMoving) {
            if (moving) return true;
        }
        return false;
    }
}