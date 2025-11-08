idf_component_register(SRCS "embedded-test.c" "stepper_motor_encoder.c"
        PRIV_REQUIRES esp_driver_rmt esp_driver_gpio
        INCLUDE_DIRS ".")
