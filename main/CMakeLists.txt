idf_component_register(
    SRCS "blink_example_main.c"
    INCLUDE_DIRS "."
    REQUIRES
        i2c_manager
        embedded-i2c-sen66
        oled_display
)