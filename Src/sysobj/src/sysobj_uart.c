/**
 * @file sysobj_uart.c
 * @brief UART messaging module implementation for sysobj
 */

#include "sysobj_uart.h"
#include <stddef.h>
#include <string.h>

/**
 * Internal Software CRC32 Implementation
 * IEEE 802.3 CRC32 polynomial: 0xEDB88320
 */
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

static void init_crc32_table(void) {
  uint32_t polynomial = 0xEDB88320;
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t crc = i;
    for (uint32_t j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ polynomial;
      } else {
        crc >>= 1;
      }
    }
    crc32_table[i] = crc;
  }
  crc32_table_initialized = true;
}

uint32_t sysobj_uart_calculate_crc32(const uint8_t *data, uint16_t len) {
  if (!crc32_table_initialized) {
    init_crc32_table();
  }
  uint32_t crc = 0xFFFFFFFF;
  for (uint16_t i = 0; i < len; i++) {
    uint8_t lookup_idx = (crc ^ data[i]) & 0xFF;
    crc = (crc >> 8) ^ crc32_table[lookup_idx];
  }
  return crc ^ 0xFFFFFFFF;
}

sysobj_uart_error_t sysobj_uart_parse(const uint8_t *buffer, uint16_t len,
                                      sysobj_uart_msg_t *msg) {
  if (buffer == NULL || msg == NULL) {
    return SYSOBJ_UART_ERROR_NULL_POINTER;
  }

  /* Minimum frame size = SOF(1) + SIZE(1) + CHK(1) + PAYLOAD_HDR(5) + CRC32(4)
   * = 12 bytes */
  if (len < 12) {
    return SYSOBJ_UART_ERROR_INVALID_FRAME_LENGTH;
  }

  if (buffer[0] != SYSOBJ_UART_SOF) {
    return SYSOBJ_UART_ERROR_INVALID_SOF;
  }

  uint8_t payload_size = buffer[1];
  if (payload_size < SYSOBJ_UART_PAYLOAD_HEADER_SIZE) {
    return SYSOBJ_UART_ERROR_INVALID_FRAME_LENGTH; /* Payload must at least
                                                      contain header */
  }

  /* Checksum is simply SOF + payload_size */
  uint8_t expected_checksum = (uint8_t)(buffer[0] + payload_size);
  if (buffer[2] != expected_checksum) {
    return SYSOBJ_UART_ERROR_INVALID_HEADER_CHECKSUM;
  }

  uint16_t expected_total_len = 3 + payload_size + 4;
  if (len < expected_total_len) {
    return SYSOBJ_UART_ERROR_INVALID_FRAME_LENGTH;
  }

  const uint8_t *payload = &buffer[3];

  /* Decouple CRC32 (Little endian format typical, but we'll extract simply
   * byte-by-byte) */
  uint32_t received_crc = ((uint32_t)buffer[3 + payload_size]) |
                          ((uint32_t)buffer[3 + payload_size + 1] << 8) |
                          ((uint32_t)buffer[3 + payload_size + 2] << 16) |
                          ((uint32_t)buffer[3 + payload_size + 3] << 24);

  uint32_t calculated_crc = sysobj_uart_calculate_crc32(payload, payload_size);
  if (received_crc != calculated_crc) {
    return SYSOBJ_UART_ERROR_INVALID_CRC32;
  }

  /* Payload header unpack */
  msg->src_id = payload[0];
  msg->dst_id = payload[1];
  /* is_ack occupies high 4 bits, need_ack occupies low 4 bits */
  msg->is_ack = (payload[2] >> 4) & 0x0F;
  msg->need_ack = payload[2] & 0x0F;
  msg->msg_type = payload[3];
  msg->msg_subtype = payload[4];

  msg->data_len = payload_size - SYSOBJ_UART_PAYLOAD_HEADER_SIZE;
  if (msg->data_len > 0) {
    msg->data = &payload[5];
  } else {
    msg->data = NULL;
  }

  return SYSOBJ_UART_ERROR_NONE;
}

/* --- Message Dispatch & Lookup Table --- */

typedef void (*sysobj_uart_handler_t)(const sysobj_uart_msg_t *msg);

typedef struct {
  uint8_t msg_type;
  uint8_t msg_subtype;
  sysobj_uart_handler_t handler;
} sysobj_uart_handler_entry_t;

__attribute__((weak)) void sysobj_uart_handle_manage_set_led(uint8_t led_id,
                                                             uint8_t state) {
  /* Default weak implementation. Override in main app application code. */
  (void)led_id;
  (void)state;
}

static void internal_handle_manage_set_led(const sysobj_uart_msg_t *msg) {
  if (msg->data != NULL && msg->data_len >= 2) {
    uint8_t led_id = msg->data[0];
    uint8_t state = msg->data[1];
    sysobj_uart_handle_manage_set_led(led_id, state);
  }
}

static const sysobj_uart_handler_entry_t msg_handler_table[] = {
    {SYSOBJ_UART_MSG_TYPE_MANAGE, SYSOBJ_UART_MANAGE_SUBTYPE_SET_LED,
     internal_handle_manage_set_led},
    /* Add future handlers here */
};

#define MSG_HANDLER_TABLE_SIZE                                                 \
  (sizeof(msg_handler_table) / sizeof(msg_handler_table[0]))

void sysobj_uart_dispatch_msg(const sysobj_uart_msg_t *msg) {
  if (msg == NULL) {
    return;
  }

  for (size_t i = 0; i < MSG_HANDLER_TABLE_SIZE; i++) {
    if (msg_handler_table[i].msg_type == msg->msg_type &&
        msg_handler_table[i].msg_subtype == msg->msg_subtype) {

      if (msg_handler_table[i].handler != NULL) {
        msg_handler_table[i].handler(msg);
        return; /* Message handled, exit loop */
      }
    }
  }
}

sysobj_uart_error_t sysobj_uart_generate(const sysobj_uart_msg_t *msg,
                                         uint8_t *buffer,
                                         uint16_t buffer_capacity,
                                         uint16_t *out_len) {
  if (msg == NULL || buffer == NULL || out_len == NULL) {
    return SYSOBJ_UART_ERROR_NULL_POINTER;
  }

  if (msg->data_len > SYSOBJ_UART_MAX_DATA_SIZE) {
    return SYSOBJ_UART_ERROR_PAYLOAD_TOO_LARGE;
  }

  uint16_t payload_size = SYSOBJ_UART_PAYLOAD_HEADER_SIZE + msg->data_len;
  uint16_t total_len = 3 + payload_size + 4;

  if (buffer_capacity < total_len) {
    return SYSOBJ_UART_ERROR_BUFFER_TOO_SMALL;
  }

  /* Wrapper Header */
  buffer[0] = SYSOBJ_UART_SOF;
  buffer[1] = (uint8_t)payload_size;
  buffer[2] = (uint8_t)(buffer[0] + buffer[1]);

  /* Payload Header */
  uint8_t *payload = &buffer[3];
  payload[0] = msg->src_id;
  payload[1] = msg->dst_id;
  payload[2] = ((msg->is_ack & 0x0F) << 4) | (msg->need_ack & 0x0F);
  payload[3] = msg->msg_type;
  payload[4] = msg->msg_subtype;

  /* Payload Data */
  if (msg->data_len > 0 && msg->data != NULL) {
    memcpy(&payload[5], msg->data, msg->data_len);
  }

  /* CRC32 Attachment */
  uint32_t crc = sysobj_uart_calculate_crc32(payload, payload_size);
  uint16_t crc_offset = 3 + payload_size;
  buffer[crc_offset] = (uint8_t)(crc & 0xFF);
  buffer[crc_offset + 1] = (uint8_t)((crc >> 8) & 0xFF);
  buffer[crc_offset + 2] = (uint8_t)((crc >> 16) & 0xFF);
  buffer[crc_offset + 3] = (uint8_t)((crc >> 24) & 0xFF);

  *out_len = total_len;

  return SYSOBJ_UART_ERROR_NONE;
}
