idf_component_register(
    SRCS "influxdb_writer.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_client esp-tls mqtt_cloud
)