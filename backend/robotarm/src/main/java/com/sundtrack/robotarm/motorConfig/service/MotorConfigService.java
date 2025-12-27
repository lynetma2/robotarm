package com.sundtrack.robotarm.motorConfig.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.sundtrack.robotarm.motorConfig.dto.*;
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
    private final TMC2209RegisterCodec tmc2209RegisterCodec;
    private final ObjectMapper objectMapper;

    @Autowired
    public MotorConfigService(MotorConfigRepository configRepository, SerialPortService serialPortService, TMC2209RegisterCodec tmc2209RegisterCodec, ObjectMapper objectMapper) {
        this.configRepository = configRepository;
        this.serialPortService = serialPortService;
        this.tmc2209RegisterCodec = tmc2209RegisterCodec;
        this.objectMapper = objectMapper;
    }

    @Transactional(readOnly = true)
    public MotorConfig getConfigForMotor(int motorId) {
        // Find existing config or return a new one with default values
        return configRepository.findById(motorId).orElseGet(() -> {
            MotorConfig defaultConfig = new MotorConfig();
            defaultConfig.setId(motorId);
            return defaultConfig;
        });
    }

    @Transactional(readOnly = true)
    public MotorConfigDTO getConfigDto(int motorId) {
        MotorConfig config = getConfigForMotor(motorId);
        return tmc2209RegisterCodec.decode(config);
    }

    // Removed @Transactional to prevent holding DB lock during Serial I/O
    public MotorConfig updateAndSyncConfig(int motorId, MotorConfig updatedConfig) {
        // With an assigned ID, save() performs an "upsert" (insert or update).
        // This is much cleaner than the previous fetch-and-update logic.
        updatedConfig.setId(motorId);
        return configRepository.save(updatedConfig);
    }

    // Removed @Transactional to prevent holding DB lock during Serial I/O
    public MotorConfigDTO updateConfigFromDto(int motorId, MotorConfigDTO dto) {
        MotorConfig config = tmc2209RegisterCodec.encode(dto);
        config.setId(motorId);
        config.setName("Motor " + motorId);
        MotorConfig saved = updateAndSyncConfig(motorId, config);
        return tmc2209RegisterCodec.decode(saved);
    }

//    private void synchronizeWithDevice(MotorConfig config) {
//        try {
//            // 1. Convert the config object to its packed integer representation.
//            TMC2209RegisterPayload payload = TMC2209RegisterPayload.fromConfig(config);
//
//            Map<String, Object> command = new HashMap<>();
//            command.put("type", "TMC2209_CONFIG");
//            command.put("payload", payload);
//            String commandJson = objectMapper.writeValueAsString(command);
//            logger.info("Sending TMC2209 config to device: {}", commandJson);
//            serialPortService.writeToSerial(commandJson);
//        } catch (JsonProcessingException e) {
//            logger.error("Failed to serialize TMC2209 config for motor {}", config.getId(), e);
//        } catch (IllegalStateException e) {
//            logger.warn("Device is not connected. Configuration for motor {} updated in DB only.", config.getId());
//        } catch (Exception e) {
//            logger.error("Failed to synchronize configuration with device for motor {}. Update persisted in DB.", config.getId(), e);
//        }
//    }
}