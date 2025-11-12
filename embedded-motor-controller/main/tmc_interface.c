#include "driver/uart.h"
#include "driver/gpio.h"
#include "tmc_interface.h"
#include "tmc/helpers/CRC.h"
#include "esp_log.h"

#define TAG "TMC_UART"

// === UART CONFIG ===
#define UART_PORT UART_NUM_2
#define UART_TX_PIN        15
#define UART_RX_PIN        16
#define UART_BAUD 57600

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

    // Install driver in half-duplex mode
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));

    // Set both TX and RX to the same pin
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART initialized.");
}

void tmc_write(uint8_t slave, uint8_t reg, uint32_t value)
{
    uint8_t packet[8];
    packet[0] = slave;
    packet[1] = reg | 0x80;
    packet[2] = (value >> 24) & 0xFF;
    packet[3] = (value >> 16) & 0xFF;
    packet[4] = (value >> 8) & 0xFF;
    packet[5] = value & 0xFF;
    packet[6] = tmc_CRC8(packet, 6, 1);

    uart_write_bytes(UART_PORT, (const char *)packet, 7);
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(10));
}

uint32_t tmc_read(uint8_t slave, uint8_t reg)
{
    uint8_t request[4];
    uint8_t response[8];

    request[0] = slave;
    request[1] = reg & 0x7F; // Read access
    request[2] = tmc_CRC8(request, 2, 1);

    uart_flush(UART_PORT);
    uart_write_bytes(UART_PORT, (const char *)request, 3);
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(10));

    int len = uart_read_bytes(UART_PORT, response, 8, pdMS_TO_TICKS(500));
    if (len != 8) {
        ESP_LOGW(TAG, "Read timeout or invalid response (len=%d)", len);
        return 0xFFFFFFFF;
    }

    // Verify CRC
    uint8_t crc = tmc_CRC8(response, 7, 1);
    if (crc != response[7]) {
        ESP_LOGW(TAG, "CRC mismatch: expected 0x%02X, got 0x%02X", crc, response[7]);
        return 0xFFFFFFFF;
    }

    uint32_t value = (response[3] << 24) |
                     (response[4] << 16) |
                     (response[5] << 8)  |
                     (response[6]);

    return value;
}
