# Middleware FreeRTOS MCU Component

![latest tag](https://img.shields.io/github/v/tag/STMicroelectronics/stm32_mw_freertos.svg?color=brightgreen)

## Overview

**STM32Cube** is an STMicroelectronics original initiative to ease developers' life by reducing efforts, time and cost.

**STM32Cube** covers the overall STM32 products portfolio. It includes a comprehensive embedded software platform delivered for each STM32 series.
   * The CMSIS modules (core and device) corresponding to the ARM(tm) core implemented in this STM32 product.
   * The STM32 HAL-LL drivers, an abstraction layer offering a set of APIs ensuring maximized portability across the STM32 portfolio.
   * The BSP drivers of each evaluation, discovery, or nucleo board provided for this STM32 series.
   * A consistent set of middleware libraries such as RTOS, USB, FatFS, graphics, touch sensing library...
   * A full set of software projects (basic examples, applications, and demonstrations) for each board provided for this STM32 series.

Two models of publication are proposed for the STM32Cube embedded software:
   * The monolithic **MCU Package**: all STM32Cube software modules of one STM32 series are present (Drivers, Middleware, Projects, Utilities) in the repository (usual name **STM32Cubexx**, xx corresponding to the STM32 series).
   * The **MCU component**: each STM32Cube software module being part of the STM32Cube MCU Package, is delivered as an individual repository, allowing the user to select and get only the required software functions.

## Description

This **stm32_mw_freertos** MCU component repository is one element **common to all** STM32Cube MCU embedded software packages, providing the **FreeRTOS Middleware** part.

## Release note

* Details about the content of this release are available in the release note [here](./Source/History.txt).
* Details about the updates made by STMicroelectronics are available in the release note [here](./Source/st_readme.txt).

## Compatibility information

Please refer to the release note in the repository of the STM32Cube **firmware** you are using to know which version of this middleware library to use with other components' versions (**e.g.**, other middleware libraries, drivers). It is **crucial** that you use a consistent set of versions.

## Troubleshooting

Please refer to the [CONTRIBUTING.md](CONTRIBUTING.md) guide.

