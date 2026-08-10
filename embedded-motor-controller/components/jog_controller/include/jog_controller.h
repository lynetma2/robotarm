#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the jog controller (UART, Motors, Pot, Buttons, LED).
 */
esp_err_t jog_controller_init(void);

/**
 * @brief Start the background task that handles jogging and ramping.
 */
void jog_controller_start(void);

#ifdef __cplusplus
}
#endif
