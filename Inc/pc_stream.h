#ifndef PC_STREAM_H
#define PC_STREAM_H

#include <stdint.h>
#include "pd_pp_output_if.h"

#ifdef __cplusplus
extern "C" {
#endif

void PC_STREAM_Init(void);
void PC_STREAM_SendFrame(const uint8_t *frame, uint32_t width, uint32_t height, uint32_t bpp);
void PC_STREAM_SendDetections(const pd_postprocess_out_t *detections,
                              uint32_t frame_id);

void PC_STREAM_SendFrameEx(const uint8_t *frame, uint32_t width,
                           uint32_t height, uint32_t bpp,
                           const char *tag);

int  PC_STREAM_ReceiveImage(uint8_t *buffer, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* PC_STREAM_H */
