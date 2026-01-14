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

#include "ssd1351.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#define SSD1351_SPI_INSTANCE spi0
#define SSD1351_DC 5
#define SSD1351_RST 4
#define SSD1351_SPI_SCK 18
#define SSD1351_SPI_DATA 19
#define SSD1351_SPI_SPEED 15000000 // 15 MHz
#define SSD1351_SPI_DATABITS 8


// SSD1351 Command Set from pages 32-37 of the datasheet

#define SSD1351_CMD_SETCOLUMN 0x15

#define SSD1351_CMD_SETROW 0x75

#define SSD1351_CMD_WRITE_RAM 0x5C

#define SSD1351_CMD_HORIZONTALSCROLL 0x96
#define SSD1351_CMD_STOPSCROLL 0x9E
#define SSD1351_CMD_STARTSCROLL 0x9F

#define SSD1351_CMD_SETREMAP 0xA0
#define SSD1351_CMD_STARTLINE 0xA1
#define SSD1351_CMD_DISPLAYOFFSET 0xA2
#define SSD1351_CMD_DISPLAYALLOFF 0xA4
#define SSD1351_CMD_DISPLAYALLON 0xA5
#define SSD1351_CMD_DISPLAYNORMAL 0xA6
#define SSD1351_CMD_DISPLAYINVERTED 0xA7
#define SSD1351_CMD_FUNCTIONSEL 0xAB
#define SSD1351_CMD_DISPLAYOFF 0xAE
#define SSD1351_CMD_DISPLAYON 0xAF

#define SSD1351_CMD_ENHANCE 0xB2
#define SSD1351_CMD_CLOCKDIV 0xB3
#define SSD1351_CMD_SETGPIO 0xB5
#define SSD1351_CMD_USELINEARLUT 0xB9

#define SSD1351_CMD_CONTRASTABC 0xC1
#define SSD1351_CMD_CONTRASTMASTER 0xC7
#define SSD1351_CMD_MUXRATIO 0xCA

#define SSD1351_CMD_COMMANDLOCK 0xFD


#define SSD1351_HORIZONTAL_INCREMENT 0x00U
#define SSD1351_VERTICAL_INCREMENT 0x01U
#define SSD1351_NORMAL_COLUMN_ADDRESSING 0x00U
#define SSD1351_REVERSED_COLUMN_ADDRESSING 0x02U
#define SSD1351_COLOR_SEQUENCE_RGB 0x00U
#define SSD1351_COLOR_SEQUENCE_BGR 0x04U
#define SSD1351_SCAN_NORMAL 0x00U
#define SSD1351_SCAN_REVERSED 0x10U
#define SSD1351_DISABLE_COM_SPLIT 0x00U
#define SSD1351_ENABLE_COM_SPLIT 0x20U
#define SSD1351_65K_COLORS 0x40U
#define SSD1351_262K_COLORS 0x80U


#define WRITE_COMMAND 0
#define WRITE_DATA 1


uint16_t screen_buffer[SSD1351_WIDTH * SSD1351_HEIGHT];


/**
 * @brief Write a command and optional data to the SSD1351 display.
 *
 * This function sends a command byte followed by optional data bytes to the
 * SSD1351 display controller over the SPI interface. It handles setting the
 * Data/Command (DC) pin appropriately for command and data transmission.
 *
 * @param cmd  The command byte to send.
 * @param data Pointer to the data bytes to send (can be NULL if no data).
 * @param len  The number of data bytes to send.
 */
static void ssd1351_write(const uint8_t cmd, const uint8_t *data, size_t len)
{
    gpio_put(SSD1351_DC, WRITE_COMMAND);
    spi_write_blocking(SSD1351_SPI_INSTANCE, &cmd, 1);
    if ((data != NULL) && (len > 0))
    {
        gpio_put(SSD1351_DC, WRITE_DATA);
        spi_write_blocking(SSD1351_SPI_INSTANCE, data, len);
    }
}


/**
 * @brief Unlock the SSD1351 display controller.
 *
 * This function sends the unlock commands to the SSD1351 display controller
 * to enable access to protected commands.
 */
static void ssd1351_unlock(void)
{
    uint8_t param[2];

    // Unlock the driver
    param[0] = 0x12;
    ssd1351_write(SSD1351_CMD_COMMANDLOCK, param, 1);

    // Unlock the commands
    param[0] = 0xB1;
    ssd1351_write(SSD1351_CMD_COMMANDLOCK, param, 1);
}



/**
 * @brief Configure the SPI interface for the SSD1351 display.
 *
 * This function initializes the SPI interface with the appropriate settings
 * for communication with the SSD1351 display controller.
 */
static void configure_spi(void)
{
    spi_init(SSD1351_SPI_INSTANCE, SSD1351_SPI_SPEED);
    spi_set_format(SSD1351_SPI_INSTANCE, SSD1351_SPI_DATABITS, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_set_function(SSD1351_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SSD1351_SPI_DATA, GPIO_FUNC_SPI);
}


/**
 * @brief Configure the GPIO pins for the SSD1351 display.
 *
 * This function initializes the Data/Command (DC) and Reset (RST) pins
 * for controlling the SSD1351 display.
 */
static void configure_gpio(void)
{
    // Setup the Data/Command pin
    gpio_init(SSD1351_DC);
    gpio_set_dir(SSD1351_DC, GPIO_OUT);
    gpio_put(SSD1351_DC, WRITE_DATA);

    // Setup the Reset pin
    gpio_init(SSD1351_RST);
    gpio_set_dir(SSD1351_RST, GPIO_OUT);
    gpio_put(SSD1351_RST, 0); // Start with display in reset
}



/**
 * @brief Perform a hardware reset of the SSD1351 display.
 *
 * This function toggles the reset pin of the SSD1351 display to perform a
 * hardware reset. It ensures that the display is properly reset before
 * software initialisation.
 */
static void perform_reset(void)
{
    gpio_put(SSD1351_RST, 1);          
    sleep_ms(10);                      
    gpio_put(SSD1351_RST, 0);            /* Put the display in reset */
    sleep_ms(500);                       /* Give the display time to reset */
    gpio_put(SSD1351_RST, 1);            /* Release the display from reset */
    gpio_put(SSD1351_DC, WRITE_COMMAND); /* Default to command mode */
    sleep_ms(50);                       
}


/**
 * Initialize the SSD1351 OLED display controller.
 */
void ssd1351_init(void)
{
    uint8_t param[4];

    configure_spi();  // Initialize SPI interface
    configure_gpio(); // Initialize GPIO pins
    perform_reset();  // Force a hardware reset

    ssd1351_unlock();

    ssd1351_display_off(); // Ensure display is off during setup

    // Max frequency, no divider. Fastest refresh rate.
    param[0] = 0xF1;
    ssd1351_write(SSD1351_CMD_CLOCKDIV, param, 1);

    // Effectively number of lines
    param[0] = SSD1351_HEIGHT - 1;
    ssd1351_write(SSD1351_CMD_MUXRATIO, param, 1);

    // Horizontal addressing, unmirrored, C->B->A colours, normal scan, 65K colours
    param[0] = SSD1351_HORIZONTAL_INCREMENT | SSD1351_NORMAL_COLUMN_ADDRESSING | SSD1351_COLOR_SEQUENCE_RGB | SSD1351_SCAN_REVERSED | SSD1351_ENABLE_COM_SPLIT | SSD1351_65K_COLORS;
    ssd1351_write(SSD1351_CMD_SETREMAP, param, 1);

    param[0] = 0x00;
    ssd1351_write(SSD1351_CMD_STARTLINE, param, 1);

    param[0] = 0x00;
    ssd1351_write(SSD1351_CMD_DISPLAYOFFSET, param, 1);

    // Disable GPIO
    //param[0] = 0x00;
    //ssd1351_write(SSD1351_CMD_SETGPIO, param, 1);

    // 8bit interface
    param[0] = 0x01;
    ssd1351_write(SSD1351_CMD_FUNCTIONSEL, param, 1);

    ssd1351_set_mode(SSD1351_DISPLAY_NORMAL);

    // R G B contrast
    param[0] = param[1] = param[2] = 0xFF;
    ssd1351_write(SSD1351_CMD_CONTRASTABC, param, 3);
    // Max master contrast
    ssd1351_set_brightness(0x0F);

    // Enable enhancement
    param[0] = 0xA4;
    param[1] = 0x00;
    param[2] = 0x00;
    ssd1351_write(SSD1351_CMD_ENHANCE, param, 3);

    ssd1351_display_on();
    ssd1351_clear();
}



void ssd1351_display_on(void)
{
    ssd1351_write(SSD1351_CMD_DISPLAYON, NULL, 0);
}



void ssd1351_display_off(void)
{
    ssd1351_write(SSD1351_CMD_DISPLAYOFF, NULL, 0);
}



void ssd1351_set_brightness(uint8_t brightness)
{
    ssd1351_write(SSD1351_CMD_CONTRASTMASTER, &brightness, 1);
}



void ssd1351_set_mode(enum SSD1351_DisplayMode mode)
{
    uint8_t cmd;
    switch (mode)
    {
    case SSD1351_DISPLAY_NORMAL:
        cmd = SSD1351_CMD_DISPLAYNORMAL;
        break;

    case SSD1351_DISPLAY_INVERTED:
        cmd = SSD1351_CMD_DISPLAYINVERTED;
        break;

    default:
        cmd = SSD1351_CMD_DISPLAYNORMAL;
        break;
    }
    ssd1351_write(cmd, NULL, 0);
}



static void set_column_address(uint8_t start, uint8_t end)
{
    uint8_t data[2];
    data[0] = start;
    data[1] = end;
    ssd1351_write(SSD1351_CMD_SETCOLUMN, data, 2);
}



static void set_row_address(uint8_t start, uint8_t end)
{
    uint8_t data[2];
    data[0] = start;
    data[1] = end;
    ssd1351_write(SSD1351_CMD_SETROW, data, 2);
}




void ssd1351_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    uint8_t data[4];

    // Set column address
    set_column_address(x, x);

    // Set row address
    set_row_address(y, y);

    // Write pixel data
    data[0] = (color >> 8) & 0xFF; // High byte
    data[1] = color & 0xFF;        // Low byte
    ssd1351_write(SSD1351_CMD_WRITE_RAM, data, 2);
}


void ssd1351_fill_screen(uint16_t color)
{
    const uint32_t total_pixels = SSD1351_WIDTH * SSD1351_HEIGHT;

    // Prepare pixel data
    for (uint32_t i = 0; i < total_pixels; i++)
    {
        screen_buffer[i] = color;
    }
}



void ssd1351_clear(void)
{
    ssd1351_fill_screen(0x0000); // Fill screen with black
}


void ssd1351_update(void)
{
    // Set column address
    set_column_address(0, SSD1351_WIDTH - 1);
    
    // Set row address
    set_row_address(0, SSD1351_HEIGHT - 1);
    
    ssd1351_write(SSD1351_CMD_WRITE_RAM, NULL, 0);
    gpio_put(SSD1351_DC, WRITE_DATA);
    spi_write_blocking(SSD1351_SPI_INSTANCE, (uint8_t*)screen_buffer, SSD1351_WIDTH * SSD1351_HEIGHT * 2);
}


uint16_t *ssd1351_get_framebuffer(void)
{
    return screen_buffer;
}
