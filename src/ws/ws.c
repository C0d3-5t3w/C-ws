#include "ws.h"
#include "utils/handshake.h"
#include "utils/frames.h"
#include "utils/fragmentation.h"
#include "utils/parse.h"
#include "utils/helper.h"
#include "utils/storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define MAX_CLIENTS 64
#define BUFFER_SIZE 8192

static void ws_accept_client(ws_server_t *server);
static void ws_process_client(ws_server_t *server, ws_connection_t *client);
static void ws_disconnect_client(ws_server_t *server, ws_connection_t *client, int code, const char *reason);

int ws_server_init(ws_server_t *server, int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        return -1;
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(server_fd);
        return -1;
    }
    
    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }
    
    // Listen for connections
    if (listen(server_fd, 10) == -1) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }
    
    // Set non-blocking
    if (ws_set_nonblocking(server_fd) == -1) {
        perror("failed to set non-blocking");
        close(server_fd);
        return -1;
    }
    
    // Initialize server structure
    server->socket = server_fd;
    server->clients = NULL;
    
    // Initialize default callbacks to prevent null pointer dereferences
    server->on_connect = NULL;
    server->on_message = NULL;
    server->on_close = NULL;
    server->on_error = NULL;
    
    printf("WebSocket server started on port %d\n", port);
    return 0;
}

int ws_server_run(ws_server_t *server) {
    while (1) {
        if (ws_server_step(server, -1) < 0) {
            return -1;
        }
    }
    
    return 0;
}

int ws_server_step(ws_server_t *server, int timeout_ms) {
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 0;
    
    // Add server socket to poll set
    fds[nfds].fd = server->socket;
    fds[nfds].events = POLLIN;
    nfds++;
    
    // Add client sockets to poll set
    ws_connection_t *client = server->clients;
    while (client != NULL && nfds < MAX_CLIENTS + 1) {
        fds[nfds].fd = client->socket;
        fds[nfds].events = POLLIN;
        nfds++;
        client = client->next;
    }
    
    // Wait for activity on any socket
    int activity = poll(fds, nfds, timeout_ms);
    
    if (activity < 0) {
        perror("poll failed");
        return -1;
    }
    
    // Check for activity on server socket (new connection)
    if (fds[0].revents & POLLIN) {
        ws_accept_client(server);
    }
    
    // Check for activity on client sockets
    client = server->clients;
    int i = 1;
    while (client != NULL && i < nfds) {
        ws_connection_t *next = client->next;
        
        if (fds[i].revents & POLLIN) {
            ws_process_client(server, client);
        }
        
        client = next;
        i++;
    }
    
    return 0;
}

static void ws_accept_client(ws_server_t *server) {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    
    int client_fd = accept(server->socket, (struct sockaddr *)&client_addr, &addrlen);
    
    if (client_fd < 0) {
        perror("accept failed");
        return;
    }
    
    // Set non-blocking
    if (ws_set_nonblocking(client_fd) < 0) {
        perror("set non-blocking failed");
        close(client_fd);
        return;
    }
    
    // Create new client connection
    ws_connection_t *conn = (ws_connection_t *)malloc(sizeof(ws_connection_t));
    if (!conn) {
        perror("malloc failed");
        close(client_fd);
        return;
    }
    
    // Initialize connection
    conn->socket = client_fd;
    conn->state = WS_STATE_CONNECTING;
    conn->host = strdup(inet_ntoa(client_addr.sin_addr));
    conn->port = ntohs(client_addr.sin_port);
    conn->user_data = NULL;
    
    // Add to connection list
    ws_connection_add(&server->clients, conn);
    
    // Perform WebSocket handshake
    if (ws_handshake(conn) != 0) {
        if (server->on_error) {
            server->on_error(conn, "Handshake failed");
        }
        ws_disconnect_client(server, conn, 1002, "Protocol error");
        return;
    }
    
    conn->state = WS_STATE_OPEN;
    
    // Call the on_connect callback
    if (server->on_connect) {
        server->on_connect(conn);
    }
}

static void ws_process_client(ws_server_t *server, ws_connection_t *client) {
    uint8_t buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client->socket, buffer, sizeof(buffer), 0);
    
    if (bytes_read <= 0) {
        // Connection closed or error
        if (bytes_read == 0) {
            ws_disconnect_client(server, client, 1000, "Connection closed");
        } else {
            if (server->on_error) {
                server->on_error(client, "Read error");
            }
            ws_disconnect_client(server, client, 1001, "Read error");
        }
        return;
    }
    
    // Parse WebSocket frame
    ws_frame_t frame;
    if (ws_parse_frame(buffer, bytes_read, &frame) != 0) {
        if (server->on_error) {
            server->on_error(client, "Invalid frame");
        }
        ws_disconnect_client(server, client, 1002, "Protocol error");
        return;
    }
    
    // Handle different frame types
    switch (frame.opcode) {
        case WS_OPCODE_TEXT:
        case WS_OPCODE_BINARY:
            if (server->on_message) {
                server->on_message(client, frame.payload, frame.payload_length, 
                                 frame.opcode == WS_OPCODE_BINARY);
            }
            break;
            
        case WS_OPCODE_CLOSE:
            {
                uint16_t code = 1000;
                char reason[124] = "";
                
                // Extract close code and reason if available
                if (frame.payload_length >= 2) {
                    code = (frame.payload[0] << 8) | frame.payload[1];
                    
                    if (frame.payload_length > 2) {
                        size_t reason_len = frame.payload_length - 2 < 123 ? 
                                          frame.payload_length - 2 : 123;
                        memcpy(reason, &frame.payload[2], reason_len);
                        reason[reason_len] = '\0';
                    }
                }
                
                ws_disconnect_client(server, client, code, reason);
            }
            break;
            
        case WS_OPCODE_PING:
            // Respond with a pong frame
            ws_send_pong(client, frame.payload, frame.payload_length);
            break;
            
        case WS_OPCODE_PONG:
            // Pong received, could update last activity time
            break;
            
        default:
            if (server->on_error) {
                server->on_error(client, "Unknown opcode");
            }
            break;
    }
}

int ws_send_text(ws_connection_t *connection, const char *text, size_t len) {
    return ws_send_frame(connection, WS_OPCODE_TEXT, (const uint8_t *)text, len);
}

int ws_send_binary(ws_connection_t *connection, const uint8_t *data, size_t len) {
    return ws_send_frame(connection, WS_OPCODE_BINARY, data, len);
}

int ws_close(ws_connection_t *connection, int code, const char *reason) {
    uint8_t payload[128];
    size_t payload_len = 2; // At least the status code
    
    // Set status code
    payload[0] = (code >> 8) & 0xFF;
    payload[1] = code & 0xFF;
    
    // Add reason if provided
    if (reason) {
        size_t reason_len = strlen(reason);
        if (reason_len > 123) reason_len = 123; // Max allowed reason length
        
        memcpy(&payload[2], reason, reason_len);
        payload_len += reason_len;
    }
    
    return ws_send_frame(connection, WS_OPCODE_CLOSE, payload, payload_len);
}

static void ws_disconnect_client(ws_server_t *server, ws_connection_t *client, int code, const char *reason) {
    // Send close frame if connection is still open
    if (client->state == WS_STATE_OPEN) {
        ws_close(client, code, reason);
        client->state = WS_STATE_CLOSING;
    }
    
    // Call the on_close callback
    if (server->on_close) {
        server->on_close(client, code, reason);
    }
    
    // Remove from connection list and free resources
    ws_connection_remove(&server->clients, client);
    
    close(client->socket);
    if (client->host) free(client->host);
    free(client);
}

void ws_server_cleanup(ws_server_t *server) {
    // Close all client connections
    ws_connection_t *client = server->clients;
    while (client) {
        ws_connection_t *next = client->next;
        ws_disconnect_client(server, client, 1001, "Server shutting down");
        client = next;
    }
    
    // Close server socket
    if (server->socket >= 0) {
        close(server->socket);
        server->socket = -1;
    }
}
