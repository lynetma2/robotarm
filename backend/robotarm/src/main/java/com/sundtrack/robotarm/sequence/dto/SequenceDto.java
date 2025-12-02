package com.sundtrack.robotarm.sequence.dto;

import java.time.Instant;
import java.util.List;
import java.util.Optional;

// Using a record for the top-level DTO as well
public record SequenceDto(
        Optional<Long> id,
        String title,
        String description,
        Instant lastModified,
        List<StepDto> steps,
        SettingsDto settings
) {
}