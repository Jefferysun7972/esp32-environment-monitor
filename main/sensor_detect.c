/*
 * Sensor Detection Implementation
 * 
 * Runtime automatic detection of SEN54 / SEN66 / SEN68 using Get Product Name command
 * Provides unified API that routes to correct sensor based on detection result
 */

#include "sensor_config.h"
#include <string.h>
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "i2c_manager.h"
#include "am2020dy.h"

static const char* TAG = "sensor_detect";

/* ============================================ */
/* GLOBAL VARIABLES (defined in header)         */
/* ============================================ */
int g_sensor_type = SENSOR_SEN66;       /* Default: SEN66 */
const char* g_sensor_name = "SEN66";    /* Default: SEN66 */
int g_has_pm1_support = 0;              /* Default: no PM1.0 */
int g_has_nox_support = 1;              /* Default: has NOx (SEN66/SEN68) */
int g_has_hcho_support = 0;             /* Default: no HCHO */
int g_has_co2_support = 1;              /* Default: has CO2 (SEN66) */
int g_has_pm10_support = 0;             /* Default: no PM10 */
int g_has_pressure_support = 0;         /* Default: no pressure */
int g_has_aq_support = 0;               /* Default: no AQ state */
int g_has_am2020dy = 0;                 /* Default: no AM2020DY */
int g_display_mode = DISPLAY_MODE_SINGLE; /* Default: single sensor */

/* ============================================ */
/* SENSOR DETECTION FUNCTION                    */
/* Call once during system initialization       */
/* ============================================ */
int detect_am2020dy(i2c_master_dev_handle_t *out_handle) {
    ESP_LOGI(TAG, "🔍 Detecting AM2020DY via command probe (addr: 0x%02X)...", AM2020DY_SLAVE_ADDR);

    esp_err_t err = am2020dy_init(out_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ AM2020DY init failed: %s", esp_err_to_name(err));
        return -1;
    }

    err = am2020dy_read_product_name(*out_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ AM2020DY not responding");
        i2c_master_bus_rm_device(*out_handle);
        *out_handle = NULL;
        /* Reset bus to recover from failed transaction state */
        i2c_master_bus_reset(i2c_manager_get_bus_handle());
        return -1;
    }

    g_sensor_type = SENSOR_AM2020DY;
    g_sensor_name = "AM2020DY";
    g_has_pm1_support = 1;
    g_has_pm10_support = 1;
    g_has_nox_support = 0;
    g_has_hcho_support = 1;
    g_has_co2_support = 0;
    g_has_am2020dy = 1;
    g_display_mode = DISPLAY_MODE_SINGLE;

    ESP_LOGI(TAG, "✅ AM2020DY detected successfully!");
    return 0;
}

int detect_all_sensors(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Auto-detecting sensors at boot...");

    /* 1. Detect SEN66 (always at 0x6B) */
    /*    AM2020DY detection is done by caller via am2020dy_init() before this function
     *    to avoid i2c_master_probe() disrupting the SEN66 device handle state */
    /* 2. Detect SEN66 (always at 0x6B) */
    ESP_LOGI(TAG, "Probing I2C sensor (SEN66)...");
    int i2c_ok = detect_sensor_type();

    /* 3. Determine display mode */
    if (g_has_am2020dy && i2c_ok == 0) {
        g_display_mode = DISPLAY_MODE_DUAL;
        g_sensor_type = SENSOR_DUAL_I2C;
        g_sensor_name = "AM2020DY vs SEN66";
        g_has_pm1_support = 1;
        g_has_pm10_support = 1;
        g_has_nox_support = 1;
        g_has_co2_support = 1;
        g_has_hcho_support = 1;
        ESP_LOGI(TAG, "  Dual I2C sensor mode: AM2020DY + SEN66");
    } else if (i2c_ok == 0) {
        g_display_mode = DISPLAY_MODE_SINGLE;
        ESP_LOGI(TAG, "  Single I2C sensor mode: %s", g_sensor_name);
    } else {
        ESP_LOGE(TAG, "  No I2C sensor detected!");
        return -1;
    }

    ESP_LOGI(TAG, "========================================");
    return 0;
}

int detect_uart_sensor(void) {
    ESP_LOGI(TAG, "Sensor configured as: **UART Multi-Sensor Module**");

    g_sensor_type = SENSOR_UART;
    g_sensor_name = "UART-MOD";
    g_has_pm1_support = 1;
    g_has_nox_support = 0;
    g_has_hcho_support = 0;
    g_has_co2_support = 1;
    g_has_pm10_support = 1;
    g_has_pressure_support = 1;
    g_has_aq_support = 1;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Sensor Configuration:");
    ESP_LOGI(TAG, "  Type:     %s", g_sensor_name);
    ESP_LOGI(TAG, "  PM1.0:    %s", g_has_pm1_support ? "YES" : "NO");
    ESP_LOGI(TAG, "  PM10:     %s", g_has_pm10_support ? "YES" : "NO");
    ESP_LOGI(TAG, "  NOx:      %s", g_has_nox_support ? "YES" : "NO");
    ESP_LOGI(TAG, "  HCHO:     %s", g_has_hcho_support ? "YES" : "NO");
    ESP_LOGI(TAG, "  CO2:      %s", g_has_co2_support ? "YES" : "NO");
    ESP_LOGI(TAG, "  Pressure: %s", g_has_pressure_support ? "YES" : "NO");
    ESP_LOGI(TAG, "  AQ State: %s", g_has_aq_support ? "YES" : "NO");
    ESP_LOGI(TAG, "========================================");

    return 0;
}

int  detect_sensor_type(void) {
    int8_t product_name[32] = {0};
    int16_t error = 0;
    
    ESP_LOGI(TAG, "🔍 Detecting sensor type via Get Product Name...");
    
    /* Try SEN5x unified product name first (all sensors support this command) */
    error = sen5x_get_product_name((unsigned char*)product_name, sizeof(product_name));
    
    if (error == 0 && strlen((char*)product_name) > 0) {
        ESP_LOGI(TAG, "✅ Product Name detected: %s", (char*)product_name);
        
        /* Check sensor type based on product name string */
        if (strstr((char*)product_name, "SEN68") != NULL) {
            ESP_LOGI(TAG, "🎯 Sensor identified as: **SEN68** (Advanced)");
            
            g_sensor_type = SENSOR_SEN68;
            g_sensor_name = "SEN68";
            g_has_pm1_support = 1;
            g_has_nox_support = 1;
            g_has_hcho_support = 1;
            g_has_co2_support = 0;
            
        } else if (strstr((char*)product_name, "SEN66") != NULL) {
            ESP_LOGI(TAG, "🎯 Sensor identified as: **SEN66** (Standard)");
            
            g_sensor_type = SENSOR_SEN66;
            g_sensor_name = "SEN66";
            g_has_pm1_support = 0;
            g_has_nox_support = 1;
            g_has_hcho_support = 0;
            g_has_co2_support = 1;
            
        } else if (strstr((char*)product_name, "SEN54") != NULL) {
            ESP_LOGI(TAG, "🎯 Sensor identified as: **SEN54** (Basic)");
            
            g_sensor_type = SENSOR_SEN54;
            g_sensor_name = "SEN54";
            g_has_pm1_support = 1;
            g_has_nox_support = 0;
            g_has_hcho_support = 0;
            g_has_co2_support = 0;
            
        } else {
            ESP_LOGW(TAG, "⚠️ Unknown sensor type: %s", (char*)product_name);
            ESP_LOGW(TAG, "   Defaulting to SEN66 mode");
            
            g_sensor_type = SENSOR_SEN66;
            g_sensor_name = "SEN66";
            g_has_pm1_support = 0;
            g_has_nox_support = 1;
            g_has_hcho_support = 0;
            g_has_co2_support = 1;
        }
        
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "Sensor Configuration:");
        ESP_LOGI(TAG, "  Type:     %s", g_sensor_name);
        ESP_LOGI(TAG, "  PM1.0:    %s", g_has_pm1_support ? "YES ✅" : "NO ❌");
        ESP_LOGI(TAG, "  NOx:      %s", g_has_nox_support ? "YES ✅" : "NO ❌");
        ESP_LOGI(TAG, "  HCHO:     %s", g_has_hcho_support ? "YES ✅" : "NO ❌");
        ESP_LOGI(TAG, "  CO2:      %s", g_has_co2_support ? "YES ✅" : "NO ❌");
        ESP_LOGI(TAG, "========================================");
        
        return 0;
        
    } else {
        ESP_LOGE(TAG, "❌ Failed to read Product Name (error: %d)", error);
        ESP_LOGE(TAG, "   Defaulting to SEN66 mode");
        
        return -1;
    }
}

/* ============================================ */
/* UNIFIED API IMPLEMENTATIONS                  */
/* Route to correct sensor based on detection   */
/* ============================================ */

int16_t sensor_device_reset(void) {
    if (g_sensor_type == SENSOR_SEN54) {
        return sen5x_device_reset();
    } else if (g_sensor_type == SENSOR_SEN68) {
        return sen68_device_reset();
    } else {
        return sen66_device_reset();
    }
}

int16_t sensor_start_measurement(void) {
    if (g_sensor_type == SENSOR_SEN54) {
        return sen5x_start_measurement();
    } else if (g_sensor_type == SENSOR_SEN68) {
        return sen68_start_continuous_measurement();
    } else {
        return sen66_start_continuous_measurement();
    }
}

int16_t sensor_stop_measurement(void) {
    if (g_sensor_type == SENSOR_SEN54) {
        return sen5x_stop_measurement();
    } else if (g_sensor_type == SENSOR_SEN68) {
        return sen68_stop_measurement();
    } else {
        return sen66_stop_measurement();
    }
}

int16_t sensor_get_product_name(int8_t* name, uint16_t size) {
    if (g_sensor_type == SENSOR_SEN54) {
        return sen5x_get_product_name((unsigned char*)name, size);
    } else if (g_sensor_type == SENSOR_SEN68) {
        return sen68_get_product_name(name, size);
    } else {
        return sen66_get_product_name(name, size);
    }
}

int16_t sensor_read_measured_values(uint16_t* pm1p0, uint16_t* pm2p5,
                                     uint16_t* pm4p0, uint16_t* pm10p0,
                                     int16_t* humidity, int16_t* temperature,
                                     int16_t* voc_index, int16_t* nox_index,
                                     uint16_t* hcho_or_co2) {
    if (g_sensor_type == SENSOR_SEN54) {
        /* SEN54: 7 params, no NOx, no CO2, no HCHO */
        int16_t ret = sen5x_read_measured_values(
            pm1p0, pm2p5, pm4p0, pm10p0,
            humidity, temperature, voc_index, nox_index);
        if (hcho_or_co2) *hcho_or_co2 = 0;
        return ret;
    } else if (g_sensor_type == SENSOR_SEN68) {
        return sen68_read_measured_values_as_integers(
            pm1p0, pm2p5, pm4p0, pm10p0,
            humidity, temperature, voc_index, nox_index,
            hcho_or_co2);
    } else {
        return sen66_read_measured_values_as_integers(
            pm1p0, pm2p5, pm4p0, pm10p0,
            humidity, temperature, voc_index, nox_index,
            hcho_or_co2);
    }
}