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
#include "driver/pulse_cnt.h"

#define STEP_MOTOR_ENABLE_LEVEL  0 // DRV8825 is enabled on low level
#define STEP_MOTOR_SPIN_DIR_CLOCKWISE 0
#define STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE !STEP_MOTOR_SPIN_DIR_CLOCKWISE

#define STEP_MOTOR_RESOLUTION_HZ 1000000 // 1MHz resolution

#define NUM_MOTORS 2
#define MOTOR_TASK_STACK_SIZE (4096)

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

    pcnt_unit_handle_t pcnt_unit;
    pcnt_channel_handle_t pcnt_channel;
    int64_t absolute_position;
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

// --- Enums for Type Safety ---
typedef enum {
    MSG_TYPE_TELEMETRY,
    MSG_TYPE_LOG,
    MSG_TYPE_MOTORMOVING
} msg_type_t;

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

// --- Payload Structures ---

// Specific data for Logs
typedef struct {
    log_level_t level;
    char *message;      // Dynamic string
    int error_code;     // Optional, 0 if none
} log_payload_t;

typedef struct {
    int motorId;
    bool isMoving;
} motor_moving_payload_t;

// Specific data for Telemetry (Customize this to your needs)
typedef struct {
    int64_t motor_positions[NUM_MOTORS];
    uint8_t num_motors;
} telemetry_payload_t;

// --- The "Smart" Wrapper Struct ---
typedef struct {
    int64_t timestamp;      // Auto-filled
    const char *source;     // Static string (e.g., "TAG")
    msg_type_t type;        // The "Tag" that tells us what the union holds

    union {
        log_payload_t log;
        telemetry_payload_t telemetry;
        motor_moving_payload_t motor_moving;
    } data;

} telemetry_packet_t;

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

/**
 * @brief FreeRTOS task for printing telemetry data.
 *
 * Looks at the pcnt counter for each motor and reports it.
 *
 * @param pvParameters unused in this.
 */
void telemetry_task(void *pvParameters);

/**
 * @brief Custom logging function to serialize log messages to JSON and print to UART.
 *
 * @param level The log level string (e.g., "INFO", "ERROR").
 * @param tag The tag string for the log message.
 * @param format The format string.
 * @param ... The variable arguments list.
 */
void log_to_json(const char *level, const char *tag, const char *format, ...);

void motor_moving_to_json(int motor_id, bool is_moving);
#endif //EMBEDDED_TEST_MOTOR_CONTROL_H