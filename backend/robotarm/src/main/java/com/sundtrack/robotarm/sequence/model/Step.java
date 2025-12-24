package com.sundtrack.robotarm.sequence.model;

import com.fasterxml.jackson.annotation.JsonIgnore;
import jakarta.persistence.*;

@Entity
@Table(name = "sequence_steps")
public class Step {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    private String name;

    @Embedded
    private Pose pose;

    @Enumerated(EnumType.STRING)
    private Interpolation interpolation;

    private long speed;

    @ManyToOne(fetch = FetchType.EAGER)
    @JoinColumn(name = "sequence_id")
    @JsonIgnore // Important: Prevents infinite loops during JSON serialization
    private Sequence sequence;

    // Getters and Setters
    public Long getId() { return id; }

    public void setId(Long id) { this.id = id; }

    public String getName() { return name; }

    public void setName(String name) { this.name = name; }

    public Pose getPose() { return pose; }

    public void setPose(Pose pose) { this.pose = pose; }

    public Interpolation getInterpolation() { return interpolation; }

    public void setInterpolation(Interpolation interpolation) { this.interpolation = interpolation; }

    public long getSpeed() { return speed; }

    public void setSpeed(long speed) { this.speed = speed; }

    public Sequence getSequence() { return sequence; }

    public void setSequence(Sequence sequence) { this.sequence = sequence; }
}