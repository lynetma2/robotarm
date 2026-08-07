//
// Created by 99sun on 07-08-2026.
//

#include <string.h>
#include "tmc2209_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "tmc2209_port";

// ============================================================
// Internal Data Structures
// ============================================================

// Represents one initialized UART bus
typedef struct {
    bool initialized;
    uart_port_t uart_port;
    bool discard_echo;
    uint32_t timeout_ms;
    SemaphoreHandle_t mutex;
} tmc2209_bus_t;

// Represents one registered TMC2209 driver
typedef struct {
    bool registered;
    uint16_t ic_id;
    uart_port_t uart_port;    // Which bus this driver is on
    uint8_t node_address;     // Node address on that bus
} tmc2209_driver_entry_t;

// Global state
static struct {
    tmc2209_bus_t buses[TMC2209_MAX_BUSES];
    tmc2209_driver_entry_t drivers[TMC2209_MAX_DRIVERS];
} s_port;

// ============================================================
// Internal Helpers
// ============================================================

static tmc2209_bus_t *find_bus(uart_port_t uart_port)
{
    for (int i = 0; i < TMC2209_MAX_BUSES; i++) {
        if (s_port.buses[i].initialized && s_port.buses[i].uart_port == uart_port) {
            return &s_port.buses[i];
        }
    }
    return NULL;
}

static tmc2209_driver_entry_t *find_driver(uint16_t ic_id)
{
    for (int i = 0; i < TMC2209_MAX_DRIVERS; i++) {
        if (s_port.drivers[i].registered && s_port.drivers[i].ic_id == ic_id) {
            return &s_port.drivers[i];
        }
    }
    return NULL;
}

static bool uart_drain_bytes(uart_port_t port, size_t length, TickType_t timeout)
{
    uint8_t dummy[32];

    while (length > 0) {
        size_t chunk = (length < sizeof(dummy)) ? length : sizeof(dummy);
        int read_count = uart_read_bytes(port, dummy, chunk, timeout);

        if (read_count <= 0) {
            ESP_LOGW(TAG, "Timeout while draining %u bytes", (unsigned)length);
            return false;
        }

        length -= (size_t)read_count;
    }

    return true;
}

// ============================================================
// Bus Initialization
// ============================================================

esp_err_t tmc2209_port_init_bus(const tmc2209_bus_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check if this bus is already initialized
    if (find_bus(config->uart_port) != NULL) {
        ESP_LOGW(TAG, "UART port %d already initialized, reconfiguring", config->uart_port);
        // Find the existing entry and reconfigure it
        tmc2209_bus_t *bus = find_bus(config->uart_port);
        bus->discard_echo = config->discard_echo;
        bus->timeout_ms = config->timeout_ms;
        return ESP_OK;
    }

    // Find a free slot
    tmc2209_bus_t *bus = NULL;
    for (int i = 0; i < TMC2209_MAX_BUSES; i++) {
        if (!s_port.buses[i].initialized) {
            bus = &s_port.buses[i];
            break;
        }
    }

    if (bus == NULL) {
        ESP_LOGE(TAG, "No free bus slots. Max buses: %d", TMC2209_MAX_BUSES);
        return ESP_ERR_NO_MEM;
    }

    // Create mutex for this bus
    bus->mutex = xSemaphoreCreateMutex();
    if (bus->mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex for UART port %d", config->uart_port);
        return ESP_ERR_NO_MEM;
    }

    // Store configuration
    bus->uart_port = config->uart_port;
    bus->discard_echo = config->discard_echo;
    bus->timeout_ms = config->timeout_ms;

    // Configure UART peripheral
    const uart_config_t uart_config = {
        .baud_rate = (int)config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err;

    err = uart_param_config(config->uart_port, &uart_config);
    ESP_RETURN_ON_ERROR(err, TAG, "uart_param_config failed for port %d", config->uart_port);

    // Set pins if specified
    if (config->tx_pin >= 0 && config->rx_pin >= 0) {
        err = uart_set_pin(
            config->uart_port,
            config->tx_pin,
            config->rx_pin,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        );
        ESP_RETURN_ON_ERROR(err, TAG, "uart_set_pin failed for port %d", config->uart_port);
    }

    // Install UART driver
    err = uart_driver_install(
        config->uart_port,
        256,    // RX buffer size
        0,      // TX buffer size
        0,      // Event queue size
        NULL,   // Event queue handle
        0       // Allocation flags
    );

    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "UART driver already installed on port %d", config->uart_port);
    } else {
        ESP_RETURN_ON_ERROR(err, TAG, "uart_driver_install failed for port %d", config->uart_port);
    }

    bus->initialized = true;
    ESP_LOGI(TAG, "UART bus %d initialized (baud=%lu, echo_discard=%d)",
             config->uart_port,
             (unsigned long)config->baud_rate,
             config->discard_echo);

    return ESP_OK;
}

esp_err_t tmc2209_port_deinit(void)
{
    // Uninstall all UART drivers
    for (int i = 0; i < TMC2209_MAX_BUSES; i++) {
        if (s_port.buses[i].initialized) {
            uart_driver_delete(s_port.buses[i].uart_port);

            if (s_port.buses[i].mutex != NULL) {
                vSemaphoreDelete(s_port.buses[i].mutex);
            }

            s_port.buses[i].initialized = false;
        }
    }

    // Clear driver registry
    memset(s_port.drivers, 0, sizeof(s_port.drivers));

    ESP_LOGI(TAG, "All UART buses deinitialized");
    return ESP_OK;
}

// ============================================================
// Driver Registration
// ============================================================

esp_err_t tmc2209_port_register(uint16_t ic_id, uart_port_t uart_port, uint8_t node_address)
{
    // Validate node address
    if (node_address > 3) {
        ESP_LOGE(TAG, "Invalid node address %d (must be 0-3)", node_address);
        return ESP_ERR_INVALID_ARG;
    }

    // Check that the UART bus has been initialized
    if (find_bus(uart_port) == NULL) {
        ESP_LOGE(TAG, "UART port %d not initialized. Call tmc2209_port_init_bus() first.", uart_port);
        return ESP_ERR_INVALID_STATE;
    }

    // Check if this icID is already registered
    tmc2209_driver_entry_t *existing = find_driver(ic_id);
    if (existing != NULL) {
        ESP_LOGW(TAG, "icID %u already registered, updating", ic_id);
        existing->uart_port = uart_port;
        existing->node_address = node_address;
        return ESP_OK;
    }

    // Find a free slot
    for (int i = 0; i < TMC2209_MAX_DRIVERS; i++) {
        if (!s_port.drivers[i].registered) {
            s_port.drivers[i].registered = true;
            s_port.drivers[i].ic_id = ic_id;
            s_port.drivers[i].uart_port = uart_port;
            s_port.drivers[i].node_address = node_address;

            ESP_LOGI(TAG, "Registered icID=%u on UART%d with node_address=%u",
                     ic_id, uart_port, node_address);
            return ESP_OK;
        }
    }

    ESP_LOGE(TAG, "No free driver slots. Max drivers: %d", TMC2209_MAX_DRIVERS);
    return ESP_ERR_NO_MEM;
}

esp_err_t tmc2209_port_unregister(uint16_t ic_id)
{
    tmc2209_driver_entry_t *entry = find_driver(ic_id);
    if (entry == NULL) {
        ESP_LOGW(TAG, "icID %u not found in registry", ic_id);
        return ESP_ERR_NOT_FOUND;
    }

    entry->registered = false;
    ESP_LOGI(TAG, "Unregistered icID=%u", ic_id);
    return ESP_OK;
}

// ============================================================
// Runtime Configuration
// ============================================================

void tmc2209_port_set_echo_discard(uart_port_t uart_port, bool enable)
{
    tmc2209_bus_t *bus = find_bus(uart_port);
    if (bus != NULL) {
        bus->discard_echo = enable;
        ESP_LOGI(TAG, "Echo discard %s on UART%d",
                 enable ? "enabled" : "disabled", uart_port);
    } else {
        ESP_LOGW(TAG, "UART port %d not initialized", uart_port);
    }
}

// ============================================================
// Vendor Library Hooks
// ============================================================

uint8_t tmc2209_getNodeAddress(uint16_t icID)
{
    tmc2209_driver_entry_t *entry = find_driver(icID);
    if (entry == NULL) {
        ESP_LOGE(TAG, "getNodeAddress: icID %u not registered", icID);
        return 0;
    }
    return entry->node_address;
}

bool tmc2209_readWriteUART(
    uint16_t icID,
    uint8_t *data,
    size_t writeLength,
    size_t readLength
)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "readWriteUART: NULL data buffer");
        return false;
    }

    if (writeLength == 0 && readLength == 0) {
        return true;
    }

    // Look up which UART bus and node address this driver uses
    tmc2209_driver_entry_t *entry = find_driver(icID);
    if (entry == NULL) {
        ESP_LOGE(TAG, "readWriteUART: icID %u not registered", icID);
        return false;
    }

    // Find the bus configuration
    tmc2209_bus_t *bus = find_bus(entry->uart_port);
    if (bus == NULL) {
        ESP_LOGE(TAG, "readWriteUART: UART port %d not initialized", entry->uart_port);
        return false;
    }

    const uart_port_t port = bus->uart_port;
    const TickType_t timeout = pdMS_TO_TICKS(bus->timeout_ms);

    // Track if we successfully acquired the mutex so we know whether to release it
    bool mutex_acquired = false;
    bool success = true;

    // Lock this specific UART bus (not all buses)
    if (bus->mutex != NULL) {
        if (xSemaphoreTake(bus->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            mutex_acquired = true;
        } else {
            ESP_LOGE(TAG, "readWriteUART: failed to acquire mutex for UART%d", port);
            return false; // Return immediately, mutex was never acquired
        }
    }

    // Flush any stale received data
    if (success && uart_flush_input(port) != ESP_OK) {
        ESP_LOGE(TAG, "readWriteUART: uart_flush_input failed on UART%d", port);
        success = false;
    }

    // --- WRITE PHASE ---
    if (success && writeLength > 0) {
        int written = uart_write_bytes(port, data, writeLength);

        if (written != (int)writeLength) {
            ESP_LOGE(TAG, "readWriteUART: short write (%d/%u) on UART%d",
                     written, (unsigned)writeLength, port);
            success = false;
        } else if (uart_wait_tx_done(port, timeout) != ESP_OK) {
            ESP_LOGE(TAG, "readWriteUART: TX timeout on UART%d", port);
            success = false;
        }
    }

    // --- ECHO DISCARD PHASE ---
    if (success && bus->discard_echo && writeLength > 0) {
        if (!uart_drain_bytes(port, writeLength, timeout)) {
            ESP_LOGE(TAG, "readWriteUART: timeout discarding echo on UART%d", port);
            success = false;
        }
    }

    // --- READ PHASE ---
    if (success && readLength > 0) {
        uint8_t *read_ptr = data;
        size_t remaining = readLength;

        while (remaining > 0 && success) {
            int read_count = uart_read_bytes(port, read_ptr, remaining, timeout);

            if (read_count <= 0) {
                ESP_LOGE(TAG, "readWriteUART: read timeout on UART%d (%u bytes remaining)",
                         port, (unsigned)remaining);
                success = false;
            } else {
                read_ptr += read_count;
                remaining -= (size_t)read_count;
            }
        }
    }

    // --- CLEANUP ---
    if (mutex_acquired) {
        xSemaphoreGive(bus->mutex);
    }

    return success;
}