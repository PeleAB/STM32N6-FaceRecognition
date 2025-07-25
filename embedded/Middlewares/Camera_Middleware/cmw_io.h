 /**
 ******************************************************************************
 * @file    cmw_io.h
 * @author  GPM Application Team
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CMW_IO_H
#define CMW_IO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Sensors parameters */
#define CAMERA_IMX335_ADDRESS          0x34U
#define CAMERA_VD66GY_ADDRESS          0x20U
#define CAMERA_VD66GY_FREQ_IN_HZ       12000000U
#define CAMERA_VD55G1_ADDRESS          0x20U
#define CAMERA_VD55G1_FREQ_IN_HZ       12000000U

#if defined (STM32N657xx)

#ifdef STM32N6570_NUCLEO_REV
/* Nucleo board */
  #define NRST_CAM_PIN                    GPIO_PIN_5
  #define NRST_CAM_PORT                   GPIOO
  #define NRST_CAM_GPIO_ENABLE_VDDIO()    HAL_PWREx_EnableVddIO2();
  #define NRST_CAM_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOO_CLK_ENABLE()

  #define EN_CAM_PIN                      GPIO_PIN_0
  #define EN_CAM_PORT                     GPIOA
  /* PD10 on VDD Core */
  #define EN_CAM_GPIO_ENABLE_VDDIO()
  #define EN_CAM_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOA_CLK_ENABLE()
#else
/* Discovery board */
  #define NRST_CAM_PIN                    GPIO_PIN_8
  #define NRST_CAM_PORT                   GPIOC
  #define NRST_CAM_GPIO_ENABLE_VDDIO()    HAL_PWREx_EnableVddIO4();
  #define NRST_CAM_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE();

  #define EN_CAM_PIN                      GPIO_PIN_2
  #define EN_CAM_PORT                     GPIOD
  /* PD2 on VDD Core */
  #define EN_CAM_GPIO_ENABLE_VDDIO()
  #define EN_CAM_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOD_CLK_ENABLE()
#endif /* STM32N6570_NUCLEO_REV */

#ifdef STM32N6570_NUCLEO_REV
/* Nucleo board */
  #define CMW_I2C_INIT BSP_I2C2_Init
  #define CMW_I2C_DEINIT BSP_I2C2_DeInit
  #define CMW_I2C_READREG16 BSP_I2C2_ReadReg16
  #define CMW_I2C_WRITEREG16 BSP_I2C2_WriteReg16
#else
/* Discovery board */
  #define CMW_I2C_INIT BSP_I2C1_Init
  #define CMW_I2C_DEINIT BSP_I2C1_DeInit
  #define CMW_I2C_READREG16 BSP_I2C1_ReadReg16
  #define CMW_I2C_WRITEREG16 BSP_I2C1_WriteReg16
#endif /* STM32N6570_NUCLEO_REV */

#define CSI2_CLK_ENABLE()               __HAL_RCC_CSI_CLK_ENABLE()
#define CSI2_CLK_SLEEP_DISABLE()        __HAL_RCC_CSI_CLK_SLEEP_DISABLE()
#define CSI2_CLK_FORCE_RESET()          __HAL_RCC_CSI_FORCE_RESET()
#define CSI2_CLK_RELEASE_RESET()        __HAL_RCC_CSI_RELEASE_RESET()

#else
#error Add camera configuration GPIO for your specific board
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CMW_IO_H */
