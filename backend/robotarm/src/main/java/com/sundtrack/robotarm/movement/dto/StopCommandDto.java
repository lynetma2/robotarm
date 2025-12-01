package com.sundtrack.robotarm.movement.dto;

import com.fasterxml.jackson.annotation.JsonTypeName;

@JsonTypeName("stop") // This matches the "cmd" value in the JSON
public class StopCommandDto extends BaseCommandDto {

    private StopData data;

    public enum StopType {
        HARD, SOFT
    }

    public record StopData(
            StopType type,
            int motorId // -1 for all motors
    ) {}

    public StopData getData() { return data; }
    public void setData(StopData data) { this.data = data; }
}