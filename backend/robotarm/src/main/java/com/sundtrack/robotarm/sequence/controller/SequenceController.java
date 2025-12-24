package com.sundtrack.robotarm.sequence.controller;

import com.sundtrack.robotarm.sequence.dto.SequenceDto;
import com.sundtrack.robotarm.sequence.service.SequenceService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/v1/sequences")
public class SequenceController {

    private static final Logger logger = LoggerFactory.getLogger(SequenceController.class);

    private final SequenceService sequenceService;

    @Autowired
    public SequenceController(SequenceService sequenceService) {
        this.sequenceService = sequenceService;
    }

    @GetMapping
    public ResponseEntity<List<SequenceDto>> getAllSequences() {
        logger.info("Received request for all sequences");
        return ResponseEntity.ok(sequenceService.getAllSequences());
    }

    @PostMapping
    public ResponseEntity<List<SequenceDto>> createSequence(@RequestBody SequenceDto sequenceDto) {
        logger.info("Received request to create sequence: {}", sequenceDto.title());
        sequenceService.createSequence(sequenceDto);
        // After creating, return the new complete list to all subscribers
        return ResponseEntity.ok(sequenceService.getAllSequences());
    }

    @PutMapping("/{id}")
    public ResponseEntity<List<SequenceDto>> updateSequence(@PathVariable Long id, @RequestBody SequenceDto sequenceDto) {
        logger.info("Received request to update sequence with id: {}", id);
        sequenceService.updateSequence(id, sequenceDto);
        // After updating, return the new complete list to all subscribers
        return ResponseEntity.ok(sequenceService.getAllSequences());
    }

    @DeleteMapping("/{id}")
    public ResponseEntity<List<SequenceDto>> deleteSequence(@PathVariable Long id) {
        logger.info("Received request to delete sequence with id: {}", id);
        sequenceService.deleteSequence(id);
        // After deleting, return the new complete list to all subscribers
        return ResponseEntity.ok(sequenceService.getAllSequences());
    }
}