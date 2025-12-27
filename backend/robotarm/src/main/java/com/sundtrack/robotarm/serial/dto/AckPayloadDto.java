package com.sundtrack.robotarm.serial.dto;

public record AckPayloadDto(
    long seq
) implements BasePayloadDto {}