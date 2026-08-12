/*
 * Blink Example with SEN66 Environmental Sensor Integration
 * 
 * Features:
 * - LED blinking on GPIO2 (D2) with configurable period
 * - SEN66 sensor reading via I2C (Temperature, Humidity, PM2.5, VOC, NOx, CO2)
 * 
 * Hardware Connections:
 * - LED: GPIO2 (D2)
 * - SEN66 I2C: SDA=GPIO21, SCL=GPIO22, VCC=3.3V, GND=GND
 */

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

/* SEN66 Sensor Driver Headers */
#include "sen66_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

/* OLED Display Driver (ESP-IDF Official) */
#include "oled_display.h"                    /* ← 新增这行 */

static const char *TAG = "example";

/* LED Configuration */
#define BLINK_GPIO CONFIG_BLINK_GPIO

/* I2C Configuration for SEN66 */
#define I2C_MASTER_SCL_IO           22        /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           21        /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0          /*!< I2C master i2c port number */
#define I2C_MASTER_FREQ_HZ          100000     /*!< I2C master clock frequency */

/* SEN66 Sensor Read Interval (ms) */
#define SEN66_READ_PERIOD_MS        5000       /*!< Read sensor every 5 seconds */

/* Sensor warm-up time before first valid reading (ms) */
#define SEN66_WARMUP_DELAY_MS       15000      /*!< Wait 15 seconds for sensor stabilization */

/* Number of initial readings to skip during warm-up */
#define SEN66_WARMUP_READINGS       3          /*!< Skip first 3 readings as they may be unreliable */

/* Alert thresholds for environmental monitoring */
#define PM25_ALERT_THRESHOLD        75         /*!< PM2.5 alert threshold in µg/m³ (WHO: >75 is unhealthy) */
#define CO2_ALERT_THRESHOLD         1000       /*!< CO₂ alert threshold in ppm (>1000 indicates poor ventilation) */

/* LED blinking intervals (ms) */
#define BLINK_INTERVAL_NORMAL       CONFIG_BLINK_PERIOD  /*!< Normal: 2000ms (from sdkconfig) */
#define BLINK_INTERVAL_ALERT        200                 /*!< Alert: 200ms (fast flash) */

static uint8_t s_led_state = 0;
static bool s_alert_active = false;              /* Track if alert mode is active */
static int s_alert_blink_counter = 0;            /* Counter for alert flashing pattern */

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
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
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

/**
 * @brief Initialize SEN66 environmental sensor
 */
static void sen66_init_sensor(void)
{
    int16_t error = NO_ERROR;

    ESP_LOGI(TAG, "Initializing SEN66 environmental sensor...");

    /* Reset sensor */
    error = sen66_device_reset();
    if (error != NO_ERROR) {
        ESP_LOGE(TAG, "❌ SEN66 device reset failed: %d", error);
        return;
    }

    /* Wait for sensor to stabilize after reset (1.2 seconds as per datasheet) */
    sensirion_i2c_hal_sleep_usec(1200000);

    /* Read and display serial number */
    int8_t serial_number[32] = {0};
    error = sen66_get_serial_number(serial_number, 32);
    if (error == NO_ERROR) {
        ESP_LOGI(TAG, "📋 SEN66 Serial Number: %s", serial_number);
    } else {
        ESP_LOGW(TAG, "⚠️  Could not read SEN66 serial number (error: %d)", error);
    }

    /* Start continuous measurement mode */
    error = sen66_start_continuous_measurement();
    if (error != NO_ERROR) {
        ESP_LOGE(TAG, "❌ Failed to start SEN66 continuous measurement: %d", error);
        return;
    }

    ESP_LOGI(TAG, "🎉 SEN66 sensor initialized successfully! Ready to read data.");
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
    
    /* Log alert status changes only */
    if (new_alert_status != s_alert_active) {
        if (new_alert_status) {
            /* Alert just started */
            ESP_LOGW(TAG, "🚨🚨🚨 ENVIRONMENTAL ALERT ACTIVATED! 🚨🚨🚨");
            if (pm25_alert) {
                ESP_LOGW(TAG, "   ⚠️  PM2.5: %.1f µg/m³ (threshold: %d)", pm25_ugm3, PM25_ALERT_THRESHOLD);
            }
            if (co2_alert) {
                ESP_LOGW(TAG, "   ⚠️  CO₂: %uppm (threshold: %d)", co2_ppm, CO2_ALERT_THRESHOLD);
            }
            ESP_LOGW(TAG, "   🔴 LED now flashing rapidly to indicate poor air quality!");
        } else {
            /* Alert cleared */
            ESP_LOGI(TAG, "✅ Environmental conditions returned to normal levels.");
            ESP_LOGI(TAG, "   💚 LED returned to normal slow blinking.");
        }
        
        s_alert_active = new_alert_status;
    } else if (new_alert_status && (s_alert_blink_counter % 10 == 0)) {
        /* Periodic reminder when in alert mode (every ~2 seconds at fast blink rate) */
        ESP_LOGW(TAG, "🔴 ALERT ACTIVE - PM2.5: %.1f | CO₂: %uppm", pm25_ugm3, co2_ppm);
    }
    
    return new_alert_status;
}

/**
 * @brief Read and display SEN66 sensor data
 * 
 * @return Returns true if alert conditions are met, false otherwise
 */
static bool sen66_read_and_display_data(void)
{
    uint16_t pm1p0 = 0, pm2p5 = 0, pm4p0 = 0, pm10p0 = 0;
    int16_t humidity = 0, temperature = 0, voc_index = 0, nox_index = 0;
    uint16_t co2 = 0;

    int16_t error = sen66_read_measured_values_as_integers(
        &pm1p0, &pm2p5, &pm4p0, &pm10p0,
        &humidity, &temperature, &voc_index, &nox_index, &co2);

    if (error != NO_ERROR) {
        ESP_LOGE(TAG, "❌ Error reading SEN66 values: %d", error);
        return false;
    }

    /* Convert raw values to physical units according to SEN66 datasheet */
    float temp_celsius = (float)temperature / 200.0f;
    float humidity_pct = (float)humidity / 100.0f;
    float voc_idx = (float)voc_index / 10.0f;
    float nox_idx = (float)nox_index / 10.0f;
    float pm2_5_ugm3 = (float)pm2p5 / 10.0f;

    /* Check for alert conditions and determine status indicator */
    bool is_alert = check_environmental_alerts(pm2_5_ugm3, co2);
    
    const char* status_icon = is_alert ? "🔴" : "💚";
    const char* status_text = is_alert ? "⚠️  ALERT" : "✅ NORMAL";

    /* Display formatted sensor data with status indicator */
    ESP_LOGI(TAG, 
             "[SEN66] %s %s | 🌡️ Temp: %.1f°C | 💧 Humidity: %.1f%% | 🌫️ PM2.5: %.1fµg/m³ | "
             "🌬️ VOC: %.1f | ⚠️  NOx: %.1f | 🔴 CO₂: %uppm",
             status_icon, status_text,
             temp_celsius, humidity_pct, pm2_5_ugm3, voc_idx, nox_idx, co2);

        /* Update OLED display with latest readings */                  /* ← 新增：注释 */
    oled_update_display(                                              /* ← 新增：调用 OLED 更新 */
        temp_celsius,          /* Temperature in Celsius */         /* ← 参数：温度 */
        humidity_pct,          /* Humidity percentage */            /* ← 参数：湿度 */
        pm2_5_ugm3,            /* PM2.5 concentration */            /* ← 参数：PM2.5 */
        co2,                   /* CO₂ PPM */                       /* ← 参数：二氧化碳 */
        voc_idx,               /* VOC index */                     /* ← 参数：VOC */
        nox_idx,               /* NOx index */                     /* ← 参数：NOx */
        is_alert               /* Alert status */                  /* ← 参数：警报状态 */
    );  

    return is_alert;
}

void app_main(void)
{
    /* Configure the peripheral according to the LED type */
    configure_led();

    /* Initialize I2C for SEN66 sensor communication */
    /* 
     * Initialize I2C and SEN66 sensor using Sensirion HAL
     * Note: sensirion_i2c_hal_init() already includes full I2C master initialization
     *       (defined in components/embedded-i2c-sen66/sensirion_i2c_hal_custom.c)
     *       So we do NOT need to call i2c_master_init() separately!
     */
    ESP_LOGI(TAG, "Initializing I2C and SEN66 sensor...");
    
    i2c_manager_init();
    /* Step 1: Initialize Sensirion I2C HAL layer (this also initializes I2C hardware) */
    sensirion_i2c_hal_init();
    
    /* Step 2: Initialize SEN66 driver with I2C address 0x69 */
    sen66_init(SEN66_I2C_ADDR_6B);
    
    /* Step 3: Start continuous measurement mode */
    sen66_init_sensor();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "🚀 System Ready! LED + SEN66 Active");
    ESP_LOGI(TAG, "========================================");

    /*
     * Wait for sensor to stabilize before reading data.
     * According to Sensirion datasheet:
     * - Temperature/Humidity: ~2 seconds to stabilize
     * - PM sensors: ~5-10 seconds for fan startup and airflow stabilization
     * - VOC/NOx algorithms: require 30+ minutes for initial learning
     * - CO₂: may show max value (0xFFFF) until first valid measurement
     *
     * We use a conservative 15-second delay to ensure first readings are reliable.
     */
    ESP_LOGI(TAG, "⏳ Waiting %d seconds for sensor warm-up and stabilization...", 
             SEN66_WARMUP_DELAY_MS / 1000);
    
    /* Initialize OLED display (using official esp_lcd driver) */     /* ← 新增：OLED 初始化 */
    if (oled_init() == ESP_OK) {                                     /* ← 新增：初始化并检查结果 */
        ESP_LOGI(TAG, "📺 OLED display initialized successfully");   /* ← 新增：成功日志 */
    } else {                                                         /* ← 新增：错误处理 */
        ESP_LOGW(TAG, "⚠️  OLED initialization failed - continuing without display");
    }
    
    int warmup_countdown = SEN66_WARMUP_DELAY_MS / CONFIG_BLINK_PERIOD;
    
    while (warmup_countdown > 0) {
        int seconds_remaining = (warmup_countdown * CONFIG_BLINK_PERIOD) / 1000;  /* ← 修改：计算秒数 */
        
        ESP_LOGI(TAG, "   ⏱️  Warm-up: %d seconds remaining...", seconds_remaining);  /* ← 修改：使用变量 */
        
        /* Update OLED with warm-up countdown */                   /* ← 新增：更新 OLED 显示 */
        oled_show_warmup(seconds_remaining);                        /* ← 新增：调用 OLED 函数 */
        
        blink_led();
        s_led_state = !s_led_state;
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        warmup_countdown--;
    }
    
    ESP_LOGI(TAG, "✅ Warm-up complete! Starting data acquisition...");
    ESP_LOGI(TAG, "");

    int counter = 0;
    int warmup_readings_remaining = SEN66_WARMUP_READINGS;  /* Track initial readings */
    
    while (1) {
        /*
         * Adaptive LED blinking based on environmental alerts:
         * - Normal mode: Slow blink every 2 seconds (CONFIG_BLINK_PERIOD)
         * - Alert mode: Fast blink every 200 milliseconds for visibility
         */
        uint32_t current_blink_interval = s_alert_active ? BLINK_INTERVAL_ALERT : BLINK_INTERVAL_NORMAL;
        
        /* Toggle LED and log state with mode indicator */
        const char* led_action = s_led_state ? "ON" : "OFF";
        const char* mode_indicator = s_alert_active ? "🔴 [ALERT]" : "💚 [NORMAL]";
        
        ESP_LOGI(TAG, "💡 Turning the LED %s! %s (cycle #%d)", 
                 led_action, mode_indicator, counter++);
        
        blink_led();
        s_led_state = !s_led_state;

        /* Read SEN66 sensor data at regular intervals */
        if (counter % (SEN66_READ_PERIOD_MS / CONFIG_BLINK_PERIOD) == 0) {
            
            /* Check if still in warm-up period for initial readings */
            if (warmup_readings_remaining > 0) {
                ESP_LOGW(TAG, "[SEN66] 🔥 WARM-UP READING (%d remaining) - Data may be inaccurate!", 
                         warmup_readings_remaining);
                sen66_read_and_display_data();
                warmup_readings_remaining--;
                
                if (warmup_readings_remaining == 0) {
                    ESP_LOGI(TAG, "[SEN66] ✅ Warm-up period complete! Subsequent readings should be accurate.");
                    ESP_LOGI(TAG, "");
                }
            } else {
                /* 
                 * Normal stable reading - this function internally calls
                 * check_environmental_alerts() which updates s_alert_active
                 */
                sen66_read_and_display_data();
            }
        }

        /* 
         * KEY: Use adaptive delay based on alert status!
         * - Normal: 2000ms (slow blink)
         * - Alert:  200ms  (fast blink - 10x faster!)
         */
        vTaskDelay(current_blink_interval / portTICK_PERIOD_MS);
        
        /* Track alert blinks for periodic logging */
        if (s_alert_active) {
            s_alert_blink_counter++;
        } else {
            s_alert_blink_counter = 0;
        }
    }
}