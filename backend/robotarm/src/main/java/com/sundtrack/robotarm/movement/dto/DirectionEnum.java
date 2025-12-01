package com.sundtrack.robotarm.movement.dto;

public enum DirectionEnum {
    CLOCKWISE(1),
    COUNTER_CLOCKWISE(-1);

    private final int value;

    DirectionEnum(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}