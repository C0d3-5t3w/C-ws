#include "handshake.h"
#include "helper.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <ctype.h>  // For case-insensitive string comparison

#define BUFFER_SIZE 4096
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

// Base64 encode a string
static char *base64_encode(const unsigned char *input, int length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;
    
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    char *result = (char *)malloc(bufferPtr->length + 1);
    if (result) {
        memcpy(result, bufferPtr->data, bufferPtr->length);
        result[bufferPtr->length] = 0;
    }
    
    BIO_free_all(bio);
    return result;
}

// Case insensitive string search
static char *strcasestr(const char *haystack, const char *needle) {
    size_t needle_len = strlen(needle);
    size_t haystack_len = strlen(haystack);
    
    if (needle_len > haystack_len) {
        return NULL;
    }
    
    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        if (strncasecmp(haystack + i, needle, needle_len) == 0) {
            return (char*)(haystack + i);
        }
    }
    
    return NULL;
}

// Find a header value in the HTTP request
static char *find_header(const char *request, const char *header) {
    char header_with_colon[256];
    snprintf(header_with_colon, sizeof(header_with_colon), "%s:", header);
    
    char *header_pos = strcasestr(request, header_with_colon);
    if (!header_pos) {
        return NULL;
    }
    
    // Skip the header name and colon
    header_pos += strlen(header_with_colon);
    
    // Skip leading spaces
    while (*header_pos == ' ') {
        header_pos++;
    }
    
    return header_pos;
}

// Extract a header value from the HTTP request
static int extract_header_value(const char *request, const char *header, char *value, size_t value_size) {
    char *header_pos = find_header(request, header);
    if (!header_pos) {
        return -1;
    }
    
    // Find the end of the header value (CR or LF)
    char *end = strpbrk(header_pos, "\r\n");
    if (!end) {
        return -1;
    }
    
    size_t len = end - header_pos;
    if (len >= value_size) {
        len = value_size - 1;
    }
    
    strncpy(value, header_pos, len);
    value[len] = '\0';
    
    return 0;
}

int ws_generate_accept_key(const char *client_key, char *accept_key) {
    // Combine client key with WebSocket GUID
    char combined[128];
    int len = snprintf(combined, sizeof(combined), "%s%s", client_key, WS_GUID);
    
    if (len < 0 || len >= (int)sizeof(combined)) {
        return -1;
    }
    
    // Calculate SHA-1 hash
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)combined, len, hash);
    
    // Base64 encode the hash
    char *base64_result = base64_encode(hash, SHA_DIGEST_LENGTH);
    
    if (!base64_result) {
        return -1;
    }
    
    // Copy to output
    strcpy(accept_key, base64_result);
    free(base64_result);
    
    return 0;
}

int ws_handshake(ws_connection_t *connection) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char key[256] = {0};
    ssize_t bytes_read;
    int total_bytes = 0;
    bool headers_complete = false;
    
    // Set a reasonable timeout for the handshake
    int timeout = 5000; // 5 seconds
    
    // Keep reading until we find the end of the HTTP headers or timeout
    while (!headers_complete && total_bytes < (BUFFER_SIZE - 1)) {
        // Wait for data with a timeout
        int ready = ws_wait_for_read(connection->socket, timeout);
        if (ready <= 0) {
            fprintf(stderr, "Handshake timeout or error for %s:%d\n", 
                   connection->host, connection->port);
            return -1;
        }
        
        bytes_read = recv(connection->socket, buffer + total_bytes, 
                         BUFFER_SIZE - total_bytes - 1, 0);
                         
        if (bytes_read <= 0) {
            fprintf(stderr, "Failed to read handshake data from %s:%d\n", 
                   connection->host, connection->port);
            return -1;
        }
        
        total_bytes += bytes_read;
        buffer[total_bytes] = '\0';
        
        // Check if we've received the end of the headers
        if (strstr(buffer, "\r\n\r\n") != NULL) {
            headers_complete = true;
        }
    }
    
    if (!headers_complete) {
        fprintf(stderr, "Incomplete HTTP headers from %s:%d\n", 
               connection->host, connection->port);
        return -1;
    }
    
    // Debug - print the received headers
    printf("Received HTTP request from %s:%d (%d bytes):\n%s\n", 
           connection->host, connection->port, total_bytes, buffer);
    
    // Verify this is a WebSocket upgrade request
    if (!strcasestr(buffer, "Upgrade: websocket") || 
        !strcasestr(buffer, "Connection: Upgrade")) {
        fprintf(stderr, "Not a valid WebSocket upgrade request from %s:%d\n", 
               connection->host, connection->port);
        return -1;
    }
    
    // Extract WebSocket key
    if (extract_header_value(buffer, "Sec-WebSocket-Key", key, sizeof(key)) != 0) {
        fprintf(stderr, "Missing or invalid Sec-WebSocket-Key from %s:%d\n", 
               connection->host, connection->port);
        return -1;
    }
    
    printf("Extracted WebSocket key: '%s'\n", key);
    
    // Generate accept key
    char accept_key[64];
    if (ws_generate_accept_key(key, accept_key) != 0) {
        fprintf(stderr, "Failed to generate accept key for %s:%d\n", 
               connection->host, connection->port);
        return -1;
    }
    
    printf("Generated accept key: '%s'\n", accept_key);
    
    // Create handshake response
    int response_len = snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n",
        accept_key);
    
    // Debug - print the response
    printf("Sending handshake response to %s:%d:\n%s\n", 
           connection->host, connection->port, response);
    
    // Send response
    if (send(connection->socket, response, response_len, 0) != response_len) {
        fprintf(stderr, "Failed to send handshake response to %s:%d\n", 
               connection->host, connection->port);
        return -1;
    }
    
    printf("Handshake successful with %s:%d\n", connection->host, connection->port);
    return 0;
}
