package com.sundtrack.robotarm.movement.dto;

public record JogEventDto(
    int motorId,
    DirectionEnum direction,
    double speed
) {
}