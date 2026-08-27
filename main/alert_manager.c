/*
 * Alert Manager Implementation
 */

#include "alert_manager.h"
#include "app_config.h"

uint16_t alert_get_color(float value, float normal_min, float normal_max,
                         float warning_max, bool is_inverted)
{
    if (!is_inverted) {
        if (value <= normal_max) {
            return TFT_BLUE;
        } else if (value <= warning_max) {
            return TFT_ORANGE;
        } else {
            return TFT_RED;
        }
    } else {
        if (value >= normal_min && value <= normal_max) {
            return TFT_BLUE;
        } else if ((value >= normal_min && value <= warning_max) ||
                   (value <= normal_max && value >= normal_min)) {
            return TFT_ORANGE;
        } else {
            return TFT_RED;
        }
    }
}

int alert_get_global_level(float temp_celsius, float humidity_pct, float nox_idx,
                           float pm2_5_ugm3, uint16_t hcho_or_co2, float voc_idx,
                           int is_hcho)
{
    int max_level = 0;

    if (temp_celsius < TEMP_DANGER_MIN || temp_celsius > TEMP_DANGER_MAX) {
        max_level = 2;
    } else if (temp_celsius < TEMP_NORMAL_MIN || temp_celsius > TEMP_NORMAL_MAX) {
        if (max_level < 1) max_level = 1;
    }

    if (humidity_pct < HUMID_DANGER_MIN || humidity_pct > HUMID_DANGER_MAX) {
        max_level = 2;
    } else if (humidity_pct < HUMID_NORMAL_MIN || humidity_pct > HUMID_NORMAL_MAX) {
        if (max_level < 1) max_level = 1;
    }

    if (nox_idx > NOX_WARNING_MAX) {
        max_level = 2;
    } else if (nox_idx > NOX_NORMAL_MAX && max_level < 1) {
        max_level = 1;
    }

    if (pm2_5_ugm3 > PM25_WARNING_MAX) {
        max_level = 2;
    } else if (pm2_5_ugm3 > PM25_NORMAL_MAX && max_level < 1) {
        max_level = 1;
    }

    if (is_hcho) {
        if ((float)hcho_or_co2 > HCHO_WARNING_MAX) {
            max_level = 2;
        } else if ((float)hcho_or_co2 > HCHO_NORMAL_MAX && max_level < 1) {
            max_level = 1;
        }
    } else {
        if (hcho_or_co2 > CO2_WARNING_MAX) {
            max_level = 2;
        } else if (hcho_or_co2 > CO2_NORMAL_MAX && max_level < 1) {
            max_level = 1;
        }
    }

    if (voc_idx > VOC_WARNING_MAX) {
        max_level = 2;
    } else if (voc_idx > VOC_NORMAL_MAX && max_level < 1) {
        max_level = 1;
    }

    return max_level;
}

bool alert_check_environmental(float pm25_ugm3, uint16_t co2_ppm)
{
    return (pm25_ugm3 > PM25_ALERT_THRESHOLD) || (co2_ppm > CO2_ALERT_THRESHOLD);
}