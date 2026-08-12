idf_component_register(
    SRCS "oled_display.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_lcd
        i2c_manager
)
