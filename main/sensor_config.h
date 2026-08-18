/*
 * Unified Sensor Configuration Header
 * 
 * Automatically selects between SEN66 and SEN68 based on menuconfig
 * Provides unified API macros for transparent sensor switching
 * 
 * Usage:
 *   1. Include this file BEFORE any sensor-specific headers
 *   2. Use sensor_*() macros instead of direct sen66_/sen68_ calls
 *   3. Compile with CONFIG_SENSOR_TYPE_SEN66 or CONFIG_SENSOR_TYPE_SEN68
 */

#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include "sdkconfig.h"

/* ============================================ */
/* SENSOR TYPE DETECTION                        */
/* Based on CONFIG_SENSOR_TYPE from Kconfig     */
/* ============================================ */

#ifdef CONFIG_SENSOR_TYPE_SEN68
    /* ======== SEN68 Advanced Sensor ======== */
    /* Features: All SEN66 + PM1.0 + HCHO */
    
    #define USE_SEN68_SENSOR        1
    #define USE_SEN66_SENSOR        0
    
    #define SENSOR_NAME             "SEN68"
    #define SENSOR_I2C_ADDR         0x6B
    
    /* Include SEN68 specific headers */
    #include "sen68_i2c.h"
    
    /* Feature flags */
    #define HAS_PM1_SUPPORT         1       /* PM1.0 ultrafine particles */
    #define HAS_HCHO_SUPPORT        1       /* HCHO formaldehyde in ppb */
    
    /* Unified API macros (map to SEN68 functions) */
    #define sensor_device_reset()              sen68_device_reset()
    #define sensor_start_measurement()         sen68_start_continuous_measurement()
    #define sensor_stop_measurement()          sen68_stop_continuous_measurement()
    #define sensor_read_measured_values(...)   \
            sen68_read_measured_values_as_integers(__VA_ARGS__)
            
#else
    /* ======== SEN66 Standard Sensor (Default) ======== */
    /* Features: Temp, Humid, PM2.5/4/10, VOC, NOx, CO2 */
    
    #define USE_SEN68_SENSOR        0
    #define USE_SEN66_SENSOR        1
    
    #define SENSOR_NAME             "SEN66"
    #define SENSOR_I2C_ADDR         0x6B
    
    /* Include SEN66 specific headers */
    #include "sen66_i2c.h"
    
    /* Feature flags */
    #define HAS_PM1_SUPPORT         0       /* Not available on SEN66 */
    #define HAS_HCHO_SUPPORT        0       /* Not available on SEN66 */
    
    /* Unified API macros (map to SEN66 functions) */
    #define sensor_device_reset()              sen66_device_reset()
    #define sensor_start_measurement()         sen66_start_continuous_measurement()
    #define sensor_stop_measurement()          sen66_stop_continuous_measurement()
    #define sensor_read_measured_values(...)   \
            sen66_read_measured_values_as_integers(__VA_ARGS__)
            
#endif

/* ============================================ */
/* COMMON CONSTANTS (shared by both sensors)    */
/* ============================================ */

#define SENSOR_READ_PERIOD_MS        5000    /* Read interval: 5 seconds */
#define SENSOR_WARMUP_DELAY_MS       15000   /* Warm-up time: 15 seconds */
#define SENSOR_WARMUP_READINGS       3       /* Skip first N readings */

#endif /* SENSOR_CONFIG_H */