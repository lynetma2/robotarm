package com.sundtrack.robotarm.movement.dto;

import com.fasterxml.jackson.annotation.JsonSubTypes;
import com.fasterxml.jackson.annotation.JsonTypeInfo;

/**
 * This is the base class for all commands sent from the frontend.
 * The annotations enable polymorphic deserialization, meaning Jackson will
 * automatically create the correct subclass (e.g., StepCommandDto, JogCommandDto)
 * based on the value of the "cmd" property in the incoming JSON.
 */
@JsonTypeInfo(
        use = JsonTypeInfo.Id.NAME,
        include = JsonTypeInfo.As.PROPERTY,
        property = "cmd"
)
@JsonSubTypes({
        @JsonSubTypes.Type(value = StepCommandDto.class, name = "step"),
        @JsonSubTypes.Type(value = JogCommandDto.class, name = "jog"),
        @JsonSubTypes.Type(value = StopCommandDto.class, name = "stop")
})
public abstract class BaseCommandDto {
}