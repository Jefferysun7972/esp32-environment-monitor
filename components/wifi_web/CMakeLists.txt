idf_component_register(
    SRCS "wifi_web.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_http_server esp_event
)