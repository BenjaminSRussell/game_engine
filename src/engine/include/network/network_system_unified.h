// include/network/network_system_unified.h
//
// Purpose: Unified networking system consolidating all network subsystems
// This replaces multiple disparate networking APIs with a single interface

#ifndef NETWORK_SYSTEM_UNIFIED_H
#define NETWORK_SYSTEM_UNIFIED_H

#include "common.h"
#include "math/vec3.h"
#include "ecs/ecs.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// UNIFIED NETWORK CONFIGURATION
// ============================================================================

typedef struct {
    uint16_t port;
    uint32_t max_clients;
    uint32_t max_packet_size;
    float timeout_seconds;
    bool enable_reliability;
    bool enable_encryption;
    uint32_t send_rate; // packets per second
    uint32_t receive_rate;
    bool enable_compression;
    bool enable_delta_compression;
} NetworkConfig;

// ============================================================================
// UNIFIED NETWORK TYPES
// ============================================================================

typedef enum {
    NET_MESSAGE_CONNECT,
    NET_MESSAGE_DISCONNECT,
    NET_MESSAGE_DATA,
    NET_MESSAGE_ACK,
    NET_MESSAGE_HEARTBEAT,
    NET_MESSAGE_ENTITY_UPDATE,
    NET_MESSAGE_ENTITY_SPAWN,
    NET_MESSAGE_ENTITY_DESTROY,
    NET_MESSAGE_CHAT,
    NET_MESSAGE_RPC,
    NET_MESSAGE_STATE_SYNC,
    NET_MESSAGE_INPUT,
    NET_MESSAGE_WORLD_UPDATE
} NetworkMessageType;

typedef enum {
    CLIENT_STATE_DISCONNECTED,
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_CONNECTED,
    CLIENT_STATE_DISCONNECTING
} ClientState;

typedef struct {
    NetworkMessageType type;
    uint32_t size;
    uint32_t sequence;
    uint32_t client_id;
    uint64_t timestamp;
    void* data;
} NetworkMessage;

typedef struct {
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t packets_lost;
    float latency_ms;
    float jitter_ms;
    float packet_loss_rate;
    uint32_t entities_replicated;
    uint32_t bandwidth_usage_bps;
} NetworkStats;

// ============================================================================
// ENTITY REPLICATION SYSTEM
// ============================================================================

typedef struct {
    Entity entity;
    uint32_t network_id;
    Vec3 position;
    Vec3 velocity;
    Vec3 rotation;
    bool is_dirty;
    uint32_t priority;
    float last_update_time;
} NetworkedEntity;

typedef struct {
    NetworkedEntity* entities;
    uint32_t max_entities;
    uint32_t active_entities;
    uint32_t next_network_id;
    bool enable_delta_compression;
    float update_rate;
    float relevancy_distance;
} EntityReplicationSystem;

// ============================================================================
// CLIENT PREDICTION SYSTEM
// ============================================================================

typedef struct {
    Entity entity;
    Vec3 predicted_position;
    Vec3 predicted_velocity;
    Vec3 server_position;
    Vec3 server_velocity;
    float prediction_error;
    bool is_correcting;
    uint32_t last_input_sequence;
} ClientPrediction;

typedef struct {
    ClientPrediction* predictions;
    uint32_t max_predictions;
    uint32_t active_predictions;
    float smoothing_factor;
    float max_correction_speed;
    bool enable_interpolation;
} ClientPredictionSystem;

// ============================================================================
// UNIFIED NETWORK SYSTEM
// ============================================================================

typedef struct NetworkSystem {
    // Configuration
    NetworkConfig config;
    ClientState client_state;
    bool is_server;
    bool is_initialized;
    
    // Connection management
    int socket_fd;
    uint32_t client_id;
    char server_address[64];
    
    // Entity replication
    EntityReplicationSystem replication;
    
    // Client prediction
    ClientPredictionSystem prediction;
    
    // Message handling
    NetworkMessage* message_queue;
    uint32_t queue_size;
    uint32_t queue_capacity;
    
    // Platform-specific data
    void* platform_data;
    
    // Statistics
    NetworkStats stats;
} NetworkSystem;

// ============================================================================
// MAIN NETWORK API
// ============================================================================

// System management
NetworkSystem* network_system_create(const NetworkConfig* config);
void network_system_destroy(NetworkSystem* network);
bool network_system_is_initialized(NetworkSystem* network);
void network_system_update(NetworkSystem* network, float delta_time);

// Server functions
bool network_start_server(NetworkSystem* network, uint16_t port);
void network_stop_server(NetworkSystem* network);
bool network_is_server_running(NetworkSystem* network);
uint32_t network_get_client_count(NetworkSystem* network);
void network_kick_client(NetworkSystem* network, uint32_t client_id);
void network_broadcast_message(NetworkSystem* network, const NetworkMessage* message);
void network_send_to_client(NetworkSystem* network, uint32_t client_id, const NetworkMessage* message);

// Client functions
bool network_connect_to_server(NetworkSystem* network, const char* address, uint16_t port);
void network_disconnect(NetworkSystem* network);
bool network_is_connected(NetworkSystem* network);
void network_send_message(NetworkSystem* network, const NetworkMessage* message);

// ============================================================================
// ENTITY REPLICATION API
// ============================================================================

// Replication management
void network_replication_init(NetworkSystem* network, uint32_t max_entities);
void network_replication_shutdown(NetworkSystem* network);
uint32_t network_replication_add_entity(NetworkSystem* network, Entity entity, uint32_t priority);
void network_replication_remove_entity(NetworkSystem* network, Entity entity);
void network_replication_update_entity(NetworkSystem* network, Entity entity, Vec3 position, Vec3 velocity);

// Delta compression
void network_replication_enable_delta_compression(NetworkSystem* network, bool enable);
void network_replication_set_update_rate(NetworkSystem* network, float rate);
void network_replication_set_relevancy_distance(NetworkSystem* network, float distance);

// ============================================================================
// CLIENT PREDICTION API
// ============================================================================

// Prediction management
void network_prediction_init(NetworkSystem* network, uint32_t max_predictions);
void network_prediction_shutdown(NetworkSystem* network);
uint32_t network_prediction_add_entity(NetworkSystem* network, Entity entity);
void network_prediction_remove_entity(NetworkSystem* network, Entity entity);
void network_prediction_update_entity(NetworkSystem* network, Entity entity, Vec3 position, Vec3 velocity);

// Correction and interpolation
void network_prediction_correct_entity(NetworkSystem* network, Entity entity, Vec3 server_position, Vec3 server_velocity);
void network_prediction_enable_interpolation(NetworkSystem* network, bool enable);
void network_prediction_set_smoothing(NetworkSystem* network, float factor);

// ============================================================================
// MESSAGE HANDLING API
// ============================================================================

// Message processing
bool network_poll_message(NetworkSystem* network, NetworkMessage* message);
void network_set_message_handler(NetworkSystem* network, NetworkMessageType type, 
                             void (*handler)(const NetworkMessage*, void*), void* user_data);

// Message creation
NetworkMessage* network_create_message(NetworkMessageType type, const void* data, uint32_t size);
void network_destroy_message(NetworkMessage* message);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Configuration
NetworkConfig network_create_default_config(void);
bool network_validate_config(const NetworkConfig* config);

// Statistics
NetworkStats network_get_stats(NetworkSystem* network);
void network_reset_stats(NetworkSystem* network);

// Utility functions
float network_calculate_latency(uint64_t send_time, uint64_t receive_time);
bool network_is_address_valid(const char* address);
uint16_t network_calculate_checksum(const void* data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // NETWORK_SYSTEM_UNIFIED_H
