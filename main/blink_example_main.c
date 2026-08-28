/*
 * ESP32 Environment Monitor with Multi-Sensor Support
 * 
 * Features:
 * - LED blinking on GPIO2 (D2) with configurable period
 * - Multi-sensor support: SEN54 / SEN66 / SEN68 with runtime auto-detection
 * - Three-color level system: BLUE (normal) / ORANGE (warning) / RED (danger)
 * - 240x320 TFT-LCD display with optimized layout and progress bars
 * - Modular architecture: config, alert, UI separated into dedicated modules
 * 
 * Hardware Connections:
 * - LED: GPIO2 (D2)
 * - Sensor I2C: SDA=GPIO21, SCL=GPIO22, VCC=3.3V, GND=GND
 */

/* ============================================ */
/* DISPLAY CONFIGURATION - MODIFY HERE         */
/* ============================================ */
#define USE_TFT_LCD      1
#define USE_OLED_DISPLAY 0

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

#include "sensor_config.h"
#include "app_config.h"
#include "alert_manager.h"
#include "ui_display.h"

#include "sensirion_common.h"
#if SENSOR_USE_UART
#include "uart_sensor.h"
#endif
#include "sensirion_i2c_hal.h"

#if USE_TFT_LCD
#include "tft_lcd.h"
#endif

#if USE_OLED_DISPLAY
#include "oled_display.h"
#endif

static const char *TAG = "env_monitor";

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define BLINK_INTERVAL_NORMAL  CONFIG_BLINK_PERIOD
#define BLINK_INTERVAL_ALERT   200


static uint8_t s_led_state = 0;
static bool s_alert_active = false;              /* Track if alert mode is active */
#if SENSOR_USE_UART
static uart_sensor_data_t s_uart_data;
#endif
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
#if !SENSOR_USE_UART
static void init_environmental_sensor(void)
{
    int16_t error = NO_ERROR;

    /* ===== RUNTIME SENSOR DETECTION ===== */
    /* Automatically identify SEN66 vs SEN68 at startup */
    if (detect_sensor_type() != 0) {
        ESP_LOGW(TAG, "⚠️ Sensor detection failed, using default (SEN66) mode");
    }   

    #if USE_TFT_LCD
    ui_draw_header(g_sensor_name);
    ESP_LOGI(TAG, "Title bar updated: %s sensor detected", g_sensor_name);
    #endif

    ESP_LOGI(TAG, "Initializing %s environmental sensor...", g_sensor_name);

    /* Reset device using unified function */
    error = sensor_device_reset();
    if (error != NO_ERROR) {
        ESP_LOGE(TAG, "❌ %s device reset failed: %d", g_sensor_name, error);
        return;
    }

    sensirion_i2c_hal_sleep_usec(1200000);

    /* Start continuous measurement using unified macro */
    error = sensor_start_measurement();
    if (error != NO_ERROR) {
        ESP_LOGE(TAG, "❌ Failed to start %s continuous measurement: %d", g_sensor_name, error);
        return;
    }

    ESP_LOGI(TAG, "🎉 %s sensor initialized successfully! Ready to read data.", g_sensor_name);
}
#endif

#if SENSOR_USE_UART

static bool read_and_display_uart_sensor_data(void)
{
    if (!uart_sensor_read(&s_uart_data)) {
        ESP_LOGW(TAG, "Failed to read UART sensor data");
        return false;
    }

    float temp_celsius = (float)s_uart_data.temperature;
    float humidity_pct = (float)s_uart_data.humidity;
    float pm2_5_ugm3 = (float)s_uart_data.pms_in_pm2_5;
    float pm1_ugm3 = (float)s_uart_data.pms_in_pm1_0;
    float pm10_ugm3 = (float)s_uart_data.pms_in_pm10;
    float voc_idx_f = (float)s_uart_data.tvoc_count;
    float pressure_hpa = (float)s_uart_data.pressure_count;
    uint16_t co2 = s_uart_data.co2_count;
    uint16_t aq_state = s_uart_data.aq_state;

    bool is_alert = alert_check_environmental(pm2_5_ugm3, co2);

    if (is_alert != s_alert_active) {
        if (is_alert) {
            ESP_LOGW(TAG, "ENVIRONMENTAL ALERT ACTIVATED!");
        } else {
            ESP_LOGI(TAG, "Environmental conditions returned to normal.");
        }
        s_alert_active = is_alert;
    } else if (is_alert && (s_alert_blink_counter % 10 == 0)) {
        ESP_LOGW(TAG, "ALERT ACTIVE - PM2.5: %.1f | CO2: %u | AQ: %u",
                 pm2_5_ugm3, co2, aq_state);
    }

    ESP_LOGI(TAG, "[UART] T:%.1fC H:%.1f%% P:%.1fhPa PM1:%.1f PM2.5:%.1f PM10:%.1f CO2:%u VOC:%.0f AQ:%u",
             temp_celsius, humidity_pct, pressure_hpa, pm1_ugm3, pm2_5_ugm3, pm10_ugm3,
             co2, voc_idx_f, aq_state);

#if USE_TFT_LCD
    {
        ui_sensor_data_t d = {
            .temp_celsius = temp_celsius,
            .humidity_pct = humidity_pct,
            .nox_idx_f = 0,
            .voc_idx_f = voc_idx_f,
            .pm2_5_ugm3 = pm2_5_ugm3,
            .pm1_ugm3 = pm1_ugm3,
            .pm10_ugm3 = pm10_ugm3,
            .pressure_hpa = pressure_hpa,
            .co2 = co2,
            .hcho_ppb = 0,
            .aq_state = aq_state,
            .color_temp  = alert_get_color(temp_celsius, TEMP_NORMAL_MIN, TEMP_NORMAL_MAX, TEMP_DANGER_MAX, true),
            .color_humid = alert_get_color(humidity_pct, HUMID_NORMAL_MIN, HUMID_NORMAL_MAX, HUMID_DANGER_MAX, true),
            .color_nox   = TFT_BLUE,
            .color_voc   = alert_get_color(voc_idx_f, 0, VOC_NORMAL_MAX, VOC_WARNING_MAX, false),
            .color_pm25  = alert_get_color(pm2_5_ugm3, 0, PM25_NORMAL_MAX, PM25_WARNING_MAX, false),
            .color_pm1   = alert_get_color(pm1_ugm3, 0, PM1_NORMAL_MAX, PM1_WARNING_MAX, false),
            .color_pm10  = alert_get_color(pm10_ugm3, 0, PM10_NORMAL_MAX, PM10_WARNING_MAX, false),
            .color_co2   = alert_get_color((float)co2, 0, CO2_NORMAL_MAX, CO2_WARNING_MAX, false),
            .color_pres  = alert_get_color(pressure_hpa, PRES_NORMAL_MIN, PRES_NORMAL_MAX, 1100.0f, true),
            .color_aq    = alert_get_color((float)aq_state, 0, AQ_NORMAL_MAX, AQ_WARNING_MAX, false),
            .color_hcho  = TFT_BLUE,
            .global_level = alert_get_global_level(temp_celsius, humidity_pct,
                                                    0, pm2_5_ugm3, co2, voc_idx_f, 0),
        };
        ui_draw_sensor_screen(&d);
    }
#endif

    return is_alert;
}

#endif /* SENSOR_USE_UART */

/**
 * @brief Read and display sensor data with three-color level system
 * 
 * Supports both SEN66 and SEN68 through unified API from sensor_config.h
 * Automatically handles PM1/HCHO data when using SEN68 sensor
 * 
 * @return Returns true if alert conditions are met, false otherwise
 */
#if !SENSOR_USE_UART
static bool read_and_display_sensor_data(void)
{
    uint16_t pm1p0 = 0, pm2p5 = 0, pm4p0 = 0, pm10p0 = 0;
    int16_t humidity = 0, temperature = 0, voc_index = 0, nox_index = 0;
    uint16_t co2 = 0, hcho_ppb = 0;

    int16_t error = sensor_read_measured_values(
        &pm1p0, &pm2p5, &pm4p0, &pm10p0,
        &humidity, &temperature,
        &voc_index, &nox_index,
        g_has_hcho_support ? &hcho_ppb : &co2
    );

    if (error != NO_ERROR) {
        ESP_LOGE(TAG, "Error reading %s values: %d", g_sensor_name, error);
        return false;
    }

    float temp_celsius = (float)temperature / 200.0f;
    float humidity_pct = (float)humidity / 100.0f;
    float voc_idx_f = (float)voc_index / 10.0f;
    float nox_idx_f = (float)nox_index / 10.0f;
    float pm2_5_ugm3 = (float)pm2p5 / 10.0f;
    float pm1_ugm3 = g_has_pm1_support ? (float)pm1p0 / 10.0f : 0.0f;

    bool is_alert = alert_check_environmental(pm2_5_ugm3, co2);

    if (is_alert != s_alert_active) {
        if (is_alert) {
            ESP_LOGW(TAG, "ENVIRONMENTAL ALERT ACTIVATED! PM2.5: %.1f", pm2_5_ugm3);
            if (co2 > CO2_ALERT_THRESHOLD) {
                ESP_LOGW(TAG, "  CO2: %uppm (threshold: %d)", co2, CO2_ALERT_THRESHOLD);
            }
        } else {
            ESP_LOGI(TAG, "Environmental conditions returned to normal.");
        }
        s_alert_active = is_alert;
    } else if (is_alert && (s_alert_blink_counter % 10 == 0)) {
        ESP_LOGW(TAG, "ALERT ACTIVE - PM2.5: %.1f | CO2: %uppm", pm2_5_ugm3, co2);
    }

    const char* status_icon = is_alert ? "!" : "*";
    const char* status_text = is_alert ? "ALERT" : "NORMAL";

    if (g_has_hcho_support) {
        ESP_LOGI(TAG, "[%s] %s %s | T:%.1fC | H:%.1f%% | PM2.5:%.1f | PM1:%.1f | VOC:%.1f | NOx:%.1f | HCHO:%u",
                 g_sensor_name, status_icon, status_text,
                 temp_celsius, humidity_pct, pm2_5_ugm3, pm1_ugm3,
                 voc_idx_f, nox_idx_f, hcho_ppb);
    } else if (g_has_co2_support) {
        ESP_LOGI(TAG, "[%s] %s %s | T:%.1fC | H:%.1f%% | PM2.5:%.1f | VOC:%.1f | NOx:%.1f | CO2:%u",
                 g_sensor_name, status_icon, status_text,
                 temp_celsius, humidity_pct, pm2_5_ugm3,
                 voc_idx_f, nox_idx_f, co2);
    } else {
        ESP_LOGI(TAG, "[%s] %s %s | T:%.1fC | H:%.1f%% | PM2.5:%.1f | PM1:%.1f | VOC:%.1f",
                 g_sensor_name, status_icon, status_text,
                 temp_celsius, humidity_pct, pm2_5_ugm3, pm1_ugm3, voc_idx_f);
    }

#if USE_TFT_LCD
    {
        ui_sensor_data_t d = {
            .temp_celsius = temp_celsius,
            .humidity_pct = humidity_pct,
            .nox_idx_f = nox_idx_f,
            .voc_idx_f = voc_idx_f,
            .pm2_5_ugm3 = pm2_5_ugm3,
            .pm1_ugm3 = pm1_ugm3,
            .co2 = co2,
            .hcho_ppb = hcho_ppb,
            .color_temp  = alert_get_color(temp_celsius, TEMP_NORMAL_MIN, TEMP_NORMAL_MAX, TEMP_DANGER_MAX, true),
            .color_humid = alert_get_color(humidity_pct, HUMID_NORMAL_MIN, HUMID_NORMAL_MAX, HUMID_DANGER_MAX, true),
            .color_nox   = g_has_nox_support ? alert_get_color(nox_idx_f, 0, NOX_NORMAL_MAX, NOX_WARNING_MAX, false) : TFT_BLUE,
            .color_voc   = alert_get_color(voc_idx_f, 0, VOC_NORMAL_MAX, VOC_WARNING_MAX, false),
            .color_pm25  = alert_get_color(pm2_5_ugm3, 0, PM25_NORMAL_MAX, PM25_WARNING_MAX, false),
            .color_pm1   = g_has_pm1_support ? alert_get_color(pm1_ugm3, 0, PM1_NORMAL_MAX, PM1_WARNING_MAX, false) : TFT_BLUE,
            .color_co2   = alert_get_color((float)co2, 0, CO2_NORMAL_MAX, CO2_WARNING_MAX, false),
            .color_hcho  = alert_get_color((float)hcho_ppb, 0, HCHO_NORMAL_MAX, HCHO_WARNING_MAX, false),
            .global_level = alert_get_global_level(temp_celsius, humidity_pct,
                                                    g_has_nox_support ? nox_idx_f : 0.0f,
                                                    pm2_5_ugm3,
                                                    g_has_hcho_support ? hcho_ppb :
                                                        (g_has_co2_support ? co2 : 0),
                                                    voc_idx_f, g_has_hcho_support),
        };
        ui_draw_sensor_screen(&d);
    }
#endif

#if USE_OLED_DISPLAY
    oled_update_display(temp_celsius, humidity_pct, pm2_5_ugm3, co2, voc_idx_f, nox_idx_f, is_alert);
#endif

    return is_alert;
}
#endif

void app_main(void)
{
    configure_led();

#if USE_TFT_LCD
    if (tft_init() == ESP_OK) {
        ESP_LOGI(TAG, "TFT-LCD display initialized successfully");
        tft_set_rotation(0);
    }
#endif

#if SENSOR_USE_UART
    /* ===== UART SENSOR PATH ===== */
    detect_uart_sensor();

#if USE_TFT_LCD
    ui_draw_header(g_sensor_name);
#endif

    if (!uart_sensor_init()) {
        ESP_LOGE(TAG, "Failed to initialize UART sensor");
        return;
    }
    ESP_LOGI(TAG, "UART sensor initialized successfully");

    ESP_LOGI(TAG, "Starting main loop - reading UART sensor every %d ms...",
             SENSOR_READ_PERIOD_MS);

    while (1) {
        bool alert_active = read_and_display_uart_sensor_data();

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

#else
    /* ===== I2C SENSOR PATH ===== */

#if USE_TFT_LCD
    ui_draw_header(g_sensor_name);
#endif

    ESP_LOGI(TAG, "Setting up I2C for %s sensor...", g_sensor_name);
    esp_err_t err = i2c_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus! Error: 0x%x", err);
        return;
    }
    ESP_LOGI(TAG, "I2C bus initialized successfully");

    /* Initialize I2C HAL layer */
    sensirion_i2c_hal_init();
    
    /* Initialize the selected sensor */
    init_environmental_sensor();

    ESP_LOGI(TAG, "Starting main loop - reading %s every %d ms...", 
             g_sensor_name, SENSOR_READ_PERIOD_MS);

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
#endif /* SENSOR_USE_UART */
}