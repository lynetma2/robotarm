package com.sundtrack.robotarm.motorConfig.controller;

import com.sundtrack.robotarm.motorConfig.service.MotorConfigService;
import com.sundtrack.robotarm.motorConfig.model.MotorConfig;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/v1/config/tmc2209")
public class MotorConfigController {

    private final MotorConfigService configService;

    @Autowired
    public MotorConfigController(MotorConfigService configService) {
        this.configService = configService;
    }

    /**
     * @param motorId The ID of the motor to get the configuration for.
     * @return The TMC2209Config for the specified motor.
     */
    @GetMapping("/{motorId}")
    public ResponseEntity<MotorConfig> getConfig(@PathVariable int motorId) {
        return ResponseEntity.ok(configService.getConfigForMotor(motorId));
    }

    /**
     * @param motorId The ID of the motor to update.
     * @param config  The new configuration object.
     * @return The saved and synchronized TMC2209Config.
     */
    @PutMapping("/{motorId}")
    public ResponseEntity<MotorConfig> updateConfig(@PathVariable int motorId, @RequestBody MotorConfig config) {
        return ResponseEntity.ok(configService.updateAndSyncConfig(motorId, config));
    }
}