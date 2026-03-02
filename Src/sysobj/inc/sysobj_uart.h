/**
 * @file sysobj_uart.h
 * @brief UART messaging module for sysobj
 */

#ifndef SYSOBJ_UART_H
#define SYSOBJ_UART_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYSOBJ_UART_SOF 0xAA

/* The protocol header is 5 bytes: src_id, dst_id, ack_info, msg_type,
 * msg_subtype */
#define SYSOBJ_UART_PAYLOAD_HEADER_SIZE 5

/* The maximum payload size is represented by a 1-byte field (255) */
#define SYSOBJ_UART_MAX_PAYLOAD_SIZE 255

/* Max data size is max payload size minus the payload header */
#define SYSOBJ_UART_MAX_DATA_SIZE                                              \
  (SYSOBJ_UART_MAX_PAYLOAD_SIZE - SYSOBJ_UART_PAYLOAD_HEADER_SIZE)

typedef enum {
  SYSOBJ_UART_MSG_TYPE_MANAGE = 0x00,
  SYSOBJ_UART_MSG_TYPE_CONFIG = 0x01,
  SYSOBJ_UART_MSG_TYPE_TEST = 0x02,
  SYSOBJ_UART_MSG_TYPE_CRITICAL = 0x03
} sysobj_uart_msg_type_t;

typedef enum {
  SYSOBJ_UART_MANAGE_SUBTYPE_SET_LED = 0x01,
} sysobj_uart_manage_subtype_t;

typedef enum {
  SYSOBJ_UART_ERROR_NONE = 0,
  SYSOBJ_UART_ERROR_NULL_POINTER,
  SYSOBJ_UART_ERROR_BUFFER_TOO_SMALL,
  SYSOBJ_UART_ERROR_INVALID_SOF,
  SYSOBJ_UART_ERROR_INVALID_HEADER_CHECKSUM,
  SYSOBJ_UART_ERROR_INVALID_FRAME_LENGTH,
  SYSOBJ_UART_ERROR_INVALID_CRC32,
  SYSOBJ_UART_ERROR_PAYLOAD_TOO_LARGE
} sysobj_uart_error_t;

/**
 * @brief Structured representation of the UART message.
 */
typedef struct {
  uint8_t src_id;
  uint8_t dst_id;
  uint8_t is_ack;   /* 4 bits */
  uint8_t need_ack; /* 4 bits */
  uint8_t msg_type; /* sysobj_uart_msg_type_t */
  uint8_t msg_subtype;
  const uint8_t *data;
  uint8_t data_len;
} sysobj_uart_msg_t;

/**
 * @brief Parses a raw byte buffer into a UART message structure.
 *
 * @param buffer Input raw byte buffer.
 * @param len Length of the buffer in bytes.
 * @param msg Output message structure. `msg->data` will point inside `buffer`.
 * @return SYSOBJ_UART_ERROR_NONE on success, or an error code.
 */
sysobj_uart_error_t sysobj_uart_parse(const uint8_t *buffer, uint16_t len,
                                      sysobj_uart_msg_t *msg);

/**
 * @brief Generates a raw byte buffer from a UART message structure.
 *
 * @param msg Input message structure.
 * @param buffer Output raw byte buffer. Must be at least 12 + msg->data_len
 * bytes long.
 * @param buffer_capacity Maximum capacity of the output buffer.
 * @param out_len Actual length of the generated frame.
 * @return SYSOBJ_UART_ERROR_NONE on success, or an error code.
 */
sysobj_uart_error_t sysobj_uart_generate(const sysobj_uart_msg_t *msg,
                                         uint8_t *buffer,
                                         uint16_t buffer_capacity,
                                         uint16_t *out_len);

/**
 * @brief Calculates CRC32 for a given buffer.
 * Internally uses IEEE 802.3 CRC32 polynomial (0xEDB88320) logic.
 *
 * @param data Array of bytes.
 * @param len Length of data.
 * @return CRC32 value.
 */
uint32_t sysobj_uart_calculate_crc32(const uint8_t *data, uint16_t len);

/**
 * @brief Dispatch a parsed message to the appropriate handler using a lookup
 * table.
 *
 * @param msg The parsed message structure.
 */
void sysobj_uart_dispatch_msg(const sysobj_uart_msg_t *msg);

/**
 * @brief Handler for MANAGE -> SET_LED message.
 * Expected payload: <LED_ID, 1 byte> <STATE, 1 byte>.
 * This is defined as a weak function in sysobj_uart.c and should be overridden
 * by the application.
 *
 * @param led_id ID of the LED to set.
 * @param state State to set the LED to (e.g., 0=off, 1=on).
 */
void sysobj_uart_handle_manage_set_led(uint8_t led_id, uint8_t state);

#ifdef __cplusplus
}
#endif

#endif /* SYSOBJ_UART_H */
