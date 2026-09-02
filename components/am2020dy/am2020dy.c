/*
 * AM2020DY I2C Driver Implementation
 *
 * Uses centralized I2C manager for bus access.
 * Protocol: Cubic AM2020DY V0.5 specification
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "i2c_manager.h"
#include "am2020dy.h"

static const char *TAG = "am2020dy";

static uint8_t am2020dy_checksum(const uint8_t *data, size_t len)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)(0x100 - (sum & 0xFF));
}

static esp_err_t am2020dy_send_command(i2c_master_dev_handle_t dev_handle, uint8_t cmd)
{
    uint8_t tx_buf[4];
    tx_buf[0] = AM2020DY_FRAME_HEAD;
    tx_buf[1] = 0x01;
    tx_buf[2] = cmd;
    tx_buf[3] = am2020dy_checksum(tx_buf, 3);

    return i2c_master_transmit(dev_handle, tx_buf, sizeof(tx_buf), 100);
}

esp_err_t am2020dy_init(i2c_master_dev_handle_t *out_dev_handle)
{
    i2c_master_bus_handle_t bus_handle = i2c_manager_get_bus_handle();
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get I2C bus handle");
        return ESP_FAIL;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AM2020DY_SLAVE_ADDR,
        .scl_speed_hz = 100000,
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, out_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add AM2020DY device: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "AM2020DY device handle created (addr: 0x%02X)", AM2020DY_SLAVE_ADDR);
    return ESP_OK;
}

esp_err_t am2020dy_read_product_name(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t err = am2020dy_send_command(dev_handle, AM2020DY_CMD_READ_PRODUCT_NAME);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send product name command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(65));

    uint8_t rx_buf[10] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), 100);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read product name: %s", esp_err_to_name(err));
        return err;
    }

    char model[7];
    memcpy(model, &rx_buf[3], 6);
    model[6] = '\0';

    ESP_LOGI(TAG, "Product name: %s", model);
    return ESP_OK;
}

esp_err_t am2020dy_read_measurement(i2c_master_dev_handle_t dev_handle, am2020dy_data_t *out_data)
{
    esp_err_t err = am2020dy_send_command(dev_handle, AM2020DY_CMD_READ_MEASUREMENT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send measurement command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t rx_buf[32] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), 100);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read measurement: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t data_len = rx_buf[1];
    uint8_t cs_pos   = 2 + data_len;

    uint8_t cs_calc = am2020dy_checksum(rx_buf, cs_pos);
    if (cs_calc != rx_buf[cs_pos]) {
        ESP_LOGE(TAG, "Checksum mismatch: calc=0x%02X, recv=0x%02X", cs_calc, rx_buf[cs_pos]);
        return ESP_FAIL;
    }

    uint16_t tvoc     = ((uint16_t)rx_buf[3]  << 8) | rx_buf[4];
    uint16_t no2      = ((uint16_t)rx_buf[5]  << 8) | rx_buf[6];
    uint16_t pm1_0    = ((uint16_t)rx_buf[7]  << 8) | rx_buf[8];
    uint16_t pm2_5    = ((uint16_t)rx_buf[9]  << 8) | rx_buf[10];
    uint16_t pm10     = ((uint16_t)rx_buf[11] << 8) | rx_buf[12];
    int16_t  temp_raw = ((uint16_t)rx_buf[13] << 8) | rx_buf[14];
    uint16_t hum_raw  = ((uint16_t)rx_buf[15] << 8) | rx_buf[16];
    uint16_t hcho     = ((uint16_t)rx_buf[21] << 8) | rx_buf[22];

    float temp = (temp_raw - 500) / 10.0f;
    float hum  = hum_raw / 10.0f;

    if (out_data) {
        out_data->tvoc        = tvoc;
        out_data->no2         = no2;
        out_data->pm1_0       = pm1_0;
        out_data->pm2_5       = pm2_5;
        out_data->pm10        = pm10;
        out_data->hcho        = hcho;
        out_data->temperature = temp;
        out_data->humidity    = hum;
    }
    return ESP_OK;
}