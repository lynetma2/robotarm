package com.sundtrack.robotarm.motorConfig.model;

import jakarta.persistence.*;

@Entity
@Table(name = "motor_config")
public class MotorConfig {

    @Id
    private Integer id;

    @Column(nullable = false)
    private String name;

    @Column(nullable = false)
    private boolean enabled;

    // GCONF Register (0x00) - Global Configuration
    @Column(name = "gconf", nullable = false)
    private Integer gconf = 0x00000000;

    // CHOPCONF Register (0x6C) - Chopper Configuration
    @Column(name = "chopconf", nullable = false)
    private Integer chopconf = 0x10000053;

    // IHOLD_IRUN Register (0x10) - Current Settings
    @Column(name = "ihold_irun", nullable = false)
    private Integer iholdIrun = 0x00001F1F;

    // TCOOLTHRS Register (0x14) - CoolStep Threshold
    @Column(name = "tcoolthrs", nullable = false)
    private Integer tcoolthrs = 0x00000000;

    // SGTHRS Register (0x40) - StallGuard Threshold
    @Column(name = "sgthrs", nullable = false)
    private Integer sgthrs = 0x00000000;

    // COOLCONF Register (0x42) - CoolStep Configuration
    @Column(name = "coolconf", nullable = false)
    private Integer coolconf = 0x00000000;

    // PWMCONF Register (0x70) - PWM Configuration
    @Column(name = "pwmconf", nullable = false)
    private Integer pwmconf = 0xC10D0024;

    // Getters and Setters
    public Integer getId() { return id; }
    public void setId(Integer id) { this.id = id; }

    public String getName() { return name; }
    public void setName(String name) { this.name = name; }

    public boolean isEnabled() { return enabled; }
    public void setEnabled(boolean enabled) { this.enabled = enabled; }

    public Integer getGconf() { return gconf; }
    public void setGconf(Integer gconf) { this.gconf = gconf; }

    public Integer getChopconf() { return chopconf; }
    public void setChopconf(Integer chopconf) { this.chopconf = chopconf; }

    public Integer getIholdIrun() { return iholdIrun; }
    public void setIholdIrun(Integer iholdIrun) { this.iholdIrun = iholdIrun; }

    public Integer getTcoolthrs() { return tcoolthrs; }
    public void setTcoolthrs(Integer tcoolthrs) { this.tcoolthrs = tcoolthrs; }

    public Integer getSgthrs() { return sgthrs; }
    public void setSgthrs(Integer sgthrs) { this.sgthrs = sgthrs; }

    public Integer getCoolconf() { return coolconf; }
    public void setCoolconf(Integer coolconf) { this.coolconf = coolconf; }

    public Integer getPwmconf() { return pwmconf; }
    public void setPwmconf(Integer pwmconf) { this.pwmconf = pwmconf; }
}