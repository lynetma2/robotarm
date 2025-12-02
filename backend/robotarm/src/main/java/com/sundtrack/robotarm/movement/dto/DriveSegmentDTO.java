package com.sundtrack.robotarm.movement.dto;

import com.fasterxml.jackson.annotation.JsonProperty;

public record DriveSegmentDTO(
    @JsonProperty("motor_id") int motorId,
    int direction,
    @JsonProperty("accel_steps") long accelSteps,
    @JsonProperty("uniform_steps") long uniformSteps,
    @JsonProperty("decel_steps") long decelSteps,
    @JsonProperty("start_freq") int startFreq,
    @JsonProperty("uniform_freq") int uniformFreq,
    @JsonProperty("end_freq") int endFreq
) {
}