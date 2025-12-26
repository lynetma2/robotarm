package com.sundtrack.robotarm.motorConfig.model;

import jakarta.persistence.Entity;
import jakarta.persistence.Id;

@Entity
public class MotorConfig {

    @Id
    private Integer motorId;

    // Default: 452 (en_spreadcycle | pdn_disable | mstep_reg_select | multistep_filt)
    private int gconf = 452;

    // Default: 335609939 (toff=3, hstrt=5, tbl=2, mres=4, intpol=true)
    private long chopconf = 335609939L;

    // Default: 532240 (ihold=16, irun=31, iholddelay=8)
    private int ihold_irun = 532240;

    // Default: 331294 (pwm_ofs=30, pwm_grad=14, pwm_freq=1, pwm_autoscale=true)
    private int pwmconf = 331294;

    /**
     * TPOWERDOWN register: Sets the delay for power-down after standstill. (0-255)
     */
    private int tpowerdown = 20;

    /**
     * TPWMTHRS register: Velocity threshold for switching to stealthChop.
     */
    private int tpwmthrs = 0;

    /**
     * SGTHRS register: StallGuard4 threshold (0-255).
     */
    private int sgthrs = 0;

    /**
     * TCOOLTHRS register: CoolStep lower velocity threshold.
     */
    private int tcoolthrs = 0;

    // Getters and Setters
    public Integer getMotorId() {
        return motorId;
    }

    public void setMotorId(Integer motorId) {
        this.motorId = motorId;
    }

    public int getGconf() {
        return gconf;
    }

    public void setGconf(int gconf) {
        this.gconf = gconf;
    }

    public long getChopconf() {
        return chopconf;
    }

    public void setChopconf(long chopconf) {
        this.chopconf = chopconf;
    }

    public int getIhold_irun() {
        return ihold_irun;
    }

    public void setIhold_irun(int ihold_irun) {
        this.ihold_irun = ihold_irun;
    }

    public int getPwmconf() {
        return pwmconf;
    }

    public void setPwmconf(int pwmconf) {
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

    public int getSgthrs() {
        return sgthrs;
    }

    public void setSgthrs(int sgthrs) {
        this.sgthrs = sgthrs;
    }

    public int getTcoolthrs() {
        return tcoolthrs;
    }

    public void setTcoolthrs(int tcoolthrs) {
        this.tcoolthrs = tcoolthrs;
    }
}