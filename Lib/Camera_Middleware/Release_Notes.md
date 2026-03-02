# Release Notes for STM32 Camera Middleware

## Purpose

The Camera Middleware serves as an intermediary layer between camera sensor drivers and user applications.
It provides a streamlined interface for initializing sensors, configuring camera pipelines, and managing data streaming.
This middleware simplifies the development process for applications that require camera functionality by abstracting hardware-specific details.

## Key Features

- Support for the STM32N6570-DK board only
- Support for the following cameras:
  - MB1854B IMX335 Camera module
  - ST VD66GY Camera module
  - ST VD55G1 Camera module
- Use ISP Library for ST VD66GY.
- Use ISP Library for IMX335.
- Enable APIs to Init and Start camera pipelines with CSI-DCMIPP.

## Software components

| Name           | Version                  | Release notes
|-----           | -------                  | -------------
| Isp Library    | v1.2.0                   | [release notes](ISP_Library/README.md)
| imx335 driver  | v1.3.1-cmw-patch-2      | [release notes](sensors/imx335/Release_Notes.html)
| vd6g driver    | v1.0.0                   |
| vd55g1 driver  | v1.0.0                   |

## Supported Devices and Boards

- STM32N6xx devices
- MB1939 STM32N6570-DK revC

## Update history

### V1.4.3 / July 2025

- Add support of custom configuration of sensors
- Modify Init API accordingly
- Update Isp library to v1.2.0


### V1.4.2 / April 2025

- Update Isp library to v1.1.0
- Add support for white balance mode listing
- Add support for manual white balance ref mod

### V1.4.1 / December 2024

Initial Version

