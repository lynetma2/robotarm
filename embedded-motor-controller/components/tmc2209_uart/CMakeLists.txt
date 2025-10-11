# Register the component with the ESP-IDF build system
idf_component_register(
        # List all source files for this component
        SRCS "tmc2209_uart.c"

        # Specify the directory containing public header files
        INCLUDE_DIRS "include"

        # List any other ESP-IDF components this one depends on
        REQUIRES esp_driver_uart
)