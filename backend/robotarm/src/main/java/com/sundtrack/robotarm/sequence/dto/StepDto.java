package com.sundtrack.robotarm.sequence.dto;

import com.sundtrack.robotarm.sequence.model.Interpolation;

public record StepDto(
        Long id,
        String name,
        PoseDto pose,
        Interpolation interpolation,
        double speed
) {
}