#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// UART Bus Setup
// ============================================================

/**
 * @brief Configuration for a UART bus shared by one or more TMC2209 drivers.
 */
typedef struct {
    uart_port_t uart_port;    // UART peripheral (e.g., UART_NUM_1)
    int tx_pin;               // GPIO for UART TX (-1 for default)
    int rx_pin;               // GPIO for UART RX (-1 for default)
    uint32_t baud_rate;       // Baud rate (typically 115200)
    bool discard_echo;        // Discard echoed TX bytes (single-wire UART)
    uint32_t timeout_ms;      // UART read/write timeout in milliseconds
} tmc2209_bus_config_t;

/**
 * @brief Initialize a UART bus for TMC2209 communication.
 *        Must be called once per bus, before any tmc2209_init().
 *
 * @param config UART bus configuration
 * @return ESP_OK on success
 */
esp_err_t tmc2209_init_bus(const tmc2209_bus_config_t *config);

/**
 * @brief Deinitialize all UART buses and free resources.
 *        Call this after all tmc2209_deinit() calls.
 */
esp_err_t tmc2209_deinit_bus(void);

// ============================================================
// Configuration
// ============================================================

/**
 * @brief Configuration for initializing a TMC2209 driver.
 */
typedef struct {
    uart_port_t uart_port;    // UART bus this motor is on
    uint16_t ic_id;           // Unique ID for this driver (used by port layer)
    uint16_t r_sense_mohm;    // Sense resistor value in milliohms (e.g., 110)
    uint8_t node_address;     // UART node address (0-3, set by MS1/MS2 pins)
} tmc2209_config_t;

// ============================================================
// Device Context
// ============================================================

/**
 * @brief TMC2209 device context.
 *
 * Semi-transparent: fields are visible but should not be
 * modified directly. Use the provided functions instead.
 */
typedef struct {
    uart_port_t uart_port;
    uint16_t ic_id;
    uint16_t r_sense_mohm;
    uint8_t node_address;
    int32_t internal_velocity;
    SemaphoreHandle_t mutex;
} tmc2209_dev_t;

// ============================================================
// Lifecycle
// ============================================================

/**
 * @brief Initialize the TMC2209 device context.
 *        Must be called before any other function.
 */
esp_err_t tmc2209_init(tmc2209_dev_t *dev, const tmc2209_config_t *config);

/**
 * @brief Clean up resources.
 */
esp_err_t tmc2209_deinit(tmc2209_dev_t *dev);

// ============================================================
// Current Control
// ============================================================

/**
 * @brief Set the motor run current (IRUN) in milliamps.
 *        This is the current used when the motor is moving.
 *
 * @param current_ma Current in milliamps (e.g., 1000 for 1A)
 */
esp_err_t tmc2209_set_run_current(tmc2209_dev_t *dev, uint32_t current_ma);

/**
 * @brief Set the motor hold current (IHOLD) in milliamps.
 *        This is the current used when the motor is stationary.
 *        Typically set lower than run current to reduce heat.
 *
 * @param current_ma Current in milliamps
 */
esp_err_t tmc2209_set_hold_current(tmc2209_dev_t *dev, uint32_t current_ma);

/**
 * @brief Get the current run current setting in milliamps.
 */
esp_err_t tmc2209_get_run_current(tmc2209_dev_t *dev, uint32_t *current_ma);

/**
 * @brief Get the actual measured motor current in milliamps.
 *        Reads the actual current from the driver's ADC.
 */
esp_err_t tmc2209_get_actual_current(tmc2209_dev_t *dev, uint32_t *current_ma);

// ============================================================
// Motor Configuration
// ============================================================

/**
 * @brief Set microstep resolution.
 * @param microsteps 1, 2, 4, 8, 16, 32, 64, 128, or 256
 */
esp_err_t tmc2209_set_microsteps(tmc2209_dev_t *dev, uint16_t microsteps);

/**
 * @brief Enable or disable StealthChop (quiet PWM mode).
 *        If disabled, uses SpreadCycle (more torque, more noise).
 */
esp_err_t tmc2209_set_stealthchop(tmc2209_dev_t *dev, bool enable);

/**
 * @brief Check if StealthChop is currently enabled.
 */
esp_err_t tmc2209_is_stealthchop_enabled(tmc2209_dev_t *dev, bool *enabled);

/**
 * @brief Set the standstill (idle) mode.
 *        Controls what the driver does when velocity is zero.
 *
 * @param mode  0: Normal operation
 *              1: Freewheeling (motor coils de-energized)
 *              2: Passive braking (short coils low-side)
 *              3: Active braking (short coils high-side)
 */
esp_err_t tmc2209_set_standstill_mode(tmc2209_dev_t *dev, uint8_t mode);

// ============================================================
// StallGuard (Sensorless Homing / Stall Detection)
// ============================================================

/**
 * @brief Enable or disable StallGuard.
 *        Must be enabled for stall detection and sensorless homing.
 */
esp_err_t tmc2209_set_stallguard_enabled(tmc2209_dev_t *dev, bool enabled);

/**
 * @brief Set the StallGuard sensitivity threshold.
 *        Range: -64 (most sensitive) to +63 (least sensitive).
 *        Typical starting point: 0 to 10.
 *
 * @param threshold Sensitivity threshold (-64 to 63)
 */
esp_err_t tmc2209_set_stallguard_threshold(tmc2209_dev_t *dev, int8_t threshold);

/**
 * @brief Set the minimum velocity for StallGuard to be active.
 *        StallGuard readings are only valid above this velocity.
 *
 * @param velocity Minimum velocity in internal units
 */
esp_err_t tmc2209_set_stallguard_velocity_threshold(tmc2209_dev_t *dev, uint32_t velocity);

/**
 * @brief Get the raw StallGuard result value.
 *        Higher values indicate lighter load, lower values indicate heavier load.
 *        Useful for tuning the threshold.
 *
 * @param result Pointer to store the StallGuard result (0-1023)
 */
esp_err_t tmc2209_get_stallguard_result(tmc2209_dev_t *dev, uint16_t *result);

// ============================================================
// Internal Motion Controller (Velocity Only)
// ============================================================

/**
 * @brief Set the internal velocity.
 *        The TMC2209 internal motion controller is velocity-only.
 *        It does not support position control or ramping.
 *
 * @param velocity Velocity in internal units.
 *                 Positive = forward, Negative = reverse, 0 = stop.
 */
esp_err_t tmc2209_set_internal_velocity(tmc2209_dev_t *dev, int32_t velocity);

/**
 * @brief Start the internal motion controller.
 *        The driver generates STEP pulses at the set velocity.
 */
esp_err_t tmc2209_start_internal_motion(tmc2209_dev_t *dev);

/**
 * @brief Stop the internal motion controller.
 */
esp_err_t tmc2209_stop_internal_motion(tmc2209_dev_t *dev);

// ============================================================
// Step/Dir Mode (External Pulse Generation)
// ============================================================

/**
 * @brief Enable the STEP/DIR interface.
 *        Call this when using ESP32 peripherals (MCPWM, RMT, etc.)
 *        to generate STEP pulses externally.
 */
esp_err_t tmc2209_enable_step_dir(tmc2209_dev_t *dev);

/**
 * @brief Disable the STEP/DIR interface.
 */
esp_err_t tmc2209_disable_step_dir(tmc2209_dev_t *dev);

// ============================================================
// Diagnostics & Status
// ============================================================

/**
 * @brief Get the raw DRV_STATUS register value.
 *        Useful for advanced diagnostics not covered by convenience functions.
 */
esp_err_t tmc2209_get_driver_status(tmc2209_dev_t *dev, uint32_t *status);

/**
 * @brief Check if the driver has flagged an error.
 *        Checks for overtemperature, short circuit, etc.
 */
esp_err_t tmc2209_has_error(tmc2209_dev_t *dev, bool *has_error);

/**
 * @brief Check if the motor is currently stalled.
 *        Requires StallGuard to be enabled.
 */
esp_err_t tmc2209_is_stalled(tmc2209_dev_t *dev, bool *is_stalled);

/**
 * @brief Check if the driver is over temperature.
 */
esp_err_t tmc2209_is_overtemp(tmc2209_dev_t *dev, bool *overtemp);

/**
 * @brief Clear all error flags in the GSTAT register.
 *        Call this after handling an error to allow the driver to resume.
 */
esp_err_t tmc2209_clear_errors(tmc2209_dev_t *dev);

#ifdef __cplusplus
}
#endif