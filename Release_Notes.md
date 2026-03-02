# Release Notes for x-cube-n6-ai-h264-usb-uvc Application

## Purpose

Computer Vision AI H264 UVC application using "TinyYOLOv2" model for people detection.

## Key Features

- Multi-threaded application flow (FreeRTOS)
- NPU accelerated quantized AI model inference
- Dual DCMIPP pipes
- DCMIPP crop, decimation, downscale
- DCMIPP ISP usage
- H264 encoder
- USB uvc (Azure RTOS USBX)
- Dev mode
- Boot from External Flash

## Software components

| Name                          | Version                         | Release notes
|-----                          | -------                         | -------------
| STM32Cube.AI runtime          | 10.2.0                          |
| Camera Middleware             | v1.4.3                          | [release notes](Lib/Camera_Middleware/Release_Notes.md)
| lib_vision_models_pp Library  | v0.10.0                         | [release notes](Lib/lib_vision_models_pp/lib_vision_models_pp/README.md)
| uvcl                          | v2.0.2                          | [release notes](Lib/uvcl/Release_Notes.html)
| post process wrapper          | v1.0.7                          | [release notes](Lib/ai-postprocessing-wrapper/Release_Notes.html)
| CMSIS                         | V5.9.0                          | [release notes](STM32Cube_FW_N6/Drivers/CMSIS/Documentation/index.html)
| STM32N6xx CMSIS Device        | V1.2.0                          | [release notes](STM32Cube_FW_N6/Drivers/CMSIS/Device/ST/STM32N6xx/Release_Notes.html)
| STM32N6xx HAL/LL Drivers      | V1.2.0                          | [release notes](STM32Cube_FW_N6/Drivers/STM32N6xx_HAL_Driver/Release_Notes.html)
| STM32N6570-DK BSP Drivers     | V1.2.0                          | [release notes](STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/Release_Notes.html)
| BSP Component aps256xx        | V1.0.6                          | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/aps256xx/Release_Notes.html)
| BSP Component Common          | V7.3.0                          | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/Common/Release_Notes.html)
| BSP Component mx66uw1g45g     | V1.1.0                          | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/mx66uw1g45g/Release_Notes.html)
| BSP Component rk050hr18       | V1.0.1                          | [release notes](STM32Cube_FW_N6/Drivers/BSP/Components/rk050hr18/Release_Notes.html)
| FreeRTOS kernel               | v10.6.2                         | [release notes](Lib/FreeRTOS/Source/History.txt)
| Azure RTOS USBX               | V6.4.0                          | [release notes](STM32Cube_FW_N6/Middlewares/ST/usbx/README.md)
|                               | ST modified 240906              | [ST release notes](STM32Cube_FW_N6/Middlewares/ST/usbx/st_readme.txt)
| VideoEncoder                  | V9.22.3.7 / ST modified 171224  | [ST release notes](STM32Cube_FW_N6/Middlewares/Third_Party/VideoEncoder/st_readme.txt)
| VideoEncoder_EWL              | V1.2.0                          | [release notes](STM32Cube_FW_N6/Middlewares/ST/VideoEncoder_EWL/Release_Notes.html)
| Fonts Utility                 | V2.0.3                          | [release notes](STM32Cube_FW_N6/Utilities/Fonts/Release_Notes.html)
| lcd Utility                   | V2.2.0                          | [release notes](STM32Cube_FW_N6/Utilities/lcd/Release_Notes.html)

## Update history

### V2.1.0 / August 2025

- Update the STM32CubeN6 Firmware Package to version 1.2.0.
- Update STEdgeAI to version 2.2.0.

### V2.0.0 / May 2025

- Replace Threadx by FreeRTOS

### V1.0.0 / December 2024

Initial Version

