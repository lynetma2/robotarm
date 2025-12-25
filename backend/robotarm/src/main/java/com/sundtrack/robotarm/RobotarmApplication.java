package com.sundtrack.robotarm;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.scheduling.annotation.EnableScheduling;

@SpringBootApplication
@EnableScheduling
public class RobotarmApplication {

	public static void main(String[] args) {
		SpringApplication.run(RobotarmApplication.class, args);
	}

}
