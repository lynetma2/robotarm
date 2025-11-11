//
// Created by 99sun on 11-11-2025.
//

#ifndef EMBEDDED_TEST_MOTOR_CONTROL_H
#define EMBEDDED_TEST_MOTOR_CONTROL_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"

#define STEP_MOTOR_ENABLE_LEVEL  0 // DRV8825 is enabled on low level
#define STEP_MOTOR_SPIN_DIR_CLOCKWISE 0
#define STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE !STEP_MOTOR_SPIN_DIR_CLOCKWISE

#define STEP_MOTOR_RESOLUTION_HZ 1000000 // 1MHz resolution

#define NUM_MOTORS 2
#define MOTOR_TASK_STACK_SIZE (2048)

// --- Enums and Structs for Motor Configuration ---
/**
 * @brief Main configuration struct for a single motor.
 */
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

/**
 * @brief Command struct for a motor move (received via JSON).
 * This remains high-level and driver-agnostic.
 */
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

// --- Global Shared Variables ---
// These are defined in motor_control.c and declared extern here
// so other files (like main.c and serial_comms.c) can access them.
extern QueueHandle_t motorQueues[NUM_MOTORS];
extern MotorConfig motors[NUM_MOTORS];


// --- Function Prototypes ---

/**
 * @brief Initializes a single motor's GPIOs, RMT channel, and driver.
 *
 * @param motor Pointer to the MotorConfig struct to initialize.
 */
void motor_init(MotorConfig *motor);

/**
 * @brief FreeRTOS task for controlling a single motor.
 *
 * Waits for commands on its dedicated queue and executes them.
 *
 * @param pvParameters The motor ID (as an integer cast to void*).
 */
void motor_task(void *pvParameters);

#endif //EMBEDDED_TEST_MOTOR_CONTROL_H