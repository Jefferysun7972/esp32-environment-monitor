/*
 * AM2020DY I2C Driver - Header
 *
 * I2C Address: 0x28
 * Protocol: Frame-based command/response with checksum
 *   Write: [0x11][len][cmd][checksum]
 *   Read:  [0x16][len][cmd][data...][checksum]
 *   Checksum: 0x100 - (sum of all preceding bytes & 0xFF)
 */

#ifndef AM2020DY_H
#define AM2020DY_H

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AM2020DY_SLAVE_ADDR               0x28

#define AM2020DY_CMD_READ_MEASUREMENT     0x16
#define AM2020DY_CMD_READ_SW_VERSION      0x1E
#define AM2020DY_CMD_READ_SERIAL          0x1F
#define AM2020DY_CMD_READ_PRODUCT_NAME    0x2E

#define AM2020DY_FRAME_HEAD               0x11

typedef struct {
    uint16_t tvoc;
    uint16_t no2;
    uint16_t pm1_0;
    uint16_t pm2_5;
    uint16_t pm10;
    uint16_t hcho;
    float temperature;
    float humidity;
} am2020dy_data_t;

esp_err_t am2020dy_init(i2c_master_dev_handle_t *out_dev_handle);
esp_err_t am2020dy_read_product_name(i2c_master_dev_handle_t dev_handle);
esp_err_t am2020dy_read_measurement(i2c_master_dev_handle_t dev_handle, am2020dy_data_t *out_data);

#ifdef __cplusplus
}
#endif

#endif /* AM2020DY_H */
