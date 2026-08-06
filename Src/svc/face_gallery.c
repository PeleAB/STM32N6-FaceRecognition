#include "svc/face_gallery.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "stm32n6570_discovery_xspi.h"
#include "stm32n6xx_hal.h"

#define FACE_GALLERY_MAGIC 0x4647414cUL /* FGAL */
#define FACE_GALLERY_VERSION 1U
#define FACE_GALLERY_FLASH_ADDR 0x07FFC000UL
#define FACE_GALLERY_MATCH_SIMILARITY 0.65f

typedef struct __attribute__((packed)) {
  uint8_t valid;
  uint8_t name_len;
  char name[FACE_GALLERY_NAME_MAX + 1];
  int16_t embedding[FACE_GALLERY_EMBEDDING_SIZE];
} gallery_flash_entry_t;

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint16_t version;
  uint8_t count;
  uint8_t reserved;
  gallery_flash_entry_t entries[FACE_GALLERY_MAX_ENTRIES];
  uint32_t crc32;
} gallery_flash_page_t;

typedef struct {
  uint8_t valid;
  char name[FACE_GALLERY_NAME_MAX + 1];
  float embedding[FACE_GALLERY_EMBEDDING_SIZE];
} gallery_entry_t;

static gallery_entry_t s_entries[FACE_GALLERY_MAX_ENTRIES];
static uint8_t s_count;
static struct {
  uint8_t active;
  uint8_t samples;
  char name[FACE_GALLERY_NAME_MAX + 1];
  float sum[FACE_GALLERY_EMBEDDING_SIZE];
} s_enroll;
static StaticSemaphore_t s_mutex_buf;
static SemaphoreHandle_t s_mutex;
/* Gallery operations run from small FreeRTOS task stacks. Keep the 2.2 KiB
 * flash image and embedding scratch space in static RAM, never on a stack. */
static gallery_flash_page_t s_flash_page;
static float s_work_embedding[FACE_GALLERY_EMBEDDING_SIZE];

static uint8_t bounded_strlen(const char *s, uint8_t limit)
{
  uint8_t len = 0;
  while (len < limit && s[len] != '\0') len++;
  return len;
}

static uint32_t crc32(const uint8_t *data, uint32_t len)
{
  uint32_t crc = 0xffffffffUL;
  for (uint32_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint32_t bit = 0; bit < 8; bit++)
      crc = (crc >> 1) ^ ((crc & 1U) ? 0xedb88320UL : 0U);
  }
  return crc ^ 0xffffffffUL;
}

static int normalize(const float *input, float *output)
{
  float sum = 0.0f;
  for (int i = 0; i < FACE_GALLERY_EMBEDDING_SIZE; i++) {
    if (!isfinite(input[i])) return -1;
    sum += input[i] * input[i];
  }
  if (!(sum > 1.0e-12f) || !isfinite(sum)) return -1;
  float inv = 1.0f / sqrtf(sum);
  for (int i = 0; i < FACE_GALLERY_EMBEDDING_SIZE; i++)
    output[i] = input[i] * inv;
  return 0;
}

static void recount(void)
{
  s_count = 0;
  for (int i = 0; i < FACE_GALLERY_MAX_ENTRIES; i++)
    if (s_entries[i].valid) s_count++;
}

static face_gallery_status_t persist(void)
{
  memset(&s_flash_page, 0xff, sizeof(s_flash_page));
  s_flash_page.magic = FACE_GALLERY_MAGIC;
  s_flash_page.version = FACE_GALLERY_VERSION;
  s_flash_page.count = s_count;
  s_flash_page.reserved = 0xff;

  for (int slot = 0; slot < FACE_GALLERY_MAX_ENTRIES; slot++) {
    gallery_flash_entry_t *dst = &s_flash_page.entries[slot];
    const gallery_entry_t *src = &s_entries[slot];
    if (!src->valid) continue;
    dst->valid = 0xa5;
    dst->name_len = bounded_strlen(src->name, FACE_GALLERY_NAME_MAX);
    memcpy(dst->name, src->name, dst->name_len + 1U);
    for (int i = 0; i < FACE_GALLERY_EMBEDDING_SIZE; i++) {
      float scaled = src->embedding[i] * 32767.0f;
      if (scaled > 32767.0f) scaled = 32767.0f;
      if (scaled < -32767.0f) scaled = -32767.0f;
      dst->embedding[i] = (int16_t)lrintf(scaled);
    }
  }
  s_flash_page.crc32 = crc32((const uint8_t *)&s_flash_page,
                             offsetof(gallery_flash_page_t, crc32));

  int32_t ret = BSP_XSPI_NOR_DisableMemoryMappedMode(0);
  if (ret == BSP_ERROR_NONE)
    ret = BSP_XSPI_NOR_Erase_Block(0, FACE_GALLERY_FLASH_ADDR,
                                   BSP_XSPI_NOR_ERASE_4K);
  if (ret == BSP_ERROR_NONE)
    ret = BSP_XSPI_NOR_Write(0, (const uint8_t *)&s_flash_page,
                             FACE_GALLERY_FLASH_ADDR, sizeof(s_flash_page));
  int32_t mmp_ret = BSP_XSPI_NOR_EnableMemoryMappedMode(0);
  return (ret == BSP_ERROR_NONE && mmp_ret == BSP_ERROR_NONE)
             ? FACE_GALLERY_OK : FACE_GALLERY_ERR_FLASH;
}

void FaceGallery_Init(void)
{
  s_mutex = xSemaphoreCreateMutexStatic(&s_mutex_buf);
  configASSERT(s_mutex != NULL);
  memset(s_entries, 0, sizeof(s_entries));
  memset(&s_enroll, 0, sizeof(s_enroll));

  memcpy(&s_flash_page, (const void *)(XSPI2_BASE + FACE_GALLERY_FLASH_ADDR),
         sizeof(s_flash_page));
  if (s_flash_page.magic != FACE_GALLERY_MAGIC ||
      s_flash_page.version != FACE_GALLERY_VERSION ||
      s_flash_page.crc32 != crc32((const uint8_t *)&s_flash_page,
                          offsetof(gallery_flash_page_t, crc32)))
    return;

  for (int slot = 0; slot < FACE_GALLERY_MAX_ENTRIES; slot++) {
    const gallery_flash_entry_t *src = &s_flash_page.entries[slot];
    if (src->valid != 0xa5 || src->name_len == 0 ||
        src->name_len > FACE_GALLERY_NAME_MAX)
      continue;
    gallery_entry_t *dst = &s_entries[slot];
    dst->valid = 1;
    memcpy(dst->name, src->name, src->name_len);
    dst->name[src->name_len] = '\0';
    for (int i = 0; i < FACE_GALLERY_EMBEDDING_SIZE; i++)
      dst->embedding[i] = (float)src->embedding[i] / 32767.0f;
    if (normalize(dst->embedding, s_work_embedding) != 0) {
      memset(dst, 0, sizeof(*dst));
      continue;
    }
    memcpy(dst->embedding, s_work_embedding, sizeof(s_work_embedding));
  }
  recount();
}

face_gallery_status_t FaceGallery_StartEnrollment(const char *name, uint8_t name_len)
{
  if (!name || name_len == 0 || name_len > FACE_GALLERY_NAME_MAX)
    return FACE_GALLERY_ERR_NAME;
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  memset(&s_enroll, 0, sizeof(s_enroll));
  for (uint8_t i = 0; i < name_len; i++) {
    char c = name[i];
    if (c < 0x20 || c > 0x7e) {
      xSemaphoreGive(s_mutex);
      return FACE_GALLERY_ERR_NAME;
    }
    s_enroll.name[i] = c;
  }
  s_enroll.name[name_len] = '\0';
  s_enroll.active = 1;
  xSemaphoreGive(s_mutex);
  return FACE_GALLERY_OK;
}

void FaceGallery_OfferEmbedding(const float *embedding)
{
  if (!embedding) return;
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  if (s_enroll.active && s_enroll.samples < FACE_GALLERY_REQUIRED_SAMPLES &&
      normalize(embedding, s_work_embedding) == 0) {
    for (int i = 0; i < FACE_GALLERY_EMBEDDING_SIZE; i++)
      s_enroll.sum[i] += s_work_embedding[i];
    s_enroll.samples++;
  }
  xSemaphoreGive(s_mutex);
}

face_gallery_status_t FaceGallery_Commit(void)
{
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  if (!s_enroll.active || s_enroll.samples < FACE_GALLERY_MIN_SAMPLES) {
    xSemaphoreGive(s_mutex);
    return FACE_GALLERY_ERR_SAMPLES;
  }
  int slot = -1;
  for (int i = 0; i < FACE_GALLERY_MAX_ENTRIES; i++) {
    if (s_entries[i].valid && strcmp(s_entries[i].name, s_enroll.name) == 0) {
      slot = i;
      break;
    }
    if (slot < 0 && !s_entries[i].valid) slot = i;
  }
  if (slot < 0) {
    xSemaphoreGive(s_mutex);
    return FACE_GALLERY_ERR_FULL;
  }
  if (normalize(s_enroll.sum, s_work_embedding) != 0) {
    xSemaphoreGive(s_mutex);
    return FACE_GALLERY_ERR_SAMPLES;
  }
  s_entries[slot].valid = 1;
  strcpy(s_entries[slot].name, s_enroll.name);
  memcpy(s_entries[slot].embedding, s_work_embedding, sizeof(s_work_embedding));
  s_enroll.active = 0;
  recount();
  face_gallery_status_t st = persist();
  xSemaphoreGive(s_mutex);
  return st;
}

face_gallery_status_t FaceGallery_ImportQ7(const char *name, uint8_t name_len,
                                           const int8_t *embedding)
{
  if (!name || !embedding || name_len == 0 || name_len > FACE_GALLERY_NAME_MAX)
    return FACE_GALLERY_ERR_NAME;

  xSemaphoreTake(s_mutex, portMAX_DELAY);
  char clean_name[FACE_GALLERY_NAME_MAX + 1];
  for (uint8_t i = 0; i < name_len; i++) {
    if (name[i] < 0x20 || name[i] > 0x7e) {
      xSemaphoreGive(s_mutex);
      return FACE_GALLERY_ERR_NAME;
    }
    clean_name[i] = name[i];
  }
  clean_name[name_len] = '\0';

  for (int i = 0; i < FACE_GALLERY_EMBEDDING_SIZE; i++)
    s_work_embedding[i] = (float)embedding[i] / 127.0f;
  if (normalize(s_work_embedding, s_work_embedding) != 0) {
    xSemaphoreGive(s_mutex);
    return FACE_GALLERY_ERR_SAMPLES;
  }

  int slot = -1;
  for (int i = 0; i < FACE_GALLERY_MAX_ENTRIES; i++) {
    if (s_entries[i].valid && strcmp(s_entries[i].name, clean_name) == 0) {
      slot = i;
      break;
    }
    if (slot < 0 && !s_entries[i].valid) slot = i;
  }
  if (slot < 0) {
    xSemaphoreGive(s_mutex);
    return FACE_GALLERY_ERR_FULL;
  }
  s_entries[slot].valid = 1;
  strcpy(s_entries[slot].name, clean_name);
  memcpy(s_entries[slot].embedding, s_work_embedding, sizeof(s_work_embedding));
  recount();
  face_gallery_status_t st = persist();
  xSemaphoreGive(s_mutex);
  return st;
}

face_gallery_status_t FaceGallery_Clear(void)
{
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  memset(s_entries, 0, sizeof(s_entries));
  memset(&s_enroll, 0, sizeof(s_enroll));
  recount();
  face_gallery_status_t st = persist();
  xSemaphoreGive(s_mutex);
  return st;
}

face_gallery_status_t FaceGallery_Delete(uint8_t slot)
{
  if (slot >= FACE_GALLERY_MAX_ENTRIES) return FACE_GALLERY_ERR_SLOT;
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  if (!s_entries[slot].valid) {
    xSemaphoreGive(s_mutex);
    return FACE_GALLERY_ERR_SLOT;
  }
  memset(&s_entries[slot], 0, sizeof(s_entries[slot]));
  recount();
  face_gallery_status_t st = persist();
  xSemaphoreGive(s_mutex);
  return st;
}

void FaceGallery_GetStatus(face_gallery_enroll_status_t *status)
{
  if (!status) return;
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  status->active = s_enroll.active;
  status->samples = s_enroll.samples;
  status->required_samples = FACE_GALLERY_REQUIRED_SAMPLES;
  status->count = s_count;
  strcpy(status->name, s_enroll.name);
  xSemaphoreGive(s_mutex);
}

uint8_t FaceGallery_Count(void) { return s_count; }

int FaceGallery_GetName(uint8_t slot, char *name, uint8_t capacity)
{
  if (!name || capacity == 0 || slot >= FACE_GALLERY_MAX_ENTRIES) return -1;
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  if (!s_entries[slot].valid) {
    xSemaphoreGive(s_mutex);
    return -1;
  }
  strncpy(name, s_entries[slot].name, capacity - 1U);
  name[capacity - 1U] = '\0';
  xSemaphoreGive(s_mutex);
  return 0;
}

int FaceGallery_Match(const float *embedding, char *name, uint8_t capacity,
                      float *similarity)
{
  if (!embedding || !name || capacity == 0) return 0;
  name[0] = '\0';
  if (similarity) *similarity = 0.0f;
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  if (normalize(embedding, s_work_embedding) != 0) {
    xSemaphoreGive(s_mutex);
    return 0;
  }
  int best = -1;
  float best_sim = -1.0f;
  for (int slot = 0; slot < FACE_GALLERY_MAX_ENTRIES; slot++) {
    if (!s_entries[slot].valid) continue;
    float sim = 0.0f;
    for (int i = 0; i < FACE_GALLERY_EMBEDDING_SIZE; i++)
      sim += s_work_embedding[i] * s_entries[slot].embedding[i];
    if (sim > best_sim) {
      best_sim = sim;
      best = slot;
    }
  }
  if (similarity) *similarity = best_sim;
  if (best >= 0 && best_sim >= FACE_GALLERY_MATCH_SIMILARITY) {
    strncpy(name, s_entries[best].name, capacity - 1U);
    name[capacity - 1U] = '\0';
    xSemaphoreGive(s_mutex);
    return 1;
  }
  xSemaphoreGive(s_mutex);
  return 0;
}
