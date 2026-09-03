#ifndef MQTT_CLOUD_H
#define MQTT_CLOUD_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float am2020dy_temp;
    float am2020dy_humi;
    float am2020dy_pm1;
    float am2020dy_pm25;
    float am2020dy_pm10;
    float am2020dy_tvoc;
    float am2020dy_no2;
    float am2020dy_hcho;

    bool sen_ready;
    float sen_temp;
    float sen_humi;
    float sen_pm1;
    float sen_pm25;
    float sen_pm10;
    float sen_tvoc;
    float sen_nox;
    float sen_co2;
    float sen_hcho;

    char sen_name[8];
    int alert_level;
} mqtt_sensor_data_t;

esp_err_t mqtt_cloud_init(void);
void mqtt_cloud_publish(const mqtt_sensor_data_t *data);

#ifdef __cplusplus
}
#endif

#endif