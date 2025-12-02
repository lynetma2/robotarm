package com.sundtrack.robotarm.movement.dto;

import com.fasterxml.jackson.annotation.JsonTypeName;

@JsonTypeName("jog") // This matches the "cmd" value in the JSON
public class JogCommandDto extends BaseCommandDto {

    private JogData data;

    public record JogData(
            int motorId,
            double speed,
            int direction // 1 for clockwise, -1 for counter-clockwise
    ) {}

    public JogData getData() { return data; }
    public void setData(JogData data) { this.data = data; }
}