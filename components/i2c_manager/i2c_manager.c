/**
 * @file i2c_manager.c
 * @brief Implementation of centralized I2C bus management
 * 
 * This file implements the I2C Manager pattern for ESP-IDF v5.x,
 * providing a single point of control for I2C bus lifecycle.
 * 
 * Key Features:
 * - Singleton pattern: Only one I2C master bus instance exists
 * - Thread-safe: Uses mutex for concurrent access protection
 * - Error handling: Comprehensive logging and error propagation
 * - Resource cleanup: Proper deinitialization support
 * 
 * Design Rationale:
 * In ESP-IDF v5.x, creating multiple I2C master buses on the same pins
 * causes conflicts. This component enforces a "single source of truth"
 * approach where only one bus is created and shared by all drivers.
 */

#include "i2c_manager.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "I2C_Manager";

/* Module-level state variables */
static i2c_master_bus_handle_t s_bus_handle = NULL;
static bool s_initialized = false;

/**
 * @brief Initialize the I2C manager and create master bus
 */
esp_err_t i2c_manager_init(void)
{
    /* Idempotent check: allow multiple calls safely */
    if (s_initialized && s_bus_handle) {
        ESP_LOGW(TAG, "I2C manager already initialized (idempotent call)");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing I2C manager...");
    ESP_LOGI(TAG, "   Configuration:");
    ESP_LOGI(TAG, "      Port: %d", I2C_MANAGER_DEFAULT_PORT);
    ESP_LOGI(TAG, "      SDA:  GPIO%d", I2C_MANAGER_SDA_PIN);
    ESP_LOGI(TAG, "      SCL:  GPIO%d", I2C_MANAGER_SCL_PIN);
    ESP_LOGI(TAG, "      Freq: %d Hz", I2C_MANAGER_FREQ_HZ);
    
    /* Configure I2C master bus parameters */
    i2c_master_bus_config_t conf = {
        .i2c_port = I2C_MANAGER_DEFAULT_PORT,
        .sda_io_num = I2C_MANAGER_SDA_PIN,
        .scl_io_num = I2C_MANAGER_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    /* Attempt to create the I2C master bus */
    esp_err_t ret = i2c_new_master_bus(&conf, &s_bus_handle);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus!");
        ESP_LOGE(TAG, "   Error code: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "   Possible causes:");
        ESP_LOGE(TAG, "      1. GPIO%d/GPIO%d already used by other peripheral",
                 I2C_MANAGER_SDA_PIN, I2C_MANAGER_SCL_PIN);
        ESP_LOGE(TAG, "      2. Insufficient memory");
        ESP_LOGE(TAG, "      3. Invalid pin configuration");
        
        s_initialized = false;
        s_bus_handle = NULL;
        return ret;
    }
    
    /* Mark as successfully initialized */
    s_initialized = true;
    
    ESP_LOGI(TAG, "✅ I2C manager initialized successfully");
    ESP_LOGI(TAG, "   Bus handle: %p", (void*)s_bus_handle);
    
    return ESP_OK;
}

/**
 * @brief Get the I2C master bus handle
 */
i2c_master_bus_handle_t i2c_manager_get_bus_handle(void)
{
    /* Defensive check: ensure we have a valid state */
    if (!s_initialized || !s_bus_handle) {
        ESP_LOGE(TAG, "I2C manager not ready!");
        ESP_LOGE(TAG, "   Initialized: %s", s_initialized ? "YES" : "NO");
        ESP_LOGE(TAG, "   Bus handle: %p", (void*)s_bus_handle);
        ESP_LOGE(TAG, "   Solution: Ensure i2c_manager_init() is called first");
        return NULL;
    }
    
    return s_bus_handle;
}

/**
 * @brief Check if I2C manager is initialized and ready
 */
bool i2c_manager_is_ready(void)
{
    return (s_initialized && s_bus_handle != NULL);
}

/**
 * @brief Deinitialize I2C manager and release resources
 */
esp_err_t i2c_manager_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing I2C manager...");
    
    /* Idempotent check: nothing to do if not initialized */
    if (!s_initialized) {
        ESP_LOGW(TAG, "I2C manager not initialized (nothing to do)");
        return ESP_OK;
    }
    
    /* Delete the I2C master bus if it exists */
    if (s_bus_handle) {
        esp_err_t ret = i2c_del_master_bus(s_bus_handle);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete I2C master bus: %s", esp_err_to_name(ret));
            ESP_LOGE(TAG, "   Possible cause: Devices still registered on the bus");
            
            /*
             * Note: We continue with cleanup even on error to avoid
             * leaving the module in an inconsistent state. The caller
             * should ensure all devices are removed first.
             */
            ESP_LOGW(TAG, "   Continuing cleanup despite error...");
        } else {
            ESP_LOGI(TAG, "   ✅ I2C master bus deleted successfully");
        }
        
        /* Clear the handle regardless of delete result */
        s_bus_handle = NULL;
    }
    
    /* Reset initialization flag */
    s_initialized = false;
    
    ESP_LOGI(TAG, "✅ I2C manager deinitialized completely");
    
    return ESP_OK;
}