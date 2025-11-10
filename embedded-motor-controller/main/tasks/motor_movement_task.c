//
// Created by 99sun on 10-11-2025.
//

void motor_task(void *pvParameters)
{
    // The parameter is the motor ID
    int motor_id = (int)pvParameters;

    // Get this task's specific motor config
    MotorConfig *motor = &motors[motor_id];
    // Get this task's specific queue
    QueueHandle_t queue = motorQueues[motor_id];

    ESP_LOGI(TAG, "Motor Task %d started.", motor_id);

    MotorCommand cmd;

    while (1) {
        // Wait for a command to arrive from this motor's specific queue
        if (xQueueReceive(queue, &cmd, portMAX_DELAY)) {
            ESP_LOGI(TAG, "[Motor %d] Received command: %u steps",
                     motor_id, cmd.accel_steps + cmd.uniform_steps + cmd.decel_steps);

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
            vTaskDelay(pdMS_TO_TICKS(10));

            // --- 4. Transmit the new profiles ---
            rmt_transmit_config_t tx_config = { .loop_count = 0 };

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

            ESP_LOGI(TAG, "[Motor %d] Move complete.", motor_id);
        }
    }
}