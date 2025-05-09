#include "fragmentation.h"
#include "frames.h"
#include <string.h>

#define INITIAL_BUFFER_SIZE 1024

int ws_fragment_init(ws_fragment_t *fragment) {
    if (!fragment) {
        return -1;
    }
    
    // Initialize fragmentation context
    fragment->in_progress = false;
    fragment->opcode = 0;
    fragment->data = NULL;
    fragment->data_length = 0;
    fragment->buffer_size = 0;
    
    return 0;
}

int ws_fragment_process(ws_fragment_t *fragment, uint8_t opcode, bool fin,
                       const uint8_t *data, size_t data_length) {
    if (!fragment) {
        return -1;
    }
    
    // Check if this is a new message or continuation
    if (!fragment->in_progress) {
        // This should be a non-continuation frame
        if (opcode == WS_OPCODE_CONTINUATION) {
            return -1; // Error: continuation without initial frame
        }
        
        // Start a new fragmented message
        fragment->in_progress = true;
        fragment->opcode = opcode;
        fragment->data_length = 0;
        
        // Allocate initial buffer if needed
        if (!fragment->data) {
            fragment->buffer_size = INITIAL_BUFFER_SIZE;
            fragment->data = (uint8_t*)malloc(fragment->buffer_size);
            
            if (!fragment->data) {
                fragment->in_progress = false;
                return -1; // Memory allocation error
            }
        }
    } else {
        // Continuing a fragmented message
        if (opcode != WS_OPCODE_CONTINUATION) {
            return -1; // Error: new message started before completing previous one
        }
    }
    
    // Ensure buffer can hold the new data
    size_t new_length = fragment->data_length + data_length;
    if (new_length > fragment->buffer_size) {
        // Resize buffer
        size_t new_size = fragment->buffer_size;
        while (new_size < new_length) {
            new_size *= 2;
        }
        
        uint8_t *new_buffer = (uint8_t*)realloc(fragment->data, new_size);
        if (!new_buffer) {
            return -1; // Memory allocation error
        }
        
        fragment->data = new_buffer;
        fragment->buffer_size = new_size;
    }
    
    // Append the new data
    memcpy(fragment->data + fragment->data_length, data, data_length);
    fragment->data_length += data_length;
    
    // Check if this is the final fragment
    if (fin) {
        fragment->in_progress = false;
        return 1; // Message is complete
    }
    
    return 0; // Still fragmenting
}

void ws_fragment_cleanup(ws_fragment_t *fragment) {
    if (fragment && fragment->data) {
        free(fragment->data);
        fragment->data = NULL;
        fragment->data_length = 0;
        fragment->buffer_size = 0;
        fragment->in_progress = false;
    }
}
