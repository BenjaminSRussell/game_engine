#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "include/network/network_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <arpa/inet.h>

// Network constants
// MAX_PACKET_SIZE is defined in packet.h
#define MAX_CLIENTS 64
#define MAX_USERNAME_LENGTH 64
#define MAX_PASSWORD_LENGTH 64
#define MAX_SERVER_NAME_LENGTH 128

// Internal types used by network manager

// Client info structure
typedef struct {
    uint32_t client_id;
    char username[MAX_USERNAME_LENGTH];
} ClientInfo;

// Authentication response
typedef struct {
    bool success;
    uint32_t client_id;
    char message[256];
} AuthResponse;

// Chat message
typedef struct {
    uint32_t sender_id;
    char username[MAX_USERNAME_LENGTH];
    char message[512];
} ChatMessage;

// Player update
typedef struct {
    uint32_t player_id;
    float position[3];
    float rotation[3];
    float velocity[3];
    bool on_ground;
} PlayerUpdate;

// Entity spawn
typedef struct {
    uint32_t entity_id;
    uint32_t entity_type;
    float position[3];
    float rotation[3];
    float scale[3];
} EntitySpawn;

// Entity update
typedef struct {
    uint32_t entity_id;
    float position[3];
    float rotation[3];
    float velocity[3];
    bool active;
} EntityUpdate;

// Block change
typedef struct {
    int32_t x, y, z;
    uint32_t block_type;
    uint32_t metadata;
} BlockChange;

// Chunk data
typedef struct {
    int32_t chunk_x, chunk_z;
    uint32_t chunk_size;
    uint8_t* block_data;
    uint8_t* metadata_data;
} ChunkData;

// Server info
typedef struct {
    char server_name[MAX_SERVER_NAME_LENGTH];
    uint32_t max_players;
    uint32_t current_players;
    bool password_required;
    char motd[256];
} ServerInfo;

// Network statistics
typedef struct {
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t packets_lost;
    float latency_ms;
    float jitter_ms;
    float packet_loss_rate;
} NetworkStats;

// Server functions
int network_server_start(const char* server_name, uint16_t port, uint32_t max_players, const char* password);
int network_server_stop(void);
int network_server_broadcast(PacketType type, const void* data, size_t data_size);
int network_server_send_to_client(uint32_t client_id, PacketType type, const void* data, size_t data_size);
uint32_t network_server_get_client_count(void);
int network_server_get_clients(uint32_t* out_client_ids, size_t max_count);

// Client functions
int network_client_connect(const char* server_address, uint16_t port, const char* username, const char* password);
int network_client_disconnect(void);
int network_client_send(PacketType type, const void* data, size_t data_size);
bool network_client_is_connected(void);
uint32_t network_client_get_id(void);

// Network update
int network_update(float delta_time);

// Utility functions
const char* network_get_error_string(int error_code);
bool network_is_valid_address(const char* address);
bool network_is_valid_port(uint16_t port);
uint32_t network_get_local_ip(void);

#endif // NETWORK_MANAGER_H
