package com.sundtrack.robotarm.serial.dto;

import com.fasterxml.jackson.annotation.JsonSubTypes;
import com.fasterxml.jackson.annotation.JsonTypeInfo;

@JsonTypeInfo(
        use = JsonTypeInfo.Id.NAME,
        include = JsonTypeInfo.As.EXTERNAL_PROPERTY,
        property = "type"
)
@JsonSubTypes({
        @JsonSubTypes.Type(value = TelemetryPayloadDto.class, name = "telemetry"),
        @JsonSubTypes.Type(value = LogPayloadDto.class, name = "log"),
        @JsonSubTypes.Type(value = MotorMovingPayloadDto.class, name = "motormoving"),
        @JsonSubTypes.Type(value = AckPayloadDto.class, name = "ack")
})
public interface BasePayloadDto {}