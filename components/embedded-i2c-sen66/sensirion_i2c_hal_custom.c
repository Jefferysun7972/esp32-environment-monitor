/*
 * Custom I2C HAL implementation for SEN66 sensor
 * Using ESP-IDF native I2C driver with hardcoded GPIO pins
 */

#include "sensirion_i2c_hal.h"
#include "sensirion_config.h"
#include "sen66_i2c.h"    /* ← 添加这一行！提供 SEN66_I2C_ADDR_6B 宏 */
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "i2c_manager.h"

static const char* TAG = "sen66_i2c_hal";

/* Hardcoded I2C configuration - same as in blink_example_main.c */
#define I2C_MASTER_SCL_IO       22      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO       21      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM          0       /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ      100000  /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS   1000    /*!< I2C timeout in milliseconds */

static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t dev_handle = NULL;
static uint8_t s_current_device_addr = 0; 

/**
 * Select the current i2c bus by index.
 *
 * @param bus_idx   Bus index to select
 * @returns         0 on success, an error code otherwise
 */
int16_t sensirion_i2c_hal_select_bus(uint8_t bus_idx) {
    (void)bus_idx;
    return 0; /* Not implemented but not needed for single bus */
}

#if 0
/**
 * Initialize all hard- and software components that are needed for the I2C
 * communication.
 */
void sensirion_i2c_hal_init(void) {
    ESP_LOGI(TAG, "Initializing custom I2C HAL using I2C Manager...");
    
    /*
     * IMPORTANT: Use centralized I2C manager instead of creating our own bus!
     * The main program should have already called i2c_manager_init().
     */
    bus_handle = i2c_manager_get_bus_handle();
    
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get I2C bus handle from I2C Manager!");
        ESP_LOGE(TAG, "Make sure i2c_manager_init() is called before this function.");
        return;
    }
    
    ESP_LOGI(TAG, "✅ Obtained I2C bus handle from I2C Manager");
    
    /* ===== 新增：I2C 设备扫描功能（修正版）===== */
    ESP_LOGI(TAG, "🔍 Scanning I2C bus for connected devices...");
    
    uint8_t found_devices = 0;
    esp_err_t err;
    
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        
        /* 创建临时设备句柄用于探测 */
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        };
        
        i2c_master_dev_handle_t temp_dev_handle = NULL;
        err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &temp_dev_handle);
        
        if (err == ESP_OK) {
            /*
             * 使用读取操作来探测设备是否存在（发送一个字节并检查 ACK/NACK）
             * 方法：尝试读取 1 字节数据
             * - 如果设备存在：返回 ESP_OK 或 ESP_ERR_TIMEOUT
             * - 如果设备不存在：返回 ESP_FAIL 或其他错误
             */
            uint8_t dummy_data[1] = {0};
            esp_err_t probe_result = i2c_master_receive(temp_dev_handle, dummy_data, 1, 20);  /* 20ms 超时 */
            
            if (probe_result == ESP_OK || probe_result == ESP_ERR_TIMEOUT) {
                /*
                 * 收到响应（即使是超时），说明该地址有设备应答了
                 * 注意：ESP_ERR_TIMEOUT 表示设备存在但没有足够时间完成传输，
                 *       这在快速扫描中是正常的
                 */
                found_devices++;
                
                const char* device_name = "Unknown";
                
                if (addr == SEN66_I2C_ADDR_6B) {
                    device_name = "✅ SEN66 Environmental Sensor";
                } else if (addr == 0x23) {
                    device_name = "Possible: SHT3x/SHT31 Temp/Humidity";
                } else if (addr == 0x40 || addr == 0x41) {
                    device_name = "Possible: HTU21D/Si7021 Humidity";
                } else if (addr == 0x48 || addr == 0x49) {
                    device_name = "Possible: TMP102/ADS1115 ADC";
                } else if (addr == 0x50) {
                    device_name = "Possible: AT24Cxx EEPROM";
                } else if (addr == 0x68) {
                    device_name = "Possible: DS1307 RTC / MPU6050 IMU";
                } else if (addr == 0x76 || addr == 0x77) {
                    device_name = "Possible: BMP280/BME280 Pressure";
                } else if (addr == 0x3C || addr == 0x3D) {
                    device_name = "Possible: SSD1306 OLED Display";
                }
                
                ESP_LOGI(TAG, "   📍 Found device at address: 0x%02X (%3d decimal) | %s", 
                         addr, addr, device_name);
            }
            
            /* 清理临时设备句柄 */
            i2c_master_bus_rm_device(temp_dev_handle);
        }
    }
    
    if (found_devices == 0) {
        ESP_LOGW(TAG, "⚠️  No I2C devices detected on the bus!");
        ESP_LOGW(TAG, "   Please check:");
        ESP_LOGW(TAG, "      1. Wiring connections (SDA, SCL, VCC, GND)");
        ESP_LOGW(TAG, "      2. Pull-up resistors (most dev boards have built-in)");
        ESP_LOGW(TAG, "      3. Power supply (stable 3.3V)");
        ESP_LOGW(TAG, "      4. Sensor is powered on and not damaged");
    } else {
        ESP_LOGI(TAG, "🎉 I2C scan complete! Found %d device(s) on the bus.", found_devices);
    }
    /* ===== 扫描结束 ===== */
    /* Reset I2C bus after scan to clear any stuck state */
    ESP_LOGI(TAG, "Resetting I2C bus after scan...");
    i2c_master_bus_reset(bus_handle);
    ESP_LOGI(TAG, "I2C bus reset complete");

    /* ===== 预创建 SEN66 设备句柄（防止 ESP_ERR_INVALID_STATE）==== */
    if (!dev_handle && bus_handle) {
        i2c_device_config_t sen66_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = SEN66_I2C_ADDR_6B,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        };
        
        esp_err_t sen66_err = i2c_master_bus_add_device(bus_handle, &sen66_cfg, &dev_handle);
        if (sen66_err == ESP_OK) {
            ESP_LOGI(TAG, "✅ Pre-created SEN66 device handle (address: 0x%02X)", SEN66_I2C_ADDR_6B);
        } else {
            ESP_LOGE(TAG, "Failed to pre-create SEN66 device handle: %s", esp_err_to_name(sen66_err));
        }
    }
    /* ===== 句柄预创建结束 ===== */
}   // ← sensirion_i2c_hal_init() 函数结束的大括号
#endif

void sensirion_i2c_hal_init(void) {
    ESP_LOGI(TAG, "Initializing custom I2C HAL using I2C Manager...");
    
    bus_handle = i2c_manager_get_bus_handle();
    
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get I2C bus handle from I2C Manager!");
        ESP_LOGE(TAG, "Make sure i2c_manager_init() is called before this function.");
        return;
    }
    
    ESP_LOGI(TAG, "✅ Obtained I2C bus handle from I2C Manager");

    /*
     * NOTE: Do NOT call i2c_master_bus_reset() here. The bus was just created
     * by i2c_new_master_bus() in i2c_manager_init(), which already initializes
     * the hardware controller correctly via i2c_hal_master_init().
     *
     * Calling i2c_master_bus_reset() triggers s_i2c_master_clear_bus() which
     * temporarily hijacks SDA/SCL as manual GPIO outputs. When the GPIO config
     * is restored, the I2C controller's internal state is not properly
     * re-synchronized, causing the first i2c_master_transmit() to fail with
     * ESP_ERR_INVALID_STATE (status never reaches I2C_STATUS_DONE).
     */

    if (dev_handle == NULL) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = SEN66_I2C_ADDR_6B,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        };
        esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(err));
            return;
        }
        s_current_device_addr = SEN66_I2C_ADDR_6B;
        ESP_LOGI(TAG, "✅ Pre-created SEN66 device handle (address: 0x%02X)", SEN66_I2C_ADDR_6B);
    }
}
/**
 * Release all resources initialized by sensirion_i2c_hal_init().
 */
void sensirion_i2c_hal_free(void) {
    if (dev_handle) {
        i2c_master_bus_rm_device(dev_handle);
        dev_handle = NULL;
    }
    
    if (bus_handle) {
        i2c_del_master_bus(bus_handle);
        bus_handle = NULL;
    }
    
    ESP_LOGI(TAG, "I2C resources released");
}

/**
 * Execute one read transaction on the I2C bus, reading a given number of bytes.
 *
 * @param address 7-bit I2C address to read from
 * @param data    pointer to the buffer where the data is to be stored
 * @param count   number of bytes to read from I2C and store in the buffer
 * @returns 0 on success, error code otherwise
 */
int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t* data, uint8_t count) {
    /* Create device handle if not exists */
    if (!dev_handle || address != s_current_device_addr) {
        if (dev_handle) {
            i2c_master_bus_rm_device(dev_handle);
            dev_handle = NULL;
        }
        if (bus_handle) {
            i2c_device_config_t dev_cfg = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = address,
                .scl_speed_hz = I2C_MASTER_FREQ_HZ,
            };
            esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(err));
                return -1;
            }
            s_current_device_addr = address;
        }
    }
    
    /* Read data from I2C device */
    esp_err_t ret = i2c_master_receive(dev_handle, data, count, I2C_MASTER_TIMEOUT_MS);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
        return -1;
    }
    
    return 0;
}

/**
 * Execute one write transaction on the I2C bus, sending a given number of
 * bytes.
 *
 * @param address 7-bit I2C address to write to
 * @param data    pointer to the buffer containing the data to write
 * @param count   number of bytes to read from the buffer and send over I2C
 * @returns 0 on success, error code otherwise
 */
int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t* data, uint8_t count) {
    /* Create device handle if not exists */
    if (!dev_handle || address != s_current_device_addr) {
        if (dev_handle) {
            i2c_master_bus_rm_device(dev_handle);
            dev_handle = NULL;
        }
        if (bus_handle) {
            i2c_device_config_t dev_cfg = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = address,
                .scl_speed_hz = I2C_MASTER_FREQ_HZ,
            };
            esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(err));
                return -1;
            }
            s_current_device_addr = address;
        }
    }
    
    /* Write data to I2C device */
    esp_err_t ret = i2c_master_transmit(dev_handle, data, count, I2C_MASTER_TIMEOUT_MS);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(ret));
        return -1;
    }
    
    return 0;
}

/**
 * Sleep for a given number of microseconds. The function should delay the
 * execution for at least the given time, but may also sleep longer.
 *
 * Despite the unit, a <10 millisecond precision is sufficient.
 *
 * @param useconds the sleep time in microseconds
 */
void sensirion_i2c_hal_sleep_usec(uint32_t useconds) {
    if (useconds >= 1000) {
        vTaskDelay(pdMS_TO_TICKS(useconds / 1000));
    } else {
        /* For sub-millisecond delays on ESP-IDF v5.x */
        esp_rom_delay_us(useconds);
    }
}
/**
 * @note This function is DEPRECATED!
 * 
 * Use i2c_manager_get_bus_handle() from i2c_manager component instead.
 * This function is kept for backward compatibility only.
 * 
 * @return Pointer to i2c_master_bus_handle_t, or NULL if not initialized
 */
i2c_master_bus_handle_t sensirion_i2c_get_bus_handle(void) {
    ESP_LOGW(TAG, "DEPRECATED: sensirion_i2c_get_bus_handle() - use i2c_manager_get_bus_handle()");
    return i2c_manager_get_bus_handle();
}