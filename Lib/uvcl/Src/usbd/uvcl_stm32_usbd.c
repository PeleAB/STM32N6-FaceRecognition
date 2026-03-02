/**
 ******************************************************************************
 * @file    uvcl_stm32_usbd.c
 * @author  MDG Application Team
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

#include "uvcl_stm32_usbd.h"

#include <assert.h>

#include "usbd_core.h"
#include "uvcl_desc.h"
#include "uvcl_internal.h"
#ifdef UVCL_USBD_USE_THREADX
#include "tx_api.h"
#endif
#ifdef UVCL_USBD_USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif

#ifdef UVC_LIB_USE_DMA
#define UVCL_USBD_ATTR UVCL_UNCACHED UVCL_ALIGN_32
#else
#define UVCL_USBD_ATTR
#endif

#define container_of(ptr, type, member) ({ \
  void *__mptr = (ptr); \
  __mptr - offsetof(type,member); \
})

typedef struct {
  USBD_HandleTypeDef usbd_dev;
  UVCL_Ctx_t *p_ctx;
} UVCL_usbd_ctx_t;

static uint8_t dev_qualifier_desc[USB_LEN_DEV_QUALIFIER_DESC] UVCL_USBD_ATTR;
static uint8_t uvc_desc_fs[UVC_MAX_CONF_LEN] UVCL_USBD_ATTR;
static uint8_t uvc_desc_hs[UVC_MAX_CONF_LEN] UVCL_USBD_ATTR;
static int uvc_desc_hs_len;
static int uvc_desc_fs_len;

static uint8_t storage_desc[USBD_MAX_STR_DESC_SIZ] UVCL_USBD_ATTR;

static UVCL_usbd_ctx_t usbd_ctx;
#ifdef UVCL_USBD_USE_THREADX
static IRQn_Type irqn_type;
static TX_THREAD irq_thread;
static uint8_t irq_tread_stack[4096];
static TX_SEMAPHORE irq_sem;
#endif
#ifdef UVCL_USBD_USE_FREERTOS
static IRQn_Type irqn_type;
static StaticTask_t irq_thread;
static StackType_t irq_tread_stack[configMINIMAL_STACK_SIZE];
static SemaphoreHandle_t irq_sem;
static StaticSemaphore_t irq_sem_buffer;
#endif

static int is_hs(USBD_HandleTypeDef *p_dev)
{
  return p_dev->dev_speed == USBD_SPEED_HIGH;
}

static UVCL_Ctx_t *UVCL_usbd_get_ctx_from_p_dev(USBD_HandleTypeDef *p_dev)
{
  UVCL_usbd_ctx_t *p_usbd_ctx = container_of(p_dev, UVCL_usbd_ctx_t, usbd_dev);

  return p_usbd_ctx->p_ctx;
}

static uint8_t *UVCL_stm32_usbd_get_device_descriptor(USBD_SpeedTypeDef speed, uint16_t *p_length)
{
  int ret;

  ret = UVCL_get_device_desc(storage_desc, sizeof(storage_desc), USBD_IDX_MFC_STR, USBD_IDX_PRODUCT_STR,
                             USBD_IDX_SERIAL_STR);
  *p_length = ret < 0 ? 0 : ret;

  return (uint8_t *) storage_desc;
}

static uint8_t *UVCL_stm32_usbd_get_lang_id_str_descriptor(USBD_SpeedTypeDef speed, uint16_t *p_length)
{
  int ret;

  ret = UVCL_get_lang_string_desc(storage_desc, sizeof(storage_desc));
  *p_length = ret < 0 ? 0 : ret;

  return (uint8_t *) storage_desc;
}

static uint8_t *UVCL_stm32_usbd_get_manufacturer_str_descriptor(USBD_SpeedTypeDef speed, uint16_t *p_length)
{
  int ret;

  ret = UVCL_get_manufacturer_string_desc(storage_desc, sizeof(storage_desc));
  *p_length = ret < 0 ? 0 : ret;

  return (uint8_t *) storage_desc;
}

static uint8_t *UVCL_stm32_usbd_get_product_str_descriptor(USBD_SpeedTypeDef speed, uint16_t *p_length)
{
  int ret;

  ret = UVCL_get_product_string_desc(storage_desc, sizeof(storage_desc));
  *p_length = ret < 0 ? 0 : ret;

  return (uint8_t *) storage_desc;
}

static uint8_t *UVCL_stm32_usbd_get_serial_str_descriptor(USBD_SpeedTypeDef speed, uint16_t *p_length)
{
  int ret;

  ret = UVCL_get_serial_string_desc(storage_desc, sizeof(storage_desc));
  *p_length = ret < 0 ? 0 : ret;

  return (uint8_t *) storage_desc;
}

static USBD_DescriptorsTypeDef usbd_desc = {
  UVCL_stm32_usbd_get_device_descriptor,
  UVCL_stm32_usbd_get_lang_id_str_descriptor,
  UVCL_stm32_usbd_get_manufacturer_str_descriptor,
  UVCL_stm32_usbd_get_product_str_descriptor,
  UVCL_stm32_usbd_get_serial_str_descriptor,
};

static uint8_t UVCL_stm32_usbd_init_instance(USBD_HandleTypeDef *p_dev, uint8_t cfgidx)
{
  uint16_t ep_mps = is_hs(p_dev) ? UVC_ISO_HS_MPS : UVC_ISO_FS_MPS;
  int ret;

  ret = USBD_LL_OpenEP(p_dev, 0x81, USBD_EP_TYPE_ISOC, ep_mps);
  assert(ret == 0);
  p_dev->ep_in[0x81 & 0xFU].is_used = 1U;
  p_dev->ep_in[0x81 & 0xFU].maxpacket = ep_mps;

  return USBD_OK;
}

static uint8_t UVCL_stm32_usbd_deinit(USBD_HandleTypeDef *p_dev, uint8_t cfgidx)
{
  int ret;

  ret = USBD_LL_CloseEP(p_dev, 0x81);
  assert(ret == 0);
  p_dev->ep_in[0x81 & 0xFU].is_used = 0U;

  return USBD_OK;
}

static uint8_t USB_DISP_DataInImpl(USBD_HandleTypeDef *p_dev, int is_incomplete)
{
  UVCL_Ctx_t *p_ctx = UVCL_usbd_get_ctx_from_p_dev(p_dev);
  int packet_size = is_hs(p_dev) ? UVC_ISO_HS_MPS : UVC_ISO_FS_MPS;
  UVCL_OnFlyCtx_t *on_fly_ctx;
  int len;

  if (p_ctx->state != UVCL_STATUS_STREAMING)
    return USBD_OK;

  if (is_incomplete) {
    len = p_ctx->on_fly_ctx ? p_ctx->on_fly_ctx->prev_len : 0;
    USBD_LL_Transmit(p_dev, 0x81, p_ctx->packet, len);

    return USBD_OK;
  }

  /* select new frame */
  if (!p_ctx->on_fly_ctx)
    p_ctx->on_fly_ctx = UVCL_StartNewFrameTransmission(p_ctx, packet_size);

  if (!p_ctx->on_fly_ctx) {
    USBD_LL_Transmit(p_dev, 0x81, p_ctx->packet, 2);

    return USBD_OK;
  }

  /* Send next frame packet */
  on_fly_ctx = p_ctx->on_fly_ctx;
  //printf("S %d\n", on_fly_ctx->packet_index);
  len = on_fly_ctx->packet_index == (on_fly_ctx->packet_nb - 1) ? on_fly_ctx->last_packet_size + 2 : packet_size;
  memcpy(&p_ctx->packet[2], on_fly_ctx->cursor, len - 2);
  USBD_LL_Transmit(p_dev, 0x81, p_ctx->packet, len);

  UVCL_UpdateOnFlyCtx(p_ctx, len);

  return USBD_OK;
}

static uint8_t UVCL_DataIn(USBD_HandleTypeDef *p_dev)
{
  return USB_DISP_DataInImpl(p_dev, 0);
}

static int UVCL_usbd_stop_streaming(void *ctx)
{
  USBD_HandleTypeDef *p_dev = ctx;
  UVCL_Ctx_t *p_ctx;

  p_ctx = UVCL_usbd_get_ctx_from_p_dev(p_dev);
  p_ctx->state = UVCL_STATUS_STOP;
  if (p_ctx->on_fly_ctx)
    UVCL_AbortOnFlyCtx(p_ctx);
  USBD_LL_FlushEP(p_dev, 0x81);

  /* Also release queue frame */
  if (p_ctx->p_frame) {
     p_ctx->cbs->frame_release(p_ctx->cbs, p_ctx->p_frame);
     p_ctx->p_frame = NULL;
  }

  if (p_ctx->cbs->streaming_inactive)
    p_ctx->cbs->streaming_inactive(p_ctx->cbs);

  return 0;
}

static int UVCL_usbd_start_streaming(void *ctx)
{
  USBD_HandleTypeDef *p_dev = ctx;
  UVCL_StreamConf_t stream_param;
  UVCL_Ctx_t *p_ctx;

  p_ctx = UVCL_usbd_get_ctx_from_p_dev(p_dev);
  UVCL_SetupStreamingStream(p_ctx, &stream_param);
  if (p_ctx->cbs->streaming_active)
    p_ctx->cbs->streaming_active(p_ctx->cbs, stream_param);

  USBD_LL_FlushEP(p_dev, 0x81);

  p_ctx->frame_period_in_ms = 1000 / stream_param.fps;
  p_ctx->packet[0] = 2;
  p_ctx->packet[1] = 0;
  p_ctx->frame_start = HAL_GetTick() - p_ctx->frame_period_in_ms;
  p_ctx->is_starting = 1;
  p_ctx->state = UVCL_STATUS_STREAMING;

  return UVCL_DataIn(p_dev);
}

static int UVCL_usbd_send_data(void *ctx, uint8_t *data, int length)
{
  USBD_HandleTypeDef *p_dev = ctx;

  return USBD_CtlSendData(p_dev, data, length);
}

static int UVCL_usbd_receive_data(void *ctx, uint8_t *data, int length)
{
  USBD_HandleTypeDef *p_dev = ctx;

  return USBD_CtlPrepareRx(p_dev, data, length);
}

static uint8_t UVCL_stm32_usbd_setup(USBD_HandleTypeDef *p_dev, USBD_SetupReqTypedef *p_req)
{
  UVCL_Ctx_t *p_ctx = UVCL_usbd_get_ctx_from_p_dev(p_dev);
  UVCL_SetupReq_t req;
  int ret;

  req.bmRequestType = p_req->bmRequest;
  req.bRequest = p_req->bRequest;
  req.wValue = p_req->wValue;
  req.wIndex = p_req->wIndex;
  req.wLength = p_req->wLength;
  req.dwMaxPayloadTransferSize = is_hs(p_dev) ? UVC_ISO_HS_MPS : UVC_ISO_FS_MPS;
  req.ctx = p_dev;
  req.stop_streaming = UVCL_usbd_stop_streaming;
  req.start_streaming = UVCL_usbd_start_streaming;
  req.send_data = UVCL_usbd_send_data;
  req.receive_data = UVCL_usbd_receive_data;

  ret = UVCL_handle_setup_request(p_ctx, &req);
  if (ret)
    USBD_CtlError(p_dev, p_req);

  return ret ? USBD_FAIL : USBD_OK;
}

static uint8_t UVCL_stm32_usbd_datain(USBD_HandleTypeDef *p_dev, uint8_t epnum)
{
  return UVCL_DataIn(p_dev);
}

static uint8_t UVCL_stm32_usbd_sof(USBD_HandleTypeDef *p_dev)
{
  /* nothing to do */

  return USBD_OK;
}

static uint8_t UVCL_stm32_usbd_iso_in_incomplete(USBD_HandleTypeDef *p_dev, uint8_t epnum)
{
  UVCL_Ctx_t *p_ctx = UVCL_usbd_get_ctx_from_p_dev(p_dev);

  if (p_ctx->state != UVCL_STATUS_STREAMING)
    return USBD_OK;

  /* restart streaming */
  return USB_DISP_DataInImpl(p_dev, 1);
}

static uint8_t *UVCL_stm32_usbd_get_hs_iso_config_desc(uint16_t *p_length)
{
  *p_length = uvc_desc_hs_len;

  return uvc_desc_hs;
}

static uint8_t *UVCL_stm32_usbd_get_fs_iso_config_desc(uint16_t *p_length)
{
  *p_length = uvc_desc_fs_len;

  return uvc_desc_fs;
}

static uint8_t *UVCL_stm32_usbd_get_other_speed_iso_config_desc(uint16_t *p_length)
{
  return UVCL_stm32_usbd_get_fs_iso_config_desc(p_length);
}

static uint8_t *UVCL_stm32_usbd_get_device_qualifier_desc(uint16_t *p_length)
{
  *p_length = sizeof(dev_qualifier_desc);

  return dev_qualifier_desc;
}

static USBD_ClassTypeDef usbd_class = {
  UVCL_stm32_usbd_init_instance,
  UVCL_stm32_usbd_deinit,
  UVCL_stm32_usbd_setup,
  NULL, /* EP0_TxSent */
  NULL, /* EP0_RxReady */
  UVCL_stm32_usbd_datain,
  NULL, /* DataOut */
  UVCL_stm32_usbd_sof,
  UVCL_stm32_usbd_iso_in_incomplete,
  NULL, /* IsoOUTIncomplete */
  UVCL_stm32_usbd_get_hs_iso_config_desc,
  UVCL_stm32_usbd_get_fs_iso_config_desc,
  UVCL_stm32_usbd_get_other_speed_iso_config_desc,
  UVCL_stm32_usbd_get_device_qualifier_desc,
};

static void UVCL_stm32_usbd_build_descriptors(UVCL_Ctx_t *p_ctx, UVCL_Conf_t *conf)
{
  UVCL_DescBuffer buffer_desc = { 0 };
  UVCL_DescConf desc_conf = { 0 };
  int len;
  int i;

  desc_conf.is_hs = 1;
  for (i = 0; i < UVCL_MAX_STREAM_CONF_NB; i++)
    desc_conf.streams[i] = conf->streams[i];
  desc_conf.streams_nb = conf->streams_nb;

  buffer_desc.buffer = p_ctx->desc_buffer_pool;
  buffer_desc.buffer_size = sizeof(p_ctx->desc_buffer_pool);
  uvc_desc_hs_len = UVCL_get_configuration_desc(uvc_desc_hs, sizeof(uvc_desc_hs), &desc_conf, &buffer_desc);
  assert(uvc_desc_hs_len > 0);

  desc_conf.is_hs = 0;
  uvc_desc_fs_len = UVCL_get_configuration_desc(uvc_desc_fs, sizeof(uvc_desc_fs), &desc_conf, &buffer_desc);
  assert(uvc_desc_fs_len > 0);

  len = UVCL_get_device_qualifier_desc(dev_qualifier_desc, sizeof(dev_qualifier_desc));
  assert(len == sizeof(dev_qualifier_desc));
}

#if defined(UVCL_USBD_USE_THREADX) || defined(UVCL_USBD_USE_FREERTOS)
#ifdef UVCL_USBD_USE_THREADX
static void irq_thread_fct(ULONG arg)
#else
static void irq_thread_fct(void *arg)
#endif
{
  int ret;

  while (1) {
#ifdef UVCL_USBD_USE_THREADX
    ret = tx_semaphore_get(&irq_sem, TX_WAIT_FOREVER);
    assert(ret == 0);
#else
    ret = xSemaphoreTake(irq_sem, portMAX_DELAY);
    assert(ret == pdTRUE);
#endif

    HAL_PCD_IRQHandler(&uvcl_pcd_handle);
    HAL_NVIC_EnableIRQ(irqn_type);
  }
}

static void UVCL_stm32_usbd_rtos(PCD_TypeDef *pcd_instance)
{
#ifdef UVCL_USBD_USE_THREADX
  const UINT irq_priority = TX_MAX_PRIORITIES / 2 - 4;
  const ULONG time_slice = 10;
  int ret;
#else
  const UBaseType_t irq_priority = tskIDLE_PRIORITY + configMAX_PRIORITIES / 2 + 4;
  TaskHandle_t hdl;
#endif

  /* FIXME : fix this */
  if (pcd_instance == USB1_OTG_HS)
    irqn_type = USB1_OTG_HS_IRQn;
  else if (pcd_instance == USB2_OTG_HS)
    irqn_type = USB2_OTG_HS_IRQn;

#ifdef UVCL_USBD_USE_THREADX
  ret = tx_semaphore_create(&irq_sem, NULL, 0);
  assert(ret == 0);

  ret = tx_thread_create(&irq_thread, "irq", irq_thread_fct, 0, irq_tread_stack,
                         sizeof(irq_tread_stack), irq_priority, irq_priority, time_slice, TX_AUTO_START);
  assert(ret == TX_SUCCESS);
#else
  irq_sem = xSemaphoreCreateCountingStatic(1, 0, &irq_sem_buffer);
  assert(irq_sem != NULL);

  hdl = xTaskCreateStatic(irq_thread_fct, "irq", configMINIMAL_STACK_SIZE, NULL, irq_priority, irq_tread_stack,
                          &irq_thread);
  assert(hdl != NULL);
#endif
}
#endif

int UVCL_stm32_usbd_init(UVCL_Ctx_t *p_ctx, PCD_HandleTypeDef *pcd_handle, PCD_TypeDef *pcd_instance, UVCL_Conf_t *conf)
{
  int ret;

  UVCL_stm32_usbd_build_descriptors(p_ctx, conf);

  usbd_ctx.p_ctx = p_ctx;
  ret = USBD_Init(&usbd_ctx.usbd_dev, &usbd_desc, 0);
  assert(ret == 0);

  /* link structure. Do I really need this ? => yes for multiple context support */
  pcd_handle->pData = &usbd_ctx.usbd_dev;
  usbd_ctx.usbd_dev.pData = pcd_handle;

  ret = USBD_RegisterClass(&usbd_ctx.usbd_dev, &usbd_class);
  assert(ret == 0);

  ret = USBD_Start(&usbd_ctx.usbd_dev);
  assert(ret == 0);

#if defined(UVCL_USBD_USE_THREADX) || defined(UVCL_USBD_USE_FREERTOS)
  UVCL_stm32_usbd_rtos(pcd_instance);
#endif

  return 0;
}

#ifdef UVCL_USBD_USE_THREADX
void UVCL_stm32_usbd_IRQHandler()
{
  int ret;

  HAL_NVIC_DisableIRQ(irqn_type);
  ret = tx_semaphore_put(&irq_sem);
  assert(ret == 0);
}
#endif

#ifdef UVCL_USBD_USE_FREERTOS
void UVCL_stm32_usbd_IRQHandler()
{
  int ret;

  HAL_NVIC_DisableIRQ(irqn_type);
  ret = xSemaphoreGive(irq_sem);
  assert(ret == pdTRUE);
}
#endif

