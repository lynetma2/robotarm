package com.sundtrack.robotarm.motorConfig.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.sundtrack.robotarm.motorConfig.dto.*;
import com.sundtrack.robotarm.motorConfig.dto.TMC2209RegisterPayload;
import com.sundtrack.robotarm.serial.service.SerialPortService;
import com.sundtrack.robotarm.motorConfig.model.MotorConfig;
import com.sundtrack.robotarm.motorConfig.repository.MotorConfigRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.HashMap;
import java.util.Map;

@Service
public class MotorConfigService {

    private static final Logger logger = LoggerFactory.getLogger(MotorConfigService.class);

    private final MotorConfigRepository configRepository;
    private final SerialPortService serialPortService;
    private final ObjectMapper objectMapper;

    @Autowired
    public MotorConfigService(MotorConfigRepository configRepository, SerialPortService serialPortService, ObjectMapper objectMapper) {
        this.configRepository = configRepository;
        this.serialPortService = serialPortService;
        this.objectMapper = objectMapper;
    }

    @Transactional(readOnly = true)
    public MotorConfig getConfigForMotor(int motorId) {
        // Find existing config or return a new one with default values
        return configRepository.findById(motorId).orElseGet(() -> {
            MotorConfig defaultConfig = new MotorConfig();
            defaultConfig.setMotorId(motorId);
            return defaultConfig;
        });
    }

    @Transactional
    public MotorConfig updateAndSyncConfig(int motorId, MotorConfig updatedConfig) {
        updatedConfig.setMotorId(motorId);
        MotorConfig savedConfig = configRepository.save(updatedConfig);
        synchronizeWithDevice(savedConfig);
        return savedConfig;
    }

    private void synchronizeWithDevice(MotorConfig config) {
        try {
            // 1. Convert the config object to its packed integer representation.
            TMC2209RegisterPayload payload = TMC2209RegisterPayload.fromConfig(config);

            Map<String, Object> command = new HashMap<>();
            command.put("type", "TMC2209_CONFIG");
            command.put("payload", payload);
            String commandJson = objectMapper.writeValueAsString(command);
            logger.info("Sending TMC2209 config to device: {}", commandJson);
            serialPortService.writeToSerial(commandJson);
        } catch (JsonProcessingException e) {
            logger.error("Failed to serialize TMC2209 config for motor {}", config.getMotorId(), e);
        }
    }

    /**
     * Translates a high-level MotorConfigsDTO into the low-level MotorConfig model
     * with packed register values suitable for the TMC2209.
     *
     * @param dto The DTO containing user-friendly settings.
     * @return A MotorConfig entity with calculated register values.
     */
    public MotorConfig mapDtoToConfig(MotorConfigsDTO dto) {
        MotorConfig config = new MotorConfig();
        config.setMotorId(dto.id());

        // 1. GCONF: Handle SpreadCycle vs StealthChop
        GCONF gconf = new GCONF();
        gconf.en_spreadcycle = dto.spreadCycle();
        // Preserve other defaults from GCONF class (pdn_disable=true, mstep_reg_select=true, etc.)
        config.setGconf(encodeGCONF(gconf));

        // 2. CHOPCONF: Microsteps, Interpolation, Enable/Disable (via TOFF)
        CHOPCONF chopconf = new CHOPCONF();
        chopconf.intpol = dto.interpolation();
        chopconf.mres = calculateMres(dto.microsteps());
        // Setting TOFF to 0 disables the driver. Default is 3.
        chopconf.toff = dto.enabled() ? 3 : 0;
        config.setChopconf(encodeCHOPCONF(chopconf));

        // 3. IHOLD_IRUN: Currents (0-31)
        IHOLD_IRUN ihold_irun = new IHOLD_IRUN();
        ihold_irun.irun = Math.min(31, Math.max(0, dto.runCurrent()));
        ihold_irun.ihold = Math.min(31, Math.max(0, dto.holdCurrent()));
        config.setIhold_irun(encodeIHOLD_IRUN(ihold_irun));

        // 4. PWMCONF: AutoScale
        PWMCONF pwmconf = new PWMCONF();
        pwmconf.pwm_autoscale = dto.pwmAutoScale();
        config.setPwmconf(encodePWMCONF(pwmconf));

        // 5. Direct Register Mappings
        config.setSgthrs(dto.stallThreshold());

        // CoolStep: If enabled, set a default threshold (e.g., 2000), otherwise 0.
        // TCOOLTHRS needs to be < VACTUAL for CoolStep to activate.
        config.setTcoolthrs(dto.coolStep() ? 2000 : 0);

        return config;
    }

    private int calculateMres(int microsteps) {
        // mres = 8 - log2(microsteps). 256 -> 0, 16 -> 4, 1 -> 8.
        if (microsteps <= 0) return 4; // Default to 16 microsteps if invalid
        return 8 - (int) (Math.log(microsteps) / Math.log(2));
    }

    // --- Encoding and Decoding Methods ---

    public GCONF decodeGCONF(int val) {
        GCONF g = new GCONF();
        g.i_scale_analog = (val & 1) != 0;
        g.internal_rsense = (val & (1 << 1)) != 0;
        g.en_spreadcycle = (val & (1 << 2)) != 0;
        g.shaft = (val & (1 << 3)) != 0;
        g.index_otpw = (val & (1 << 4)) != 0;
        g.index_step = (val & (1 << 5)) != 0;
        g.pdn_disable = (val & (1 << 6)) != 0;
        g.mstep_reg_select = (val & (1 << 7)) != 0;
        g.multistep_filt = (val & (1 << 8)) != 0;
        g.test_mode = (val & (1 << 9)) != 0;
        return g;
    }

    public int encodeGCONF(GCONF g) {
        return g.toInt();
    }

    public CHOPCONF decodeCHOPCONF(long val) {
        CHOPCONF c = new CHOPCONF();
        c.toff = (int) (val & 0xF);
        c.hstrt = (int) ((val >> 4) & 0x7);
        c.hend = (int) ((val >> 7) & 0xF);
        // Bit 11 is unused/reserved in standard TMC2209 map
        c.disfdcc = (val & (1L << 12)) != 0;
        // Bit 13 is unused
        c.chm = (int) ((val >> 14) & 1);
        c.tbl = (int) ((val >> 15) & 0x3);
        c.vsense = (val & (1L << 17)) != 0;
        c.mres = (int) ((val >> 24) & 0xF);
        c.intpol = (val & (1L << 28)) != 0;
        c.dedge = (val & (1L << 29)) != 0;
        c.diss2g = (val & (1L << 30)) != 0;
        c.diss2vs = (val & (1L << 31)) != 0;
        return c;
    }

    public long encodeCHOPCONF(CHOPCONF c) {
        return c.toInt();
    }

    public IHOLD_IRUN decodeIHOLD_IRUN(int val) {
        IHOLD_IRUN i = new IHOLD_IRUN();
        i.ihold = val & 0x1F;
        i.irun = (val >> 8) & 0x1F;
        i.iholddelay = (val >> 16) & 0x0F;
        return i;
    }

    public int encodeIHOLD_IRUN(IHOLD_IRUN i) {
        return i.toInt();
    }

    public PWMCONF decodePWMCONF(int val) {
        PWMCONF p = new PWMCONF();
        p.pwm_ofs = val & 0xFF;
        p.pwm_grad = (val >> 8) & 0xFF;
        p.pwm_freq = (val >> 16) & 0x03;
        p.pwm_autoscale = (val & (1 << 18)) != 0;
        return p;
    }

    public int encodePWMCONF(PWMCONF p) {
        return p.toInt();
    }
}