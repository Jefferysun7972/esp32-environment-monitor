#ifndef INFLUXDB_WRITER_H
#define INFLUXDB_WRITER_H

#include <stdbool.h>
#include "esp_err.h"
#include "mqtt_cloud.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t influxdb_writer_init(void);
void influxdb_writer_send(const mqtt_sensor_data_t *data);

#ifdef __cplusplus
}
#endif

#endif