#include "wifi_web.h"

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

static const char *TAG = "wifi_web";

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
    #define WIFI_SSID      LOCAL_WIFI_SSID
    #define WIFI_PASS      LOCAL_WIFI_PASS
    #define WIFI_MAX_RETRY LOCAL_WIFI_MAX_RETRY
    
    #ifdef LOCAL_CREDENTIALS_DEBUG
        #if LOCAL_CREDENTIALS_DEBUG
            #pragma message ("🔧 WiFi: 使用 credentials.local.h 中的配置")
        #endif
    #endif
#else
    /* 使用默认占位符（需手动替换为真实值） */
    #define WIFI_SSID      "YOUR_WIFI_SSID"
    #define WIFI_PASS      "YOUR_WIFI_PASSWORD"
    #define WIFI_MAX_RETRY 10
    
    #warning "⚠️ WiFi: 使用默认占位符，请配置 credentials.local.h 或直接修改下方值"
#endif

static int s_retry_num = 0;
static char s_ip_str[16] = {0};

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
            s_retry_num++;
            ESP_LOGI(TAG, "WiFi retry %d/%d (waiting 3s...)", s_retry_num, WIFI_MAX_RETRY);
            vTaskDelay(pdMS_TO_TICKS(3000));
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "WiFi connected! IP: %s", s_ip_str);
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_set_ps(WIFI_PS_NONE);

        {
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif) {
                esp_netif_dns_info_t dns;
                dns.ip.type = ESP_IPADDR_TYPE_V4;
                dns.ip.u_addr.ip4.addr = esp_ip4addr_aton("8.8.8.8");
                esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns);
                dns.ip.u_addr.ip4.addr = esp_ip4addr_aton("114.114.114.114");
                esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns);
                ESP_LOGI(TAG, "DNS set via esp_netif: 8.8.8.8 / 114.114.114.114");
            } else {
                ESP_LOGW(TAG, "esp_netif_get_handle_from_ifkey returned NULL");
            }
        }

        {
            ip_addr_t dns1, dns2;
            ipaddr_aton("8.8.8.8", &dns1);
            ipaddr_aton("114.114.114.114", &dns2);
            dns_setserver(0, &dns1);
            dns_setserver(1, &dns2);
            ESP_LOGI(TAG, "DNS set via lwIP: 8.8.8.8 / 114.114.114.114");
        }
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

esp_err_t wifi_web_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();
    return ESP_OK;
}

const char* wifi_web_get_ip_str(void)
{
    return s_ip_str;
}