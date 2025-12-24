package com.sundtrack.robotarm.motorConfig.dto;

import jakarta.persistence.Embeddable;

/**
 * Represents the PWMCONF (PWM Configuration) register of the TMC2209.
 * These settings fine-tune the behavior of the stealthChop voltage chopper.
 */
@Embeddable
public class PWMCONF {
    // Amplitude offset for stealthChop
    public int pwm_ofs = 30;

    // Amplitude gradient for stealthChop
    public int pwm_grad = 14;

    // PWM frequency selection
    public int pwm_freq = 1; // 0 = 2/1024, 1 = 2/683, 2 = 2/512, 3 = 2/410

    // Enable automatic amplitude scaling
    public boolean pwm_autoscale = true;

    /**
     * Packs the fields into a single integer according to the TMC2209 PWMCONF register layout.
     * @return An integer representing the configured PWMCONF register value.
     */
    public int toInt() {
        return (pwm_ofs & 0xFF)       << 0 |
               (pwm_grad & 0xFF)      << 8 |
               (pwm_freq & 0x03)      << 16 |
               (pwm_autoscale ? 1 : 0) << 18;
    }
}