# Pico Environmental Sensor Display

A Raspberry Pi Pico 2 project that interfaces with multiple environmental sensors and displays real-time data on an SSD1351 OLED display.

## Overview

This project demonstrates a complete environmental monitoring system running on the Raspberry Pi Pico 2. It reads data from multiple I2C sensors and displays the information on a colorful 128x128 OLED display.

## Features

- **Temperature & Humidity Monitoring**: AM2320 sensor provides ambient temperature and humidity readings
- **Pressure Sensing**: BMP280 barometric pressure sensor for atmospheric pressure and temperature
- **Air Quality Index**: ENS160 air quality sensor measures eCO2, TVOC, and AQI levels
- **Real-time Display**: 128x128 SSD1351 OLED display with color graphics and text rendering
- **Continuous Monitoring**: Updates sensor data every second with live display refresh

## Hardware Components

| Component | Purpose |
|-----------|---------|
| Raspberry Pi Pico 2 | Main microcontroller |
| AM2320 | Temperature and humidity sensor (I2C) |
| BMP280 | Barometric pressure and temperature sensor (I2C) |
| ENS160 | Air quality index sensor (I2C) |
| SSD1351 | 128x128 16-bit color OLED display (SPI) |

## Sensor Data

The display shows the following information updated in real-time:

- **Temperature** (from AM2320)
- **Humidity** (from AM2320)
- **eCO2** - Equivalent CO2 level in ppm
- **TVOC** - Total Volatile Organic Compounds in ppb
- **Air Quality Index** - Text representation (Excellent, Good, Moderate, Poor, Unhealthy, etc.)
- **Barometric Temperature** (from BMP280)
- **Atmospheric Pressure** (from BMP280)

## Project Structure

```
.
├── test.c                          # Main application
├── ssd1351.c/h                     # OLED display driver
├── am2320.c/h                      # Temperature/humidity sensor driver
├── bmp280.c/h                      # Pressure sensor driver
├── ens160.c/h                      # Air quality sensor driver
├── character_data.c/h              # Character rendering data for display
├── CMakeLists.txt                  # CMake build configuration
├── pico_sdk_import.cmake           # Pico SDK integration
└── build/                          # Build output directory
```

## Building

### Prerequisites

- Raspberry Pi Pico SDK (v2.2.0)
- CMake 3.13+
- Arm GNU Toolchain
- OpenOCD (for debugging)
- Ninja build system

### Build Instructions

1. Configure and generate build files:
   ```bash
   cmake -B build -G Ninja
   ```

2. Build the project:
   ```bash
   ninja -C build
   ```

   Or use the VS Code task: **Build Project** (Ctrl+Shift+B)

## Programming the Device

### Via USB (picotool)

1. Connect the Pico in bootloader mode (hold BOOTSEL while plugging in)
2. Run the **Run Project** task in VS Code
   ```bash
   picotool load build/test.uf2 -fx
   ```

### Via CMSIS-DAP Debugger

1. Connect the debugger to the Pico SWDIO/SWCLK pins
2. Run the **Flash** task in VS Code to program via OpenOCD

### Emergency Recovery

If the device becomes unresponsive, use the **Rescue Reset** task to put the Pico into recovery mode.

## Pinout Configuration

The sensor connections use the Pico's I2C and SPI interfaces:

- **I2C Bus** (GPIO 20/21 - i2c0): AM2320, ENS160
- **I2C Bus** (GPIO 14/15 - i2c1): BMP280
- **SPI Bus** (GPIO 18/19 spi0): SSD1351 display
- **GPIO 4/5**: SSD1351 display
- **GPIO 25**: Built-in LED (status indicator)

Refer to the individual driver files (*.c files) for detailed pin configurations.

## Operation

After programming:

1. The Pico initializes all sensors
2. The display shows the ENS160 part ID as a boot confirmation
3. Sensor data is read every 1 second
4. Display updates with current readings
5. The built-in LED toggles during each update cycle

Temperature and humidity from AM2320 are fed back to the ENS160 sensor to improve air quality measurements.

## Error Handling

- The display shows error messages in red if sensor reads fail
- BMP280 errors are reported on-screen with error codes
- The ENS160 sensor validates data with status checks before updating

## License

Copyright (c) 2026 Rob McKay. Licensed under the MIT License - see LICENSE file for details.

## Notes

- All communication uses I2C at standard speeds
- The display updates at 1 Hz (1000ms intervals)
- Character rendering uses custom bitmap font data
- The project uses the standard Pico SDK stdio initialization for potential USB serial debugging
