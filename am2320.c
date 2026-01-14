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

#include "am2320.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "character_data.h"
#include "ssd1351.h"

#include "pico/binary_info.h"

#include <math.h>
#include <stdio.h>

#define AM2320_I2C_INSTANCE i2c0
#define AM2320_I2C_SCL 21
#define AM2320_I2C_SDA 20
#define AM2320_I2C_SPEED 100000 // 100 kHz

#define AM2320_I2C_ADDRESS (0x5CU) // 7-bit address

#define AM2320_READ_REG_COMMAND 0x03
#define AM2320_WRITE_REG_COMMAND 0x10

#define AM2320_REG_HUMIDITY_MSB 0x00
#define AM2320_REG_HUMIDITY_LSB 0x01
#define AM2320_REG_TEMPERATURE_MSB 0x02
#define AM2320_REG_TEMPERATURE_LSB 0x03

#define AM2320_REG_MODEL_ID_MSB 0x08
#define AM2320_REG_MODEL_ID_LSB 0x09
#define AM2320_REG_VERSION 0x0A
#define AM2320_REG_DEVICE_ID_BYTE4 0x0B
#define AM2320_REG_DEVICE_ID_BYTE3 0x0C
#define AM2320_REG_DEVICE_ID_BYTE2 0x0D
#define AM2320_REG_DEVICE_ID_BYTE1 0x0E

#define AM2320_REG_STATUS 0x0F

#define AM2320_REG_USER_REG1_MSB 0x10
#define AM2320_REG_USER_REG1_LSB 0x11
#define AM2320_REG_USER_REG2_MSB 0x12
#define AM2320_REG_USER_REG2_LSB 0x13


void am2320_init(void)
{
    int rate = i2c_init(AM2320_I2C_INSTANCE, AM2320_I2C_SPEED);

    gpio_set_function(AM2320_I2C_SCL, GPIO_FUNC_I2C);
    gpio_set_function(AM2320_I2C_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(AM2320_I2C_SCL);
    gpio_pull_up(AM2320_I2C_SDA);

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(AM2320_I2C_SDA, AM2320_I2C_SCL, GPIO_FUNC_I2C));
}



static uint16_t am2320_crc16(uint8_t *ptr, size_t len)
{
    uint16_t crc = 0xFFFF;

    while (len--)
    {
        crc ^= *ptr++;
        for (size_t i = 0; i < 8; i++)
        {
            if (crc & 0x01)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}



uint8_t am2320_read_temperature_humidity(float *temperature, float *humidity)
{
    uint8_t write_buffer[3] = {0x03, AM2320_REG_HUMIDITY_MSB, 4}; // Read model command
    uint8_t read_buffer[12] = {0};
    char buffer[64];

    *temperature = NAN;
    *humidity = NAN;

    size_t retries = 0;

    // Send read command
    int res;
    do
    {
        absolute_time_t timeout_val = get_absolute_time() + 10000; // 10ms timeout
        res = i2c_write_blocking_until(AM2320_I2C_INSTANCE, AM2320_I2C_ADDRESS, write_buffer, sizeof(write_buffer), false, timeout_val);
        if (res < 0)
        {
            timeout_val = get_absolute_time() + 10000; // 10ms timeout
            int res2 = i2c_read_blocking_until(AM2320_I2C_INSTANCE, AM2320_I2C_ADDRESS, read_buffer, 8, false, timeout_val);
            printf("I2C Write Error: %d, Read Attempt Result: %d\n", res, res2);
        }
        sleep_ms(2);
    } while ((res < 0) && (retries++ < 50));

    sleep_ms(15); // Wait for sensor to process

    // Read 6 bytes of data
    absolute_time_t timeout_val = get_absolute_time() + 10000; // 10ms timeout
    res = i2c_read_blocking_until(AM2320_I2C_INSTANCE, AM2320_I2C_ADDRESS, read_buffer, 8, false, timeout_val);

    if (res < 0)
    {
        // sprintf(buffer, "I2C Read Error: %d\n", res);
        // write_string_at(buffer, 0, 0, 0xF800, 0x0000, ssd1351_get_framebuffer(), SSD1351_WIDTH, SSD1351_HEIGHT);
        // ssd1351_update();
        return -2; // I2C read error
    }

    // Verify checksum
    uint16_t crc_received = ((uint16_t)read_buffer[7] << 8) | read_buffer[6];
    uint16_t crc_calculated = am2320_crc16(read_buffer, 6);

    if (crc_calculated != crc_received)
    {
        // sprintf(buffer, "CRC Error!\nCalculated %04X\nReceived %04X\n", crc_calculated, crc_received);
        // write_string_at(buffer, 0, 8, 0xF800, 0x0000, ssd1351_get_framebuffer(), SSD1351_WIDTH, SSD1351_HEIGHT);
        // ssd1351_update();
        return -1; // Checksum error
    }

    // Extract humidity and temperature
    uint16_t raw_humidity = ((uint16_t)read_buffer[2] << 8) | read_buffer[3];
    uint16_t raw_temperature = ((uint16_t)read_buffer[4] << 8) | read_buffer[5];

    *humidity = raw_humidity / 10.0f;
    *temperature = (raw_temperature & 0x7FFF) / 10.0f;
    if (raw_temperature & 0x8000)
    {
        *temperature = -*temperature;
    }

    // sprintf(buffer, "Temp: %04X\n", raw_temperature);
    // write_string_at(buffer, 0, 0, 0xFFFF, 0x0000, ssd1351_get_framebuffer(), SSD1351_WIDTH, SSD1351_HEIGHT);
    // ssd1351_update();

    return 0; // Success
}
