package com.sundtrack.robotarm.motorConfig.model;

import com.sundtrack.robotarm.motorConfig.dto.CHOPCONF;
import com.sundtrack.robotarm.motorConfig.dto.GCONF;
import com.sundtrack.robotarm.motorConfig.dto.IHOLD_IRUN;
import com.sundtrack.robotarm.motorConfig.dto.PWMCONF;
import jakarta.persistence.*;

@Entity
public class MotorConfig {

    @Id
    private Integer motorId;

    @Embedded
    private GCONF gconf = new GCONF();

    @Embedded
    private CHOPCONF chopconf = new CHOPCONF();

    @Embedded
    private IHOLD_IRUN ihold_irun = new IHOLD_IRUN();

    @Embedded
    private PWMCONF pwmconf = new PWMCONF();

    /**
     * TPOWERDOWN register: Sets the delay for power-down after standstill. (0-255)
     */
    private int tpowerdown = 20;

    /**
     * TPWMTHRS register: Velocity threshold for switching to stealthChop.
     */
    private int tpwmthrs = 0;

    // Getters and Setters
    public Integer getMotorId() {
        return motorId;
    }

    public void setMotorId(Integer motorId) {
        this.motorId = motorId;
    }

    public GCONF getGconf() {
        return gconf;
    }

    public void setGconf(GCONF gconf) {
        this.gconf = gconf;
    }

    public CHOPCONF getChopconf() { return chopconf; }

    public void setChopconf(CHOPCONF chopconf) { this.chopconf = chopconf; }

    public IHOLD_IRUN getIhold_irun() {
        return ihold_irun;
    }

    public void setIhold_irun(IHOLD_IRUN ihold_irun) {
        this.ihold_irun = ihold_irun;
    }

    public PWMCONF getPwmconf() {
        return pwmconf;
    }

    public void setPwmconf(PWMCONF pwmconf) {
        this.pwmconf = pwmconf;
    }

    public int getTpowerdown() {
        return tpowerdown;
    }

    public void setTpowerdown(int tpowerdown) {
        this.tpowerdown = tpowerdown;
    }

    public int getTpwmthrs() {
        return tpwmthrs;
    }

    public void setTpwmthrs(int tpwmthrs) {
        this.tpwmthrs = tpwmthrs;
    }
}