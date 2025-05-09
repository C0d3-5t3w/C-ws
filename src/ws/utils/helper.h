#ifndef WS_HELPER_H
#define WS_HELPER_H

/**
 * Set socket to non-blocking mode
 *
 * @param socket Socket file descriptor
 * @return 0 on success, -1 on error
 */
int ws_set_nonblocking(int socket);

/**
 * Wait for socket to be ready for reading
 *
 * @param socket Socket file descriptor
 * @param timeout_ms Timeout in milliseconds
 * @return 1 if socket is ready, 0 on timeout, -1 on error
 */
int ws_wait_for_read(int socket, int timeout_ms);

/**
 * Wait for socket to be ready for writing
 *
 * @param socket Socket file descriptor
 * @param timeout_ms Timeout in milliseconds
 * @return 1 if socket is ready, 0 on timeout, -1 on error
 */
int ws_wait_for_write(int socket, int timeout_ms);

#endif /* WS_HELPER_H */
