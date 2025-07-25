/**
  ******************************************************************************
  * @file    stm32n6570_discovery_xspi.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32n6570_discovery_xspi.c driver.
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
#ifndef STM32N6570_DISCOVERY_XSPI_H
#define STM32N6570_DISCOVERY_XSPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32n6570_discovery_conf.h"
#include "stm32n6570_discovery_errno.h"

#ifndef USE_NOR_MEMORY_MX66UW1G45G
#define USE_NOR_MEMORY_MX66UW1G45G           1
#endif /* USE_NOR_MEMORY_MX66UW1G45G */

#ifndef USE_RAM_MEMORY_APS256XX
#define USE_RAM_MEMORY_APS256XX              1
#endif /* USE_RAM_MEMORY_APS256XX */

#if (USE_NOR_MEMORY_MX66UW1G45G == 1)
#include "../Components/mx66uw1g45g/mx66uw1g45g.h"
#endif /* (USE_NOR_MEMORY_MX66UW1G45G == 1) */

#if (USE_RAM_MEMORY_APS256XX == 1)
#include "../Components/aps256xx/aps256xx.h"
#endif /* (USE_RAM_MEMORY_APS256XX == 1) */

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32N6570_DK
  * @{
  */

/** @addtogroup STM32N6570_DK_XSPI
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup STM32N6570_DK_XSPI_Exported_Types XSPI Exported Types
  * @{
  */
typedef enum
{
  XSPI_ACCESS_NONE = 0,          /*!<  Instance not initialized,              */
  XSPI_ACCESS_INDIRECT,          /*!<  Instance use indirect mode access      */
  XSPI_ACCESS_MMP                /*!<  Instance use Memory Mapped Mode read   */
} XSPI_Access_t;

#if (USE_HAL_XSPI_REGISTER_CALLBACKS == 1)
typedef struct
{
  pXSPI_CallbackTypeDef  pMspInitCb;
  pXSPI_CallbackTypeDef  pMspDeInitCb;
} BSP_XSPI_Cb_t;
#endif /* (USE_HAL_XSPI_REGISTER_CALLBACKS == 1) */

typedef struct
{
  uint32_t MemorySize;
  uint32_t ClockPrescaler;
  uint32_t SampleShifting;
  uint32_t TransferRate;
} MX_XSPI_InitTypeDef;
/**
  * @}
  */

#if (USE_NOR_MEMORY_MX66UW1G45G == 1)
/** @defgroup STM32N6570_DK_XSPI_NOR_Exported_Types XSPI_NOR Exported Types
  * @{
  */
#define BSP_XSPI_NOR_Info_t                MX66UW1G45G_Info_t
#define BSP_XSPI_NOR_Interface_t           MX66UW1G45G_Interface_t
#define BSP_XSPI_NOR_Transfer_t            MX66UW1G45G_Transfer_t
#define BSP_XSPI_NOR_Erase_t               MX66UW1G45G_Erase_t

typedef struct
{
  XSPI_Access_t              IsInitialized;  /*!<  Instance access Flash method     */
  BSP_XSPI_NOR_Interface_t   InterfaceMode;  /*!<  Flash Interface mode of Instance */
  BSP_XSPI_NOR_Transfer_t    TransferRate;   /*!<  Flash Transfer mode of Instance  */
} XSPI_NOR_Ctx_t;

typedef struct
{
  BSP_XSPI_NOR_Interface_t   InterfaceMode;      /*!<  Current Flash Interface mode */
  BSP_XSPI_NOR_Transfer_t    TransferRate;       /*!<  Current Flash Transfer rate  */
} BSP_XSPI_NOR_Init_t;
/**
  * @}
  */
#endif /* (USE_NOR_MEMORY_MX66UW1G45G == 1) */

#if (USE_RAM_MEMORY_APS256XX == 1)
/** @defgroup STM32N6570_DK_XSPI_RAM_Exported_Types XSPI_RAM Exported Types
  * @{
  */
#define BSP_XSPI_RAM_BurstLength_t APS256XX_BurstLength_t

typedef enum
{
  BSP_XSPI_RAM_VARIABLE_LATENCY = HAL_XSPI_VARIABLE_LATENCY,
  BSP_XSPI_RAM_FIXED_LATENCY    = HAL_XSPI_FIXED_LATENCY
} BSP_XSPI_RAM_Latency_t;

typedef enum
{
  BSP_XSPI_RAM_HYBRID_BURST = 0,
  BSP_XSPI_RAM_LINEAR_BURST
} BSP_XSPI_RAM_BurstType_t;

typedef struct
{
  XSPI_Access_t               IsInitialized;   /*!< Instance access Flash method */
  BSP_XSPI_RAM_Latency_t      LatencyType;   /*!< Latency Type of Instance     */
  BSP_XSPI_RAM_BurstType_t    BurstType;     /*!< Burst Type of Instance       */
  BSP_XSPI_RAM_BurstLength_t  BurstLength;   /*!< Burst Length of Instance     */
} XSPI_RAM_Ctx_t;

/**
  * @}
  */
#endif /* (USE_RAM_MEMORY_APS256XX == 1) */

/* Exported constants --------------------------------------------------------*/
/** @defgroup STM32N6570_DK_XSPI_Exported_Constants XSPI Exported Constants
  * @{
  */
#if (USE_NOR_MEMORY_MX66UW1G45G == 1)
/* Definition for XSPIM NOR power resources */
#define XSPI_NOR_PWR_ENABLE()                 HAL_PWREx_EnableXSPIM2()

/* Definition for XSPI NOR clock resources */
#define XSPI_NOR_CLK_ENABLE()                 __HAL_RCC_XSPI2_CLK_ENABLE()
#define XSPI_NOR_CLK_DISABLE()                __HAL_RCC_XSPI2_CLK_DISABLE()

#define XSPI_NOR_CLK_GPIO_CLK_ENABLE()        __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_NOR_DQS_GPIO_CLK_ENABLE()        __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_NOR_CS_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_NOR_D0_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_NOR_D1_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_NOR_D2_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_NOR_D3_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_NOR_D4_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_NOR_D5_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_NOR_D6_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_NOR_D7_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()

#define XSPI_NOR_FORCE_RESET()                __HAL_RCC_XSPI2_FORCE_RESET()
#define XSPI_NOR_RELEASE_RESET()              __HAL_RCC_XSPI2_RELEASE_RESET()

/* Definition for NOR XSPI Pins */
/* XSPI_CLK */
#define XSPI_NOR_CLK_PIN                      GPIO_PIN_6
#define XSPI_NOR_CLK_GPIO_PORT                GPION
#define XSPI_NOR_CLK_PIN_AF                   GPIO_AF9_XSPIM_P2
/* XSPI_DQS */
#define XSPI_NOR_DQS_PIN                      GPIO_PIN_0
#define XSPI_NOR_DQS_GPIO_PORT                GPION
#define XSPI_NOR_DQS_PIN_AF                   GPIO_AF9_XSPIM_P2
/* XSPI_CS */
#define XSPI_NOR_CS_PIN                       GPIO_PIN_1
#define XSPI_NOR_CS_GPIO_PORT                 GPION
#define XSPI_NOR_CS_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D0 */
#define XSPI_NOR_D0_PIN                       GPIO_PIN_2
#define XSPI_NOR_D0_GPIO_PORT                 GPION
#define XSPI_NOR_D0_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D1 */
#define XSPI_NOR_D1_PIN                       GPIO_PIN_3
#define XSPI_NOR_D1_GPIO_PORT                 GPION
#define XSPI_NOR_D1_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D2 */
#define XSPI_NOR_D2_PIN                       GPIO_PIN_4
#define XSPI_NOR_D2_GPIO_PORT                 GPION
#define XSPI_NOR_D2_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D3 */
#define XSPI_NOR_D3_PIN                       GPIO_PIN_5
#define XSPI_NOR_D3_GPIO_PORT                 GPION
#define XSPI_NOR_D3_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D4 */
#define XSPI_NOR_D4_PIN                       GPIO_PIN_8
#define XSPI_NOR_D4_GPIO_PORT                 GPION
#define XSPI_NOR_D4_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D5 */
#define XSPI_NOR_D5_PIN                       GPIO_PIN_9
#define XSPI_NOR_D5_GPIO_PORT                 GPION
#define XSPI_NOR_D5_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D6 */
#define XSPI_NOR_D6_PIN                       GPIO_PIN_10
#define XSPI_NOR_D6_GPIO_PORT                 GPION
#define XSPI_NOR_D6_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D7 */
#define XSPI_NOR_D7_PIN                       GPIO_PIN_11
#define XSPI_NOR_D7_GPIO_PORT                 GPION
#define XSPI_NOR_D7_PIN_AF                    GPIO_AF9_XSPIM_P2
#endif /* (USE_NOR_MEMORY_MX66UW1G45G == 1) */

#if (USE_RAM_MEMORY_APS256XX == 1)
/* Definition for XSPI RAM power resources */
#define XSPI_RAM_PWR_ENABLE()                 HAL_PWREx_EnableXSPIM1()

/* Definition for XSPI RAM clock resources */
#define XSPI_RAM_CLK_ENABLE()                 __HAL_RCC_XSPI1_CLK_ENABLE()
#define XSPI_RAM_CLK_DISABLE()                __HAL_RCC_XSPI1_CLK_DISABLE()

#define XSPI_RAM_CLK_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOO_CLK_ENABLE()
#define XSPI_RAM_DQS_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOO_CLK_ENABLE()
#define XSPI_RAM_CS_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOO_CLK_ENABLE()
#define XSPI_RAM_D0_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI_RAM_D1_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI_RAM_D2_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI_RAM_D3_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI_RAM_D4_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI_RAM_D5_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI_RAM_D6_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI_RAM_D7_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()

#define XSPI_RAM_FORCE_RESET()                __HAL_RCC_XSPI1_FORCE_RESET()
#define XSPI_RAM_RELEASE_RESET()              __HAL_RCC_XSPI1_RELEASE_RESET()

/* Definition for XSPI RAM Pins */
/* XSPI_CLK */
#define XSPI_RAM_CLK_PIN                      GPIO_PIN_4
#define XSPI_RAM_CLK_GPIO_PORT                GPIOO
#define XSPI_RAM_CLK_PIN_AF                   GPIO_AF9_XSPIM_P1
/* XSPI_DQS0 */
#define XSPI_RAM_DQS0_PIN                     GPIO_PIN_2
#define XSPI_RAM_DQS0_GPIO_PORT               GPIOO
#define XSPI_RAM_DQS0_PIN_AF                  GPIO_AF9_XSPIM_P1
/* XSPI_DQS1 */
#define XSPI_RAM_DQS1_PIN                      GPIO_PIN_3
#define XSPI_RAM_DQS1_GPIO_PORT                GPIOO
#define XSPI_RAM_DQS1_PIN_AF                   GPIO_AF9_XSPIM_P1
/* XSPI_CS */
#define XSPI_RAM_CS_PIN                       GPIO_PIN_0
#define XSPI_RAM_CS_GPIO_PORT                 GPIOO
#define XSPI_RAM_CS_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D0 */
#define XSPI_RAM_D0_PIN                       GPIO_PIN_0
#define XSPI_RAM_D0_GPIO_PORT                 GPIOP
#define XSPI_RAM_D0_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D1 */
#define XSPI_RAM_D1_PIN                       GPIO_PIN_1
#define XSPI_RAM_D1_GPIO_PORT                 GPIOP
#define XSPI_RAM_D1_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D2 */
#define XSPI_RAM_D2_PIN                       GPIO_PIN_2
#define XSPI_RAM_D2_GPIO_PORT                 GPIOP
#define XSPI_RAM_D2_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D3 */
#define XSPI_RAM_D3_PIN                       GPIO_PIN_3
#define XSPI_RAM_D3_GPIO_PORT                 GPIOP
#define XSPI_RAM_D3_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D4 */
#define XSPI_RAM_D4_PIN                       GPIO_PIN_4
#define XSPI_RAM_D4_GPIO_PORT                 GPIOP
#define XSPI_RAM_D4_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D5 */
#define XSPI_RAM_D5_PIN                       GPIO_PIN_5
#define XSPI_RAM_D5_GPIO_PORT                 GPIOP
#define XSPI_RAM_D5_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D6 */
#define XSPI_RAM_D6_PIN                       GPIO_PIN_6
#define XSPI_RAM_D6_GPIO_PORT                 GPIOP
#define XSPI_RAM_D6_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D7 */
#define XSPI_RAM_D7_PIN                       GPIO_PIN_7
#define XSPI_RAM_D7_GPIO_PORT                 GPIOP
#define XSPI_RAM_D7_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D8 */
#define XSPI_RAM_D8_PIN                       GPIO_PIN_8
#define XSPI_RAM_D8_GPIO_PORT                 GPIOP
#define XSPI_RAM_D8_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D9 */
#define XSPI_RAM_D9_PIN                       GPIO_PIN_9
#define XSPI_RAM_D9_GPIO_PORT                 GPIOP
#define XSPI_RAM_D9_PIN_AF                    GPIO_AF9_XSPIM_P1
/* XSPI_D10 */
#define XSPI_RAM_D10_PIN                      GPIO_PIN_10
#define XSPI_RAM_D10_GPIO_PORT                GPIOP
#define XSPI_RAM_D10_PIN_AF                   GPIO_AF9_XSPIM_P1
/* XSPI_D11 */
#define XSPI_RAM_D11_PIN                      GPIO_PIN_11
#define XSPI_RAM_D11_GPIO_PORT                GPIOP
#define XSPI_RAM_D11_PIN_AF                   GPIO_AF9_XSPIM_P1
/* XSPI_D12 */
#define XSPI_RAM_D12_PIN                      GPIO_PIN_12
#define XSPI_RAM_D12_GPIO_PORT                GPIOP
#define XSPI_RAM_D12_PIN_AF                   GPIO_AF9_XSPIM_P1
/* XSPI_D13 */
#define XSPI_RAM_D13_PIN                      GPIO_PIN_13
#define XSPI_RAM_D13_GPIO_PORT                GPIOP
#define XSPI_RAM_D13_PIN_AF                   GPIO_AF9_XSPIM_P1
/* XSPI_D14 */
#define XSPI_RAM_D14_PIN                      GPIO_PIN_14
#define XSPI_RAM_D14_GPIO_PORT                GPIOP
#define XSPI_RAM_D14_PIN_AF                   GPIO_AF9_XSPIM_P1
/* XSPI_D15 */
#define XSPI_RAM_D15_PIN                      GPIO_PIN_15
#define XSPI_RAM_D15_GPIO_PORT                GPIOP
#define XSPI_RAM_D15_PIN_AF                   GPIO_AF9_XSPIM_P1
#endif /* (USE_RAM_MEMORY_APS256XX == 1) */
/**
  * @}
  */

#if (USE_NOR_MEMORY_MX66UW1G45G == 1)
/** @defgroup STM32N6570_DK_XSPI_NOR_Exported_Constants XSPI_NOR Exported Constants
  * @{
  */
#define XSPI_NOR_INSTANCES_NUMBER         1U

/* Definition for XSPI modes */
#define BSP_XSPI_NOR_SPI_MODE (BSP_XSPI_NOR_Interface_t)MX66UW1G45G_SPI_MODE /* 1 Cmd, 1 Address and 1 Data Lines */
#define BSP_XSPI_NOR_OPI_MODE (BSP_XSPI_NOR_Interface_t)MX66UW1G45G_OPI_MODE /* 8 Cmd, 8 Address and 8 Data Lines */

/* Definition for XSPI transfer rates */
#define BSP_XSPI_NOR_STR_TRANSFER  (BSP_XSPI_NOR_Transfer_t)MX66UW1G45G_STR_TRANSFER   /* Single Transfer Rate */
#define BSP_XSPI_NOR_DTR_TRANSFER  (BSP_XSPI_NOR_Transfer_t)MX66UW1G45G_DTR_TRANSFER   /* Double Transfer Rate */

/* XSPI erase types */
#define BSP_XSPI_NOR_ERASE_4K             MX66UW1G45G_ERASE_4K
#define BSP_XSPI_NOR_ERASE_64K            MX66UW1G45G_ERASE_64K
#define BSP_XSPI_NOR_ERASE_CHIP           MX66UW1G45G_ERASE_BULK

/* XSPI block sizes */
#define BSP_XSPI_NOR_BLOCK_4K             MX66UW1G45G_SUBSECTOR_4K
#define BSP_XSPI_NOR_BLOCK_64K            MX66UW1G45G_SECTOR_64K
/**
  * @}
  */
#endif /* (USE_NOR_MEMORY_MX66UW1G45G == 1) */

#if (USE_RAM_MEMORY_APS256XX == 1)
/** @defgroup STM32N6570_DK_XSPI_RAM_Exported_Constants XSPI_RAM Exported Constants
  * @{
  */
#define XSPI_RAM_INSTANCES_NUMBER         1U

/* XSPI Burst length */
#define BSP_XSPI_RAM_BURST_16_BYTES       (BSP_XSPI_RAM_BurstLength_t)APS256XX_BURST_16_BYTES
#define BSP_XSPI_RAM_BURST_32_BYTES       (BSP_XSPI_RAM_BurstLength_t)APS256XX_BURST_32_BYTES
#define BSP_XSPI_RAM_BURST_64_BYTES       (BSP_XSPI_RAM_BurstLength_t)APS256XX_BURST_64_BYTES
#define BSP_XSPI_RAM_BURST_128_BYTES      (BSP_XSPI_RAM_BurstLength_t)APS256XX_BURST_128_BYTES
/**
  * @}
  */
#endif /* (USE_RAM_MEMORY_APS256XX == 1) */

/* Exported variables --------------------------------------------------------*/

#if (USE_NOR_MEMORY_MX66UW1G45G == 1)
/** @defgroup STM32N6570_DK_XSPI_NOR_Exported_Variables XSPI_NOR Exported Variables
  * @{
  */
extern XSPI_HandleTypeDef hxspi_nor[XSPI_NOR_INSTANCES_NUMBER];
extern XSPI_NOR_Ctx_t Ospi_Nor_Ctx[XSPI_NOR_INSTANCES_NUMBER];
/**
  * @}
  */
#endif /* (USE_NOR_MEMORY_MX66UW1G45G == 1) */

#if (USE_RAM_MEMORY_APS256XX == 1)
/** @defgroup STM32N6570_DK_XSPI_RAM_Exported_Variables XSPI_RAM Exported Variables
  * @{
  */
extern XSPI_HandleTypeDef hxspi_ram[XSPI_RAM_INSTANCES_NUMBER];
extern XSPI_RAM_Ctx_t Ospi_Ram_Ctx[XSPI_RAM_INSTANCES_NUMBER];
/**
  * @}
  */
#endif /* (USE_RAM_MEMORY_APS256XX == 1) */

/* Exported functions --------------------------------------------------------*/

#if (USE_NOR_MEMORY_MX66UW1G45G == 1)
/** @defgroup STM32N6570_DK_XSPI_NOR_Exported_Functions XSPI_NOR Exported Functions
  * @{
  */
int32_t BSP_XSPI_NOR_Init(uint32_t Instance, BSP_XSPI_NOR_Init_t *Init);
int32_t BSP_XSPI_NOR_DeInit(uint32_t Instance);
#if (USE_HAL_XSPI_REGISTER_CALLBACKS == 1)
int32_t BSP_XSPI_NOR_RegisterMspCallbacks(uint32_t Instance, BSP_XSPI_Cb_t *CallBacks);
int32_t BSP_XSPI_NOR_RegisterDefaultMspCallbacks(uint32_t Instance);
#endif /* (USE_HAL_XSPI_REGISTER_CALLBACKS == 1) */
int32_t BSP_XSPI_NOR_Read(uint32_t Instance, uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
int32_t BSP_XSPI_NOR_Write(uint32_t Instance, const uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
int32_t BSP_XSPI_NOR_Erase_Block(uint32_t Instance, uint32_t BlockAddress, BSP_XSPI_NOR_Erase_t BlockSize);
int32_t BSP_XSPI_NOR_Erase_Chip(uint32_t Instance);
int32_t BSP_XSPI_NOR_GetStatus(uint32_t Instance);
int32_t BSP_XSPI_NOR_GetInfo(uint32_t Instance, BSP_XSPI_NOR_Info_t *pInfo);
int32_t BSP_XSPI_NOR_EnableMemoryMappedMode(uint32_t Instance);
int32_t BSP_XSPI_NOR_DisableMemoryMappedMode(uint32_t Instance);
int32_t BSP_XSPI_NOR_ReadID(uint32_t Instance, uint8_t *Id);
int32_t BSP_XSPI_NOR_ConfigFlash(uint32_t Instance, BSP_XSPI_NOR_Interface_t Mode, BSP_XSPI_NOR_Transfer_t Rate);
int32_t BSP_XSPI_NOR_SuspendErase(uint32_t Instance);
int32_t BSP_XSPI_NOR_ResumeErase(uint32_t Instance);
int32_t BSP_XSPI_NOR_EnterDeepPowerDown(uint32_t Instance);
int32_t BSP_XSPI_NOR_LeaveDeepPowerDown(uint32_t Instance);

/**
  * @}
  */
#endif /* (USE_NOR_MEMORY_MX66UW1G45G == 1) */

#if (USE_RAM_MEMORY_APS256XX == 1)
/** @defgroup STM32N6570_DK_XSPI_RAM_Exported_Functions XSPI_RAM Exported Functions
  * @{
  */
int32_t BSP_XSPI_RAM_Init(uint32_t Instance);
int32_t BSP_XSPI_RAM_DeInit(uint32_t Instance);
#if (USE_HAL_XSPI_REGISTER_CALLBACKS == 1)
int32_t BSP_XSPI_RAM_RegisterMspCallbacks(uint32_t Instance, BSP_XSPI_Cb_t *CallBacks);
int32_t BSP_XSPI_RAM_RegisterDefaultMspCallbacks(uint32_t Instance);
#endif /* (USE_HAL_XSPI_REGISTER_CALLBACKS == 1) */
int32_t BSP_XSPI_RAM_Read(uint32_t Instance, uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
int32_t BSP_XSPI_RAM_Write(uint32_t Instance, uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
int32_t BSP_XSPI_RAM_EnableMemoryMappedMode(uint32_t Instance);
int32_t BSP_XSPI_RAM_DisableMemoryMappedMode(uint32_t Instance);
int32_t BSP_XSPI_RAM_ReadID(uint32_t Instance, uint8_t *Id);
/**
  * @}
  */
#endif /* (USE_RAM_MEMORY_APS256XX == 1) */


/** @defgroup STM32N6570_DK_XSPI_Exported_Init_Functions XSPI_Init Exported Functions
  * @{
  */

/* These functions can be modified in case the current settings
   need to be changed for specific application needs */

#if (USE_NOR_MEMORY_MX66UW1G45G == 1)
HAL_StatusTypeDef MX_XSPI_NOR_Init(XSPI_HandleTypeDef *hxspi, MX_XSPI_InitTypeDef *Init);
#endif /* (USE_NOR_MEMORY_MX66UW1G45G == 1) */

#if (USE_RAM_MEMORY_APS256XX == 1)
HAL_StatusTypeDef MX_XSPI_RAM_Init(XSPI_HandleTypeDef *hxspi, MX_XSPI_InitTypeDef *Init);
#endif /* (USE_RAM_MEMORY_APS256XX == 1) */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32N6570_DISCOVERY_XSPI_H */
