/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "stepper_motor_encoder.h"
#include "freertos/queue.h"
#include "cJSON.h"
#include "string.h"
#include "driver/uart.h"

///////////////////////////////Change the following configurations according to your board//////////////////////////////
#define STEP_MOTOR_GPIO_EN       0
#define STEP_MOTOR_GPIO_DIR      2
#define STEP_MOTOR_GPIO_STEP     4
#define STEP_MOTOR_ENABLE_LEVEL  0 // DRV8825 is enabled on low level
#define STEP_MOTOR_SPIN_DIR_CLOCKWISE 0
#define STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE !STEP_MOTOR_SPIN_DIR_CLOCKWISE

#define STEP_MOTOR_RESOLUTION_HZ 1000000 // 1MHz resolution

#define NUM_MOTORS 2

static const char *TAG = "MOTOR_CONTROL";

typedef struct
{
    int id;
    gpio_num_t en_gpio;
    gpio_num_t dir_gpio;
    gpio_num_t step_gpio;
    rmt_channel_handle_t rmt_channel;

    rmt_encoder_handle_t accel_encoder;
    rmt_encoder_handle_t uniform_encoder;
    rmt_encoder_handle_t decel_encoder;
} MotorConfig;

typedef struct
{
    int motor_id;
    int direction;
    uint32_t accel_steps;
    uint32_t uniform_steps;
    uint32_t decel_steps;
    uint32_t start_freq_hz;
    uint32_t uniform_freq_hz;
    uint32_t end_freq_hz;
} MotorCommand;

// --- Serial/UART Configuration ---
#define UART_PORT_NUM      UART_NUM_0
#define UART_BAUD_RATE     115200
#define UART_BUF_SIZE      (1024)
#define UART_TASK_STACK_SIZE (2048)
#define MOTOR_TASK_STACK_SIZE (2048)

static QueueHandle_t motorQueues[NUM_MOTORS];
static MotorConfig motors[NUM_MOTORS];

void motor_init(MotorConfig *motor)
{
    ESP_LOGI(TAG, "Initializing Motor %d", motor->id);

    // Configure EN + DIR GPIOs
    gpio_config_t en_dir_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << motor->dir_gpio) | (1ULL << motor->en_gpio),
    };
    ESP_ERROR_CHECK(gpio_config(&en_dir_gpio_config));

    // Create RMT TX channel
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = motor->step_gpio,
        .mem_block_symbols = 64,
        .resolution_hz = STEP_MOTOR_RESOLUTION_HZ,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &motor->rmt_channel));

    // Set initial state (disabled)
    gpio_set_level(motor->dir_gpio, 0);
    gpio_set_level(motor->en_gpio, !STEP_MOTOR_ENABLE_LEVEL);

    // Encoders are set to NULL, they will be created by the motor_task
    motor->accel_encoder = NULL;
    motor->uniform_encoder = NULL;
    motor->decel_encoder = NULL;

    // Enable RMT channel
    ESP_ERROR_CHECK(rmt_enable(motor->rmt_channel));
}

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
    motors[0].en_gpio = STEP_MOTOR_GPIO_EN;     // e.g., GPIO 0
    motors[0].dir_gpio = STEP_MOTOR_GPIO_DIR;   // e.g., GPIO 2
    motors[0].step_gpio = STEP_MOTOR_GPIO_STEP; // e.g., GPIO 4
    motor_init(&motors[0]);

    // --- 3. Configure Motor 1 (Scalability!) ---
    // Example: Assign different GPIOs for the second motor
    // Make sure these are valid GPIOs on your board
    motors[1].id = 1;
    motors[1].en_gpio = 12;
    motors[1].dir_gpio = 13;
    motors[1].step_gpio = 14;
    motor_init(&motors[1]);

    // Add more motor inits here...

    // --- 4. Create Tasks ---
    xTaskCreate(serial_task, "serial_task", UART_TASK_STACK_SIZE, NULL, 5, NULL);

    // for (int i = 0; i < NUM_MOTORS; i++) {
    //     char task_name[20];
    //     sprintf(task_name, "motor_task_%d", i);
    //
    //     // Pass the motor ID (i) as the task parameter
    //     xTaskCreate(motor_task, task_name, MOTOR_TASK_STACK_SIZE, (void *)i, 10, NULL);
    // }

    ESP_LOGI(TAG, "Initialization complete. Tasks started.");
}