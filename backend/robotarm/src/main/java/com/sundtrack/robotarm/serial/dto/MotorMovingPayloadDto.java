package com.sundtrack.robotarm.serial.dto;

import com.fasterxml.jackson.annotation.JsonProperty;

public record MotorMovingPayloadDto(
    @JsonProperty("motor_id") int motorId,
    @JsonProperty("is_moving") boolean isMoving
) implements BasePayloadDto {}