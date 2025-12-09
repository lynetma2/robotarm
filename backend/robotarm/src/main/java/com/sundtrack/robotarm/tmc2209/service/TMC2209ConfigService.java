package com.sundtrack.robotarm.tmc2209.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.sundtrack.robotarm.tmc2209.dto.TMC2209RegisterPayload;
import com.sundtrack.robotarm.serial.service.SerialPortService;
import com.sundtrack.robotarm.tmc2209.model.TMC2209Config;
import com.sundtrack.robotarm.tmc2209.repository.TMC2209ConfigRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.HashMap;
import java.util.Map;

@Service
public class TMC2209ConfigService {

    private static final Logger logger = LoggerFactory.getLogger(TMC2209ConfigService.class);

    private final TMC2209ConfigRepository configRepository;
    private final SerialPortService serialPortService;
    private final ObjectMapper objectMapper;

    @Autowired
    public TMC2209ConfigService(TMC2209ConfigRepository configRepository, SerialPortService serialPortService, ObjectMapper objectMapper) {
        this.configRepository = configRepository;
        this.serialPortService = serialPortService;
        this.objectMapper = objectMapper;
    }

    @Transactional(readOnly = true)
    public TMC2209Config getConfigForMotor(int motorId) {
        // Find existing config or return a new one with default values
        return configRepository.findById(motorId).orElseGet(() -> {
            TMC2209Config defaultConfig = new TMC2209Config();
            defaultConfig.setMotorId(motorId);
            return defaultConfig;
        });
    }

    @Transactional
    public TMC2209Config updateAndSyncConfig(int motorId, TMC2209Config updatedConfig) {
        updatedConfig.setMotorId(motorId);
        TMC2209Config savedConfig = configRepository.save(updatedConfig);
        synchronizeWithDevice(savedConfig);
        return savedConfig;
    }

    private void synchronizeWithDevice(TMC2209Config config) {
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
}