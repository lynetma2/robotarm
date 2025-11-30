package com.sundtrack.robotarm.sequence.model;

import jakarta.persistence.Embeddable;

@Embeddable
public class Settings {

    private boolean loopSequence = false; // Using a more descriptive name to avoid conflict with SQL keywords

    // Getters and Setters
    public boolean isLoopSequence() { return loopSequence; }

    public void setLoopSequence(boolean loop) { this.loopSequence = loop; }
}