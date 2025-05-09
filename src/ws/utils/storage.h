#ifndef WS_STORAGE_H
#define WS_STORAGE_H

#include "../ws.h"

/**
 * Add a connection to the list
 *
 * @param head Pointer to head of connection list
 * @param connection Connection to add
 * @return 0 on success
 */
int ws_connection_add(ws_connection_t **head, ws_connection_t *connection);

/**
 * Remove a connection from the list
 *
 * @param head Pointer to head of connection list
 * @param connection Connection to remove
 * @return 0 on success, -1 if not found
 */
int ws_connection_remove(ws_connection_t **head, ws_connection_t *connection);

/**
 * Get connection count in the list
 *
 * @param head Head of connection list
 * @return Number of connections
 */
int ws_connection_count(ws_connection_t *head);

#endif /* WS_STORAGE_H */
