#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "tmc2209_dev.h"
#include "driver/uart.h"

static const char *TAG = "TMC_TEST";

// Forward declarations
static void motor_test_no_movement();
static void motor_test_movement();
static void motor_test_ramp();
static void motor_test_stepdir();  // <-- NEW

void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    printf("Doing motor test now\n");

    // ============================================================
    // SWAP THE TEST HERE:
    // ============================================================
    // motor_test_no_movement();
    // motor_test_movement();
    // motor_test_ramp();
    motor_test_stepdir();

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
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
            .step_gpio = 5,   // Wire TMC2209 STEP pin to GPIO 5
            .dir_gpio = 6,    // Wire TMC2209 DIR pin to GPIO 6
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