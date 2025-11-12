#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief Initialize UART in half-duplex mode for TMC drivers (e.g. TMC2209)
     *
     * Configures UART on one pin for both TX and RX.
     */
    void tmc_interface_init(void);

    /**
     * @brief Write a 32-bit register value to a TMC driver over UART (half-duplex)
     *
     * @param slave  The TMC UART slave address (usually 0x00)
     * @param reg    The TMC register address
     * @param value  The 32-bit value to write
     */
    bool tmc_write(uint8_t slave, uint8_t reg, uint32_t value);

    /**
     * @brief Read a 32-bit register value from a TMC driver over UART (half-duplex)
     *
     * @param slave  The TMC UART slave address (usually 0x00)
     * @param reg    The TMC register address
     * @return The 32-bit register value read from the driver
     */
    uint32_t tmc_read(uint8_t slave, uint8_t reg);

#ifdef __cplusplus
}
#endif
