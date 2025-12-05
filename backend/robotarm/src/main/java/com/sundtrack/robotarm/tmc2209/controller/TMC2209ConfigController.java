package com.sundtrack.robotarm.tmc2209.controller;

import com.sundtrack.robotarm.tmc2209.service.TMC2209ConfigService;
import com.sundtrack.robotarm.tmc2209.model.TMC2209Config;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.messaging.handler.annotation.DestinationVariable;
import org.springframework.messaging.handler.annotation.MessageMapping;
import org.springframework.messaging.handler.annotation.SendTo;
import org.springframework.stereotype.Controller;

@Controller
public class TMC2209ConfigController {

    private final TMC2209ConfigService configService;

    @Autowired
    public TMC2209ConfigController(TMC2209ConfigService configService) {
        this.configService = configService;
    }

    /**
     * Handles a request to fetch the TMC2209 configuration for a specific motor.
     * The client should send a message to /app/config/tmc2209/get/{motorId}.
     * The result is broadcast to all subscribers of /topic/config/tmc2209/{motorId}.
     *
     * @param motorId The ID of the motor to get the configuration for.
     * @return The TMC2209Config for the specified motor.
     */
    @MessageMapping("/config/tmc2209/get/{motorId}")
    @SendTo("/topic/config/tmc2209/{motorId}")
    public TMC2209Config getConfig(@DestinationVariable int motorId) {
        return configService.getConfigForMotor(motorId);
    }

    /**
     * Handles a request to update and synchronize the TMC2209 configuration for a motor.
     * The client should send a TMC2209Config payload to /app/config/tmc2209/update/{motorId}.
     * The updated configuration is broadcast to all subscribers of /topic/config/tmc2209/{motorId}.
     *
     * @param motorId The ID of the motor to update.
     * @param config  The new configuration object.
     * @return The saved and synchronized TMC2209Config.
     */
    @MessageMapping("/config/tmc2209/update/{motorId}")
    @SendTo("/topic/config/tmc2209/{motorId}")
    public TMC2209Config updateConfig(@DestinationVariable int motorId, TMC2209Config config) {
        return configService.updateAndSyncConfig(motorId, config);
    }
}