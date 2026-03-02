/**
 ******************************************************************************
 * @file    uvcl_usbx.c
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

#include "uvcl_usbx.h"

#include <assert.h>

#include "uvcl_desc.h"
#include "uvcl_internal.h"
#include "ux_api.h"
#include "ux_dcd_stm32.h"
#include "ux_device_class_video.h"
#ifdef UVCL_USBX_USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif

static uint8_t usbx_mem_pool[USBX_MEM_SIZE] UVCL_ALIGN_32;
#ifdef UVC_LIB_USE_DMA
static uint8_t usbx_mem_pool_uncached[USBX_MEM_SIZE] UVCL_UNCACHED UVCL_ALIGN_32;
#endif
static uint8_t uvc_desc_fs[UVC_MAX_CONF_LEN];
static uint8_t uvc_desc_hs[UVC_MAX_CONF_LEN];
static uint8_t uvc_dev_strings[UVC_MAX_STRING_LEN];
static uint8_t uvc_dev_langid[UVC_MAX_LANGID_LEN];
#ifdef  UVCL_USBX_USE_FREERTOS
static IRQn_Type irqn_type;
static StaticTask_t cb_thread;
static StackType_t cb_tread_stack[configMINIMAL_STACK_SIZE];
static SemaphoreHandle_t cb_sem;
static StaticSemaphore_t cb_sem_buffer;
#endif

static int is_hs()
{
  assert(_ux_system_slave);

  return _ux_system_slave->ux_system_slave_speed == UX_HIGH_SPEED_DEVICE;
}

static UVCL_Ctx_t *UVCL_usbx_get_ctx_from_video_instance(UX_DEVICE_CLASS_VIDEO *video_instance)
{
  /* Should use ux_device_class_video_ioctl + UX_DEVICE_CLASS_VIDEO_IOCTL_GET_ARG. But direct access is faster */

  return video_instance->ux_device_class_video_callbacks.ux_device_class_video_arg;
}

static UVCL_Ctx_t *UVCL_usbx_get_ctx_from_stream(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream)
{
  return UVCL_usbx_get_ctx_from_video_instance(stream->ux_device_class_video_stream_video);
}

static void UVCL_SendPacket(UVCL_Ctx_t *p_ctx, UX_DEVICE_CLASS_VIDEO_STREAM *stream, int len)
{
  ULONG buffer_length;
  UCHAR *buffer;
  int ret;

  ret = ux_device_class_video_write_payload_get(stream, &buffer, &buffer_length);
  assert(ret == UX_SUCCESS);
  assert(buffer_length >= len);

  memcpy(buffer, p_ctx->packet, len);
  ret = ux_device_class_video_write_payload_commit(stream, len);
  assert(ret == UX_SUCCESS);
}

static void UVCL_DataIn(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream)
{
  int packet_size = is_hs() ? UVC_ISO_HS_MPS : UVC_ISO_FS_MPS;
  UVCL_Ctx_t *p_ctx = UVCL_usbx_get_ctx_from_stream(stream);
  UVCL_OnFlyCtx_t *on_fly_ctx;
  int len;

  if (p_ctx->state != UVCL_STATUS_STREAMING)
  {
    return ;
  }

  /* select new frame */
  if (!p_ctx->on_fly_ctx)
    p_ctx->on_fly_ctx = UVCL_StartNewFrameTransmission(p_ctx, packet_size);

  if (!p_ctx->on_fly_ctx) {
    UVCL_SendPacket(p_ctx, stream, 2);
    return ;
  }

  /* Send next frame packet */
  on_fly_ctx = p_ctx->on_fly_ctx;
  //printf("S %d\n", on_fly_ctx->packet_index);
  len = on_fly_ctx->packet_index == (on_fly_ctx->packet_nb - 1) ? on_fly_ctx->last_packet_size + 2 : packet_size;
  memcpy(&p_ctx->packet[2], on_fly_ctx->cursor, len - 2);
  UVCL_SendPacket(p_ctx, stream, len);

  UVCL_UpdateOnFlyCtx(p_ctx, len);
}

static void UVCL_StopStreaming(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream)
{
  UVCL_Ctx_t *p_ctx = UVCL_usbx_get_ctx_from_stream(stream);

  p_ctx->state = UVCL_STATUS_STOP;
  if (p_ctx->on_fly_ctx)
    UVCL_AbortOnFlyCtx(p_ctx);

  /* Also release queue frame */
  if (p_ctx->p_frame) {
     p_ctx->cbs->frame_release(p_ctx->cbs, p_ctx->p_frame);
     p_ctx->p_frame = NULL;
  }

  if (p_ctx->cbs->streaming_inactive)
    p_ctx->cbs->streaming_inactive(p_ctx->cbs);
}

static void UVCL_StartStreaming(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream)
{
  UVCL_Ctx_t *p_ctx = UVCL_usbx_get_ctx_from_stream(stream);
  UVCL_StreamConf_t stream_param;
  int i;

  UVCL_SetupStreamingStream(p_ctx, &stream_param);
  if (p_ctx->cbs->streaming_active)
    p_ctx->cbs->streaming_active(p_ctx->cbs, stream_param);

  p_ctx->frame_period_in_ms = 1000 / stream_param.fps;
  p_ctx->packet[0] = 2;
  p_ctx->packet[1] = 0;
  p_ctx->frame_start = HAL_GetTick() - p_ctx->frame_period_in_ms;
  p_ctx->is_starting = 1;
  p_ctx->state = UVCL_STATUS_STREAMING;

  for (i = 0; i < p_ctx->buffer_nb; i++)
    UVCL_DataIn(stream);
}

static void UVCL_stream_change(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream, ULONG alternate_setting)
{
  int ret;

  if (alternate_setting == 0) {
    UVCL_StopStreaming(stream);
    return ;
  }

  UVCL_StartStreaming(stream);

  ret = ux_device_class_video_transmission_start(stream);
  assert(ret == UX_SUCCESS);
}

static int UVCL_usbd_stop_streaming(void *ctx)
{
  /* we should never reach this function */
  assert(0);

  return -1;
}

static int UVCL_usbd_start_streaming(void *ctx)
{
  /* we should never reach this function */
  assert(0);

  return -1;
}

static int UVCL_usbx_send_data(void *ctx, uint8_t *data, int length)
{
  UX_SLAVE_TRANSFER *transfer = ctx;
  uint8_t *buffer = transfer->ux_slave_transfer_request_data_pointer;
  int ret;

  memcpy(buffer, data, length);
  ret = ux_device_stack_transfer_request(transfer, length, length);

  return ret;
}

static int UVCL_usbx_receive_data(void *ctx, uint8_t *data, int length)
{
  UX_SLAVE_TRANSFER *transfer = ctx;

  if (transfer->ux_slave_transfer_request_actual_length != length)
    return UX_ERROR;

  memcpy(data, transfer->ux_slave_transfer_request_data_pointer, length);

  return UX_SUCCESS;
}

static UINT UVCL_stream_request(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream, UX_SLAVE_TRANSFER *transfer)
{
  UVCL_Ctx_t *p_ctx = UVCL_usbx_get_ctx_from_stream(stream);
  UVCL_SetupReq_t req;
  int ret;

  req.bmRequestType = transfer->ux_slave_transfer_request_setup[UX_SETUP_REQUEST_TYPE];
  req.bRequest = transfer->ux_slave_transfer_request_setup[UX_SETUP_REQUEST];
  req.wValue = ux_utility_short_get(transfer->ux_slave_transfer_request_setup + UX_SETUP_VALUE);
  req.wIndex = ux_utility_short_get(transfer->ux_slave_transfer_request_setup + UX_SETUP_INDEX);
  req.wLength = ux_utility_short_get(transfer->ux_slave_transfer_request_setup + UX_SETUP_LENGTH);
  req.dwMaxPayloadTransferSize = is_hs() ? UVC_ISO_HS_MPS : UVC_ISO_FS_MPS;
  req.ctx = transfer;
  req.stop_streaming = UVCL_usbd_stop_streaming;
  req.start_streaming = UVCL_usbd_start_streaming;
  req.send_data = UVCL_usbx_send_data;
  req.receive_data = UVCL_usbx_receive_data;

  ret = UVCL_handle_setup_request(p_ctx, &req);

  return ret ? UX_ERROR : UX_SUCCESS;
}

static VOID UVCL_stream_payload_done(struct UX_DEVICE_CLASS_VIDEO_STREAM_STRUCT *stream, ULONG length)
{
  UVCL_DataIn(stream);
}

static VOID UVCL_instance_activate(VOID *video_instance)
{
  UVCL_Ctx_t *p_ctx = UVCL_usbx_get_ctx_from_video_instance(video_instance);

  p_ctx->state = UVCL_STATUS_STOP;
}

static VOID UVCL_instance_deactivate(VOID *video_instance)
{
  ;
}

static int UVCL_usbx_extract_string(uint8_t langid[2], int index, uint8_t *string_desc, uint8_t *p_dst, int dst_len)
{
  int str_len = (string_desc[0] - 2) / 2;
  int i;

  if (dst_len < str_len + 4)
    return -1;

  p_dst[0] = langid[0];
  p_dst[1] = langid[1];
  p_dst[2] = index;
  p_dst[3] = str_len;
  for (i = 0; i < str_len; i++)
    p_dst[4 + i] = string_desc[2 + 2 * i];

  return str_len + 4;
}

static int UVCL_usbx_build_dev_strings(uint8_t langid[2], uint8_t *p_dst, int dst_len)
{
  uint8_t string_desc[128];
  int res = 0;
  int len;

  len = UVCL_get_manufacturer_string_desc(string_desc, sizeof(string_desc));
  if (len < 0)
    return len;
  res += UVCL_usbx_extract_string(langid, 1, string_desc, &p_dst[res], dst_len - res);
  if (res < 0)
    return 0;

  len = UVCL_get_product_string_desc(string_desc, sizeof(string_desc));
  if (len < 0)
    return len;
  res += UVCL_usbx_extract_string(langid, 2, string_desc, &p_dst[res], dst_len - res);
  if (res < 0)
    return 0;

  len = UVCL_get_serial_string_desc(string_desc, sizeof(string_desc));
  if (len < 0)
    return len;
  res += UVCL_usbx_extract_string(langid, 3, string_desc, &p_dst[res], dst_len - res);
  if (res < 0)
    return 0;

  return res;
}

#ifdef  UVCL_USBX_USE_FREERTOS
static void cb_thread_fct(void *arg)
{
  int ret;

  while (1) {
    ret = xSemaphoreTake(cb_sem, portMAX_DELAY);
    assert(ret == pdTRUE);

    ux_system_tasks_run();
    ux_system_tasks_run();
    HAL_NVIC_EnableIRQ(irqn_type);
  }
}

static void UVCL_usbx_freertos(PCD_TypeDef *pcd_instance)
{
  const UBaseType_t cb_priority = tskIDLE_PRIORITY + configMAX_PRIORITIES / 2;
  TaskHandle_t hdl;

  /* FIXME : fix this */
  if (pcd_instance == USB1_OTG_HS)
    irqn_type = USB1_OTG_HS_IRQn;
  else if (pcd_instance == USB2_OTG_HS)
    irqn_type = USB2_OTG_HS_IRQn;

  cb_sem = xSemaphoreCreateCountingStatic(1, 0, &cb_sem_buffer);
  assert(cb_sem != NULL);

  hdl = xTaskCreateStatic(cb_thread_fct, "irq", configMINIMAL_STACK_SIZE, NULL, cb_priority, cb_tread_stack,
                          &cb_thread);
  assert(hdl != NULL);
}
#endif

int UVCL_usbx_init(UVCL_Ctx_t *p_ctx, PCD_HandleTypeDef *pcd_handle, PCD_TypeDef *pcd_instance, UVCL_Conf_t *conf)
{
  UX_DEVICE_CLASS_VIDEO_STREAM_PARAMETER vsp[1] = { 0 };
  UX_DEVICE_CLASS_VIDEO_PARAMETER vp = { 0 };
  UVCL_DescBuffer buffer_desc = { 0 };
  UVCL_DescConf desc_conf = { 0 };
  uint8_t lang_string_desc[4];
  int uvc_dev_strings_len;
  int uvc_dev_langid_len;
  int uvc_desc_hs_len;
  int uvc_desc_fs_len;
  int len;
  int ret;
  int i;

  assert(UVC_ISO_HS_MPS >= UVC_ISO_FS_MPS);

#ifdef UVC_LIB_USE_DMA
  ret = ux_system_initialize(usbx_mem_pool, USBX_MEM_SIZE, usbx_mem_pool_uncached, USBX_MEM_SIZE);
#else
  ret = ux_system_initialize(usbx_mem_pool, USBX_MEM_SIZE, UX_NULL, 0);
#endif
  if (ret)
    return ret;

  desc_conf.is_hs = 1;
  for (i = 0; i < UVCL_MAX_STREAM_CONF_NB; i++)
    desc_conf.streams[i] = conf->streams[i];
  desc_conf.streams_nb = conf->streams_nb;
  uvc_desc_hs_len = UVCL_get_device_desc(uvc_desc_hs, sizeof(uvc_desc_hs), 1, 2, 3);
  assert(uvc_desc_hs_len > 0);
  buffer_desc.buffer = p_ctx->desc_buffer_pool;
  buffer_desc.buffer_size = sizeof(p_ctx->desc_buffer_pool);
  len = UVCL_get_configuration_desc(&uvc_desc_hs[uvc_desc_hs_len], sizeof(uvc_desc_hs) - uvc_desc_hs_len,
                                    &desc_conf, &buffer_desc);
  assert(len > 0);
  uvc_desc_hs_len += len;

  desc_conf.is_hs = 0;
  uvc_desc_fs_len = UVCL_get_device_desc(uvc_desc_fs, sizeof(uvc_desc_fs), 1, 2, 3);
  assert(uvc_desc_fs_len > 0);
  len = UVCL_get_configuration_desc(&uvc_desc_fs[uvc_desc_fs_len], sizeof(uvc_desc_fs) - uvc_desc_fs_len,
                                    &desc_conf, &buffer_desc);
  assert(len > 0);
  uvc_desc_fs_len += len;

  len = UVCL_get_lang_string_desc(lang_string_desc, sizeof(lang_string_desc));
  assert(len == sizeof(lang_string_desc));
  uvc_dev_langid[0] = lang_string_desc[2];
  uvc_dev_langid[1] = lang_string_desc[3];
  uvc_dev_langid_len = 2;

  uvc_dev_strings_len = UVCL_usbx_build_dev_strings(uvc_dev_langid, uvc_dev_strings, sizeof(uvc_dev_strings));
  assert(uvc_dev_strings_len > 0);

  ret = ux_device_stack_initialize(uvc_desc_hs, uvc_desc_hs_len,
                                   uvc_desc_fs, uvc_desc_fs_len,
                                   uvc_dev_strings, uvc_dev_strings_len,
                                   uvc_dev_langid, uvc_dev_langid_len, NULL);
  if (ret)
    return ret;

#if defined(UX_DEVICE_STANDALONE)
  vsp[0].ux_device_class_video_stream_parameter_task_function = ux_device_class_video_write_task_function;
#else
  vsp[0].ux_device_class_video_stream_parameter_thread_stack_size = 0;
  vsp[0].ux_device_class_video_stream_parameter_thread_entry = ux_device_class_video_write_thread_entry;
#endif
  vsp[0].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_change = UVCL_stream_change;
  vsp[0].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_request = UVCL_stream_request;
  vsp[0].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_payload_done = UVCL_stream_payload_done;
  vsp[0].ux_device_class_video_stream_parameter_max_payload_buffer_nb = USBX_BUFFER_NB;
  vsp[0].ux_device_class_video_stream_parameter_max_payload_buffer_size = UVC_ISO_HS_MPS;
  vp.ux_device_class_video_parameter_callbacks.ux_slave_class_video_instance_activate = UVCL_instance_activate;
  vp.ux_device_class_video_parameter_callbacks.ux_slave_class_video_instance_deactivate = UVCL_instance_deactivate;
  vp.ux_device_class_video_parameter_callbacks.ux_device_class_video_request = NULL;
  vp.ux_device_class_video_parameter_callbacks.ux_device_class_video_arg = p_ctx;
  vp.ux_device_class_video_parameter_streams_nb = 1;
  vp.ux_device_class_video_parameter_streams = vsp;
  ret = ux_device_stack_class_register(_ux_system_device_class_video_name, ux_device_class_video_entry, 1, 0, &vp);
  if (ret)
    return ret;

#ifdef  UVCL_USBX_USE_FREERTOS
  UVCL_usbx_freertos(pcd_instance);
#endif

  return ux_dcd_stm32_initialize((ULONG)pcd_instance, (ULONG)pcd_handle);
}

#if defined(UX_DEVICE_STANDALONE)
ALIGN_TYPE _ux_utility_interrupt_disable(VOID)
{
  ALIGN_TYPE ret = __get_PRIMASK();

  __disable_irq();

  return ret;
}

VOID _ux_utility_interrupt_restore(ALIGN_TYPE flags)
{
  if (!flags)
    __enable_irq();
}
#endif

void UVCL_stm32_usbx_IRQHandler()
{
  HAL_PCD_IRQHandler(&uvcl_pcd_handle);

#ifdef UX_STANDALONE
#ifdef UVCL_USBX_USE_FREERTOS
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int ret;

    ret = xSemaphoreGiveFromISR(cb_sem, &xHigherPriorityTaskWoken);
    assert(ret == pdTRUE);
    HAL_NVIC_DisableIRQ(irqn_type);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
#else
  ux_system_tasks_run();
  ux_system_tasks_run();
#endif
#endif
}

