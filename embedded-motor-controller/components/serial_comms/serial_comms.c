//
// Created by 99sun on 11-11-2025.
//

#include "serial_comms.h"
#include "motor_control.h" // Needs this for motorQueues, MotorCommand, and NUM_MOTORS
#include "driver/uart.h"
#include "cJSON.h"
#include "string.h"
#include "esp_log.h"
#include <stdio.h>
#include <TMC2209_HW_Abstraction.h>

#include "TMC2209.h"
#include "freertos/FreeRTOS.h"
#include "driver/rmt_tx.h"
#include "freertos/queue.h"

static const char *TAG = "SERIAL_COMMS";

// --- TMC2209 register default base values (matching tmc_task initialization) ---

#define TMC2209_IHOLD_IRUN_DEFAULT  0x00071703  // IHOLDDELAY=7, IRUN=23, IHOLD=3
#define TMC2209_CHOPCONF_DEFAULT    0x10000053  // 256 microsteps, TOFF=3, etc.
#define TMC2209_TPWMTHRS_DEFAULT    0x00000000
#define TMC2209_TCOOLTHRS_DEFAULT   0x00000000

// --- IHOLD_IRUN bit layout ---
// [4:0]   IHOLD      (hold current)
// [12:8]  IRUN       (run current)
// [19:16] IHOLDDELAY (delay before hold current kicks in)

#define IHOLD_MASK       0x0000001F
#define IHOLD_SHIFT      0
#define IRUN_MASK        0x00001F00
#define IRUN_SHIFT       8
#define IHOLDDELAY_MASK  0x000F0000
#define IHOLDDELAY_SHIFT 16

// --- CHOPCONF bit layout (microstep resolution) ---
// [27:24] MRES  (0=256, 1=128, 2=64, 3=32, 4=16, 5=8, 6=4, 7=2, 8=1)

#define MRES_MASK   0x0F000000
#define MRES_SHIFT  24

// Converts a microstep count to the MRES register value
static uint8_t microsteps_to_mres(int microsteps) {
    switch (microsteps) {
        case 256: return 0;
        case 128: return 1;
        case  64: return 2;
        case  32: return 3;
        case  16: return 4;
        case   8: return 5;
        case   4: return 6;
        case   2: return 7;
        case   1: return 8;
        default:
            ESP_LOGW(TAG, "Invalid microsteps value %d, defaulting to 256", microsteps);
            return 0;
    }
}

/**
 * @brief Handles the "settings" command to update TMC2209 register values at runtime.
 *
 * Reconstructs affected registers from their default base values, applies only
 * the fields present in the JSON, then writes the full 32-bit register value.
 * Omitted fields retain their default values.
 *
 * Supported fields:
 *   "ihold"      [0–31]  Hold current
 *   "irun"       [0–31]  Run current
 *   "iholddelay" [0–15]  Delay before dropping to hold current
 *   "tpwmthrs"   [0–2^32] SpreadCycle/StealthChop threshold (0 = always SpreadCycle)
 *   "tcoolthrs"  [0–2^32] CoolStep threshold (0 = disabled)
 *   "microsteps" [1,2,4,8,16,32,64,128,256]
 *
 * Example JSON:
 * {
 *   "seq": 3,
 *   "cmd": "settings",
 *   "data": {
 *     "motor_id": 0,
 *     "irun": 28,
 *     "ihold": 16,
 *     "iholddelay": 7,
 *     "tpwmthrs": 0,
 *     "tcoolthrs": 0,
 *     "microsteps": 16
 *   }
 * }
 */
static void handle_settings_command(const cJSON *data) {
    if (!cJSON_IsObject(data)) {
        ESP_LOGE(TAG, "Settings command missing 'data' object");
        return;
    }

    const cJSON *motor_id_item = cJSON_GetObjectItem(data, "motor_id");
    if (!cJSON_IsNumber(motor_id_item)) {
        ESP_LOGE(TAG, "Settings command missing 'motor_id'");
        return;
    }

    int motor_id = motor_id_item->valueint;
    if (motor_id < 0 || motor_id >= NUM_MOTORS) {
        ESP_LOGE(TAG, "Settings command invalid motor_id: %d", motor_id);
        return;
    }

    const cJSON *irun       = cJSON_GetObjectItem(data, "irun");
    const cJSON *ihold      = cJSON_GetObjectItem(data, "ihold");
    const cJSON *iholddelay = cJSON_GetObjectItem(data, "iholddelay");
    const cJSON *tpwmthrs   = cJSON_GetObjectItem(data, "tpwmthrs");
    const cJSON *tcoolthrs  = cJSON_GetObjectItem(data, "tcoolthrs");
    const cJSON *microsteps = cJSON_GetObjectItem(data, "microsteps");

    // --- IHOLD_IRUN: reconstruct from default, patch present fields ---
    if (cJSON_IsNumber(irun) || cJSON_IsNumber(ihold) || cJSON_IsNumber(iholddelay)) {
        uint32_t reg = TMC2209_IHOLD_IRUN_DEFAULT;

        if (cJSON_IsNumber(ihold)) {
            reg = (reg & ~IHOLD_MASK) | (((uint32_t)ihold->valueint << IHOLD_SHIFT) & IHOLD_MASK);
        }
        if (cJSON_IsNumber(irun)) {
            reg = (reg & ~IRUN_MASK) | (((uint32_t)irun->valueint << IRUN_SHIFT) & IRUN_MASK);
        }
        if (cJSON_IsNumber(iholddelay)) {
            reg = (reg & ~IHOLDDELAY_MASK) | (((uint32_t)iholddelay->valueint << IHOLDDELAY_SHIFT) & IHOLDDELAY_MASK);
        }

        tmc2209_writeRegister(motor_id, TMC2209_IHOLD_IRUN, reg);
        ESP_LOGI(TAG, "Motor %d IHOLD_IRUN updated: 0x%08" PRIX32, motor_id, reg);
    }

    // --- CHOPCONF: reconstruct from default, patch microsteps ---
    if (cJSON_IsNumber(microsteps)) {
        uint32_t mres = microsteps_to_mres(microsteps->valueint);
        uint32_t reg  = (TMC2209_CHOPCONF_DEFAULT & ~MRES_MASK) | ((mres << MRES_SHIFT) & MRES_MASK);

        tmc2209_writeRegister(motor_id, TMC2209_CHOPCONF, reg);
        ESP_LOGI(TAG, "Motor %d CHOPCONF updated: 0x%08" PRIX32, motor_id, reg);
    }

    // --- TPWMTHRS and TCOOLTHRS: single-field registers, write directly ---
    if (cJSON_IsNumber(tpwmthrs)) {
        uint32_t reg = (uint32_t)tpwmthrs->valueint;
        tmc2209_writeRegister(motor_id, TMC2209_TPWMTHRS, reg);
        ESP_LOGI(TAG, "Motor %d TPWMTHRS updated: 0x%08" PRIX32, motor_id, reg);
    }

    if (cJSON_IsNumber(tcoolthrs)) {
        uint32_t reg = (uint32_t)tcoolthrs->valueint;
        tmc2209_writeRegister(motor_id, TMC2209_TCOOLTHRS, reg);
        ESP_LOGI(TAG, "Motor %d TCOOLTHRS updated: 0x%08" PRIX32, motor_id, reg);
    }
}

/**
 * @brief Handles the "step" command, enqueuing motion segments for one or more motors.
 *
 * Each entry in "segments" targets a single motor and describes a trapezoidal
 * motion profile (accelerate → uniform → decelerate). Multiple segments can be
 * sent in one command to move several motors.
 *
 * Example JSON:
 * {
 *   "seq": 2,
 *   "cmd": "step",
 *   "data": {
 *     "segments": [
 *       {
 *         "motor_id": 0,
 *         "direction": 1,
 *         "accel_steps": 200,
 *         "uniform_steps": 1000,
 *         "decel_steps": 200,
 *         "start_freq": 100,
 *         "uniform_freq": 1000,
 *         "end_freq": 100
 *       },
 *       {
 *         "motor_id": 1,
 *         "direction": 0,
 *         "accel_steps": 100,
 *         "uniform_steps": 500,
 *         "decel_steps": 100,
 *         "start_freq": 200,
 *         "uniform_freq": 800,
 *         "end_freq": 200
 *       }
 *     ]
 *   }
 * }
 */
static void handle_step_command(const cJSON *data) {
    if (!cJSON_IsObject(data)) {
        ESP_LOGE(TAG, "Step command missing 'data' object");
        return;
    }

    const cJSON *segments = cJSON_GetObjectItem(data, "segments");
    if (!cJSON_IsArray(segments)) {
        ESP_LOGE(TAG, "Step command 'data' missing 'segments' array");
        return;
    }

    cJSON *segment = NULL;
    cJSON_ArrayForEach(segment, segments) {
        const cJSON *motor_id      = cJSON_GetObjectItem(segment, "motor_id");
        const cJSON *direction     = cJSON_GetObjectItem(segment, "direction");
        const cJSON *accel_steps   = cJSON_GetObjectItem(segment, "accel_steps");
        const cJSON *uniform_steps = cJSON_GetObjectItem(segment, "uniform_steps");
        const cJSON *decel_steps   = cJSON_GetObjectItem(segment, "decel_steps");
        const cJSON *start_freq    = cJSON_GetObjectItem(segment, "start_freq");
        const cJSON *uniform_freq  = cJSON_GetObjectItem(segment, "uniform_freq");
        const cJSON *end_freq      = cJSON_GetObjectItem(segment, "end_freq");

        if (!motor_id || !direction || !accel_steps || !uniform_steps ||
            !decel_steps || !start_freq || !uniform_freq || !end_freq) {
            ESP_LOGW(TAG, "Skipping segment due to missing fields");
            continue;
        }

        int id = motor_id->valueint;
        if (id < 0 || id >= NUM_MOTORS) {
            ESP_LOGE(TAG, "Invalid motor_id: %d", id);
            continue;
        }

        MotorCommand cmd = {
            .motor_id        = id,
            .direction       = direction->valueint,
            .accel_steps     = accel_steps->valueint,
            .uniform_steps   = uniform_steps->valueint,
            .decel_steps     = decel_steps->valueint,
            .start_freq_hz   = start_freq->valueint,
            .uniform_freq_hz = uniform_freq->valueint,
            .end_freq_hz     = end_freq->valueint,
        };

        if (xQueueSend(motorQueues[id], &cmd, pdMS_TO_TICKS(10)) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to enqueue command for motor %d (queue full)", id);
        }
    }
}

/**
 * @brief Parses an incoming JSON string and dispatches it to the appropriate command handler.
 *
 * The top-level object must contain a "cmd" string field. An optional "seq" number
 * field, if present, will be ACK'd after the command is dispatched regardless of
 * whether the command succeeded.
 *
 * Supported commands: "step", "settings"
 *
 * Minimal example (seq is optional):
 * {
 *   "seq": 7,
 *   "cmd": "step",
 *   "data": { ... }
 * }
 */
static void parse_and_dispatch_command(const char *json_string) {
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return;
    }

    double seq = -1.0;
    const cJSON *seq_item = cJSON_GetObjectItem(root, "seq");
    if (cJSON_IsNumber(seq_item)) {
        seq = seq_item->valuedouble;
    }

    const cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    const cJSON *data     = cJSON_GetObjectItem(root, "data");

    if (!cJSON_IsString(cmd_item) || cmd_item->valuestring == NULL) {
        ESP_LOGE(TAG, "JSON missing or invalid 'cmd' field");
    } else if (strcmp(cmd_item->valuestring, "step") == 0) {
        handle_step_command(data);
    } else if (strcmp(cmd_item->valuestring, "settings") == 0) {
        handle_settings_command(data);
    } else {
        ESP_LOGW(TAG, "Unknown command: %s", cmd_item->valuestring);
    }

    if (seq != -1.0) {
        send_ack(seq);
    }

    cJSON_Delete(root);
}

void serial_task(void *pvParameters)
{
    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));

    // Allocate buffer for reading data
    uint8_t *data = (uint8_t *)malloc(UART_BUF_SIZE);
    // Allocate buffer for accumulating a line
    char *line_buffer = (char *)malloc(UART_BUF_SIZE);
    int line_pos = 0;

    ESP_LOGI(TAG, "Serial task started. Waiting for JSON commands...");

    while (1) {
        // Read data from UART
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE - 1, pdMS_TO_TICKS(20));

        if (len > 0) {
            // Process all received bytes
            for (int i = 0; i < len; i++) {
                char c = data[i];
                // If newline, we have a complete line
                if (c == '\n' || c == '\r') {
                    if (line_pos > 0) { // We have data in the buffer
                        line_buffer[line_pos] = '\0'; // Null-terminate
                        ESP_LOGI(TAG, "Received line: %s", line_buffer);

                        parse_and_dispatch_command(line_buffer);
                    }
                    line_pos = 0; // Reset for next line
                } else if (line_pos < (UART_BUF_SIZE - 1)) {
                    // Add char to buffer
                    line_buffer[line_pos++] = c;
                }
            }
        }
    }
    free(data);
    free(line_buffer);
}