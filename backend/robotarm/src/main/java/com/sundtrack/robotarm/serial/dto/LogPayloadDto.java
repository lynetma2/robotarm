package com.sundtrack.robotarm.serial.dto;

import com.fasterxml.jackson.annotation.JsonProperty;

public record LogPayloadDto(
        String level,
        String message,
        @JsonProperty("error_code") int errorCode
) implements BasePayloadDto {}