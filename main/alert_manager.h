/*
 * Alert Manager Interface
 * 
 * Unified alert level calculation and color mapping.
 * Decouples business logic (threshold comparison) from UI rendering.
 */

#ifndef ALERT_MANAGER_H
#define ALERT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "tft_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get color level for a parameter value
 * 
 * @param value        Current measurement value
 * @param normal_min   Normal range lower bound
 * @param normal_max   Normal range upper bound (blue zone)
 * @param warning_max  Warning/danger boundary (>this is red)
 * @param is_inverted  If true, values OUTSIDE [normal_min, normal_max] are bad
 * @return uint16_t TFT color (TFT_BLUE / TFT_ORANGE / TFT_RED)
 */
uint16_t alert_get_color(float value, float normal_min, float normal_max,
                         float warning_max, bool is_inverted);

/**
 * @brief Calculate worst-case global alert level across all parameters
 * 
 * @return 0 = NORMAL (all blue), 1 = WARNING (at least one orange), 2 = DANGER (at least one red)
 */
int alert_get_global_level(float temp_celsius, float humidity_pct, float nox_idx,
                           float pm2_5_ugm3, uint16_t hcho_or_co2, float voc_idx,
                           int is_hcho);

/**
 * @brief Check if environmental thresholds are exceeded
 * 
 * @return true if PM2.5 or CO2 exceeds alert threshold
 */
bool alert_check_environmental(float pm25_ugm3, uint16_t co2_ppm);

#ifdef __cplusplus
}
#endif

#endif /* ALERT_MANAGER_H */