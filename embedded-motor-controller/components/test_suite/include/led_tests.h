//
// Created by 99sun on 10-08-2026.
//

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Addressable RGB LED (WS2812) on the ESP32-S3-DevKitC-1 */
#define LED_TEST_GPIO_NUM 48

/* Init the onboard RGB LED (idempotent). */
esp_err_t led_test_init(void);

/* Helpers, usable from other tests too (e.g. status indication) */
void led_test_set_color(uint8_t red, uint8_t green, uint8_t blue);
void led_test_off(void);

/* Blocking tests, same style as the motor tests */
void led_test_blink(void);      /* blink red forever */
void led_test_rgb_cycle(void);  /* R -> G -> B -> W forever */

#ifdef __cplusplus
}
#endif