#ifndef FACE_GALLERY_H
#define FACE_GALLERY_H

#include <stdint.h>

#include "stai_faceid.h"

#define FACE_GALLERY_EMBEDDING_SIZE STAI_FACEID_OUT_1_SIZE
#define FACE_GALLERY_MAX_ENTRIES 8
#define FACE_GALLERY_NAME_MAX 15
#define FACE_GALLERY_REQUIRED_SAMPLES 8
#define FACE_GALLERY_MIN_SAMPLES 5

typedef enum {
  FACE_GALLERY_OK = 0,
  FACE_GALLERY_ERR_NAME = 1,
  FACE_GALLERY_ERR_BUSY = 2,
  FACE_GALLERY_ERR_SAMPLES = 3,
  FACE_GALLERY_ERR_FULL = 4,
  FACE_GALLERY_ERR_FLASH = 5,
  FACE_GALLERY_ERR_SLOT = 6,
} face_gallery_status_t;

typedef struct {
  uint8_t active;
  uint8_t samples;
  uint8_t required_samples;
  uint8_t count;
  char name[FACE_GALLERY_NAME_MAX + 1];
} face_gallery_enroll_status_t;

void FaceGallery_Init(void);
face_gallery_status_t FaceGallery_StartEnrollment(const char *name, uint8_t name_len);
void FaceGallery_OfferEmbedding(const float *embedding);
face_gallery_status_t FaceGallery_Commit(void);
face_gallery_status_t FaceGallery_ImportQ7(const char *name, uint8_t name_len,
                                           const int8_t *embedding);
face_gallery_status_t FaceGallery_Clear(void);
face_gallery_status_t FaceGallery_Delete(uint8_t slot);
void FaceGallery_GetStatus(face_gallery_enroll_status_t *status);
uint8_t FaceGallery_Count(void);
int FaceGallery_GetName(uint8_t slot, char *name, uint8_t capacity);
int FaceGallery_Match(const float *embedding, char *name, uint8_t capacity,
                      float *similarity);

#endif
