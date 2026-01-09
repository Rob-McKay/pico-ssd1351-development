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


#include <stdbool.h>
#include <stdint.h>

#ifndef ENS160_H
#define ENS160_H

#define ENS160_SUCCESS 0
#define ENS160_ERROR -1



/** Initialize the ENS160 sensor connection */
void ens160_init(void);

/** Read the part ID from the ENS160 sensor 
 * @param part_id Pointer to store the part ID
 * 
 * @return ENS160_SUCCESS on success
 */
int ens160_read_part_id(uint16_t *part_id);



/** Read the config from the ENS160 sensor 
 * @param config Pointer to store the config
 * 
 * @return ENS160_SUCCESS on success
 */
int ens160_read_config(uint8_t *config);



typedef enum ENS160_mode_e { ENS160_DEEP_SLEEP_MODE = 0x00, ENS160_IDLE_MODE = 0x01, ENS160_STANDARD_MODE = 0x02, ENS160_RESET_MODE = 0xFF } ENS160_mode_t;

/** Read the operating mode from the ENS160 sensor 
 * @param mode Pointer to store the operating mode
 * 
 * @return ENS160_SUCCESS on success
 */
int ens160_read_mode(ENS160_mode_t *mode);




int ens160_write_mode(ENS160_mode_t mode);



/** Read temperature and humidity from the ENS160 sensor 
 * @param temperature Pointer to store the temperature in Celsius
 * @param humidity Pointer to store the relative humidity in percentage
 * 
 * @return ENS160_SUCCESS on success
 */
int ens160_read_temperature_humidity(float *temperature, float *humidity);



/** Write temperature and humidity to the ENS160 sensor 
 * @param temperature Temperature in Celsius
 * @param humidity Relative humidity in percentage
 * 
 * @return ENS160_SUCCESS on success
 */
int ens160_write_temperature_humidity(float temperature, float humidity);


struct ENS160_status_s {
    bool running : 1; /**< Running in OP Mode */
    bool error: 1; /**< Error. Invalid OP Mode */
    bool new_data: 1; /**< New data available */
    bool new_gpr: 1; /**< New general purpose data available */
    uint8_t status : 2; /**< Status code. 0 = Normal, 1 = Warm-up phase, 2 = Initial start-up phase, 3 = Invalid output */
};

/** Read the status from the ENS160 sensor 
 * @param status Pointer to store the status
 * 
 * @return ENS160_SUCCESS on success
 */
int ens160_read_status(struct ENS160_status_s *status);



/** Read the air quality index from the ENS160 sensor 
 * @param aqi Pointer to store the air quality index
 * 
 * @return ENS160_SUCCESS on success
 */
int ens160_read_air_quality_index(uint8_t *aqi);



/** Read the TVOC value from the ENS160 sensor 
 * @param tvoc Pointer to store the TVOC value in ppb
 * 
 * @return ENS160_SUCCESS on success
 */
int ens160_read_tvoc(uint16_t *tvoc);



/** Read the eCO2 value from the ENS160 sensor 
 * @param eco2 Pointer to store the eCO2 value in ppm
 * 
 * @return ENS160_SUCCESS on success
 */
int ens160_read_eco2(uint16_t *eco2);

#endif // ENS160_H
