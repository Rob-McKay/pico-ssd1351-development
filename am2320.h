#include <stdint.h>

#ifndef AM2320_H
#define AM2320_H

void am2320_init(void);

uint8_t am2320_read_temperature_humidity(float* temperature, float* humidity);

#endif // AM2320_H
