package com.sundtrack.robotarm.motorConfig.dto;

public record MotorConfigDTO(
        int id,
        String name,
        boolean enabled,
        int runCurrent,
        int holdCurrent,
        int microsteps,
        int stallThreshold,
        boolean coolStep,
        boolean spreadCycle,
        boolean interpolation,
        boolean pwmAutoScale
) {

    public static Builder builder() {
        return new Builder();
    }

    public static class Builder {
        private int id;
        private String name;
        private boolean enabled;
        private int runCurrent;
        private int holdCurrent;
        private int microsteps;
        private int stallThreshold;
        private boolean coolStep;
        private boolean spreadCycle;
        private boolean interpolation;
        private boolean pwmAutoScale;

        public Builder id(int id) {
            this.id = id;
            return this;
        }

        public Builder name(String name) {
            this.name = name;
            return this;
        }

        public Builder enabled(boolean enabled) {
            this.enabled = enabled;
            return this;
        }

        public Builder runCurrent(int runCurrent) {
            this.runCurrent = runCurrent;
            return this;
        }

        public Builder holdCurrent(int holdCurrent) {
            this.holdCurrent = holdCurrent;
            return this;
        }

        public Builder microsteps(int microsteps) {
            this.microsteps = microsteps;
            return this;
        }

        public Builder stallThreshold(int stallThreshold) {
            this.stallThreshold = stallThreshold;
            return this;
        }

        public Builder coolStep(boolean coolStep) {
            this.coolStep = coolStep;
            return this;
        }

        public Builder spreadCycle(boolean spreadCycle) {
            this.spreadCycle = spreadCycle;
            return this;
        }

        public Builder interpolation(boolean interpolation) {
            this.interpolation = interpolation;
            return this;
        }

        public Builder pwmAutoScale(boolean pwmAutoScale) {
            this.pwmAutoScale = pwmAutoScale;
            return this;
        }

        public MotorConfigDTO build() {
            return new MotorConfigDTO(
                    id,
                    name,
                    enabled,
                    runCurrent,
                    holdCurrent,
                    microsteps,
                    stallThreshold,
                    coolStep,
                    spreadCycle,
                    interpolation,
                    pwmAutoScale
            );
        }
    }
}
