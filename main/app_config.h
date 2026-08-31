/*
 * Application Configuration
 * 
 * Centralized management of hardware configuration and threshold constants.
 * Extracted from blink_example_main.c for modular architecture.
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* ============================================ */
/* SENSOR SELECTION                            */
/* Set to 1 to use UART multi-sensor module     */
/* instead of I2C SEN54/SEN66/SEN68             */
/* ============================================ */
// #define SENSOR_USE_UART             1

/* ============================================ */
/* I2C CONFIGURATION                           */
/* ============================================ */
#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          100000

/* ============================================ */
/* THREE-COLOR LEVEL SYSTEM THRESHOLDS          */
/* Level 0: BLUE (Normal)                       */
/* Level 1: ORANGE (Warning)                    */
/* Level 2: RED (Danger)                        */
/* ============================================ */

/* Temperature thresholds (°C) - Comfort standard */
#define TEMP_NORMAL_MIN    18.0f
#define TEMP_NORMAL_MAX    26.0f
#define TEMP_DANGER_MIN    10.0f
#define TEMP_DANGER_MAX    35.0f

/* Humidity thresholds (%) - Health range */
#define HUMID_NORMAL_MIN   40.0f
#define HUMID_NORMAL_MAX   70.0f
#define HUMID_DANGER_MIN   20.0f
#define HUMID_DANGER_MAX   90.0f

/* NOx thresholds (index) - EU environmental standard */
#define NOX_NORMAL_MAX     100.0f
#define NOX_WARNING_MAX    200.0f

/* PM1 thresholds (µg/m³) - WHO air quality guideline */
#define PM1_NORMAL_MAX      25.0f
#define PM1_WARNING_MAX     50.0f

/* PM2.5 thresholds (µg/m³) - WHO air quality guideline */
#define PM25_NORMAL_MAX    35.0f
#define PM25_WARNING_MAX   75.0f

/* CO2 thresholds (ppm) - ASHRAE ventilation standard */
#define CO2_NORMAL_MAX     800.0f
#define CO2_WARNING_MAX    1200.0f

/* VOC thresholds (index) - German indoor air standard */
#define VOC_NORMAL_MAX     150.0f
#define VOC_WARNING_MAX    300.0f

/* HCHO thresholds (ppb) - WHO indoor air quality guideline (SEN68 only) */
#define HCHO_NORMAL_MAX    80.0f
#define HCHO_WARNING_MAX   150.0f

/* PM10 thresholds (ug/m3) - WHO air quality guideline */
#define PM10_NORMAL_MAX     50.0f
#define PM10_WARNING_MAX   100.0f

/* Pressure thresholds (hPa) - Standard atmospheric */
#define PRES_NORMAL_MIN    980.0f
#define PRES_NORMAL_MAX   1030.0f

/* AQ State thresholds (0-4) */
#define AQ_NORMAL_MAX       2.0f
#define AQ_WARNING_MAX      3.0f

/* ============================================ */
/* LED ALERT THRESHOLDS                        */
/* ============================================ */
#define PM25_ALERT_THRESHOLD        75
#define CO2_ALERT_THRESHOLD         1000

#endif /* APP_CONFIG_H */