// include/engine/network_system.h
//
// Purpose: Complete networking system for multiplayer
//
#ifndef NETWORK_SYSTEM_H
#define NETWORK_SYSTEM_H

#include <common.h>
#include <math/vec3.h>

// Forward declarations
typedef struct NetworkSystem NetworkSystem;
typedef struct NetworkClient NetworkClient;
typedef struct NetworkServer NetworkServer;

// Network configuration
typedef struct {
    u16 port;
    u32 max_clients;
    u32 max_packet_size;
    f32 timeout_seconds;
    bool enable_reliability;
    bool enable_encryption;
    u32 send_rate; // packets per second
    u32 receive_rate;
} NetworkConfig;

// Network message types
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
    NET_MESSAGE_RPC
} NetworkMessageType;

// Network message
typedef struct {
    NetworkMessageType type;
    u32 size;
    u32 sequence;
    u32 client_id;
    u64 timestamp;
    void* data;
} NetworkMessage;

// Network statistics
typedef struct {
    u64 packets_sent;
    u64 packets_received;
    u64 bytes_sent;
    u64 bytes_received;
    u32 packets_lost;
    f32 latency_ms;
    f32 jitter_ms;
    f32 packet_loss_rate;
} NetworkStats;

// Client connection state
typedef enum {
    CLIENT_STATE_DISCONNECTED,
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_CONNECTED,
    CLIENT_STATE_DISCONNECTING
} ClientState;

// Network system interface
typedef struct NetworkSystem {
    // Lifecycle
    bool (*init)(NetworkSystem* network, const NetworkConfig* config);
    void (*shutdown)(NetworkSystem* network);
    void (*update)(NetworkSystem* network, f32 delta_time);
    
    // Server functions
    bool (*start_server)(NetworkSystem* network, u16 port);
    void (*stop_server)(NetworkSystem* network);
    bool (*is_server_running)(NetworkSystem* network);
    u32 (*get_client_count)(NetworkSystem* network);
    void (*kick_client)(NetworkSystem* network, u32 client_id);
    void (*broadcast_message)(NetworkSystem* network, const NetworkMessage* message);
    void (*send_to_client)(NetworkSystem* network, u32 client_id, const NetworkMessage* message);
    
    // Client functions
    bool (*connect_to_server)(NetworkSystem* network, const char* address, u16 port);
    void (*disconnect)(NetworkSystem* network);
    bool (*is_connected)(NetworkSystem* network);
    void (*send_message)(NetworkSystem* network, const NetworkMessage* message);
    
    // Message handling
    bool (*poll_message)(NetworkSystem* network, NetworkMessage* message);
    void (*set_message_handler)(NetworkSystem* network, NetworkMessageType type, void (*handler)(const NetworkMessage*, void*), void* user_data);
    
    // Statistics
    NetworkStats (*get_stats)(NetworkSystem* network);
    void (*reset_stats)(NetworkSystem* network);
    
    // Platform-specific data
    void* platform_data;
} NetworkSystem;

// Factory functions
NetworkSystem* create_network_system(void);

// Utility functions
NetworkConfig network_create_default_config(void);
bool network_validate_config(const NetworkConfig* config);

#endif // NETWORK_SYSTEM_H



