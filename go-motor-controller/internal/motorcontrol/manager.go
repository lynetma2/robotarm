package motorcontrol

import (
	"log"
	"robotarm/internal/comms"
	"time"
)

// StartManager launches the motor control goroutine.
func StartManager(channels comms.Channels) {
	log.Println("Starting motor manager goroutine...")

	go func() {
		// Initialize hardware here, inside the goroutine
		//port, err := serial.Open("/dev/ttyAMA0", &serial.Mode{BaudRate: 115200})
		//if err != nil {
		//	log.Fatalf("FATAL: Failed to open serial port: %v", err)
		//}
		//defer port.Close()
		//log.Println("✅ Motor hardware initialized successfully.")

		// This loop continuously consumes commands from the channel
		for cmd := range channels.CommandChan {
			log.Printf("Motor received command: %+v", cmd)

			switch cmd.Action {
			case "start":
				log.Println("Starting motor")
			case "set_velocity":
				// Placeholder for your TMC2209 library call
				// tmc.SetVelocity(cmd.Value)
				log.Printf("Setting motor velocity to %.0f", cmd.Value)
			case "stop":
				// tmc.Stop()
				log.Println("Stopping motor")
			}
		}
	}()

	// A separate goroutine to periodically send diagnostics
	go func() {
		ticker := time.NewTicker(500 * time.Millisecond)
		defer ticker.Stop()
		for range ticker.C {
			// Placeholder for reading real data from the motor
			diag := comms.Diagnostics{Position: time.Now().Second()}
			channels.DiagnosticsChan <- diag
		}
	}()
}
