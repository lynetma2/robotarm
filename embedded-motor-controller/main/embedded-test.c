/*
 * ESP-IDF program to communicate with a TMC2209 via UART
 * using the analogdevicesinc/TMC-API.
 *
 * See README.md for project setup and wiring instructions.
 */

#include <stdio.h>
#include <TMC2209_HW_Abstraction.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
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

// --- TMC-API Includes ---
// Wrap in extern "C" to prevent C++/C linkage errors
#include <TMC2209.h>

// --- Configuration ---
#define UART_PORT_NUM       UART_NUM_2     // UART port to use
#define UART_TX_PIN         8    // ESP32-S3 TX pin
#define UART_RX_PIN         7    // ESP32-S3 RX pin
#define UART_BAUD_RATE      115200         // Baud rate for TMC2209
#define MOTOR_ID            0              // Motor ID and UART Address (0-3)

#define UART_BUFFER_SIZE    (1024)
#define READ_TIMEOUT_MS     100             // Timeout for UART reads
#define TASK_STACK_SIZE     (4096)

//static const char *TAG = "TMC2209";

void reverse_byte_array(uint8_t *array, size_t len) {
    if (!array || len == 0) {
        return;
    }
    size_t start = 0;
    size_t end = len - 1;
    uint8_t temp;
    while (start < end) {
        temp = array[start];
        array[start] = array[end];
        array[end] = temp;
        start++;
        end--;
    }
}

// ====================================================================
// Hardware Abstraction Layer (HAL)
//
// These are the functions the TMC-API needs us to implement.
// We "teach" the library how to use the ESP-IDF UART driver.
//
// These functions are called by the TMC2209.c file.
// ====================================================================


/**
 * @brief Returns the UART node address for the given motor.
 * @param icID The motor ID (0 in our case).
 * @return The 8-bit UART node address (0-3 for TMC2209).
 */
uint8_t tmc2209_getNodeAddress(uint16_t icID)
{
    // For a single motor setup, we just return the hardcoded address.
    // This function becomes important if you have multiple motors
    // and need to map icID (0, 1, 2...) to UART addresses (0, 1, 2, 3).
    return (uint8_t)MOTOR_ID;
}

/**
 * @brief The core UART read/write function for the TMC2209.
 * The API uses a single buffer for both TX and RX.
 *
 * @param icID The motor ID (0 in our case).
 * @param data Pointer to the buffer. Data to be sent is already in here.
 * Received data will be written back into this same buffer.
 * @param tx_len Number of bytes to transmit.
 * @param rx_len Number of bytes to receive (expected response length).
 * @return true on success, false on failure.
 */
bool tmc2209_readWriteUART(uint16_t icID, uint8_t *data, size_t tx_len, size_t rx_len)
{
    // This HAL function is specific to the TMC2209 UART driver
    // It uses a single buffer for TX and RX.

    // 1. Flush any old data from the RX buffer
    uart_flush_input(UART_PORT_NUM);
    ESP_LOGW(TAG, "DEBUG: Trying to write %zu bytes:", tx_len);
    ESP_LOG_BUFFER_HEX(TAG, data, tx_len);
    //reverse_byte_array(data, tx_len);
    //ESP_LOGW(TAG, "After reverse: Writing %zu bytes:", tx_len);
    //ESP_LOG_BUFFER_HEX(TAG, data, tx_len);


    // 2. Write the datagram
    int bytes_written = uart_write_bytes(UART_PORT_NUM, (const char *)data, tx_len);
    if (bytes_written != tx_len) {
        ESP_LOGE(TAG, "HAL Error: Wrote %d bytes, expected %d", bytes_written, tx_len);
        return false;
    }

    // 3. Wait for the write to complete
    esp_err_t err = uart_wait_tx_done(UART_PORT_NUM, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HAL Error: uart_wait_tx_done failed (%s)", esp_err_to_name(err));
        return false;
    }

    // 4. If a response is expected, read it back into the *same* buffer
    uint8_t buffer[8];
    if (rx_len > 0)
    {
        // Read the expected number of bytes.
        int bytes_read = uart_read_bytes(UART_PORT_NUM, &data[0], (UART_BUFFER_SIZE - 1), pdMS_TO_TICKS(READ_TIMEOUT_MS));

        if (bytes_read == 0) {
            ESP_LOGW(TAG, "HAL Warning: UART Read Timeout! No data received.");
            return false;
        } else if (bytes_read < rx_len) {
            ESP_LOGW(TAG, "HAL Warning: Read %d bytes, expected %d", bytes_read, rx_len);
            return false;
        }
    }

    ESP_LOGW(TAG, "After read: reading %zu bytes:", rx_len);
    ESP_LOG_BUFFER_HEX(TAG, data, rx_len);

    return true;
}


// ====================================================================
// Main Application
// ====================================================================

/**
 * @brief Initializes the UART peripheral for TMC2209 communication.
 */
void tmc_uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_LOGI(TAG, "Installing UART driver");
    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUFFER_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));

    ESP_LOGI(TAG, "Setting UART pins: TX=%d, RX=%d", UART_TX_PIN, UART_RX_PIN);
    // Set UART pins
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

/**
 * @brief Main task to interact with the TMC2209.
 */
void tmc_task(void *pvParameters) {
    ESP_LOGI(TAG, "TMC Task Started. Waiting 1s for motor power...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    tmc2209_writeRegister(MOTOR_ID, TMC2209_GCONF, 0x00000040);
    tmc2209_writeRegister(MOTOR_ID, TMC2209_IHOLD_IRUN, 0x00071703);
    tmc2209_writeRegister(MOTOR_ID, TMC2209_TPOWERDOWN, 0x00000014);
    tmc2209_writeRegister(MOTOR_ID, TMC2209_CHOPCONF, 0x10000053);
    tmc2209_writeRegister(MOTOR_ID, TMC2209_PWMCONF, 0xC10D0024);
    tmc2209_writeRegister(MOTOR_ID, TMC2209_VACTUAL, 0);

    while (1) {
        vTaskDelay(portMAX_DELAY); // Idle the task
    }
}

/**
 * @brief Main entry point for ESP-IDF.
 */
void app_main(void) {
    // Initialize the UART for the TMC driver
    //tmc_uart_init();

    // Create the task that will communicate with the TMC
    //xTaskCreate(tmc_task, "tmc_task", TASK_STACK_SIZE, NULL, 5, NULL);
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