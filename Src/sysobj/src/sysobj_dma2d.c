/**
 ******************************************************************************
 * @file    fal_dma2d.c
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

#include "fal_dma2d.h"

#include "stm32n6xx_hal.h"

static DMA2D_HandleTypeDef dma2d_handle;
static fal_dma2d_cb_t complete_cb;
static fal_dma2d_cb_t error_cb;
static void *cb_user;

static void fal_dma2d_clear_callbacks(void)
{
  complete_cb = NULL;
  error_cb = NULL;
  cb_user = NULL;
}

static void fal_dma2d_on_complete(DMA2D_HandleTypeDef *handle)
{
  (void) handle;

  HAL_NVIC_DisableIRQ(DMA2D_IRQn);
  if (complete_cb)
    complete_cb(cb_user);

  fal_dma2d_clear_callbacks();
}

static void fal_dma2d_on_error(DMA2D_HandleTypeDef *handle)
{
  (void) handle;

  HAL_NVIC_DisableIRQ(DMA2D_IRQn);
  if (error_cb)
    error_cb(cb_user);

  fal_dma2d_clear_callbacks();
}

static int fal_dma2d_prepare(uint32_t mode, uint32_t output_offset)
{
  dma2d_handle.Instance = DMA2D;
  dma2d_handle.Init.Mode = mode;
  dma2d_handle.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  dma2d_handle.Init.OutputOffset = output_offset;
  dma2d_handle.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
  dma2d_handle.Init.RedBlueSwap = DMA2D_RB_REGULAR;
  dma2d_handle.Init.LineOffsetMode = DMA2D_LOM_PIXELS;
  dma2d_handle.Init.BytesSwap = DMA2D_BYTES_REGULAR;

  if (HAL_DMA2D_Init(&dma2d_handle) != HAL_OK)
    return -1;

  dma2d_handle.XferCpltCallback = fal_dma2d_on_complete;
  dma2d_handle.XferErrorCallback = fal_dma2d_on_error;

  HAL_NVIC_EnableIRQ(DMA2D_IRQn);

  return 0;
}

int FAL_DMA2D_Blend(const fal_dma2d_blend_t *cfg)
{
  uint32_t dst_addr;
  int ret;

  if (!cfg || !cfg->dst || !cfg->src)
    return -1;

  fal_dma2d_clear_callbacks();
  ret = fal_dma2d_prepare(DMA2D_M2M_BLEND, cfg->dst_width - cfg->src_width);
  if (ret)
    return ret;

  dma2d_handle.LayerCfg[0].InputAlpha     = 0xFF;
  dma2d_handle.LayerCfg[0].InputColorMode = DMA2D_INPUT_ARGB8888;
  dma2d_handle.LayerCfg[0].InputOffset    = cfg->dst_width - cfg->src_width;
  dma2d_handle.LayerCfg[0].AlphaInverted  = DMA2D_REGULAR_ALPHA;
  dma2d_handle.LayerCfg[0].RedBlueSwap    = DMA2D_RB_REGULAR;

  dma2d_handle.LayerCfg[1].AlphaMode      = DMA2D_NO_MODIF_ALPHA;
  dma2d_handle.LayerCfg[1].InputAlpha     = 0xFF;
  dma2d_handle.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  dma2d_handle.LayerCfg[1].InputOffset    = 0;
  dma2d_handle.LayerCfg[1].AlphaInverted  = DMA2D_REGULAR_ALPHA;
  dma2d_handle.LayerCfg[1].RedBlueSwap    = DMA2D_RB_REGULAR;
  dma2d_handle.LayerCfg[0].AlphaMode      = DMA2D_NO_MODIF_ALPHA;

  ret = HAL_DMA2D_ConfigLayer(&dma2d_handle, 1);
  if (ret != HAL_OK)
    return -1;

  ret = HAL_DMA2D_ConfigLayer(&dma2d_handle, 0);
  if (ret != HAL_OK)
    return -1;

  complete_cb = cfg->on_complete;
  error_cb = cfg->on_error;
  cb_user = cfg->user;

  dst_addr = (uint32_t) (cfg->dst + (cfg->dst_width * cfg->y_offset + cfg->x_offset) * 4U);
  ret = HAL_DMA2D_BlendingStart_IT(&dma2d_handle, (uint32_t) cfg->src, dst_addr, dst_addr,
                                   cfg->src_width, cfg->src_height);

  return (ret == HAL_OK) ? 0 : -1;
}

int FAL_DMA2D_Fill(const fal_dma2d_fill_t *cfg)
{
  uint32_t dst_addr;
  int ret;

  if (!cfg || !cfg->dst)
    return -1;

  fal_dma2d_clear_callbacks();
  ret = fal_dma2d_prepare(DMA2D_R2M, cfg->dst_width - cfg->width);
  if (ret)
    return ret;

  ret = HAL_DMA2D_ConfigLayer(&dma2d_handle, 1);
  if (ret != HAL_OK)
    return -1;

  complete_cb = cfg->on_complete;
  error_cb = cfg->on_error;
  cb_user = cfg->user;

  dst_addr = (uint32_t) (cfg->dst + (cfg->dst_width * cfg->y_offset + cfg->x_offset) * 4U);
  ret = HAL_DMA2D_Start_IT(&dma2d_handle, cfg->color, dst_addr, cfg->width, cfg->height);

  return (ret == HAL_OK) ? 0 : -1;
}

void FAL_DMA2D_IRQHandler(void)
{
  HAL_DMA2D_IRQHandler(&dma2d_handle);
}

void DMA2D_IRQHandler(void)
{
  FAL_DMA2D_IRQHandler();
}

