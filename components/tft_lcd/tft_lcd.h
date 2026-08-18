#ifndef TFT_LCD_H
#define TFT_LCD_H
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/* Screen dimensions */
#define TFT_WIDTH       240
#define TFT_HEIGHT      320

/* SPI GPIO pins - based on J1 interface */
#define TFT_SPI_CLK     18  /* J1-3: SPI Clock (VSPI SCK) */
#define TFT_SPI_MOSI    23  /* J1-4: SPI MOSI (VSPI MOSI) */
#define TFT_SPI_CS      5   /* J1-9: LCD Chip Select */
#define TFT_DC          4   /* J1-6: Data/Command select */
#define TFT_RST         16  /* J1-5: Reset pin */
#define TFT_BLK         17  /* J1-7: Backlight (-1 = always on) */

/* Color definitions (RGB565 format) */
#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_ORANGE      0xFD20
#define TFT_GRAY        0x8410
#define TFT_LIGHTGRAY   0xD69A
#define TFT_DARKGRAY    0x2945

/* Custom colors for sensor display */
#define TFT_TEMP_COLOR  0xF800  /* Red for temperature */
#define TFT_HUMID_COLOR 0x07FF  /* Cyan for humidity */
#define TFT_PM25_COLOR  0xFFE0  /* Yellow for PM2.5 */
#define TFT_CO2_COLOR   0x07E0  /* Green for CO2 */
#define TFT_VOC_COLOR   0xF81F  /* Magenta for VOC */
#define TFT_BG_COLOR    0x0000  /* Black background */
#define TFT_TEXT_COLOR  0xFFFF  /* White text */

/* Font size (6x8 or 8x16) */
#define FONT_WIDTH      6
#define FONT_HEIGHT     8

/**
 * @brief Initialize TFT-LCD with ILI9341 driver
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t tft_init(void);

/**
 * @brief Fill entire screen with a color
 * 
 * @param color RGB565 color value
 */
void tft_fill_screen(uint16_t color);

/**
 * @brief Draw a single pixel at (x, y)
 * 
 * @param x X coordinate (0 to TFT_WIDTH-1)
 * @param y Y coordinate (0 to TFT_HEIGHT-1)
 * @param color RGB565 color value
 */
void tft_draw_pixel(int x, int y, uint16_t color);

/**
 * @brief Draw a horizontal line
 * 
 * @param x Starting X coordinate
 * @param y Y coordinate
 * @param w Line width in pixels
 * @param color RGB565 color value
 */
void tft_draw_hline(int x, int y, int w, uint16_t color);

/**
 * @brief Draw a vertical line
 * 
 * @param x X coordinate
 * @param y Starting Y coordinate
 * @param h Line height in pixels
 * @param color RGB565 color value
 */
void tft_draw_vline(int x, int y, int h, uint16_t color);

/**
 * @brief Draw a rectangle (outlined)
 * 
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param w Width in pixels
 * @param h Height in pixels
 * @param color RGB565 color value
 */
void tft_draw_rect(int x, int y, int w, int h, uint16_t color);

/**
 * @brief Fill a rectangle (solid)
 * 
 * @param x Top-left X coordinate
 * @param y Top-left Y coordinate
 * @param w Width in pixels
 * @param h Height in pixels
 * @param color RGB565 color value
 */
void tft_fill_rect(int x, int y, int w, int h, uint16_t color);

/**
 * @brief Display a single character at current cursor position
 * 
 * @param c Character to display
 * @param color Text color (RGB565)
 * @param bg_color Background color (RGB565)
 * @param size Font size multiplier (1=6x8, 2=12x16, etc.)
 */
void tft_draw_char(char c, uint16_t color, uint16_t bg_color, uint8_t size);

/**
 * @brief Display a string at specified position
 * 
 * @param str String to display
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Text color (RGB565)
 * @param bg_color Background color (RGB565)
 * @param size Font size multiplier
 */
void tft_draw_string(const char *str, int x, int y, uint16_t color, uint16_t bg_color, uint8_t size);

/**
 * @brief Set text cursor position
 * 
 * @param x X coordinate
 * @param y Y coordinate
 */
void tft_set_cursor(int x, int y);

/**
 * @brief Get current cursor X position
 * 
 * @return Current X position
 */
int tft_get_cursor_x(void);

/**
 * @brief Get current cursor Y position
 * 
 * @return Current Y position
 */
int tft_get_cursor_y(void);

/**
 * @brief Set screen rotation (orientation)
 * 
 * @param rotation: 0=portrait, 1=landscape, 2=reverse portrait, 3=reverse landscape
 */
void tft_set_rotation(uint8_t rotation);

/**
 * @brief Convert RGB888 to RGB565 color
 * 
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 * @return RGB565 color value
 */
uint16_t tft_color565(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Get current screen width (affected by rotation)
 * 
 * @return Screen width in pixels
 */
int tft_get_width(void);

/**
 * @brief Get current screen height (affected by rotation)
 * 
 * @return Screen height in pixels
 */
int tft_get_height(void);

#endif /* TFT_LCD_H */