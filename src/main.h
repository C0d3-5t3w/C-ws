#ifndef MAIN_H
#define MAIN_H

#include "ws/ws.h"

/**
 * WebSocket connection callback
 */
void on_connect(ws_connection_t *connection);

/**
 * WebSocket message callback
 */
void on_message(ws_connection_t *connection, const uint8_t *data, size_t len, bool is_binary);

/**
 * WebSocket close callback
 */
void on_close(ws_connection_t *connection, int code, const char *reason);

/**
 * WebSocket error callback
 */
void on_error(ws_connection_t *connection, const char *error);

#endif /* MAIN_H */
