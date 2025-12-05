package com.sundtrack.robotarm.tmc2209.repository;

import com.sundtrack.robotarm.tmc2209.model.TMC2209Config;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

@Repository
public interface TMC2209ConfigRepository extends JpaRepository<TMC2209Config, Integer> {
}