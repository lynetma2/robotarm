//
// Created by 99sun on 11-11-2025.
//

#ifndef EMBEDDED_TEST_SERIAL_COMMS_H
#define EMBEDDED_TEST_SERIAL_COMMS_H

#include "freertos/FreeRTOS.h"

// --- Serial/UART Configuration ---
#define UART_PORT_NUM      UART_NUM_0
#define UART_BAUD_RATE     115200
#define UART_BUF_SIZE      (1024)
#define UART_TASK_STACK_SIZE (2048)

/**
 * @brief FreeRTOS task for processing JSON commands from UART_NUM_0
 *
 * Reads lines from UART, parses them as JSON, and sends
 * valid MotorCommands to the appropriate motor's queue.
 *
 * @param pvParameters Unused.
 */
void serial_task(void *pvParameters);

#endif //EMBEDDED_TEST_SERIAL_COMMS_H