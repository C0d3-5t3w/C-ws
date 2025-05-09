#include "frames.h"
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <time.h>

int ws_create_frame(uint8_t opcode, const uint8_t *payload, uint64_t payload_length,
                   uint8_t *buffer, size_t buffer_size, bool use_mask) {
    // Calculate frame size
    size_t frame_size = 2; // Base header size
    
    // Extended payload length bytes
    if (payload_length > 125 && payload_length <= 65535) {
        frame_size += 2;
    } else if (payload_length > 65535) {
        frame_size += 8;
    }
    
    // Masking key bytes
    if (use_mask) {
        frame_size += 4;
    }
    
    // Payload bytes
    frame_size += payload_length;
    
    // Check if buffer is large enough
    if (buffer_size < frame_size) {
        return -1;
    }
    
    // Create the frame header
    int idx = 0;
    
    // FIN bit + RSV bits + opcode
    buffer[idx++] = 0x80 | (opcode & 0x0F);
    
    // MASK bit + payload length
    if (payload_length <= 125) {
        buffer[idx++] = (use_mask ? 0x80 : 0) | (uint8_t)payload_length;
    } else if (payload_length <= 65535) {
        buffer[idx++] = (use_mask ? 0x80 : 0) | 126;
        buffer[idx++] = (payload_length >> 8) & 0xFF;
        buffer[idx++] = payload_length & 0xFF;
    } else {
        buffer[idx++] = (use_mask ? 0x80 : 0) | 127;
        buffer[idx++] = (payload_length >> 56) & 0xFF;
        buffer[idx++] = (payload_length >> 48) & 0xFF;
        buffer[idx++] = (payload_length >> 40) & 0xFF;
        buffer[idx++] = (payload_length >> 32) & 0xFF;
        buffer[idx++] = (payload_length >> 24) & 0xFF;
        buffer[idx++] = (payload_length >> 16) & 0xFF;
        buffer[idx++] = (payload_length >> 8) & 0xFF;
        buffer[idx++] = payload_length & 0xFF;
    }
    
    // Add masking key if needed
    if (use_mask) {
        // Generate random mask
        uint8_t mask[4];
        srand(time(NULL));
        for (int i = 0; i < 4; i++) {
            mask[i] = rand() & 0xFF;
            buffer[idx++] = mask[i];
        }
        
        // Copy and mask payload
        for (uint64_t i = 0; i < payload_length; i++) {
            buffer[idx++] = payload[i] ^ mask[i % 4];
        }
    } else {
        // Copy payload without masking
        memcpy(&buffer[idx], payload, payload_length);
        idx += payload_length;
    }
    
    return idx;
}

int ws_send_frame(ws_connection_t *connection, uint8_t opcode,
                 const uint8_t *payload, size_t payload_length) {
    // Create a buffer for the frame
    // Calculate max frame size: header (14) + payload
    size_t buffer_size = 14 + payload_length;
    uint8_t *buffer = (uint8_t *)malloc(buffer_size);
    
    if (!buffer) {
        return -1;
    }
    
    // Create frame
    int frame_size = ws_create_frame(opcode, payload, payload_length, buffer, buffer_size, false);
    
    if (frame_size < 0) {
        free(buffer);
        return -1;
    }
    
    // Send frame
    int bytes_sent = send(connection->socket, buffer, frame_size, 0);
    
    free(buffer);
    return bytes_sent;
}

int ws_send_ping(ws_connection_t *connection, const uint8_t *payload, size_t payload_length) {
    // Ping payload should be 125 bytes or fewer
    if (payload_length > 125) {
        return -1;
    }
    
    return ws_send_frame(connection, WS_OPCODE_PING, payload, payload_length);
}

int ws_send_pong(ws_connection_t *connection, const uint8_t *payload, size_t payload_length) {
    // Pong payload should be 125 bytes or fewer
    if (payload_length > 125) {
        return -1;
    }
    
    return ws_send_frame(connection, WS_OPCODE_PONG, payload, payload_length);
}
