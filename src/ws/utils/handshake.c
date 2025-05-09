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
    
    // Read HTTP request
    bytes_read = recv(connection->socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        return -1;
    }
    
    buffer[bytes_read] = '\0';
    
    // Check if it's a valid WebSocket upgrade request
    if (strstr(buffer, "Upgrade: websocket") == NULL) {
        return -1;
    }
    
    // Extract WebSocket key
    char *key_start = strstr(buffer, "Sec-WebSocket-Key: ");
    if (key_start == NULL) {
        return -1;
    }
    
    key_start += 19; // Length of "Sec-WebSocket-Key: "
    char *key_end = strstr(key_start, "\r\n");
    
    if (key_end == NULL) {
        return -1;
    }
    
    size_t key_length = key_end - key_start;
    if (key_length > 255) {
        return -1;
    }
    
    strncpy(key, key_start, key_length);
    key[key_length] = '\0';
    
    // Generate accept key
    char accept_key[64];
    if (ws_generate_accept_key(key, accept_key) != 0) {
        return -1;
    }
    
    // Create handshake response
    int response_len = snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n",
        accept_key);
    
    // Send response
    if (send(connection->socket, response, response_len, 0) != response_len) {
        return -1;
    }
    
    return 0;
}
