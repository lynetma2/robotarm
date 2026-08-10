//
// Created by 99sun on 10-08-2026.
//

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Blocking tests, same style as the motor tests */
void led_test_blink(void);      /* blink red forever */
void led_test_rgb_cycle(void);  /* R -> G -> B -> W forever */

#ifdef __cplusplus
}
#endif