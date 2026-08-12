/**
 * @file oled_display.h
 * @brief OLED display wrapper using ESP-IDF official esp_lcd driver for SSD1306
 * 
 * This module provides a simplified interface to the official ESP-IDF LCD driver,
 * specifically configured for SSD1306 128x64 OLED displays.
 */

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/* SSD1306 OLED Display Specifications */
#define OLED_WIDTH       128   /* Display width in pixels */
#define OLED_HEIGHT      64    /* Display height in pixels */
#define OLED_I2C_ADDRESS 0x3C  /* I2C address (0x3C or 0x3D) */
#define OLED_I2C_PORT    0     /* I2C port number */

/* 
 * Debug mode: Uncomment to enable minimal test pattern during initialization
 * This will display basic patterns to diagnose data format issues.
 * Comment out after debugging to return to normal operation.
 */
#define OLED_DEBUG_TEST  /* ← Uncomment this line to enable debug test patterns */

/**
 * @brief Initialize OLED display with official esp_lcd driver
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t oled_init(void);

/**
 * @brief Update display with latest sensor data
 * 
 * @param temperature Temperature in Celsius
 * @param humidity Relative humidity in percentage  
 * @param pm25 PM2.5 concentration in µg/m³
 * @param co2 CO₂ concentration in ppm
 * @param voc VOC index value
 * @param nox NOx index value
 * @param is_alert Current alert status (true=alert, false=normal)
 * @return ESP_OK on success
 */
esp_err_t oled_update_display(
    float temperature,
    float humidity,
    float pm25,
    uint16_t co2,
    float voc,
    float nox,
    bool is_alert
);

/**
 * @brief Show startup/welcome message
 * @return ESP_OK on success
 */
esp_err_t oled_show_startup(void);

/**
 * @brief Show warm-up progress indicator
 * @param seconds_remaining Seconds until warm-up complete
 * @return ESP_OK on success
 */
esp_err_t oled_show_warmup(int seconds_remaining);

/**
 * @brief Clear entire screen
 * @return ESP_OK on success
 */
esp_err_t oled_clear(void);

/**
 * @brief Deinitialize and free resources
 * @return ESP_OK on success
 */
esp_err_t oled_deinit(void);

#endif /* OLED_DISPLAY_H */
