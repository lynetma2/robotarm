package com.sundtrack.robotarm.sequence.model;

import jakarta.persistence.*;
import org.hibernate.annotations.UpdateTimestamp;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

@Entity
@Table(name = "sequences")
public class Sequence {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    private String name;

    @UpdateTimestamp
    private Instant lastModified;

    @OneToMany(
            mappedBy = "sequence",
            cascade = CascadeType.ALL,
            orphanRemoval = true,
            fetch = FetchType.EAGER // Use EAGER for simplicity, LAZY is better for performance on large collections
    )
    private List<Step> steps = new ArrayList<>();

    @Embedded
    private Settings settings;

    // Getters and Setters
    public Long getId() { return id; }

    public void setId(Long id) { this.id = id; }

    public String getName() { return name; }

    public void setName(String name) { this.name = name; }

    public Instant getLastModified() { return lastModified; }

    public void setLastModified(Instant lastModified) { this.lastModified = lastModified; }

    public List<Step> getSteps() { return steps; }

    public void setSteps(List<Step> steps) { this.steps = steps; }

    public Settings getSettings() { return settings; }

    public void setSettings(Settings settings) { this.settings = settings; }

    // Helper methods to keep both sides of the relationship in sync
    public void addStep(Step step) {
        steps.add(step);
        step.setSequence(this);
    }
}