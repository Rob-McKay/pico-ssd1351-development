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

#include <stdint.h>

#ifndef BMP280_H
#define BMP280_H

#define BMP280_SUCCESS 0
#define BMP280_ERROR -1

/** Initialize the BMP280 sensor connection */
void bmp280_init(void);



/** Read temperature and pressure from the BMP280 sensor 
 * @param temperature Pointer to store the temperature in degrees Celsius
 * @param pressure Pointer to store the pressure in hPa
 * 
 * @return BMP280_SUCCESS on success
 */
int bmp280_read_temperature_pressure(float* temperature, float* pressure);

#endif // BMP280_H
