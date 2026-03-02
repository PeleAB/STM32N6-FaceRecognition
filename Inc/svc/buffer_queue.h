/**
 ******************************************************************************
 * @file    buffer_queue.h
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

#ifndef SVC_BUFFER_QUEUE_H
#define SVC_BUFFER_QUEUE_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

#define BQUEUE_MAX_BUFFERS 2

typedef struct {
  SemaphoreHandle_t free;
  StaticSemaphore_t free_buffer;
  SemaphoreHandle_t ready;
  StaticSemaphore_t ready_buffer;
  int buffer_nb;
  uint8_t *buffers[BQUEUE_MAX_BUFFERS];
  int free_idx;
  int ready_idx;
} bqueue_t;

int bqueue_init(bqueue_t *bq, int buffer_nb, uint8_t **buffers);
uint8_t *bqueue_get_free(bqueue_t *bq, int is_blocking);
void bqueue_put_free(bqueue_t *bq);
uint8_t *bqueue_get_ready(bqueue_t *bq);
void bqueue_put_ready(bqueue_t *bq);

#endif /* SVC_BUFFER_QUEUE_H */

