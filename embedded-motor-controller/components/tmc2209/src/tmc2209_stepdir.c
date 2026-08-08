//
// Created by 99sun on 08-08-2026.
//

#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_rom_sys.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "tmc2209_dev.h"
#include "TMC2209.h"

static const char *TAG = "tmc2209_stepdir";

// 1 MHz RMT resolution → 1 tick = 1 µs
#define STEPDIR_RESOLUTION_HZ  1000000u
// Max steps per RMT chunk (also the RMT memory block size)
#define STEPDIR_BUF_MAX       64u
// Rate limits imposed by the 15-bit symbol duration fields
#define STEPDIR_MIN_RATE       16u       // steps/s
#define STEPDIR_MAX_RATE       100000u   // steps/s
// Sentinel meaning "step forever" (velocity mode)
#define STEPDIR_INFINITE       INT64_MAX

// ============================================================
// Private pulse-engine state (defined here, hidden from header)
// ============================================================

struct tmc2209_stepdir {
    rmt_channel_handle_t chan;      // created once, never per move
    rmt_encoder_handle_t enc;       // copy encoder
    gpio_num_t step_pin;
    gpio_num_t dir_pin;

    uint32_t rate;                  // stored step rate (steps/s)
    volatile uint32_t period;       // full step period in ticks (µs)
    volatile int64_t remaining;     // steps left; INFINITE = velocity mode
    volatile bool running;          // pulse train active
    volatile bool busy;             // a counted move is active

    rmt_symbol_word_t symbols[STEPDIR_BUF_MAX];  // one symbol = one step
};

// ============================================================
// Internal helpers
// ============================================================

/** Full step period in ticks for a given rate. */
static uint32_t IRAM_ATTR rate_to_period(uint32_t rate)
{
    return STEPDIR_RESOLUTION_HZ / rate;
}

/**
 * How many steps to put in the next RMT chunk.
 * ~20 ms worth of steps, clamped to [1, BUF_MAX].
 *
 * Why adaptive?
 * - Bounds stop latency (~20 ms worst case) even at low rates.
 * - Gives a future ramp a chance to update the rate every ~20 ms.
 * - Keeps callback frequency sane at high rates.
 */
static uint32_t IRAM_ATTR chunk_steps(const struct tmc2209_stepdir *sd)
{
    uint32_t n = sd->rate / 50;   // rate steps/s / 50 = steps per 20 ms
    if (n < 1) n = 1;
    if (n > STEPDIR_BUF_MAX) n = STEPDIR_BUF_MAX;
    return n;
}

/** Fill the symbol buffer with n step pulses at the current period. */
static void IRAM_ATTR encode_chunk(struct tmc2209_stepdir *sd, uint32_t n)
{
    uint32_t half = sd->period / 2;
    if (half < 1) half = 1;

    for (uint32_t i = 0; i < n; i++) {
        // One RMT symbol = high half + low half = one STEP pulse
        sd->symbols[i].duration0 = half;
        sd->symbols[i].level0    = 1;
        sd->symbols[i].duration1 = half;
        sd->symbols[i].level1    = 0;
    }
}

/** Transmit n steps on the (persistent) channel. */
static esp_err_t IRAM_ATTR transmit_chunk(struct tmc2209_stepdir *sd, uint32_t n)
{
    encode_chunk(sd, n);

    rmt_transmit_config_t cfg = {
        .loop_count = 0,
        .flags.eot_level = 0,   // STEP idles low after the chunk
    };
    return rmt_transmit(sd->chan, sd->enc, sd->symbols,
                        n * sizeof(rmt_symbol_word_t), &cfg);
}

// ============================================================
// The refill callback (ISR context!)
//
// Fires each time the hardware finishes the current chunk.
// We simply encode and transmit the next chunk at the CURRENT
// rate. This one loop serves velocity mode AND counted moves.
// ============================================================
static bool IRAM_ATTR stepdir_tx_done_cb(rmt_channel_handle_t chan,
                                         const rmt_tx_done_event_data_t *edata,
                                         void *user_ctx)
{
    struct tmc2209_stepdir *sd = (struct tmc2209_stepdir *)user_ctx;

    // Stopped by the user: do not refill
    if (!sd->running) {
        return false;
    }

    uint32_t n = chunk_steps(sd);

    if (sd->remaining != STEPDIR_INFINITE) {
        // Counted move: clamp to what is left, then finish exactly
        if (sd->remaining <= 0) {
            sd->running = false;
            sd->busy = false;
            return false;
        }
        if ((int64_t)n > sd->remaining) {
            n = (uint32_t)sd->remaining;
        }
        sd->remaining -= n;
    }

    transmit_chunk(sd, n);   // same channel, next chunk, current rate
    return false;
}

// ============================================================
// Internal cleanup helper (no goto, single place to free)
// ============================================================

static void stepdir_destroy(struct tmc2209_stepdir *sd)
{
    if (sd == NULL) {
        return;
    }
    if (sd->enc != NULL) {
        rmt_del_encoder(sd->enc);
    }
    if (sd->chan != NULL) {
        rmt_disable(sd->chan);
        rmt_del_channel(sd->chan);
    }
    free(sd);
}

// ============================================================
// Lifecycle
// ============================================================

esp_err_t tmc2209_stepdir_init(tmc2209_dev_t *dev)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (dev->stepdir != NULL) {
        ESP_LOGW(TAG, "stepdir already initialized");
        return ESP_OK;
    }

    const tmc2209_stepdir_config_t *cfg = &dev->stepdir_cfg;
    if (cfg->step_gpio < 0 || cfg->dir_gpio < 0) {
        ESP_LOGE(TAG, "step/dir GPIOs not configured (set them in tmc2209_config_t)");
        return ESP_ERR_INVALID_ARG;
    }

    struct tmc2209_stepdir *sd = calloc(1, sizeof(*sd));
    if (sd == NULL) {
        return ESP_ERR_NO_MEM;
    }

    sd->step_pin = (gpio_num_t)cfg->step_gpio;
    sd->dir_pin  = (gpio_num_t)cfg->dir_gpio;

    // DIR pin: plain output, idle at forward
    esp_err_t err = gpio_set_direction(sd->dir_pin, GPIO_MODE_OUTPUT);
    if (err != ESP_OK) { stepdir_destroy(sd); return err; }
    gpio_set_level(sd->dir_pin, 0);

    // RMT TX channel: created ONCE, reused for every move
    rmt_tx_channel_config_t chan_cfg = {
        .gpio_num = sd->step_pin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = STEPDIR_RESOLUTION_HZ,
        .mem_block_symbols = STEPDIR_BUF_MAX,
        .trans_queue_depth = 4,
    };
    err = rmt_new_tx_channel(&chan_cfg, &sd->chan);
    if (err != ESP_OK) { stepdir_destroy(sd); return err; }

    // Copy encoder: transmits our symbol buffer verbatim
    rmt_copy_encoder_config_t enc_cfg;
    err = rmt_new_copy_encoder(&enc_cfg, &sd->enc);
    if (err != ESP_OK) { stepdir_destroy(sd); return err; }

    // Refill callback
    rmt_tx_event_callbacks_t cbs = {
        .on_trans_done = stepdir_tx_done_cb,
    };
    err = rmt_tx_register_event_callbacks(sd->chan, &cbs, sd);
    if (err != ESP_OK) { stepdir_destroy(sd); return err; }

    err = rmt_enable(sd->chan);
    if (err != ESP_OK) { stepdir_destroy(sd); return err; }

    // Force the TMC2209 to listen to the STEP pin (VACTUAL = 0)
    if (dev->mutex != NULL) {
        xSemaphoreTake(dev->mutex, portMAX_DELAY);
    }
    tmc2209_writeRegister(dev->ic_id, TMC2209_VACTUAL, 0);
    if (dev->mutex != NULL) {
        xSemaphoreGive(dev->mutex);
    }

    dev->stepdir = sd;
    ESP_LOGI(TAG, "stepdir initialized (step=%d, dir=%d)", cfg->step_gpio, cfg->dir_gpio);
    return ESP_OK;
}

esp_err_t tmc2209_stepdir_deinit(tmc2209_dev_t *dev)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    struct tmc2209_stepdir *sd = dev->stepdir;
    if (sd == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    sd->running = false;   // callback will stop refilling
    sd->busy = false;

    gpio_num_t step_pin = sd->step_pin;
    stepdir_destroy(sd);
    dev->stepdir = NULL;

    // Release the pin and drive it low so STEP never floats
    gpio_set_direction(step_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(step_pin, 0);

    ESP_LOGI(TAG, "stepdir deinitialized");
    return ESP_OK;
}

// ============================================================
// Direction & rate
// ============================================================

esp_err_t tmc2209_set_direction(tmc2209_dev_t *dev, bool forward)
{
    if (dev == NULL || dev->stepdir == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    struct tmc2209_stepdir *sd = dev->stepdir;

    gpio_set_level(sd->dir_pin, forward ? 1 : 0);
    esp_rom_delay_us(2);   // DIR setup time before the next step edge
    return ESP_OK;
}

esp_err_t tmc2209_set_step_rate(tmc2209_dev_t *dev, uint32_t steps_per_sec)
{
    if (dev == NULL || dev->stepdir == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    struct tmc2209_stepdir *sd = dev->stepdir;

    if (steps_per_sec < STEPDIR_MIN_RATE) steps_per_sec = STEPDIR_MIN_RATE;
    if (steps_per_sec > STEPDIR_MAX_RATE) steps_per_sec = STEPDIR_MAX_RATE;

    sd->rate = steps_per_sec;
    sd->period = rate_to_period(steps_per_sec);
    // If stepping is running, the next chunk automatically uses the new
    // period. This is the live-update hook a ramp will use.
    return ESP_OK;
}

// ============================================================
// Starting / stopping
// ============================================================

esp_err_t tmc2209_start_stepping(tmc2209_dev_t *dev)
{
    if (dev == NULL || dev->stepdir == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    struct tmc2209_stepdir *sd = dev->stepdir;

    if (sd->rate == 0) {
        ESP_LOGE(TAG, "call tmc2209_set_step_rate() first");
        return ESP_ERR_INVALID_STATE;
    }
    if (sd->busy) {
        ESP_LOGE(TAG, "a counted move is already running");
        return ESP_ERR_INVALID_STATE;
    }

    sd->remaining = STEPDIR_INFINITE;   // velocity mode
    sd->busy = false;
    sd->running = true;

    return transmit_chunk(sd, chunk_steps(sd));   // kick the loop
}

esp_err_t tmc2209_stop_stepping(tmc2209_dev_t *dev)
{
    if (dev == NULL || dev->stepdir == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    struct tmc2209_stepdir *sd = dev->stepdir;

    sd->running = false;   // callback stops refilling after current chunk
    sd->busy = false;
    sd->remaining = 0;
    return ESP_OK;
}

esp_err_t tmc2209_move_steps(tmc2209_dev_t *dev, int32_t steps)
{
    if (dev == NULL || dev->stepdir == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    struct tmc2209_stepdir *sd = dev->stepdir;

    if (steps == 0) {
        return ESP_OK;
    }
    if (sd->rate == 0) {
        ESP_LOGE(TAG, "call tmc2209_set_step_rate() first");
        return ESP_ERR_INVALID_STATE;
    }
    if (sd->running) {
        ESP_LOGE(TAG, "already stepping; stop first");
        return ESP_ERR_INVALID_STATE;
    }

    // Sign = direction
    bool forward = (steps > 0);
    gpio_set_level(sd->dir_pin, forward ? 1 : 0);
    esp_rom_delay_us(2);

    int64_t n = (steps > 0) ? (int64_t)steps : -(int64_t)steps;

    sd->remaining = n;     // counted move
    sd->busy = true;
    sd->running = true;

    return transmit_chunk(sd, chunk_steps(sd));
}

esp_err_t tmc2209_stepdir_is_busy(tmc2209_dev_t *dev, bool *busy)
{
    if (dev == NULL || busy == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (dev->stepdir == NULL) {
        *busy = false;
        return ESP_ERR_INVALID_STATE;
    }
    *busy = dev->stepdir->busy;
    return ESP_OK;
}