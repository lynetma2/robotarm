#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "tmc2209_dev.h"
#include "driver/uart.h"

static const char *TAG = "TMC_TEST";

// Forward declarations
static void motor_test_no_movement();
static void motor_test_movement();
static void motor_test_ramp();   // <-- NEW

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
    // Comment out one, and uncomment the other to switch tests.
    // ============================================================
    // motor_test_no_movement();
    // motor_test_movement();
    motor_test_ramp();

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

    ESP_LOGI(TAG, "Setup complete. Motor should be holding position now.");

    // Keep the app running so you can feel the motor torque by hand
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================================
// NEW MOTION TEST FUNCTION
// ============================================================
void motor_test_movement()
{
    // --- ENABLE THE MOTOR DRIVER ---
    ESP_LOGI(TAG, "Enabling TMC2209 via GPIO 4...");
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_4, 0); // 0 = LOW = ENABLE the driver
    vTaskDelay(pdMS_TO_TICKS(100)); // Give the bridges 100ms to wake up
    // ------------------------------------

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

    // IMPORTANT: Disable step/dir so the TMC2209 listens to internal velocity commands
    tmc2209_disable_step_dir(&motor);

    ESP_LOGI(TAG, "Setup complete. Starting motion test...");

    // 5. Motion Test Sequence
    // We run the forward/stop/backward/stop sequence twice
    for (int i = 0; i < 2; i++) {
        ESP_LOGI(TAG, "Moving forward for 2 seconds...");
        tmc2209_set_internal_velocity(&motor, 2000); // Moderate speed
        tmc2209_start_internal_motion(&motor);
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Stopping...");
        tmc2209_stop_internal_motion(&motor);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Moving backward for 2 seconds...");
        tmc2209_set_internal_velocity(&motor, -2000); // Negative for reverse
        tmc2209_start_internal_motion(&motor);
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Stopping...");
        tmc2209_stop_internal_motion(&motor);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Motion test complete.");
    // Function returns here, which goes back to app_main() for the 10-second countdown
}

// ============================================================
// RAMP HELPER
// Gradually changes velocity from 'from' to 'to' in steps of 'step',
// waiting 'delay_ms' between each step.
// ============================================================
static void ramp_velocity(tmc2209_dev_t *motor, int32_t from, int32_t to,
                          int32_t step, uint32_t delay_ms)
{
    int32_t v = from;
    int32_t dir = (to > from) ? step : -step;

    while ((dir > 0 && v < to) || (dir < 0 && v > to)) {
        v += dir;

        // Clamp so we don't overshoot the target
        if (dir > 0 && v > to) v = to;
        if (dir < 0 && v < to) v = to;

        tmc2209_set_internal_velocity(motor, v);
        tmc2209_start_internal_motion(motor);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

// ============================================================
// HIGH SPEED RAMP TEST
// ============================================================
void motor_test_ramp()
{
    // --- ENABLE THE MOTOR DRIVER ---
    ESP_LOGI(TAG, "Enabling TMC2209 via GPIO 4...");
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_4, 0); // 0 = LOW = ENABLE the driver
    vTaskDelay(pdMS_TO_TICKS(100)); // Give the bridges 100ms to wake up
    // ------------------------------------

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
    tmc2209_set_run_current(&motor, 1000);   // A bit more current for high-speed torque
    tmc2209_set_hold_current(&motor, 500);
    tmc2209_set_microsteps(&motor, 16);
    tmc2209_set_stealthchop(&motor, false);  // SpreadCycle: more torque at high speed
    tmc2209_disable_step_dir(&motor);

    // Ramp parameters
    const int32_t start_v   = 2000;    // Safe starting velocity
    const int32_t top_v     = 20000;   // Target high velocity
    const int32_t ramp_step = 500;     // Velocity increase per step
    const uint32_t ramp_delay = 25;    // ms between steps

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