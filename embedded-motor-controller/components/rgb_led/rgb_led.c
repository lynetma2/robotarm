#include "rgb_led.h"
#include "app_config.h"

#include "led_strip.h"
#include "esp_log.h"

static const char *TAG = "rgb_led";
static led_strip_handle_t s_strip = NULL;

esp_err_t rgb_led_init(void)
{
    if (s_strip != NULL) return ESP_OK;

    led_strip_config_t strip_config = {
        .strip_gpio_num = HW_RGB_LED_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init failed: %s", esp_err_to_name(err));
        s_strip = NULL;
        return err;
    }
    led_strip_clear(s_strip);
    return ESP_OK;
}

void rgb_led_set(uint8_t red, uint8_t green, uint8_t blue)
{
    if (rgb_led_init() != ESP_OK) return;
    led_strip_set_pixel(s_strip, 0, red, green, blue);
    led_strip_refresh(s_strip);
}

void rgb_led_off(void)
{
    rgb_led_set(0, 0, 0);
}