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

#include "bmp280.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "pico/binary_info.h"
#include "pico/time.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define BMP280_I2C_INSTANCE i2c1
#define BMP280_I2C_SCL 15
#define BMP280_I2C_SDA 14
#define BMP280_I2C_SPEED 400000    // 100 kHz
#define BMP280_I2C_ADDRESS (0x77U) // 7-bit address - SDO high


#define BMP280_REG_T1 0X88
#define BMP280_REG_T2 0X8A
#define BMP280_REG_T3 0X8C
#define BMP280_REG_P1 0X8E
#define BMP280_REG_P2 0X90
#define BMP280_REG_P3 0X92
#define BMP280_REG_P4 0X94
#define BMP280_REG_P5 0X96
#define BMP280_REG_P6 0X98
#define BMP280_REG_P7 0X9A
#define BMP280_REG_P8 0X9C
#define BMP280_REG_P9 0X9E
#define BMP280_REG_ID 0XD0

#define BMP280_REG_CTRL_MEAS 0xF4U
#define BMP280_REG_CONFIG 0xF5U

#define BMP280_REG_PRESSURE_MSB 0xF7U
#define BMP280_REG_PRESSURE_LSB 0xF8U
#define BMP280_REG_PRESSURE_XLSB 0xF9U

#define BMP280_REG_TEMPERATURE_MSB 0xFAU
#define BMP280_REG_TEMPERATURE_LSB 0xFBU
#define BMP280_REG_TEMPERATURE_XLSB 0xFCU


#define MAX_REGISTER_WRITE_SIZE 8


static struct
{
    uint16_t T1;
    int16_t T2;
    int16_t T3;
    uint16_t P1;
    int16_t P2;
    int16_t P3;
    int16_t P4;
    int16_t P5;
    int16_t P6;
    int16_t P7;
    int16_t P8;
    int16_t P9;
} bmp280_calibration_params;



static int write_register(uint8_t reg, size_t size, void *data)
{
    if (size > MAX_REGISTER_WRITE_SIZE)
    {
        return BMP280_ERROR; // Error: Size too large
    }

    uint8_t write_buffer[1 + MAX_REGISTER_WRITE_SIZE];
    write_buffer[0] = reg;
    memmove(&write_buffer[1], data, size);

    // Write register address and value
    i2c_write_blocking(BMP280_I2C_INSTANCE, BMP280_I2C_ADDRESS, write_buffer, 1 + size, false);

    return BMP280_SUCCESS; // Success
}



static int read_register(uint8_t reg, size_t size, uint8_t *data)
{
    uint8_t write_buffer[1] = {reg};

    absolute_time_t timeout_val = get_absolute_time() + 10000; // 10ms timeout

    // Write register address
    if (i2c_write_blocking_until(BMP280_I2C_INSTANCE, BMP280_I2C_ADDRESS, write_buffer, sizeof(write_buffer), true, timeout_val) != sizeof(write_buffer))
    {
        printf("BMP280 I2C write error reading reg %02X\n", reg);
        return BMP280_ERROR;
    }

    timeout_val = get_absolute_time() + 10000; // 10ms timeout
    // Read register value
    if (i2c_read_blocking_until(BMP280_I2C_INSTANCE, BMP280_I2C_ADDRESS, data, size, false, timeout_val) != size)
    {
        printf("BMP280 I2C read error reading reg %02X\n", reg);
        return BMP280_ERROR;
    }
    return BMP280_SUCCESS;
}



static int bmp280_read_calibration_word_signed(uint8_t reg, int16_t *value)
{
    uint8_t data[2];
    memset(data, 0, 2);

    int res = read_register(reg, 2, data);
    *value = (int16_t)((data[1] << 8) | data[0]);
    return res;
}



static int bmp280_read_calibration_word_unsigned(uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    memset(data, 0, 2);

    int res = read_register(reg, 2, data);
    *value = (uint16_t)((data[1] << 8) | data[0]);
    return res;
}



static int get_calibration_data(void)
{
    memset(&bmp280_calibration_params, 0, sizeof(bmp280_calibration_params));

    // Placeholder for reading calibration data from the BMP280
    int16_t result_signed;
    uint16_t result_unsigned;

    /*Getting calibration data byte by byte and saving it to the handle*/
    int error = bmp280_read_calibration_word_unsigned(BMP280_REG_T1, &result_unsigned);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error T1: %d\n", error);
        return error;
    }
    bmp280_calibration_params.T1 = result_unsigned;

    error = bmp280_read_calibration_word_signed(BMP280_REG_T2, &result_signed);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error T2: %d\n", error);
        return error;
    }
    bmp280_calibration_params.T2 = result_signed;

    error = bmp280_read_calibration_word_signed(BMP280_REG_T3, &result_signed);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error T3: %d\n", error);
        return error;
    }
    bmp280_calibration_params.T3 = result_signed;

    error = bmp280_read_calibration_word_unsigned(BMP280_REG_P1, &result_unsigned);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error P1: %d\n", error);
        return error;
    }
    bmp280_calibration_params.P1 = result_unsigned;

    error = bmp280_read_calibration_word_signed(BMP280_REG_P2, &result_signed);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error P2: %d\n", error);
        return error;
    }
    bmp280_calibration_params.P2 = result_signed;

    error = bmp280_read_calibration_word_signed(BMP280_REG_P3, &result_signed);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error P3: %d\n", error);
        return error;
    }
    bmp280_calibration_params.P3 = result_signed;

    error = bmp280_read_calibration_word_signed(BMP280_REG_P4, &result_signed);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error P4: %d\n", error);
        return error;
    }
    bmp280_calibration_params.P4 = result_signed;

    error = bmp280_read_calibration_word_signed(BMP280_REG_P5, &result_signed);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error P5: %d\n", error);
        return error;
    }
    bmp280_calibration_params.P5 = result_signed;

    error = bmp280_read_calibration_word_signed(BMP280_REG_P6, &result_signed);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error P6: %d\n", error);
        return error;
    }
    bmp280_calibration_params.P6 = result_signed;

    error = bmp280_read_calibration_word_signed(BMP280_REG_P7, &result_signed);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error P7: %d\n", error);
        return error;
    }
    bmp280_calibration_params.P7 = result_signed;

    error = bmp280_read_calibration_word_signed(BMP280_REG_P8, &result_signed);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error P8: %d\n", error);
        return error;
    }
    bmp280_calibration_params.P8 = result_signed;

    error = bmp280_read_calibration_word_signed(BMP280_REG_P9, &result_signed);
    if (error != BMP280_SUCCESS)
    {
        printf("BMP280 calibration read error P9: %d\n", error);
        return error;
    }
    bmp280_calibration_params.P9 = result_signed;

    return error;
}



void bmp280_init(void)
{
    int rate = i2c_init(BMP280_I2C_INSTANCE, BMP280_I2C_SPEED);

    gpio_set_function(BMP280_I2C_SCL, GPIO_FUNC_I2C);
    gpio_set_function(BMP280_I2C_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(BMP280_I2C_SCL);
    gpio_pull_up(BMP280_I2C_SDA);

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(BMP280_I2C_SDA, BMP280_I2C_SCL, GPIO_FUNC_I2C));

    get_calibration_data();

    // Configure the BMP280 (normal mode, temp and pressure oversampling x4, standby 500ms)
    uint8_t ctrl_meas = (0x03U << 5) | (0x03U << 2) | 0x03U; // osrs_t=3, osrs_p=3, mode=normal
    write_register(BMP280_REG_CTRL_MEAS, 1, &ctrl_meas);
}



int32_t t_fine;

static double compensate_temperature(int32_t temperature_raw)
{
     double var1, var2, T;

    var1 = (((double)temperature_raw)/16384.0 - (((double)bmp280_calibration_params.T1) / 1024.0)) * ((double)bmp280_calibration_params.T2);
    var2 = ((((double)temperature_raw)/131072.0 - ((double)bmp280_calibration_params.T1)/8192.0) * (((double)temperature_raw)/131072.0 - ((double)bmp280_calibration_params.T1)/8192.0)) *
            ((double)bmp280_calibration_params.T3);
    t_fine = (int32_t)(var1 + var2);

    return (var1 + var2) / 5120.0;
}


static double compensate_pressure(int32_t pressure_raw)
{
    double var1, var2, p;

    var1 = ((double)t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double)bmp280_calibration_params.P6) / 32768.0;
    var2 = var2 + var1 * ((double)bmp280_calibration_params.P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)bmp280_calibration_params.P4) * 65536.0);
    var1 = (((double)bmp280_calibration_params.P3) * var1 * var1 / 524288.0 + ((double)bmp280_calibration_params.P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)bmp280_calibration_params.P1);
    if (var1 == 0.0)
    {
        return 0.; // avoid exception caused by division by zero
    }
    p = 1048576.0 - (double)pressure_raw;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)bmp280_calibration_params.P9) * p * p / 2147483648.0;
    var2 = p * ((double)bmp280_calibration_params.P8) / 32768.0;
    p = p + (var1 + var2 + ((double)bmp280_calibration_params.P7)) / 16.0;

    return p;
}





int bmp280_read_temperature_pressure(float *temperature, float *pressure)
{
    uint8_t data[6];
    memset(data, 0, 6);

    if (read_register(BMP280_REG_PRESSURE_MSB, 6, data) != BMP280_SUCCESS)
    {
        *temperature = NAN;
        *pressure = NAN;
        return BMP280_ERROR;
    }

    // Combine the 3 bytes of pressure data
    uint32_t pressure_raw = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);

    // Combine the 3 bytes of temperature data
    uint32_t temperature_raw = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

    // Convert raw values to actual temperature and pressure

    *temperature = (float)compensate_temperature(temperature_raw);
    *pressure = (float)compensate_pressure(pressure_raw)/1000.0f; // Convert to kPa

    return BMP280_SUCCESS;
}
