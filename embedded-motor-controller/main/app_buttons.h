//
// Created by 99sun on 08-08-2026.
//

#pragma once

#include "esp_err.h"
#include "iot_button.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_BTN_1 = 0,
    APP_BTN_2,
    APP_BTN_3,
    APP_BTN_COUNT,
} app_button_t;

/**
 * @brief Create the three buttons (GPIO 10/11/12, internal pull-up)
 *        and register default logging callbacks for all common events,
 *        including press-down / press-up (hold detection).
 */
esp_err_t app_buttons_init(void);

/**
 * @brief Register an extra behavior callback on a button.
 *        Use this later for jogging, mode switching, etc.
 */
esp_err_t app_buttons_register_cb(app_button_t btn, button_event_t event,
                                  button_cb_t cb, void *usr_data);

#ifdef __cplusplus
}
#endif
