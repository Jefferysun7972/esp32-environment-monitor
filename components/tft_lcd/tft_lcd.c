#include "tft_lcd.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* ============================================ */
/* DIAGNOSTIC MODE SELECTION                   */
/* Set to 1 to enable, 0 to disable            */
/* ============================================ */
#define DIAG_SPI_MODE_0_TEST     0   /* Test: Try SPI Mode 0 instead of Mode 3 */
#define DIAG_GPIO_BITBANG_TEST   0   /* Test: Manual GPIO bit-banging (slow but reliable) */
#define DIAG_HW_RESET_TEST       0   /* Test: Extended hardware reset sequence */
/* ============================================ */

static const char *TAG = "TFT_LCD";

/* SPI handle */
static spi_device_handle_t spi = NULL;

/* Cursor position */
static int cursor_x = 0;
static int cursor_y = 0;

/* Current screen dimensions (affected by rotation) */
static int screen_width = TFT_WIDTH;
static int screen_height = TFT_HEIGHT;

/* Font data: TomThumb 5x7 (6x8 with spacing) - column-oriented format */
static const uint8_t font_6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, /* space 0x20 */
    {0x00,0x00,0x5F,0x00,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, /* $ */
    {0x23,0x13,0x08,0x64,0x62,0x00}, /* % */
    {0x36,0x49,0x55,0x22,0x50,0x00}, /* & */
    {0x00,0x05,0x03,0x00,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00,0x00}, /* ) */
    {0x14,0x08,0x3E,0x08,0x14,0x00}, /* * */
    {0x08,0x08,0x3E,0x08,0x08,0x00}, /* + */
    {0x00,0x50,0x30,0x00,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08,0x00}, /* - */
    {0x00,0x60,0x60,0x00,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02,0x00}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, /* 0 0x30 */
    {0x00,0x42,0x7F,0x40,0x00,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46,0x00}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31,0x00}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10,0x00}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39,0x00}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03,0x00}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36,0x00}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E,0x00}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00,0x00}, /* ; */
    {0x08,0x14,0x22,0x41,0x00,0x00}, /* < */
    {0x14,0x14,0x14,0x14,0x14,0x00}, /* = */
    {0x41,0x22,0x14,0x08,0x00,0x00}, /* > */
    {0x02,0x01,0x51,0x09,0x06,0x00}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E,0x00}, /* @ 0x40 */
    {0x7E,0x11,0x11,0x11,0x7E,0x00}, /* A */
    {0x7F,0x49,0x49,0x49,0x36,0x00}, /* B */
    {0x3E,0x41,0x41,0x41,0x22,0x00}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, /* D */
    {0x7F,0x49,0x49,0x49,0x41,0x00}, /* E */
    {0x7F,0x09,0x09,0x09,0x01,0x00}, /* F */
    {0x3E,0x41,0x49,0x49,0x7A,0x00}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, /* H */
    {0x00,0x41,0x7F,0x41,0x00,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01,0x00}, /* J */
    {0x7F,0x08,0x14,0x22,0x41,0x00}, /* K */
    {0x7F,0x40,0x40,0x40,0x40,0x00}, /* L */
    {0x7F,0x02,0x0C,0x02,0x7F,0x00}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, /* O */
    {0x7F,0x09,0x09,0x09,0x06,0x00}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46,0x00}, /* R */
    {0x46,0x49,0x49,0x49,0x31,0x00}, /* S */
    {0x01,0x01,0x7F,0x01,0x01,0x00}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F,0x00}, /* W */
    {0x63,0x14,0x08,0x14,0x63,0x00}, /* X */
    {0x07,0x08,0x70,0x08,0x07,0x00}, /* Y */
    {0x61,0x51,0x49,0x45,0x43,0x00}, /* Z */
    {0x00,0x7F,0x41,0x41,0x00,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20,0x00}, /* backslash */
    {0x00,0x41,0x41,0x7F,0x00,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04,0x00}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40,0x00}, /* _ */
    {0x00,0x01,0x02,0x04,0x00,0x00}, /* ` */
    {0x20,0x54,0x54,0x54,0x78,0x00}, /* a 0x60 */
    {0x7F,0x48,0x44,0x44,0x38,0x00}, /* b */
    {0x38,0x44,0x44,0x44,0x20,0x00}, /* c */
    {0x38,0x44,0x44,0x48,0x7F,0x00}, /* d */
    {0x38,0x54,0x54,0x54,0x18,0x00}, /* e */
    {0x08,0x7E,0x09,0x01,0x02,0x00}, /* f */
    {0x0C,0x52,0x52,0x52,0x3E,0x00}, /* g */
    {0x7F,0x08,0x04,0x04,0x78,0x00}, /* h */
    {0x00,0x44,0x7D,0x40,0x00,0x00}, /* i */
    {0x20,0x40,0x44,0x3D,0x00,0x00}, /* j */
    {0x7F,0x10,0x28,0x44,0x00,0x00}, /* k */
    {0x00,0x41,0x7F,0x40,0x00,0x00}, /* l */
    {0x7C,0x04,0x18,0x04,0x78,0x00}, /* m */
    {0x7C,0x08,0x04,0x04,0x78,0x00}, /* n */
    {0x38,0x44,0x44,0x44,0x38,0x00}, /* o */
    {0x7C,0x14,0x14,0x14,0x08,0x00}, /* p */
    {0x08,0x14,0x14,0x18,0x7C,0x00}, /* q */
    {0x7C,0x08,0x04,0x04,0x08,0x00}, /* r */
    {0x48,0x54,0x54,0x54,0x20,0x00}, /* s */
    {0x04,0x3F,0x44,0x40,0x20,0x00}, /* t */
    {0x3C,0x40,0x40,0x20,0x7C,0x00}, /* u */
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, /* v */
    {0x3C,0x40,0x30,0x40,0x3C,0x00}, /* w */
    {0x44,0x28,0x10,0x28,0x44,0x00}, /* x */
    {0x0C,0x50,0x50,0x50,0x3C,0x00}, /* y */
    {0x44,0x64,0x54,0x4C,0x44,0x00}, /* z */
    {0x00,0x08,0x36,0x41,0x00,0x00}, /* { */
    {0x00,0x00,0x7F,0x00,0x00,0x00}, /* | */
    {0x00,0x41,0x36,0x08,0x00,0x00}, /* } */
    {0x08,0x08,0x2A,0x1C,0x08,0x00}, /* ~ */
};

/**
 * @brief Send command to ILI9341
 */
static void ili9341_command(uint8_t cmd) {
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
        .rx_buffer = NULL,
    };
    gpio_set_level(TFT_DC, 0);  /* Command mode */
    spi_device_transmit(spi, &t);
}

/**
 * @brief Send data to ILI9341
 */
static void ili9341_data(uint8_t data) {
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .rx_buffer = NULL,
    };
    gpio_set_level(TFT_DC, 1);  /* Data mode */
    spi_device_transmit(spi, &t);
}

/**
 * @brief Send multiple bytes of data to ILI9341
 */
static void ili9341_data_bulk(const uint8_t *data, size_t len) {
    if (len == 0) return;
    
    /* CRITICAL: Always ensure DC is HIGH for data transfers */
    gpio_set_level(TFT_DC, 1);  /* Force Data mode - no check, just set! */
    
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
        .rx_buffer = NULL,
    };
    spi_device_transmit(spi, &t);  /* Ignore errors for performance */
}

/**
 * @brief Initialize ILI9341 display controller
 */
static esp_err_t ili9341_init(void) {
    ESP_LOGI(TAG, "Initializing ILI9341...");
    
    /* Hardware reset */
#if DIAG_HW_RESET_TEST
    /* Extended hardware reset sequence */
    ESP_LOGW(TAG, "DIAG: Extended hardware reset sequence");
    gpio_set_level(TFT_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));  /* 更长的低电平脉冲 (原10ms) */
    gpio_set_level(TFT_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(200)); /* 更长的等待时间 (原120ms) */
#else
    /* Standard hardware reset */
    gpio_set_level(TFT_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TFT_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
#endif
    
    /* Send initialization commands */
    ili9341_command(0xCB);  /* Power control A */
    ili9341_data(0x39);
    ili9341_data(0x2C);
    ili9341_data(0x00);
    ili9341_data(0x34);
    ili9341_data(0x04);
    
    ili9341_command(0xCF);  /* Power control B */
    ili9341_data(0x00);
    ili9341_data(0xC1);
    ili9341_data(0x30);
    
    ili9341_command(0xE8);  /* Driver timing control A */
    ili9341_data(0x85);
    ili9341_data(0x00);
    ili9341_data(0x78);
    
    ili9341_command(0xEA);  /* Driver timing control B */
    ili9341_data(0x00);
    ili9341_data(0x00);
    
    ili9341_command(0xED);  /* Power on sequence control */
    ili9341_data(0x64);
    ili9341_data(0x03);
    ili9341_data(0x12);
    ili9341_data(0x81);
    
    ili9341_command(0xF7);  /* Pump ratio control */
    ili9341_data(0x20);
    
    ili9341_command(0xC0);  /* Power control 1 */
    ili9341_data(0x23);
    
    ili9341_command(0xC1);  /* Power control 2 */
    ili9341_data(0x10);
    
    ili9341_command(0xC5);  /* VCOM control 1 */
    ili9341_data(0x3E);
    ili9341_data(0x28);
    
    ili9341_command(0xC7);  /* VCOM control 2 */
    ili9341_data(0x86);
    
    ili9341_command(0x36);  /* Memory Access Control */
    ili9341_data(0x48);     /* MX=1, MY=1, BGR=1 (landscape mode) */
    
    ili9341_command(0x3A);  /* Pixel Format Set */
    ili9341_data(0x55);     /* 16-bit color (RGB565) */
    
    ili9341_command(0xB1);  /* Frame Rate Control */
    ili9341_data(0x00);
    ili9341_data(0x18);
    
    ili9341_command(0xB6);  /* Display Function Control */
    ili9341_data(0x08);
    ili9341_data(0x82);
    ili9341_data(0x27);
    
    ili9341_command(0xF2);  /* Enable 3 gamma control */
    ili9341_data(0x00);
    
    ili9341_command(0x26);  /* Gamma Set */
    ili9341_data(0x01);
    
    /* Positive Gamma Correction */
    ili9341_command(0xE0);
    ili9341_data(0x0F);
    ili9341_data(0x31);
    ili9341_data(0x2B);
    ili9341_data(0x0C);
    ili9341_data(0x0E);
    ili9341_data(0x08);
    ili9341_data(0x4E);
    ili9341_data(0xF1);
    ili9341_data(0x37);
    ili9341_data(0x07);
    ili9341_data(0x10);
    ili9341_data(0x03);
    ili9341_data(0x0E);
    ili9341_data(0x09);
    ili9341_data(0x00);
    
    /* Negative Gamma Correction */
    ili9341_command(0xE1);
    ili9341_data(0x00);
    ili9341_data(0x0E);
    ili9341_data(0x14);
    ili9341_data(0x03);
    ili9341_data(0x11);
    ili9341_data(0x07);
    ili9341_data(0x31);
    ili9341_data(0xC1);
    ili9341_data(0x48);
    ili9341_data(0x08);
    ili9341_data(0x0F);
    ili9341_data(0x0C);
    ili9341_data(0x31);
    ili9341_data(0x36);
    ili9341_data(0x0F);
    
    ili9341_command(0x11);  /* Exit Sleep */
    vTaskDelay(pdMS_TO_TICKS(120));
    
    ili9341_command(0x29);  /* Display On */
    
    ESP_LOGI(TAG, "ILI9341 initialized successfully!");
    return ESP_OK;
}

/**
 * @brief Set display window for pixel writing
 */
static void set_address_window(int x0, int y0, int x1, int y1) {
    ili9341_command(0x2A);  // Column Address Set
    ili9341_data(x0 >> 8);
    ili9341_data(x0 & 0xFF);
    ili9341_data(x1 >> 8);
    ili9341_data(x1 & 0xFF);
    
    ili9341_command(0x2B);  // Page Address Set
    ili9341_data(y0 >> 8);
    ili9341_data(y0 & 0xFF);
    ili9341_data(y1 >> 8);
    ili9341_data(y1 & 0xFF);
    
    ili9341_command(0x2C);  // Memory Write
    
    /* CRITICAL: Ensure DC stays HIGH for upcoming data transfers */
    gpio_set_level(TFT_DC, 1);  // Force Data mode ← 新增！
    // vTaskDelay(pdMS_TO_TICKS(1));  // Small delay to ensure stable ← 新增！
}

/* Public API Implementation */

esp_err_t tft_init(void) {
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing TFT-LCD...");
    // ✅ 新增代码：在 gpio_config 中包含 BLK 引脚
    uint64_t pin_mask = (1ULL << TFT_DC) | (1ULL << TFT_RST);

    #if (TFT_BLK >= 0)
    pin_mask |= (1ULL << TFT_BLK);  /* 添加背光引脚 */
    #endif

    /* Configure GPIO pins */
    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    /* Initialize DC and RST pins */
    gpio_set_level(TFT_DC, 1);
    gpio_set_level(TFT_RST, 1);

    // ✅ 新增代码：初始化后开启背光
    #if (TFT_BLK >= 0)
    gpio_set_level(TFT_BLK, 1);  /* 开启背光 */
    ESP_LOGI(TAG, "Backlight enabled on GPIO%d", TFT_BLK);
    #endif

    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Initialize DC and RST pins */
    gpio_set_level(TFT_DC, 1);
    gpio_set_level(TFT_RST, 1);
    
    /* Configure SPI bus */
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,  /* Not used (no touch) */
        .mosi_io_num = TFT_SPI_MOSI,
        .sclk_io_num = TFT_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TFT_WIDTH * TFT_HEIGHT * 2 + 8,
    };
    
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    #if DIAG_SPI_MODE_0_TEST
    ESP_LOGW(TAG, "DIAG: Using SPI MODE 0 (testing compatibility)");
    #endif
    /* Add device to SPI bus */
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20 * 1000 * 1000,  /* 20 MHz (safe speed) */
        .mode = 0,                           /* ← SPI Mode 0 (标准) */  
        .spics_io_num = TFT_SPI_CS,           /* CS pin */
        .queue_size = 7,
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .flags = 0,
    };
    
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Initialize ILI9341 */
    ret = ili9341_init();
    if (ret != ESP_OK) {
        return ret;
    }
    
    /* Clear screen to black */
    tft_fill_screen(TFT_RED);
    
    ESP_LOGI(TAG, "TFT-LCD initialized successfully!");
    ESP_LOGI(TAG, "Screen size: %dx%d", screen_width, screen_height);
    
    return ESP_OK;
}

void tft_fill_screen(uint16_t color) {
    ESP_LOGI(TAG, "fill_screen: 0x%04X @ %dx%d", color, screen_width, screen_height);
    
    set_address_window(0, 0, screen_width - 1, screen_height - 1);
    
    /* Prepare color data buffer */
    uint16_t *color_buf = heap_caps_malloc(screen_width * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!color_buf) {
        ESP_LOGE(TAG, "Buffer alloc failed!");
        return;
    }
    
    /* Use little-endian format (no byte swap) */
    for (int i = 0; i < screen_width; i++) {
        color_buf[i] = color;  /* Native format, no swap */
    }
    
    /* Send all lines efficiently with watchdog feeding */
    for (int y = 0; y < screen_height; y++) {
        ili9341_data_bulk((uint8_t *)color_buf, screen_width * sizeof(uint16_t));
        
        /* Feed watchdog every 10 lines to prevent timeout */
        if (y % 10 == 0) {
            taskYIELD();  /* Let other tasks run */
        }
    }
    
    free(color_buf);
    ESP_LOGI(TAG, "fill_screen done");
}

void tft_draw_pixel(int x, int y, uint16_t color) {
    if ((x < 0) || (x >= screen_width) || (y < 0) || (y >= screen_height)) {
        return;
    }
    
    set_address_window(x, y, x, y);
    
    uint16_t color_be = __builtin_bswap16(color);
    ili9341_data_bulk((uint8_t *)&color_be, 2);  // 正确！使用 bulk 函数
}

void tft_draw_hline(int x, int y, int w, uint16_t color) {
    if ((x < 0) || (x >= screen_width) || (y < 0) || (y >= screen_height)) {
        return;
    }
    
    /* Clamp width to screen boundary */
    if (x + w >= screen_width) {
        w = screen_width - x;
    }
    if (w <= 0) return;  // 新增：边界检查
    
    set_address_window(x, y, x + w - 1, y);
    
    uint16_t *color_buf = heap_caps_malloc(w * sizeof(uint16_t), MALLOC_CAP_DMA);  // 堆分配
    if (!color_buf) return;  // 新增：NULL检查
    
    for (int i = 0; i < w; i++) {
        color_buf[i] = __builtin_bswap16(color);
    }
    
    ili9341_data_bulk((uint8_t *)color_buf, w * sizeof(uint16_t));  // 正确的大小
    free(color_buf);  // 新增：释放内存
}

void tft_draw_vline(int x, int y, int h, uint16_t color) {
    if ((x < 0) || (x >= screen_width) || (y < 0) || (y >= screen_height)) {
        return;
    }
    
    /* Clamp height to screen boundary */
    if (y + h >= screen_height) {
        h = screen_height - y;
    }
    
    set_address_window(x, y, x, y + h - 1);
    
    uint16_t color_be = __builtin_bswap16(color);
    for (int i = 0; i < h; i++) {
        ili9341_data_bulk((uint8_t *)&color_be, 2);  // 正确！使用 bulk 函数
    }
}

void tft_draw_rect(int x, int y, int w, int h, uint16_t color) {
    tft_draw_hline(x, y, w, color);
    tft_draw_hline(x, y + h - 1, w, color);
    tft_draw_vline(x, y, h, color);
    tft_draw_vline(x + w - 1, y, h, color);
}

void tft_fill_rect(int x, int y, int w, int h, uint16_t color) {
    if ((x < 0) || (x >= screen_width) || (y < 0) || (y >= screen_height)) {
        return;
    }
    
    /* Clamp dimensions to screen boundary */
    if (x + w >= screen_width) w = screen_width - x;
    if (y + h >= screen_height) h = screen_height - y;
    if (w <= 0 || h <= 0) return;  // 新增：边界验证
    
    set_address_window(x, y, x + w - 1, y + h - 1);
    
    uint16_t *color_buf = heap_caps_malloc(w * sizeof(uint16_t), MALLOC_CAP_DMA);  // 堆分配
    if (!color_buf) return;  // 新增：NULL检查
    
    for (int i = 0; i < w; i++) {
        color_buf[i] = __builtin_bswap16(color);
    }
    
    for (int j = 0; j < h; j++) {
        ili9341_data_bulk((uint8_t *)color_buf, w * sizeof(uint16_t));  // 正确大小
    }
    free(color_buf);  // 新增：释放内存
}

void tft_draw_char(char c, uint16_t color, uint16_t bg_color, uint8_t size) {
    /* Handle special characters */
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT * size;
        return;
    } else if (c == '\r') {
        cursor_x = 0;
        return;
    }
    
    /* Check character range */
    int char_index = (int)c - 0x20;  /* Space starts at 0x20 */
    if (char_index < 0 || char_index >= sizeof(font_6x8) / sizeof(font_6x8[0])) {
        char_index = 0;  /* Use space for unknown characters */
    }
    
    const uint8_t *char_data = font_6x8[char_index];
    
    /* Draw character pixels */
    for (int col = 0; col < FONT_WIDTH; col++) {
        uint8_t col_data = char_data[col];
        for (int row = 0; row < FONT_HEIGHT; row++) {
            if (col_data & (0x01 << row)) {
                /* Foreground pixel */
                if (size == 1) {
                    tft_draw_pixel(cursor_x + col, cursor_y + row, color);
                } else {
                    tft_fill_rect(
                        cursor_x + col * size,
                        cursor_y + row * size,
                        size, size, color
                    );
                }
            } else if (bg_color != color) {
                /* Background pixel (only if different from foreground) */
                if (size == 1) {
                    tft_draw_pixel(cursor_x + col, cursor_y + row, bg_color);
                } else {
                    tft_fill_rect(
                        cursor_x + col * size,
                        cursor_y + row * size,
                        size, size, bg_color
                    );
                }
            }
        }
    }
    
    /* Advance cursor */
    cursor_x += FONT_WIDTH * size;
    
    /* Wrap to next line if needed */
    if (cursor_x >= screen_width) {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT * size;
    }
}

void tft_draw_string(const char *str, int x, int y, uint16_t color, uint16_t bg_color, uint8_t size) {
    tft_set_cursor(x, y);
    
    while (*str) {
        tft_draw_char(*str++, color, bg_color, size);
    }
}

void tft_set_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}

int tft_get_cursor_x(void) {
    return cursor_x;
}

int tft_get_cursor_y(void) {
    return cursor_y;
}
void tft_set_rotation(uint8_t rotation) {
    uint8_t madctl = 0;
    
    switch (rotation) {
        case 0:
            /* Portrait mode */
            madctl = 0x48;  /* MX=1, MY=1, MV=0, ML=0, RGB=0, MH=0, BGR=1 */
            screen_width = TFT_WIDTH;
            screen_height = TFT_HEIGHT;
            break;
        case 1:
            /* Landscape mode */
            madctl = 0x28;  /* MX=1, MY=0, MV=1, ML=0, RGB=0, MH=0, BGR=1 */
            screen_width = TFT_HEIGHT;
            screen_height = TFT_WIDTH;
            break;
        case 2:
            /* Reverse portrait */
            madctl = 0x88;  /* MX=0, MY=0, MV=0, ML=1, RGB=0, MH=0, BGR=1 */
            screen_width = TFT_WIDTH;
            screen_height = TFT_HEIGHT;
            break;
        case 3:
            /* Reverse landscape */
            madctl = 0xE8;  /* MX=0, MY=1, MV=1, ML=1, RGB=0, MH=0, BGR=1 */
            screen_width = TFT_HEIGHT;
            screen_height = TFT_WIDTH;
            break;
        default:
            ESP_LOGW(TAG, "Invalid rotation value: %d", rotation);
            return;
    }
    
    ili9341_command(0x36);  /* Memory Access Control */
    ili9341_data(madctl);
    
    ESP_LOGI(TAG, "Rotation set to %d, screen size: %dx%d", rotation, screen_width, screen_height);
}

int tft_get_width(void) {
    return screen_width;
}

int tft_get_height(void) {
    return screen_height;
}

uint16_t tft_color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


