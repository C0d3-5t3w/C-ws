#include "storage.h"

int ws_connection_add(ws_connection_t **head, ws_connection_t *connection) {
    if (!head || !connection) {
        return -1;
    }
    
    // Add connection to the head of the list
    connection->next = *head;
    *head = connection;
    
    return 0;
}

int ws_connection_remove(ws_connection_t **head, ws_connection_t *connection) {
    if (!head || !*head || !connection) {
        return -1;
    }
    
    // Check if connection is the head
    if (*head == connection) {
        *head = connection->next;
        return 0;
    }
    
    // Search for connection in the list
    ws_connection_t *current = *head;
    while (current->next && current->next != connection) {
        current = current->next;
    }
    
    if (current->next == connection) {
        current->next = connection->next;
        return 0;
    }
    
    return -1; // Connection not found
}

int ws_connection_count(ws_connection_t *head) {
    int count = 0;
    ws_connection_t *current = head;
    
    while (current) {
        count++;
        current = current->next;
    }
    
    return count;
}
