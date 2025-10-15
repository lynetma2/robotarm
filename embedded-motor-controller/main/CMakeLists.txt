idf_component_register(SRCS "embedded-motor-controller.c"
                    REQUIRES spi_flash esp_driver_uart esp_driver_gpio tmc2209_uart
                    INCLUDE_DIRS ".")
