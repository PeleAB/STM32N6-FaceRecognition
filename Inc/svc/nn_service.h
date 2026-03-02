/**
 ******************************************************************************
 * @file    nn_service.h
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

#ifndef NN_SERVICE_H
#define NN_SERVICE_H

#include <stdint.h>

#include "ll_aton_rt_user_api.h"

#define NN_SERVICE_MAX_MODELS 4
#define NN_SERVICE_INVALID_HANDLE (-1)

typedef int nn_service_handle_t;

typedef enum
{
  NN_SERVICE_OK = 0,
  NN_SERVICE_ERR_INIT = -1,
  NN_SERVICE_ERR_ARGS = -2,
  NN_SERVICE_ERR_FULL = -3,
  NN_SERVICE_ERR_NO_BUFFERS = -4,
  NN_SERVICE_ERR_IO = -5,
} nn_service_status_t;

typedef struct
{
  const char *name;
  NN_Instance_TypeDef *instance;
  uint32_t postprocess_type;
} nn_service_model_cfg_t;

typedef struct
{
  nn_service_handle_t handle;
  const char *name;
  NN_Instance_TypeDef *instance;
  uint32_t user_input_size;
  uint32_t user_output_size;
  uint32_t user_input_count;
  uint32_t user_output_count;
  uint32_t postprocess_type;
  uint8_t is_initialized;
} nn_service_model_t;

nn_service_status_t nn_service_init(void);
nn_service_status_t nn_service_register(const nn_service_model_cfg_t *cfg, nn_service_handle_t *out_handle);
nn_service_status_t nn_service_select(nn_service_handle_t handle);
const nn_service_model_t *nn_service_active(void);
const nn_service_model_t *nn_service_get(nn_service_handle_t handle);
nn_service_status_t nn_service_prepare_io(uint8_t *input, uint32_t input_len, uint8_t *output, uint32_t output_len);
uint32_t nn_service_max_input_size(void);
uint32_t nn_service_max_output_size(void);
uint32_t nn_service_count(void);

#endif /* NN_SERVICE_H */

