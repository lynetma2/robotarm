package com.sundtrack.robotarm.motorConfig.service;

import com.sundtrack.robotarm.motorConfig.dto.MotorConfigDTO;
import com.sundtrack.robotarm.motorConfig.model.MotorConfig;
import org.springframework.stereotype.Service;

@Service
public class TMC2209RegisterCodec {

    // TMC2209 Constants
    private static final float RSENSE = 0.11f; // Typical sense resistor value in Ohms
    private static final int CURRENT_SCALE_MAX = 31; // 5-bit value (0-31)

    /**
     * Encode user-friendly DTO into TMC2209 register format
     */
    public MotorConfig encode(MotorConfigDTO dto) {
        MotorConfig entity = new MotorConfig();

        entity.setId(dto.id() == 0 ? null : dto.id());
        entity.setName(dto.name());
        entity.setEnabled(dto.enabled());

        // Encode IHOLD_IRUN register (0x10)
        entity.setIholdIrun(encodeIholdIrun(dto.runCurrent(), dto.holdCurrent()));

        // Encode CHOPCONF register (0x6C)
        entity.setChopconf(encodeChopconf(dto.microsteps(), dto.interpolation(), dto.spreadCycle()));

        // Encode SGTHRS register (0x40)
        entity.setSgthrs(dto.stallThreshold());

        // Encode COOLCONF register (0x42)
        entity.setCoolconf(encodeCoolconf(dto.coolStep()));

        // Encode TCOOLTHRS register (0x14) - enables CoolStep when velocity exceeds this
        entity.setTcoolthrs(dto.coolStep() ? 0xFFFFF : 0); // Max value enables it at low speeds

        // Encode PWMCONF register (0x70)
        entity.setPwmconf(encodePwmconf(dto.pwmAutoScale()));

        // Encode GCONF register (0x00)
        entity.setGconf(encodeGconf(dto.spreadCycle()));

        return entity;
    }

    /**
     * Decode TMC2209 register format into user-friendly DTO
     */
    public MotorConfigDTO decode(MotorConfig entity) {
        return MotorConfigDTO.builder()
                .id(entity.getId())
                .name(entity.getName())
                .enabled(entity.isEnabled())
                .runCurrent(decodeRunCurrent(entity.getIholdIrun()))
                .holdCurrent(decodeHoldCurrent(entity.getIholdIrun()))
                .microsteps(decodeMicrosteps(entity.getChopconf()))
                .stallThreshold(entity.getSgthrs())
                .coolStep(entity.getTcoolthrs() > 0)
                .spreadCycle(decodeSpreadCycle(entity.getChopconf()))
                .interpolation(decodeInterpolation(entity.getChopconf()))
                .pwmAutoScale(decodePwmAutoScale(entity.getPwmconf()))
                .build();
    }

    // ==================== ENCODING METHODS ====================

    private int encodeIholdIrun(int runCurrentMa, int holdCurrentMa) {
        int irun = milliampsToCurrentScale(runCurrentMa);
        int ihold = milliampsToCurrentScale(holdCurrentMa);
        int iholddelay = 10; // Standard delay value

        // IHOLD_IRUN format: [23:20]=IHOLDDELAY, [12:8]=IRUN, [4:0]=IHOLD
        return (iholddelay << 16) | (irun << 8) | ihold;
    }

    private int encodeChopconf(int microsteps, boolean interpolation, boolean spreadCycle) {
        int mres = microstepsToMres(microsteps);
        int intpol = interpolation ? 1 : 0;
        int tbl = 2; // Blank time select (default)
        int toff = spreadCycle ? 3 : 5; // Off time (3 for SpreadCycle, 5 for StealthChop)

        // CHOPCONF format bits we care about:
        // [28]=intpol, [27:24]=mres, [16:15]=tbl, [3:0]=toff
        return (intpol << 28) | (mres << 24) | (tbl << 15) | toff;
    }

    private int encodeCoolconf(boolean coolStepEnabled) {
        if (!coolStepEnabled) {
            return 0;
        }
        // Default CoolStep configuration
        // semin=5, semax=2, sedn=1
        int semin = 5;
        int semax = 2;
        int sedn = 1;

        return (semax << 8) | (semin << 0) | (sedn << 13);
    }

    private int encodePwmconf(boolean pwmAutoScale) {
        // Default PWMCONF with configurable autoscale
        int pwmOfs = 36;  // PWM offset
        int pwmGrad = 14; // PWM gradient
        int pwmFreq = 0;  // PWM frequency
        int pwmAutoscale = pwmAutoScale ? 1 : 0;
        int pwmAutograd = pwmAutoScale ? 1 : 0;
        int freewheel = 0; // Normal operation
        int pwmReg = 4;    // Regulation loop gradient
        int pwmLim = 12;   // PWM limit

        return (pwmLim << 28) | (pwmReg << 24) | (freewheel << 20) |
                (pwmAutograd << 19) | (pwmAutoscale << 18) | (pwmFreq << 16) |
                (pwmGrad << 8) | pwmOfs;
    }

    private int encodeGconf(boolean spreadCycle) {
        // GCONF bits: spread_en, shaft, etc.
        int shaft = 0; // Direction
        int spreadEn = spreadCycle ? 1 : 0;

        return (spreadEn << 2) | shaft;
    }

    // ==================== DECODING METHODS ====================

    private int decodeRunCurrent(int iholdIrun) {
        int irun = (iholdIrun >> 8) & 0x1F;
        return currentScaleToMilliamps(irun);
    }

    private int decodeHoldCurrent(int iholdIrun) {
        int ihold = iholdIrun & 0x1F;
        return currentScaleToMilliamps(ihold);
    }

    private int decodeMicrosteps(int chopconf) {
        int mres = (chopconf >> 24) & 0x0F;
        return mresToMicrosteps(mres);
    }

    private boolean decodeSpreadCycle(int chopconf) {
        int toff = chopconf & 0x0F;
        // SpreadCycle typically uses toff=3, StealthChop uses toff=5
        return toff <= 3;
    }

    private boolean decodeInterpolation(int chopconf) {
        return ((chopconf >> 28) & 0x01) == 1;
    }

    private boolean decodePwmAutoScale(int pwmconf) {
        return ((pwmconf >> 18) & 0x01) == 1;
    }

    // ==================== HELPER METHODS ====================

    /**
     * Convert milliamps to TMC2209 current scale (0-31)
     * Formula: CS = (I_RMS * 32 * sqrt(2) * R_SENSE) / V_FS - 1
     * Where V_FS = 0.325V for TMC2209
     */
    private int milliampsToCurrentScale(int milliamps) {
        float amps = milliamps / 1000.0f;
        float vfs = 0.325f;
        float cs = (amps * 32 * 1.41421f * RSENSE / vfs) - 1;

        int scale = Math.round(cs);
        return Math.max(0, Math.min(CURRENT_SCALE_MAX, scale));
    }

    /**
     * Convert TMC2209 current scale (0-31) to milliamps
     */
    private int currentScaleToMilliamps(int scale) {
        float vfs = 0.325f;
        float amps = ((scale + 1) * vfs) / (32 * 1.41421f * RSENSE);
        return Math.round(amps * 1000);
    }

    /**
     * Convert microsteps to MRES register value
     */
    private int microstepsToMres(int microsteps) {
        return switch (microsteps) {
            case 256 -> 0;
            case 128 -> 1;
            case 64 -> 2;
            case 32 -> 3;
            case 16 -> 4;
            case 8 -> 5;
            case 4 -> 6;
            case 2 -> 7;
            default -> 8; // Full step
        };
    }

    /**
     * Convert MRES register value to microsteps
     */
    private int mresToMicrosteps(int mres) {
        return switch (mres) {
            case 0 -> 256;
            case 1 -> 128;
            case 2 -> 64;
            case 3 -> 32;
            case 4 -> 16;
            case 5 -> 8;
            case 6 -> 4;
            case 7 -> 2;
            default -> 1; // Full step
        };
    }
}