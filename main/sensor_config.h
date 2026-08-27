/*
 * Unified Sensor Configuration Header
 * 
 * Runtime automatic detection of SEN54 / SEN66 / SEN68 sensor type
 * Uses Get Product Name command (0xD014) to identify sensor at startup
 * 
 * Usage:
 *   1. Include this file BEFORE any sensor-specific headers
 *   2. Call detect_sensor_type() during initialization
 *   3. Use global variables: g_sensor_type, g_sensor_name
 *   4. Check feature flags: g_has_pm1_support, g_has_nox_support, g_has_hcho_support, g_has_co2_support
 */

#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <string.h>
#include "esp_log.h"

/* Include ALL sensor headers for runtime switching */
#include "sen5x_i2c.h"
#include "sen66_i2c.h"
#include "sen68_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================ */
/* SENSOR TYPE ENUMERATION                     */
/* ============================================ */
#define SENSOR_SEN54  0
#define SENSOR_SEN66  1
#define SENSOR_SEN68  2

/* ============================================ */
/* RUNTIME SENSOR DETECTION GLOBALS            */
/* Set by detect_sensor_type() at runtime      */
/* ============================================ */
extern int g_sensor_type;              /* SENSOR_SEN54 / SENSOR_SEN66 / SENSOR_SEN68 */
extern const char* g_sensor_name;       /* "SEN54" / "SEN66" / "SEN68" */
extern int g_has_pm1_support;          /* PM1.0 ultrafine particles */
extern int g_has_nox_support;          /* NOx index (SEN66/SEN68 only) */
extern int g_has_hcho_support;         /* HCHO formaldehyde in ppb (SEN68 only) */
extern int g_has_co2_support;          /* CO2 ppm (SEN66 only) */

/* ============================================ */
/* SENSOR DETECTION FUNCTION                   */
/* Call once during initialization             */
/* ============================================ */
int detect_sensor_type(void);

/* ============================================ */
/* UNIFIED API FUNCTIONS                       */
/* Automatically route to correct sensor       */
/* Based on runtime detection result           */
/* ============================================ */
int16_t sensor_device_reset(void);
int16_t sensor_start_measurement(void);
int16_t sensor_stop_measurement(void);
int16_t sensor_get_product_name(int8_t* name, uint16_t size);
int16_t sensor_read_measured_values(uint16_t* pm1p0, uint16_t* pm2p5,
                                     uint16_t* pm4p0, uint16_t* pm10p0,
                                     int16_t* humidity, int16_t* temperature,
                                     int16_t* voc_index, int16_t* nox_index,
                                     uint16_t* hcho_or_co2);

/* ============================================ */
/* COMMON CONSTANTS (shared by both sensors)    */
/* ============================================ */
#define SENSOR_I2C_ADDR             0x6B
#define SENSOR_READ_PERIOD_MS        5000    /* Read interval: 5 seconds */
#define SENSOR_WARMUP_DELAY_MS       15000   /* Warm-up time: 15 seconds */
#define SENSOR_WARMUP_READINGS       3       /* Skip first N readings */

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_CONFIG_H */