#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "tmc2209_dev.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// Vendor Library Hooks
// These MUST match the declarations in TMC2209.h exactly.
// ============================================================

bool tmc2209_readWriteUART(
    uint16_t icID,
    uint8_t *data,
    size_t writeLength,
    size_t readLength
);

uint8_t tmc2209_getNodeAddress(uint16_t icID);

/**
 * @brief Maximum number of UART buses that can be initialized.
 */
#define TMC2209_MAX_BUSES 3

/**
 * @brief Maximum number of TMC2209 drivers that can be registered.
 */
#define TMC2209_MAX_DRIVERS 8

// ============================================================
// Bus Initialization
// ============================================================

/**
 * @brief Initialize a UART bus for TMC2209 communication.
 *        Call this once per UART bus before registering drivers.
 *
 * @param config UART bus configuration
 * @return ESP_OK on success
 */
esp_err_t tmc2209_port_init_bus(const tmc2209_bus_config_t *config);

/**
 * @brief Deinitialize all UART buses and free resources.
 */
esp_err_t tmc2209_port_deinit(void);

// ============================================================
// Driver Registration
// ============================================================

/**
 * @brief Register a TMC2209 driver instance.
 *        Maps an icID to a specific UART bus and node address.
 *
 * @param ic_id        Unique driver instance ID
 * @param uart_port    Which UART bus this driver is on
 * @param node_address UART node address (0-3, set by MS1/MS2 pins)
 * @return ESP_OK on success
 */
esp_err_t tmc2209_port_register(uint16_t ic_id, uart_port_t uart_port, uint8_t node_address);

/**
 * @brief Unregister a TMC2209 driver instance.
 *
 * @param ic_id Driver instance ID to remove
 * @return ESP_OK on success
 */
esp_err_t tmc2209_port_unregister(uint16_t ic_id);

// ============================================================
// Runtime Configuration
// ============================================================

/**
 * @brief Enable or disable echo discard for a specific UART bus.
 *
 * @param uart_port Which UART bus to configure
 * @param enable true to discard echo, false to skip
 */
void tmc2209_port_set_echo_discard(uart_port_t uart_port, bool enable);

#ifdef __cplusplus
}
#endif