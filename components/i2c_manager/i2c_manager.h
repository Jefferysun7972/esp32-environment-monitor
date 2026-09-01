/**
 * @file i2c_manager.h
 * @brief Centralized I2C bus management for ESP-IDF v5.x multi-device sharing
 * 
 * This component provides a clean API for managing shared I2C buses,
 * decoupling individual device drivers from bus initialization details.
 * 
 * Architecture Pattern: Service Locator / Dependency Injection
 * 
 * Design Principles:
 * - Single Responsibility: Only manages I2C bus lifecycle
 * - Open/Closed Principle: New devices can use the manager without modification
 * - Dependency Inversion: Device drivers depend on abstraction (this interface)
 * - Interface Segregation: Small, focused API surface
 * 
 * Usage Example:
 * @code
 * // In app_main() or system startup:
 * esp_err_t ret = i2c_manager_init();
 * if (ret != ESP_OK) {
 *     // Handle error
 * }
 * 
 * // In any device driver:
 * i2c_master_bus_handle_t bus = i2c_manager_get_bus_handle();
 * if (bus) {
 *     // Use bus to add device
 * }
 * @endcode
 */

#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

/* Default I2C configuration for this project */
#define I2C_MANAGER_DEFAULT_PORT    I2C_NUM_0
#define I2C_MANAGER_SDA_PIN         21
#define I2C_MANAGER_SCL_PIN         22
#define I2C_MANAGER_FREQ_HZ         100000

/**
 * @brief Initialize the I2C manager and create master bus
 * 
 * This function should be called once during system startup,
 * before any device driver initialization.
 * 
 * It creates a single I2C master bus instance that can be shared
 * by multiple device drivers through i2c_manager_get_bus_handle().
 * 
 * @return 
 * - ESP_OK: Successfully initialized
 * - ESP_ERR_INVALID_STATE: Already initialized (idempotent, returns OK)
 * - ESP_FAIL: Hardware initialization failed
 * 
 * @note This function is idempotent - calling it multiple times is safe
 * @note Must be called before any device driver that uses I2C
 * 
 * @code
 * // Correct usage in app_main():
 * void app_main(void) {
 *     // Step 1: Initialize I2C infrastructure FIRST
 *     esp_err_t ret = i2c_manager_init();
 *     assert(ret == ESP_OK);
 *     
 *     // Step 2: Now initialize device drivers (they will get bus from manager)
 *     sensirion_i2c_hal_init();  // SEN66 sensor
 *     oled_init();                // OLED display
 *     // ... more devices can be added here
 * }
 * @endcode
 */
esp_err_t i2c_manager_init(void);

/**
 * @brief Get the I2C master bus handle
 * 
 * Device drivers call this to obtain the shared bus handle
 * instead of creating their own. This ensures all devices
 * share the same physical I2C bus without conflicts.
 * 
 * @return 
 * - Valid i2c_master_bus_handle_t if initialized and ready
 * - NULL if not initialized or initialization failed
 * 
 * @warning Always check return value for NULL before using!
 * 
 * @code
 * // In device driver initialization:
 * static esp_err_t my_device_init(void) {
 *     i2c_master_bus_handle_t bus = i2c_manager_get_bus_handle();
 *     
 *     if (!bus) {
 *         ESP_LOGE(TAG, "I2C Manager not ready!");
 *         return ESP_ERR_NOT_FOUND;
 *     }
 *     
 *     // Add this device to the shared bus
 *     i2c_device_config_t dev_cfg = {...};
 *     return i2c_master_bus_add_device(bus, &dev_cfg, &my_dev_handle);
 * }
 * @endcode
 */
i2c_master_bus_handle_t i2c_manager_get_bus_handle(void);

/**
 * @brief Check if I2C manager is initialized and ready
 * 
 * Useful for defensive programming and debugging.
 * 
 * @return true if manager is initialized and has valid bus handle, false otherwise
 * 
 * @code
 * // Before performing critical I2C operations:
 * if (!i2c_manager_is_ready()) {
 *     ESP_LOGW(TAG, "I2C system not ready - deferring operation");
 *     return;
 * }
 * @endcode
 */
bool i2c_manager_is_ready(void);

/**
 * @brief Deinitialize I2C manager and release resources
 * 
 * Call during system shutdown or when I2C is no longer needed.
 * After calling this, all devices must be re-initialized if
 * I2C is needed again.
 * 
 * @return ESP_OK on success, error code otherwise
 * 
 * @note This will invalidate all previously obtained bus handles
 * @note Typically called in system shutdown handlers only
 * 
 * @code
 * // In shutdown handler:
 * void system_shutdown(void) {
 *     // Clean up devices first
 *     my_device_deinit();
 *     oled_deinit();
 *     
 *     // Then release I2C resources
 *     i2c_manager_deinit();
 * }
 * @endcode
 */
esp_err_t i2c_manager_deinit(void);

/**
 * @brief Scan I2C bus for connected devices
 *
 * Probes all 7-bit addresses from 0x08 to 0x77 using i2c_master_probe().
 * Logs found devices and returns the count.
 *
 * @return Number of devices found on the bus, or -1 if bus is not ready
 *
 * @note Must be called after i2c_manager_init() and before any device
 *       handles are added to the bus.
 */
int i2c_manager_scan(void);

#endif /* I2C_MANAGER_H */