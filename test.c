#include "pico/stdlib.h"
#include <stdio.h>

#include "ssd1351.h"
#include "character_data.h"
#include "am2320.h"


int main()
{
    stdio_init_all();
    ssd1351_init();
    am2320_init();

    uint16_t colour = 0x0000U;
    uint16_t* framebuffer = ssd1351_get_framebuffer();
    char buffer[64];

    float temperature = 0.0f;
    float humidity = 0.0f;
    while (true)
    {
        ssd1351_fill_screen(colour);
        // write_string_at(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 0, 0, 0xFFFF, colour, framebuffer, SSD1351_WIDTH, SSD1351_HEIGHT);

        am2320_read_temperature_humidity(&temperature, &humidity);

        sprintf(buffer, "Temp: %.1f C\nHumidity: %.1f%%\n", temperature, humidity);
        write_string_at(buffer, 0, 0, 0xFFFF, colour, ssd1351_get_framebuffer(), SSD1351_WIDTH, SSD1351_HEIGHT);
        ssd1351_update();


        colour+=32;
        //printf("Colour = %04X\n", colour);
        sleep_ms(1000);
    }
}
