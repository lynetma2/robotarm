package com.sundtrack.robotarm.motorConfig.dto;

public record MotorConfigsDTO(
        int id,
        String name,
        boolean enabled,
        int runCurrent,
        int holdCurrent,
        int microsteps,
        int stallThreshold,
        boolean coolStep,
        boolean spreadCycle,
        boolean interpolation,
        boolean pwmAutoScale
) {
}
