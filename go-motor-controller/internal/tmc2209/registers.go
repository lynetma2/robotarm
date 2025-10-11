package tmc2209

// Register addresses
const (
	regGCONF      byte = 0x00 // Global configuration
	regGSTAT      byte = 0x01 // Global status
	regIHOLD_IRUN byte = 0x10 // Driver current control
	regCHOPCONF   byte = 0x6C // Chopper configuration
	regVACTUAL    byte = 0x22 // Actual motor velocity
)
