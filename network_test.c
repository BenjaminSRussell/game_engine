// network_test.c - Simple test for the networking system
#include "src/engine/include/network/network_manager.h"
#include "src/engine/include/network/rpc_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// Test RPC handlers
void handle_chat_message(uint32_t sender_id, void *data, uint32_t size) {
    char message[256];
    if (size < sizeof(message)) {
        memcpy(message, data, size);
        message[size] = '\0';
        printf("Chat from client %u: %s\n", sender_id, message);
    }
}

void handle_player_update(uint32_t sender_id, void *data, uint32_t size) {
    printf("Player update from client %u (%u bytes)\n", sender_id, size);
}

void* server_thread(void *arg) {
    printf("Starting server...\n");
    
    // Register RPC handlers
    rpc_register("chat_message", handle_chat_message);
    rpc_register("player_update", handle_player_update);
    
    // Start server
    if (network_server_start("Test Server", 8080, 8, NULL) != 0) {
        printf("Failed to start server\n");
        return NULL;
    }
    
    printf("Server running on port 8080\n");
    
    // Run server loop
    for (int i = 0; i < 100; i++) {
        network_update(0.016f); // 60 FPS
        usleep(16000); // 16ms
        
        // Broadcast test message every 2 seconds
        if (i % 120 == 0) {
            const char *test_msg = "Server broadcast!";
            rpc_broadcast("chat_message", (void*)test_msg, strlen(test_msg));
        }
    }
    
    network_server_stop();
    printf("Server stopped\n");
    return NULL;
}

void* client_thread(void *arg) {
    printf("Starting client...\n");
    
    // Register RPC handlers
    rpc_register("chat_message", handle_chat_message);
    rpc_register("player_update", handle_player_update);
    
    // Connect to server
    if (network_client_connect("127.0.0.1", 8080, "TestPlayer", NULL) != 0) {
        printf("Failed to connect to server\n");
        return NULL;
    }
    
    printf("Connected to server\n");
    
    // Wait for connection to establish
    sleep(1);
    
    // Run client loop
    for (int i = 0; i < 50; i++) {
        network_update(0.016f); // 60 FPS
        usleep(16000); // 16ms
        
        // Send test message every second
        if (i % 60 == 0 && network_client_is_connected()) {
            char test_msg[64];
            snprintf(test_msg, sizeof(test_msg), "Hello from client! %d", i / 60);
            rpc_send("chat_message", test_msg, strlen(test_msg));
        }
    }
    
    network_client_disconnect();
    printf("Client disconnected\n");
    return NULL;
}

int main() {
    printf("Network System Test\n");
    printf("===================\n");
    
    pthread_t server_tid, client_tid;
    
    // Start server
    if (pthread_create(&server_tid, NULL, server_thread, NULL) != 0) {
        printf("Failed to create server thread\n");
        return 1;
    }
    
    // Give server time to start
    sleep(1);
    
    // Start client
    if (pthread_create(&client_tid, NULL, client_thread, NULL) != 0) {
        printf("Failed to create client thread\n");
        return 1;
    }
    
    // Wait for threads to complete
    pthread_join(server_tid, NULL);
    pthread_join(client_tid, NULL);
    
    printf("Test completed\n");
    return 0;
}
