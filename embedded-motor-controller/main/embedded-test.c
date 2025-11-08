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

static QueueHandle_t commandQueue = NULL;
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

void motor_task(void *pvParameters)
{
    MotorCommand cmd;

    while (1) {
        // Wait for a command to arrive from the queue
        if (xQueueReceive(commandQueue, &cmd, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Received command for motor %d: %u steps", cmd.motor_id, cmd.accel_steps + cmd.uniform_steps + cmd.decel_steps);

            // Get the motor config
            if (cmd.motor_id >= NUM_MOTORS) {
                ESP_LOGE(TAG, "Invalid motor_id: %d", cmd.motor_id);
                continue; // Skip this command
            }
            MotorConfig *motor = &motors[cmd.motor_id];

            // --- 1. Delete old encoders (if they exist) ---
            if (motor->accel_encoder) {
                ESP_ERROR_CHECK(rmt_del_encoder(motor->accel_encoder));
                motor->accel_encoder = NULL;
            }
            if (motor->uniform_encoder) {
                ESP_ERROR_CHECK(rmt_del_encoder(motor->uniform_encoder));
                motor->uniform_encoder = NULL;
            }
            if (motor->decel_encoder) {
                ESP_ERROR_CHECK(rmt_del_encoder(motor->decel_encoder));
                motor->decel_encoder = NULL;
            }

            // --- 2. Create new encoders based on the command ---
            stepper_motor_curve_encoder_config_t accel_encoder_config = {
                .resolution = STEP_MOTOR_RESOLUTION_HZ,
                .sample_points = cmd.accel_steps,
                .start_freq_hz = cmd.start_freq_hz,
                .end_freq_hz = cmd.uniform_freq_hz,
            };
            ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&accel_encoder_config, &motor->accel_encoder));

            stepper_motor_uniform_encoder_config_t uniform_encoder_config = {
                .resolution = STEP_MOTOR_RESOLUTION_HZ,
            };
            ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_encoder_config, &motor->uniform_encoder));

            stepper_motor_curve_encoder_config_t decel_encoder_config = {
                .resolution = STEP_MOTOR_RESOLUTION_HZ,
                .sample_points = cmd.decel_steps,
                .start_freq_hz = cmd.uniform_freq_hz,
                .end_freq_hz = cmd.end_freq_hz,
            };
            ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&decel_encoder_config, &motor->decel_encoder));

            // --- 3. Set direction and enable motor ---
            gpio_set_level(motor->dir_gpio, cmd.direction);
            gpio_set_level(motor->en_gpio, STEP_MOTOR_ENABLE_LEVEL);
            vTaskDelay(pdMS_TO_TICKS(10)); // Small delay for driver to stabilize

            // --- 4. Transmit the new profiles ---
            rmt_transmit_config_t tx_config = { .loop_count = 0 };

            // Acceleration
            if (cmd.accel_steps > 0) {
                ESP_ERROR_CHECK(rmt_transmit(motor->rmt_channel, motor->accel_encoder, &cmd.accel_steps, sizeof(cmd.accel_steps), &tx_config));
            }
            // Uniform
            if (cmd.uniform_steps > 0) {
                tx_config.loop_count = cmd.uniform_steps;
                ESP_ERROR_CHECK(rmt_transmit(motor->rmt_channel, motor->uniform_encoder, &cmd.uniform_freq_hz, sizeof(cmd.uniform_freq_hz), &tx_config));
            }
            // Deceleration
            if (cmd.decel_steps > 0) {
                tx_config.loop_count = 0;
                ESP_ERROR_CHECK(rmt_transmit(motor->rmt_channel, motor->decel_encoder, &cmd.decel_steps, sizeof(cmd.decel_steps), &tx_config));
            }

            // --- 5. Wait for move to finish and disable motor ---
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor->rmt_channel, -1));
            gpio_set_level(motor->en_gpio, !STEP_MOTOR_ENABLE_LEVEL);

            ESP_LOGI(TAG, "Motor %d move complete.", cmd.motor_id);
        }
    }
}

void serial_task(void *pvParameters)
{
    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));

    // Allocate buffer for reading data
    uint8_t *data = (uint8_t *)malloc(UART_BUF_SIZE);
    // Allocate buffer for accumulating a line
    char *line_buffer = (char *)malloc(UART_BUF_SIZE);
    int line_pos = 0;

    ESP_LOGI(TAG, "Serial task started. Waiting for JSON commands...");

    while (1) {
        // Read data from UART
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE - 1, pdMS_TO_TICKS(20));

        if (len > 0) {
            // Process all received bytes
            for (int i = 0; i < len; i++) {
                char c = data[i];
                // If newline, we have a complete line
                if (c == '\n' || c == '\r') {
                    if (line_pos > 0) { // We have data in the buffer
                        line_buffer[line_pos] = '\0'; // Null-terminate
                        ESP_LOGI(TAG, "Received line: %s", line_buffer);

                        // --- Try to parse JSON ---
                        cJSON *json = cJSON_Parse(line_buffer);
                        if (json == NULL) {
                            ESP_LOGE(TAG, "Error parsing JSON");
                        } else {
                            // --- JSON is valid, extract command ---
                            MotorCommand cmd;
                            // Basic validation
                            const cJSON *motor_id = cJSON_GetObjectItem(json, "motor_id");
                            const cJSON *direction = cJSON_GetObjectItem(json, "direction");
                            const cJSON *accel_steps = cJSON_GetObjectItem(json, "accel_steps");
                            const cJSON *uniform_steps = cJSON_GetObjectItem(json, "uniform_steps");
                            const cJSON *decel_steps = cJSON_GetObjectItem(json, "decel_steps");
                            const cJSON *start_freq = cJSON_GetObjectItem(json, "start_freq");
                            const cJSON *uniform_freq = cJSON_GetObjectItem(json, "uniform_freq");
                            const cJSON *end_freq = cJSON_GetObjectItem(json, "end_freq");

                            if (cJSON_IsNumber(motor_id) && cJSON_IsNumber(direction) && cJSON_IsNumber(accel_steps)) {
                                // Populate the command struct
                                cmd.motor_id = motor_id->valueint;
                                cmd.direction = direction->valueint;
                                cmd.accel_steps = accel_steps->valueint;
                                cmd.uniform_steps = uniform_steps->valueint;
                                cmd.decel_steps = decel_steps->valueint;
                                cmd.start_freq_hz = start_freq->valueint;
                                cmd.uniform_freq_hz = uniform_freq->valueint;
                                cmd.end_freq_hz = end_freq->valueint;

                                // Send the command to the queue
                                if (xQueueSend(commandQueue, &cmd, pdMS_TO_TICKS(10)) != pdTRUE) {
                                    ESP_LOGE(TAG, "Failed to send command to queue (queue full)");
                                }
                            } else {
                                ESP_LOGE(TAG, "JSON missing required fields");
                            }
                            cJSON_Delete(json); // Free memory
                        }
                    }
                    line_pos = 0; // Reset for next line
                } else if (line_pos < (UART_BUF_SIZE - 1)) {
                    // Add char to buffer
                    line_buffer[line_pos++] = c;
                }
            }
        }
    }
    free(data);
    free(line_buffer);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Motor Control Application");

    // --- 1. Create the command queue ---
    commandQueue = xQueueCreate(10, sizeof(MotorCommand)); // Queue can hold 10 commands
    if (commandQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create command queue");
        return;
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
    //xTaskCreate(motor_task, "motor_task", MOTOR_TASK_STACK_SIZE, NULL, 10, NULL); // Motor task has higher priority

    ESP_LOGI(TAG, "Initialization complete. Tasks started.");
}