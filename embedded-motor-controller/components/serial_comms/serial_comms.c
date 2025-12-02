//
// Created by 99sun on 11-11-2025.
//

#include "serial_comms.h"
#include "motor_control.h" // Needs this for motorQueues, MotorCommand, and NUM_MOTORS
#include "driver/uart.h"
#include "cJSON.h"
#include "string.h"
#include "esp_log.h"

static const char *TAG = "SERIAL_COMMS";

/**
 * @brief Parses a JSON string representing a motor command and dispatches it to the correct queue.
 *
 * The expected JSON structure is:
 * {
 *   "cmd": "step",
 *   "data": {
 *     "id": 123,
 *     "name": "Some Step Name",
 *     "segments": [
 *       {
 *         "motor_id": 0,
 *         "direction": 1,
 *         "accel_steps": 100,
 *         "uniform_steps": 500,
 *         "decel_steps": 100,
 *         "start_freq": 500,
 *         "uniform_freq": 2000,
 *         "end_freq": 500
 *       },
 *       ...
 *     ]
 *   }
 * }
 *
 * @param json_string The null-terminated JSON string to parse.
 */
static void parse_and_dispatch_command(const char *json_string) {
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        ESP_LOGE(TAG, "Error parsing JSON: %s", cJSON_GetErrorPtr());
        return;
    }

    const cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (!cJSON_IsString(cmd_item) || (cmd_item->valuestring == NULL)) {
        ESP_LOGE(TAG, "JSON missing or invalid 'cmd' field");
        cJSON_Delete(root);
        return;
    }

    if (strcmp(cmd_item->valuestring, "step") != 0) {
        ESP_LOGW(TAG, "Received unknown command: %s", cmd_item->valuestring);
        cJSON_Delete(root);
        return;
    }

    const cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!cJSON_IsObject(data)) {
        ESP_LOGE(TAG, "JSON missing 'data' object");
        cJSON_Delete(root);
        return;
    }

    const cJSON *segments = cJSON_GetObjectItem(data, "segments");
    if (!cJSON_IsArray(segments)) {
        ESP_LOGE(TAG, "JSON 'data' missing 'segments' array");
        cJSON_Delete(root);
        return;
    }

    cJSON *segment = NULL;
    cJSON_ArrayForEach(segment, segments) {
        MotorCommand cmd;
        const cJSON *motor_id = cJSON_GetObjectItem(segment, "motor_id");
        const cJSON *direction = cJSON_GetObjectItem(segment, "direction");
        const cJSON *accel_steps = cJSON_GetObjectItem(segment, "accel_steps");
        const cJSON *uniform_steps = cJSON_GetObjectItem(segment, "uniform_steps");
        const cJSON *decel_steps = cJSON_GetObjectItem(segment, "decel_steps");
        const cJSON *start_freq = cJSON_GetObjectItem(segment, "start_freq");
        const cJSON *uniform_freq = cJSON_GetObjectItem(segment, "uniform_freq");
        const cJSON *end_freq = cJSON_GetObjectItem(segment, "end_freq");

        if (motor_id && direction && accel_steps && uniform_steps && decel_steps && start_freq && uniform_freq && end_freq) {
            cmd.motor_id = motor_id->valueint;
            cmd.direction = direction->valueint;
            cmd.accel_steps = accel_steps->valueint;
            cmd.uniform_steps = uniform_steps->valueint;
            cmd.decel_steps = decel_steps->valueint;
            cmd.start_freq_hz = start_freq->valueint;
            cmd.uniform_freq_hz = uniform_freq->valueint;
            cmd.end_freq_hz = end_freq->valueint;

            if (cmd.motor_id < 0 || cmd.motor_id >= NUM_MOTORS) {
                ESP_LOGE(TAG, "Invalid motor_id: %d", cmd.motor_id);
                continue; // Skip to the next segment
            }

            if (xQueueSend(motorQueues[cmd.motor_id], &cmd, pdMS_TO_TICKS(10)) != pdTRUE) {
                ESP_LOGE(TAG, "Failed to send command to queue for motor %d (queue full)", cmd.motor_id);
            }

        } else {
            ESP_LOGW(TAG, "Skipping segment due to missing fields.");
        }
    }

    cJSON_Delete(root);
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

                        parse_and_dispatch_command(line_buffer);
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