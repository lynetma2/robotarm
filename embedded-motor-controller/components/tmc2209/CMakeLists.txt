idf_component_register(
        SRCS
            "src/TMC2209.c"
            "src/tmc2209_port.c"
            "src/tmc2209_dev.c"
        INCLUDE_DIRS
            "include"
        PRIV_INCLUDE_DIRS
            "private_include"
        REQUIRES                      # needed by tmc2209_dev.h (public header)
            esp_driver_uart           #   → uart_port_t, UART_NUM_1
            freertos                  #   → SemaphoreHandle_t
        PRIV_REQUIRES                 # only needed by your .c files
            esp_driver_gpio
            log
)