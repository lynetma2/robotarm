package com.sundtrack.robotarm.motorConfig.dto;

import jakarta.persistence.Embeddable;

/**
 * Represents the CHOPCONF (Chopper Configuration) register of the TMC2209.
 * These settings control the chopper and driver behavior, especially for StealthChop and SpreadCycle.
 */
@Embeddable
public class CHOPCONF {
    public int toff = 3;        // Off time and driver enable
    public int hstrt = 5;       // Hysteresis start
    public int hend = 0;        // Hysteresis end
    public int fd3 = 0;
    public boolean disfdcc = false; // Fast decay comparator disable
    public int chm = 0;         // Chopper mode (0=SpreadCycle, 1=StealthChop)
    public int tbl = 2;         // Blanking time
    public boolean vsense = false;    // Sense resistor voltage scaling
    public int mres = 4;        // Microstep resolution (4=16 microsteps)
    public boolean intpol = true;     // Interpolation to 256 microsteps
    public boolean dedge = false;
    public boolean diss2g = false;    // Short to ground protection disable
    public boolean diss2vs = false;   // Short to VS protection disable

    /**
     * Packs the fields into a single 32-bit integer according to the TMC2209 CHOPCONF register layout.
     * @return A long (to hold the 32-bit unsigned value) representing the configured CHOPCONF register value.
     */
    public long toInt() {
        return (toff & 0xF)          << 0  |
               (hstrt & 0x7)         << 4  |
               (hend & 0xF)          << 7  |
               // Bit 11 is unused
               (disfdcc ? 1L : 0L)   << 12 |
               // Bit 13 is unused
               ((chm > 0) ? 1L : 0L) << 14 |
               (tbl & 0x3)           << 15 |
               (vsense ? 1L : 0L)    << 17 |
               (mres & 0xF)          << 24 |
               (intpol ? 1L : 0L)    << 28 |
               (dedge ? 1L : 0L)     << 29 |
               (diss2g ? 1L : 0L)    << 30 |
               (diss2vs ? 1L : 0L)   << 31;
    }
}