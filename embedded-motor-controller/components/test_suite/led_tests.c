#include "led_tests.h"
#include "rgb_led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define LED_TEST_BRIGHTNESS 16
#define LED_TEST_PERIOD_MS  500

static const char *TAG = "led_tests";

void led_test_blink(void)
{
    ESP_LOGI(TAG, "led_test_blink(): blinking red");
    while (1) {
        rgb_led_set(LED_TEST_BRIGHTNESS, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
        rgb_led_off();
        vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
    }
}

void led_test_rgb_cycle(void)
{
    ESP_LOGI(TAG, "led_test_rgb_cycle(): R -> G -> B -> W");
    while (1) {
        rgb_led_set(LED_TEST_BRIGHTNESS, 0, 0);               vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
        rgb_led_set(0, LED_TEST_BRIGHTNESS, 0);               vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
        rgb_led_set(0, 0, LED_TEST_BRIGHTNESS);               vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
        rgb_led_set(LED_TEST_BRIGHTNESS, LED_TEST_BRIGHTNESS,
                    LED_TEST_BRIGHTNESS);                     vTaskDelay(pdMS_TO_TICKS(LED_TEST_PERIOD_MS));
    }
}