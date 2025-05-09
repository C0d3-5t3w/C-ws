#ifndef WS_PARSE_H
#define WS_PARSE_H

#include "frames.h"

/**
 * Parse a WebSocket frame from raw data
 *
 * @param data Raw frame data
 * @param length Length of data
 * @param frame Output frame structure
 * @return 0 on success, -1 on error
 */
int ws_parse_frame(const uint8_t *data, size_t length, ws_frame_t *frame);

/**
 * Unmask WebSocket payload data
 *
 * @param payload Masked payload data
 * @param length Length of payload
 * @param mask_key 4-byte masking key
 */
void ws_unmask_payload(uint8_t *payload, size_t length, const uint8_t mask_key[4]);

#endif /* WS_PARSE_H */
