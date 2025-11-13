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

// --- TMC-API Includes ---
// Wrap in extern "C" to prevent C++/C linkage errors
#include <TMC2209.h>

// --- Configuration ---
#define UART_PORT_NUM       UART_NUM_2     // UART port to use
#define UART_TX_PIN         GPIO_NUM_16    // ESP32-S3 TX pin
#define UART_RX_PIN         GPIO_NUM_17    // ESP32-S3 RX pin
#define UART_BAUD_RATE      115200         // Baud rate for TMC2209
#define MOTOR_ID            0              // Motor ID and UART Address (0-3)

#define UART_BUFFER_SIZE    (1024)
#define READ_TIMEOUT_MS     10             // Timeout for UART reads
#define TASK_STACK_SIZE     (4096)

static const char *TAG = "TMC2209";

// ====================================================================
// Hardware Abstraction Layer (HAL)
//
// These are the functions the TMC-API needs us to implement.
// We "teach" the library how to use the ESP-IDF UART driver.
//
// These functions are called by the TMC2209.c file.
// ====================================================================

extern "C" {

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
bool tmc2209_readWriteUART(uint16_t icID, uint8_t *data, uint8_t tx_len, uint8_t rx_len)
{
    // This HAL function is specific to the TMC2209 UART driver
    // It uses a single buffer for TX and RX.

    // 1. Flush any old data from the RX buffer
    uart_flush_input(UART_PORT_NUM);

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
    if (rx_len > 0)
    {
        // Read the expected number of bytes.
        int bytes_read = uart_read_bytes(UART_PORT_NUM, data, rx_len, pdMS_TO_TICKS(READ_TIMEOUT_MS));

        if (bytes_read == 0) {
            ESP_LOGW(TAG, "HAL Warning: UART Read Timeout! No data received.");
            return false;
        } else if (bytes_read < rx_len) {
            ESP_LOGW(TAG, "HAL Warning: Read %d bytes, expected %d", bytes_read, rx_len);
            return false;
        }
    }

    return true;
}

} // extern "C"

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

    ESP_LOGI(TAG, "Attempting to read GCONF...");

    // --- Test 1: Read a register ---
    int32_t gconf_value = 0;

    // Call the library function to read the GCONF register
    // This call (tmc2209_read) is defined in TMC_API.c
    // It will internally call tmc2209_readRegister (in TMC2209.c)
    // which then calls our HAL function tmc2209_readWriteUART.
    tmc2209_read(MOTOR_ID, TMC2209_GCONF, &gconf_value);

    ESP_LOGI(TAG, "GCONF register value: 0x%08lX", gconf_value);

    if (gconf_value == 0) {
        ESP_LOGW(TAG, "Warning: Read 0x0. Check wiring and motor power.");
        ESP_LOGW(TAG, "The TMC2209 MUST have motor power (VM) applied to respond.");
    } else {
        ESP_LOGI(TAG, "Successfully read GCONF. Communication is working!");
    }

    // --- Test 2: Write a register ---
    // Let's enable 'internal_rsense' (bit 0 of GCONF)
    // This is just an example write to test the write function.
    int32_t new_gconf = gconf_value | 0x01; // Set bit 0

    ESP_LOGI(TAG, "Writing new GCONF value: 0x%08lX", new_gconf);
    // This call will eventually use our HAL function
    tmc2209_write(MOTOR_ID, TMC2209_GCONF, new_gconf);

    // Give the write time to process
    vTaskDelay(pdMS_TO_TICKS(100));

    // --- Test 3: Read back the value to confirm ---
    int32_t readback_gconf = 0;
    tmc2209_read(MOTOR_ID, TMC2209_GCONF, &readback_gconf);
    ESP_LOGI(TAG, "Read back GCONF value: 0x%08lX", readback_gconf);

    if (readback_gconf == new_gconf) {
        ESP_LOGI(TAG, "Success! Readback matches written value.");
    } else {
        ESP_LOGE(TAG, "Error: Readback value does not match written value.");
    }

    ESP_LOGI(TAG, "Test complete. Task will now idle.");

    while (1) {
        vTaskDelay(portMAX_DELAY); // Idle the task
    }
}

/**
 * @brief Main entry point for ESP-IDF.
 */
void app_main(void) {
    // Initialize the UART for the TMC driver
    tmc_uart_init();

    // Create the task that will communicate with the TMC
    xTaskCreate(tmc_task, "tmc_task", TASK_STACK_SIZE, NULL, 5, NULL);
}