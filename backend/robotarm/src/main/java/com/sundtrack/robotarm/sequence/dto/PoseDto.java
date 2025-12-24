package com.sundtrack.robotarm.sequence.dto;

// Using records for concise, immutable DTOs (requires Java 16+)
public record PoseDto(
    long x,
    long y,
    long z,
    long roll,
    long pitch,
    long yaw
) {
    public long[] toArray() {
        return new long[]{x, y, z, roll, pitch, yaw};
    }
}