#ifndef WIFI_WEB_H
#define WIFI_WEB_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_web_init(void);
const char* wifi_web_get_ip_str(void);

#ifdef __cplusplus
}
#endif

#endif