/**
  ******************************************************************************
  * @file    imx335_reg.h
  * @author  MCD Application Team
  * @brief   Header of imx335_reg.c
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef IMX335_REG_H
#define IMX335_REG_H

#include <cmsis_compiler.h>

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/** @addtogroup BSP
  * @{
  */

/** @addtogroup Components
  * @{
  */

/** @addtogroup IMX335
  * @{
  */

/** @defgroup IMX335_Exported_Types
  * @{
  */

/**
  * @}
  */

/** @defgroup IMX335_Exported_Constants IMX335 Exported Constants
  * @{
  */
#define IMX335_REG_MODE_SELECT    0x3000
#define IMX335_MODE_STREAMING       0x00
#define IMX335_MODE_STANDBY         0x01

#define IMX335_REG_HOLD           0x3001
#define IMX335_REG_VMAX           0x3030
#define IMX335_REG_SHUTTER        0x3058
#define IMX335_REG_GAIN           0x30e8
#define IMX335_REG_TPG            0x329e

#define IMX335_REG_ID             0x3912
#define IMX335_CHIP_ID            0x00

#define IMX335_SHUTTER_MIN        9

#define IMX335_EXPOSURE_DEFAULT   23814

#define IMX335_NAME               "IMX335"
#define IMX335_BAYER_PATTERN      0 /* From ISP definition RGGB / TODO comnon enumeration in camera */
#define IMX335_COLOR_DEPTH        10 /* in bits */
#define IMX335_GAIN_MIN           (0 * 1000)
#define IMX335_GAIN_MAX           (72 * 1000)
#define IMX335_GAIN_DEFAULT       (20 * 1000)
#define IMX335_GAIN_UNIT_MDB      300
#define IMX335_EXPOSURE_MIN       0           /* in us */
#define IMX335_EXPOSURE_MAX       33266       /* in us, for sensor @30fps */



#define IMX335_REG_HREVERSE       0x304EU
#define IMX335_REG_VREVERSE       0x304FU
#define AREA3_ST_ADR_1_LSB        0x3074U
#define AREA3_ST_ADR_1_MSB        0x3075U

/* For 2592x1944 */
#define IMX335_WIDTH              2592
#define IMX335_HEIGHT             1944
#define IMX335_PCLK               396000000

/**
  * @}
  */

/************** Generic Function  *******************/

typedef int32_t (*IMX335_Write_Func)(void *, uint16_t, uint8_t*, uint16_t);
typedef int32_t (*IMX335_Read_Func) (void *, uint16_t, uint8_t*, uint16_t);

typedef struct
{
  IMX335_Write_Func   WriteReg;
  IMX335_Read_Func    ReadReg;
  void                *handle;
} imx335_ctx_t;

/*******************************************************************************
* Register      : Generic - All
* Address       : Generic - All
* Bit Group Name: None
* Permission    : W
*******************************************************************************/
int32_t imx335_write_reg(imx335_ctx_t *ctx, uint16_t reg, uint8_t *pdata, uint16_t length);
int32_t imx335_read_reg(imx335_ctx_t *ctx, uint16_t reg, uint8_t *pdata, uint16_t length);

int32_t imx335_register_set(imx335_ctx_t *ctx, uint16_t reg, uint8_t value);


/**
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif /* IMX335_REG_H */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
