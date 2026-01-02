#ifndef SSD1351_H
#define SSD1351_H

#include <stdint.h>

/**
 * @brief Initialize the SSD1351 OLED display controller.
 * 
 * This function initializes the SSD1351 display controller with the necessary
 * configuration settings including power supply, display mode, color depth, and
 * GPIO pin assignments. It prepares the display hardware for operation.
 * 
 * @note This function should be called once during system initialization before
 *       any other display operations are performed.
 */
void ssd1351_init(void);



// Display control
void ssd1351_display_on(void);
void ssd1351_display_off(void);
void ssd1351_set_brightness(uint8_t brightness);

enum SSD1351_DisplayMode {
    SSD1351_DISPLAY_NORMAL,
    SSD1351_DISPLAY_INVERTED
};

void ssd1351_set_mode(enum SSD1351_DisplayMode mode);


// Drawing primitives
void ssd1351_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void ssd1351_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void ssd1351_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void ssd1351_fill_screen(uint16_t color);

// Buffer management
void ssd1351_clear(void);
void ssd1351_update(void);

#endif