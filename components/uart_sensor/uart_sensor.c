/*
 * UART Sensor Driver Implementation
 *
 * Frame format (44 bytes):
 *   [0x2D] [0x23] [len_lo] [len_hi] [38 bytes data] [cs_lo] [cs_hi]
 *
 * Parsing strategy:
 *   1. Sync to frame header (0x2D 0x23)
 *   2. Read length field (should be 38 = 19 × 2)
 *   3. Read 38 bytes of data + 2 bytes checksum
 *   4. Validate checksum (sum of header + len + data bytes)
 *   5. Populate uart_sensor_data_t fields (little-endian)
 */

#include "uart_sensor.h"
#include <string.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "uart_sensor";

/* UART receive buffer */
#define UART_BUF_SIZE    (UART_MAX_FRAME * 2)

/* ============================================ */
/* INITIALIZATION                               */
/* ============================================ */

bool uart_sensor_init(void)
{
    ESP_LOGI(TAG, "Initializing UART sensor on UART%d (TX=%d, RX=%d, %d baud)...",
             UART_SENSOR_PORT, UART_SENSOR_TX, UART_SENSOR_RX, UART_SENSOR_BAUD);

    uart_config_t uart_cfg = {
        .baud_rate = UART_SENSOR_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(UART_SENSOR_PORT,
                                         UART_BUF_SIZE, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(err));
        return false;
    }

    err = uart_param_config(UART_SENSOR_PORT, &uart_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(err));
        return false;
    }

    err = uart_set_pin(UART_SENSOR_PORT,
                       UART_SENSOR_TX, UART_SENSOR_RX,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "UART sensor initialized successfully");
    return true;
}

/* ============================================ */
/* FRAME PARSING                                */
/* ============================================ */

/**
 * @brief Calculate 16-bit checksum: sum of all bytes from header through data
 */
static uint16_t calc_checksum(const uint8_t *buf, uint16_t byte_count)
{
    uint16_t sum = 0;
    for (uint16_t i = 0; i < byte_count; i++) {
        sum += buf[i];
    }
    return sum;
}

/**
 * @brief Parse little-endian uint16_t from buffer
 */
static inline uint16_t read_u16_le(const uint8_t *buf)
{
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

/**
 * @brief Populate data structure from raw frame buffer
 */
static void parse_frame(const uint8_t *buf, uart_sensor_data_t *data)
{
    const uint8_t *p = buf + UART_HEADER_LEN + UART_LEN_LEN; /* skip header + len */

    data->pms_in_pm1_0    = read_u16_le(p +  0);
    data->pms_in_pm2_5    = read_u16_le(p +  2);
    data->pms_in_pm10     = read_u16_le(p +  4);
    data->pms_out_pm1_0   = read_u16_le(p +  6);
    data->pms_out_pm2_5   = read_u16_le(p +  8);
    data->pms_out_pm10    = read_u16_le(p + 10);
    data->temperature     = read_u16_le(p + 12);
    data->humidity        = read_u16_le(p + 14);
    data->tvoc_count      = read_u16_le(p + 16);
    data->aq_state        = read_u16_le(p + 18);
    data->audio_state     = read_u16_le(p + 20);
    data->pir_state       = read_u16_le(p + 22);
    data->co2_count       = read_u16_le(p + 24);
    data->light_state     = read_u16_le(p + 26);
    data->pressure_count  = read_u16_le(p + 28);
    data->reserved1       = read_u16_le(p + 30);
    data->reserved2       = read_u16_le(p + 32);
    data->reserved3       = read_u16_le(p + 34);
    data->ave_state_sum   = read_u16_le(p + 36);
}

static esp_err_t read_one_frame(uint8_t *buf, uart_sensor_data_t *out)
{
    /* Sync to frame header (0x2D 0x23) */
    int state = 0;
    uint8_t ch;

    while (1) {
        int n = uart_read_bytes(UART_SENSOR_PORT, &ch, 1, pdMS_TO_TICKS(50));
        if (n <= 0) return ESP_ERR_TIMEOUT;

        if (state == 0 && ch == UART_HEAD_LO) state = 1;
        else if (state == 1) state = (ch == UART_HEAD_HI) ? 2 : (ch == UART_HEAD_LO ? 1 : 0);
        if (state == 2) break;
    }

    buf[0] = UART_HEAD_LO;
    buf[1] = UART_HEAD_HI;

    /* Read length field (2 bytes LE) */
    int n = uart_read_bytes(UART_SENSOR_PORT, buf + 2, 2, pdMS_TO_TICKS(100));
    if (n < 2) return ESP_ERR_TIMEOUT;

    uint16_t frame_len = read_u16_le(buf + 2);
    if (frame_len < 8 || frame_len > UART_MAX_FRAME) return ESP_FAIL;

    /* Read remaining: data + checksum */
    int remain = frame_len - UART_HEADER_LEN - UART_LEN_LEN;
    n = uart_read_bytes(UART_SENSOR_PORT, buf + 4, remain, pdMS_TO_TICKS(200));
    if (n < remain) return ESP_ERR_TIMEOUT;

    /* Validate checksum: sum of header + len + data */
    uint16_t cs_recv = read_u16_le(buf + frame_len - UART_CS_LEN);
    uint16_t cs_calc = calc_checksum(buf, frame_len - UART_CS_LEN);
    if (cs_calc != cs_recv) return ESP_FAIL;

    /* Parse fields from buf + 4 (after header + len) */
    parse_frame(buf, out);

    return ESP_OK;
}

bool uart_sensor_read(uart_sensor_data_t *data)
{
    if (data == NULL) {
        return false;
    }

    uint8_t buf[UART_MAX_FRAME];
    bool got_one = false;
    uart_sensor_data_t last;

    /* Drain buffer: keep only the last valid frame */
    while (1) {
        uart_sensor_data_t tmp;
        if (read_one_frame(buf, &tmp) == ESP_OK) {
            last = tmp;
            got_one = true;
        } else {
            break;
        }
    }

    if (!got_one) return false;

    *data = last;

    ESP_LOGI(TAG, "PM_in: %u/%u/%u  PM_out: %u/%u/%u",
             data->pms_in_pm1_0, data->pms_in_pm2_5, data->pms_in_pm10,
             data->pms_out_pm1_0, data->pms_out_pm2_5, data->pms_out_pm10);
    ESP_LOGI(TAG, "Temp=%u  Hum=%u  TVOC=%u  AQ=%u  CO2=%u  Pres=%u",
             data->temperature, data->humidity,
             data->tvoc_count, data->aq_state,
             data->co2_count, data->pressure_count);

    return true;
}
