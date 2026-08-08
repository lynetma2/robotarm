idf_component_register(
        SRCS
            "pot.c"
        INCLUDE_DIRS
            "include"
        REQUIRES
            esp_adc       # the ESP-IDF ADC driver (oneshot + calibration)
            log
)