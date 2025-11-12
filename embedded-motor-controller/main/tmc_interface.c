#include "driver/uart.h"
#include "driver/gpio.h"
#include "tmc_interface.h"
#include "tmc/helpers/CRC.h"   // or "tmc/helpers/CRC8.h" depending on your header name
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "TMC_UART"

// === UART CONFIG ===
#define UART_PORT       UART_NUM_2
#define UART_TX_PIN     8
#define UART_RX_PIN     7
#define UART_BAUD       115200

// A small helper to compute the inter-transfer pause (ms) like the Python code:
// communication_pause (s) = 500 / baud -> ms = 500000 / baud
static inline TickType_t tmc_comm_pause_ticks(void)
{
    uint32_t ms = (500000UL + (UART_BAUD/2)) / UART_BAUD; // rounded
    if (ms == 0) ms = 1;
    return pdMS_TO_TICKS(ms);
}

void tmc_interface_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install driver
    // RX buffer size: 1024 (adjust if needed), TX buffer 0 (we use blocking transmit)
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));

    // Set TX and RX pins (no RTS/CTS)
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART initialized (port=%d, tx=%d, rx=%d, baud=%d)", UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_BAUD);
}

bool tmc_write(uint8_t slave, uint8_t reg, uint32_t value)
{
    uint8_t packet[8];

    // Build write frame like the Python code:
    // [0x55, slave, reg|0x80, b3, b2, b1, b0, crc]
    packet[0] = 0x55;
    packet[1] = slave;
    packet[2] = (reg | 0x80);
    packet[3] = (value >> 24) & 0xFF;
    packet[4] = (value >> 16) & 0xFF;
    packet[5] = (value >> 8) & 0xFF;
    packet[6] = (value) & 0xFF;

    // compute CRC over all bytes except final CRC (matching your helper signature)
    // previous code used tmc_CRC8(packet, 6, 1) — Python uses compute_crc8_atm(w_frame[:-1])
    // For our 8-byte frame the CRC should be over first 7 bytes.
    packet[7] = tmc_CRC8(packet, 7, 1);

    int written = uart_write_bytes(UART_PORT, (const char *)packet, sizeof(packet));
    if (written != (int)sizeof(packet)) {
        ESP_LOGW(TAG, "tmc_write: uart_write_bytes wrote %d / %d", written, (int)sizeof(packet));
        return false;
    }

    // wait for TX to complete
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(20));
    // small pause after write (mirrors Python communication pause)
    vTaskDelay(tmc_comm_pause_ticks());

    return true;
}

uint32_t tmc_read(uint8_t slave, uint8_t reg)
{
    uint8_t request[4];
    uint8_t response[12];

    // Build read request like Python:
    // [0x55, slave, reg & 0x7F, crc]
    request[0] = 0x55;
    request[1] = slave;
    request[2] = (reg & 0x7F);
    request[3] = tmc_CRC8(request, 3, 1);

    // flush rx/tx buffers before sending
    uart_flush(UART_PORT);
    // send request
    int written = uart_write_bytes(UART_PORT, (const char *)request, sizeof(request));
    if (written != (int)sizeof(request)) {
        ESP_LOGW(TAG, "tmc_read: uart_write_bytes wrote %d / %d", written, (int)sizeof(request));
        return 0xFFFFFFFF;
    }
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(20));

    // small pause (like Python's time.sleep(communication_pause))
    vTaskDelay(tmc_comm_pause_ticks());

    // read response: Python expects 12 bytes, so we read that many
    int len = uart_read_bytes(UART_PORT, response, sizeof(response), pdMS_TO_TICKS(500));
    if (len != (int)sizeof(response)) {
        ESP_LOGW(TAG, "Read timeout or invalid response (len=%d)", len);
        return 0xFFFFFFFF;
    }

    // Python checks CRC: payload CRC computed over response[4..10] and compared to response[11]
    // compute CRC on bytes 4..10 (7 bytes)
    uint8_t crc = tmc_CRC8(&response[4], 7, 1);
    if (crc != response[11]) {
        ESP_LOGW(TAG, "CRC mismatch: computed=0x%02X, received=0x%02X", crc, response[11]);
        return 0xFFFFFFFF;
    }

    // extract 4 data bytes: Python used rtn_data = rtn[7:11]; then struct.unpack(">i", rtn_data)
    // so big-endian signed 4-byte int stored in response[7..10]
    uint32_t value = ((uint32_t)response[7] << 24) |
                     ((uint32_t)response[8] << 16) |
                     ((uint32_t)response[9] << 8)  |
                     ((uint32_t)response[10]);

    return value;
}
