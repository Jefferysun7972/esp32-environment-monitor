/*
 * UI Display Implementation
 */

#include "ui_display.h"
#include "sensor_config.h"
#include <stdio.h>
#include <string.h>

/* ============================================ */
/* LAYOUT CONSTANTS                             */
/* ============================================ */
#define UI_HEADER_HEIGHT      40
#define UI_STATUS_BAR_HEIGHT  24
#define UI_LEFT_MARGIN        10
#define UI_RIGHT_MARGIN       4
#define UI_CHAR_W             12
#define UI_CHAR_H             16
#define UI_PROGRESS_BAR_H     14
#define UI_PROGRESS_BAR_GAP   6

/* Progress bar max values for scaling */
#define PM25_MAX_SCALE        500.0f
#define HCHO_MAX_SCALE        300.0f
#define CO2_MAX_SCALE         5000.0f
#define VOC_MAX_SCALE         500.0f

/* Value X offsets for centered alignment */
#define VAL_X_BASIC           85
#define VAL_X_PM              90

/* ============================================ */
/* STATIC STATE FOR INCREMENTAL UPDATES         */
/* ============================================ */
static bool s_first_draw = true;
static int s_last_alert_state = -1;

void ui_draw_header(const char *sensor_name)
{
    int screen_w = tft_get_width();

    tft_fill_screen(TFT_BG_COLOR);

    tft_draw_string("ESP32 ", 5, 3, TFT_CYAN, TFT_BG_COLOR, 2);

    {
        int name_len = strlen(sensor_name);
        int x_sensor = screen_w - (name_len * 6) - 5;
        tft_draw_string(sensor_name, x_sensor, 7, TFT_GREEN, TFT_BG_COLOR, 1);
    }

    tft_draw_string("@FELLOWES", screen_w - 8 * 6 - 5, 22, TFT_YELLOW, TFT_BG_COLOR, 1);
    tft_draw_string("Environment Monitor", 5, 22, TFT_WHITE, TFT_BG_COLOR, 1);

    tft_fill_rect(0, 36, screen_w, 2, TFT_YELLOW);

    tft_draw_string("Initializing...", 5, 42, TFT_YELLOW, TFT_BG_COLOR, 1);
}

/* ============================================ */
/* UART SENSOR LAYOUT (compact, multi-parameter) */
/* ============================================ */
static void ui_draw_uart_sensor_screen(const ui_sensor_data_t *d)
{
    char buf[64];
    int screen_w = tft_get_width();
    int screen_h = tft_get_height();

    int pb_x = UI_LEFT_MARGIN;
    int pb_w = screen_w - (UI_LEFT_MARGIN * 2);

    int row_gap = 20;

    int y_temp  = UI_HEADER_HEIGHT + 6;
    int y_humid = y_temp + row_gap;
    int y_pres  = y_humid + row_gap;
    int y_sep1  = y_pres + 16;

    int y_pm1   = y_sep1 + 6;
    int y_pm25  = y_pm1 + row_gap;
    int y_pb_pm25 = y_pm25 + UI_CHAR_H + UI_PROGRESS_BAR_GAP;
    int y_pm10  = y_pb_pm25 + UI_PROGRESS_BAR_H + 8;

    int y_sep2  = y_pm10 + row_gap;

    int y_co2   = y_sep2 + 6;
    int y_pb_co2 = y_co2 + UI_CHAR_H + UI_PROGRESS_BAR_GAP;
    int y_voc   = y_pb_co2 + UI_PROGRESS_BAR_H + 6;
    int y_pb_voc = y_voc + UI_CHAR_H + UI_PROGRESS_BAR_GAP;

    int status_y = screen_h - UI_STATUS_BAR_HEIGHT;

    float co2_max = CO2_MAX_SCALE;
    int val_x_basic = UI_LEFT_MARGIN + VAL_X_BASIC;
    int val_x_pm = UI_LEFT_MARGIN + VAL_X_PM;

    if (s_first_draw) {
        tft_fill_rect(0, UI_HEADER_HEIGHT, screen_w,
                      screen_h - UI_HEADER_HEIGHT - UI_STATUS_BAR_HEIGHT, TFT_BG_COLOR);

        tft_draw_string("Temp:", UI_LEFT_MARGIN, y_temp, d->color_temp, TFT_BG_COLOR, 2);
        tft_draw_string("C", screen_w - 1 * UI_CHAR_W - UI_RIGHT_MARGIN, y_temp, d->color_temp, TFT_BG_COLOR, 2);

        tft_draw_string("Hum:", UI_LEFT_MARGIN, y_humid, d->color_humid, TFT_BG_COLOR, 2);
        tft_draw_string("%", screen_w - 1 * UI_CHAR_W - UI_RIGHT_MARGIN, y_humid, d->color_humid, TFT_BG_COLOR, 2);

        tft_draw_string("Pres:", UI_LEFT_MARGIN, y_pres, d->color_pres, TFT_BG_COLOR, 2);
        tft_draw_string("hPa", screen_w - 3 * UI_CHAR_W - UI_RIGHT_MARGIN, y_pres, d->color_pres, TFT_BG_COLOR, 2);

        tft_fill_rect(8, y_sep1, screen_w - 16, 2, TFT_DARKGRAY);

        tft_draw_string("PM1.0:", UI_LEFT_MARGIN, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);
        tft_draw_string("ug/m3", screen_w - 5 * UI_CHAR_W - UI_RIGHT_MARGIN, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);
        tft_draw_string("PM2.5:", UI_LEFT_MARGIN, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);
        tft_draw_string("ug/m3", screen_w - 5 * UI_CHAR_W - UI_RIGHT_MARGIN, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);

        tft_draw_string("PM10:", UI_LEFT_MARGIN, y_pm10, d->color_pm10, TFT_BG_COLOR, 2);
        tft_draw_string("ug/m3", screen_w - 5 * UI_CHAR_W - UI_RIGHT_MARGIN, y_pm10, d->color_pm10, TFT_BG_COLOR, 2);

        tft_fill_rect(8, y_sep2, screen_w - 16, 2, TFT_DARKGRAY);

        tft_draw_string("CO2:", UI_LEFT_MARGIN, y_co2, d->color_co2, TFT_BG_COLOR, 2);
        tft_draw_string("ppm", screen_w - 3 * UI_CHAR_W - UI_RIGHT_MARGIN, y_co2, d->color_co2, TFT_BG_COLOR, 2);

        tft_draw_string("VOC:", UI_LEFT_MARGIN, y_voc, d->color_voc, TFT_BG_COLOR, 2);

        tft_fill_rect(pb_x, y_pb_pm25, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        tft_fill_rect(pb_x, y_pb_co2, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        tft_fill_rect(pb_x, y_pb_voc, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);

        s_first_draw = false;
    }

    /* === UPDATE MODE === */

    tft_draw_string("Temp:", UI_LEFT_MARGIN, y_temp, d->color_temp, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_basic, y_temp, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6.1f", d->temp_celsius);
    tft_draw_string(buf, val_x_basic, y_temp, d->color_temp, TFT_BG_COLOR, 2);

    tft_draw_string("Hum:", UI_LEFT_MARGIN, y_humid, d->color_humid, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_basic, y_humid, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6.1f", d->humidity_pct);
    tft_draw_string(buf, val_x_basic, y_humid, d->color_humid, TFT_BG_COLOR, 2);

    tft_draw_string("Pres:", UI_LEFT_MARGIN, y_pres, d->color_pres, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_basic, y_pres, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6.1f", d->pressure_hpa);
    tft_draw_string(buf, val_x_basic, y_pres, d->color_pres, TFT_BG_COLOR, 2);

    tft_draw_string("PM1.0:", UI_LEFT_MARGIN, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_basic, y_pm1, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6.1f", d->pm1_ugm3);
    tft_draw_string(buf, val_x_basic, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);

    tft_draw_string("PM2.5:", UI_LEFT_MARGIN, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_pm, y_pm25, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6.1f", d->pm2_5_ugm3);
    tft_draw_string(buf, val_x_pm, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);

    tft_draw_string("PM10:", UI_LEFT_MARGIN, y_pm10, d->color_pm10, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_basic, y_pm10, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6.1f", d->pm10_ugm3);
    tft_draw_string(buf, val_x_basic, y_pm10, d->color_pm10, TFT_BG_COLOR, 2);

    tft_draw_string("CO2:", UI_LEFT_MARGIN, y_co2, d->color_co2, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_basic, y_co2, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6u", d->co2);
    tft_draw_string(buf, val_x_basic, y_co2, d->color_co2, TFT_BG_COLOR, 2);

    tft_draw_string("VOC:", UI_LEFT_MARGIN, y_voc, d->color_voc, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_basic, y_voc, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6.1f", d->voc_idx_f);
    tft_draw_string(buf, val_x_basic, y_voc, d->color_voc, TFT_BG_COLOR, 2);

    /* PM2.5 Progress Bar */
    {
        int pm25_pct = (int)((d->pm2_5_ugm3 / PM25_MAX_SCALE) * 100);
        if (pm25_pct > 100) pm25_pct = 100;
        int pm25_fill_w = (pb_w * pm25_pct) / 100;
        tft_fill_rect(pb_x, y_pb_pm25, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (pm25_fill_w > 0) {
            tft_fill_rect(pb_x, y_pb_pm25, pm25_fill_w, UI_PROGRESS_BAR_H, d->color_pm25);
        }
    }

    /* CO2 Progress Bar */
    {
        int co2_pct = (int)(((float)d->co2 / co2_max) * 100);
        if (co2_pct > 100) co2_pct = 100;
        int co2_fill_w = (pb_w * co2_pct) / 100;
        tft_fill_rect(pb_x, y_pb_co2, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (co2_fill_w > 0) {
            tft_fill_rect(pb_x, y_pb_co2, co2_fill_w, UI_PROGRESS_BAR_H, d->color_co2);
        }
    }

    /* VOC Progress Bar */
    {
        int voc_pct = (int)((d->voc_idx_f / VOC_MAX_SCALE) * 100);
        if (voc_pct > 100) voc_pct = 100;
        int voc_fill_w = (pb_w * voc_pct) / 100;
        tft_fill_rect(pb_x, y_pb_voc, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (voc_fill_w > 0) {
            tft_fill_rect(pb_x, y_pb_voc, voc_fill_w, UI_PROGRESS_BAR_H, d->color_voc);
        }
    }

    /* Status Bar */
    if (s_last_alert_state != d->global_level) {
        switch (d->global_level) {
            case 0:
                tft_fill_rect(0, status_y, screen_w, UI_STATUS_BAR_HEIGHT, TFT_BLUE);
                tft_draw_string("* NORMAL *", (screen_w - 10 * UI_CHAR_W) / 2 + 2, status_y + 5,
                               TFT_WHITE, TFT_BLUE, 2);
                break;
            case 1:
                tft_fill_rect(0, status_y, screen_w, UI_STATUS_BAR_HEIGHT, TFT_ORANGE);
                tft_draw_string("* WARNING *", (screen_w - 11 * UI_CHAR_W) / 2 + 2, status_y + 5,
                               TFT_WHITE, TFT_ORANGE, 2);
                break;
            case 2:
                tft_fill_rect(0, status_y, screen_w, UI_STATUS_BAR_HEIGHT, TFT_RED);
                tft_draw_string("* DANGER !", (screen_w - 11 * UI_CHAR_W) / 2 + 2, status_y + 5,
                               TFT_WHITE, TFT_RED, 2);
                break;
            default:
                break;
        }
        s_last_alert_state = d->global_level;
    }
}

void ui_draw_sensor_screen(const ui_sensor_data_t *d)
{
    if (g_sensor_type == SENSOR_UART) {
    ui_draw_uart_sensor_screen(d);
    return;
    }

    char buf[64];
    int screen_w = tft_get_width();
    int screen_h = tft_get_height();

    int pb_x = UI_LEFT_MARGIN;
    int pb_w = screen_w - (UI_LEFT_MARGIN * 2);

    int basic_row_gap = g_has_pm1_support ? 24 : 30;
    int sep_gap = g_has_pm1_support ? 20 : 24;
    int air_quality_gap = g_has_pm1_support ? 6 : 10;

    int y_temp  = UI_HEADER_HEIGHT + 6;
    int y_humid = y_temp + basic_row_gap;
    int y_nox = 0;
    int y_sep1;

    if (g_has_nox_support) {
        y_nox = y_humid + basic_row_gap;
        y_sep1 = y_nox + sep_gap;
    } else {
        y_sep1 = y_humid + sep_gap;
    }

    int y_pm1 = 0, y_pb_pm1 = 0;
    int y_pm25, y_pb_pm25;
    int y_hcho = 0, y_pb_hcho = 0;
    int y_co2 = 0, y_pb_co2 = 0;
    int y_voc, y_pb_voc;

    float hcho_max = HCHO_MAX_SCALE;
    float co2_max = CO2_MAX_SCALE;

    if (g_has_pm1_support) {
        y_pm1 = y_sep1 + 6;
        y_pb_pm1 = y_pm1 + UI_CHAR_H + UI_PROGRESS_BAR_GAP;
        y_pm25 = y_pb_pm1 + UI_PROGRESS_BAR_H + 10;
    } else {
        y_pm25 = y_sep1 + 8;
    }
    y_pb_pm25 = y_pm25 + UI_CHAR_H + UI_PROGRESS_BAR_GAP;

    if (g_has_hcho_support) {
        y_hcho = y_pb_pm25 + UI_PROGRESS_BAR_H + air_quality_gap;
        y_pb_hcho = y_hcho + UI_CHAR_H + UI_PROGRESS_BAR_GAP;
        y_voc = y_pb_hcho + UI_PROGRESS_BAR_H + air_quality_gap;
    } else if (g_has_co2_support) {
        y_co2 = y_pb_pm25 + UI_PROGRESS_BAR_H + air_quality_gap;
        y_pb_co2 = y_co2 + UI_CHAR_H + UI_PROGRESS_BAR_GAP;
        y_voc = y_pb_co2 + UI_PROGRESS_BAR_H + air_quality_gap;
    } else {
        y_voc = y_pb_pm25 + UI_PROGRESS_BAR_H + air_quality_gap;
    }
    y_pb_voc = y_voc + UI_CHAR_H + UI_PROGRESS_BAR_GAP;

    int status_y = screen_h - UI_STATUS_BAR_HEIGHT;

    int val_x_basic = UI_LEFT_MARGIN + VAL_X_BASIC;

    /* ===== FIRST DRAW: Complete layout initialization ===== */
    if (s_first_draw) {
        tft_fill_rect(0, UI_HEADER_HEIGHT, screen_w,
                      screen_h - UI_HEADER_HEIGHT - UI_STATUS_BAR_HEIGHT, TFT_BG_COLOR);

        /* Zone 1: Basic parameters */
        tft_draw_string("Temp:", UI_LEFT_MARGIN, y_temp, d->color_temp, TFT_BG_COLOR, 2);
        tft_draw_string("C", screen_w - 1 * UI_CHAR_W - UI_RIGHT_MARGIN, y_temp, d->color_temp, TFT_BG_COLOR, 2);

        tft_draw_string("Hum:", UI_LEFT_MARGIN, y_humid, d->color_humid, TFT_BG_COLOR, 2);
        tft_draw_string("%", screen_w - 1 * UI_CHAR_W - UI_RIGHT_MARGIN, y_humid, d->color_humid, TFT_BG_COLOR, 2);

        if (g_has_nox_support) {
            tft_draw_string("NOx:", UI_LEFT_MARGIN, y_nox, d->color_nox, TFT_BG_COLOR, 2);
        }

        if (g_has_pm1_support) {
            tft_draw_string("PM1.0:", UI_LEFT_MARGIN, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);
            tft_draw_string("ug/m3", screen_w - 5 * UI_CHAR_W - UI_RIGHT_MARGIN, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);
        }

        /* Separator */
        tft_fill_rect(8, y_sep1, screen_w - 16, 2, TFT_DARKGRAY);

        /* Zone 2: Air quality */
        tft_draw_string("PM2.5:", UI_LEFT_MARGIN, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);
        tft_draw_string("ug/m3", screen_w - 5 * UI_CHAR_W - UI_RIGHT_MARGIN, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);

        if (g_has_hcho_support) {
            tft_draw_string("HCHO:", UI_LEFT_MARGIN, y_hcho, d->color_hcho, TFT_BG_COLOR, 2);
            tft_draw_string("ppb", screen_w - 3 * UI_CHAR_W - UI_RIGHT_MARGIN, y_hcho, d->color_hcho, TFT_BG_COLOR, 2);
            tft_fill_rect(val_x_basic, y_hcho, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
            snprintf(buf, sizeof(buf), "%6.1f", (float)d->hcho_ppb);
            tft_draw_string(buf, val_x_basic, y_hcho, d->color_hcho, TFT_BG_COLOR, 2);
        } else if (g_has_co2_support) {
            tft_draw_string("CO2:", UI_LEFT_MARGIN, y_co2, d->color_co2, TFT_BG_COLOR, 2);
            tft_draw_string("ppm", screen_w - 3 * UI_CHAR_W - UI_RIGHT_MARGIN, y_co2, d->color_co2, TFT_BG_COLOR, 2);
        }

        /* Zone 3: Gas sensor */
        tft_draw_string("VOC:", UI_LEFT_MARGIN, y_voc, d->color_voc, TFT_BG_COLOR, 2);

        /* Progress bar backgrounds */
        tft_fill_rect(pb_x, y_pb_pm25, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (g_has_pm1_support) {
            tft_fill_rect(pb_x, y_pb_pm1, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        }
        if (g_has_hcho_support) {
            tft_fill_rect(pb_x, y_pb_hcho, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        } else if (g_has_co2_support) {
            tft_fill_rect(pb_x, y_pb_co2, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        }
        tft_fill_rect(pb_x, y_pb_voc, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);

        s_first_draw = false;
    }

    /* ===== UPDATE MODE: Dynamic three-color refresh ===== */

    /* Temperature */
    tft_draw_string("Temp:", UI_LEFT_MARGIN, y_temp, d->color_temp, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_basic, y_temp, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6.1f", d->temp_celsius);
    tft_draw_string(buf, val_x_basic, y_temp, d->color_temp, TFT_BG_COLOR, 2);
    tft_draw_string("C", screen_w - 1 * UI_CHAR_W - UI_RIGHT_MARGIN, y_temp, d->color_temp, TFT_BG_COLOR, 2);

    /* Humidity */
    tft_draw_string("Hum:", UI_LEFT_MARGIN, y_humid, d->color_humid, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_basic, y_humid, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6.1f", d->humidity_pct);
    tft_draw_string(buf, val_x_basic, y_humid, d->color_humid, TFT_BG_COLOR, 2);
    tft_draw_string("%", screen_w - 1 * UI_CHAR_W - UI_RIGHT_MARGIN, y_humid, d->color_humid, TFT_BG_COLOR, 2);

    /* NOx */
    if (g_has_nox_support) {
        tft_draw_string("NOx:", UI_LEFT_MARGIN, y_nox, d->color_nox, TFT_BG_COLOR, 2);
        tft_fill_rect(val_x_basic, y_nox, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%6.1f", d->nox_idx_f);
        tft_draw_string(buf, val_x_basic, y_nox, d->color_nox, TFT_BG_COLOR, 2);
    }

    /* PM1.0 (SEN68 only) */
    if (g_has_pm1_support) {
        tft_draw_string("PM1.0:", UI_LEFT_MARGIN, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);
        tft_fill_rect(val_x_basic, y_pm1, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%6.1f", d->pm1_ugm3);
        tft_draw_string(buf, val_x_basic, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);
        tft_draw_string("ug/m3", screen_w - 5 * UI_CHAR_W - UI_RIGHT_MARGIN, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);
    }

    /* PM2.5 */
    {
        int val_x_pm = UI_LEFT_MARGIN + VAL_X_PM;
        tft_draw_string("PM2.5:", UI_LEFT_MARGIN, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);
        tft_fill_rect(val_x_pm, y_pm25, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%6.1f", d->pm2_5_ugm3);
        tft_draw_string(buf, val_x_pm, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);
        tft_draw_string("ug/m3", screen_w - 5 * UI_CHAR_W - UI_RIGHT_MARGIN, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);
    }

    /* HCHO or CO2 Progress Bar */
    if (g_has_hcho_support) {
        tft_draw_string("HCHO:", UI_LEFT_MARGIN, y_hcho, d->color_hcho, TFT_BG_COLOR, 2);
        tft_fill_rect(val_x_basic, y_hcho, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
        snprintf(buf, sizeof(buf), "%6.1f", (float)d->hcho_ppb);
        tft_draw_string(buf, val_x_basic, y_hcho, d->color_hcho, TFT_BG_COLOR, 2);
        tft_draw_string("ppb", screen_w - 3 * UI_CHAR_W - UI_RIGHT_MARGIN, y_hcho, d->color_hcho, TFT_BG_COLOR, 2);

        int hcho_pct = (int)(((float)d->hcho_ppb / hcho_max) * 100);
        if (hcho_pct > 100) hcho_pct = 100;
        int hcho_fill_w = (pb_w * hcho_pct) / 100;
        tft_fill_rect(pb_x, y_pb_hcho, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (hcho_fill_w > 0) {
            tft_fill_rect(pb_x, y_pb_hcho, hcho_fill_w, UI_PROGRESS_BAR_H, d->color_hcho);
        }
    } else if (g_has_co2_support) {
        int co2_pct = (int)(((float)d->co2 / co2_max) * 100);
        if (co2_pct > 100) co2_pct = 100;
        int co2_fill_w = (pb_w * co2_pct) / 100;
        tft_fill_rect(pb_x, y_pb_co2, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (co2_fill_w > 0) {
            tft_fill_rect(pb_x, y_pb_co2, co2_fill_w, UI_PROGRESS_BAR_H, d->color_co2);
        }
    }

    /* VOC */
    tft_draw_string("VOC:", UI_LEFT_MARGIN, y_voc, d->color_voc, TFT_BG_COLOR, 2);
    tft_fill_rect(val_x_basic, y_voc, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%6.1f", d->voc_idx_f);
    tft_draw_string(buf, val_x_basic, y_voc, d->color_voc, TFT_BG_COLOR, 2);

    /* ===== PROGRESS BARS ===== */

    /* PM1.0 Progress Bar (SEN68 only) */
    if (g_has_pm1_support) {
        int pm1_pct = (int)((d->pm1_ugm3 / PM25_MAX_SCALE) * 100);
        if (pm1_pct > 100) pm1_pct = 100;
        int pm1_fill_w = (pb_w * pm1_pct) / 100;
        tft_fill_rect(pb_x, y_pb_pm1, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (pm1_fill_w > 0) {
            tft_fill_rect(pb_x, y_pb_pm1, pm1_fill_w, UI_PROGRESS_BAR_H, d->color_pm1);
        }
    }

    /* PM2.5 Progress Bar */
    {
        int pm25_pct = (int)((d->pm2_5_ugm3 / PM25_MAX_SCALE) * 100);
        if (pm25_pct > 100) pm25_pct = 100;
        int pm25_fill_w = (pb_w * pm25_pct) / 100;
        tft_fill_rect(pb_x, y_pb_pm25, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (pm25_fill_w > 0) {
            tft_fill_rect(pb_x, y_pb_pm25, pm25_fill_w, UI_PROGRESS_BAR_H, d->color_pm25);
        }
    }

    /* HCHO or CO2 Progress Bar */
    if (g_has_hcho_support) {
        int hcho_pct = (int)(((float)d->hcho_ppb / hcho_max) * 100);
        if (hcho_pct > 100) hcho_pct = 100;
        int hcho_fill_w = (pb_w * hcho_pct) / 100;
        tft_fill_rect(pb_x, y_pb_hcho, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (hcho_fill_w > 0) {
            tft_fill_rect(pb_x, y_pb_hcho, hcho_fill_w, UI_PROGRESS_BAR_H, d->color_hcho);
        }
    } else if (g_has_co2_support) {
        int co2_pct = (int)(((float)d->co2 / co2_max) * 100);
        if (co2_pct > 100) co2_pct = 100;
        int co2_fill_w = (pb_w * co2_pct) / 100;
        tft_fill_rect(pb_x, y_pb_co2, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (co2_fill_w > 0) {
            tft_fill_rect(pb_x, y_pb_co2, co2_fill_w, UI_PROGRESS_BAR_H, d->color_co2);
        }
    }

    /* VOC Progress Bar */
    {
        int voc_pct = (int)((d->voc_idx_f / VOC_MAX_SCALE) * 100);
        if (voc_pct > 100) voc_pct = 100;
        int voc_fill_w = (pb_w * voc_pct) / 100;
        tft_fill_rect(pb_x, y_pb_voc, pb_w, UI_PROGRESS_BAR_H, TFT_DARKGRAY);
        if (voc_fill_w > 0) {
            tft_fill_rect(pb_x, y_pb_voc, voc_fill_w, UI_PROGRESS_BAR_H, d->color_voc);
        }
    }

    /* Status Bar - AQ State */
    if (s_last_alert_state != (int)d->aq_state) {
        const char *aq_text;
        uint16_t aq_bg;
        switch (d->aq_state) {
            case 0:
                aq_text = "*  GOOD   *";
                aq_bg = TFT_GREEN;
                break;
            case 1:
                aq_text = "* MODERATE *";
                aq_bg = TFT_YELLOW;
                break;
            case 2:
                aq_text = "* UNHEALTHY *";
                aq_bg = TFT_ORANGE;
                break;
            case 3:
                aq_text = "* VERY UNHEALTHY *";
                aq_bg = TFT_RED;
                break;
            case 4:
                aq_text = "* HAZARDOUS *";
                aq_bg = TFT_MAGENTA;
                break;
            default:
                aq_text = "* UNKNOWN *";
                aq_bg = TFT_DARKGRAY;
                break;
        }
        tft_fill_rect(0, status_y, screen_w, UI_STATUS_BAR_HEIGHT, aq_bg);
        int text_w = strlen(aq_text) * UI_CHAR_W;
        tft_draw_string(aq_text, (screen_w - text_w) / 2, status_y + 5,
                       TFT_WHITE, aq_bg, 2);
        s_last_alert_state = (int)d->aq_state;
    }
}

/* ============================================ */
/* DUAL I2C SENSOR COMPARISON TABLE             */
/* ============================================ */
void ui_draw_compare_table(const ui_dual_i2c_data_t *d)
{
    char buf[64];
    int screen_w = tft_get_width();
    int screen_h = tft_get_height();

    int col1_x = 6;
    int col2_x = 75;
    int col3_x = 162;
    int row_gap = 24;

    int y_hdr  = UI_HEADER_HEIGHT + 8;
    int y_temp = y_hdr + 34;
    int y_hum  = y_temp + row_gap;
    int y_pm1  = y_hum + row_gap;
    int y_pm25 = y_pm1 + row_gap;
    int y_pm10 = y_pm25 + row_gap;
    int y_hcho = y_pm10 + row_gap;
    int y_tvoc = y_hcho + row_gap;
    int y_nox  = y_tvoc + row_gap;
    int y_sep  = y_nox + row_gap;

    if (s_first_draw) {
        tft_fill_rect(0, UI_HEADER_HEIGHT, screen_w,
                      screen_h - UI_HEADER_HEIGHT - UI_STATUS_BAR_HEIGHT, TFT_BG_COLOR);

        tft_draw_string("AM2020", col2_x, y_hdr + 2, TFT_CYAN, TFT_BG_COLOR, 2);
        tft_draw_string("SEN68", col3_x + 4, y_hdr + 2, TFT_CYAN, TFT_BG_COLOR, 2);
        tft_fill_rect(2, y_hdr + 22, screen_w - 4, 2, TFT_YELLOW);

        tft_draw_string("Temp", col1_x, y_temp, TFT_WHITE, TFT_BG_COLOR, 2);
        tft_draw_string("Hum", col1_x, y_hum, TFT_WHITE, TFT_BG_COLOR, 2);
        tft_draw_string("PM1.0", col1_x, y_pm1, TFT_WHITE, TFT_BG_COLOR, 2);
        tft_draw_string("PM2.5", col1_x, y_pm25, TFT_WHITE, TFT_BG_COLOR, 2);
        tft_draw_string("PM10", col1_x, y_pm10, TFT_WHITE, TFT_BG_COLOR, 2);
        tft_draw_string("HCHO", col1_x, y_hcho, TFT_WHITE, TFT_BG_COLOR, 2);
        tft_draw_string("TVOC", col1_x, y_tvoc, TFT_WHITE, TFT_BG_COLOR, 2);
        tft_draw_string("NOx", col1_x, y_nox, TFT_WHITE, TFT_BG_COLOR, 2);

        tft_fill_rect(2, y_sep, screen_w - 4, 2, TFT_YELLOW);
    }

    /* --- shared comparison rows --- */
    tft_fill_rect(col2_x, y_temp, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->a_temp);
    tft_draw_string(buf, col2_x, y_temp, d->color_temp, TFT_BG_COLOR, 2);
    tft_fill_rect(col3_x, y_temp, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->s_temp);
    tft_draw_string(buf, col3_x, y_temp, d->color_temp, TFT_BG_COLOR, 2);

    tft_fill_rect(col2_x, y_hum, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->a_humidity);
    tft_draw_string(buf, col2_x, y_hum, d->color_humid, TFT_BG_COLOR, 2);
    tft_fill_rect(col3_x, y_hum, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->s_humidity);
    tft_draw_string(buf, col3_x, y_hum, d->color_humid, TFT_BG_COLOR, 2);

    tft_fill_rect(col2_x, y_pm1, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->a_pm1);
    tft_draw_string(buf, col2_x, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);
    tft_fill_rect(col3_x, y_pm1, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->s_pm1);
    tft_draw_string(buf, col3_x, y_pm1, d->color_pm1, TFT_BG_COLOR, 2);

    tft_fill_rect(col2_x, y_pm25, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->a_pm25);
    tft_draw_string(buf, col2_x, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);
    tft_fill_rect(col3_x, y_pm25, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->s_pm25);
    tft_draw_string(buf, col3_x, y_pm25, d->color_pm25, TFT_BG_COLOR, 2);

    tft_fill_rect(col2_x, y_pm10, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->a_pm10);
    tft_draw_string(buf, col2_x, y_pm10, d->color_pm10, TFT_BG_COLOR, 2);
    tft_fill_rect(col3_x, y_pm10, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->s_pm10);
    tft_draw_string(buf, col3_x, y_pm10, d->color_pm10, TFT_BG_COLOR, 2);

    tft_fill_rect(col2_x, y_hcho, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", (float)d->a_hcho);
    tft_draw_string(buf, col2_x, y_hcho, d->color_hcho, TFT_BG_COLOR, 2);
    tft_fill_rect(col3_x, y_hcho, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->s_hcho);
    tft_draw_string(buf, col3_x, y_hcho, d->color_hcho, TFT_BG_COLOR, 2);

    tft_fill_rect(col2_x, y_tvoc, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", (float)d->a_tvoc);
    tft_draw_string(buf, col2_x, y_tvoc, d->color_tvoc, TFT_BG_COLOR, 2);
    tft_fill_rect(col3_x, y_tvoc, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->s_tvoc);
    tft_draw_string(buf, col3_x, y_tvoc, d->color_tvoc, TFT_BG_COLOR, 2);

    tft_fill_rect(col2_x, y_nox, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", (float)d->a_no2);
    tft_draw_string(buf, col2_x, y_nox, d->color_nox, TFT_BG_COLOR, 2);
    tft_fill_rect(col3_x, y_nox, 6 * UI_CHAR_W, UI_CHAR_H, TFT_BG_COLOR);
    snprintf(buf, sizeof(buf), "%5.1f", d->s_nox);
    tft_draw_string(buf, col3_x, y_nox, d->color_nox, TFT_BG_COLOR, 2);

    /* Status Bar */
    int status_y = screen_h - UI_STATUS_BAR_HEIGHT;
    if (s_last_alert_state != d->global_level) {
        switch (d->global_level) {
            case 0:
                tft_fill_rect(0, status_y, screen_w, UI_STATUS_BAR_HEIGHT, TFT_BLUE);
                tft_draw_string("* NORMAL *", (screen_w - 10 * UI_CHAR_W) / 2 + 2, status_y + 5,
                               TFT_WHITE, TFT_BLUE, 2);
                break;
            case 1:
                tft_fill_rect(0, status_y, screen_w, UI_STATUS_BAR_HEIGHT, TFT_ORANGE);
                tft_draw_string("* WARNING *", (screen_w - 11 * UI_CHAR_W) / 2 + 2, status_y + 5,
                               TFT_WHITE, TFT_ORANGE, 2);
                break;
            case 2:
                tft_fill_rect(0, status_y, screen_w, UI_STATUS_BAR_HEIGHT, TFT_RED);
                tft_draw_string("* DANGER !", (screen_w - 11 * UI_CHAR_W) / 2 + 2, status_y + 5,
                               TFT_WHITE, TFT_RED, 2);
                break;
            default:
                break;
        }
        s_last_alert_state = d->global_level;
    }

    s_first_draw = false;
}