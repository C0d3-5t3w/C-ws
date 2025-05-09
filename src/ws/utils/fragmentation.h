#ifndef WS_FRAGMENTATION_H
#define WS_FRAGMENTATION_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * Structure to track fragmented message state
 */
typedef struct {
    bool in_progress;          // Whether fragmentation is in progress
    uint8_t opcode;            // Opcode of the fragmented message
    uint8_t *data;             // Buffer for fragmented data
    size_t data_length;        // Current data length
    size_t buffer_size;        // Allocated buffer size
} ws_fragment_t;

/**
 * Initialize a fragmentation context
 *
 * @param fragment Fragmentation context
 * @return 0 on success, -1 on error
 */
int ws_fragment_init(ws_fragment_t *fragment);

/**
 * Process a frame as part of fragmentation
 *
 * @param fragment Fragmentation context
 * @param opcode Frame opcode
 * @param fin FIN bit status
 * @param data Frame payload data
 * @param data_length Frame payload length
 * @return 0 if still fragmented, 1 if complete, -1 on error
 */
int ws_fragment_process(ws_fragment_t *fragment, uint8_t opcode, bool fin,
                       const uint8_t *data, size_t data_length);

/**
 * Clean up fragmentation context
 *
 * @param fragment Fragmentation context
 */
void ws_fragment_cleanup(ws_fragment_t *fragment);

#endif /* WS_FRAGMENTATION_H */
