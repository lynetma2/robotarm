package com.sundtrack.robotarm.serial.dto;

import com.fasterxml.jackson.annotation.JsonProperty;

public record MotorMovingPayloadDto(
        @JsonProperty("motorId") int motorId,
        @JsonProperty("isMoving") boolean isMoving
) implements BasePayloadDto {}