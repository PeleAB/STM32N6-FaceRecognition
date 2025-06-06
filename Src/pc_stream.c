#include "pc_stream.h"
#include "stm32n6570_discovery.h"
#include "stm32n6570_discovery_conf.h"
#include "stm32n6xx_hal_uart.h"
#include <stdio.h>

#if (USE_BSP_COM_FEATURE > 0)
extern UART_HandleTypeDef hcom_uart[COMn];

static MX_UART_InitTypeDef PcUartInit = {
    .BaudRate = 115200,
    .WordLength = UART_WORDLENGTH_8B,
    .StopBits = UART_STOPBITS_1,
    .Parity = UART_PARITY_NONE,
    .HwFlowCtl = UART_HWCONTROL_NONE
};

void PC_STREAM_Init(void)
{
    BSP_COM_Init(COM1, &PcUartInit);
#if (USE_COM_LOG > 0)
    BSP_COM_SelectLogPort(COM1);
#endif
}

void PC_STREAM_SendFrame(const uint8_t *frame, uint32_t width, uint32_t height, uint32_t bpp)
{
    uint32_t size = width * height * bpp;
    printf("FRAME %lu %lu %lu\n", (unsigned long)width, (unsigned long)height, (unsigned long)bpp);
    HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)frame, size, HAL_MAX_DELAY);
}

void PC_STREAM_SendDetections(const od_pp_out_t *detections, uint32_t frame_id)
{
    printf("DETS %lu %d\n", (unsigned long)frame_id, (int)detections->nb_detect);
    for(int i=0; i<detections->nb_detect; i++)
    {
        const od_pp_outBuffer_t *r = &detections->pOutBuff[i];
        printf("%d %.3f %.3f %.3f %.3f %.2f\n", r->class_index, (double)r->x_center,
               (double)r->y_center, (double)r->width, (double)r->height, (double)r->conf);
    }
    printf("END\n");
}

#endif /* USE_BSP_COM_FEATURE */
