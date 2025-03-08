---
pagetitle: Release Notes for IMX335 camera sensor Component Driver
lang: en
header-includes: <link rel="icon" type="image/x-icon" href="_htmresc/favicon.png" />
---
::: {.row}
::: {.col-sm-12 .col-lg-4}

<center>
# Release Notes for <mark>IMX335</mark> camera sensor Component Driver
Copyright &copy; 2023 STMicroelectronics\

[![ST logo](_htmresc/st_logo_2020.png)](https://www.st.com){.logo}
</center>

# Purpose

This driver provides a set of functions offered by IMX335 camera sensor component

:::

::: {.col-sm-12 .col-lg-8}
# Update history

::: {.collapse}
<input type="checkbox" id="collapse-section6" checked aria-hidden="true">
<label for="collapse-section6" aria-hidden="true">__V1.3.1-cmw-patch / November-2024__</label>
<div>

## Main changes

- Remove IMX335_CAMERA_Driver and IMX335_GetSensorInfo.
- Add start and stop functions.

</div>
:::


::: {.collapse}
<input type="checkbox" id="collapse-section5" aria-hidden="true">
<label for="collapse-section5" aria-hidden="true">__V1.3.1 / 22-October-2024__</label>
<div>

## Main changes

- Fix Misra-C:2012 compilation warnings

## Backward Compatibility

None

## Dependencies

- This software release is compatible with:
	- BSP Common v7.3.0 or above

</div>
:::


::: {.collapse}
<input type="checkbox" id="collapse-section4" aria-hidden="true">
<label for="collapse-section4" aria-hidden="true">__V1.3.0 / 21-June-2024__</label>
<div>

## Main changes

- Adjust the camera resolution to 2592x1944 (Sensor native)
- Add the GetSensorInfo API
- Support test pattern generation

## Backward Compatibility

None

## Dependencies

- This software release is compatible with:
	- BSP Common v7.3.0 or above

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section3" aria-hidden="true">
<label for="collapse-section3" aria-hidden="true">__V1.2.0 / 26-April-2024__</label>
<div>

## Main changes

- Add support of new services:
	- IMX335_SetFramerate()
	- IMX335_SetResolution()
	- IMX335_MirrorFlipConfig()
- Add 24 MHz as possible camera frequency

## Backward Compatibility

None

## Dependencies

- This software release is compatible with:
	- BSP Common v7.3.0 or above

</div>
:::


::: {.collapse}
<input type="checkbox" id="collapse-section2" aria-hidden="true">
<label for="collapse-section2" aria-hidden="true">__V1.1.0 / 28-September-2023__</label>
<div>

## Main changes

- Add support of new services:
	- IMX335_SetGain()
	- IMX335_SetExposure()
	- IMX335_SetFrequency

## Backward Compatibility

None

## Dependencies

- This software release is compatible with:
	- BSP Common v7.3.0 or above

</div>
:::

::: {.collapse}
<input type="checkbox" id="collapse-section1" aria-hidden="true">
<label for="collapse-section1" aria-hidden="true">__V1.0.0 / 01-February-2023__</label>
<div>

## Main changes

First Official release of IMX335 camera sensor Component drivers in line with STM32Cube BSP drivers development guidelines (UM2298)

## Contents

- The component drivers are composed of
	- component core drivers files: imx335.h/.c
	- component register drivers files: imx335_reg.h/.c

## Backward Compatibility

None

## Dependencies

- This software release is compatible with:
	- BSP Common v6.0.1 or above

</div>
:::

:::
:::

<footer class="sticky">
::: {.columns}
::: {.column width="95%"}
For complete documentation on **STM32 Microcontrollers** ,
visit: [[www.st.com](http://www.st.com/STM32)]
:::
::: {.column width="5%"}
<abbr title="Based on template cx566953 version 2.1">Info</abbr>
:::
:::
</footer>
