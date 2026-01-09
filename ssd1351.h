/*
 *   Copyright (c) 2026 Rob McKay
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#ifndef SSD1351_H
#define SSD1351_H

#include <stdint.h>

#define SSD1351_WIDTH 128
#define SSD1351_HEIGHT 128



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

uint16_t* ssd1351_get_framebuffer(void);

#endif