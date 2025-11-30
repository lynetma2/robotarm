package com.sundtrack.robotarm.sequence.service;

import com.sundtrack.robotarm.sequence.dto.*;
import com.sundtrack.robotarm.sequence.model.*;
import com.sundtrack.robotarm.sequence.repository.SequenceRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
import java.util.Optional;
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

    @Transactional
    public SequenceDto updateSequence(Long id, SequenceDto sequenceDto) {
        // 1. Find the existing sequence or throw an exception
        Sequence existingSequence = sequenceRepository.findById(id)
                .orElseThrow(() -> new RuntimeException("Sequence not found with id: " + id));

        // 2. Update the properties from the DTO
        existingSequence.setTitle(sequenceDto.title());
        existingSequence.setDescription(sequenceDto.description());

        // 3. Update the embedded settings object
        Settings settings = existingSequence.getSettings();
        if (settings == null) {
            settings = new Settings();
        }
        settings.setLoopSequence(sequenceDto.settings().loopSequence());
        existingSequence.setSettings(settings);

        // 4. Clear the old steps and add the new ones from the DTO.
        // Because of `orphanRemoval=true`, the old steps will be deleted from the database.
        existingSequence.getSteps().clear();
        sequenceDto.steps().forEach(stepDto -> existingSequence.addStep(toStepEntity(stepDto)));

        // 5. The transaction will be committed on method exit, saving the changes.
        return toDto(existingSequence);
    }

    // --- Helper Mapper Methods ---

    private SequenceDto toDto(Sequence entity) {
        List<StepDto> stepDtos = entity.getSteps().stream().map(this::toStepDto).collect(Collectors.toList());
        SettingsDto settingsDto = new SettingsDto(entity.getSettings().isLoopSequence());

        return new SequenceDto(
                Optional.ofNullable(entity.getId()),
                entity.getTitle(),
                entity.getDescription(),
                entity.getLastModified(),
                stepDtos,
                settingsDto
        );
    }

    private StepDto toStepDto(Step step) {
        PoseDto poseDto = new PoseDto(step.getPose().getX(), step.getPose().getY(), step.getPose().getZ(), step.getPose().getRoll(), step.getPose().getPitch(), step.getPose().getYaw());
        return new StepDto(Optional.ofNullable(step.getId()), step.getName(), poseDto, step.getInterpolation(), step.getSpeed());
    }

    private Sequence toEntity(SequenceDto dto) {
        Sequence entity = new Sequence();
        entity.setTitle(dto.title());
        entity.setDescription(dto.description());

        if (dto.settings() != null) {
            Settings settings = new Settings();
            settings.setLoopSequence(dto.settings().loopSequence());
            entity.setSettings(settings);
        }

        if (dto.steps() != null) {
            dto.steps().forEach(stepDto -> entity.addStep(toStepEntity(stepDto)));
        }

        return entity;
    }

    private Step toStepEntity(StepDto stepDto) {
        Step step = new Step();
        step.setName(stepDto.name());
        step.setSpeed(stepDto.speed());
        step.setInterpolation(stepDto.interpolation());
        // You can create a similar helper for the Pose if you wish
        step.setPose(toPoseEntity(stepDto.pose()));
        return step;
    }

    private Pose toPoseEntity(PoseDto poseDto) {
        Pose pose = new Pose();
        pose.setX(poseDto.x());
        pose.setY(poseDto.y());
        pose.setZ(poseDto.z());
        pose.setRoll(poseDto.roll());
        pose.setPitch(poseDto.pitch());
        pose.setYaw(poseDto.yaw());
        return pose;
    }
}