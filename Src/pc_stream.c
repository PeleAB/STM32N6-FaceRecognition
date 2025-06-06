#include "pc_stream.h"
#include "stm32n6570_discovery.h"
#include "stm32n6570_discovery_conf.h"
#include "stm32n6xx_hal_uart.h"
#include "app_config.h"
#include <stdio.h>

#if (USE_BSP_COM_FEATURE > 0)
extern UART_HandleTypeDef hcom_uart[COMn];

static MX_UART_InitTypeDef PcUartInit = {
    .BaudRate = 921600,
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

#define STREAM_SCALE 5
#define STREAM_WIDTH  (LCD_FG_WIDTH / STREAM_SCALE)
#define STREAM_HEIGHT (LCD_FG_HEIGHT / STREAM_SCALE)

static uint8_t stream_buffer[STREAM_WIDTH * STREAM_HEIGHT];

static uint8_t rgb565_to_gray(uint16_t pixel)
{
    uint8_t r8 = ((pixel >> 11) & 0x1F) << 3;
    uint8_t g8 = ((pixel >> 5) & 0x3F) << 2;
    uint8_t b8 = (pixel & 0x1F) << 3;
    return (uint8_t)((r8 * 30 + g8 * 59 + b8 * 11) / 100);
}

void PC_STREAM_SendFrame(const uint8_t *frame, uint32_t width, uint32_t height, uint32_t bpp)
{
    (void)bpp; /* expect RGB565 */
    const uint16_t *src = (const uint16_t *)frame;
    for (uint32_t y = 0; y < STREAM_HEIGHT; y++)
    {
        const uint16_t *line = src + (y * STREAM_SCALE) * width;
        for (uint32_t x = 0; x < STREAM_WIDTH; x++)
        {
            uint16_t px = line[x * STREAM_SCALE];
            stream_buffer[y * STREAM_WIDTH + x] = rgb565_to_gray(px);
        }
    }

    char header[32];
    int hl = snprintf(header, sizeof(header), "FRAME %u %u 1\n", (unsigned)STREAM_WIDTH, (unsigned)STREAM_HEIGHT);
    if (hl > 0)
    {
        HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)header, (uint16_t)hl, HAL_MAX_DELAY);
    }
    HAL_UART_Transmit(&hcom_uart[COM1], stream_buffer, sizeof(stream_buffer), HAL_MAX_DELAY);
}

void PC_STREAM_SendDetections(const od_pp_out_t *detections, uint32_t frame_id)
{
    char line[64];
    int ll = snprintf(line, sizeof(line), "DETS %lu %d\n", (unsigned long)frame_id, (int)detections->nb_detect);
    if (ll > 0)
    {
        HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)line, (uint16_t)ll, HAL_MAX_DELAY);
    }
    for(int i=0; i<detections->nb_detect; i++)
    {
        const od_pp_outBuffer_t *r = &detections->pOutBuff[i];
        ll = snprintf(line, sizeof(line), "%d %.3f %.3f %.3f %.3f %.2f\n", (int)r->class_index,
                      (double)r->x_center, (double)r->y_center, (double)r->width,
                      (double)r->height, (double)r->conf);
        if (ll > 0)
        {
            HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)line, (uint16_t)ll, HAL_MAX_DELAY);
        }
    }
    static const char end_marker[] = "END\n";
    HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)end_marker, sizeof(end_marker) - 1, HAL_MAX_DELAY);
}

#endif /* USE_BSP_COM_FEATURE */
