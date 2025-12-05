package com.sundtrack.robotarm.tmc2209.dto;

import jakarta.persistence.Embeddable;

/**
 * Represents the GCONF (Global Configuration) register of the TMC2209.
 * Booleans are used for single-bit flags.
 */
@Embeddable
public class GCONF {
    public boolean i_scale_analog = false;
    public boolean internal_rsense = false; // Default to false for external sense resistors
    public boolean en_spreadcycle = true;   // Default to StealthChop disabled
    public boolean shaft = false;
    public boolean index_otpw = false;
    public boolean index_step = false;
    public boolean pdn_disable = true;      // Use UART for standstill current control
    public boolean mstep_reg_select = true; // Use MSTEP register for microstepping
    public boolean multistep_filt = true;
    public boolean test_mode = false;

    /**
     * Packs the boolean flags into a single integer according to the TMC2209 GCONF register layout.
     * @return An integer representing the configured GCONF register value.
     */
    public int toInt() {
        return (i_scale_analog   ? 1 : 0) << 0 |
               (internal_rsense  ? 1 : 0) << 1 |
               (en_spreadcycle   ? 1 : 0) << 2 |
               (shaft            ? 1 : 0) << 3 |
               (index_otpw       ? 1 : 0) << 4 |
               (index_step       ? 1 : 0) << 5 |
               (pdn_disable      ? 1 : 0) << 6 |
               (mstep_reg_select ? 1 : 0) << 7 |
               (multistep_filt   ? 1 : 0) << 8 |
               (test_mode        ? 1 : 0) << 9;
    }
}