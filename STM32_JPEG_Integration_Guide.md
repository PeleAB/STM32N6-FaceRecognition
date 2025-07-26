# STM32 JPEG Hardware Compression Integration Guide

## Overview

This guide shows how to integrate JPEG hardware compression/decompression into STM32N6 projects based on the STM32N6570-DK example implementation.

## Hardware Requirements

- STM32N6 series MCU with built-in JPEG hardware codec
- Sufficient RAM for image buffers and DMA transfers
- HPDMA controller for efficient data transfer

## Key Components

### 1. HAL Configuration (`stm32n6xx_hal_conf.h`)

Enable required HAL modules:

```c
#define HAL_MODULE_ENABLED
#define HAL_JPEG_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
```

### 2. JPEG Utils Configuration (`jpeg_utils_conf.h`)

```c
#include "stm32n6xx_hal.h"
#include "stm32n6xx_hal_jpeg.h"

// RGB format options
#define JPEG_ARGB8888    0
#define JPEG_RGB888      1  
#define JPEG_RGB565      2

// Enable encoder/decoder
#define USE_JPEG_DECODER 1
#define USE_JPEG_ENCODER 1

// Select RGB format for your application
#define JPEG_RGB_FORMAT JPEG_ARGB8888

// Color swap configuration
#define JPEG_SWAP_RG 0
```

### 3. Hardware Initialization

#### JPEG Peripheral Init

```c
static void jpeg_init(void)
{
    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_JPEG, 
                                          RIF_ATTRIBUTE_PRIV | RIF_ATTRIBUTE_SEC);
    hjpeg.Instance = JPEG;
    if (HAL_JPEG_Init(&hjpeg) != HAL_OK) {
        Error_Handler();
    }
}
```

#### DMA Configuration

```c
static void dma_init(void)
{
    __HAL_RCC_HPDMA1_CLK_ENABLE();
    
    // Configure input DMA channel (Memory to JPEG)
    handle_HPDMA1_Channel0.Instance = HPDMA1_Channel0;
    handle_HPDMA1_Channel0.Init.Request = HPDMA1_REQUEST_JPEG_RX;
    handle_HPDMA1_Channel0.Init.Direction = DMA_MEMORY_TO_PERIPH;
    handle_HPDMA1_Channel0.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_HPDMA1_Channel0.Init.DestInc = DMA_DINC_FIXED;
    handle_HPDMA1_Channel0.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    handle_HPDMA1_Channel0.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    
    // Configure output DMA channel (JPEG to Memory)  
    handle_HPDMA1_Channel1.Instance = HPDMA1_Channel1;
    handle_HPDMA1_Channel1.Init.Request = HPDMA1_REQUEST_JPEG_TX;
    handle_HPDMA1_Channel1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_HPDMA1_Channel1.Init.SrcInc = DMA_SINC_FIXED;
    handle_HPDMA1_Channel1.Init.DestInc = DMA_DINC_INCREMENTED;
    
    HAL_DMA_Init(&handle_HPDMA1_Channel0);
    HAL_DMA_Init(&handle_HPDMA1_Channel1);
    
    __HAL_LINKDMA(&hjpeg, hdmain, handle_HPDMA1_Channel0);
    __HAL_LINKDMA(&hjpeg, hdmaout, handle_HPDMA1_Channel1);
}
```

## JPEG Encoding Implementation

### 1. Basic Encoding Function

```c
uint32_t jpeg_encode_dma(JPEG_HandleTypeDef *hjpeg, 
                         uint32_t rgb_image_address, 
                         uint32_t rgb_image_size_bytes, 
                         uint32_t *jpeg_buffer_address)
{
    // Get image configuration
    RGB_GetInfo(&jpeg_conf);
    JPEG_GetEncodeColorConvertFunc(&jpeg_conf, &rgb_to_ycbcr_convert_func, &mcu_total_nb);
    
    // Configure JPEG encoding parameters
    HAL_JPEG_ConfigEncoding(hjpeg, &jpeg_conf);
    
    // Start DMA-based encoding
    HAL_JPEG_Encode_DMA(hjpeg, input_buffer, input_size, output_buffer, output_size);
    
    return 0;
}
```

### 2. Image Configuration

```c
void RGB_GetInfo(JPEG_ConfTypeDef *pInfo)
{
    pInfo->ImageWidth = RGB_IMAGE_WIDTH;
    pInfo->ImageHeight = RGB_IMAGE_HEIGHT;
    pInfo->ChromaSubsampling = JPEG_420_SUBSAMPLING;  // or JPEG_444_SUBSAMPLING
    pInfo->ColorSpace = JPEG_YCBCR_COLORSPACE;
    pInfo->ImageQuality = 90;  // 1-100 quality level
    
    // Validate image dimensions (must be multiples of 8/16)
    if (((pInfo->ImageWidth % 8) != 0) || ((pInfo->ImageHeight % 8) != 0)) {
        Error_Handler();
    }
}
```

### 3. Required Callbacks

```c
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbEncodedData)
{
    // Handle input data consumption
    if (NbEncodedData == input_buffer_size) {
        input_buffer_state = JPEG_BUFFER_EMPTY;
        HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_INPUT);
        input_is_paused = 1;
    } else {
        HAL_JPEG_ConfigInputBuffer(hjpeg, input_buffer + NbEncodedData, 
                                   input_buffer_size - NbEncodedData);
    }
}

void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, 
                                uint8_t *pDataOut, 
                                uint32_t OutDataLength)
{
    // Handle output data availability
    output_buffer_state = JPEG_BUFFER_FULL;
    output_buffer_size = OutDataLength;
    
    HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
    output_is_paused = 1;
    
    HAL_JPEG_ConfigOutputBuffer(hjpeg, output_buffer, CHUNK_SIZE_OUT);
}

void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{
    jpeg_hw_encoding_end = 1;
}

void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg)
{
    Error_Handler();
}
```

## JPEG Decoding Implementation

### Basic Decoding Function

```c
uint32_t jpeg_decode_dma(JPEG_HandleTypeDef *hjpeg, 
                         uint32_t jpeg_source_address, 
                         uint32_t jpeg_frame_size, 
                         uint32_t dest_address)
{
    jpeg_source_address_ptr = jpeg_source_address;
    frame_buffer_address = dest_address;
    input_frame_index = 0;
    input_frame_size = jpeg_frame_size;
    jpeg_hw_decoding_end = 0;
    
    HAL_JPEG_Decode_DMA(hjpeg, (uint8_t *)jpeg_source_address, 
                        CHUNK_SIZE_IN, (uint8_t *)dest_address, CHUNK_SIZE_OUT);
    
    return 0;
}
```

## Memory Management

### Buffer Sizing

```c
// Input buffers for encoding
#define MAX_INPUT_WIDTH     640
#define MAX_INPUT_LINES     16
#define BYTES_PER_PIXEL     4  // ARGB8888
#define CHUNK_SIZE_IN       (MAX_INPUT_WIDTH * BYTES_PER_PIXEL * MAX_INPUT_LINES)

// Output buffer for compressed data
#define CHUNK_SIZE_OUT      4096

// Allocate buffers
uint8_t input_buffer[CHUNK_SIZE_IN] __attribute__((aligned(32)));
uint8_t output_buffer[CHUNK_SIZE_OUT] __attribute__((aligned(32)));
uint32_t jpeg_output_final[IMAGE_WIDTH * IMAGE_HEIGHT]; // Final output buffer
```

## Integration Steps

### 1. Project Configuration

Add to your project:
- Enable JPEG HAL module
- Configure DMA channels
- Add JPEG interrupt handlers
- Include JPEG utility libraries

### 2. File Structure

```
Inc/
├── jpeg_utils_conf.h      // JPEG configuration
├── encode_dma.h           // Encoding functions
├── decode_dma.h           // Decoding functions  
└── main.h                 // Main declarations

Src/
├── encode_dma.c           // Encoding implementation
├── decode_dma.c           // Decoding implementation
├── stm32n6xx_hal_msp.c    // MSP configuration
└── main.c                 // Application main
```

### 3. Interrupt Handlers

Add to `stm32n6xx_it.c`:

```c
void JPEG_IRQHandler(void)
{
    HAL_JPEG_IRQHandler(&hjpeg);
}

void HPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&handle_HPDMA1_Channel0);
}

void HPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&handle_HPDMA1_Channel1);
}
```

### 4. Main Application Flow

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    // Initialize peripherals
    dma_init();
    jpeg_init();
    
    // Initialize JPEG color tables
    JPEG_InitColorTables();
    
    // Start encoding
    jpeg_encode_dma(&hjpeg, rgb_image_address, rgb_image_size, jpeg_buffer);
    
    // Process in background
    uint32_t encode_complete = 0;
    uint32_t tick_start = HAL_GetTick();
    
    do {
        jpeg_encode_input_handler(&hjpeg);
        encode_complete = jpeg_encode_output_handler(&hjpeg);
    } while ((encode_complete == 0) && ((HAL_GetTick() - tick_start) < 5000));
    
    while (1) {
        // Application loop
    }
}
```

## Performance Considerations

- Use DMA for efficient data transfer
- Align buffers to 32-byte boundaries
- Configure appropriate DMA burst sizes
- Process data in chunks to minimize memory usage
- Use hardware color space conversion when possible

## Troubleshooting

1. **Image dimension errors**: Ensure width/height are multiples of 8 (or 16 for 4:2:0 subsampling)
2. **DMA transfer issues**: Check buffer alignment and size constraints
3. **Quality issues**: Adjust `ImageQuality` parameter (1-100)
4. **Memory errors**: Verify sufficient RAM for buffers and intermediate processing

This integration provides hardware-accelerated JPEG compression/decompression with efficient DMA-based data handling suitable for real-time applications.