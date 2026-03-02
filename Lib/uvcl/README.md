# Uvc library

## Purpose

This document provides an overview of the Uvc Library (aka uvcl).

Uvcl will allows you to easily add Usb Uvc support to your N6 application. It has a very simple API and can be
configured at compile time.

## Build configuration

You can configure following items at build time :

* Low level usb stack : Can be either USBX or STM32_USBD
* Rtos support : Can be either THREADX, FREERTOS or NONE
* Dma support : Can be either YES or NO

## APIs

### Initialization

```C
/* Use UVCL_PAYLOAD_UNCOMPRESSED_YUY2 or UVCL_PAYLOAD_JPEG for maximal compatibility */
#define UVCL_PAYLOAD_UNCOMPRESSED_YUY2 0
#define UVCL_PAYLOAD_JPEG 1
/* Frame based formats */
#define UVCL_PAYLOAD_FB_RGB565 2
#define UVCL_PAYLOAD_FB_BGR3 3
#define UVCL_PAYLOAD_FB_GREY 4
#define UVCL_PAYLOAD_FB_H264 5
#define UVCL_PAYLOAD_FB_JPEG 6
#define UVCL_PAYLOAD_FB_GREY_D3DFMT_L8 7
```

```C
typedef struct {
  int payload_type;
  int width;
  int height;
  int fps;
  /* Only for variable size payload. If zero a default value is choose */
  uint32_t dwMaxVideoFrameSize;
} UVCL_StreamConf_t;
```

```C
typedef struct {
  UVCL_StreamConf_t streams[UVCL_MAX_STREAM_CONF_NB];
  int streams_nb;
  int is_immediate_mode;
} UVCL_Conf_t;
```

```C
typedef struct uvcl_callbacks {
  void (*streaming_active)(struct uvcl_callbacks *cbs, UVCL_StreamConf_t stream);
  void (*streaming_inactive)(struct uvcl_callbacks *cbs);
  void (*frame_release)(struct uvcl_callbacks *cbs, void *frame);
} UVCL_Callbacks_t;
```

```C
int UVCL_Init(PCD_TypeDef *pcd_instance, UVCL_Conf_t *conf, UVCL_Callbacks_t *cbs);
int UVCL_Deinit(void);
```

**NOTE**: UVCL_Deinit() is not yet implemented

### Display frame

User must call UVCL_ShowFrame(). Provided frame will be sent only once and then frame_release() callback is called to notify user that frame is available for next usage.

```C
/* return 0 if frame will be displayed. else it won't be displayed */
int UVCL_ShowFrame(void *frame, int frame_size);
```

## Software integration

### Interrupt
Application must implement USBx_OTG_HS_IRQHandler() interrupt handler and call UVCL_IRQHandler() from here.

```C
void USB1_OTG_HS_IRQHandler(void)
{
  UVCL_IRQHandler();
}
```

### USB device initialisation
Application must implement HAL_PCD_MspInit() and initialize USBx device instance (clocks, power, enable irqs, ...)

```C
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
  assert(hpcd->Instance == USB1_OTG_HS);

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_SLEEP_ENABLE();

  /* Enable the VDD33USB independent USB 33 voltage monitor */
  HAL_PWREx_EnableVddUSBVMEN();

  /* Wait until VDD33USB is ready */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_USB33RDY) == 0U);

  /* Enable VDDUSB supply */
  HAL_PWREx_EnableVddUSB();

  /* Enable USB1 OTG clock */
  __HAL_RCC_USB1_OTG_HS_CLK_ENABLE();
  __HAL_RCC_USB1_OTG_HS_CLK_SLEEP_ENABLE();

  /* Set FSEL to 24 Mhz */
  USB1_HS_PHYC->USBPHYC_CR &= ~(0x7U << 0x4U);
  USB1_HS_PHYC->USBPHYC_CR |= (0x2U << 0x4U);

  /* Enable USB1 OTG PHY clock */
  __HAL_RCC_USB1_OTG_HS_PHY_CLK_ENABLE();
  __HAL_RCC_USB1_OTG_HS_PHY_CLK_SLEEP_ENABLE();

  HAL_NVIC_SetPriority(USB1_OTG_HS_IRQn, 6U, 0U);

  /* Enable USB OTG interrupt */
  HAL_NVIC_EnableIRQ(USB1_OTG_HS_IRQn);
}
```

### Dma
In case you turn on dma usage you must provide uncache section name `.uncached_bss`

Needed uncache memory size is:
 * around 34K for usbx stack
 * around 6K for stm32_usbd stack

