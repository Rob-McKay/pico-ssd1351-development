#include "pico/stdlib.h"
#include <stdio.h>

#include "ssd1351.h"


int main()
{
    stdio_init_all();
    ssd1351_init();

    uint16_t colour = 0x0000U;
    while (true)
    {
        ssd1351_fill_screen(colour);
        colour++;
        printf("Colour = %04X\n", colour);
        sleep_ms(1000);
    }
}
