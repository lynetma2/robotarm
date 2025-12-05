package com.sundtrack.robotarm.tmc2209.dto;

import com.sundtrack.robotarm.tmc2209.model.TMC2209Config;

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
        int tpwmthrs
) {
    public static TMC2209RegisterPayload fromConfig(TMC2209Config config) {
        return new TMC2209RegisterPayload(config.getMotorId(), config.getGconf().toInt(), config.getChopconf().toInt(), config.getIhold_irun().toInt(), config.getPwmconf().toInt(), config.getTpowerdown(), config.getTpwmthrs());
    }
}