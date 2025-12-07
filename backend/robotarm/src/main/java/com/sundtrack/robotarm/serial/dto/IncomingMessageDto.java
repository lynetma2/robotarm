package com.sundtrack.robotarm.serial.dto;

import com.fasterxml.jackson.annotation.JsonTypeId;

public record IncomingMessageDto(
        long timestamp,
        String source,
        // This will be polymorphically deserialized into either TelemetryPayloadDto or LogPayloadDto
        BasePayloadDto data
) {
}