#include <stdio.h>
#include <inttypes.h>
#include "tmc_interface.h"
#include "tmc2209.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    printf("Scanning TMC2209 addresses...\n");
    tmc_interface_init();

    for (uint8_t addr = 0; addr < 8; addr++) {
        uint32_t val = tmc_read(addr, TMC2209_IOIN);
        if (val != 0xFFFFFFFF) {
            printf("✅ Found TMC2209 at address %u, IOIN=0x%08" PRIX32 "\n", addr, val);
        } else {
            printf("No response at address %u\n", addr);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
