package comms

// Command represents a universal action for the motor manager.
// Using a struct provides type safety, a big advantage over Python's dict.
type Command struct {
	Action string  `json:"action"`
	Value  float64 `json:"value,omitempty"`
}

// Diagnostics represents the state sent back to the web clients.
type Diagnostics struct {
	Position int `json:"position"`
}

// Channels holds the shared communication channels for the application.
type Channels struct {
	CommandChan     chan Command
	DiagnosticsChan chan Diagnostics
}
