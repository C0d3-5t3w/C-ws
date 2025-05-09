#include "config.h"

void ws_config_init(ws_config_t *config) {
    if (!config) {
        return;
    }
    
    // Set default values
    config->port = WS_DEFAULT_PORT;
    config->max_clients = WS_MAX_CLIENTS;
    config->max_frame_size = WS_MAX_FRAME_SIZE;
    config->buffer_size = WS_BUFFER_SIZE;
    config->ping_interval = WS_PING_INTERVAL;
    config->timeout = WS_TIMEOUT;
}
