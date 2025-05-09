#include "helper.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

int ws_set_nonblocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    
    if (flags == -1) {
        return -1;
    }
    
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

int ws_wait_for_read(int socket, int timeout_ms) {
    struct pollfd pfd;
    
    pfd.fd = socket;
    pfd.events = POLLIN;
    pfd.revents = 0;
    
    int result = poll(&pfd, 1, timeout_ms);
    
    if (result < 0) {
        return -1; // Error
    } else if (result == 0) {
        return 0;  // Timeout
    } else {
        if (pfd.revents & POLLIN) {
            return 1; // Ready to read
        } else {
            return -1; // Error condition
        }
    }
}

int ws_wait_for_write(int socket, int timeout_ms) {
    struct pollfd pfd;
    
    pfd.fd = socket;
    pfd.events = POLLOUT;
    pfd.revents = 0;
    
    int result = poll(&pfd, 1, timeout_ms);
    
    if (result < 0) {
        return -1; // Error
    } else if (result == 0) {
        return 0;  // Timeout
    } else {
        if (pfd.revents & POLLOUT) {
            return 1; // Ready to write
        } else {
            return -1; // Error condition
        }
    }
}
