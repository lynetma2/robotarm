package com.sundtrack.robotarm.movement.config;

import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

@Component
@ConfigurationProperties(prefix = "robot.movement")
public class MovementConfig {

    /**
     * The maximum speed the robot can move, in units per second.
     */
    private double maxSpeed = 200.0;

    private double maxSpeedFrequency = 10000;

    /**
     * The time in milliseconds between sending interpolated micro-steps.
     * A smaller value results in smoother movement but higher serial traffic.
     */
    private long interpolationStepMillis = 50;

    public double getMaxSpeed() {
        return maxSpeed;
    }

    public void setMaxSpeed(double maxSpeed) {
        this.maxSpeed = maxSpeed;
    }

    public long getInterpolationStepMillis() { return interpolationStepMillis; }

    public void setInterpolationStepMillis(long interpolationStepMillis) { this.interpolationStepMillis = interpolationStepMillis; }

    public double getMaxSpeedFrequency() {
        return maxSpeedFrequency;
    }

    public void setMaxSpeedFrequency(double maxSpeedFrequency) {
        this.maxSpeedFrequency = maxSpeedFrequency;
    }
}