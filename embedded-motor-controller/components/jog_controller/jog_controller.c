#include "jog_controller.h"
#include "app_config.h"

#include "tmc2209_dev.h"
#include "pot.h"
#include "app_buttons.h"
#include "rgb_led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "jog_ctrl";

// ============================================================
// State machine
// ============================================================
typedef enum {
    APP_STATE_IDLE = 0,
    APP_STATE_JOG_CW,
    APP_STATE_JOG_CCW,
} app_state_t;

// Pulled counts out of the enums to prevent -Wswitch errors
typedef enum {
    MOTOR_1 = 0,
    MOTOR_2,
} motor_id_t;
#define MOTOR_COUNT 2

typedef enum {
    PROFILE_MOTOR_1 = 0,
    PROFILE_MOTOR_2,
    PROFILE_BOTH,
} jog_profile_t;
#define PROFILE_COUNT 3

typedef struct {
    tmc2209_dev_t dev;
    hw_motion_mode_t mode;
    bool running;
} jog_motor_t;

static volatile bool          s_btn2_held     = false;
static volatile bool          s_btn3_held     = false;
static volatile jog_profile_t s_profile       = PROFILE_MOTOR_1;
static volatile int32_t       s_current_speed = 0;   // signed, usteps/s
static jog_motor_t s_motors[MOTOR_COUNT];
static pot_t *s_pot = NULL;

// Unified color map for all profiles
static const uint8_t s_profile_color[PROFILE_COUNT][3] = {
    [PROFILE_MOTOR_1] = { 255, 0, 0 },   // Red
    [PROFILE_MOTOR_2] = { 0, 0, 255 },   // Blue
    [PROFILE_BOTH]    = { 255, 0, 255 }, // Magenta
};

// ============================================================
// Button callbacks (run in iot_button task: keep them trivial!)
// ============================================================
static void on_btn1_single_click(void *arg, void *data)
{
    // Cycle: M1 -> M2 -> BOTH -> M1 ...
    s_profile = (s_profile + 1) % PROFILE_COUNT;

    // Safety: Reset the ramp so the newly activated motor(s)
    // smoothly accelerate from 0 to the current pot position.
    s_current_speed = 0;

    // Use the color map instead of hardcoded values
    rgb_led_set(s_profile_color[s_profile][0],
                s_profile_color[s_profile][1],
                s_profile_color[s_profile][2]);

    ESP_LOGI(TAG, "Profile -> %s",
        s_profile == PROFILE_MOTOR_1 ? "Motor 1" :
        s_profile == PROFILE_MOTOR_2 ? "Motor 2" : "BOTH");
}

static void on_btn2_down(void *arg, void *data) { s_btn2_held = true;  }
static void on_btn2_up(void *arg, void *data)   { s_btn2_held = false; }
static void on_btn3_down(void *arg, void *data) { s_btn3_held = true;  }
static void on_btn3_up(void *arg, void *data)   { s_btn3_held = false; }

// ============================================================
// Motion backend abstraction (the UART / STEP-DIR seam)
// ============================================================
static void motor_apply(jog_motor_t *m, int32_t speed)
{
    if (m->mode == HW_MOTION_STEPDIR) {
        if (speed == 0) {
            if (m->running) {
                tmc2209_stop_stepping(&m->dev);
                m->running = false;
            }
        } else {
            tmc2209_set_direction(&m->dev, speed > 0);
            tmc2209_set_step_rate(&m->dev, (uint32_t)(speed > 0 ? speed : -speed));
            if (!m->running) {
                tmc2209_start_stepping(&m->dev);
                m->running = true;
            }
        }
    } else {
        if (speed == 0) {
            if (m->running) {
                tmc2209_stop_internal_motion(&m->dev);
                m->running = false;
            }
        } else {
            tmc2209_set_internal_velocity(&m->dev, speed);
            if (!m->running) {
                tmc2209_start_internal_motion(&m->dev);
                m->running = true;
            }
        }
    }
}

static esp_err_t motor_init(jog_motor_t *m, uart_port_t port, int tx, int rx,
                            int en, int step, int dir,
                            hw_motion_mode_t mode, uint16_t ic_id)
{
    tmc2209_bus_config_t bus_cfg = {
        .uart_port = port, .tx_pin = tx, .rx_pin = rx,
        .baud_rate = HW_UART_BAUD, .discard_echo = true, .timeout_ms = 20,
    };
    ESP_ERROR_CHECK(tmc2209_init_bus(&bus_cfg));

    tmc2209_config_t cfg = {
        .uart_port = port,
        .ic_id = ic_id,
        .r_sense_mohm = 110,
        .node_address = 0,   // one driver per bus -> no addressing needed
        .enable_gpio = en,
        .stepdir = {
            .enabled = true, // UART mode: library holds STEP/DIR low (safety)
            .step_gpio = step,
            .dir_gpio = dir,
        },
    };
    ESP_ERROR_CHECK(tmc2209_init(&m->dev, &cfg));

    tmc2209_set_run_current(&m->dev, 800);
    tmc2209_set_hold_current(&m->dev, 400);
    tmc2209_set_microsteps(&m->dev, 16);
    // SpreadCycle tends to behave better at high step rates; tune to taste
    tmc2209_set_stealthchop(&m->dev, mode == HW_MOTION_UART);

    if (mode == HW_MOTION_STEPDIR) {
        ESP_ERROR_CHECK(tmc2209_stepdir_init(&m->dev));
    }

    tmc2209_set_enabled(&m->dev, true);
    m->mode = mode;
    m->running = false;
    return ESP_OK;
}

// ============================================================
// Helpers
// ============================================================
static int32_t pot_to_speed(float f)
{
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    return JOG_MIN_SPEED + (int32_t)(f * (float)(JOG_MAX_SPEED - JOG_MIN_SPEED));
}

// ============================================================
// Control loop: derive state -> ramp -> apply
// ============================================================
static void jog_task(void *arg)
{
    const float dt = 1.0f / JOG_TASK_RATE_HZ;
    const int32_t ramp_step = (int32_t)(JOG_ACCEL * dt);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000 / JOG_TASK_RATE_HZ));

        // 1. Derive state from held flags (both held = safe stop)
        app_state_t state;
        if      (s_btn2_held && !s_btn3_held) state = APP_STATE_JOG_CW;
        else if (s_btn3_held && !s_btn2_held) state = APP_STATE_JOG_CCW;
        else                                  state = APP_STATE_IDLE;

        // 2. Signed target speed = direction * pot magnitude
        float f = 0.0f;
        pot_read(s_pot, &f);
        const int32_t magnitude = pot_to_speed(f);

        int32_t target = 0;
        switch (state) {
            case APP_STATE_JOG_CW:  target =  magnitude; break;
            case APP_STATE_JOG_CCW: target = -magnitude; break;
            case APP_STATE_IDLE:    target =  0;         break;
        }

        // 3. Ramp current speed toward target
        int32_t speed = s_current_speed;
        if (speed < target) {
            speed = (speed + ramp_step > target) ? target : speed + ramp_step;
        } else if (speed > target) {
            speed = (speed - ramp_step < target) ? target : speed - ramp_step;
        }
        s_current_speed = speed;

        // 4. Active motor follows the ramp; the other one is stopped
        switch (s_profile) {
            case PROFILE_MOTOR_1:
                motor_apply(&s_motors[MOTOR_1], speed);
                motor_apply(&s_motors[MOTOR_2], 0);
                break;

            case PROFILE_MOTOR_2:
                motor_apply(&s_motors[MOTOR_1], 0);
                motor_apply(&s_motors[MOTOR_2], speed);
                break;

            case PROFILE_BOTH:
                motor_apply(&s_motors[MOTOR_1], speed);
                motor_apply(&s_motors[MOTOR_2], speed);
                break;
        }
    }
}

// ============================================================
// Public API
// ============================================================
esp_err_t jog_controller_init(void)
{
    ESP_LOGI(TAG, "Initializing jog controller...");

    ESP_ERROR_CHECK(motor_init(&s_motors[MOTOR_1],
                               HW_MOT1_UART_PORT, HW_MOT1_UART_TX, HW_MOT1_UART_RX,
                               HW_MOT1_EN_GPIO, HW_MOT1_STEP_GPIO, HW_MOT1_DIR_GPIO,
                               HW_MOT1_MOTION_MODE, 0));
    ESP_ERROR_CHECK(motor_init(&s_motors[MOTOR_2],
                               HW_MOT2_UART_PORT, HW_MOT2_UART_TX, HW_MOT2_UART_RX,
                               HW_MOT2_EN_GPIO, HW_MOT2_STEP_GPIO, HW_MOT2_DIR_GPIO,
                               HW_MOT2_MOTION_MODE, 1));

    pot_config_t pot_cfg = {
        .unit = HW_POT_ADC_UNIT,
        .channel = HW_POT_ADC_CHANNEL,
        .samples = 8,
    };
    ESP_ERROR_CHECK(pot_create(&pot_cfg, &s_pot));

    ESP_ERROR_CHECK(rgb_led_init());

    // Use the color map for the initial boot state
    rgb_led_set(s_profile_color[PROFILE_MOTOR_1][0],
                s_profile_color[PROFILE_MOTOR_1][1],
                s_profile_color[PROFILE_MOTOR_1][2]);

    ESP_ERROR_CHECK(app_buttons_init());

    // Register SINGLE CLICK for profile cycling
    ESP_ERROR_CHECK(app_buttons_register_cb(APP_BTN_1, BUTTON_SINGLE_CLICK, on_btn1_single_click, NULL));

    // Register HOLD events for jogging
    ESP_ERROR_CHECK(app_buttons_register_cb(APP_BTN_2, BUTTON_PRESS_DOWN, on_btn2_down, NULL));
    ESP_ERROR_CHECK(app_buttons_register_cb(APP_BTN_2, BUTTON_PRESS_UP,   on_btn2_up,   NULL));
    ESP_ERROR_CHECK(app_buttons_register_cb(APP_BTN_3, BUTTON_PRESS_DOWN, on_btn3_down, NULL));
    ESP_ERROR_CHECK(app_buttons_register_cb(APP_BTN_3, BUTTON_PRESS_UP,   on_btn3_up,   NULL));

    ESP_LOGI(TAG, "Jog controller ready.");
    return ESP_OK;
}

void jog_controller_start(void)
{
    xTaskCreate(jog_task, "jog_task", 4096, NULL, 5, NULL);
}