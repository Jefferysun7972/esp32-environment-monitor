#include "wifi_web.h"
#include "web_dashboard.h"

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static const char *TAG = "wifi_web";

#define WIFI_SSID      "HUAWEI-mate9"
#define WIFI_PASS      "13208286167"
#define WIFI_MAX_RETRY 5

static int s_retry_num = 0;
static char s_ip_str[16] = {0};
static wifi_sensor_data_t s_sensor_data;
static bool s_wifi_connected = false;

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "WiFi retry %d/%d", s_retry_num, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "WiFi connected! IP: %s", s_ip_str);
        s_retry_num = 0;
        s_wifi_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                        ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                        IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi STA init finished. Connecting to %s...", WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected successfully! IP: %s", s_ip_str);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "WiFi connection failed after %d retries", WIFI_MAX_RETRY);
    }
}

static esp_err_t api_sensors_get_handler(httpd_req_t *req)
{
    char buf[1024];
    int len = snprintf(buf, sizeof(buf),
        "{"
        "\"ip\":\"%s\","
        "\"am2020dy_temp\":%.1f,\"am2020dy_humi\":%.1f,"
        "\"am2020dy_pm1\":%.1f,\"am2020dy_pm25\":%.1f,\"am2020dy_pm10\":%.1f,"
        "\"am2020dy_tvoc\":%.1f,\"am2020dy_no2\":%.1f,\"am2020dy_hcho\":%.1f,"
        "\"sen_ready\":%s,"
        "\"sen_temp\":%.1f,\"sen_humi\":%.1f,"
        "\"sen_pm1\":%.1f,\"sen_pm25\":%.1f,\"sen_pm10\":%.1f,"
        "\"sen_tvoc\":%.1f,\"sen_nox\":%.1f,\"sen_co2\":%.1f,\"sen_hcho\":%.1f,"
        "\"sen_name\":\"%s\","
        "\"alert_level\":%d,\"alert_msg\":\"%s\""
        "}",
        s_ip_str,
        s_sensor_data.am2020dy_temp, s_sensor_data.am2020dy_humi,
        s_sensor_data.am2020dy_pm1, s_sensor_data.am2020dy_pm25, s_sensor_data.am2020dy_pm10,
        s_sensor_data.am2020dy_tvoc, s_sensor_data.am2020dy_no2, s_sensor_data.am2020dy_hcho,
        s_sensor_data.sen_ready ? "true" : "false",
        s_sensor_data.sen_temp, s_sensor_data.sen_humi,
        s_sensor_data.sen_pm1, s_sensor_data.sen_pm25, s_sensor_data.sen_pm10,
        s_sensor_data.sen_tvoc, s_sensor_data.sen_nox, s_sensor_data.sen_co2, s_sensor_data.sen_hcho,
        s_sensor_data.sen_name,
        s_sensor_data.alert_level, s_sensor_data.alert_msg
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, buf, len);
    return ESP_OK;
}

static esp_err_t index_html_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_index = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = index_html_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &uri_index);

        httpd_uri_t uri_api = {
            .uri       = "/api/sensors",
            .method    = HTTP_GET,
            .handler   = api_sensors_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &uri_api);

        ESP_LOGI(TAG, "Web server started on http://%s/", s_ip_str);
    }
    return server;
}

esp_err_t wifi_web_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();
    start_webserver();
    return ESP_OK;
}

void wifi_web_update_data(const wifi_sensor_data_t *data)
{
    memcpy(&s_sensor_data, data, sizeof(wifi_sensor_data_t));
}

const char* wifi_web_get_ip_str(void)
{
    return s_ip_str;
}