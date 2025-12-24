package com.sundtrack.robotarm.motorConfig.dto;

import jakarta.persistence.Embeddable;

/**
 * Represents the IHOLD_IRUN register of the TMC2209.
 * This register controls the motor current settings.
 * Values range from 0 to 31.
 */
@Embeddable
public class IHOLD_IRUN {
    // Current used during standstill
    public int ihold = 16; // Default to half of max

    // Current used during motion
    public int irun = 31; // Default to max

    // Delay before current reduction to ihold
    public int iholddelay = 8; // Default to a moderate delay

    /**
     * Packs the fields into a single integer according to the TMC2209 IHOLD_IRUN register layout.
     * @return An integer representing the configured IHOLD_IRUN register value.
     */
    public int toInt() {
        return (ihold & 0x1F)      << 0 |
               (irun & 0x1F)       << 8 |
               (iholddelay & 0x0F) << 16;
    }
}