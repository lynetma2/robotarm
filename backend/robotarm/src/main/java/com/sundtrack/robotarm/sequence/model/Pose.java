package com.sundtrack.robotarm.sequence.model;

import jakarta.persistence.Embeddable;

@Embeddable
public class Pose {

    private long x;
    private long y;
    private long z;
    private long roll;
    private long pitch;
    private long yaw;

    // Getters and Setters
    public long getX() { return x; }

    public void setX(long x) { this.x = x; }

    public long getY() { return y; }

    public void setY(long y) { this.y = y; }

    public long getZ() { return z; }

    public void setZ(long z) { this.z = z; }

    public long getRoll() { return roll; }

    public void setRoll(long roll) { this.roll = roll; }

    public long getPitch() { return pitch; }

    public void setPitch(long pitch) { this.pitch = pitch; }

    public long getYaw() { return yaw; }

    public void setYaw(long yaw) { this.yaw = yaw; }
}