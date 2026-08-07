//
// Created by 99sun on 07-08-2026.
//

#include <string.h>
#include <math.h>
#include "tmc2209_dev.h"
#include "tmc2209_port.h"
#include "TMC2209.h"        // The vendor header (Layer 1)
#include "esp_log.h"

static const char *TAG = "tmc2209_dev";

// ============================================================
// Internal Helpers: Current Calculation
// ============================================================

/**
 * @brief Calculates the IRUN register value and the required VSENSE bit.
 *
 * The TMC2209 datasheet formula is:
 * IRUN = (I_rms * 32 * sqrt(2) * (R_sense + R_internal)) / V_FS - 1
 *
 * Where:
 * - R_internal is ~0.02 ohms (20 milliohms)
 * - V_FS is 0.325V if VSENSE=0 (low sensitivity)
 * - V_FS is 0.180V if VSENSE=1 (high sensitivity)
 *
 * @param current_ma     Desired RMS current in milliamps
 * @param r_sense_mohm   Sense resistor value in milliohms
 * @param out_high_vsense Pointer to store whether VSENSE should be 1
 * @return The 5-bit IRUN value (0-31)
 */
static uint8_t calculate_irun(uint32_t current_ma, uint16_t r_sense_mohm, bool *out_high_vsense)
{
    // 1. Setup variables
    float current_a = (float)current_ma / 1000.0f;
    float r_total = ((float)r_sense_mohm / 1000.0f) + 0.02f;

    // 2. Calculate using the default LOW sensitivity range (0.325V)
    float cs = (32.0f * 1.414213f * current_a * r_total) / 0.325f - 1.0f;

    // 3. Check the threshold
    if (cs < 16.0f) {
        // The current is low enough that we can switch to HIGH sensitivity
        // to get better resolution.
        *out_high_vsense = true;
        cs = (32.0f * 1.414213f * current_a * r_total) / 0.180f - 1.0f;
    } else {
        // The current is high enough that we must stay on LOW sensitivity
        // to avoid overflowing the 5-bit register.
        *out_high_vsense = false;
    }

    // 4. Clamp to valid 5-bit range (0 to 31)
    if (cs > 31.0f) cs = 31.0f;
    if (cs < 0.0f) cs = 0.0f;

    return (uint8_t)cs;
}

// ============================================================
// Internal Helpers: Microstep Conversion
// ============================================================

/**
 * @brief Converts a user-friendly microstep count to the MRES register value.
 *
 * The MRES field in CHOPCONF is inverted:
 * 0 = 256 microsteps, 1 = 128, ..., 8 = Full step
 *
 * @param microsteps Desired microstep resolution (1, 2, 4, 8, 16, 32, 64, 128, 256)
 * @return The 4-bit MRES register value
 */
static uint8_t microsteps_to_mres(uint16_t microsteps)
{
    switch (microsteps) {
    case 256: return 0;
    case 128: return 1;
    case 64:  return 2;
    case 32:  return 3;
    case 16:  return 4;
    case 8:   return 5;
    case 4:   return 6;
    case 2:   return 7;
    case 1:   return 8;
    default:
        ESP_LOGW(TAG, "Invalid microstep value %u, defaulting to 16", microsteps);
        return 4; // Default to 16 microsteps
    }
}

// ============================================================
// Lifecycle
// ============================================================

esp_err_t tmc2209_init(tmc2209_dev_t *dev, const tmc2209_config_t *config)
{
    if (dev == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->r_sense_mohm == 0) {
        ESP_LOGE(TAG, "r_sense_mohm cannot be 0");
        return ESP_ERR_INVALID_ARG;
    }

    // 1. Copy configuration into the device struct
    dev->uart_port = config->uart_port;
    dev->ic_id = config->ic_id;
    dev->r_sense_mohm = config->r_sense_mohm;
    dev->node_address = config->node_address;

    // 2. Create the mutex to protect UART access for this specific motor
    if (dev->mutex == NULL) {
        dev->mutex = xSemaphoreCreateMutex();
        if (dev->mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex for icID %u", dev->ic_id);
            return ESP_ERR_NO_MEM;
        }
    }

    // 3. Register this icID, uart_port, and node_address with the port layer
    esp_err_t err = tmc2209_port_register(config->ic_id, config->uart_port, config->node_address);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register driver with port layer");
        return err;
    }

    ESP_LOGI(TAG, "Initialized TMC2209 (icID=%u, node_addr=%u, uart=%d)",
             dev->ic_id, dev->node_address, dev->uart_port);

    return ESP_OK;
}

esp_err_t tmc2209_deinit(tmc2209_dev_t *dev)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 1. Unregister from the port layer (ic_id is unique, so we don't need to pass uart_port)
    tmc2209_port_unregister(dev->ic_id);

    // 2. Free the mutex
    if (dev->mutex != NULL) {
        vSemaphoreDelete(dev->mutex);
        dev->mutex = NULL; // Set to NULL to allow safe re-initialization later
    }

    ESP_LOGI(TAG, "Deinitialized TMC2209 (icID=%u, uart=%d)", dev->ic_id, dev->uart_port);

    return ESP_OK;
}

// ============================================================
// Current Control
// ============================================================

esp_err_t tmc2209_set_run_current(tmc2209_dev_t *dev, uint32_t current_ma)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // 1. Calculate the 5-bit register value and the VSENSE mode
    bool high_vsense = false;
    uint8_t irun_val = calculate_irun(current_ma, dev->r_sense_mohm, &high_vsense);

    // 2. Lock the mutex (prevents other tasks from interrupting our UART transactions)
    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for set_run_current");
        return ESP_ERR_TIMEOUT;
    }

    // 3. Update the VSENSE bit in the GCONF register
    // The vendor library handles the read-modify-write automatically!
    tmc2209_fieldWrite(dev->ic_id, TMC2209_VSENSE_FIELD, high_vsense ? 1 : 0);

    // 4. Update the IRUN field in the IHOLD_IRUN register
    tmc2209_fieldWrite(dev->ic_id, TMC2209_IRUN_FIELD, irun_val);

    // 5. Release the mutex
    xSemaphoreGive(dev->mutex);

    ESP_LOGI(TAG, "Set run current to %lu mA (IRUN=%u, VSENSE=%d)",
             (unsigned long)current_ma, irun_val, high_vsense);

    return ESP_OK;
}

esp_err_t tmc2209_set_hold_current(tmc2209_dev_t *dev, uint32_t current_ma)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // The math for hold current is exactly the same as run current,
    // we just write to the IHOLD field instead of IRUN.
    bool high_vsense = false;
    uint8_t ihold_val = calculate_irun(current_ma, dev->r_sense_mohm, &high_vsense);

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for set_hold_current");
        return ESP_ERR_TIMEOUT;
    }

    // Note: Technically VSENSE applies to both hold and run.
    // If the user sets a run current of 2000mA (low vsense) and a hold current of 10mA (high vsense),
    // the hardware can only be in one vsense state at a time.
    // In practice, hold current is usually a fraction of run current, so run current dictates vsense.
    // For safety, we won't touch vsense here, we just update the IHOLD field.

    tmc2209_fieldWrite(dev->ic_id, TMC2209_IHOLD_FIELD, ihold_val);

    xSemaphoreGive(dev->mutex);

    ESP_LOGI(TAG, "Set hold current to %lu mA (IHOLD=%u)",
             (unsigned long)current_ma, ihold_val);

    return ESP_OK;
}

// ============================================================
// Motor Configuration
// ============================================================

esp_err_t tmc2209_set_microsteps(tmc2209_dev_t *dev, uint16_t microsteps)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t mres_val = microsteps_to_mres(microsteps);

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Write the MRES field in the CHOPCONF register
    // The vendor library handles the read-modify-write automatically
    tmc2209_fieldWrite(dev->ic_id, TMC2209_MRES_FIELD, mres_val);

    xSemaphoreGive(dev->mutex);

    ESP_LOGI(TAG, "Set microsteps to %u (MRES=%u)", microsteps, mres_val);
    return ESP_OK;
}

esp_err_t tmc2209_set_stealthchop(tmc2209_dev_t *dev, bool enable)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // en_spreadcycle = 0 means StealthChop is ON
    // en_spreadcycle = 1 means StealthChop is OFF (SpreadCycle is ON)
    // So we invert the user's boolean: enable=true -> spreadcycle=0
    tmc2209_fieldWrite(dev->ic_id, TMC2209_EN_SPREADCYCLE_FIELD, enable ? 0 : 1);

    xSemaphoreGive(dev->mutex);

    ESP_LOGI(TAG, "StealthChop %s", enable ? "enabled" : "disabled");
    return ESP_OK;
}

esp_err_t tmc2209_is_stealthchop_enabled(tmc2209_dev_t *dev, bool *enabled)
{
    if (dev == NULL || enabled == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Read the en_spreadcycle bit from GCONF
    uint32_t spreadcycle_val = tmc2209_fieldRead(dev->ic_id, TMC2209_EN_SPREADCYCLE_FIELD);

    xSemaphoreGive(dev->mutex);

    // If spreadcycle is 0, stealthchop is enabled
    *enabled = (spreadcycle_val == 0);

    return ESP_OK;
}

esp_err_t tmc2209_set_standstill_mode(tmc2209_dev_t *dev, uint8_t mode)
{
    if (dev == NULL || dev->mutex == NULL) return ESP_ERR_INVALID_STATE;
    if (mode > 3) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) return ESP_ERR_TIMEOUT;

    tmc2209_fieldWrite(dev->ic_id, TMC2209_FREEWHEEL_FIELD, mode);

    xSemaphoreGive(dev->mutex);
    return ESP_OK;
}

// ============================================================
// StallGuard (Sensorless Homing / Stall Detection)
// ============================================================

esp_err_t tmc2209_set_stallguard_enabled(tmc2209_dev_t *dev, bool enabled)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (enabled) {
        // To enable StallGuard, we set TCOOLTHRS to a reasonable default.
        // StallGuard is only active when velocity > TCOOLTHRS.
        // A typical starting value is around 50000, but the user should
        // tune this with tmc2209_set_stallguard_velocity_threshold().
        tmc2209_writeRegister(dev->ic_id, TMC2209_TCOOLTHRS, 50000);
        ESP_LOGI(TAG, "StallGuard enabled (TCOOLTHRS=50000)");
    } else {
        // To disable StallGuard, set TCOOLTHRS to 0
        tmc2209_writeRegister(dev->ic_id, TMC2209_TCOOLTHRS, 0);
        ESP_LOGI(TAG, "StallGuard disabled (TCOOLTHRS=0)");
    }

    xSemaphoreGive(dev->mutex);
    return ESP_OK;
}

esp_err_t tmc2209_set_stallguard_threshold(tmc2209_dev_t *dev, int8_t threshold)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // SGTHRS is an 8-bit register (0-255).
    // We cast the int8_t to uint8_t to handle the full range.
    uint8_t sgthrs_val = (uint8_t)threshold;

    tmc2209_writeRegister(dev->ic_id, TMC2209_SGTHRS, sgthrs_val);

    xSemaphoreGive(dev->mutex);

    ESP_LOGI(TAG, "Set StallGuard threshold to %d (register value: %u)",
             threshold, sgthrs_val);
    return ESP_OK;
}

esp_err_t tmc2209_set_stallguard_velocity_threshold(tmc2209_dev_t *dev, uint32_t velocity)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // TCOOLTHRS is a 20-bit register
    // Clamp to 20-bit max value (0xFFFFF = 1048575)
    if (velocity > 0xFFFFF) {
        velocity = 0xFFFFF;
    }

    tmc2209_writeRegister(dev->ic_id, TMC2209_TCOOLTHRS, velocity);

    xSemaphoreGive(dev->mutex);

    ESP_LOGI(TAG, "Set StallGuard velocity threshold to %lu", (unsigned long)velocity);
    return ESP_OK;
}

esp_err_t tmc2209_get_stallguard_result(tmc2209_dev_t *dev, uint16_t *result)
{
    if (dev == NULL || result == NULL || dev->mutex == NULL) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) return ESP_ERR_TIMEOUT;

    uint32_t raw = tmc2209_readRegister(dev->ic_id, TMC2209_SG_RESULT);
    *result = (uint16_t)(raw & 0x3FF);

    xSemaphoreGive(dev->mutex);
    return ESP_OK;
}

// ============================================================
// Internal Motion Controller (Velocity Only)
// ============================================================

esp_err_t tmc2209_set_internal_velocity(tmc2209_dev_t *dev, int32_t velocity)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // VACTUAL is a 24-bit signed register.
    // Clamp to valid range: -8388608 to +8388607
    if (velocity > 8388607) velocity = 8388607;
    if (velocity < -8388608) velocity = -8388608;

    // Store the velocity in the struct.
    // We don't write to hardware yet — that happens in start_internal_motion().
    dev->internal_velocity = velocity;

    ESP_LOGI(TAG, "Internal velocity set to %ld (not yet applied)", (long)velocity);
    return ESP_OK;
}

esp_err_t tmc2209_start_internal_motion(tmc2209_dev_t *dev)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (dev->internal_velocity == 0) {
        ESP_LOGW(TAG, "Cannot start internal motion: velocity is 0. Call set_internal_velocity first.");
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Write the stored velocity to VACTUAL.
    // The TMC2209 will immediately start generating STEP pulses.
    tmc2209_writeRegister(dev->ic_id, TMC2209_VACTUAL, (uint32_t)dev->internal_velocity);

    xSemaphoreGive(dev->mutex);

    ESP_LOGI(TAG, "Internal motion started (velocity=%ld)", (long)dev->internal_velocity);
    return ESP_OK;
}

esp_err_t tmc2209_stop_internal_motion(tmc2209_dev_t *dev)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Write 0 to VACTUAL to stop the internal pulse generator
    tmc2209_writeRegister(dev->ic_id, TMC2209_VACTUAL, 0);

    xSemaphoreGive(dev->mutex);

    ESP_LOGI(TAG, "Internal motion stopped");
    return ESP_OK;
}

// ============================================================
// Step/Dir Mode (External Pulse Generation)
// ============================================================

esp_err_t tmc2209_enable_step_dir(tmc2209_dev_t *dev)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // 1. Stop internal motion (ensure VACTUAL is 0)
    tmc2209_writeRegister(dev->ic_id, TMC2209_VACTUAL, 0);

    // 2. Enable the STEP/DIR interface by clearing pdn_disable in GCONF
    // pdn_disable = 0 means STEP/DIR pins are active
    tmc2209_fieldWrite(dev->ic_id, TMC2209_PDN_DISABLE_FIELD, 0);

    xSemaphoreGive(dev->mutex);

    ESP_LOGI(TAG, "Step/Dir mode enabled");
    return ESP_OK;
}

esp_err_t tmc2209_disable_step_dir(tmc2209_dev_t *dev)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Disable the STEP/DIR interface by setting pdn_disable in GCONF
    // pdn_disable = 1 means STEP/DIR pins are ignored (UART control only)
    tmc2209_fieldWrite(dev->ic_id, TMC2209_PDN_DISABLE_FIELD, 1);

    xSemaphoreGive(dev->mutex);

    ESP_LOGI(TAG, "Step/Dir mode disabled");
    return ESP_OK;
}

// ============================================================
// Diagnostics & Status
// ============================================================

esp_err_t tmc2209_get_driver_status(tmc2209_dev_t *dev, uint32_t *status)
{
    if (dev == NULL || status == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Read the entire DRV_STATUS register (32 bits)
    *status = tmc2209_readRegister(dev->ic_id, TMC2209_DRV_STATUS);

    xSemaphoreGive(dev->mutex);
    return ESP_OK;
}

esp_err_t tmc2209_has_error(tmc2209_dev_t *dev, bool *has_error)
{
    if (dev == NULL || has_error == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Read DRV_STATUS to check for real-time errors
    uint32_t drv_status = tmc2209_readRegister(dev->ic_id, TMC2209_DRV_STATUS);

    // Read GSTAT to check for latched errors
    uint32_t gstat = tmc2209_readRegister(dev->ic_id, TMC2209_GSTAT);

    xSemaphoreGive(dev->mutex);

    // Check for any error condition:
    // - Overtemperature shutdown (ot)
    // - Short to ground (s2ga or s2gb)
    // - Open load (ola or olb)
    // - Latched driver error (drv_err)
    // - Undervoltage on charge pump (uv_cp)

    bool ot = (drv_status >> 17) & 0x1;       // Overtemperature
    bool s2ga = (drv_status >> 19) & 0x1;     // Short to ground A
    bool s2gb = (drv_status >> 20) & 0x1;     // Short to ground B
    bool ola = (drv_status >> 21) & 0x1;      // Open load A
    bool olb = (drv_status >> 22) & 0x1;      // Open load B
    bool drv_err = (gstat >> 1) & 0x1;        // Latched driver error
    bool uv_cp = (gstat >> 2) & 0x1;          // Undervoltage charge pump

    *has_error = (ot || s2ga || s2gb || ola || olb || drv_err || uv_cp);

    if (*has_error) {
        ESP_LOGW(TAG, "Error detected! DRV_STATUS=0x%08lX, GSTAT=0x%08lX",
                 (unsigned long)drv_status, (unsigned long)gstat);
    }

    return ESP_OK;
}

esp_err_t tmc2209_is_stalled(tmc2209_dev_t *dev, bool *is_stalled)
{
    if (dev == NULL || is_stalled == NULL || dev->mutex == NULL) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) return ESP_ERR_TIMEOUT;

    uint32_t sg_result = tmc2209_readRegister(dev->ic_id, TMC2209_SG_RESULT) & 0x3FF;
    uint32_t sgthrs = tmc2209_readRegister(dev->ic_id, TMC2209_SGTHRS) & 0xFF;

    // A stall occurs when the measured load is less than or equal to the threshold
    *is_stalled = (sg_result <= sgthrs);

    xSemaphoreGive(dev->mutex);
    return ESP_OK;
}

esp_err_t tmc2209_is_overtemp(tmc2209_dev_t *dev, bool *overtemp)
{
    if (dev == NULL || overtemp == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Read the ot (overtemperature) flag from DRV_STATUS (bit 17)
    uint32_t ot_flag = tmc2209_fieldRead(dev->ic_id, TMC2209_OT_FIELD);

    xSemaphoreGive(dev->mutex);

    *overtemp = (ot_flag != 0);

    if (*overtemp) {
        ESP_LOGE(TAG, "OVERTEMPERATURE DETECTED! Motor driver has shut down.");
    }

    return ESP_OK;
}

esp_err_t tmc2209_clear_errors(tmc2209_dev_t *dev)
{
    if (dev == NULL || dev->mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // GSTAT uses "write-1-to-clear" behavior.
    // To clear all latched error flags, we read the current GSTAT
    // and write it back. This clears any flags that are set.
    uint32_t gstat = tmc2209_readRegister(dev->ic_id, TMC2209_GSTAT);

    // Write the error bits back to clear them
    // We only write bits that are actually set (to avoid accidentally triggering something)
    if (gstat != 0) {
        tmc2209_writeRegister(dev->ic_id, TMC2209_GSTAT, gstat);
        ESP_LOGI(TAG, "Cleared GSTAT error flags (was 0x%08lX)", (unsigned long)gstat);
    }

    xSemaphoreGive(dev->mutex);
    return ESP_OK;
}

// ============================================================
// Bus Lifecycle (public wrappers around the private port layer)
// ============================================================

esp_err_t tmc2209_init_bus(const tmc2209_bus_config_t *config)
{
    // All the real work happens in the port layer
    return tmc2209_port_init_bus(config);
}

esp_err_t tmc2209_deinit_bus(void)
{
    return tmc2209_port_deinit();
}