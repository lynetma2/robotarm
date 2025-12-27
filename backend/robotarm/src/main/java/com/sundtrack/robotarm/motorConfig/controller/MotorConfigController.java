package com.sundtrack.robotarm.motorConfig.controller;

import com.sundtrack.robotarm.motorConfig.dto.MotorConfigDTO;
import com.sundtrack.robotarm.motorConfig.service.MotorConfigService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/v1/config/tmc2209")
public class MotorConfigController {

    private final MotorConfigService configService;
    private static final Logger logger = LoggerFactory.getLogger(MotorConfigController.class);


    @Autowired
    public MotorConfigController(MotorConfigService configService) {
        this.configService = configService;
    }

    /**
     * @param motorId The ID of the motor to get the configuration for.
     * @return The TMC2209Config for the specified motor.
     */
    @GetMapping("/{motorId}")
    public ResponseEntity<MotorConfigDTO> getConfig(@PathVariable int motorId) {
        return ResponseEntity.ok(configService.getConfigDto(motorId));
    }

    /**
     * @param motorId The ID of the motor to update.
     * @param configDto  The new configuration object.
     * @return The saved and synchronized TMC2209Config.
     */
    @PutMapping("/{motorId}")
    public ResponseEntity<MotorConfigDTO> updateConfig(@PathVariable int motorId, @RequestBody MotorConfigDTO configDto) {
        logger.info("Received request to update config for motor {}, with DTO {}", motorId, configDto);
        return ResponseEntity.ok(configService.updateConfigFromDto(motorId, configDto));
    }
}