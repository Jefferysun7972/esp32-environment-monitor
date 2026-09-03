#include "mqtt_cloud.h"

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt_cloud";

#define MQTT_BROKER_URI  "mqtts://f4319339.ala.cn-hangzhou.emqxsl.cn:8883"
#define MQTT_USERNAME    "jerrysun"
#define MQTT_PASSWORD    "renyi1004"

static esp_mqtt_client_handle_t s_client = NULL;
static bool s_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected to broker");
        s_connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT disconnected");
        s_connected = false;
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;
    default:
        break;
    }
}

esp_err_t mqtt_cloud_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_BROKER_URI,
            .verification.crt_bundle_attach = esp_crt_bundle_attach,
        },
        .credentials = {
            .username = MQTT_USERNAME,
            .authentication.password = MQTT_PASSWORD,
        },
        .session = {
            .keepalive = 120,
            .disable_clean_session = false,
        },
        .network = {
            .disable_auto_reconnect = false,
            .reconnect_timeout_ms = 15000,
            .timeout_ms = 10000,
        },
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT client");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);

    ESP_LOGI(TAG, "MQTT client started, connecting to %s...", MQTT_BROKER_URI);
    return ESP_OK;
}

void mqtt_cloud_publish(const mqtt_sensor_data_t *data)
{
    if (!s_connected || s_client == NULL) {
        return;
    }

    char buf[512];

    int len = snprintf(buf, sizeof(buf),
        "{\"temp\":%.1f,\"humi\":%.1f,\"pm1\":%.1f,\"pm25\":%.1f,\"pm10\":%.1f,"
        "\"tvoc\":%.1f,\"no2\":%.1f,\"hcho\":%.1f}",
        data->am2020dy_temp, data->am2020dy_humi,
        data->am2020dy_pm1, data->am2020dy_pm25, data->am2020dy_pm10,
        data->am2020dy_tvoc, data->am2020dy_no2, data->am2020dy_hcho);
    esp_mqtt_client_publish(s_client, "sensor/am2020dy", buf, 0, 1, 0);

    if (data->sen_ready) {
        len = snprintf(buf, sizeof(buf),
            "{\"temp\":%.1f,\"humi\":%.1f,\"pm1\":%.1f,\"pm25\":%.1f,\"pm10\":%.1f,"
            "\"tvoc\":%.1f,\"nox\":%.1f,\"co2\":%.1f,\"hcho\":%.1f}",
            data->sen_temp, data->sen_humi,
            data->sen_pm1, data->sen_pm25, data->sen_pm10,
            data->sen_tvoc, data->sen_nox, data->sen_co2, data->sen_hcho);
        char topic[32];
        snprintf(topic, sizeof(topic), "sensor/%s", data->sen_name);
        esp_mqtt_client_publish(s_client, topic, buf, 0, 1, 0);
    }
}