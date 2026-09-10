#include "influxdb_writer.h"

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include <sys/socket.h>

static const char *TAG = "influxdb";

/* ===========================================
 * 🔐 凭证配置 - 支持本地开发模式
 * =========================================== 
 * 优先级：
 * 1. credentials.local.h (本地开发，真实凭证)
 * 2. 默认占位符 (公开代码，需替换)
 */

#ifdef __has_include
    #if __has_include("credentials.local.h")
        #include "credentials.local.h"
        #define USE_LOCAL_CREDENTIALS 1
    #else
        #define USE_LOCAL_CREDENTIALS 0
    #endif
#else
    #define USE_LOCAL_CREDENTIALS 0
#endif

#if USE_LOCAL_CREDENTIALS
    /* 使用本地凭证配置 */
    #define INFLUXDB_URL    LOCAL_INFLUXDB_URL
    #define INFLUXDB_HOST   LOCAL_INFLUXDB_HOST
    #define INFLUXDB_ORG    LOCAL_INFLUXDB_ORG
    #define INFLUXDB_BUCKET LOCAL_INFLUXDB_BUCKET
    #define INFLUXDB_TOKEN  LOCAL_INFLUXDB_TOKEN
    
    #ifdef LOCAL_CREDENTIALS_DEBUG
        #if LOCAL_CREDENTIALS_DEBUG
            #pragma message ("🔧 InfluxDB: 使用 credentials.local.h 中的配置")
        #endif
    #endif
#else
    /* 使用默认占位符（需手动替换为真实值） */
    #define INFLUXDB_URL    "https://YOUR_INFLUXDB_URL"
    #define INFLUXDB_HOST   "YOUR_INFLUXDB_URL"
    #define INFLUXDB_ORG    "YOUR_ORG_NAME"
    #define INFLUXDB_BUCKET "sensor_data"
    #define INFLUXDB_TOKEN  "YOUR_INFLUXDB_TOKEN"
    
    #warning "⚠️ InfluxDB: 使用默认占位符，请配置 credentials.local.h 或直接修改下方值"
#endif

#define INFLUXDB_TASK_STACK  8192
#define INFLUXDB_TASK_PRIO   5
#define INFLUXDB_QUEUE_LEN   16

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

        if (data.uart_ready) {
            len = snprintf(line, sizeof(line),
                "uart,device=esp32 temp=%.1f,humi=%.1f,pm1=%.1f,pm25=%.1f,pm10=%.1f,tvoc=%.1f,co2=%.1f,pres=%.1f,aq=%u\n",
                data.uart_temp, data.uart_humi,
                data.uart_pm1, data.uart_pm25, data.uart_pm10,
                data.uart_tvoc, data.uart_co2, data.uart_pres, data.uart_aq);
            if (len > 0 && len < (int)sizeof(line)) {
                memcpy(body + total_len, line, len);
                total_len += len;
            }
        }

        if (total_len == 0) {
            continue;
        }

        ESP_LOGI(TAG, "Sending to URL: %s", s_write_url);

        {
            struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
            struct addrinfo *res = NULL;
            int rc = getaddrinfo(INFLUXDB_HOST, NULL, &hints, &res);
            if (rc == 0 && res != NULL) {
                char ip_str[16];
                struct sockaddr_in *sa = (struct sockaddr_in *)res->ai_addr;
                snprintf(ip_str, sizeof(ip_str), "%s", inet_ntoa(sa->sin_addr));
                ESP_LOGI(TAG, "DNS resolved: %s -> %s", INFLUXDB_HOST, ip_str);
                freeaddrinfo(res);
            } else {
                ESP_LOGW(TAG, "DNS pre-check failed: getaddrinfo returned %d", rc);
            }
        }

        esp_http_client_config_t config = {
            .url = s_write_url,
            .method = HTTP_METHOD_POST,
            .timeout_ms = 15000,
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