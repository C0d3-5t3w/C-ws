#include "parse.h"
#include <string.h>

int ws_parse_frame(const uint8_t *data, size_t length, ws_frame_t *frame) {
    if (length < 2) {
        return -1; // Not enough data for a valid frame
    }
    
    // Parse first byte
    frame->fin = (data[0] & 0x80) != 0;
    frame->rsv1 = (data[0] & 0x40) != 0;
    frame->rsv2 = (data[0] & 0x20) != 0;
    frame->rsv3 = (data[0] & 0x10) != 0;
    frame->opcode = data[0] & 0x0F;
    
    // Parse second byte
    frame->mask = (data[1] & 0x80) != 0;
    uint8_t payload_len = data[1] & 0x7F;
    
    // Parse payload length
    size_t header_size = 2;
    if (payload_len == 126) {
        if (length < 4) {
            return -1; // Not enough data
        }
        frame->payload_length = ((uint16_t)data[2] << 8) | data[3];
        header_size = 4;
    } else if (payload_len == 127) {
        if (length < 10) {
            return -1; // Not enough data
        }
        frame->payload_length = 0;
        for (int i = 0; i < 8; i++) {
            frame->payload_length = (frame->payload_length << 8) | data[2 + i];
        }
        header_size = 10;
    } else {
        frame->payload_length = payload_len;
    }
    
    // Parse masking key
    if (frame->mask) {
        if (length < header_size + 4) {
            return -1; // Not enough data
        }
        memcpy(frame->mask_key, data + header_size, 4);
        header_size += 4;
    }
    
    // Check if we have the full payload
    if (length < header_size + frame->payload_length) {
        return -1; // Not enough data
    }
    
    // Get payload
    frame->payload = (uint8_t*)(data + header_size);
    
    // Unmask payload if needed
    if (frame->mask) {
        ws_unmask_payload((uint8_t*)frame->payload, frame->payload_length, frame->mask_key);
    }
    
    return 0;
}

void ws_unmask_payload(uint8_t *payload, size_t length, const uint8_t mask_key[4]) {
    for (size_t i = 0; i < length; i++) {
        payload[i] ^= mask_key[i % 4];
    }
}
