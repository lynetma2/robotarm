package com.sundtrack.robotarm.tmc2209.controller;

import com.sundtrack.robotarm.tmc2209.service.TMC2209ConfigService;
import com.sundtrack.robotarm.tmc2209.model.TMC2209Config;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/v1/config/tmc2209")
public class TMC2209ConfigController {

    private final TMC2209ConfigService configService;

    @Autowired
    public TMC2209ConfigController(TMC2209ConfigService configService) {
        this.configService = configService;
    }

    @GetMapping("/{motorId}")
    public ResponseEntity<TMC2209Config> getConfig(@PathVariable int motorId) {
        return ResponseEntity.ok(configService.getConfigForMotor(motorId));
    }

    @PutMapping("/{motorId}")
    public ResponseEntity<TMC2209Config> updateConfig(@PathVariable int motorId, @RequestBody TMC2209Config config) {
        TMC2209Config updatedConfig = configService.updateAndSyncConfig(motorId, config);
        return ResponseEntity.ok(updatedConfig);
    }
}