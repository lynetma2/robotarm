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
}