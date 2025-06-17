#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#include "stm32n6xx_hal.h"

void SystemClock_Config(void);
void NPURam_enable(void);
void NPUCache_config(void);
void Security_Config(void);
void set_clk_sleep_mode(void);
void IAC_Config(void);
HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp);

#endif /* SYSTEM_UTILS_H */
