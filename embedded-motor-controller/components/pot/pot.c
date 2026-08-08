#include <stdlib.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "pot.h"

static const char *TAG = "pot";

// With 12 dB attenuation on the ESP32-S3, the usable range tops out
// a little below the rail. We normalize against this, not 3300 mV.
#define POT_FULL_SCALE_MV 3100

// ============================================================
// Private types (hidden from the public header)
// ============================================================

typedef enum {
    POT_CALI_NONE = 0,
    POT_CALI_CURVE,
    POT_CALI_LINE,
} pot_cali_scheme_t;

struct pot_t {
    adc_oneshot_unit_handle_t unit;    // owns the ADC peripheral
    adc_channel_t channel;
    adc_cali_handle_t cali;            // NULL if calibration unavailable
    pot_cali_scheme_t cali_scheme;     // which scheme created 'cali' at runtime
    uint8_t samples;                   // readings averaged per read call
    float avg_mv;                      // smoothed average in millivolts
    bool avg_valid;                    // becomes true after the first read
};

// ============================================================
// Internal helpers
// ============================================================

/**
 * @brief Try to create a calibration handle.
 *
 * Tries curve fitting first (most accurate where supported), then
 * line fitting. Remembers which scheme succeeded so the matching
 * delete function can be called later. If neither is available,
 * pot->cali stays NULL and we fall back to raw counts with a warning.
 */
static void create_calibration(struct pot_t *pot, adc_unit_t unit, adc_channel_t channel)
{
    pot->cali = NULL;
    pot->cali_scheme = POT_CALI_NONE;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t curve_cfg = {
        .unit_id = unit,
        .chan = channel,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    if (adc_cali_create_scheme_curve_fitting(&curve_cfg, &pot->cali) == ESP_OK) {
        pot->cali_scheme = POT_CALI_CURVE;
        ESP_LOGI(TAG, "Calibration: curve fitting");
        return;
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t line_cfg = {
        .unit_id = unit,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    if (adc_cali_create_scheme_line_fitting(&line_cfg, &pot->cali) == ESP_OK) {
        pot->cali_scheme = POT_CALI_LINE;
        ESP_LOGI(TAG, "Calibration: line fitting");
        return;
    }
#endif
}

/**
 * @brief Delete the calibration handle using the matching scheme-specific
 *        delete function (required by the ESP-IDF v6 API).
 */
static void delete_calibration(struct pot_t *pot)
{
    if (pot->cali == NULL) {
        return;
    }

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (pot->cali_scheme == POT_CALI_CURVE) {
        adc_cali_delete_scheme_curve_fitting(pot->cali);
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (pot->cali_scheme == POT_CALI_LINE) {
        adc_cali_delete_scheme_line_fitting(pot->cali);
    }
#endif

    pot->cali = NULL;
    pot->cali_scheme = POT_CALI_NONE;
}

/**
 * @brief Sample the ADC `samples` times and average the result.
 * @param out_mv Receives the averaged value in millivolts.
 * @return ESP_OK on success, or an ADC error.
 */
static esp_err_t sample_avg_mv(struct pot_t *pot, int *out_mv)
{
    int raw_sum = 0;

    for (uint8_t i = 0; i < pot->samples; i++) {
        int raw = 0;
        esp_err_t err = adc_oneshot_read(pot->unit, pot->channel, &raw);
        if (err != ESP_OK) {
            return err;
        }
        raw_sum += raw;
    }

    int raw_avg = raw_sum / pot->samples;

    if (pot->cali != NULL) {
        // Calibrated path: raw count -> real millivolts
        int mv = 0;
        esp_err_t err = adc_cali_raw_to_voltage(pot->cali, raw_avg, &mv);
        if (err != ESP_OK) {
            return err;
        }
        *out_mv = mv;
    } else {
        // Fallback path: scale raw counts directly
        *out_mv = (raw_avg * POT_FULL_SCALE_MV) / 4095;
    }
    return ESP_OK;
}

// ============================================================
// Public API
// ============================================================

esp_err_t pot_create(const pot_config_t *config, pot_t **out_handle)
{
    if (config == NULL || out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    struct pot_t *pot = calloc(1, sizeof(*pot));
    if (pot == NULL) {
        return ESP_ERR_NO_MEM;
    }

    pot->channel = config->channel;
    pot->samples = (config->samples == 0) ? 1 : config->samples;
    create_calibration(pot, config->unit, config->channel);

    // --- Create the ADC unit ---
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = config->unit,
    };
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &pot->unit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC unit");
        delete_calibration(pot);
        free(pot);
        return err;
    }

    // --- Configure the channel ---
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,        // full ~0..3.3V range
        .bitwidth = ADC_BITWIDTH_12,     // 0..4095
    };
    err = adc_oneshot_config_channel(pot->unit, config->channel, &chan_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel");
        adc_oneshot_del_unit(pot->unit);
        delete_calibration(pot);
        free(pot);
        return err;
    }

    if (pot->cali == NULL) {
        ESP_LOGW(TAG, "No calibration available, using raw counts (values may be imprecise)");
    }

    *out_handle = pot;

    // Log which physical GPIO this channel maps to — handy for wiring checks
    int io_num = -1;
    if (adc_oneshot_channel_to_io(config->unit, config->channel, &io_num) == ESP_OK) {
        ESP_LOGI(TAG, "Potentiometer created (unit %d, channel %d = GPIO %d, %u samples/read)",
                 config->unit, config->channel, io_num, (unsigned)pot->samples);
    } else {
        ESP_LOGI(TAG, "Potentiometer created (unit %d, channel %d, %u samples/read)",
                 config->unit, config->channel, (unsigned)pot->samples);
    }
    return ESP_OK;
}

esp_err_t pot_read(pot_t *handle, float *normalized)
{
    if (handle == NULL || normalized == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int mv = 0;
    esp_err_t err = sample_avg_mv(handle, &mv);
    if (err != ESP_OK) {
        return err;
    }

    // Exponential smoothing: first read seeds the average,
    // every later read blends 50% old / 50% new.
    if (!handle->avg_valid) {
        handle->avg_mv = (float)mv;
        handle->avg_valid = true;
    } else {
        handle->avg_mv = handle->avg_mv * 0.5f + (float)mv * 0.5f;
    }

    float f = handle->avg_mv / (float)POT_FULL_SCALE_MV;
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    *normalized = f;
    return ESP_OK;
}

esp_err_t pot_read_mv(pot_t *handle, int *millivolts)
{
    if (handle == NULL || millivolts == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return sample_avg_mv(handle, millivolts);
}

esp_err_t pot_read_raw(pot_t *handle, int *raw)
{
    if (handle == NULL || raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int raw_sum = 0;
    for (uint8_t i = 0; i < handle->samples; i++) {
        int r = 0;
        esp_err_t err = adc_oneshot_read(handle->unit, handle->channel, &r);
        if (err != ESP_OK) {
            return err;
        }
        raw_sum += r;
    }
    *raw = raw_sum / handle->samples;
    return ESP_OK;
}

esp_err_t pot_destroy(pot_t *handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    adc_oneshot_del_unit(handle->unit);
    delete_calibration(handle);
    free(handle);
    return ESP_OK;
}