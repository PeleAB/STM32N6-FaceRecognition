#ifndef PC_STREAM_H
#define PC_STREAM_H

#include <stdint.h>
#include "od_pp_output_if.h"
#if POSTPROCESS_TYPE == POSTPROCESS_MPE_PD_UF
#include "pd_pp_output_if.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void PC_STREAM_Init(void);
void PC_STREAM_SendFrame(const uint8_t *frame, uint32_t width, uint32_t height, uint32_t bpp);
#if POSTPROCESS_TYPE == POSTPROCESS_MPE_PD_UF
void PC_STREAM_SendDetections(const pd_postprocess_out_t *detections,
                              uint32_t frame_id);
#else
void PC_STREAM_SendDetections(const od_pp_out_t *detections,
                              uint32_t frame_id);
#endif
int  PC_STREAM_ReceiveImage(uint8_t *buffer, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* PC_STREAM_H */
