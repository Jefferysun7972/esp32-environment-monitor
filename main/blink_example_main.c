/*
 * ESP32 Environment Monitor with Dual Sensor Support
 * 
 * Features:
 * - LED blinking on GPIO2 (D2) with configurable period
 * - Dual sensor support: SEN66 (default) / SEN68 (with PM1 + HCHO)
 *   Controlled by CONFIG_SENSOR_TYPE in menuconfig
 * - Three-color level system: BLUE (normal) / ORANGE (warning) / RED (danger)
 * - 240x320 TFT-LCD display with optimized layout and progress bars
 * 
 * Sensor Selection:
 *   - SEN66: Temperature, Humidity, PM2.5/4/10, VOC, NOx, CO2
 *   - SEN68: All SEN66 features + PM1.0 ultrafine particles + HCHO formaldehyde
 * 
 * Hardware Connections:
 * - LED: GPIO2 (D2)
 * - Sensor I2C: SDA=GPIO21, SCL=GPIO22, VCC=3.3V, GND=GND
 * 
 * Build Configuration:
 *   idf.py menuconfig → Example Configuration → Sensor Type
 *   Choose: SENSOR_TYPE_SEN66 (default) or SENSOR_TYPE_SEN68
 */

/* ============================================ */
/* DISPLAY CONFIGURATION - MODIFY HERE         */
/* ============================================ */
#define USE_TFT_LCD      1   /* 1=Enable TFT-LCD (ILI9341), 0=Disable */
#define USE_OLED_DISPLAY 0   /* 1=Enable OLED (SSD1306), 0=Disable */
/* ============================================ */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "i2c_manager.h"
#include "led_strip.h"
#include "sdkconfig.h"

/* ============================================ */
/* UNIFIED SENSOR CONFIGURATION                */
/* Auto-selects SEN66 or SEN68 based on menuconfig*/
/* ============================================ */
#include "sensor_config.h"  /* Must be included before sensor headers! */

/* Sensirion common headers (shared by both sensors) */
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

#if USE_TFT_LCD
#include "tft_lcd.h"
#endif

#if USE_OLED_DISPLAY
#include "oled_display.h"
#endif

static const char *TAG = "env_monitor";

/* LED Configuration */
#define BLINK_GPIO CONFIG_BLINK_GPIO

/* I2C Configuration (shared by both sensors) */
#define I2C_MASTER_SCL_IO           22        /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           21        /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0          /*!< I2C master i2c port number */
#define I2C_MASTER_FREQ_HZ          100000     /*!< I2C master clock frequency */

/* Sensor Read Interval (ms) - applies to both SEN66 and SEN68 */
#define SENSOR_READ_PERIOD_MS        5000       /*!< Read sensor every 5 seconds */

/* Sensor warm-up time before first valid reading (ms) */
#define SENSOR_WARMUP_DELAY_MS       15000      /*!< Wait 15 seconds for sensor stabilization */

/* Number of initial readings to skip during warm-up */
#define SENSOR_WARMUP_READINGS       3          /*!< Skip first 3 readings as they may be unreliable */

/* ============================================ */
/* THREE-COLOR LEVEL SYSTEM THRESHOLDS          */
/* Level 0: BLUE (Normal)                       */
/* Level 1: ORANGE (Warning)                    */
/* Level 2: RED (Danger)                        */
/* ============================================ */

/* Temperature thresholds (°C) - Comfort standard */
#define TEMP_NORMAL_MIN    18.0f   /* Blue range lower bound */
#define TEMP_NORMAL_MAX    26.0f   /* Blue range upper bound */
#define TEMP_DANGER_MIN    10.0f   /* Red danger: too cold */
#define TEMP_DANGER_MAX    35.0f   /* Red danger: too hot */

/* Humidity thresholds (%) - Health range */
#define HUMID_NORMAL_MIN   40.0f   /* Blue range lower bound */
#define HUMID_NORMAL_MAX   70.0f   /* Blue range upper bound */
#define HUMID_DANGER_MIN   20.0f   /* Red danger: too dry */
#define HUMID_DANGER_MAX   90.0f   /* Red danger: too humid */

/* NOx thresholds (index) - EU environmental standard */
#define NOX_NORMAL_MAX     100.0f  /* Blue: excellent */
#define NOX_WARNING_MAX    200.0f  /* Orange warning threshold */

/* PM1 thresholds (µg/m³) - WHO air quality guideline */
#if HAS_PM1_SUPPORT
#define PM1_NORMAL_MAX      25.0f   /* Blue: good air quality */
#define PM1_WARNING_MAX     50.0f   /* Orange: moderate */
#endif

/* PM2.5 thresholds (µg/m³) - WHO air quality guideline */
#define PM25_NORMAL_MAX    35.0f   /* Blue: good air quality */
#define PM25_WARNING_MAX   75.0f   /* Orange: unhealthy for sensitive groups */

/* CO2 thresholds (ppm) - ASHRAE ventilation standard */
#define CO2_NORMAL_MAX     800.0f  /* Blue: good ventilation */
#define CO2_WARNING_MAX    1200.0f /* Orange: poor ventilation */

/* VOC thresholds (index) - German indoor air standard */
#define VOC_NORMAL_MAX     150.0f  /* Blue: good indoor air */
#define VOC_WARNING_MAX    300.0f  /* Orange: elevated VOC levels */

/* LED blinking intervals (ms) */
#define BLINK_INTERVAL_NORMAL       CONFIG_BLINK_PERIOD  /*!< Normal: 2000ms (from sdkconfig) */
#define BLINK_INTERVAL_ALERT        200                 /*!< Alert: 200ms (fast flash) */

// Alert thresholds for LED behavior
#define PM25_ALERT_THRESHOLD        75         /* PM2.5: >75 µg/m³ */
#define CO2_ALERT_THRESHOLD         1000       /* CO2: >1000 ppm */

/* HCHO thresholds (ppb) - WHO indoor air quality guideline (SEN68 only) */
#if HAS_HCHO_SUPPORT
#define HCHO_NORMAL_MAX    80.0f   /* Blue: <0.08ppm */
#define HCHO_WARNING_MAX   150.0f  /* Orange: 0.08-0.15ppm */
#endif

static uint8_t s_led_state = 0;
static bool s_alert_active = false;              /* Track if alert mode is active */
static int s_alert_blink_counter = 0;            /* Counter for alert flashing pattern */

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static void blink_led(void)
{
    if (s_led_state) {
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        led_strip_refresh(led_strip);
    } else {
        led_strip_clear(led_strip);
    }
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1,
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

/**
 * @brief Initialize environmental sensor (SEN66 or SEN68 based on config)
 * 
 * Uses unified macros from sensor_config.h that automatically call
 * the correct API functions based on CONFIG_SENSOR_TYPE selection.
 */
static void init_environmental_sensor(void)
{
    int16_t error = NO_ERROR;

    ESP_LOGI(TAG, "Initializing %s environmental sensor...", SENSOR_NAME);

    /* Reset device using unified macro */
    error = sensor_device_reset();
    if (error != NO_ERROR) {
        ESP_LOGE(TAG, "❌ %s device reset failed: %d", SENSOR_NAME, error);
        return;
    }

    sensirion_i2c_hal_sleep_usec(1200000);

    /* Try to get serial number (if supported by sensor) */
#if USE_SEN66_SENSOR
    int8_t serial_number[32] = {0};
    error = sen66_get_serial_number(serial_number, 32);
    if (error == NO_ERROR) {
        ESP_LOGI(TAG, "📋 %s Serial Number: %s", SENSOR_NAME, serial_number);
    } else {
        ESP_LOGW(TAG, "⚠️  Could not read %s serial number (error: %d)", SENSOR_NAME, error);
    }
#endif

    /* Start continuous measurement using unified macro */
    error = sensor_start_measurement();
    if (error != NO_ERROR) {
        ESP_LOGE(TAG, "❌ Failed to start %s continuous measurement: %d", SENSOR_NAME, error);
        return;
    }

    ESP_LOGI(TAG, "🎉 %s sensor initialized successfully! Ready to read data.", SENSOR_NAME);
}

/**
 * @brief Get color level based on parameter value and thresholds
 * 
 * @param value Current measurement value
 * @param normal_min Normal range lower bound (blue zone)
 * @param normal_max Normal range upper bound (blue zone)
 * @param warning_max Warning/danger upper limit (>this is red)
 * @param is_inverted If true, values BELOW normal_min are dangerous (like temp/humidity)
 * @return Color: TFT_BLUE (0), TFT_ORANGE (1), or TFT_RED (2)
 */
static uint16_t get_level_color(float value, float normal_min, float normal_max, 
                                float warning_max, bool is_inverted)
{
    if (!is_inverted) {
        /* Normal pattern: low values are good (NOx, PM2.5, CO2, VOC) */
        if (value <= normal_max) {
            return TFT_BLUE;      /* Normal range → Blue */
        } else if (value <= warning_max) {
            return TFT_ORANGE;    /* Warning range → Orange */
        } else {
            return TFT_RED;       /* Danger range → Red */
        }
    } else {
        /* Inverted pattern: middle range is good (Temp, Humidity) */
        if (value >= normal_min && value <= normal_max) {
            return TFT_BLUE;      /* Comfortable range → Blue */
        } else if ((value >= normal_min && value <= warning_max) || 
                   (value <= normal_max && value >= normal_min)) {
            return TFT_ORANGE;    /* Warning range → Orange */
        } else {
            return TFT_RED;       /* Danger range → Red */
        }
    }
}

/**
 * @brief Calculate global worst-case alert level from all parameters
 * 
 * Returns the highest severity level across all sensors:
 * 0 = All parameters in BLUE (normal)
 * 1 = At least one parameter in ORANGE (warning)
 * 2 = At least one parameter in RED (danger)
 * 
 * @return Global alert level (0, 1, or 2)
 */
static int get_global_alert_level(
    float temp_celsius, float humidity_pct, float nox_idx,
    float pm2_5_ugm3, uint16_t co2_ppm, float voc_idx)
{
    int max_level = 0;  /* Start with best case: BLUE (0) */
    
    /* Check each parameter and track maximum severity */
    
    /* Temperature check (inverted: both extremes are bad) */
    if (temp_celsius < TEMP_DANGER_MIN || temp_celsius > TEMP_DANGER_MAX) {
        max_level = 2;  /* RED: Extreme temperature danger */
    } else if (temp_celsius < TEMP_NORMAL_MIN || temp_celsius > TEMP_NORMAL_MAX) {
        if (max_level < 1) max_level = 1;  /* ORANGE: Outside comfort zone */
    }
    
    /* Humidity check (inverted: both extremes are bad) */
    if (humidity_pct < HUMID_DANGER_MIN || humidity_pct > HUMID_DANGER_MAX) {
        max_level = 2;  /* RED: Extreme humidity danger */
    } else if (humidity_pct < HUMID_NORMAL_MIN || humidity_pct > HUMID_NORMAL_MAX) {
        if (max_level < 1) max_level = 1;  /* ORANGE: Outside healthy range */
    }
    
    /* NOx check (normal: higher is worse) */
    if (nox_idx > NOX_WARNING_MAX) {
        max_level = 2;  /* RED: High NOx danger */
    } else if (nox_idx > NOX_NORMAL_MAX && max_level < 1) {
        max_level = 1;  /* ORANGE: Elevated NOx */
    }
    
    /* PM2.5 check (normal: higher is worse) */
    if (pm2_5_ugm3 > PM25_WARNING_MAX) {
        max_level = 2;  /* RED: High PM2.5 danger */
    } else if (pm2_5_ugm3 > PM25_NORMAL_MAX && max_level < 1) {
        max_level = 1;  /* ORANGE: Elevated PM2.5 */
    }
    
    /* CO2 check (normal: higher is worse) */
    if (co2_ppm > CO2_WARNING_MAX) {
        max_level = 2;  /* RED: High CO2 danger */
    } else if (co2_ppm > CO2_NORMAL_MAX && max_level < 1) {
        max_level = 1;  /* ORANGE: High CO2 */
    }
    
    /* VOC check (normal: higher is worse) */
    if (voc_idx > VOC_WARNING_MAX) {
        max_level = 2;  /* RED: High VOC danger */
    } else if (voc_idx > VOC_NORMAL_MAX && max_level < 1) {
        max_level = 1;  /* ORANGE: Elevated VOC */
    }
    
    return max_level;
}

/**
 * @brief Check sensor values against thresholds and update alert status
 * 
 * @param pm25_ugm3 PM2.5 value in µg/m³
 * @param co2_ppm CO₂ value in ppm
 * @return true if alert condition detected, false otherwise
 */
static bool check_environmental_alerts(float pm25_ugm3, uint16_t co2_ppm)
{
    bool pm25_alert = (pm25_ugm3 > PM25_ALERT_THRESHOLD);
    bool co2_alert = (co2_ppm > CO2_ALERT_THRESHOLD);
    
    bool new_alert_status = (pm25_alert || co2_alert);
    
    if (new_alert_status != s_alert_active) {
        if (new_alert_status) {
            ESP_LOGW(TAG, "🚨🚨🚨 ENVIRONMENTAL ALERT ACTIVATED! 🚨🚨🚨");
            if (pm25_alert) {
                ESP_LOGW(TAG, "   ⚠️  PM2.5: %.1f µg/m³ (threshold: %d)", pm25_ugm3, PM25_ALERT_THRESHOLD);
            }
            if (co2_alert) {
                ESP_LOGW(TAG, "   ⚠️  CO₂: %uppm (threshold: %d)", co2_ppm, CO2_ALERT_THRESHOLD);
            }
            ESP_LOGW(TAG, "   🔴 LED now flashing rapidly to indicate poor air quality!");
        } else {
            ESP_LOGI(TAG, "✅ Environmental conditions returned to normal levels.");
            ESP_LOGI(TAG, "   💚 LED returned to normal slow blinking.");
        }
        
        s_alert_active = new_alert_status;
    } else if (new_alert_status && (s_alert_blink_counter % 10 == 0)) {
        ESP_LOGW(TAG, "🔴 ALERT ACTIVE - PM2.5: %.1f | CO₂: %uppm", pm25_ugm3, co2_ppm);
    }
    
    return new_alert_status;
}

/**
 * @brief Read and display sensor data with three-color level system
 * 
 * Supports both SEN66 and SEN68 through unified API from sensor_config.h
 * Automatically handles PM1/HCHO data when using SEN68 sensor
 * 
 * @return Returns true if alert conditions are met, false otherwise
 */
static bool read_and_display_sensor_data(void)
{
    /* Declare variables for all possible sensor outputs */
    uint16_t pm1p0 = 0, pm2p5 = 0, pm4p0 = 0, pm10p0 = 0;
    int16_t humidity = 0, temperature = 0, voc_index = 0, nox_index = 0;
    uint16_t co2 = 0;
#if HAS_HCHO_SUPPORT
    uint16_t hcho_ppb = 0;  /* Only used with SEN68 */
#endif

    /* Use unified macro to read values (auto-selects correct function) */
    int16_t error = sensor_read_measured_values(
#if HAS_PM1_SUPPORT && HAS_HCHO_SUPPORT
        /* SEN68: Includes PM1 and HCHO parameters */
        &pm1p0, &pm2p5, &pm4p0, &pm10p0,
        &humidity, &temperature,
        &voc_index, &nox_index,
        &hcho_ppb
#else
        /* SEN66: Standard parameters without PM1 and HCHO */
        &pm1p0, &pm2p5, &pm4p0, &pm10p0,
        &humidity, &temperature,
        &voc_index, &nox_index, &co2
#endif
    );

    if (error != NO_ERROR) {
        ESP_LOGE(TAG, "❌ Error reading %s values: %d", SENSOR_NAME, error);
        return false;
    }

    /* Convert raw values to physical units according to datasheet */
    float temp_celsius = (float)temperature / 200.0f;
    float humidity_pct = (float)humidity / 100.0f;
    float voc_idx_f = (float)voc_index / 10.0f;
    float nox_idx_f = (float)nox_index / 10.0f;
    float pm2_5_ugm3 = (float)pm2p5 / 10.0f;
    
#if HAS_PM1_SUPPORT
    float pm1_ugm3 = (float)pm1p0 / 10.0f;  /* PM1 only available on SEN68 */
#endif

    /* Check for alert conditions and determine status indicator */
    bool is_alert = check_environmental_alerts(pm2_5_ugm3, co2);
    
    const char* status_icon = is_alert ? "🔴" : "💚";
    const char* status_text = is_alert ? "⚠️  ALERT" : "✅ NORMAL";

    /* Display formatted sensor data with status indicator */
    /* Display formatted sensor data - Single line output for clarity */
    #if HAS_PM1_SUPPORT && HAS_HCHO_SUPPORT
    /* SEN68 mode: Include PM1 and HCHO in single line */
    ESP_LOGI(TAG,
             "[%s] %s %s | 🌡️ %.1f°C | 💧 %.1f%% | 🌫️ PM2.5: %.1fµg/m³ | ✨ PM1: %.1fµg/m³ | "
             "🌬️ VOC: %.1f | ⚠️ NOx: %.1f | 🧪 HCHO: %u ppb",
             SENSOR_NAME, status_icon, status_text,
             temp_celsius, humidity_pct, pm2_5_ugm3, pm1_ugm3,
             voc_idx_f, nox_idx_f, hcho_ppb);
    #elif HAS_HCHO_SUPPORT && !HAS_PM1_SUPPORT
    /* SEN66 with HCHO only (unlikely but supported) */
    ESP_LOGI(TAG,
             "[%s] %s %s | 🌡️ %.1f°C | 💧 %.1f%% | 🌫️ PM2.5: %.1fµg/m³ | "
             "🌬️ VOC: %.1f | ⚠️ NOx: %.1f | 🧪 HCHO: %u ppb",
             SENSOR_NAME, status_icon, status_text,
             temp_celsius, humidity_pct, pm2_5_ugm3,
             voc_idx_f, nox_idx_f, hcho_ppb);
    #else
    /* Standard SEN66 mode: CO2 instead of HCHO, no PM1 */
    ESP_LOGI(TAG,
             "[%s] %s %s | 🌡️ %.1f°C | 💧 %.1f%% | 🌫️ PM2.5: %.1fµg/m³ | "
             "🌬️ VOC: %.1f | ⚠️ NOx: %.1f | 🔴 CO₂: %uppm",
             SENSOR_NAME, status_icon, status_text,
             temp_celsius, humidity_pct, pm2_5_ugm3,
             voc_idx_f, nox_idx_f, co2);
    #endif
    /* Update display(s) with latest readings */

    #if USE_TFT_LCD
    {
        /* Smart Refresh v7.0: Three-color level system with dynamic coloring */
        static bool first_draw = true;
        static int last_alert_state = -1;
        
        char buf[64];
        
        /* Layout constants - Optimized for 240x320 screen */
        const int header_height = 40;
        const int status_bar_height = 24;
        const int left_margin = 10;
        const int screen_w = tft_get_width();
        const int char_w = 12;
        const int char_h = 16;
        const int right_margin = 4;
        
        /* Progress bar configuration - Increased for better visibility */
        const int pb_x = left_margin;
        const int pb_w = screen_w - (left_margin * 2);
        const int pb_h = 14;       /* Increased from 10 to 12 */
        const int pb_gap = 6;      /* Increased from 6 to 8 */
        
        /* Sensor max values for progress bar scaling */
        const float pm25_max = 500.0f;
        
        /* Y positions - Adaptive layout based on sensor type */
        #if HAS_PM1_SUPPORT
            /* ===== SEN68 MODE: Ultra-compact layout for PM1.0 progress bar ===== */
            const int basic_row_gap = 24;             /* Compressed gap (was 30) to save space */
            const int sep_gap = 20;                   /* Reduced separator gap (was 24) */
            const int air_quality_gap = 6;            /* ← 新增！紧凑模式间距 */
        #else
            /* ===== SEN66 MODE: Original spacious layout ===== */
            const int basic_row_gap = 30;             /* Standard comfortable spacing */
            const int sep_gap = 24;                   /* Normal separator position */
            const int air_quality_gap = 10;           /* ← 新增！标准模式间距 */
        #endif
        const int y_temp = header_height + 6;          /* Zone 1: Basic parameters */
        const int y_humid = y_temp + basic_row_gap;   /* Row 2: Humidity */
        const int y_nox = y_humid + basic_row_gap;    /* Row 3: NOx */
        
        /* ===== SEPARATOR: Basic vs Air Quality Parameters ===== */
        const int y_sep1 = y_nox + sep_gap;           /* Separator after basic params */
        
        #if HAS_PM1_SUPPORT
            const int y_pm1 = y_sep1 + 6;             /* Zone 2: PM1.0 data row (SEN68 only) */
            const int y_pb_pm1 = y_pm1 + char_h + pb_gap;  /* PM1.0 progress bar Y coordinate */
            const int y_pm25 = y_pb_pm1 + pb_h + 8;   /* Zone 2: PM2.5 (after PM1.0 progress bar) */
        #else
            const int y_pm25 = y_sep1 + 8;            /* Zone 2: PM2.5 (SEN66, no PM1.0) */
        #endif
        
        const int y_pb_pm25 = y_pm25 + char_h + pb_gap;
        
        #if HAS_HCHO_SUPPORT
            const int y_hcho = y_pb_pm25 + pb_h + air_quality_gap; /* HCHO row (SEN68 only) */
            const int y_pb_hcho = y_hcho + char_h + pb_gap;
            const int y_voc = y_pb_hcho + pb_h + air_quality_gap;  /* VOC row */
            
            const float hcho_max = 300.0f;  /* Max: 300 ppb */
        #else
            const int y_co2 = y_pb_pm25 + pb_h + air_quality_gap; /* CO2 row (SEN66 only) */
            const int y_pb_co2 = y_co2 + char_h + pb_gap;
            const int y_voc = y_pb_co2 + pb_h + air_quality_gap;  /* VOC row */
            
            const float co2_max = 5000.0f;
        #endif
        
        const int y_pb_voc = y_voc + char_h + pb_gap;
        const int status_y = tft_get_height() - status_bar_height;

        /* ===== CALCULATE DYNAMIC COLORS FOR EACH PARAMETER ===== */
        
        /* Get individual color levels for each parameter */
        uint16_t color_temp  = get_level_color(temp_celsius, TEMP_NORMAL_MIN, TEMP_NORMAL_MAX, 
                                               TEMP_DANGER_MAX, true);  /* Inverted: comfort range */
        uint16_t color_humid = get_level_color(humidity_pct, HUMID_NORMAL_MIN, HUMID_NORMAL_MAX, 
                                               HUMID_DANGER_MAX, true); /* Inverted: health range */
        uint16_t color_nox   = get_level_color(nox_idx_f, 0, NOX_NORMAL_MAX, 
                                               NOX_WARNING_MAX, false); /* Normal: high is bad */
        uint16_t color_pm25  = get_level_color(pm2_5_ugm3, 0, PM25_NORMAL_MAX, 
                                               PM25_WARNING_MAX, false); /* Normal: high is bad */
                                               
        #if HAS_HCHO_SUPPORT
        uint16_t color_hcho  = get_level_color((float)hcho_ppb, 0, HCHO_NORMAL_MAX,
                                               HCHO_WARNING_MAX, false); /* Normal: high is bad */
        #else
        uint16_t color_co2   = get_level_color((float)co2, 0, CO2_NORMAL_MAX, 
                                               CO2_WARNING_MAX, false); /* Normal: high is bad */
        #endif
        
        uint16_t color_voc   = get_level_color(voc_idx_f, 0, VOC_NORMAL_MAX, 
                                               VOC_WARNING_MAX, false); /* Normal: high is bad */
                                               
        #if HAS_PM1_SUPPORT
        uint16_t color_pm1   = get_level_color(pm1_ugm3, 0, PM1_NORMAL_MAX, 
                                               PM1_WARNING_MAX, false); /* Normal: high is bad */
        #endif
        
        /* Calculate global alert level for status bar */
        int global_level = get_global_alert_level(
            temp_celsius, humidity_pct, nox_idx_f, 
            pm2_5_ugm3, co2, voc_idx_f
        );

        if (first_draw) {
            /* ===== FIRST DRAW: Complete layout initialization ===== */
            tft_fill_rect(0, header_height, screen_w, 
                        tft_get_height() - header_height - status_bar_height, TFT_BG_COLOR);

            /* === Zone 1: Basic parameters (no progress bars) === */
            tft_draw_string("Temp:", left_margin, y_temp, color_temp, TFT_BG_COLOR, 2);
            tft_draw_string("C", screen_w - 1 * char_w - right_margin, y_temp, color_temp, TFT_BG_COLOR, 2);
            
            tft_draw_string("Hum:", left_margin, y_humid, color_humid, TFT_BG_COLOR, 2);
            tft_draw_string("%", screen_w - 1 * char_w - right_margin, y_humid, color_humid, TFT_BG_COLOR, 2);
            
            tft_draw_string("NOx:", left_margin, y_nox, color_nox, TFT_BG_COLOR, 2);
            
#if HAS_PM1_SUPPORT
            /* PM1 label (only shown when using SEN68) */
            tft_draw_string("PM1.0:", left_margin, y_pm1, color_pm1, TFT_BG_COLOR, 2);
            tft_draw_string("ug/m3", screen_w - 5 * char_w - right_margin, y_pm1, color_pm1, TFT_BG_COLOR, 2);
#endif
            
            /* ===== SEPARATOR: Basic vs Air Quality Parameters ===== */
            tft_fill_rect(8, y_sep1, screen_w - 16, 2, TFT_DARKGRAY);
            
            /* === Zone 2: Air quality (with progress bars) === */
            tft_draw_string("PM2.5:", left_margin, y_pm25, color_pm25, TFT_BG_COLOR, 2);
            tft_draw_string("ug/m3", screen_w - 5 * char_w - right_margin, y_pm25, color_pm25, TFT_BG_COLOR, 2);
            
            #if HAS_HCHO_SUPPORT
            /* HCHO display (SEN68 only - replaces CO2) */
            tft_draw_string("HCHO:", left_margin, y_hcho, color_hcho, TFT_BG_COLOR, 2);
            tft_draw_string("ppb", screen_w - 3 * char_w - right_margin, y_hcho, color_hcho, TFT_BG_COLOR, 2);
            #else
            /* CO2 display (SEN66 only) */
            tft_draw_string("CO2:", left_margin, y_co2, color_co2, TFT_BG_COLOR, 2);
            tft_draw_string("ppm", screen_w - 3 * char_w - right_margin, y_co2, color_co2, TFT_BG_COLOR, 2);
            #endif
            
            /* === Zone 3: Gas sensor (with progress bar) === */
            tft_draw_string("VOC:", left_margin, y_voc, color_voc, TFT_BG_COLOR, 2);
            
            /* Draw progress bar backgrounds */
            tft_fill_rect(pb_x, y_pb_pm25, pb_w, pb_h, TFT_DARKGRAY);
            #if HAS_PM1_SUPPORT
            tft_fill_rect(pb_x, y_pb_pm1, pb_w, pb_h, TFT_DARKGRAY);    /* PM1.0 progress bar (SEN68) */
            #endif
            #if HAS_HCHO_SUPPORT
            tft_fill_rect(pb_x, y_pb_hcho, pb_w, pb_h, TFT_DARKGRAY);  /* HCHO progress bar */
            #else
            tft_fill_rect(pb_x, y_pb_co2, pb_w, pb_h, TFT_DARKGRAY);   /* CO2 progress bar */
            #endif
            tft_fill_rect(pb_x, y_pb_voc, pb_w, pb_h, TFT_DARKGRAY);
            
            first_draw = false;
        }

        /* ===== UPDATE MODE: Dynamic three-color refresh ===== */
        
        /* Value X offsets for centered alignment */
        const int val_x_temp  = left_margin + 85;
        const int val_x_humid = left_margin + 85;
        const int val_x_nox   = left_margin + 85;
        const int val_x_pm25  = left_margin + 90;
        
        #if !HAS_HCHO_SUPPORT
        const int val_x_co2   = left_margin + 85;
        #endif
        
        const int val_x_voc   = left_margin + 85;
        
        /* === Update Zone 1 values with dynamic colors === */
        
        /* Temperature label and unit with level-based color */
        tft_draw_string("Temp:", left_margin, y_temp, color_temp, TFT_BG_COLOR, 2);
        tft_fill_rect(val_x_temp, y_temp, 6 * char_w, char_h, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%6.1f", temp_celsius);
        tft_draw_string(buf, val_x_temp, y_temp, color_temp, TFT_BG_COLOR, 2);
        tft_draw_string("C", screen_w - 1 * char_w - right_margin, y_temp, color_temp, TFT_BG_COLOR, 2);

        /* Humidity label and unit with level-based color */
        tft_draw_string("Hum:", left_margin, y_humid, color_humid, TFT_BG_COLOR, 2);
        tft_fill_rect(val_x_humid, y_humid, 6 * char_w, char_h, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%6.1f", humidity_pct);
        tft_draw_string(buf, val_x_humid, y_humid, color_humid, TFT_BG_COLOR, 2);
        tft_draw_string("%", screen_w - 1 * char_w - right_margin, y_humid, color_humid, TFT_BG_COLOR, 2);

        /* NOx label with level-based color */
        tft_draw_string("NOx:", left_margin, y_nox, color_nox, TFT_BG_COLOR, 2);
        tft_fill_rect(val_x_nox, y_nox, 6 * char_w, char_h, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%6.1f", nox_idx_f);
        tft_draw_string(buf, val_x_nox, y_nox, color_nox, TFT_BG_COLOR, 2);

        
        #if HAS_PM1_SUPPORT
        /* PM1 Display (only shown when using SEN68 sensor) */
        tft_draw_string("PM1.0:", left_margin, y_pm1, color_pm1, TFT_BG_COLOR, 2);
        tft_fill_rect(left_margin + 85, y_pm1, 6 * char_w, char_h, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%6.1f", pm1_ugm3);
        tft_draw_string(buf, left_margin + 85, y_pm1, color_pm1, TFT_BG_COLOR, 2);
        tft_draw_string("ug/m3", screen_w - 5 * char_w - right_margin, y_pm1, color_pm1, TFT_BG_COLOR, 2);
        #endif

        /* === Update Zone 2 values with dynamic colors === */
        
        /* PM2.5 label and unit with level-based color */
        tft_draw_string("PM2.5:", left_margin, y_pm25, color_pm25, TFT_BG_COLOR, 2);
        tft_fill_rect(val_x_pm25, y_pm25, 6 * char_w, char_h, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%6.1f", pm2_5_ugm3);
        tft_draw_string(buf, val_x_pm25, y_pm25, color_pm25, TFT_BG_COLOR, 2);
        tft_draw_string("ug/m3", screen_w - 5 * char_w - right_margin, y_pm25, color_pm25, TFT_BG_COLOR, 2);

        #if HAS_HCHO_SUPPORT
        /* HCHO label and unit (SEN68 only) */
        tft_draw_string("HCHO:", left_margin, y_hcho, color_hcho, TFT_BG_COLOR, 2);
        tft_fill_rect(left_margin + 85, y_hcho, 6 * char_w, char_h, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%4u", hcho_ppb);
        tft_draw_string(buf, left_margin + 85, y_hcho, color_hcho, TFT_BG_COLOR, 2);
        tft_draw_string("ppb", screen_w - 3 * char_w - right_margin, y_hcho, color_hcho, TFT_BG_COLOR, 2);
        #else
        /* CO2 label and unit with level-based color (SEN66 only) */
        tft_draw_string("CO2:", left_margin, y_co2, color_co2, TFT_BG_COLOR, 2);
        tft_fill_rect(val_x_co2, y_co2, 6 * char_w, char_h, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%4u", co2);
        tft_draw_string(buf, val_x_co2, y_co2, color_co2, TFT_BG_COLOR, 2);
        tft_draw_string("ppm", screen_w - 3 * char_w - right_margin, y_co2, color_co2, TFT_BG_COLOR, 2);
        #endif
        
        /* === Update Zone 3 value with dynamic color === */
        
        /* VOC label with level-based color */
        tft_draw_string("VOC:", left_margin, y_voc, color_voc, TFT_BG_COLOR, 2);
        tft_fill_rect(val_x_voc, y_voc, 6 * char_w, char_h, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%6.1f", voc_idx_f);
        tft_draw_string(buf, val_x_voc, y_voc, color_voc, TFT_BG_COLOR, 2);

        /* ===== UPDATE PROGRESS BARS WITH THREE-COLOR SYSTEM ===== */
        
        /* PM2.5 Progress Bar - uses same color as value */
        {
            int pm25_pct = (int)((pm2_5_ugm3 / pm25_max) * 100);
            if (pm25_pct > 100) pm25_pct = 100;
            int pm25_fill_w = (pb_w * pm25_pct) / 100;
            
            tft_fill_rect(pb_x, y_pb_pm25, pb_w, pb_h, TFT_DARKGRAY);
            if (pm25_fill_w > 0) {
                tft_fill_rect(pb_x, y_pb_pm25, pm25_fill_w, pb_h, color_pm25);
            }
        }
        
        #if HAS_HCHO_SUPPORT
        /* HCHO Progress Bar - uses same color as value (SEN68 only) */
        {
            int hcho_pct = (int)(((float)hcho_ppb / hcho_max) * 100);
            if (hcho_pct > 100) hcho_pct = 100;
            int hcho_fill_w = (pb_w * hcho_pct) / 100;
            
            tft_fill_rect(pb_x, y_pb_hcho, pb_w, pb_h, TFT_DARKGRAY);
            if (hcho_fill_w > 0) {
                tft_fill_rect(pb_x, y_pb_hcho, hcho_fill_w, pb_h, color_hcho);
            }
        }
        #else
        /* CO2 Progress Bar - uses same color as value (SEN66 only) */
        {
            int co2_pct = (int)(((float)co2 / co2_max) * 100);
            if (co2_pct > 100) co2_pct = 100;
            int co2_fill_w = (pb_w * co2_pct) / 100;
            
            tft_fill_rect(pb_x, y_pb_co2, pb_w, pb_h, TFT_DARKGRAY);
            if (co2_fill_w > 0) {
                tft_fill_rect(pb_x, y_pb_co2, co2_fill_w, pb_h, color_co2);
            }
        }
        #endif
        
        /* VOC Progress Bar - uses same color as value */
        {
            int voc_pct = (int)((voc_idx_f / 500.0f) * 100);
            if (voc_pct > 100) voc_pct = 100;
            int voc_fill_w = (pb_w * voc_pct) / 100;
            
            tft_fill_rect(pb_x, y_pb_voc, pb_w, pb_h, TFT_DARKGRAY);
            if (voc_fill_w > 0) {
                tft_fill_rect(pb_x, y_pb_voc, voc_fill_w, pb_h, color_voc);
            }
        }

        /* ===== STATUS BAR: Three-level system ===== */
        /* Only redraw when global alert level changes */
        if (last_alert_state != global_level) {
            switch (global_level) {
                case 0:  /* BLUE: All parameters normal */
                    tft_fill_rect(0, status_y, screen_w, status_bar_height, TFT_BLUE);
                    tft_draw_string("* NORMAL *", (screen_w - 10 * 12) / 2 + 2, status_y + 5, 
                                   TFT_WHITE, TFT_BLUE, 2);
                    break;
                    
                case 1:  /* ORANGE: At least one parameter in warning */
                    tft_fill_rect(0, status_y, screen_w, status_bar_height, TFT_ORANGE);
                    tft_draw_string("* WARNING *", (screen_w - 11 * 12) / 2 + 2, status_y + 5, 
                                   TFT_WHITE, TFT_ORANGE, 2);
                    break;
                    
                case 2:  /* RED: At least one parameter in danger */
                    tft_fill_rect(0, status_y, screen_w, status_bar_height, TFT_RED);
                    tft_draw_string("* DANGER !", (screen_w - 11 * 12) / 2 + 2, status_y + 5, 
                                   TFT_WHITE, TFT_RED, 2);
                    break;
                    
                default:
                    break;
            }
            last_alert_state = global_level;
        }
    }
    #endif

    #if USE_OLED_DISPLAY
    oled_update_display(
        temp_celsius,
        humidity_pct,
        pm2_5_ugm3,
        co2,
        voc_idx_f,
        nox_idx_f,
        is_alert
    );
    #endif

    return is_alert;
}

void app_main(void)
{
    configure_led();

#if USE_TFT_LCD
    if (tft_init() == ESP_OK) {
        ESP_LOGI(TAG, "🖥️  TFT-LCD display initialized successfully");
        
        tft_fill_screen(TFT_BLACK);
        
        tft_set_rotation(0);
        
        tft_fill_screen(TFT_BG_COLOR);

        /* 第1行：主标题 + 传感器名称 */
        tft_draw_string("ESP32 ", 5, 3, TFT_CYAN, TFT_BG_COLOR, 2);
        /*                     ↑ 空格分隔 */
        {
            const char* sensor_name = SENSOR_NAME;  /* 自动获取 "SEN68" 或 "SEN66" */
            int name_len = strlen(sensor_name);     /* 动态计算长度 */
            int x_sensor = tft_get_width() - (name_len * 12) - 5;  /* 自适应屏幕宽度 */
            
            tft_draw_string(sensor_name, x_sensor, 3, TFT_GREEN, TFT_BG_COLOR, 2);
        }
        tft_draw_string("@FELLOWES", tft_get_width() - 8 * 6 - 5, 22, TFT_YELLOW, TFT_BG_COLOR, 1);
        tft_draw_string("Environment Monitor", 5, 22, TFT_WHITE, TFT_BG_COLOR, 1);

        tft_fill_rect(0, 36, tft_get_width(), 2, TFT_YELLOW);

        tft_draw_string("Initializing...", 5, 42, TFT_YELLOW, TFT_BG_COLOR, 1);
    }
#endif

    ESP_LOGI(TAG, "Setting up I2C for %s sensor...", SENSOR_NAME);
    esp_err_t err = i2c_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to initialize I2C bus! Error: 0x%x", err);
        return;
    }
    ESP_LOGI(TAG, "✅ I2C bus initialized successfully");

    /* Initialize I2C HAL layer (uses sensirion_i2c_hal_custom.c) */
    sensirion_i2c_hal_init();
    
    /* Initialize the selected sensor (SEN66 or SEN68) */
    init_environmental_sensor();

    ESP_LOGI(TAG, "Starting main loop - reading %s every %d ms...", 
             SENSOR_NAME, SENSOR_READ_PERIOD_MS);

    while (1) {
        bool alert_active = read_and_display_sensor_data();

        if (alert_active) {
            s_alert_blink_counter++;
            gpio_set_level(BLINK_GPIO, !gpio_get_level(BLINK_GPIO));
            vTaskDelay(pdMS_TO_TICKS(BLINK_INTERVAL_ALERT));
        } else {
            s_led_state = !s_led_state;
            blink_led();
            vTaskDelay(pdMS_TO_TICKS(BLINK_INTERVAL_NORMAL));
        }
    }
}
