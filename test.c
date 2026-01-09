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

#include "pico/stdlib.h"
#include <stdio.h>

#include "ssd1351.h"
#include "character_data.h"
#include "am2320.h"
#include "ens160.h"

int main()
{
    stdio_init_all();
    ssd1351_init();
    am2320_init();
    ens160_init();

    uint16_t colour = 0x0000U;
    uint16_t* framebuffer = ssd1351_get_framebuffer();
    char buffer[64];

    float temperature = 0.0f;
    float humidity = 0.0f;
    uint16_t part_id = 0;
    ens160_read_part_id(&part_id);

    uint8_t current_ens160_mode = 0;
    uint8_t current_ens160_config = 0;

    uint16_t eco2 = 0;
    uint16_t tvoc = 0;
    uint8_t aqi = 0;

    struct ENS160_status_s status = {0};
    ens160_write_mode(ENS160_STANDARD_MODE);

    while (true)
    {
        ssd1351_fill_screen(colour);
        // write_string_at(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 0, 0, 0xFFFF, colour, framebuffer, SSD1351_WIDTH, SSD1351_HEIGHT);

        am2320_read_temperature_humidity(&temperature, &humidity);
        sprintf(buffer, "Temp: %.1f C\nHumidity: %.1f%%\n", temperature, humidity);
        write_string_at(buffer, 0, 0, 0xFFFF, colour, ssd1351_get_framebuffer(), SSD1351_WIDTH, SSD1351_HEIGHT);

        
        ens160_read_eco2(&eco2);
        ens160_read_tvoc(&tvoc);
        ens160_read_air_quality_index(&aqi);
        sprintf(buffer, "eCO2: %4d ppm\nTVOC: %4d ppb\nAQI: %d\n", eco2, tvoc, aqi);
        write_string_at(buffer, 0, 64, 0xFFFF, colour, ssd1351_get_framebuffer(), SSD1351_WIDTH, SSD1351_HEIGHT);
        
        ssd1351_update();

        // Update ENS160 with latest temperature and humidity
        ens160_write_temperature_humidity(temperature, humidity);
        
        colour+=32;
        sleep_ms(1000);
    }
}
