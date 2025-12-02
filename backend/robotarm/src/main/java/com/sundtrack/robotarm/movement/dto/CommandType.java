package com.sundtrack.robotarm.movement.dto;

import com.fasterxml.jackson.annotation.JsonValue;

public enum CommandType {
    STEP("step"),
    JOG("jog"),
    STOP("stop"),
    CONFIG("config");

    private final String value;
    CommandType(String value) { this.value = value; }
    @JsonValue public String getValue() { return value; }
}