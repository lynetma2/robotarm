package com.sundtrack.robotarm.sequence.model;

import jakarta.persistence.Embeddable;

@Embeddable
public class Pose {

    private double x;
    private double y;
    private double z;
    private double roll;
    private double pitch;
    private double yaw;

    // Getters and Setters
    public double getX() { return x; }

    public void setX(double x) { this.x = x; }

    public double getY() { return y; }

    public void setY(double y) { this.y = y; }

    public double getZ() { return z; }

    public void setZ(double z) { this.z = z; }

    public double getRoll() { return roll; }

    public void setRoll(double roll) { this.roll = roll; }

    public double getPitch() { return pitch; }

    public void setPitch(double pitch) { this.pitch = pitch; }

    public double getYaw() { return yaw; }

    public void setYaw(double yaw) { this.yaw = yaw; }
}