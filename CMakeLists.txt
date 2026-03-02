cmake_minimum_required(VERSION 3.22)
include(ExternalProject)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
# User is free to modify the file as much as necessary
#

# Core MCU/linker settings consumed by the toolchain file
set(STM32_MCU_FLAGS "-mcpu=cortex-m55 -mfpu=fpv5-d16 -mfloat-abi=hard -mcmse ")
set(STM32_LINKER_SCRIPT "Gcc/STM32N657xx.ld")
set(STM32_LINKER_OPTION "-Wl,--no-warn-rwx-segments -u _printf_float")

# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")

# Set the project name
set(CMAKE_PROJECT_NAME x-cube-n6-ai-h264-usb-uvc)
project(${CMAKE_PROJECT_NAME})

# Include external project file
include("mx-generated.cmake")
