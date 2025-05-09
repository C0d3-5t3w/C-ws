#ifndef WS_CONFIG_H
#define WS_CONFIG_H

// Default WebSocket configuration
#define WS_DEFAULT_PORT 8080
#define WS_MAX_CLIENTS 64
#define WS_MAX_FRAME_SIZE 65536
#define WS_BUFFER_SIZE 8192
#define WS_PING_INTERVAL 30000 // 30 seconds
#define WS_TIMEOUT 60000       // 60 seconds

// WebSocket server configuration structure
typedef struct {
    int port;                  // Port to listen on
    int max_clients;           // Maximum number of clients
    int max_frame_size;        // Maximum frame size
    int buffer_size;           // Buffer size for reading/writing
    int ping_interval;         // Ping interval in milliseconds
    int timeout;               // Connection timeout in milliseconds
} ws_config_t;

/**
 * Initialize configuration with default values
 *
 * @param config Configuration structure
 */
void ws_config_init(ws_config_t *config);

#endif /* WS_CONFIG_H */
