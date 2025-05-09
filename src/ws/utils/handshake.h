#ifndef WS_HANDSHAKE_H
#define WS_HANDSHAKE_H

#include "../ws.h"

/**
 * Perform WebSocket handshake with a client
 *
 * @param connection Client connection
 * @return 0 on success, -1 on error
 */
int ws_handshake(ws_connection_t *connection);

/**
 * Generate the WebSocket accept key
 *
 * @param client_key Client's WebSocket key
 * @param accept_key Buffer to store the accept key (should be at least 29 bytes)
 * @return 0 on success, -1 on error
 */
int ws_generate_accept_key(const char *client_key, char *accept_key);

#endif /* WS_HANDSHAKE_H */
