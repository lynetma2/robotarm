#pragma once

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle to a potentiometer instance.
 *        The struct is defined privately inside pot.c.
 */
typedef struct pot_t pot_t;

/**
 * @brief Potentiometer configuration.
 */
typedef struct {
    adc_unit_t unit;        // Which ADC peripheral (usually ADC_UNIT_1)
    adc_channel_t channel;  // e.g. ADC1_CHANNEL_0 = GPIO1 on the ESP32-S3
    uint8_t samples;        // Readings averaged per read call (0 = 1)
} pot_config_t;

/**
 * @brief Create a potentiometer instance.
 *
 * Configures the ADC channel at 12 dB attenuation (full 0..3.3V range)
 * and creates a calibration handle when supported. If calibration is
 * unavailable, falls back to raw counts with a warning (never fails).
 *
 * @param config      Configuration
 * @param out_handle  Receives the new handle on success
 */
esp_err_t pot_create(const pot_config_t *config, pot_t **out_handle);

/**
 * @brief Read the knob position as a smoothed value between 0.0 and 1.0.
 */
esp_err_t pot_read(pot_t *handle, float *normalized);

/**
 * @brief Read the smoothed pin voltage in millivolts.
 */
esp_err_t pot_read_mv(pot_t *handle, int *millivolts);

/**
 * @brief Read the smoothed raw ADC count (0..4095 at 12-bit resolution).
 */
esp_err_t pot_read_raw(pot_t *handle, int *raw);

/**
 * @brief Destroy the instance and release the ADC hardware.
 */
esp_err_t pot_destroy(pot_t *handle);

#ifdef __cplusplus
}
#endif