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

#include "ens160.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "character_data.h"
#include "ssd1351.h"

#include "pico/binary_info.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ENS160_I2C_INSTANCE i2c0
#define ENS160_I2C_SCL 21
#define ENS160_I2C_SDA 20
#define ENS160_I2C_SPEED 100000 // 100 kHz

#define ENS160_I2C_ADDRESS (0x52U) // ADD pulled down at power up


#define ENS160_REG_PART_ID 0x00
#define ENS160_REG_PART_ID_SIZE 2

#define ENS160_REG_OP_MODE 0x10
#define ENS160_REG_OP_MODE_SIZE 1

#define ENS160_REG_CONFIG 0x11
#define ENS160_REG_CONFIG_SIZE 1

#define ENS160_REG_COMMAND 0x12
#define ENS160_REG_COMMAND_SIZE 1

#define ENS160_REG_TEMP_IN 0x13
#define ENS160_REG_TEMP_IN_SIZE 2

#define ENS160_REG_HUMIDITY_IN 0x15
#define ENS160_REG_HUMIDITY_IN_SIZE 2

#define ENS160_REG_STATUS 0x20
#define ENS160_REG_STATUS_SIZE 1

#define ENS160_REG_AQI 0x21
#define ENS160_REG_AQI_SIZE 1

#define ENS160_REG_TVOC 0x22
#define ENS160_REG_TVOC_SIZE 2

#define ENS160_REG_ECO2 0x24
#define ENS160_REG_ECO2_SIZE 2


#define MAX_REGISTER_WRITE_SIZE 8

#define KELVIN_TEMPERATURE_OFFSET 273.15f
#define TEMPERATURE_SCALAR 64U

#define HUMIDITY_SCALAR 512U

void ens160_init(void)
{
    int rate = i2c_init(ENS160_I2C_INSTANCE, ENS160_I2C_SPEED);

    gpio_set_function(ENS160_I2C_SCL, GPIO_FUNC_I2C);
    gpio_set_function(ENS160_I2C_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(ENS160_I2C_SCL);
    gpio_pull_up(ENS160_I2C_SDA);

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(ENS160_I2C_SDA, ENS160_I2C_SCL, GPIO_FUNC_I2C));
}



static void ens160_read_register(uint8_t reg, size_t size, uint8_t *data)
{
    uint8_t write_buffer[1] = {reg};

    // Write register address
    i2c_write_blocking(ENS160_I2C_INSTANCE, ENS160_I2C_ADDRESS, write_buffer, sizeof(write_buffer), true);

    // Read register value
    i2c_read_blocking(ENS160_I2C_INSTANCE, ENS160_I2C_ADDRESS, data, size, false);
}



static int ens160_write_register(uint8_t reg, size_t size, void *data)
{
    if (size > MAX_REGISTER_WRITE_SIZE)
    {
        return ENS160_ERROR; // Error: Size too large
    }

    uint8_t write_buffer[1 + MAX_REGISTER_WRITE_SIZE];
    write_buffer[0] = reg;
    memmove(&write_buffer[1], data, size);

    // Write register address and value
    i2c_write_blocking(ENS160_I2C_INSTANCE, ENS160_I2C_ADDRESS, write_buffer, 1 + size, false);

    return ENS160_SUCCESS; // Success
}



int ens160_read_part_id(uint16_t *part_id)
{
    uint8_t read_buffer[ENS160_REG_PART_ID_SIZE] = {0};
    ens160_read_register(ENS160_REG_PART_ID, ENS160_REG_PART_ID_SIZE, read_buffer);
    *part_id = ((uint16_t)read_buffer[1] << 8) | read_buffer[0];

    char buffer[64];
    sprintf(buffer, "Part ID: %04X\n", *part_id);
    write_string_at(buffer, 0, 40, 0xFFFF, 0x0000, ssd1351_get_framebuffer(), SSD1351_WIDTH, SSD1351_HEIGHT);
    ssd1351_update();

    return ENS160_SUCCESS; // Success
}



int ens160_read_mode(ENS160_mode_t *mode)
{
    uint8_t read_buffer[ENS160_REG_OP_MODE_SIZE] = {0};
    ens160_read_register(ENS160_REG_OP_MODE, ENS160_REG_OP_MODE_SIZE, read_buffer);
    *mode = (ENS160_mode_t)read_buffer[0];

    return ENS160_SUCCESS; // Success
}



int ens160_write_mode(ENS160_mode_t mode)
{
    uint8_t write_buffer[ENS160_REG_OP_MODE_SIZE];
    write_buffer[0] = ((int)mode) & 0xFFU;
    return ens160_write_register(ENS160_REG_OP_MODE, ENS160_REG_OP_MODE_SIZE, &write_buffer);
}



int ens160_read_config(uint8_t *config)
{
    uint8_t read_buffer[ENS160_REG_CONFIG_SIZE] = {0};
    ens160_read_register(ENS160_REG_CONFIG, ENS160_REG_CONFIG_SIZE, read_buffer);
    *config = read_buffer[0];

    return ENS160_SUCCESS; // Success
}



int ens160_read_temperature_humidity(float *temperature, float *humidity)
{
    uint8_t read_buffer[ENS160_REG_TEMP_IN_SIZE] = {0};
    ens160_read_register(ENS160_REG_TEMP_IN, ENS160_REG_TEMP_IN_SIZE, read_buffer);
    *temperature = ((float)((int16_t)((read_buffer[1] << 8) | read_buffer[0])) / TEMPERATURE_SCALAR) - KELVIN_TEMPERATURE_OFFSET;

    ens160_read_register(ENS160_REG_HUMIDITY_IN, ENS160_REG_HUMIDITY_IN_SIZE, read_buffer);
    *humidity = ((float)((read_buffer[1] << 8) | read_buffer[0])) / HUMIDITY_SCALAR;

    return ENS160_SUCCESS; // Success
}



int ens160_write_temperature_humidity(float temperature, float humidity)
{
    uint16_t temp_raw = (uint16_t)((KELVIN_TEMPERATURE_OFFSET + temperature) * TEMPERATURE_SCALAR); // Example conversion
    uint16_t humidity_raw = (uint16_t)(humidity * HUMIDITY_SCALAR);                                 // Example conversion

    uint8_t write_buffer[ENS160_REG_TEMP_IN_SIZE];
    write_buffer[0] = (temp_raw >> 8) & 0xFFU;
    write_buffer[1] = temp_raw & 0xFFU;
    ens160_write_register(ENS160_REG_TEMP_IN, ENS160_REG_TEMP_IN_SIZE, &write_buffer);

    write_buffer[0] = (humidity_raw >> 8) & 0xFFU;
    write_buffer[1] = humidity_raw & 0xFFU;
    ens160_write_register(ENS160_REG_HUMIDITY_IN, ENS160_REG_HUMIDITY_IN_SIZE, &write_buffer);

    return ENS160_SUCCESS; // Success
}



int ens160_read_status(struct ENS160_status_s *status)
{
    uint8_t read_buffer[ENS160_REG_STATUS_SIZE] = {0};

    ens160_read_register(ENS160_REG_STATUS, ENS160_REG_STATUS_SIZE, read_buffer);

    status->running = (read_buffer[0] & 0x80U) != 0;
    status->error = (read_buffer[0] & 0x40U) != 0;
    status->new_data = (read_buffer[0] & 0x02U) != 0;
    status->new_gpr = (read_buffer[0] & 0x01U) != 0;
    status->status = (read_buffer[1] >> 2U) & 3U;

    return ENS160_SUCCESS; // Success
}


#define AQI_MASK 0x03U



int ens160_read_air_quality_index(uint8_t *aqi)
{
    uint8_t read_buffer[ENS160_REG_AQI_SIZE] = {0};
    ens160_read_register(ENS160_REG_AQI, ENS160_REG_AQI_SIZE, read_buffer);
    *aqi = read_buffer[0] & AQI_MASK;

    return ENS160_SUCCESS; // Success
}



int ens160_read_tvoc(uint16_t *tvoc)
{
    uint8_t read_buffer[ENS160_REG_TVOC_SIZE] = {0};
    ens160_read_register(ENS160_REG_TVOC, ENS160_REG_TVOC_SIZE, read_buffer);
    *tvoc = ((uint16_t)read_buffer[1] << 8) | read_buffer[0];

    return ENS160_SUCCESS; // Success
}



int ens160_read_eco2(uint16_t *eco2)
{
    uint8_t read_buffer[ENS160_REG_ECO2_SIZE] = {0};
    ens160_read_register(ENS160_REG_ECO2, ENS160_REG_ECO2_SIZE, read_buffer);
    *eco2 = ((uint16_t)read_buffer[1] << 8) | read_buffer[0];

    return ENS160_SUCCESS; // Success
}


