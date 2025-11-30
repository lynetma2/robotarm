package com.sundtrack.robotarm.sequence.service;

import com.sundtrack.robotarm.sequence.dto.*;
import com.sundtrack.robotarm.sequence.model.*;
import com.sundtrack.robotarm.sequence.repository.SequenceRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
import java.util.stream.Collectors;

@Service
public class SequenceService {

    private final SequenceRepository sequenceRepository;

    @Autowired
    public SequenceService(SequenceRepository sequenceRepository) {
        this.sequenceRepository = sequenceRepository;
    }

    @Transactional
    public SequenceDto createSequence(SequenceDto sequenceDto) {
        Sequence sequence = toEntity(sequenceDto);
        Sequence savedSequence = sequenceRepository.save(sequence);
        return toDto(savedSequence);
    }

    @Transactional(readOnly = true)
    public List<SequenceDto> getAllSequences() {
        return sequenceRepository.findAll()
                .stream()
                .map(this::toDto)
                .collect(Collectors.toList());
    }

    @Transactional(readOnly = true)
    public SequenceDto getSequenceById(Long id) {
        return sequenceRepository.findById(id)
                .map(this::toDto)
                .orElseThrow(() -> new RuntimeException("Sequence not found with id: " + id)); // Replace with a proper exception
    }

    // --- Helper Mapper Methods ---

    private SequenceDto toDto(Sequence entity) {
        List<StepDto> stepDtos = entity.getSteps().stream().map(this::toStepDto).collect(Collectors.toList());
        SettingsDto settingsDto = new SettingsDto(entity.getSettings().isLoopSequence());

        return new SequenceDto(
                entity.getId(),
                entity.getName(),
                entity.getLastModified(),
                stepDtos,
                settingsDto
        );
    }

    private StepDto toStepDto(Step step) {
        PoseDto poseDto = new PoseDto(step.getPose().getX(), step.getPose().getY(), step.getPose().getZ(), step.getPose().getRoll(), step.getPose().getPitch(), step.getPose().getYaw());
        return new StepDto(step.getId(), step.getName(), poseDto, step.getInterpolation(), step.getSpeed());
    }

    private Sequence toEntity(SequenceDto dto) {
        Sequence entity = new Sequence();
        entity.setName(dto.name());

        Settings settings = new Settings();
        settings.setLoopSequence(dto.settings().loopSequence());
        entity.setSettings(settings);

        dto.steps().forEach(stepDto -> {
            Step step = new Step();
            step.setName(stepDto.name());
            step.setSpeed(stepDto.speed());
            step.setInterpolation(stepDto.interpolation());
            Pose pose = new Pose();
            // ... set pose fields from stepDto.pose()
            step.setPose(pose);
            entity.addStep(step); // Use the helper method to link the relationship
        });

        return entity;
    }
}