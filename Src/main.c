 /**
 ******************************************************************************
 * @file    main.c
 * @author  GPM Application Team
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#include "cmw_camera.h"
#include "stm32n6570_discovery_bus.h"
#include "stm32n6570_discovery_lcd.h"
#include "stm32n6570_discovery_xspi.h"
#include "stm32n6570_discovery.h"
#include "stm32_lcd.h"
#include "app_fuseprogramming.h"
#include "stm32_lcd_ex.h"
#include "app_postprocess.h"
#include "ll_aton_runtime.h"
#include "app_cam.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include "stm32n6xx_hal_rif.h"
#include "pc_stream.h"
#include "app_config.h"
#include "crop_img.h"
#include "display_utils.h"
#include "img_buffer.h"
#include "system_utils.h"
#include "blazeface_anchors.h"
#include "face_utils.h"
#include "target_embedding.h"
#include "dummy_fr_input.h"


#define MAX_NUMBER_OUTPUT 5

#define FR_WIDTH 96
#define FR_HEIGHT 112



pd_model_pp_static_param_t pp_params;

volatile int32_t cameraFrameReceived;
uint8_t *nn_in;
int8_t  *fr_nn_in;
int8_t  *fr_nn_out;
__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
uint8_t nn_rgb[NN_WIDTH * NN_HEIGHT * NN_BPP];
__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
uint8_t fr_rgb[FR_WIDTH * FR_HEIGHT * NN_BPP];
void* pp_input;
pd_postprocess_out_t pp_output;

uint32_t fr_in_len;
uint32_t fr_out_len;

#define ALIGN_TO_16(value) (((value) + 15) & ~15)

/* Working buffer for camera capture when pitch differs */
#define DCMIPP_OUT_NN_LEN (ALIGN_TO_16(NN_WIDTH * NN_BPP) * NN_HEIGHT)
#define DCMIPP_OUT_NN_BUFF_LEN (DCMIPP_OUT_NN_LEN + 32 - DCMIPP_OUT_NN_LEN%32)

__attribute__ ((aligned (32)))
uint8_t dcmipp_out_nn[DCMIPP_OUT_NN_BUFF_LEN];


/* Utility functions handling the various I/O configurations */
static void App_InputInit(uint32_t *pitch_nn);
static int  App_GetFrame(uint8_t *dest, uint32_t pitch_nn);
static void App_PreInference(const uint8_t *frame);
static void App_Output(pd_postprocess_out_t *res, uint32_t inf_ms,
                       uint32_t boot_ms);

static void RunNetworkSync(NN_Instance_TypeDef *inst);

/*-------------------------------------------------------------------------*/
static void App_InputInit(uint32_t *pitch_nn)
{
#if INPUT_SRC_MODE == INPUT_SRC_CAMERA
  CAM_Init(&lcd_bg_area.XSize, &lcd_bg_area.YSize, pitch_nn);
#else
  lcd_bg_area.XSize = NN_WIDTH;
  lcd_bg_area.YSize = NN_HEIGHT;
  (void)pitch_nn;
#endif
#ifdef ENABLE_LCD_DISPLAY
  LCD_init();
#else
  (void)pitch_nn;
#endif
#if INPUT_SRC_MODE == INPUT_SRC_CAMERA
  CAM_DisplayPipe_Start(img_buffer, CMW_MODE_CONTINUOUS);
#endif
}

static int App_GetFrame(uint8_t *dest, uint32_t pitch_nn)
{
#if INPUT_SRC_MODE == INPUT_SRC_CAMERA
  CAM_IspUpdate();
  if (pitch_nn != (NN_WIDTH * NN_BPP))
  {
    CAM_NNPipe_Start(dcmipp_out_nn, CMW_MODE_SNAPSHOT);
  }
  else
  {
    CAM_NNPipe_Start(dest, CMW_MODE_SNAPSHOT);
  }

  while (cameraFrameReceived == 0) {}
  cameraFrameReceived = 0;

  if (pitch_nn != (NN_WIDTH * NN_BPP))
  {
    SCB_InvalidateDCache_by_Addr(dcmipp_out_nn, sizeof(dcmipp_out_nn));
    img_crop(dcmipp_out_nn, dest, pitch_nn, NN_WIDTH, NN_HEIGHT, NN_BPP);
  }
  else
  {
    SCB_InvalidateDCache_by_Addr(dest, NN_WIDTH * NN_HEIGHT * NN_BPP);
  }
  return 0;
#else
  return PC_STREAM_ReceiveImage(dest, NN_WIDTH * NN_HEIGHT * NN_BPP);
#endif
}

static void App_PreInference(const uint8_t *frame)
{
#if INPUT_SRC_MODE != INPUT_SRC_CAMERA
#ifdef ENABLE_PC_STREAM
  PC_STREAM_SendFrame(frame, NN_WIDTH, NN_HEIGHT, NN_BPP);
#endif
#else
  (void)frame;
#endif
}

static void App_Output(pd_postprocess_out_t *res, uint32_t inf_ms,
                       uint32_t boot_ms)

{
#ifdef ENABLE_PC_STREAM
  Display_NetworkOutput(res, inf_ms, boot_ms);
#elif defined(ENABLE_LCD_DISPLAY)
  Display_NetworkOutput(res, inf_ms, boot_ms);
#else
  (void)res;
  (void)inf_ms;
#endif
}

static void RunNetworkSync(NN_Instance_TypeDef *inst)
{
  LL_ATON_RT_Init_Network(inst);
  LL_ATON_RT_RetValues_t st;
  do
  {
    st = LL_ATON_RT_RunEpochBlock(inst);
    if (st == LL_ATON_RT_WFE)
    {
      LL_ATON_OSAL_WFE();
    }
  } while (st != LL_ATON_RT_DONE);
}



/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Power on ICACHE */
  MEMSYSCTL->MSCR |= MEMSYSCTL_MSCR_ICACTIVE_Msk;

  /* Set back system and CPU clock source to HSI */
  __HAL_RCC_CPUCLK_CONFIG(RCC_CPUCLKSOURCE_HSI);
  __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI);

  HAL_Init();

  SCB_EnableICache();

#if defined(USE_DCACHE)
  /* Power on DCACHE */
  MEMSYSCTL->MSCR |= MEMSYSCTL_MSCR_DCACTIVE_Msk;
  SCB_EnableDCache();
#endif

  SystemClock_Config();

  NPURam_enable();

  Fuse_Programming();

  NPUCache_config();

#ifdef ENABLE_PC_STREAM
  PC_STREAM_Init();
#endif

  /*** External RAM and NOR Flash *********************************************/
  BSP_XSPI_RAM_Init(0);
  BSP_XSPI_RAM_EnableMemoryMappedMode(0);

  BSP_XSPI_NOR_Init_t NOR_Init;
  NOR_Init.InterfaceMode = BSP_XSPI_NOR_OPI_MODE;
  NOR_Init.TransferRate = BSP_XSPI_NOR_DTR_TRANSFER;
  BSP_XSPI_NOR_Init(0, &NOR_Init);
  BSP_XSPI_NOR_EnableMemoryMappedMode(0);

  /* Set all required IPs as secure privileged */
  Security_Config();

  IAC_Config();
  set_clk_sleep_mode();

  LL_ATON_RT_RuntimeInit();

  /*** NN Init ****************************************************************/
  LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(face_detection);
  const LL_Buffer_InfoTypeDef *nn_in_info = LL_ATON_Input_Buffers_Info_face_detection();
  const LL_Buffer_InfoTypeDef *nn_out_info = LL_ATON_Output_Buffers_Info_face_detection();

  nn_in = (uint8_t *) LL_Buffer_addr_start(&nn_in_info[0]);
  float32_t *nn_out[MAX_NUMBER_OUTPUT];
  int32_t nn_out_len[MAX_NUMBER_OUTPUT];

  int number_output = 0;

  /* Count number of outputs */
  while (nn_out_info[number_output].name != NULL)
  {
    number_output++;
  }
  assert(number_output <= MAX_NUMBER_OUTPUT);

  for (int i = 0; i < number_output; i++)
  {
    nn_out[i] = (float32_t *) LL_Buffer_addr_start(&nn_out_info[i]);
    nn_out_len[i] = LL_Buffer_len(&nn_out_info[i]);
  }

  uint32_t nn_in_len = LL_Buffer_len(&nn_in_info[0]);
  uint32_t pitch_nn = 0;

  LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(face_recognition);
  const LL_Buffer_InfoTypeDef *fr_in_info = LL_ATON_Input_Buffers_Info_face_recognition();
  const LL_Buffer_InfoTypeDef *fr_out_info = LL_ATON_Output_Buffers_Info_face_recognition();
  fr_nn_in = (int8_t *) LL_Buffer_addr_start(&fr_in_info[0]);
  fr_nn_out = (int8_t *) LL_Buffer_addr_start(&fr_out_info[0]);
  fr_in_len = LL_Buffer_len(&fr_in_info[0]);
  fr_out_len = LL_Buffer_len(&fr_out_info[0]);

  UNUSED(nn_in_len);

  /* Test recognition with a fixed input to compare embeddings */
  memcpy(fr_nn_in, dummy_fr_input, DUMMY_FR_INPUT_SIZE);
  SCB_CleanInvalidateDCache_by_Addr(fr_nn_in, fr_in_len);
  RunNetworkSync(&NN_Instance_face_recognition);
  SCB_InvalidateDCache_by_Addr(fr_nn_out, fr_out_len);
  float32_t verify_tmp[EMBEDDING_SIZE];
  for (uint32_t i = 0; i < EMBEDDING_SIZE; i++)
  {
    verify_tmp[i] = ((float32_t)fr_nn_out[i]) / 128.f;
  }
  float verify_similarity =
    embedding_cosine_similarity(verify_tmp, target_embedding, EMBEDDING_SIZE);
  Display_Similarity(verify_similarity);
  LL_ATON_RT_DeInit_Network(&NN_Instance_face_recognition);

  /*** Post Processing Init ***************************************************/
  app_postprocess_init(&pp_params);

  /* Initialize camera/LCD/PC stream depending on configuration */
  App_InputInit(&pitch_nn);

  uint32_t ts[3] = { 0 };
  /*** App Loop ***************************************************************/
  while (1)
  {
    if (App_GetFrame(nn_rgb, pitch_nn) != 0)
    {
      continue;
    }

    img_rgb_to_hwc_float(nn_rgb, (float32_t *)nn_in, NN_WIDTH * NN_BPP,
                        NN_WIDTH, NN_HEIGHT);
    SCB_CleanInvalidateDCache_by_Addr(nn_in, nn_in_len);

    ts[0] = HAL_GetTick();
    RunNetworkSync(&NN_Instance_face_detection);
    LL_ATON_RT_DeInit_Network(&NN_Instance_face_detection);

    int32_t ret = app_postprocess_run((void **) nn_out, number_output, &pp_output, &pp_params);
    if (pp_output.box_nb > 0)
    {
      pd_pp_box_t *box = (pd_pp_box_t *)pp_output.pOutData;
      for (uint32_t b = 0; b < pp_output.box_nb; b++)
      {
        float cx = box[b].x_center * lcd_bg_area.XSize;
        float cy = box[b].y_center * lcd_bg_area.YSize;
        float w  = box[b].width  * lcd_bg_area.XSize;
        float h  = box[b].height * lcd_bg_area.YSize;
        float lx = box[b].pKps[0].x * lcd_bg_area.XSize;
        float ly = box[b].pKps[0].y * lcd_bg_area.YSize;
        float rx = box[b].pKps[1].x * lcd_bg_area.XSize;
        float ry = box[b].pKps[1].y * lcd_bg_area.YSize;
        img_crop_align565_to_888(img_buffer, lcd_bg_area.XSize, fr_rgb,
                                 lcd_bg_area.XSize, lcd_bg_area.YSize,
                                 FR_WIDTH, FR_HEIGHT,
                                 cx, cy, w, h, lx, ly, rx, ry);
        img_rgb_to_chw_s8(fr_rgb, fr_nn_in,
                          FR_WIDTH * NN_BPP, FR_WIDTH, FR_HEIGHT);
        SCB_CleanInvalidateDCache_by_Addr(fr_nn_in, fr_in_len);
        RunNetworkSync(&NN_Instance_face_recognition);
        SCB_InvalidateDCache_by_Addr(fr_nn_out, fr_out_len);
        float32_t tmp[EMBEDDING_SIZE];


        for (uint32_t i = 0; i < EMBEDDING_SIZE; i++)
        {
          tmp[i] = ((float32_t)fr_nn_out[i]) / 128.f;
        }
        float similarity = embedding_cosine_similarity(tmp, target_embedding, EMBEDDING_SIZE);
        box[b].prob = similarity;
        if (b == 0)
        {
          Display_Similarity(similarity);
        }
        LL_ATON_RT_DeInit_Network(&NN_Instance_face_recognition);
      }
    }
    ts[1] = HAL_GetTick();
    if (ts[2] == 0)
    {
      ts[2] = HAL_GetTick();
    }
    assert(ret == 0);

    App_Output(&pp_output, ts[1] - ts[0], ts[2]);
#ifdef ENABLE_PC_STREAM
    if (pp_output.box_nb > 0)
    {
      PC_STREAM_SendFrameEx(fr_rgb, FR_WIDTH, FR_HEIGHT, NN_BPP, "ALN");
    }
#endif

    /* Discard nn_out region (used by pp_input and pp_outputs variables) to avoid Dcache evictions during nn inference */
    for (int i = 0; i < number_output; i++)
    {
      float32_t *tmp = nn_out[i];
      SCB_InvalidateDCache_by_Addr(tmp, nn_out_len[i]);
    }
  }
}


void HAL_CACHEAXI_MspInit(CACHEAXI_HandleTypeDef *hcacheaxi)
{
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_ENABLE();
  __HAL_RCC_CACHEAXI_CLK_ENABLE();
  __HAL_RCC_CACHEAXI_FORCE_RESET();
  __HAL_RCC_CACHEAXI_RELEASE_RESET();
}

void HAL_CACHEAXI_MspDeInit(CACHEAXI_HandleTypeDef *hcacheaxi)
{
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_DISABLE();
  __HAL_RCC_CACHEAXI_CLK_DISABLE();
  __HAL_RCC_CACHEAXI_FORCE_RESET();
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  UNUSED(file);
  UNUSED(line);
  __BKPT(0);
  while (1)
  {
  }
}

#endif
