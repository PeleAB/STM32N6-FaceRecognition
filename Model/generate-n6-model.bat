:: Model Generation Script for STM32N6 Dual Model Pipeline
@echo off
setlocal enabledelayedexpansion

:: Switch to the directory where the script is located
pushd %~dp0

:: Update this path if stedgeai is installed elsewhere
set STEDGEAI_EXE="C:\ST\STEdgeAI\4.0\Utilities\windows\stedgeai.exe"

if not exist %STEDGEAI_EXE% (
    echo Error: stedgeai.exe not found at %STEDGEAI_EXE%
    echo Please update the STEDGEAI_EXE variable in this script.
    popd
    exit /b 1
)

echo --- Generating Model 1: Object Detection (od) ---
%STEDGEAI_EXE% generate --name od ^
  --no-inputs-allocation ^
  --model quantized_tiny_yolo_v2_224_.tflite ^
  --target stm32n6 ^
  --st-neural-art od@user_neuralart.json ^
  --input-data-type uint8 --output-data-type int8

if %errorlevel% neq 0 (popd & exit /b %errorlevel%)

copy st_ai_output\od_ecblobs.h .
copy st_ai_output\od.c .
copy st_ai_output\stai_od.c .
copy st_ai_output\stai_od.h .
copy st_ai_output\od_atonbuf.xSPI2.raw od_data.xSPI2.bin

:: Convert to HEX for flashing (assuming arm-none-eabi-objcopy is in PATH)
arm-none-eabi-objcopy -I binary od_data.xSPI2.bin --change-addresses 0x70380000 -O ihex od_data.hex

echo --- Generating Model 2: Re-Identification (reid) ---
%STEDGEAI_EXE% generate --name reid ^
  --no-outputs-allocation ^
  --model mobilenetv2_a100_256_128_fft_int8.tflite ^
  --target stm32n6 ^
  --st-neural-art reid@user_neuralart.json ^
  --input-data-type uint8 --output-data-type uint8

if %errorlevel% neq 0 (popd & exit /b %errorlevel%)

copy st_ai_output\reid_ecblobs.h .
copy st_ai_output\reid.c .
copy st_ai_output\stai_reid.c .
copy st_ai_output\stai_reid.h .
copy st_ai_output\reid_atonbuf.xSPI2.raw reid_data.xSPI2.bin

arm-none-eabi-objcopy -I binary reid_data.xSPI2.bin --change-addresses 0x72000000 -O ihex reid_data.hex

echo.
echo Model generation complete. 
echo Please flash od_data.hex and reid_data.hex to your board.

popd
