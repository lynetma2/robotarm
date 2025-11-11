idf_component_register(SRCS "serial_comms.c"
        INCLUDE_DIRS "include"
        # It ALSO needs our motor_control component
        # to get the queue handle and command struct
        REQUIRES motor_control esp_driver_rmt esp_driver_gpio esp_driver_uart json
)