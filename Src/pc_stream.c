#include "pc_stream.h"
#include "stm32n6570_discovery.h"
#include "stm32n6570_discovery_conf.h"
#include "stm32n6xx_hal_uart.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define UART_CHUNK_SIZE 1024*16  // adjust as needed

#if (USE_BSP_COM_FEATURE > 0)
extern UART_HandleTypeDef hcom_uart[COMn];

static MX_UART_InitTypeDef PcUartInit = {
    .BaudRate = 921600*8,
    .WordLength = UART_WORDLENGTH_8B,
    .StopBits = UART_STOPBITS_1,
    .Parity = UART_PARITY_NONE,
    .HwFlowCtl = UART_HWCONTROL_NONE
};
static uint8_t jpeg_buf[64 * 512];
void PC_STREAM_Init(void)
{
    BSP_COM_Init(COM1, &PcUartInit);
#if (USE_COM_LOG > 0)
    BSP_COM_SelectLogPort(COM1);
#endif
}

#define STREAM_SCALE 2
#define STREAM_MAX_WIDTH  (LCD_FG_WIDTH / STREAM_SCALE)
#define STREAM_MAX_HEIGHT (LCD_FG_HEIGHT / STREAM_SCALE)

static uint8_t stream_buffer[STREAM_MAX_WIDTH * STREAM_MAX_HEIGHT];

typedef struct {
    uint8_t *buf;
    int size;
    int cap;
} mem_writer_t;

static void mem_write(void *context, void *data, int size)
{
    mem_writer_t *wr = (mem_writer_t *)context;
    if (wr->size + size <= wr->cap)
    {
        memcpy(wr->buf + wr->size, data, size);
        wr->size += size;
    }
}

static uint8_t rgb565_to_gray(uint16_t pixel)
{
    uint8_t r8 = ((pixel >> 11) & 0x1F) << 3;
    uint8_t g8 = ((pixel >> 5) & 0x3F) << 2;
    uint8_t b8 = (pixel & 0x1F) << 3;
    return (uint8_t)((r8 * 30 + g8 * 59 + b8 * 11) / 100);
}

static uint8_t rgb888_to_gray(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint8_t)((r * 30 + g * 59 + b * 11) / 100);
}

void PC_STREAM_SendFrame(const uint8_t *frame, uint32_t width, uint32_t height, uint32_t bpp)
{

    uint32_t sw = width / STREAM_SCALE;
    uint32_t sh = height / STREAM_SCALE;
    if (sw > STREAM_MAX_WIDTH)  sw = STREAM_MAX_WIDTH;
    if (sh > STREAM_MAX_HEIGHT) sh = STREAM_MAX_HEIGHT;

    // Convert input to grayscale and store in stream_buffer
    for (uint32_t y = 0; y < sh; y++)
    {
        const uint8_t *line = frame + (y * STREAM_SCALE) * width * bpp;
        for (uint32_t x = 0; x < sw; x++)
        {
            if (bpp == 2)
            {
                const uint16_t *line16 = (const uint16_t *)line;
                uint16_t px = line16[x * STREAM_SCALE];
                stream_buffer[y * sw + x] = rgb565_to_gray(px);
            }
            else if (bpp == 3)
            {
                const uint8_t *px = line + x * STREAM_SCALE * 3;
                stream_buffer[y * sw + x] = rgb888_to_gray(px[0], px[1], px[2]);
            }
            else
            {
                stream_buffer[y * sw + x] = line[x * STREAM_SCALE];
            }
        }
    }

    // Encode to JPEG into jpeg_buf
    mem_writer_t w = { jpeg_buf, 0, sizeof(jpeg_buf) };
    stbi_write_jpg_to_func(mem_write, &w, sw, sh, 1, stream_buffer, 80);

    // Send a simple header first
    char header[32];
    int hl = snprintf(header, sizeof(header), "JPG %u %u %u\n",
                      (unsigned)sw, (unsigned)sh, (unsigned)w.size);
    if (hl > 0)
    {
        HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)header, (uint16_t)hl, HAL_MAX_DELAY);
    }

    // Now send jpeg_buf in chunks
    uint32_t bytes_left = w.size;
    uint8_t *ptr = jpeg_buf;
    while (bytes_left > 0)
    {
        uint16_t this_chunk = (bytes_left > UART_CHUNK_SIZE)
                               ? UART_CHUNK_SIZE
                               : (uint16_t)bytes_left;

        HAL_UART_Transmit(&hcom_uart[COM1], ptr, this_chunk, HAL_MAX_DELAY);

        ptr         += this_chunk;
        bytes_left  -= this_chunk;
    }
}

void PC_STREAM_SendDetections(const pd_postprocess_out_t *detections, uint32_t frame_id)
{
    char line[128];
    int ll = snprintf(line, sizeof(line), "DETS %lu %lu\n", (unsigned long)frame_id,
                      (unsigned long)detections->box_nb);
    if (ll > 0)
    {
        HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)line, (uint16_t)ll, HAL_MAX_DELAY);
    }
    for (uint32_t i = 0; i < detections->box_nb; i++)
    {
        const pd_pp_box_t *b = &detections->pOutData[i];
        ll = snprintf(line, sizeof(line), "0 %.3f %.3f %.3f %.3f %.2f",
                      (double)b->x_center, (double)b->y_center,
                      (double)b->width, (double)b->height,
                      (double)b->prob);
        for (uint32_t k = 0; k < AI_PD_MODEL_PP_NB_KEYPOINTS; k++)
        {
            int n = snprintf(line + ll, sizeof(line) - ll, " %.3f %.3f",
                             (double)b->pKps[k].x, (double)b->pKps[k].y);
            if (n > 0)
                ll += n;
        }
        if (ll < (int)sizeof(line) - 1)
        {
            line[ll++] = '\n';
            line[ll] = '\0';
        }
        HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)line, (uint16_t)ll, HAL_MAX_DELAY);
    }
    static const char end_marker[] = "END\n";
    HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)end_marker, sizeof(end_marker) - 1, HAL_MAX_DELAY);
}


int PC_STREAM_ReceiveImage(uint8_t *buffer, uint32_t length)
{
    HAL_StatusTypeDef st = HAL_OK;
    uint32_t remaining = length;
    uint8_t *ptr = buffer;

    while (remaining > 0 && st == HAL_OK)
    {
        /* HAL UART API takes a uint16_t size parameter */
        uint16_t chunk = (remaining > 0xFFFFU) ? 0xFFFFU : (uint16_t)remaining;
        st = HAL_UART_Receive(&hcom_uart[COM1], ptr, chunk, HAL_MAX_DELAY);
        if (st != HAL_OK)
        {
            break;
        }
        ptr += chunk;
        remaining -= chunk;
    }

    return (st == HAL_OK && remaining == 0U) ? 0 : -1;
}

#endif /* USE_BSP_COM_FEATURE */
