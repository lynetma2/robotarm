//
// Created by 99sun on 10-08-2026.
//

#pragma once
#include "driver/uart.h"
#include "soc/gpio_num.h"
#include "esp_adc/adc_oneshot.h"

// ============================================================
// HARDWARE PINOUT
// ============================================================
#define HW_UART_PORT      UART_NUM_2
#define HW_UART_TX_GPIO   GPIO_NUM_8
#define HW_UART_RX_GPIO   GPIO_NUM_7
#define HW_UART_BAUD      115200

// Motor 1 (Update GPIOs to your wiring)
#define HW_MOT1_EN_GPIO   GPIO_NUM_4
#define HW_MOT1_STEP_GPIO GPIO_NUM_1
#define HW_MOT1_DIR_GPIO  GPIO_NUM_2

// Motor 2 (Update GPIOs to your wiring)
#define HW_MOT2_EN_GPIO   GPIO_NUM_5
#define HW_MOT2_STEP_GPIO GPIO_NUM_6
#define HW_MOT2_DIR_GPIO  GPIO_NUM_10

// Buttons (Update GPIOs to your wiring)
#define HW_BTN1_GPIO      GPIO_NUM_11  // Toggle Motor
#define HW_BTN2_GPIO      GPIO_NUM_12  // Jog CW
#define HW_BTN3_GPIO      GPIO_NUM_13  // Jog CCW

// Potentiometer
#define HW_POT_ADC_UNIT   ADC_UNIT_1
#define HW_POT_ADC_CHAN   ADC_CHANNEL_8  // GPIO 9 on ESP32-S3

// ============================================================
// TUNING PARAMETERS
// ============================================================
#define JOG_MIN_SPEED     100     // microsteps/sec
#define JOG_MAX_SPEED     20000   // microsteps/sec
#define JOG_ACCEL         100000  // microsteps/sec^2 (ramp rate)
#define JOG_TASK_RATE_HZ  100     // 100 Hz = 10ms tick for smooth ramping
