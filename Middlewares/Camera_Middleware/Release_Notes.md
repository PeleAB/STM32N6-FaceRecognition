---
pagetitle: Release Notes for STM32 Camera Middleware
lang: en
header-includes: <link rel="icon" type="image/x-icon" href="_htmresc/favicon.png" />
---

::: {.row}
::: {.col-sm-12 .col-lg-4}

<center>
# Release Notes for <mark>STM32 Camera Middleware</mark>
Copyright &copy; 2024 STMicroelectronics\

[![ST logo](_htmresc/st_logo_2020.png)](https://www.st.com){.logo}
</center>

# Purpose

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

:::

::: {.col-sm-12 .col-lg-8}
# Update history

::: {.collapse}
<input type="checkbox" id="collapse-section5" checked aria-hidden="true"/>
<label for="collapse-section5" aria-hidden="true">__V1.4.1 / December 2024__</label>
<div>

## Software components

| Name           | Version                  | Release notes
|-----           | -------                  | -------------
| Isp Library    | v1.0.1                   | [release notes](ISP_Library/README.md)
| imx335 driver  | v1.3.1-cmw-custom        | [release notes](sensors/imx335/Release_Notes.html)
| vd6g driver    | v1.0.0                   |
| vd55g1 driver  | v1.0.0                   |

## Supported Devices and Boards

- STM32N6xx devices
- MB1939 STM32N6570-DK revC

</div>
:::

:::
:::

<footer class="sticky">
::: {.columns}
::: {.column width="95%"}
For complete information on __X-CUBE-AI__ ,
visit: [X-CUBE-AI AI expansion pack for STM32CubeMX on st.com](https://www.st.com/content/st_com/en/products/embedded-software/mcu-mpu-embedded-software/stm32-embedded-software/stm32cube-expansion-packages/x-cube-ai.html) and [X-CUBE-AI Documentation](Documentation/index.html)
:::
:::
</footer>
