/*
 * UI Display Interface
 * 
 * Encapsulates all TFT-LCD drawing logic.
 * Decouples rendering from sensor reading and alert logic.
 */

#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

#include <stdint.h>
#include "tft_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================ */
/* SENSOR DATA STRUCTURE FOR UI RENDERING       */
/* ============================================ */
typedef struct {
    float temp_celsius;
    float humidity_pct;
    float nox_idx_f;
    float voc_idx_f;
    float pm2_5_ugm3;
    float pm1_ugm3;
    float pm10_ugm3;
    float pressure_hpa;
    uint16_t co2;
    uint16_t hcho_ppb;
    uint16_t aq_state;
    uint16_t color_temp;
    uint16_t color_humid;
    uint16_t color_nox;
    uint16_t color_voc;
    uint16_t color_pm25;
    uint16_t color_pm1;
    uint16_t color_pm10;
    uint16_t color_co2;
    uint16_t color_hcho;
    uint16_t color_pres;
    uint16_t color_aq;
    int global_level;
} ui_sensor_data_t;

/* ============================================ */
/* UI DRAWING FUNCTIONS                         */
/* ============================================ */

/**
 * @brief Draw the header area: title, sensor name, logo, separator
 * 
 * @param sensor_name  Runtime-detected sensor name ("SEN66" or "SEN68")
 */
void ui_draw_header(const char *sensor_name);

/**
 * @brief Draw the full sensor data screen with values and progress bars
 * 
 * Handles both initial layout (first draw) and incremental updates.
 * Adapts layout based on sensor type (SEN66 vs SEN68).
 * 
 * @param data  Pointer to sensor data structure with values and colors
 */
void ui_draw_sensor_screen(const ui_sensor_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* UI_DISPLAY_H */