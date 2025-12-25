package com.sundtrack.robotarm.motorConfig.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
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
}