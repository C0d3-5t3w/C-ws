#ifndef WS_FRAMES_H
#define WS_FRAMES_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../ws.h"

/**
 * WebSocket frame opcodes
 */
#define WS_OPCODE_CONTINUATION 0x0
#define WS_OPCODE_TEXT         0x1
#define WS_OPCODE_BINARY       0x2
#define WS_OPCODE_CLOSE        0x8
#define WS_OPCODE_PING         0x9
#define WS_OPCODE_PONG         0xA

/**
 * WebSocket frame structure
 */
typedef struct {
    bool fin;                  // FIN bit
    bool rsv1;                 // RSV1 bit
    bool rsv2;                 // RSV2 bit
    bool rsv3;                 // RSV3 bit
    uint8_t opcode;            // Opcode
    bool mask;                 // MASK bit
    uint8_t mask_key[4];       // Masking key (if mask bit is set)
    uint64_t payload_length;   // Payload length
    uint8_t *payload;          // Payload data
} ws_frame_t;

/**
 * Create a WebSocket frame
 *
 * @param opcode Frame opcode
 * @param payload Payload data
 * @param payload_length Payload length
 * @param buffer Output buffer for the frame
 * @param buffer_size Size of the output buffer
 * @param use_mask Whether to mask the payload
 * @return Size of the frame, or -1 if buffer is too small
 */
int ws_create_frame(uint8_t opcode, const uint8_t *payload, uint64_t payload_length,
                   uint8_t *buffer, size_t buffer_size, bool use_mask);

/**
 * Send a WebSocket frame to a client
 *
 * @param connection Client connection
 * @param opcode Frame opcode
 * @param payload Payload data
 * @param payload_length Payload length
 * @return Number of bytes sent, or -1 on error
 */
int ws_send_frame(ws_connection_t *connection, uint8_t opcode,
                 const uint8_t *payload, size_t payload_length);

/**
 * Send a ping frame to a client
 *
 * @param connection Client connection
 * @param payload Optional payload data
 * @param payload_length Payload length
 * @return Number of bytes sent, or -1 on error
 */
int ws_send_ping(ws_connection_t *connection, const uint8_t *payload, size_t payload_length);

/**
 * Send a pong frame to a client
 *
 * @param connection Client connection
 * @param payload Optional payload data
 * @param payload_length Payload length
 * @return Number of bytes sent, or -1 on error
 */
int ws_send_pong(ws_connection_t *connection, const uint8_t *payload, size_t payload_length);

#endif /* WS_FRAMES_H */
