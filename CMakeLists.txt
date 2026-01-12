# SPDX-License-Identifier: MIT
#
# Copyright (c) 2026 Rob McKay
# All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
 

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#set(PICO_STDIO_USB:BOOL ON)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)

if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

# ====================================================================================

set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(test C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_link_options("-Wl,--print-memory-usage")

# Add executable. Default name is the project name, version 0.1

add_executable(test test.c ssd1351.c ssd1351.h character_data.cpp character_data.h am2320.c am2320.h ens160.c ens160.h bmp280.c bmp280.h)

pico_set_program_name(test "test")
pico_set_program_version(test "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(test 0)
pico_enable_stdio_usb(test 1)

# Add the standard library to the build
target_link_libraries(test
        pico_stdlib hardware_spi hardware_gpio hardware_i2c)


# Add the standard include files to the build
target_include_directories(test PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(test)
