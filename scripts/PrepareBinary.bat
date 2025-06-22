@echo off
rem ============================================================== 
rem Batch Script: Sign and Convert STM32N6 ObjectDetection & Model Binaries
rem --------------------------------------------------------------
rem 1) cd into the STM32CubeIDE\Debug folder
rem 2) Sign the main .bin
rem 3) Convert signed .bin to Intel HEX
rem 4) Convert model RAW to Intel HEX
rem --------------------------------------------------------------
rem Usage: Double-click or run from a command prompt
rem ============================================================== 

setlocal

rem === Configuration ===
set "CUBE_DEBUG_DIR=..\STM32CubeIDE\Debug"
set "OUTPUT_DIR=..\..\Binary"
set "INPUT_BIN=STM32N6_GettingStarted_ObjectDetection.bin"
set "SIGNED_BIN=STM32N6_GettingStarted_ObjectDetection_signed.bin"
set "OUTPUT_HEX=STM32N6_GettingStarted_ObjectDetection_signed.hex"
set "ADDRESS_OFFSET=0x70100000"

rem Model conversion settings
set "MODEL_PATH=..\..\Model"
set "MODEL_DETECTION_RAW=face_detection_atonbuf.xSPI2.raw"
set "MODEL_RECOGNITION_RAW=face_recognition_atonbuf.xSPI2.raw"
set "MODEL_DETECTION_ADDRESS_OFFSET=0x71000000"
set "MODEL_RECOGNITION_ADDRESS_OFFSET=0x72000000"
set "MODEL_DETECTION_HEX=det_network_data.hex"
set "MODEL_RECOGNITION_HEX=rec_network_data.hex"

set "SIGN_TOOL=STM32_SigningTool_CLI"
set "OBJCPY=arm-none-eabi-objcopy"

rem === Prepare output folder ===

rem === Step 1: Change Directory ===
echo [1/4] Changing directory to "%CUBE_DEBUG_DIR%"...
pushd "%CUBE_DEBUG_DIR%" >nul 2>&1 || (
    echo ERROR: Could not locate Debug folder "%CUBE_DEBUG_DIR%".
    exit /b 1
)
if not exist "%OUTPUT_DIR%" (
    echo Creating output folder "%OUTPUT_DIR%"...
    mkdir "%OUTPUT_DIR%"
)

rem === Step 2: Sign Binary ===
echo [2/4] Signing "%INPUT_BIN%" --> "%OUTPUT_DIR%\%SIGNED_BIN%"...
"%SIGN_TOOL%" -bin "%INPUT_BIN%" -nk -t ssbl -hv 2.3 -o "%OUTPUT_DIR%\%SIGNED_BIN%"
if errorlevel 1 (
    echo ERROR: Signing failed. Check your signing tool parameters.
    popd
    exit /b 1
)

rem === Step 3: Convert to Intel HEX ===
echo [3/4] Converting "%OUTPUT_DIR%\%SIGNED_BIN%" --> "%OUTPUT_DIR%\%OUTPUT_HEX%" with offset %ADDRESS_OFFSET%...
"%OBJCPY%" -I binary -O ihex --change-addresses=%ADDRESS_OFFSET% "%OUTPUT_DIR%\%SIGNED_BIN%" "%OUTPUT_DIR%\%OUTPUT_HEX%"
if errorlevel 1 (
    echo ERROR: objcopy conversion failed. Verify your toolchain installation.
    popd
    exit /b 1
)

rem === Step 4: Convert Model RAW to Intel HEX ===
echo [4/4] Converting model RAW "%MODEL_PATH%\binary\%MODEL_DETECTION_RAW%" --> "%OUTPUT_DIR%\%MODEL_DETECTION_HEX%" with offset %MODEL_DETECTION_ADDRESS_OFFSET%...
"%OBJCPY%" -I binary -O ihex --change-addresses=%MODEL_DETECTION_ADDRESS_OFFSET% "%MODEL_PATH%\%MODEL_DETECTION_RAW%" "%OUTPUT_DIR%\%MODEL_DETECTION_HEX%"
if errorlevel 1 (
    echo ERROR: Model conversion failed. Check MODEL_PATH and raw filename.
    popd
    exit /b 1
)

rem === Step 4: Convert Model RAW to Intel HEX ===
echo [4/4] Converting model RAW "%MODEL_PATH%\binary\%MODEL_RECOGNITION_RAW%" --> "%OUTPUT_DIR%\%MODEL_RECOGNITION_HEX%" with offset %MODEL_RECOGNITION_ADDRESS_OFFSET%...
"%OBJCPY%" -I binary -O ihex --change-addresses=%MODEL_RECOGNITION_ADDRESS_OFFSET% "%MODEL_PATH%\%MODEL_RECOGNITION_RAW%" "%OUTPUT_DIR%\%MODEL_RECOGNITION_HEX%"
if errorlevel 1 (
    echo ERROR: Model conversion failed. Check MODEL_PATH and raw filename.
    popd
    exit /b 1
)
rem === Done ===
echo.
echo SUCCESS: Outputs available in "%OUTPUT_DIR%".
popd
endlocal
exit /b 0
