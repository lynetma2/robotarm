package com.sundtrack.robotarm.sequence.dto;

import com.sundtrack.robotarm.sequence.model.Interpolation;

import java.util.Optional;

public record StepDto(
        Optional<Long> id,
        String name,
        PoseDto pose,
        Interpolation interpolation,
        double speed
) {
}