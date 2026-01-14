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
#define AQI_MASK 0x03U



/**
 * @brief Initialize the ENS160 sensor.
 *
 * This function initializes the I2C interface for communication with the
 * ENS160 air quality sensor.
 */
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



/**
 * @brief Read a register from the ENS160 sensor.
 *
 * This function reads data from a specified register of the ENS160 sensor
 * over the I2C interface.
 *
 * @param reg The register address to read from.
 * @param size The number of bytes to read.
 * @param data Pointer to the buffer to store the read data.
 * @return ENS160_SUCCESS on success, error code otherwise.
 */
static int ens160_read_register(uint8_t reg, size_t size, uint8_t *data)
{
    uint8_t write_buffer[1] = {reg};

    absolute_time_t timeout_val = get_absolute_time() + 10000; // 10ms timeout

    // Write register address
    int err = i2c_write_blocking_until(ENS160_I2C_INSTANCE, ENS160_I2C_ADDRESS, write_buffer, sizeof(write_buffer), true, timeout_val);

    if (err != sizeof(write_buffer))
    {
        printf("ENS160 I2C write error %d reading reg %02X\n", err, reg);
        return err;
    }
    timeout_val = get_absolute_time() + 10000; // 10ms timeout

    // Read register value
    err = i2c_read_blocking_until(ENS160_I2C_INSTANCE, ENS160_I2C_ADDRESS, data, size, false, timeout_val);
    if (err != size)
    {
        printf("ENS160 I2C read error %d reading reg %02X\n", err, reg);
        return err;
    }

    return ENS160_SUCCESS; // Success
}



/**
 * @brief Write to a register of the ENS160 sensor.
 *
 * This function writes data to a specified register of the ENS160 sensor
 * over the I2C interface.
 *
 * @param reg The register address to write to.
 * @param size The number of bytes to write.
 * @param data Pointer to the buffer containing the data to write.
 * @return The number of bytes written on success, error code otherwise.
 */
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
    absolute_time_t timeout_val = get_absolute_time() + 10000; // 10ms timeout
    int err = i2c_write_blocking_until(ENS160_I2C_INSTANCE, ENS160_I2C_ADDRESS, write_buffer, 1 + size, false, timeout_val);

    return err;
}



/**
 * @brief Read the part ID of the ENS160 sensor.
 *
 * This function reads the part ID from the ENS160 sensor.
 *
 * @param part_id Pointer to store the read part ID.
 * @return ENS160_SUCCESS on success, error code otherwise.
 */
int ens160_read_part_id(uint16_t *part_id)
{
    uint8_t read_buffer[ENS160_REG_PART_ID_SIZE] = {0};
    ens160_read_register(ENS160_REG_PART_ID, ENS160_REG_PART_ID_SIZE, read_buffer);
    *part_id = ((uint16_t)read_buffer[1] << 8) | read_buffer[0];
    return ENS160_SUCCESS; // Success
}



/**
 * @brief Read the current operating mode of the ENS160 sensor.
 *
 * This function reads the current operating mode from the ENS160 sensor.
 *
 * @param mode Pointer to store the read operating mode.
 * @return ENS160_SUCCESS on success, error code otherwise.
 */
int ens160_read_mode(ENS160_mode_t *mode)
{
    uint8_t read_buffer[ENS160_REG_OP_MODE_SIZE] = {0};
    ens160_read_register(ENS160_REG_OP_MODE, ENS160_REG_OP_MODE_SIZE, read_buffer);
    *mode = (ENS160_mode_t)read_buffer[0];

    return ENS160_SUCCESS; // Success
}



/**
 * @brief Write the operating mode to the ENS160 sensor.
 *
 * This function writes the specified operating mode to the ENS160 sensor.
 *
 * @param mode The operating mode to set.
 * @return The number of bytes written on success, error code otherwise.
 */
int ens160_write_mode(ENS160_mode_t mode)
{
    uint8_t write_buffer[ENS160_REG_OP_MODE_SIZE];
    write_buffer[0] = ((int)mode) & 0xFFU;
    return ens160_write_register(ENS160_REG_OP_MODE, ENS160_REG_OP_MODE_SIZE, &write_buffer);
}



/**
 * @brief Read the configuration register of the ENS160 sensor.
 *
 * This function reads the configuration register from the ENS160 sensor.
 *
 * @param config Pointer to store the read configuration value.
 * @return ENS160_SUCCESS on success, error code otherwise.
 */
int ens160_read_config(uint8_t *config)
{
    uint8_t read_buffer[ENS160_REG_CONFIG_SIZE] = {0};
    ens160_read_register(ENS160_REG_CONFIG, ENS160_REG_CONFIG_SIZE, read_buffer);
    *config = read_buffer[0];

    return ENS160_SUCCESS; // Success
}



/**
 * @brief Write the configuration register to the ENS160 sensor.
 *
 * This function writes the specified configuration value to the ENS160 sensor.
 *
 * @param config The configuration value to set.
 * @return The number of bytes written on success, error code otherwise.
 */
int ens160_read_temperature_humidity(float *temperature, float *humidity)
{
    uint8_t read_buffer[ENS160_REG_TEMP_IN_SIZE] = {0};
    ens160_read_register(ENS160_REG_TEMP_IN, ENS160_REG_TEMP_IN_SIZE, read_buffer);
    *temperature = ((float)((int16_t)((read_buffer[1] << 8) | read_buffer[0])) / TEMPERATURE_SCALAR) - KELVIN_TEMPERATURE_OFFSET;

    ens160_read_register(ENS160_REG_HUMIDITY_IN, ENS160_REG_HUMIDITY_IN_SIZE, read_buffer);
    *humidity = ((float)((read_buffer[1] << 8) | read_buffer[0])) / HUMIDITY_SCALAR;

    return ENS160_SUCCESS; // Success
}



/**
 * @brief Write temperature and humidity to the ENS160 sensor.
 *
 * This function writes the specified temperature and humidity values to the
 * ENS160 sensor.
 *
 * @param temperature The temperature value to set (in Celsius).
 * @param humidity The humidity value to set (in %RH).
 * @return The number of bytes written on success, error code otherwise.
 */
int ens160_write_temperature_humidity(float temperature, float humidity)
{
    int res = ENS160_SUCCESS;
    if (temperature == temperature)
    {
        uint16_t temp_raw = (uint16_t)((KELVIN_TEMPERATURE_OFFSET + temperature) * TEMPERATURE_SCALAR);

        uint8_t write_buffer[ENS160_REG_TEMP_IN_SIZE];
        write_buffer[0] = (temp_raw >> 8) & 0xFFU;
        write_buffer[1] = temp_raw & 0xFFU;
        res = ens160_write_register(ENS160_REG_TEMP_IN, ENS160_REG_TEMP_IN_SIZE, &write_buffer);
    }

    if ((res == ENS160_SUCCESS) && (humidity == humidity))
    {
        uint16_t humidity_raw = (uint16_t)(humidity * HUMIDITY_SCALAR);
        uint8_t write_buffer[ENS160_REG_HUMIDITY_IN_SIZE];
        write_buffer[0] = (humidity_raw >> 8) & 0xFFU;
        write_buffer[1] = humidity_raw & 0xFFU;
        res = ens160_write_register(ENS160_REG_HUMIDITY_IN, ENS160_REG_HUMIDITY_IN_SIZE, &write_buffer);
    }

    return res;
}



/**
 * @brief Read the status register of the ENS160 sensor.
 *
 * This function reads the status register from the ENS160 sensor.
 *
 * @param status Pointer to store the read status.
 * @return ENS160_SUCCESS on success, error code otherwise.
 */
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



/**
 * @brief Read the Air Quality Index (AQI) from the ENS160 sensor.
 *
 * This function reads the AQI value from the ENS160 sensor.
 *
 * @param aqi Pointer to store the read AQI value.
 * @return ENS160_SUCCESS on success, error code otherwise.
 */
int ens160_read_air_quality_index(enum ENS160_air_quality_index_e *aqi)
{
    uint8_t read_buffer[ENS160_REG_AQI_SIZE] = {0};
    if (ens160_read_register(ENS160_REG_AQI, ENS160_REG_AQI_SIZE, read_buffer) == ENS160_SUCCESS)
    {
        *aqi = (enum ENS160_air_quality_index_e)(read_buffer[0] & AQI_MASK);
        return ENS160_SUCCESS; // Success
    }
    
    *aqi = ENS160_AQI_UNKNOWN;
    return ENS160_ERROR; // Error
}



/**
 * @brief Read the TVOC value from the ENS160 sensor.
 *
 * This function reads the Total Volatile Organic Compounds (TVOC) value
 * from the ENS160 sensor.
 *
 * @param tvoc Pointer to store the read TVOC value.
 * @return ENS160_SUCCESS on success, error code otherwise.
 */
int ens160_read_tvoc(uint16_t *tvoc)
{
    uint8_t read_buffer[ENS160_REG_TVOC_SIZE] = {0};
    ens160_read_register(ENS160_REG_TVOC, ENS160_REG_TVOC_SIZE, read_buffer);
    *tvoc = ((uint16_t)read_buffer[1] << 8) | read_buffer[0];

    return ENS160_SUCCESS; // Success
}



/**
 * @brief Read the eCO2 value from the ENS160 sensor.
 *
 * This function reads the equivalent CO2 (eCO2) value from the ENS160 sensor.
 *
 * @param eco2 Pointer to store the read eCO2 value.
 * @return ENS160_SUCCESS on success, error code otherwise.
 */
int ens160_read_eco2(uint16_t *eco2)
{
    uint8_t read_buffer[ENS160_REG_ECO2_SIZE] = {0};
    ens160_read_register(ENS160_REG_ECO2, ENS160_REG_ECO2_SIZE, read_buffer);
    *eco2 = ((uint16_t)read_buffer[1] << 8) | read_buffer[0];

    return ENS160_SUCCESS; // Success
}
