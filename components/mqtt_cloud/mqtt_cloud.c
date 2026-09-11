#include "mqtt_cloud.h"

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt_cloud";

/* ===========================================
 * 🔐 凭证配置 - 支持本地开发模式
 * =========================================== 
 * 
 * 配置优先级：
 * 1. credentials.local.h (本地开发，真实凭证) - 推荐
 * 2. 默认占位符 (公开代码，需手动替换)
 */

// 尝试检测并包含本地配置文件
#define USE_LOCAL_CREDENTIALS_MQTT 0  // 默认不使用

#ifdef __has_include
    // 🔑 关键：使用相对路径 ../../ 指向项目根目录
    #if __has_include("../../credentials.local.h")
        #ifndef CREDENTIALS_LOCAL_H
            #include "../../credentials.local.h"
        #endif
        #undef USE_LOCAL_CREDENTIALS_MQTT
        #define USE_LOCAL_CREDENTIALS_MQTT 1
    #elif __has_include("credentials.local.h")
        // 备用：尝试当前目录
        #ifndef CREDENTIALS_LOCAL_H
            #include "credentials.local.h"
        #endif
        #undef USE_LOCAL_CREDENTIALS_MQTT
        #define USE_LOCAL_CREDENTIALS_MQTT 1
    #endif
#endif

// 回退方案：如果 __has_include 不可用或失败
#if !USE_LOCAL_CREDENTIALS_MQTT
    #ifndef CREDENTIALS_LOCAL_H
        // 先尝试项目根目录
        #include "../../credentials.local.h"
    #endif
    // 检查是否成功定义了必要的宏
    #ifdef LOCAL_MQTT_BROKER_URI
        #undef USE_LOCAL_CREDENTIALS_MQTT
        #define USE_LOCAL_CREDENTIALS_MQTT 1
    #else
        // 再尝试当前目录
        #ifndef CREDENTIALS_LOCAL_H
            #include "credentials.local.h"
        #endif
        #ifdef LOCAL_MQTT_BROKER_URI
            #undef USE_LOCAL_CREDENTIALS_MQTT
            #define USE_LOCAL_CREDENTIALS_MQTT 1
        #endif
    #endif
#endif

// 最终确定使用哪个配置源
#if USE_LOCAL_CREDENTIALS_MQTT
    /* ✅ 使用本地凭证配置（从 credentials.local.h） */
    #ifdef LOCAL_MQTT_BROKER_URI
        #define MQTT_BROKER_URI  LOCAL_MQTT_BROKER_URI
    #else
        #define MQTT_BROKER_URI  "mqtts://YOUR_MQTT_BROKER:8883"
    #endif
    
    #ifdef LOCAL_MQTT_USERNAME
        #define MQTT_USERNAME    LOCAL_MQTT_USERNAME
    #else
        #define MQTT_USERNAME    "YOUR_MQTT_USERNAME"
    #endif
    
    #ifdef LOCAL_MQTT_PASSWORD
        #define MQTT_PASSWORD    LOCAL_MQTT_PASSWORD
    #else
        #define MQTT_PASSWORD    "YOUR_MQTT_PASSWORD"
    #endif
    
    #pragma message ("✅ MQTT: 使用 credentials.local.h 中的配置")
#else
    /* ⚠️ 使用默认占位符（未找到 credentials.local.h 或其中未定义必要宏） */
    #define MQTT_BROKER_URI  "mqtts://YOUR_MQTT_BROKER:8883"
    #define MQTT_USERNAME    "YOUR_MQTT_USERNAME"
    #define MQTT_PASSWORD    "YOUR_MQTT_PASSWORD"
    
    #warning "⚠️ MQTT: 未检测到 credentials.local.h，使用默认占位符"
    #warning "   解决方案: 运行 bash scripts/setup-credentials.sh 或手动创建 credentials.local.h"
#endif

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

    if (data->uart_ready) {
        len = snprintf(buf, sizeof(buf),
            "{\"temp\":%.1f,\"humi\":%.1f,\"pm1\":%.1f,\"pm25\":%.1f,\"pm10\":%.1f,"
            "\"tvoc\":%.1f,\"co2\":%.1f,\"pres\":%.1f,\"aq\":%u}",
            data->uart_temp, data->uart_humi,
            data->uart_pm1, data->uart_pm25, data->uart_pm10,
            data->uart_tvoc, data->uart_co2, data->uart_pres, data->uart_aq);
        esp_mqtt_client_publish(s_client, "sensor/uart", buf, 0, 1, 0);
    }
}