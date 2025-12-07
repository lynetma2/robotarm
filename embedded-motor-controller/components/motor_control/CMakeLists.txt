idf_component_register(SRCS "motor_control.c" "stepper_motor_encoder.c"
        INCLUDE_DIRS "include"
        REQUIRES esp_driver_pcnt esp_driver_rmt esp_driver_gpio json
)