package com.sundtrack.robotarm.sequence.dto;

import java.time.Instant;
import java.util.List;

// Using a record for the top-level DTO as well
public record SequenceDto(
        Long id,
        String name,
        Instant lastModified,
        List<StepDto> steps,
        SettingsDto settings
) {
}