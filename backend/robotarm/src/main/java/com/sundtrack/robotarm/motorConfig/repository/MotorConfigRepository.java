package com.sundtrack.robotarm.motorConfig.repository;

import com.sundtrack.robotarm.motorConfig.model.MotorConfig;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

@Repository
public interface MotorConfigRepository extends JpaRepository<MotorConfig, Integer> {
}