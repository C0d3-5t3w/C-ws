#include "main.h"
#include "ws/ws.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static ws_server_t server;
static int running = 1;

void signal_handler(int signal) {
    running = 0;
}

void on_connect(ws_connection_t *connection) {
    printf("Client connected: %s:%d\n", connection->host, connection->port);
}

void on_message(ws_connection_t *connection, const uint8_t *data, size_t len, bool is_binary) {
    printf("Received %s message (%zu bytes) from %s:%d\n", 
           is_binary ? "binary" : "text", len, connection->host, connection->port);
    
    // Echo the message back
    if (is_binary) {
        ws_send_binary(connection, data, len);
    } else {
        ws_send_text(connection, (const char *)data, len);
    }
}

void on_close(ws_connection_t *connection, int code, const char *reason) {
    printf("Client disconnected: %s:%d (code: %d, reason: %s)\n", 
           connection->host, connection->port, code, reason ? reason : "");
}

void on_error(ws_connection_t *connection, const char *error) {
    printf("Error on connection %s:%d: %s\n", 
           connection->host, connection->port, error);
}

int main(int argc, char *argv[]) {
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Default port
    int port = 8080;
    
    // Parse command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    // Initialize WebSocket server
    if (ws_server_init(&server, port) != 0) {
        fprintf(stderr, "Failed to initialize WebSocket server\n");
        return 1;
    }
    
    // Set callbacks
    server.on_connect = on_connect;
    server.on_message = on_message;
    server.on_close = on_close;
    server.on_error = on_error;
    
    printf("WebSocket server started on port %d\n", port);
    printf("Press Ctrl+C to exit\n");
    
    // Run the server in non-blocking mode
    while (running) {
        ws_server_step(&server, 100); // 100ms timeout
    }
    
    // Clean up
    ws_server_cleanup(&server);
    printf("WebSocket server stopped\n");
    
    return 0;
}
