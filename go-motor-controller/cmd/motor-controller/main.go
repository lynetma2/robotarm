package main

import (
	"log"
	"robotarm/internal/comms"
	"robotarm/internal/motorcontrol"
	"robotarm/internal/web"
)

func main() {
	log.Println("--- Application Starting ---")

	// Create the shared communication channels
	channels := comms.Channels{
		CommandChan:     make(chan comms.Command),
		DiagnosticsChan: make(chan comms.Diagnostics),
	}

	// Start the motor manager goroutine (the consumer)
	motorcontrol.StartManager(channels)

	// Start the DualSense manager (producer, placeholder)
	// dualsense.StartManager(channels.CommandChan)

	// Start the web server (producer and diagnostics broadcaster)
	// This is a blocking call, so it must be last.
	web.StartServer(":8080", channels)
}
