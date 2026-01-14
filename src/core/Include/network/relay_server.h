#ifndef RELAY_SERVER_H
#define RELAY_SERVER_H

#include "../core/core.h"
#include "include/network/network_types.h"

// Relay protocol message types
typedef enum {
    RELAY_MSG_REGISTER = 0x01,
    RELAY_MSG_REGISTER_RESPONSE = 0x02,
    RELAY_MSG_CONNECT = 0x03,
    RELAY_MSG_CONNECT_RESPONSE = 0x04,
    RELAY_MSG_DATA = 0x05,
    RELAY_MSG_DISCONNECT = 0x06,
    RELAY_MSG_HEARTBEAT = 0x07,
    RELAY_MSG_PEER_INFO = 0x08,
    RELAY_MSG_ERROR = 0xFF
} RelayMessageType;

// Relay error codes
typedef enum {
    RELAY_ERROR_NONE = 0,
    RELAY_ERROR_INVALID_MESSAGE = 1,
    RELAY_ERROR_SESSION_NOT_FOUND = 2,
    RELAY_ERROR_PEER_NOT_FOUND = 3,
    RELAY_ERROR_SESSION_FULL = 4,
    RELAY_ERROR_AUTHENTICATION_FAILED = 5,
    RELAY_ERROR_RATE_LIMITED = 6,
    RELAY_ERROR_SERVER_OVERLOAD = 7
} RelayErrorCode;

// Relay session state
typedef enum {
    RELAY_SESSION_DISCONNECTED = 0,
    RELAY_SESSION_CONNECTING,
    RELAY_SESSION_CONNECTED,
    RELAY_SESSION_AUTHENTICATED,
    RELAY_SESSION_RELAYING
} RelaySessionState;

// Relay client information
typedef struct {
    uint32_t client_id;
    NetAddress address;
    char username[64];
    uint64_t connect_time;
    uint64_t last_activity;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    bool is_authenticated;
    RelaySessionState state;
} RelayClient;

// Relay session (between two peers)
typedef struct {
    uint32_t session_id;
    uint32_t client1_id;
    uint32_t client2_id;
    uint64_t created_time;
    uint64_t last_activity;
    uint32_t bytes_relayed;
    bool is_active;
    bool is_p2p_possible;
} RelaySession;

// Relay message header
typedef struct {
    uint8_t message_type;
    uint8_t flags;
    uint16_t session_id;
    uint32_t client_id;
    uint32_t sequence;
    uint32_t timestamp;
    uint16_t data_length;
} RelayMessageHeader;

// Relay server configuration
typedef struct {
    uint16_t port;
    uint32_t max_clients;
    uint32_t max_sessions;
    uint32_t heartbeat_interval_ms;
    uint32_t session_timeout_ms;
    uint32_t bandwidth_limit_bps;
    bool enable_authentication;
    bool enable_compression;
    char auth_token[128];
} RelayServerConfig;

// Relay server interface
typedef struct RelayServer {
    // Lifecycle
    bool (*init)(struct RelayServer* server, const RelayServerConfig* config);
    void (*shutdown)(struct RelayServer* server);
    bool (*start)(struct RelayServer* server);
    void (*stop)(struct RelayServer* server);
    void (*update)(struct RelayServer* server, float delta_time);
    
    // Client management
    uint32_t (*register_client)(struct RelayServer* server, const NetAddress* address, const char* username);
    bool (*unregister_client)(struct RelayServer* server, uint32_t client_id);
    RelayClient* (*get_client)(struct RelayServer* server, uint32_t client_id);
    bool (*authenticate_client)(struct RelayServer* server, uint32_t client_id, const char* token);
    
    // Session management
    uint32_t (*create_session)(struct RelayServer* server, uint32_t client1_id, uint32_t client2_id);
    bool (*destroy_session)(struct RelayServer* server, uint32_t session_id);
    RelaySession* (*get_session)(struct RelayServer* server, uint32_t session_id);
    bool (*connect_clients)(struct RelayServer* server, uint32_t client1_id, uint32_t client2_id);
    
    // Message handling
    bool (*send_message)(struct RelayServer* server, uint32_t client_id, const void* data, uint32_t length);
    bool (*relay_message)(struct RelayServer* server, uint32_t from_client_id, uint32_t to_client_id, 
                         const void* data, uint32_t length);
    bool (*broadcast_message)(struct RelayServer* server, uint32_t client_id, const void* data, uint32_t length);
    
    // Statistics
    uint32_t (*get_client_count)(struct RelayServer* server);
    uint32_t (*get_session_count)(struct RelayServer* server);
    uint32_t (*get_bytes_relayed)(struct RelayServer* server);
    float (*get_average_latency)(struct RelayServer* server);
    
    // Internal state
    RelayClient* clients;
    uint32_t client_count;
    uint32_t client_capacity;
    RelaySession* sessions;
    uint32_t session_count;
    uint32_t session_capacity;
    RelayServerConfig config;
    int server_socket;
    bool is_running;
    uint32_t next_client_id;
    uint32_t next_session_id;
    uint64_t last_update;
    uint64_t total_bytes_relayed;
} RelayServer;

// Relay client interface (for connecting to relay server)
typedef struct RelayClient {
    // Lifecycle
    bool (*init)(struct RelayClient* client, const NetAddress* server_address);
    void (*shutdown)(struct RelayClient* client);
    bool (*connect)(struct RelayClient* client, const char* username);
    void (*disconnect)(struct RelayClient* client);
    void (*update)(struct RelayClient* client, float delta_time);
    
    // Connection management
    bool (*is_connected)(struct RelayClient* client);
    bool (*is_authenticated)(struct RelayClient* client);
    bool (*send_heartbeat)(struct RelayClient* client);
    
    // Peer communication
    bool (*connect_to_peer)(struct RelayClient* client, uint32_t peer_id);
    bool (*send_to_peer)(struct RelayClient* client, uint32_t peer_id, const void* data, uint32_t length);
    bool (*receive_from_peer)(struct RelayClient* client, uint32_t* peer_id, void* data, uint32_t* length);
    
    // Statistics
    uint32_t (*get_client_id)(struct RelayClient* client);
    float (*get_latency)(struct RelayClient* client);
    uint32_t (*get_bytes_sent)(struct RelayClient* client);
    uint32_t (*get_bytes_received)(struct RelayClient* client);
    
    // Internal state
    NetAddress server_address;
    NetAddress local_address;
    int client_socket;
    uint32_t client_id;
    RelaySessionState state;
    uint64_t connect_time;
    uint64_t last_heartbeat;
    uint64_t last_activity;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    float latency_ms;
    bool is_connected;
    bool is_authenticated;
} RelayClient;

// Factory functions
RelayServer* relay_create_server(void);
void relay_destroy_server(RelayServer* server);

RelayClient* relay_create_client(void);
void relay_destroy_client(RelayClient* client);

// Utility functions
const char* relay_message_type_to_string(RelayMessageType type);
const char* relay_error_to_string(RelayErrorCode error);
const char* relay_session_state_to_string(RelaySessionState state);

bool relay_validate_message(const RelayMessageHeader* header, uint32_t data_length);
uint32_t relay_calculate_checksum(const void* data, uint32_t length);

#endif // RELAY_SERVER_H
