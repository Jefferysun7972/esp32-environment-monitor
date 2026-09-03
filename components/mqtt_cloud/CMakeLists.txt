idf_component_register(
    SRCS "mqtt_cloud.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt esp_wifi nvs_flash
)