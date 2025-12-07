package com.sundtrack.robotarm.serial.dto;

import com.fasterxml.jackson.annotation.JsonProperty;

public record TelemetryPayloadDto(
        @JsonProperty("motor_positions") long[] motorPositions,
        @JsonProperty("num_motors") int numMotors
) implements BasePayloadDto {}