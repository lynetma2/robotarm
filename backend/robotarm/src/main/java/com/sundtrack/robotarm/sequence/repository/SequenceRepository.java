package com.sundtrack.robotarm.sequence.repository;

import com.sundtrack.robotarm.sequence.model.Sequence;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;

@Repository
public interface SequenceRepository extends JpaRepository<Sequence, Long> {
    // Spring Data JPA provides findAll(), findById(), save(), deleteById(), etc.

    // Method using Spring Data's query creation from the method name
    //Optional<Sequence> findByName(String name);

    // Method using an explicit JPQL query for maximum clarity and control
    @org.springframework.data.jpa.repository.Query("SELECT s FROM Sequence s WHERE s.settings.loopSequence = true")
    List<Sequence> findAllLoopingSequences();
}