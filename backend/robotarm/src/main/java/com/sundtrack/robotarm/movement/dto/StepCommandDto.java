package com.sundtrack.robotarm.movement.dto;

import com.fasterxml.jackson.annotation.JsonTypeName;
import com.sundtrack.robotarm.sequence.model.Interpolation;

import java.util.Optional;

@JsonTypeName("step") // This matches the "cmd" value in the JSON
public class StepCommandDto extends BaseCommandDto {

    private StepData data;

    public record StepData(
            Optional<Long> id,
            String name,
            double[] pose, // e.g., [x, y, z, roll, pitch, yaw]
            Interpolation interpolation,
            double speed
    ) {}

    public StepData getData() { return data; }
    public void setData(StepData data) { this.data = data; }
}