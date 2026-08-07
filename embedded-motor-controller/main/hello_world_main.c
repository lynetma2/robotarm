/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "tmc2209_dev.h"
#include "driver/uart.h"

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

    // 1. Initialize the UART bus once
    tmc2209_bus_config_t bus = {
        .uart_port = UART_NUM_1,
        .tx_pin = 17,
        .rx_pin = 16,
        .baud_rate = 115200,
        .discard_echo = true,
        .timeout_ms = 20,
    };
    ESP_ERROR_CHECK(tmc2209_init_bus(&bus));

    // 2. Initialize one motor
    tmc2209_dev_t motor;
    tmc2209_config_t cfg = {
        .uart_port = UART_NUM_1,
        .ic_id = 0,
        .r_sense_mohm = 110,
        .node_address = 0,
    };
    ESP_ERROR_CHECK(tmc2209_init(&motor, &cfg));

    // 3. Configure it
    ESP_ERROR_CHECK(tmc2209_set_run_current(&motor, 1000));
    ESP_ERROR_CHECK(tmc2209_set_microsteps(&motor, 16));

    printf("TMC2209 component is working!\n");

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
