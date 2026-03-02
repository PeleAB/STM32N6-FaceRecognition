# Release Notes for STM32N6 Uvc Library (aka uvcl)

## Purpose

Uvcl will allows you to easily add Usb Uvc support to your N6 application. It has a very simple API and can be
configured at compile time

## Key Features

- Use either USBX or STM32_USBD usb stack
- Can be use either with threadx, freertos or without any rtos
- Can use usb dma engine or not
- Support various payloads type

## Software components

None

## Update history

### V3.0.0 / ????

- Add support to expose multiple payload/resolution/frame rate

### V2.0.2 / May 2025

- Update release notes format

### V2.0.1 / March 2025

- Fix HAL_PCD_MspInit() typical implementation.

### V2.0.0 / March 2025

- Add FREERTOS support
- Add more payloads type (UVCL_PAYLOAD_FB_JPEG and UVCL_PAYLOAD_FB_GREY_D3DFMT_L8)
- Remove SOF usage for USBX in standalone mode
- Fix wrong dwMaxVideoFrameSize value in descriptor for USBX

### V1.0.0 / December 2024

Initial release

