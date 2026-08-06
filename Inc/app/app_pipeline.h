/**
 ******************************************************************************
 * @file    app_pipeline.h
 * @author  GPM Application Team
 *
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

#ifndef APP_PIPELINE_H
#define APP_PIPELINE_H

#include <stdint.h>
#include "svc/face_gallery.h"

void app_pipeline_init(void);
void app_pipeline_start(void);
void app_pipeline_set_face_input_mode(uint32_t mode);
face_gallery_status_t app_pipeline_gallery_commit(void);
face_gallery_status_t app_pipeline_gallery_clear(void);
face_gallery_status_t app_pipeline_gallery_delete(uint8_t slot);
face_gallery_status_t app_pipeline_gallery_import_q7(const char *name,
                                                     uint8_t name_len,
                                                     const int8_t *embedding);

#endif

