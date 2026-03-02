/**
 ******************************************************************************
 * @file    nn_service.c
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

#include "svc/nn_service.h"

#include <assert.h>
#include <string.h>

typedef struct
{
  nn_service_model_t models[NN_SERVICE_MAX_MODELS];
  uint32_t count;
  uint32_t max_input_size;
  uint32_t max_output_size;
  uint8_t runtime_ready;
  nn_service_model_t *active;
} nn_service_ctx_t;

static nn_service_ctx_t nn_ctx;

static const LL_Buffer_InfoTypeDef *nn_service_first_user_buffer(const LL_Buffer_InfoTypeDef *buffers,
                                                                 uint32_t *user_count)
{
  const LL_Buffer_InfoTypeDef *first = NULL;
  uint32_t count = 0;

  if (buffers == NULL)
    goto end;

  while (buffers->name != NULL) {
    if (buffers->is_user_allocated) {
      if (first == NULL)
        first = buffers;
      count++;
    }
    buffers++;
  }

end:
  if (user_count)
    *user_count = count;

  return first;
}

static const char *nn_service_model_name(const nn_service_model_cfg_t *cfg)
{
  if (cfg->name)
    return cfg->name;

  if (cfg->instance && cfg->instance->network)
    return cfg->instance->network->network_name;

  return "unnamed";
}

nn_service_status_t nn_service_init(void)
{
  memset(&nn_ctx, 0, sizeof(nn_ctx));
  LL_ATON_RT_RuntimeInit();
  nn_ctx.runtime_ready = 1;

  return NN_SERVICE_OK;
}

nn_service_status_t nn_service_register(const nn_service_model_cfg_t *cfg, nn_service_handle_t *out_handle)
{
  const LL_Buffer_InfoTypeDef *inputs_info;
  const LL_Buffer_InfoTypeDef *outputs_info;
  const LL_Buffer_InfoTypeDef *first_input;
  const LL_Buffer_InfoTypeDef *first_output;
  nn_service_model_t *model;

  if (!nn_ctx.runtime_ready)
    return NN_SERVICE_ERR_INIT;
  if (!cfg || !cfg->instance)
    return NN_SERVICE_ERR_ARGS;
  if (nn_ctx.count >= NN_SERVICE_MAX_MODELS)
    return NN_SERVICE_ERR_FULL;

  model = &nn_ctx.models[nn_ctx.count];
  memset(model, 0, sizeof(*model));

  model->handle = (nn_service_handle_t) nn_ctx.count;
  model->name = nn_service_model_name(cfg);
  model->instance = cfg->instance;
  model->postprocess_type = cfg->postprocess_type;

  inputs_info = model->instance->network && model->instance->network->input_buffers_info
                    ? model->instance->network->input_buffers_info()
                    : NULL;
  outputs_info = model->instance->network && model->instance->network->output_buffers_info
                     ? model->instance->network->output_buffers_info()
                     : NULL;

  first_input = nn_service_first_user_buffer(inputs_info, &model->user_input_count);
  first_output = nn_service_first_user_buffer(outputs_info, &model->user_output_count);
  if (!first_input || !first_output)
    return NN_SERVICE_ERR_NO_BUFFERS;

  model->user_input_size = LL_Buffer_len(first_input);
  model->user_output_size = LL_Buffer_len(first_output);

  if (model->user_input_size > nn_ctx.max_input_size)
    nn_ctx.max_input_size = model->user_input_size;
  if (model->user_output_size > nn_ctx.max_output_size)
    nn_ctx.max_output_size = model->user_output_size;

  nn_ctx.count++;
  if (nn_ctx.active == NULL)
    nn_ctx.active = model;

  if (out_handle)
    *out_handle = model->handle;

  return NN_SERVICE_OK;
}

nn_service_status_t nn_service_select(nn_service_handle_t handle)
{
  if (!nn_ctx.runtime_ready)
    return NN_SERVICE_ERR_INIT;
  if (handle < 0 || (uint32_t) handle >= nn_ctx.count)
    return NN_SERVICE_ERR_ARGS;

  nn_ctx.active = &nn_ctx.models[handle];
  if (!nn_ctx.active->is_initialized) {
    LL_ATON_RT_Init_Network(nn_ctx.active->instance);
    nn_ctx.active->is_initialized = 1;
  }

  return NN_SERVICE_OK;
}

const nn_service_model_t *nn_service_active(void)
{
  return nn_ctx.active;
}

const nn_service_model_t *nn_service_get(nn_service_handle_t handle)
{
  if (handle < 0 || (uint32_t) handle >= nn_ctx.count)
    return NULL;

  return &nn_ctx.models[handle];
}

nn_service_status_t nn_service_prepare_io(uint8_t *input, uint32_t input_len, uint8_t *output, uint32_t output_len)
{
  if (!nn_ctx.runtime_ready || nn_ctx.active == NULL)
    return NN_SERVICE_ERR_INIT;
  if (!input || !output)
    return NN_SERVICE_ERR_ARGS;

  if (!nn_ctx.active->is_initialized) {
    nn_service_status_t status = nn_service_select(nn_ctx.active->handle);
    if (status != NN_SERVICE_OK)
      return status;
  }

  if (input_len < nn_ctx.active->user_input_size || output_len < nn_ctx.active->user_output_size)
    return NN_SERVICE_ERR_ARGS;

  if (LL_ATON_Set_User_Input_Buffer(nn_ctx.active->instance, 0, input, input_len) != LL_ATON_User_IO_NOERROR)
    return NN_SERVICE_ERR_IO;
  if (LL_ATON_Set_User_Output_Buffer(nn_ctx.active->instance, 0, output, output_len) != LL_ATON_User_IO_NOERROR)
    return NN_SERVICE_ERR_IO;

  return NN_SERVICE_OK;
}

uint32_t nn_service_max_input_size(void)
{
  return nn_ctx.max_input_size;
}

uint32_t nn_service_max_output_size(void)
{
  return nn_ctx.max_output_size;
}

uint32_t nn_service_count(void)
{
  return nn_ctx.count;
}

