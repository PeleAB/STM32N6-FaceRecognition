@echo off
rem ==============================================================
rem Batch Script: Sign and Convert STM32N6 ObjectDetection Binary
rem --------------------------------------------------------------
rem 1) cd into the STM32CubeIDE\Debug folder
rem 2) Run STM32_SigningTool_CLI to sign the .bin
rem 3) Convert signed .bin to Intel HEX with arm-none-eabi-objcopy
rem --------------------------------------------------------------
rem Usage: Just double-click or run from a command prompt
rem ==============================================================

setlocal

rem === Configuration ===
set "CUBE_DEBUG_DIR=..\STM32CubeIDE\Debug"
set "INPUT_BIN=STM32N6_GettingStarted_ObjectDetection.bin"
set "SIGNED_BIN=STM32N6_GettingStarted_ObjectDetection_signed.bin"
set "OUTPUT_HEX=STM32N6_GettingStarted_ObjectDetection_signed.hex"
set "ADDRESS_OFFSET=0x70100000"
set "SIGN_TOOL=STM32_SigningTool_CLI"
set "OBJCPY=arm-none-eabi-objcopy"

rem === Step 1: Change Directory ===
echo [1/3] Changing directory to "%CUBE_DEBUG_DIR%"...
pushd "%CUBE_DEBUG_DIR%" >nul 2>&1 || (
    echo ERROR: Could not locate Debug folder "%CUBE_DEBUG_DIR%".
    exit /b 1
)

rem === Step 2: Sign Binary ===
echo [2/3] Signing "%INPUT_BIN%" --> "%SIGNED_BIN%"...
"%SIGN_TOOL%" -bin "%INPUT_BIN%" -nk -t ssbl -hv 2.3 -o "%SIGNED_BIN%"
if errorlevel 1 (
    echo ERROR: Signing failed. Check your signing tool parameters.
    popd
    exit /b 1
)

rem === Step 3: Convert to Intel HEX ===
echo [3/3] Converting "%SIGNED_BIN%" --> "%OUTPUT_HEX%" with offset %ADDRESS_OFFSET%...
"%OBJCPY%" -I binary -O ihex --change-addresses=%ADDRESS_OFFSET% "%SIGNED_BIN%" "%OUTPUT_HEX%"
if errorlevel 1 (
    echo ERROR: objcopy conversion failed. Verify your toolchain installation.
    popd
    exit /b 1
)

rem === Done ===
echo.
echo SUCCESS: Signed HEX available at "%CUBE_DEBUG_DIR%\%OUTPUT_HEX%".
popd
endlocal
exit /b 0
