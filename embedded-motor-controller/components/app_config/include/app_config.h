#pragma once

#include "driver/uart.h"
#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// Motion backend selection (per motor)
// ============================================================
typedef enum {
    HW_MOTION_UART = 0,   // velocity over UART (VACTUAL)
    HW_MOTION_STEPDIR,    // pulse engine on STEP/DIR pins (RMT)
} hw_motion_mode_t;

// ============================================================
// Buttons (active-low, internal pull-up; owned by app_buttons)
// ============================================================
#define HW_BTN1_GPIO          10   // toggle active motor
#define HW_BTN2_GPIO          11   // hold = jog CW
#define HW_BTN3_GPIO          12   // hold = jog CCW

// ============================================================
// Potentiometer
// ============================================================
#define HW_POT_ADC_UNIT       ADC_UNIT_1
#define HW_POT_ADC_CHANNEL    ADC_CHANNEL_8   // GPIO 9

// ============================================================
// Onboard RGB LED (WS2812, ESP32-S3-DevKitC-1)
// ============================================================
#define HW_RGB_LED_GPIO     48

// ============================================================
// Motor 1 (dedicated UART bus)
// ============================================================
#define HW_MOT1_UART_PORT     UART_NUM_2
#define HW_MOT1_UART_TX       8
#define HW_MOT1_UART_RX       7
#define HW_MOT1_EN_GPIO       4
#define HW_MOT1_STEP_GPIO     1
#define HW_MOT1_DIR_GPIO      2
#define HW_MOT1_MOTION_MODE   HW_MOTION_UART

// ============================================================
// Motor 2 (dedicated UART bus) — adjust to your wiring!
// ============================================================
#define HW_MOT2_UART_PORT     UART_NUM_1
#define HW_MOT2_UART_TX       17
#define HW_MOT2_UART_RX       18
#define HW_MOT2_EN_GPIO       5
#define HW_MOT2_STEP_GPIO     6
#define HW_MOT2_DIR_GPIO      16
#define HW_MOT2_MOTION_MODE   HW_MOTION_UART

// ============================================================
// Jog tuning
// ============================================================
#define HW_UART_BAUD          115200
#define JOG_MIN_SPEED         100       // usteps/s at pot minimum
#define JOG_MAX_SPEED         20000     // usteps/s at pot maximum
#define JOG_ACCEL             100000    // usteps/s^2
#define JOG_TASK_RATE_HZ      100

#ifdef __cplusplus
}
#endif