package com.sundtrack.robotarm.sequence.dto;

// Using records for concise, immutable DTOs (requires Java 16+)
public record PoseDto(
    double x,
    double y,
    double z,
    double roll,
    double pitch,
    double yaw
) {}