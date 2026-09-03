idf_component_register(
    SRCS "blink_example_main.c"
        "sensor_detect.c"
        "alert_manager.c"
        "ui_display.c"
    INCLUDE_DIRS "."
    REQUIRES
        i2c_manager
        embedded-i2c-sen5x
        embedded-i2c-sen66
        embedded-i2c-sen68
        uart_sensor
        am2020dy
        oled_display
        tft_lcd
        wifi_web
        mqtt_cloud
        influxdb_writer
)