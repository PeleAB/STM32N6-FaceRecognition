:: Model Generation Script for STM32N6 Face Detection Pipeline
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

echo --- Generating Model 1: Face Detection (fd) - CenterFace ---
%STEDGEAI_EXE% generate --name fd ^
  --no-inputs-allocation ^
  --model centerface.tflite ^
  --target stm32n6 ^
  --st-neural-art fd@user_neuralart.json ^
  --input-data-type uint8 --output-data-type float32

if %errorlevel% neq 0 (popd & exit /b %errorlevel%)

copy st_ai_output\fd_ecblobs.h .
copy st_ai_output\fd.c .
copy st_ai_output\stai_fd.c .
copy st_ai_output\stai_fd.h .
copy st_ai_output\fd_atonbuf.xSPI2.raw fd_data.xSPI2.bin

arm-none-eabi-objcopy -I binary fd_data.xSPI2.bin --change-addresses 0x70380000 -O ihex fd_data.hex

echo --- Generating Model 2: Face ID Embedding (faceid) - MobileFaceNet ---
%STEDGEAI_EXE% generate --name faceid ^
  --no-outputs-allocation ^
  --model mobilefacenet_int8_faces.onnx ^
  --target stm32n6 ^
  --st-neural-art faceid@user_neuralart.json ^
  --input-data-type uint8 --output-data-type float32

if %errorlevel% neq 0 (popd & exit /b %errorlevel%)

copy st_ai_output\faceid_ecblobs.h .
copy st_ai_output\faceid.c .
copy st_ai_output\stai_faceid.c .
copy st_ai_output\stai_faceid.h .
copy st_ai_output\faceid_atonbuf.xSPI2.raw faceid_data.xSPI2.bin

arm-none-eabi-objcopy -I binary faceid_data.xSPI2.bin --change-addresses 0x72000000 -O ihex faceid_data.hex

echo.
echo Model generation complete.
echo Please flash fd_data.hex and faceid_data.hex to your board.

popd
