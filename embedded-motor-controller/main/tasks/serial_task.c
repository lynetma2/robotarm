//
// Created by 99sun on 10-11-2025.
//

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
                                if (xQueueSend(motorQueues[cmd.motor_id], &cmd, pdMS_TO_TICKS(10)) != pdTRUE) {
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