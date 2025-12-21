//
// Created by 99sun on 11-11-2025.
//

#include "motor_control.h"
#include <esp_log.h>
#include "cJSON.h"
#include <stdio.h>
#include "stepper_motor_encoder.h"

#define PCNT_HIGH_LIMIT 30000
#define PCNT_LOW_LIMIT  -30000

QueueHandle_t motorQueues[NUM_MOTORS];
MotorConfig motors[NUM_MOTORS];

static const char *TAG = "MOTOR_CONTROL";

/*
 * @brief Custom logging function to serialize log messages to JSON and print to UART.
 *
 * @param level The log level string (e.g., "INFO", "ERROR").
 * @param tag The tag string for the log message.
 * @param format The format string.
 * @param args The variable arguments list.
 */
void log_to_json(const char *level, const char *tag, const char *format, ...) {
    // 1. Format the message string from the variable arguments
    char message[256];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // 1. Format the message string from the variable arguments
    // 2. Create the JSON object
    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddNumberToObject(root, "timestamp", xTaskGetTickCount() * portTICK_PERIOD_MS);
    cJSON_AddStringToObject(root, "source", tag);

    cJSON *data = cJSON_AddObjectToObject(root, "data");
    cJSON_AddStringToObject(data, "type", "log");
    cJSON_AddStringToObject(data, "level", level);
    cJSON_AddStringToObject(data, "message", message);

    // 3. Serialize and print the JSON string
    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string) {
        printf("%s\n", json_string);
        cJSON_free(json_string);
    }

    cJSON_Delete(root);
}

void telemetry_task(void *pvParameters)
{
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 1000ms interval (1Hz)
    TickType_t xLastWakeTime = xTaskGetTickCount();   // Initialize current time

    log_to_json("INFO", TAG, "Telemetry task started");

    while (1)
    {
        // 1. Create the telemetry packet structure
        telemetry_packet_t packet = {
            .timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS, // Get current time in ms
            .source = TAG,
            .type = MSG_TYPE_TELEMETRY,
            .data.telemetry.num_motors = NUM_MOTORS
        };

        int pcnt_val;
        for (int i = 0; i < NUM_MOTORS; i++)
        {
            // 2. Update motor position from hardware counter
            pcnt_unit_get_count(motors[i].pcnt_unit, &pcnt_val);
            motors[i].absolute_position += pcnt_val;
            pcnt_unit_clear_count(motors[i].pcnt_unit);

            // 3. Populate the packet with the latest data
            packet.data.telemetry.motor_positions[i] = motors[i].absolute_position;
        }

        // 4. Create the JSON object
        cJSON *root = cJSON_CreateObject();
        if (root) {
            cJSON_AddNumberToObject(root, "timestamp", packet.timestamp);
            cJSON_AddStringToObject(root, "source", packet.source);

            cJSON *data = cJSON_AddObjectToObject(root, "data");
            cJSON *positions = cJSON_CreateArray();
            cJSON_AddStringToObject(data, "type", "telemetry");
            cJSON_AddItemToObject(data, "motor_positions", positions);

            for (int i = 0; i < packet.data.telemetry.num_motors; i++) {
                cJSON_AddItemToArray(positions, cJSON_CreateNumber(packet.data.telemetry.motor_positions[i]));
            }

            // 5. Serialize the JSON object and send it via UART
            char *json_string = cJSON_PrintUnformatted(root);
            if (json_string) {
                printf("%s\n", json_string); // Print to the console/USB UART
                cJSON_free(json_string);
            }
            cJSON_Delete(root);
        }

        // Wait for the next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void motor_task(void *pvParameters)
{
    // The parameter is the motor ID
    int motor_id = (int)pvParameters;

    // Get this task's specific motor config
    MotorConfig *motor = &motors[motor_id];
    // Get this task's specific queue
    QueueHandle_t queue = motorQueues[motor_id];

    log_to_json("INFO", TAG, "Motor Task %d started.", motor_id);

    MotorCommand cmd;

    while (1) {
        // Wait for a command to arrive from this motor's specific queue
        if (xQueueReceive(queue, &cmd, portMAX_DELAY)) {
            log_to_json("INFO", TAG, "[Motor %d] Received command: %u steps",
                     motor_id, cmd.accel_steps + cmd.uniform_steps + cmd.decel_steps);
            log_to_json("INFO", TAG, "[Motor %d] steps Accel: %u, Uniform: %u, Decel: %u]", motor_id, cmd.accel_steps, cmd.uniform_steps, cmd.decel_steps);
            log_to_json("INFO", TAG, "[Motor %d] start freq: %u, uniform freq: %u, end freq: %u", motor_id, cmd.start_freq_hz, cmd.uniform_freq_hz, cmd.end_freq_hz);

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
                .sample_points = cmd.start_freq_hz,
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
                .sample_points = cmd.end_freq_hz,
                .start_freq_hz = cmd.uniform_freq_hz,
                .end_freq_hz = cmd.end_freq_hz,
            };
            ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&decel_encoder_config, &motor->decel_encoder));

            // --- 3. Set direction and enable motor ---
            gpio_set_level(motor->dir_gpio, cmd.direction);
            gpio_set_level(motor->en_gpio, STEP_MOTOR_ENABLE_LEVEL);
            vTaskDelay(pdMS_TO_TICKS(10));

            // --- 4. Transmit the new profiles ---
            rmt_transmit_config_t tx_config = { .loop_count = 0 };
            motor_moving_to_json(motor_id, true);

            if (cmd.accel_steps > 0) {
                ESP_ERROR_CHECK(rmt_transmit(motor->rmt_channel, motor->accel_encoder, &cmd.accel_steps, sizeof(cmd.accel_steps), &tx_config));
            }
            if (cmd.uniform_steps > 0) {
                tx_config.loop_count = cmd.uniform_steps;
                ESP_ERROR_CHECK(rmt_transmit(motor->rmt_channel, motor->uniform_encoder, &cmd.uniform_freq_hz, sizeof(cmd.uniform_freq_hz), &tx_config));
            }
            if (cmd.decel_steps > 0) {
                tx_config.loop_count = 0;
                ESP_ERROR_CHECK(rmt_transmit(motor->rmt_channel, motor->decel_encoder, &cmd.decel_steps, sizeof(cmd.decel_steps), &tx_config));
            }

            // This task will block *here*, but other motor tasks can run.
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor->rmt_channel, -1));
            gpio_set_level(motor->en_gpio, !STEP_MOTOR_ENABLE_LEVEL);

            motor_moving_to_json(motor_id, false);
            log_to_json("INFO", TAG, "[Motor %d] Move complete.", motor_id);
        }
    }
}

void motor_init(MotorConfig *motor)
{
    log_to_json("INFO", TAG, "Initializing Motor %d", motor->id);

    // Configure EN + DIR GPIOs
    gpio_config_t en_dir_gpio_config = {
        .mode = GPIO_MODE_INPUT_OUTPUT,
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

    //Set up PCNT
    log_to_json("INFO", TAG, "[Motor %d] Install pcnt unit", motor->id);
    pcnt_unit_config_t unit_config = {
        .high_limit = PCNT_HIGH_LIMIT,
        .low_limit = PCNT_LOW_LIMIT,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &motor->pcnt_unit));

    log_to_json("INFO", TAG, "[Motor %d] Setting pcnt glitch filter", motor->id);
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(motor->pcnt_unit, &filter_config));

    log_to_json("INFO", TAG, "[Motor %d] Setting pcnt channel", motor->id);
    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = motor->step_gpio,  // The Pulse Source
        .level_gpio_num = motor->dir_gpio,  // The Direction Source (Control)
    };
    pcnt_new_channel(motor->pcnt_unit, &chan_config, &motor->pcnt_channel);

    // Step A: Define the base behavior (Count Up on Rising Edge)
    pcnt_channel_set_edge_action(motor->pcnt_channel,
                                 PCNT_CHANNEL_EDGE_ACTION_INCREASE, // On Rising
                                 PCNT_CHANNEL_EDGE_ACTION_HOLD);    // On Falling

    // Step B: Define how the DIR pin modifies that behavior
    // If DIR is HIGH: KEEP the behavior (Count Up)
    // If DIR is LOW:  INVERSE the behavior (Count Down)
    pcnt_channel_set_level_action(motor->pcnt_channel,
                                  PCNT_CHANNEL_LEVEL_ACTION_INVERSE, // Action when DIR is LOW
                                  PCNT_CHANNEL_LEVEL_ACTION_KEEP);   // Action when DIR is HIGH

    // Start the counter
    log_to_json("INFO", TAG, "enable pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(motor->pcnt_unit));
    log_to_json("INFO", TAG, "clear pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(motor->pcnt_unit));
    log_to_json("INFO", TAG, "start pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_start(motor->pcnt_unit));

    log_to_json("INFO", TAG, "[Motor %d] Initialized", motor->id);
}

void motor_moving_to_json(int motor_id, bool is_moving)
{
    cJSON *root = cJSON_CreateObject();
    if (root) {
        cJSON_AddNumberToObject(root, "timestamp", xTaskGetTickCount() * portTICK_PERIOD_MS);
        cJSON_AddStringToObject(root, "source", TAG);

        cJSON *data = cJSON_AddObjectToObject(root, "data");
        cJSON_AddStringToObject(data, "type", "motormoving");
        cJSON_AddNumberToObject(data, "motorId", motor_id);
        cJSON_AddBoolToObject(data, "isMoving", is_moving);


        // 5. Serialize the JSON object and send it via UART
        char *json_string = cJSON_PrintUnformatted(root);
        if (json_string) {
            printf("%s\n", json_string); // Print to the console/USB UART
            cJSON_free(json_string);
        }
        cJSON_Delete(root);
    }
}
