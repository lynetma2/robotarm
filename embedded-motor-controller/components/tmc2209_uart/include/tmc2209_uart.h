#ifndef TMC2209_UART_H
#define TMC2209_UART_H
#include <stdint.h>
#include "driver/uart.h"
#include "tmc2209_unit.h"

// UART Helper constants
// SYNK sequence
#define TMC2209_SYNC 0b00000101

// TMC2209 address options
#define TMC2209_ADR_0 0x00
#define TMC2209_ADR_1 0x01
#define TMC2209_ADR_2 0x02
#define TMC2209_ADR_3 0x03

//Constants for faster CRC calculations
#define TMC2209_CRC_START_ADR_0 0x18
#define TMC2209_CRC_START_ADR_1 0x91
#define TMC2209_CRC_START_ADR_2 0xDF
#define TMC2209_CRC_START_ADR_3 0x56
#define TMC2209_CRC_START_ADR_RESPOND 0xEB

/**
 * 64 bit datagram for write message or read response message
 */
typedef union
{
    struct
    {
        uint64_t sync : 8;
        uint64_t serial_address : 8;
        uint64_t register_address : 8;
        uint64_t data : 32;
        uint64_t crc : 8;
    } REG;

     uint64_t UINT64;
} Datagram64;

/**
 *  32 bit datagram for read request message
 */
typedef union
{
    struct
    {
        uint32_t sync : 8;
        uint32_t serial_address : 8;
        uint32_t register_address : 8;
        uint32_t crc : 8;
    } REG;

    uint32_t UINT32;
} Datagram32;

/*******************************************************************************
 * LOW-LEVEL UART COMMUNICATION
 ******************************************************************************/

/**
 * @brief Writes data to a specific register of a TMC2209 driver.
 * @details This is a low-level function that constructs and sends a UART datagram
 * to the specified driver address and register.
 *
 * @param uart_num The UART port number to use for communication.
 * @param address The slave address of the target TMC2209 chip.
 * @param reg The register address to write to.
 * @param data The 32-bit data value to write into the register.
 */
void write(uart_port_t uart_num, uint8_t address, uint8_t reg, uint32_t data);

/**
 * @brief Reads data from a specific register of a TMC2209 driver.
 * @details This function sends a read request datagram via UART and waits for
 * the response from the TMC2209 driver.
 *
 * @param uart_num The UART port number to use for communication.
 * @param address The slave address of the target TMC2209 chip.
 * @param reg The register address to read from.
 * @return The 32-bit data value read from the register. Returns 0 on communication failure.
 */
uint32_t read(uart_port_t uart_num, uint8_t address, uint8_t reg);


/*******************************************************************************
 * REGISTER-SPECIFIC WRITE FUNCTIONS
 ******************************************************************************/

/**
 * @brief Writes to the GCONF (Global Configuration) register.
 * @details Sets global driver settings like I_scale_analog, internal_Rsense,
 * multistep_filt, and PDN_disable.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @param value The 32-bit value to be written to the GCONF register.
 */
void writeGCONF(TMC2209_UNIT *config, uint32_t value);

/**
 * @brief Writes to the IHOLD_IRUN register.
 * @details Configures motor current settings: IHOLD (standstill current),
 * IRUN (motor run current), and IHOLDDELAY (delay before current reduction).
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @param value The 32-bit value containing IHOLD, IRUN, and IHOLDDELAY settings.
 */
void writeIHOLD_IRUN(TMC2209_UNIT *config, uint32_t value);

/**
 * @brief Writes to the CHOPCONF (Chopper Configuration) register.
 * @details This is a key register for motor control, configuring settings like
 * TOFF, HSTRT, HEND, TBL, CHM (chopper mode), and microstep resolution (MRES).
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @param value The 32-bit value to be written to the CHOPCONF register.
 */
void writeCHOPCONF(TMC2209_UNIT *config, uint32_t value);

/**
 * @brief Writes to the PWMCONF (PWM Configuration) register.
 * @details Fine-tunes the PWM and StealthChop chopper modes for optimal smoothness
 * and low noise operation.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @param value The 32-bit value containing PWM configuration settings.
 */
void writePWMCONF(TMC2209_UNIT *config, uint32_t value);

/**
 * @brief Writes to the COOLCONF register.
 * @details Configures the CoolStep and StallGuard features for sensorless load
 * measurement and homing. Sets parameters like SEMIN, SEMAX, and SEDN.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @param value The 32-bit value to be written to the COOLCONF register.
 */
void writeCOOLCONF(TMC2209_UNIT *config, uint32_t value);

/**
 * @brief Writes to the TCOOLTHRS (CoolStep Threshold) register.
 * @details Sets the velocity threshold below which CoolStep is disabled. This
 * prevents load-based current control from activating at low speeds.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @param value The 20-bit threshold value.
 */
void writeTCOOLTHRS(TMC2209_UNIT *config, uint32_t value);

/**
 * @brief Writes to the TPWMTHRS (StealthChop PWM Threshold) register.
 * @details Sets the velocity threshold for switching from StealthChop to
 * SpreadCycle mode when the motor exceeds this speed.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @param value The 20-bit threshold value.
 */
void writeTPWMTHRS(TMC2209_UNIT *config, uint32_t value);

/**
 * @brief Writes to the SGTHRS (StallGuard Threshold) register.
 * @details Sets the sensitivity for StallGuard4 stall detection. A lower value
 * increases sensitivity, triggering a stall signal on smaller loads.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @param value The 8-bit threshold value (0-255).
 */
void writeSGTHRS(TMC2209_UNIT *config, uint32_t value);

/**
 * @brief Writes to the TPOWERDOWN register.
 * @details Sets the delay time in multiples of 2^18 clock cycles before the driver
 * enters a low-power standby mode after the motor has stopped.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @param value The 8-bit delay value.
 */
void writeTPOWERDOWN(TMC2209_UNIT *config, uint32_t value);


/*******************************************************************************
 * REGISTER-SPECIFIC READ FUNCTIONS
 ******************************************************************************/

/**
 * @brief Reads the IOIN (Input/Status) register.
 * @details Provides the status of various pins (ENN, MS1, MS2, etc.) and
 * internal driver states, such as version number and error flags.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @return The 32-bit value of the IOIN register.
 */
uint32_t readIOIN(TMC2209_UNIT *config);

/**
 * @brief Reads the SG_RESULT (StallGuard Result) register.
 * @details Returns the current StallGuard measurement value, which indicates
 * the mechanical load on the motor.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @return A 10-bit value representing the motor load (0-1023).
 */
uint32_t readSG_RESULT(TMC2209_UNIT *config);

/**
 * @brief Reads the IFCNT (Interface Transmission Counter) register.
 * @details Returns the number of successful datagrams transmitted over the
 * UART interface since the last reset. Useful for diagnosing communication issues.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @return An 8-bit value representing the transmission count.
 */
uint32_t readIFCNT(TMC2209_UNIT *config);


/*******************************************************************************
 * INITIALIZATION & DESTRUCTION
 ******************************************************************************/

/**
 * @brief Initializes a TMC2209 driver with safe, default settings.
 * @details This is a helper function that populates the config struct with
 * default values and then calls init_tmc2209.
 *
 * @param uart_num The UART port to use.
 * @param address The slave address of the TMC2209.
 * @param config Pointer to the TMC2209 configuration structure to be populated.
 */
bool init_tmc2209_default(uart_port_t uart_num, uint8_t address, TMC2209_UNIT *config);

/**
 * @brief Checks if the TMC2209 driver is available.
 * @details This function attempts to communicate with the driver by reading the
 * register IOIN to get the version to verify its presence.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 * @return
 * - true: The driver is available and responding.
 * - false: The driver is not responding.
 */
bool available_tmc2209(TMC2209_UNIT *config);

/**
 * @brief De-initializes the TMC2209 driver and releases resources.
 * @details Disables the driver and deletes the UART driver instance associated with it.
 *
 * @param config Pointer to the TMC2209 configuration structure.
 */
void destroy_tmc2209(TMC2209_UNIT *config);

#endif
