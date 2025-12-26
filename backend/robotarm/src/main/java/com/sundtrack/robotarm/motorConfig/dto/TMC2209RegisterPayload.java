package com.sundtrack.robotarm.motorConfig.dto;

import com.sundtrack.robotarm.motorConfig.model.MotorConfig;

/**
 * A DTO representing the packed, integer values of the TMC2209 registers.
 * This is the format that will be serialized and sent to the microcontroller.
 */
public record TMC2209RegisterPayload(
        int motorId,
        int gconf,
        long chopconf,
        int ihold_irun,
        int pwmconf,
        int tpowerdown,
        int tpwmthrs,
        int sgthrs,
        int tcoolthrs
) {
    public static TMC2209RegisterPayload fromConfig(MotorConfig config) {
        return new TMC2209RegisterPayload(config.getMotorId(), config.getGconf(), config.getChopconf(), config.getIhold_irun(), config.getPwmconf(), config.getTpowerdown(), config.getTpwmthrs(), config.getSgthrs(), config.getTcoolthrs());
    }
}