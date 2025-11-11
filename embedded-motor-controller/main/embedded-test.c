/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <motor_control.h>
#include <serial_comms.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "cJSON.h"
#include "string.h"

static const char *TAG = "MOTOR_CONTROL";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Motor Control Application");

    // --- 1. Create the command queue ---
    for (int i = 0; i < NUM_MOTORS; i++) {
        motorQueues[i] = xQueueCreate(10, sizeof(MotorCommand)); // Queue can hold 10 commands
        if (motorQueues[i] == NULL) {
            ESP_LOGE(TAG, "Failed to create command queue for motor %d", i);
            return;
        }
    }

    // --- 2. Configure Motor 0 ---
    motors[0].id = 0;
    motors[0].en_gpio = 4;     // e.g., GPIO 4
    motors[0].dir_gpio = 2;   // e.g., GPIO 2
    motors[0].step_gpio = 1; // e.g., GPIO 1
    motor_init(&motors[0]);

    // --- 3. Configure Motor 1 (Scalability!) ---
    // Example: Assign different GPIOs for the second motor
    // Make sure these are valid GPIOs on your board
    motors[1].id = 1;
    motors[1].en_gpio = 5;
    motors[1].dir_gpio = 38;
    motors[1].step_gpio = 39;
    motor_init(&motors[1]);

    // Add more motor inits here...

    // --- 4. Create Tasks ---
    xTaskCreate(serial_task, "serial_task", UART_TASK_STACK_SIZE, NULL, 5, NULL);

    for (int i = 0; i < NUM_MOTORS; i++) {
        char task_name[20];
        sprintf(task_name, "motor_task_%d", i);

        // Pass the motor ID (i) as the task parameter
        xTaskCreate(motor_task, task_name, MOTOR_TASK_STACK_SIZE, (void *)i, 10, NULL);
    }

    ESP_LOGI(TAG, "Initialization complete. Tasks started.");
}