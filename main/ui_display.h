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
/* DUAL I2C SENSOR DATA STRUCTURE               */
/* ============================================ */
typedef struct {
    float a_temp;
    float a_humidity;
    float a_pm1;
    float a_pm25;
    float a_pm10;
    uint16_t a_tvoc;
    uint16_t a_no2;
    uint16_t a_hcho;

    float s_temp;
    float s_humidity;
    float s_pm1;
    float s_pm25;
    float s_pm10;
    float s_nox;
    float s_voc;
    uint16_t s_co2;

    uint16_t color_temp;
    uint16_t color_humid;
    uint16_t color_pm1;
    uint16_t color_pm25;
    uint16_t color_pm10;
    uint16_t color_co2;
    uint16_t color_voc;
} ui_dual_i2c_data_t;

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

/**
 * @brief Draw dual I2C sensor comparison table
 *
 * Common parameters: Temp, Humidity, PM1.0, PM2.5, PM10
 * Unique parameters shown below the table
 *
 * @param data  Pointer to dual sensor data structure
 */
void ui_draw_compare_table(const ui_dual_i2c_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* UI_DISPLAY_H */