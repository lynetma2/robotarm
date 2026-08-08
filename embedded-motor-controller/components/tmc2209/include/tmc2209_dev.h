#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

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
 */
esp_err_t tmc2209_init_bus(const tmc2209_bus_config_t *config);

/**
 * @brief Deinitialize all UART buses and free resources.
 */
esp_err_t tmc2209_deinit_bus(void);

// ============================================================
// Configuration
// ============================================================

/**
 * @brief Step/Dir pin configuration. Set both to -1 if not used.
 */
typedef struct {
    bool enabled;             // false (default) = STEP/DIR pins not used
    int step_gpio;            // GPIO wired to the TMC2209 STEP pin
    int dir_gpio;             // GPIO wired to the TMC2209 DIR pin
} tmc2209_stepdir_config_t;

/**
 * @brief Configuration for initializing a TMC2209 driver.
 */
typedef struct {
    uart_port_t uart_port;    // Which UART bus this driver is on
    uint16_t ic_id;           // Unique ID for this driver
    uint16_t r_sense_mohm;    // Sense resistor value in milliohms (e.g., 110)
    uint8_t node_address;     // UART node address (0-3, set by MS1/MS2 pins)
    int enable_gpio;          // GPIO wired to ENN, or -1 if hardwired to GND
    bool enable_active_high;  // false (default) = active-low, like the TMC2209
    tmc2209_stepdir_config_t stepdir;  // Step/Dir pins (optional)
} tmc2209_config_t;

// ============================================================
// Device Context
// ============================================================

/**
 * @brief Opaque handle to the private step/dir pulse engine.
 *        Defined only inside the library; never touch it.
 */
typedef struct tmc2209_stepdir tmc2209_stepdir_t;

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
    int32_t internal_velocity;      // Stored velocity for internal motion
    bool internal_running;
    int enable_gpio;
    bool enable_active_high;
    tmc2209_stepdir_config_t stepdir_cfg;
    tmc2209_stepdir_t *stepdir;     // Private pulse-engine handle (NULL until stepdir_init)
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
 * @brief Clean up resources (also stops and frees the step/dir engine).
 */
esp_err_t tmc2209_deinit(tmc2209_dev_t *dev);

// ============================================================
// Driver Enable (ENN pin)
// ============================================================

/**
 * @brief Enable or disable the motor driver outputs.
 *
 * The library translates this semantic flag into the correct electrical
 * level using enable_active_high. On a normal TMC2209 (active-low ENN),
 * enabled=true drives the pin LOW.
 *
 * If enable_gpio is -1 (not wired), this is a no-op returning ESP_OK.
 */
esp_err_t tmc2209_set_enabled(tmc2209_dev_t *dev, bool enabled);

// ============================================================
// Current Control
// ============================================================

/** @brief Set the motor run current (IRUN) in milliamps. */
esp_err_t tmc2209_set_run_current(tmc2209_dev_t *dev, uint32_t current_ma);

/** @brief Set the motor hold current (IHOLD) in milliamps. */
esp_err_t tmc2209_set_hold_current(tmc2209_dev_t *dev, uint32_t current_ma);

/** @brief Get the current run current setting in milliamps. */
esp_err_t tmc2209_get_run_current(tmc2209_dev_t *dev, uint32_t *current_ma);

/** @brief Get the actual measured motor current in milliamps. */
esp_err_t tmc2209_get_actual_current(tmc2209_dev_t *dev, uint32_t *current_ma);

// ============================================================
// Motor Configuration
// ============================================================

/** @brief Set microstep resolution (1, 2, 4, ..., 256). */
esp_err_t tmc2209_set_microsteps(tmc2209_dev_t *dev, uint16_t microsteps);

/** @brief Enable or disable StealthChop (quiet PWM mode). */
esp_err_t tmc2209_set_stealthchop(tmc2209_dev_t *dev, bool enable);

/** @brief Check if StealthChop is currently enabled. */
esp_err_t tmc2209_is_stealthchop_enabled(tmc2209_dev_t *dev, bool *enabled);

/**
 * @brief Set the standstill (idle) mode.
 * @param mode 0: Normal, 1: Freewheeling, 2: Passive braking, 3: Active braking
 */
esp_err_t tmc2209_set_standstill_mode(tmc2209_dev_t *dev, uint8_t mode);

// ============================================================
// StallGuard (Sensorless Homing / Stall Detection)
// ============================================================

/** @brief Enable or disable StallGuard. */
esp_err_t tmc2209_set_stallguard_enabled(tmc2209_dev_t *dev, bool enabled);

/** @brief Set the StallGuard sensitivity threshold. */
esp_err_t tmc2209_set_stallguard_threshold(tmc2209_dev_t *dev, int8_t threshold);

/** @brief Set the minimum velocity for StallGuard to be active. */
esp_err_t tmc2209_set_stallguard_velocity_threshold(tmc2209_dev_t *dev, uint32_t velocity);

/** @brief Get the raw StallGuard result value (0-1023). */
esp_err_t tmc2209_get_stallguard_result(tmc2209_dev_t *dev, uint16_t *result);

// ============================================================
// Internal Motion Controller (Velocity Only, via UART)
// ============================================================

/** @brief Store the internal velocity (applied by start_internal_motion). */
esp_err_t tmc2209_set_internal_velocity(tmc2209_dev_t *dev, int32_t velocity);

/** @brief Start the internal motion controller at the stored velocity. */
esp_err_t tmc2209_start_internal_motion(tmc2209_dev_t *dev);

/** @brief Stop the internal motion controller. */
esp_err_t tmc2209_stop_internal_motion(tmc2209_dev_t *dev);

// ============================================================
// Step/Dir Motion (External Pulse Generation, RMT backend)
// ============================================================

/**
 * @brief Initialize the step/dir pulse engine.
 *
 * Creates the RMT channel once and writes VACTUAL=0 over UART so the
 * driver listens to the STEP pin. Requires step_gpio/dir_gpio to be set.
 */
esp_err_t tmc2209_stepdir_init(tmc2209_dev_t *dev);

/** @brief Tear down the pulse engine and release the hardware. */
esp_err_t tmc2209_stepdir_deinit(tmc2209_dev_t *dev);

/** @brief Set the direction for continuous stepping. true = forward. */
esp_err_t tmc2209_set_direction(tmc2209_dev_t *dev, bool forward);

/**
 * @brief Set the step rate in steps (microsteps) per second.
 *
 * The value is stored and used by start_stepping() and move_steps().
 * If velocity stepping is already running, the new rate is applied live
 * (this is the hook a future ramp module will use).
 */
esp_err_t tmc2209_set_step_rate(tmc2209_dev_t *dev, uint32_t steps_per_sec);

/** @brief Start continuous stepping at the stored rate. Rate must be > 0. */
esp_err_t tmc2209_start_stepping(tmc2209_dev_t *dev);

/** @brief Stop any stepping immediately. STEP idles low afterwards. */
esp_err_t tmc2209_stop_stepping(tmc2209_dev_t *dev);

/**
 * @brief Move exactly `steps` microsteps, then stop (non-blocking).
 *
 * The sign selects the direction: positive = forward, negative = reverse.
 * Uses the stored step rate, so call tmc2209_set_step_rate() first.
 * Example: tmc2209_move_steps(&motor, 256) moves exactly 256 microsteps.
 *
 * @param steps Number of microsteps (sign = direction)
 */
esp_err_t tmc2209_move_steps(tmc2209_dev_t *dev, int32_t steps);

/** @brief Check whether a counted move is still running. */
esp_err_t tmc2209_stepdir_is_busy(tmc2209_dev_t *dev, bool *busy);

// ============================================================
// Diagnostics & Status
// ============================================================

/** @brief Get the raw DRV_STATUS register value. */
esp_err_t tmc2209_get_driver_status(tmc2209_dev_t *dev, uint32_t *status);

/** @brief Check if the driver has flagged an error. */
esp_err_t tmc2209_has_error(tmc2209_dev_t *dev, bool *has_error);

/** @brief Check if the motor is currently stalled. */
esp_err_t tmc2209_is_stalled(tmc2209_dev_t *dev, bool *is_stalled);

/** @brief Check if the driver is over temperature. */
esp_err_t tmc2209_is_overtemp(tmc2209_dev_t *dev, bool *overtemp);

/** @brief Clear all latched error flags in the GSTAT register. */
esp_err_t tmc2209_clear_errors(tmc2209_dev_t *dev);

#ifdef __cplusplus
}
#endif