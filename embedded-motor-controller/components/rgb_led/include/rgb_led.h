#pragma once
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Init the onboard WS2812 (idempotent). */
esp_err_t rgb_led_init(void);

/** @brief Set color, 0-255 per channel. Inits on first use. */
void rgb_led_set(uint8_t red, uint8_t green, uint8_t blue);

/** @brief Turn the LED off. */
void rgb_led_off(void);

#ifdef __cplusplus
}
#endif