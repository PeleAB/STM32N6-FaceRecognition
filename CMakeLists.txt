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

# ---------------------------------------------------------------------------
# n6-platform: reusable STM32N6 platform library
# Must be configured and added BEFORE include("mx-generated.cmake") so that
# the n6platform target exists when target_link_libraries is called below.
# ---------------------------------------------------------------------------
set(N6PLAT_CUBE_FW  "${CMAKE_SOURCE_DIR}/STM32Cube_FW_N6" CACHE PATH "Path to STM32Cube_FW_N6")
set(N6PLAT_LIB_ROOT "${CMAKE_SOURCE_DIR}/Lib"             CACHE PATH "Path to Lib/")
# Pass project-specific config headers (FreeRTOSConfig.h, stm32n6xx_hal_conf.h, etc.)
set(N6PLAT_INC_EXTRA "${CMAKE_SOURCE_DIR}/Inc")

add_subdirectory(../n6-platform n6-platform-build)

# ---------------------------------------------------------------------------
# Include CubeMX-generated build rules (defines the app executable target)
# ---------------------------------------------------------------------------
include("mx-generated.cmake")

# ---------------------------------------------------------------------------
# Suppress compilation of files now provided by n6platform.
# mx-generated.cmake lists these in APP_SOURCES (which is already consumed
# by add_executable above), so we mark them header-only to avoid duplicate
# symbol errors. This block survives CubeMX regeneration automatically.
# ---------------------------------------------------------------------------
set_source_files_properties(
    ${PROJECT_ROOT}/Src/sysobj/src/sysobj_cache.c
    ${PROJECT_ROOT}/Src/sysobj/src/sysobj_dma2d.c
    ${PROJECT_ROOT}/Src/sysobj/src/sysobj_encoder.c
    ${PROJECT_ROOT}/Src/sysobj/src/sysobj_uart.c
    ${PROJECT_ROOT}/Src/bsp/platform.c
    ${PROJECT_ROOT}/Src/bsp/freertos_platform.c
    ${PROJECT_ROOT}/Src/bsp/fuse_programming.c
    ${PROJECT_ROOT}/Src/svc/buffer_queue.c
    ${PROJECT_ROOT}/Src/svc/draw.c
    ${PROJECT_ROOT}/Src/svc/nn_service.c
    ${PROJECT_ROOT}/Src/svc/app_stats.c
    ${PROJECT_ROOT}/Src/svc/utils.c
    PROPERTIES HEADER_FILE_ONLY TRUE
)

# Link the platform library
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE n6platform)
