#include "pico/stdlib.h"
#include <stdio.h>

#include "ssd1351.h"
#include "character_data.h"



int main()
{
    stdio_init_all();
    ssd1351_init();

    uint16_t colour = 0x0000U;
    uint16_t* framebuffer = ssd1351_get_framebuffer();

    while (true)
    {
        ssd1351_fill_screen(colour);
        write_string_at(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 0, 0, 0xFFFF, colour, framebuffer, SSD1351_WIDTH, SSD1351_HEIGHT);
        ssd1351_update();

        colour+=32;
        //printf("Colour = %04X\n", colour);
        sleep_ms(200);
    }
}
