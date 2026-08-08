//
// Created by 99sun on 08-08-2026.
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "tmc2209_dev.h"
#include "app_buttons.h"
#include "motor_tests.h"

static const char *TAG = "TMC_TEST";

// ... paste ramp_velocity, motor_test_no_movement, motor_test_movement,
//     motor_test_ramp, motor_test_stepdir here (bodies unchanged) ...

// The button test becomes tiny now:
void motor_test_buttons(void)
{
    ESP_ERROR_CHECK(app_buttons_init());
    ESP_LOGI(TAG, "Button test running - press the buttons!");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void motor_test_no_movement()
{
    // 1. Setup UART Bus
    tmc2209_bus_config_t bus_cfg = {
        .uart_port = UART_NUM_2,
        .tx_pin = 8,
        .rx_pin = 7,
        .baud_rate = 115200,
        .discard_echo = true,
        .timeout_ms = 20,
    };
    ESP_ERROR_CHECK(tmc2209_init_bus(&bus_cfg));

    // 2. Setup Motor Instance
    tmc2209_dev_t motor = {0};
    tmc2209_config_t motor_cfg = {
        .uart_port = UART_NUM_2,
        .ic_id = 0,
        .r_sense_mohm = 110,
        .node_address = 0,
        .enable_gpio = 4,  // REFACTORED: now uses library function
    };
    ESP_ERROR_CHECK(tmc2209_init(&motor, &motor_cfg));

    // 3. PROVE COMMUNICATION
    uint32_t drv_status = 0;
    esp_err_t err = tmc2209_get_driver_status(&motor, &drv_status);

    if (err == ESP_OK && drv_status != 0x00000000) {
        ESP_LOGI(TAG, "SUCCESS! UART is working. DRV_STATUS = 0x%08lX", (unsigned long)drv_status);
    } else {
        ESP_LOGE(TAG, "FAILED! UART read timeout or chip did not reply.");
        return;
    }

    // 4. Basic Configuration
    ESP_LOGI(TAG, "Setting safe defaults...");
    tmc2209_set_run_current(&motor, 800);
    tmc2209_set_hold_current(&motor, 400);
    tmc2209_set_microsteps(&motor, 16);
    tmc2209_set_stealthchop(&motor, true);

    // Enable the driver
    tmc2209_set_enabled(&motor, true);

    ESP_LOGI(TAG, "Setup complete. Motor should be holding position now.");

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void motor_test_movement()
{
    // 1. Setup UART Bus
    tmc2209_bus_config_t bus_cfg = {
        .uart_port = UART_NUM_2,
        .tx_pin = 8,
        .rx_pin = 7,
        .baud_rate = 115200,
        .discard_echo = true,
        .timeout_ms = 20,
    };
    ESP_ERROR_CHECK(tmc2209_init_bus(&bus_cfg));

    // 2. Setup Motor Instance
    tmc2209_dev_t motor = {0};
    tmc2209_config_t motor_cfg = {
        .uart_port = UART_NUM_2,
        .ic_id = 0,
        .r_sense_mohm = 110,
        .node_address = 0,
        .enable_gpio = 4,  // REFACTORED
    };
    ESP_ERROR_CHECK(tmc2209_init(&motor, &motor_cfg));

    // 3. PROVE COMMUNICATION
    uint32_t drv_status = 0;
    esp_err_t err = tmc2209_get_driver_status(&motor, &drv_status);

    if (err != ESP_OK || drv_status == 0x00000000) {
        ESP_LOGE(TAG, "FAILED! UART read timeout or chip did not reply.");
        return;
    }
    ESP_LOGI(TAG, "SUCCESS! UART is working. DRV_STATUS = 0x%08lX", (unsigned long)drv_status);

    // 4. Basic Configuration
    ESP_LOGI(TAG, "Setting safe defaults...");
    tmc2209_set_run_current(&motor, 800);
    tmc2209_set_hold_current(&motor, 400);
    tmc2209_set_microsteps(&motor, 16);
    tmc2209_set_stealthchop(&motor, true);

    // Enable the driver
    tmc2209_set_enabled(&motor, true);

    ESP_LOGI(TAG, "Setup complete. Starting motion test...");

    // 5. Motion Test Sequence
    for (int i = 0; i < 2; i++) {
        ESP_LOGI(TAG, "Moving forward for 2 seconds...");
        tmc2209_set_internal_velocity(&motor, 2000);
        tmc2209_start_internal_motion(&motor);
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Stopping...");
        tmc2209_stop_internal_motion(&motor);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Moving backward for 2 seconds...");
        tmc2209_set_internal_velocity(&motor, -2000);
        tmc2209_start_internal_motion(&motor);
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Stopping...");
        tmc2209_stop_internal_motion(&motor);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Motion test complete.");
}

static void ramp_velocity(tmc2209_dev_t *motor, int32_t from, int32_t to,
                          int32_t step, uint32_t delay_ms)
{
    int32_t v = from;
    int32_t dir = (to > from) ? step : -step;

    while ((dir > 0 && v < to) || (dir < 0 && v > to)) {
        v += dir;

        if (dir > 0 && v > to) v = to;
        if (dir < 0 && v < to) v = to;

        tmc2209_set_internal_velocity(motor, v);
        tmc2209_start_internal_motion(motor);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void motor_test_ramp()
{
    // 1. Setup UART Bus
    tmc2209_bus_config_t bus_cfg = {
        .uart_port = UART_NUM_2,
        .tx_pin = 8,
        .rx_pin = 7,
        .baud_rate = 115200,
        .discard_echo = true,
        .timeout_ms = 20,
    };
    ESP_ERROR_CHECK(tmc2209_init_bus(&bus_cfg));

    // 2. Setup Motor Instance
    tmc2209_dev_t motor = {0};
    tmc2209_config_t motor_cfg = {
        .uart_port = UART_NUM_2,
        .ic_id = 0,
        .r_sense_mohm = 110,
        .node_address = 0,
        .enable_gpio = 4,  // REFACTORED
    };
    ESP_ERROR_CHECK(tmc2209_init(&motor, &motor_cfg));

    // 3. PROVE COMMUNICATION
    uint32_t drv_status = 0;
    esp_err_t err = tmc2209_get_driver_status(&motor, &drv_status);
    if (err != ESP_OK || drv_status == 0x00000000) {
        ESP_LOGE(TAG, "FAILED! UART read timeout or chip did not reply.");
        return;
    }
    ESP_LOGI(TAG, "SUCCESS! UART is working. DRV_STATUS = 0x%08lX", (unsigned long)drv_status);

    // 4. Configuration tuned for HIGH SPEED
    ESP_LOGI(TAG, "Setting high-speed defaults...");
    tmc2209_set_run_current(&motor, 1000);
    tmc2209_set_hold_current(&motor, 500);
    tmc2209_set_microsteps(&motor, 16);
    tmc2209_set_stealthchop(&motor, false);

    // Enable the driver
    tmc2209_set_enabled(&motor, true);

    // Ramp parameters
    const int32_t start_v   = 2000;
    const int32_t top_v     = 20000;
    const int32_t ramp_step = 500;
    const uint32_t ramp_delay = 25;

    // --- FORWARD ---
    ESP_LOGI(TAG, "Ramping up to %ld...", (long)top_v);
    tmc2209_set_internal_velocity(&motor, start_v);
    tmc2209_start_internal_motion(&motor);
    ramp_velocity(&motor, start_v, top_v, ramp_step, ramp_delay);

    ESP_LOGI(TAG, "At top speed for 2 seconds");
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Ramping down...");
    ramp_velocity(&motor, top_v, start_v, ramp_step, ramp_delay);
    tmc2209_stop_internal_motion(&motor);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // --- REVERSE ---
    ESP_LOGI(TAG, "Ramping up in reverse...");
    tmc2209_set_internal_velocity(&motor, -start_v);
    tmc2209_start_internal_motion(&motor);
    ramp_velocity(&motor, -start_v, -top_v, ramp_step, ramp_delay);

    ESP_LOGI(TAG, "At top speed (reverse) for 2 seconds");
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Ramping down...");
    ramp_velocity(&motor, -top_v, -start_v, ramp_step, ramp_delay);
    tmc2209_stop_internal_motion(&motor);

    ESP_LOGI(TAG, "Ramp test complete.");
}

// ============================================================
// STEP/DIR TEST FUNCTION
// ============================================================
void motor_test_stepdir()
{
    // 1. Setup UART Bus
    tmc2209_bus_config_t bus_cfg = {
        .uart_port = UART_NUM_2,
        .tx_pin = 8,
        .rx_pin = 7,
        .baud_rate = 115200,
        .discard_echo = true,
        .timeout_ms = 20,
    };
    ESP_ERROR_CHECK(tmc2209_init_bus(&bus_cfg));

    // 2. Setup Motor Instance with step/dir pins configured
    tmc2209_dev_t motor = {0};
    tmc2209_config_t motor_cfg = {
        .uart_port = UART_NUM_2,
        .ic_id = 0,
        .r_sense_mohm = 110,
        .node_address = 0,
        .enable_gpio = 4,
        .stepdir = {
            .step_gpio = 1,   // Wire TMC2209 STEP pin to GPIO 1
            .dir_gpio = 2,    // Wire TMC2209 DIR pin to GPIO 2
        },
    };
    ESP_ERROR_CHECK(tmc2209_init(&motor, &motor_cfg));

    // 3. PROVE COMMUNICATION
    uint32_t drv_status = 0;
    esp_err_t err = tmc2209_get_driver_status(&motor, &drv_status);
    if (err != ESP_OK || drv_status == 0x00000000) {
        ESP_LOGE(TAG, "FAILED! UART read timeout or chip did not reply.");
        return;
    }
    ESP_LOGI(TAG, "SUCCESS! UART is working. DRV_STATUS = 0x%08lX", (unsigned long)drv_status);

    // 4. Configuration
    ESP_LOGI(TAG, "Setting defaults...");
    tmc2209_set_run_current(&motor, 800);
    tmc2209_set_hold_current(&motor, 400);
    tmc2209_set_microsteps(&motor, 16);
    tmc2209_set_stealthchop(&motor, true);

    // Initialize step/dir engine (also sets VACTUAL=0 so TMC2209 listens to STEP)
    ESP_ERROR_CHECK(tmc2209_stepdir_init(&motor));

    // Enable the driver
    tmc2209_set_enabled(&motor, true);

    ESP_LOGI(TAG, "Setup complete. Starting step/dir test...");

    // 5. Velocity Mode Test
    ESP_LOGI(TAG, "--- Velocity Mode ---");

    ESP_LOGI(TAG, "Forward at 2000 steps/s for 2 seconds");
    tmc2209_set_direction(&motor, true);
    tmc2209_set_step_rate(&motor, 2000);
    tmc2209_start_stepping(&motor);
    vTaskDelay(pdMS_TO_TICKS(2000));
    tmc2209_stop_stepping(&motor);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Backward at 2000 steps/s for 2 seconds");
    tmc2209_set_direction(&motor, false);
    tmc2209_set_step_rate(&motor, 2000);
    tmc2209_start_stepping(&motor);
    vTaskDelay(pdMS_TO_TICKS(2000));
    tmc2209_stop_stepping(&motor);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 6. Counted Move Test (exact steps)
    ESP_LOGI(TAG, "--- Counted Move Mode ---");

    ESP_LOGI(TAG, "Move exactly 256 steps forward at 1000 steps/s");
    tmc2209_set_step_rate(&motor, 1000);
    tmc2209_move_steps(&motor, 256);

    // Wait for the move to complete
    bool busy = true;
    while (busy) {
        tmc2209_stepdir_is_busy(&motor, &busy);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI(TAG, "256 steps complete");
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Move exactly 512 steps backward at 2000 steps/s");
    tmc2209_set_step_rate(&motor, 2000);
    tmc2209_move_steps(&motor, -512);  // negative = reverse

    busy = true;
    while (busy) {
        tmc2209_stepdir_is_busy(&motor, &busy);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI(TAG, "512 steps complete");
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 7. Cleanup
    ESP_LOGI(TAG, "Step/dir test complete.");
    tmc2209_stepdir_deinit(&motor);
}

// ============================================================
// JOG CONFIGURATION
// Change this value to adjust the jogging speed (microsteps/sec)
// ============================================================
#define JOG_VELOCITY 5000

// --- State tracking for jog buttons ---
static bool s_btn2_held = false;
static bool s_btn3_held = false;
static tmc2209_dev_t *s_motor_ptr = NULL;

// --- Helper to determine motor state based on buttons ---
static void update_jog_state(void)
{
    if (s_motor_ptr == NULL) return;

    if (s_btn2_held && !s_btn3_held) {
        // Only Button 2 is held -> Clockwise
        tmc2209_set_internal_velocity(s_motor_ptr, JOG_VELOCITY);
        tmc2209_start_internal_motion(s_motor_ptr);
    }
    else if (s_btn3_held && !s_btn2_held) {
        // Only Button 3 is held -> Counter-Clockwise
        tmc2209_set_internal_velocity(s_motor_ptr, -JOG_VELOCITY);
        tmc2209_start_internal_motion(s_motor_ptr);
    }
    else {
        // Neither pressed, OR both pressed -> Stop (safety)
        tmc2209_stop_internal_motion(s_motor_ptr);
    }
}

// --- Button Callbacks ---
static void on_jog_btn2_down(void *arg, void *data) {
    s_btn2_held = true;
    update_jog_state();
}
static void on_jog_btn2_up(void *arg, void *data) {
    s_btn2_held = false;
    update_jog_state();
}
static void on_jog_btn3_down(void *arg, void *data) {
    s_btn3_held = true;
    update_jog_state();
}
static void on_jog_btn3_up(void *arg, void *data) {
    s_btn3_held = false;
    update_jog_state();
}

// ============================================================
// JOG TEST FUNCTION
// ============================================================
void motor_test_jog(void)
{
    // 1. Setup UART Bus
    tmc2209_bus_config_t bus_cfg = {
        .uart_port = UART_NUM_2,
        .tx_pin = 8,
        .rx_pin = 7,
        .baud_rate = 115200,
        .discard_echo = true,
        .timeout_ms = 20,
    };
    ESP_ERROR_CHECK(tmc2209_init_bus(&bus_cfg));

    // 2. Setup Motor Instance
    tmc2209_dev_t motor = {0};
    tmc2209_config_t motor_cfg = {
        .uart_port = UART_NUM_2,
        .ic_id = 0,
        .r_sense_mohm = 110,
        .node_address = 0,
        .enable_gpio = 4,
    };
    ESP_ERROR_CHECK(tmc2209_init(&motor, &motor_cfg));

    // 3. Configure Motor
    ESP_LOGI(TAG, "Setting jog defaults...");
    tmc2209_set_run_current(&motor, 800);
    tmc2209_set_hold_current(&motor, 400);
    tmc2209_set_microsteps(&motor, 16);
    tmc2209_set_stealthchop(&motor, true);

    // Enable the motor driver outputs
    tmc2209_set_enabled(&motor, true);

    // 4. Initialize Buttons and link to motor
    ESP_ERROR_CHECK(app_buttons_init());

    // Save a pointer to the motor so the button callbacks can access it
    s_motor_ptr = &motor;

    // Register the jog callbacks for Button 2 and Button 3
    app_buttons_register_cb(APP_BTN_2, BUTTON_PRESS_DOWN, on_jog_btn2_down, NULL);
    app_buttons_register_cb(APP_BTN_2, BUTTON_PRESS_UP,   on_jog_btn2_up,   NULL);

    app_buttons_register_cb(APP_BTN_3, BUTTON_PRESS_DOWN, on_jog_btn3_down, NULL);
    app_buttons_register_cb(APP_BTN_3, BUTTON_PRESS_UP,   on_jog_btn3_up,   NULL);

    ESP_LOGI(TAG, "Jog test running. Hold Btn 2 (CW) or Btn 3 (CCW).");

    // Keep the task alive. The button library's internal task handles the callbacks.
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}