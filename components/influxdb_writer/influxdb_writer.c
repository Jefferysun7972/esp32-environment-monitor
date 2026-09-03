#include "influxdb_writer.h"

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

static const char *TAG = "influxdb";

#define INFLUXDB_URL    "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_ORG    "Fellowes"
#define INFLUXDB_BUCKET "sensor_data"
#define INFLUXDB_TOKEN  "doR-H4EoxcxidC5AYN0NjzYQB7kJ5cusQvXe16b7j1W_tO4ouL35MlFayhPfTlnxR0djAgCwCFfgOVZSCXyzog=="

#define INFLUXDB_TASK_STACK  8192
#define INFLUXDB_TASK_PRIO   5
#define INFLUXDB_QUEUE_LEN   3

static char s_write_url[256];
static QueueHandle_t s_queue = NULL;

static void influxdb_task(void *pvParameters)
{
    mqtt_sensor_data_t data;
    char line[512];
    char body[1024];
    int total_len;
    int len;

    while (1) {
        if (xQueueReceive(s_queue, &data, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        total_len = 0;

        len = snprintf(line, sizeof(line),
            "am2020dy,device=esp32 temp=%.1f,humi=%.1f,pm1=%.1f,pm25=%.1f,pm10=%.1f,tvoc=%.1f,no2=%.1f,hcho=%.1f\n",
            data.am2020dy_temp, data.am2020dy_humi,
            data.am2020dy_pm1, data.am2020dy_pm25, data.am2020dy_pm10,
            data.am2020dy_tvoc, data.am2020dy_no2, data.am2020dy_hcho);
        if (len > 0 && len < (int)sizeof(line)) {
            memcpy(body + total_len, line, len);
            total_len += len;
        }

        if (data.sen_ready) {
            len = snprintf(line, sizeof(line),
                "%s,device=esp32 temp=%.1f,humi=%.1f,pm1=%.1f,pm25=%.1f,pm10=%.1f,tvoc=%.1f,nox=%.1f,hcho=%.1f\n",
                data.sen_name,
                data.sen_temp, data.sen_humi,
                data.sen_pm1, data.sen_pm25, data.sen_pm10,
                data.sen_tvoc, data.sen_nox, data.sen_hcho);
            if (len > 0 && len < (int)sizeof(line)) {
                memcpy(body + total_len, line, len);
                total_len += len;
            }
        }

        if (total_len == 0) {
            continue;
        }

        esp_http_client_config_t config = {
            .url = s_write_url,
            .method = HTTP_METHOD_POST,
            .timeout_ms = 10000,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);

        esp_http_client_set_header(client, "Authorization", "Token " INFLUXDB_TOKEN);
        esp_http_client_set_header(client, "Content-Type", "text/plain; charset=utf-8");
        esp_http_client_set_post_field(client, body, total_len);

        esp_err_t err = esp_http_client_perform(client);

        if (err == ESP_OK) {
            int status = esp_http_client_get_status_code(client);
            if (status == 204) {
                ESP_LOGI(TAG, "Data written to InfluxDB successfully");
            } else {
                ESP_LOGW(TAG, "InfluxDB write returned status %d", status);
            }
        } else {
            ESP_LOGW(TAG, "InfluxDB write failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);
    }
}

esp_err_t influxdb_writer_init(void)
{
    snprintf(s_write_url, sizeof(s_write_url),
             "%s/api/v2/write?org=%s&bucket=%s&precision=s",
             INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET);

    s_queue = xQueueCreate(INFLUXDB_QUEUE_LEN, sizeof(mqtt_sensor_data_t));
    if (s_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return ESP_FAIL;
    }

    BaseType_t ret = xTaskCreate(influxdb_task, "influxdb_task",
                                 INFLUXDB_TASK_STACK, NULL,
                                 INFLUXDB_TASK_PRIO, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        vQueueDelete(s_queue);
        s_queue = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "InfluxDB writer initialized");
    ESP_LOGI(TAG, "  URL: %s", INFLUXDB_URL);
    ESP_LOGI(TAG, "  Org: %s, Bucket: %s", INFLUXDB_ORG, INFLUXDB_BUCKET);

    return ESP_OK;
}

void influxdb_writer_send(const mqtt_sensor_data_t *data)
{
    if (s_queue == NULL) {
        return;
    }

    mqtt_sensor_data_t copy = *data;
    if (xQueueSend(s_queue, &copy, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Queue full, dropping data point");
    }
}