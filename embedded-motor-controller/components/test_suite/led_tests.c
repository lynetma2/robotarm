//
// Created by 99sun on 10-08-2026.
//

#include "include/led_tests.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

#define LED_TEST_COUNT      1
#define LED_TEST_BRIGHTNESS 16    /* 0..255; the onboard LED is very bright */
#define LED_TEST_PERIOD_MS  500

static const char *TAG = "led_tests";
static led_strip_handle_t s_led_strip = NULL;

esp_err_t led_test_init(void)
{
    if (s_led_strip != NULL) {
        return ESP_OK;  /* already initialised */
    }

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_TEST_GPIO_NUM,
        .max_leds = LED_TEST_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,  /* 10 MHz */
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip init failed: %s", esp_err_to_name(err));
        s_led_strip = NULL;
        return err;
    }

    led_strip_clear(s_led_strip);
    ESP_LOGI(TAG, "RGB LED ready on GPIO%d", LED_TEST_GPIO_NUM);
    return ESP_OK;
}

void led_test_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    if (led_test_init() != ESP_OK) {
        return;
    }
    led_strip_set_pixel(s_led_strip, 0, red, green, blue);
    led_strip_refresh(s_led_strip);
}

void led_test_off(void)
{
    led_test_set_color(0, 0, 0);
}

void led_test_blink(void)
{
    ESP_LOGI(TAG, "led_test_blink(): blinking red");
    while (1) {
        led_test_set_color(LED_TEST_BRIGHTNESS, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
        led_test_off();
        vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
    }
}

void led_test_rgb_cycle(void)
{
    ESP_LOGI(TAG, "led_test_rgb_cycle(): R -> G -> B -> W");
    while (1) {
        led_test_set_color(LED_TEST_BRIGHTNESS, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
        led_test_set_color(0, LED_TEST_BRIGHTNESS, 0);
        vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
        led_test_set_color(0, 0, LED_TEST_BRIGHTNESS);
        vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
        led_test_set_color(LED_TEST_BRIGHTNESS, LED_TEST_BRIGHTNESS, LED_TEST_BRIGHTNESS);
        vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
    }
}