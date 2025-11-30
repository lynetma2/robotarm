package com.sundtrack.robotarm.sequence.controller;

import com.sundtrack.robotarm.sequence.dto.SequenceDto;
import com.sundtrack.robotarm.sequence.service.SequenceService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.messaging.handler.annotation.MessageMapping;
import org.springframework.messaging.handler.annotation.Payload;
import org.springframework.messaging.handler.annotation.SendTo;
import org.springframework.stereotype.Controller;

@Controller
public class SequenceController {

    private static final Logger logger = LoggerFactory.getLogger(SequenceController.class);

    private final SequenceService sequenceService;

    @Autowired
    public SequenceController(SequenceService sequenceService) {
        this.sequenceService = sequenceService;
    }

    /**
     * Handles a request to fetch all sequences.
     * A client sends a message to "/app/sequences.getAll" and the response is sent to "/topic/sequences".
     */
    @MessageMapping("/sequences.getAll")
    @SendTo("/topic/sequences")
    public java.util.List<SequenceDto> getAllSequences() {
        logger.info("Received request for all sequences");
        return sequenceService.getAllSequences();
    }

    @MessageMapping("/sequences.create")
    @SendTo("/topic/sequences")
    public java.util.List<SequenceDto> createSequence(@Payload SequenceDto sequenceDto) {
        logger.info("Received request to create sequence: {}", sequenceDto.name());
        sequenceService.createSequence(sequenceDto);
        // After creating, return the new complete list to all subscribers
        return sequenceService.getAllSequences();
    }
}