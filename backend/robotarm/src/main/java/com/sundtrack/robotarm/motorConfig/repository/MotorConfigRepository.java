package com.sundtrack.robotarm.motorConfig.repository;

import com.sundtrack.robotarm.motorConfig.model.MotorConfig;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;

@Repository
public interface MotorConfigRepository extends JpaRepository<MotorConfig, Integer> {

    // --- Explicitly declared methods for readability ---

    @Override
    List<MotorConfig> findAll();

    @Override
    Optional<MotorConfig> findById(Integer id);

    @Override
    <S extends MotorConfig> S save(S entity);

    @Override
    void deleteById(Integer id);

    @Override
    boolean existsById(Integer id);
}