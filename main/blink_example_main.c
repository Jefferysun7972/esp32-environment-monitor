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

#include <math.h>
#include "am2020dy.h"
#include "sensirion_i2c_hal.h"
#include "sen68_i2c.h"
#include "sensirion_common.h"

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
static bool s_alert_active = false;
static int s_alert_blink_counter = 0;
static i2c_master_dev_handle_t s_am2020dy_dev_handle = NULL;
static bool g_sen68_ready = false;
static bool g_am2020dy_ready = false;

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
 * @brief Initialize AM2020DY I2C sensor
 *
 * Uses command-based detection (sends Read Product Name command)
 * instead of i2c_master_probe, because AM2020DY uses a frame-based
 * protocol and may not respond to bare address probes.
 */
static void init_am2020dy_sensor(void)
{
    ESP_LOGI(TAG, "Initializing AM2020DY sensor...");

    if (detect_am2020dy(&s_am2020dy_dev_handle) != 0) {
        ESP_LOGE(TAG, "❌ AM2020DY sensor not found!");
        return;
    }

#if USE_TFT_LCD
    ui_draw_header(g_sensor_name);
    ESP_LOGI(TAG, "Title bar updated: %s sensor detected", g_sensor_name);
#endif

    g_am2020dy_ready = true;
    ESP_LOGI(TAG, "🎉 AM2020DY sensor initialized successfully!");
}

/**
 * @brief Initialize SEN66 I2C sensor
 *
 * Uses the shared I2C bus from i2c_manager. The SEN66 HAL creates
 * its own device handle at 0x6B on the same bus as AM2020DY (0x28).
 */
static bool read_and_display_am2020dy_data(void)
{
    am2020dy_data_t data;

    if (am2020dy_read_measurement(s_am2020dy_dev_handle, &data) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read AM2020DY data");
        return false;
    }

    float pm25 = (float)data.pm2_5;
    bool is_alert = alert_check_environmental(pm25, 0);

    if (is_alert != s_alert_active) {
        if (is_alert) {
            ESP_LOGW(TAG, "ENVIRONMENTAL ALERT ACTIVATED! PM2.5: %.1f", pm25);
        } else {
            ESP_LOGI(TAG, "Environmental conditions returned to normal.");
        }
        s_alert_active = is_alert;
    }

    ESP_LOGI(TAG, "[AM2020DY] T:%.1fC H:%.1f%% PM1:%u PM2.5:%u PM10:%u TVOC:%u NO2:%u HCHO:%u",
             data.temperature, data.humidity,
             data.pm1_0, data.pm2_5, data.pm10,
             data.tvoc, data.no2, data.hcho);

#if USE_TFT_LCD
    {
        ui_sensor_data_t d = {
            .temp_celsius = data.temperature,
            .humidity_pct = data.humidity,
            .nox_idx_f = (float)data.no2,
            .voc_idx_f = (float)data.tvoc,
            .pm2_5_ugm3 = (float)data.pm2_5,
            .pm1_ugm3 = (float)data.pm1_0,
            .pm10_ugm3 = (float)data.pm10,
            .co2 = 0,
            .hcho_ppb = data.hcho,
            .aq_state = 0,
            .pressure_hpa = 0,
            .color_temp  = alert_get_color(data.temperature, TEMP_NORMAL_MIN, TEMP_NORMAL_MAX, TEMP_DANGER_MAX, true),
            .color_humid = alert_get_color(data.humidity, HUMID_NORMAL_MIN, HUMID_NORMAL_MAX, HUMID_DANGER_MAX, true),
            .color_nox   = TFT_BLUE,
            .color_voc   = alert_get_color((float)data.tvoc, 0, VOC_NORMAL_MAX, VOC_WARNING_MAX, false),
            .color_pm25  = alert_get_color((float)data.pm2_5, 0, PM25_NORMAL_MAX, PM25_WARNING_MAX, false),
            .color_pm1   = alert_get_color((float)data.pm1_0, 0, PM1_NORMAL_MAX, PM1_WARNING_MAX, false),
            .color_pm10  = alert_get_color((float)data.pm10, 0, PM10_NORMAL_MAX, PM10_WARNING_MAX, false),
            .color_co2   = TFT_BLUE,
            .color_hcho  = alert_get_color((float)data.hcho, 0, HCHO_NORMAL_MAX, HCHO_WARNING_MAX, false),
            .color_pres  = TFT_BLUE,
            .color_aq    = TFT_BLUE,
            .global_level = alert_get_global_level(data.temperature, data.humidity,
                                                    0, (float)data.pm2_5, 0, (float)data.tvoc, 0),
        };
        ui_draw_sensor_screen(&d);
    }
#endif

    return is_alert;
}

static void init_sen68_sensor(void)
{
    ESP_LOGI(TAG, "Initializing SEN68 sensor...");

    sensirion_i2c_hal_init();

    sen68_init(SEN68_I2C_ADDR_6B);

    int16_t err = sen68_device_reset();
    if (err != NO_ERROR) {
        ESP_LOGW(TAG, "SEN68 device reset failed (error: %d), continuing...", err);
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    int8_t product_name[32] = {0};
    err = sen68_get_product_name(product_name, sizeof(product_name));
    if (err != NO_ERROR) {
        ESP_LOGE(TAG, "❌ SEN68 not detected (error: %d)", err);
        return;
    }
    ESP_LOGI(TAG, "SEN68 product name: %s", (char*)product_name);

    err = sen68_start_continuous_measurement();
    if (err != NO_ERROR) {
        ESP_LOGE(TAG, "❌ SEN68 failed to start measurement (error: %d)", err);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    g_sen68_ready = true;
    g_has_am2020dy = 1;
    g_display_mode = DISPLAY_MODE_DUAL;
    g_sensor_name = "AM2020DY vs SEN68";
    g_sensor_type = SENSOR_DUAL_I2C;

#if USE_TFT_LCD
    ui_draw_header(g_sensor_name);
    ESP_LOGI(TAG, "Title bar updated: %s", g_sensor_name);
#endif

    ESP_LOGI(TAG, "🎉 SEN68 sensor initialized successfully!");
    ESP_LOGI(TAG, "Dual I2C sensor mode: AM2020DY + SEN68");
}

static bool read_and_display_dual_data(void)
{
    am2020dy_data_t a_data;
    uint16_t s_pm1, s_pm25, s_pm4, s_pm10;
    int16_t s_hum, s_temp, s_voc, s_nox;
    uint16_t s_hcho;

    bool a_ok = (am2020dy_read_measurement(s_am2020dy_dev_handle, &a_data) == ESP_OK);
    bool s_ok = (sen68_read_measured_values_as_integers(
                     &s_pm1, &s_pm25, &s_pm4, &s_pm10,
                     &s_hum, &s_temp, &s_voc, &s_nox, &s_hcho) == NO_ERROR);

    if (!a_ok) {
        ESP_LOGW(TAG, "Failed to read AM2020DY data");
    }
    if (!s_ok) {
        ESP_LOGW(TAG, "Failed to read SEN68 data");
    }

    float s_temp_f = s_temp / 200.0f;
    float s_hum_f = s_hum / 100.0f;
    float s_voc_f = s_voc / 10.0f;
    float s_nox_f = s_nox / 10.0f;
    float s_hcho_f = s_hcho / 10.0f;

    float voc_clamped = (s_voc_f > 500.0f) ? 500.0f : s_voc_f;
    float s_tvoc_f = (logf(501.0f - voc_clamped) - 6.24f) * (-313.6f);

    if (a_ok) {
        ESP_LOGI(TAG, "[AM2020DY] T:%5.1fC H:%5.1f%% PM1:%4u PM2.5:%4u PM10:%4u TVOC:%5u NO2:%4u HCHO:%6u",
                 a_data.temperature, a_data.humidity,
                 a_data.pm1_0, a_data.pm2_5, a_data.pm10,
                 a_data.tvoc, a_data.no2, a_data.hcho);
    }
    if (s_ok) {
        ESP_LOGI(TAG, "[SEN68   ] T:%5.1fC H:%5.1f%% PM1:%4u PM2.5:%4u PM10:%4u TVOC:%5.1f NOx:%4.1f HCHO:%6.1f",
                 s_temp_f, s_hum_f,
                 s_pm1, s_pm25, s_pm10,
                 s_tvoc_f, s_nox_f, s_hcho_f);
    }

    float a_temp = a_data.temperature;
    float a_hum = a_data.humidity;
    float pm25_for_alert = a_ok ? (float)a_data.pm2_5 : (float)s_pm25;
    bool is_alert = alert_check_environmental(pm25_for_alert, 0);

    if (is_alert != s_alert_active) {
        if (is_alert) {
            ESP_LOGW(TAG, "ENVIRONMENTAL ALERT ACTIVATED!");
        } else {
            ESP_LOGI(TAG, "Environmental conditions returned to normal.");
        }
        s_alert_active = is_alert;
    }

#if USE_TFT_LCD
    {
        ui_dual_i2c_data_t d = {
            .a_temp     = a_temp,
            .a_humidity = a_hum,
            .a_pm1      = (float)a_data.pm1_0,
            .a_pm25     = (float)a_data.pm2_5,
            .a_pm10     = (float)a_data.pm10,
            .a_tvoc     = a_data.tvoc,
            .a_no2      = a_data.no2,
            .a_hcho     = a_data.hcho,
            .s_temp     = s_temp_f,
            .s_humidity = s_hum_f,
            .s_pm1      = (float)s_pm1,
            .s_pm25     = (float)s_pm25,
            .s_pm10     = (float)s_pm10,
            .s_tvoc     = s_tvoc_f,
            .s_nox      = s_nox_f,
            .s_hcho     = s_hcho_f,
            .color_temp  = alert_get_color(fmaxf(a_temp, s_temp_f), TEMP_NORMAL_MIN, TEMP_NORMAL_MAX, TEMP_DANGER_MAX, true),
            .color_humid = alert_get_color(fmaxf(a_hum, s_hum_f), HUMID_NORMAL_MIN, HUMID_NORMAL_MAX, HUMID_DANGER_MAX, true),
            .color_pm1   = alert_get_color(fmaxf((float)a_data.pm1_0, (float)s_pm1), 0, PM1_NORMAL_MAX, PM1_WARNING_MAX, false),
            .color_pm25  = alert_get_color(fmaxf((float)a_data.pm2_5, (float)s_pm25), 0, PM25_NORMAL_MAX, PM25_WARNING_MAX, false),
            .color_pm10  = alert_get_color(fmaxf((float)a_data.pm10, (float)s_pm10), 0, PM10_NORMAL_MAX, PM10_WARNING_MAX, false),
            .color_hcho  = alert_get_color(fmaxf((float)a_data.hcho, (float)s_hcho), 0, HCHO_NORMAL_MAX, HCHO_WARNING_MAX, false),
            .color_tvoc  = alert_get_color(fmaxf((float)a_data.tvoc, s_tvoc_f), 0, VOC_NORMAL_MAX, VOC_WARNING_MAX, false),
            .color_nox   = alert_get_color((float)a_data.no2, 0, NOX_NORMAL_MAX, NOX_WARNING_MAX, false),
            .global_level = alert_get_global_level(a_temp, a_hum, (float)a_data.no2,
                                                    (float)a_data.pm2_5, a_data.hcho,
                                                    (float)a_data.tvoc, 1),
        };
        ui_draw_compare_table(&d);
    }
#endif

    return is_alert;
}

void app_main(void)
{
    configure_led();

#if USE_TFT_LCD
    if (tft_init() == ESP_OK) {
        ESP_LOGI(TAG, "TFT-LCD display initialized successfully");
        tft_set_rotation(0);
    }
#endif

    /* ===== I2C BUS INITIALIZATION ===== */
    ESP_LOGI(TAG, "Setting up I2C bus...");
    esp_err_t err = i2c_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus! Error: 0x%x", err);
        return;
    }
    ESP_LOGI(TAG, "I2C bus initialized successfully");

    /* Skip i2c_master_probe scan: AM2020DY uses frame-based protocol
     * that does not respond to bare address ACK probes.
     * Command-based detection will be done in init_am2020dy_sensor(). */
    ESP_LOGI(TAG, "Skipping i2c_master_probe scan (AM2020DY uses frame protocol)");

    /* Initialize AM2020DY sensor */
    init_am2020dy_sensor();

    if (!g_am2020dy_ready) {
        ESP_LOGE(TAG, "❌ AM2020DY not found - halting. Check wiring and pull-up resistors.");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    /* Initialize SEN68 sensor */
    init_sen68_sensor();

    if (g_sen68_ready) {
        g_display_mode = DISPLAY_MODE_DUAL;
        g_sensor_name = "AM2020DY vs SEN68";
    } else {
        g_display_mode = DISPLAY_MODE_SINGLE;
        g_sensor_name = "AM2020DY";
    }

#if USE_TFT_LCD
    ui_draw_header(g_sensor_name);
    ESP_LOGI(TAG, "Title bar updated: %s", g_sensor_name);
#endif

    ESP_LOGI(TAG, "Starting main loop - reading %s every %d ms...",
             g_sensor_name, SENSOR_READ_PERIOD_MS);

    while (1) {
        bool alert_active;

        if (g_sen68_ready) {
            alert_active = read_and_display_dual_data();
        } else {
            alert_active = read_and_display_am2020dy_data();
        }

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