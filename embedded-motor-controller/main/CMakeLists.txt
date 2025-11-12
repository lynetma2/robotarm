idf_component_register(SRCS "embedded-test.c" "tmc_interface.c"
        REQUIRES motor_control serial_comms tmc
        PRIV_REQUIRES esp_driver_rmt esp_driver_gpio esp_driver_uart json
        INCLUDE_DIRS ".")