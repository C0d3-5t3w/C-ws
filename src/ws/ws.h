#ifndef WS_H
#define WS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * WebSocket connection states
 */
typedef enum {
    WS_STATE_CONNECTING,
    WS_STATE_OPEN,
    WS_STATE_CLOSING,
    WS_STATE_CLOSED
} ws_state_t;

/**
 * WebSocket connection structure
 */
typedef struct ws_connection {
    int socket;                 // Client socket
    ws_state_t state;           // Connection state
    char *host;                 // Client host
    int port;                   // Client port
    void *user_data;            // User data associated with this connection
    struct ws_connection *next; // Next connection in list
} ws_connection_t;

/**
 * WebSocket server structure
 */
typedef struct {
    int socket;                 // Server socket
    ws_connection_t *clients;   // Linked list of clients
    
    // Callbacks
    void (*on_connect)(ws_connection_t *connection);
    void (*on_message)(ws_connection_t *connection, const uint8_t *data, size_t len, bool is_binary);
    void (*on_close)(ws_connection_t *connection, int code, const char *reason);
    void (*on_error)(ws_connection_t *connection, const char *error);
} ws_server_t;

/**
 * Initialize the WebSocket server
 * 
 * @param server Pointer to server structure
 * @param port Port to listen on
 * @return 0 on success, -1 on failure
 */
int ws_server_init(ws_server_t *server, int port);

/**
 * Run the WebSocket server (blocking)
 * 
 * @param server Pointer to server structure
 * @return 0 on success, -1 on failure
 */
int ws_server_run(ws_server_t *server);

/**
 * Run the WebSocket server single step (non-blocking)
 * 
 * @param server Pointer to server structure
 * @param timeout_ms Maximum time to wait in milliseconds, 0 for no waiting
 * @return 0 on success, -1 on failure
 */
int ws_server_step(ws_server_t *server, int timeout_ms);

/**
 * Send text message to a client
 * 
 * @param connection Client connection
 * @param text Text message to send
 * @param len Length of message
 * @return Number of bytes sent, or -1 on error
 */
int ws_send_text(ws_connection_t *connection, const char *text, size_t len);

/**
 * Send binary message to a client
 * 
 * @param connection Client connection
 * @param data Binary data to send
 * @param len Length of data
 * @return Number of bytes sent, or -1 on error
 */
int ws_send_binary(ws_connection_t *connection, const uint8_t *data, size_t len);

/**
 * Close a WebSocket connection
 * 
 * @param connection Client connection
 * @param code Status code
 * @param reason Reason string (can be NULL)
 * @return 0 on success, -1 on failure
 */
int ws_close(ws_connection_t *connection, int code, const char *reason);

/**
 * Clean up WebSocket server
 * 
 * @param server Pointer to server structure
 */
void ws_server_cleanup(ws_server_t *server);

#endif /* WS_H */
