#include <stdint.h>
#include "esp_log.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "include/app_buttons.h"

static const char *TAG = "app_buttons";

#define BTN_1_GPIO 10
#define BTN_2_GPIO 11
#define BTN_3_GPIO 12

static button_handle_t s_buttons[APP_BTN_COUNT];

static const char *btn_name(app_button_t btn)
{
    switch (btn) {
        case APP_BTN_1: return "Button 1";
        case APP_BTN_2: return "Button 2";
        case APP_BTN_3: return "Button 3";
        default:        return "Button ?";
    }
}

// ---------- default logging callbacks ----------
static void log_press_down(void *arg, void *d) { ESP_LOGI(TAG, "%s: PRESS DOWN (held)", btn_name((app_button_t)(uintptr_t)d)); }
static void log_press_up(void *arg, void *d)   { ESP_LOGI(TAG, "%s: PRESS UP (released)", btn_name((app_button_t)(uintptr_t)d)); }
static void log_single(void *arg, void *d)     { ESP_LOGI(TAG, "%s: single click", btn_name((app_button_t)(uintptr_t)d)); }
static void log_double(void *arg, void *d)     { ESP_LOGI(TAG, "%s: double click", btn_name((app_button_t)(uintptr_t)d)); }
static void log_long(void *arg, void *d)       { ESP_LOGI(TAG, "%s: long press", btn_name((app_button_t)(uintptr_t)d)); }

esp_err_t app_buttons_init(void)
{
    static const int gpios[APP_BTN_COUNT] = { BTN_1_GPIO, BTN_2_GPIO, BTN_3_GPIO };

    // In v4.x, the generic config is usually empty for GPIO buttons
    const button_config_t btn_cfg = {0};

    for (int i = 0; i < APP_BTN_COUNT; i++) {
        // This struct is specific to GPIO hardware
        button_gpio_config_t gpio_cfg = {
            .gpio_num = gpios[i],
            .active_level = 0,   // pressed = LOW, internal pull-up enabled
        };

        button_handle_t btn = NULL;

        // v4.x API: create the GPIO device and return a generic button handle
        esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn);
        if (ret != ESP_OK || btn == NULL) {
            ESP_LOGE(TAG, "Failed to create button on GPIO %d", gpios[i]);
            return ESP_FAIL;
        }
        s_buttons[i] = btn;

        void *id = (void *)(uintptr_t)i;

        // Register our logging callbacks
        iot_button_register_cb(btn, BUTTON_PRESS_DOWN,       NULL, log_press_down, id);
        iot_button_register_cb(btn, BUTTON_PRESS_UP,         NULL, log_press_up,   id);
        iot_button_register_cb(btn, BUTTON_SINGLE_CLICK,     NULL, log_single,     id);
        iot_button_register_cb(btn, BUTTON_DOUBLE_CLICK,     NULL, log_double,     id);
        iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, NULL, log_long,       id);
    }

    ESP_LOGI(TAG, "Buttons initialized (GPIO %d, %d, %d)", BTN_1_GPIO, BTN_2_GPIO, BTN_3_GPIO);
    return ESP_OK;
}

esp_err_t app_buttons_register_cb(app_button_t btn, button_event_t event,
                                  button_cb_t cb, void *usr_data)
{
    if (btn >= APP_BTN_COUNT || s_buttons[btn] == NULL) {
        return ESP_ERR_INVALID_STATE;   // app_buttons_init() not called?
    }
    // v4.x API: requires an extra NULL parameter for event args
    return iot_button_register_cb(s_buttons[btn], event, NULL, cb, usr_data);
}