#ifndef NAT_TRAVERSAL_H
#define NAT_TRAVERSAL_H

#include "../core/core.h"
#include "include/network/network_types.h"

// NAT types
typedef enum {
    NAT_TYPE_UNKNOWN = 0,
    NAT_TYPE_OPEN,           // No NAT, direct connection
    NAT_TYPE_FULL_CONE,      // Full cone NAT
    NAT_TYPE_RESTRICTED,     // Restricted cone NAT
    NAT_TYPE_PORT_RESTRICTED, // Port restricted cone NAT
    NAT_TYPE_SYMMETRIC,      // Symmetric NAT
    NAT_TYPE_BLOCKED         // UDP blocked
} NatType;

// STUN message types
typedef enum {
    STUN_BINDING_REQUEST = 0x0001,
    STUN_BINDING_RESPONSE = 0x0101,
    STUN_BINDING_ERROR = 0x0111,
    STUN_CHANGE_REQUEST = 0x0003,
    STUN_ALLOCATE_REQUEST = 0x0003,
    STUN_ALLOCATE_RESPONSE = 0x0103
} StunMessageType;

// STUN attribute types
typedef enum {
    STUN_ATTR_MAPPED_ADDRESS = 0x0001,
    STUN_ATTR_RESPONSE_ADDRESS = 0x0002,
    STUN_ATTR_CHANGE_REQUEST = 0x0003,
    STUN_ATTR_SOURCE_ADDRESS = 0x0004,
    STUN_ATTR_CHANGED_ADDRESS = 0x0005,
    STUN_ATTR_USERNAME = 0x0006,
    STUN_ATTR_MESSAGE_INTEGRITY = 0x0008,
    STUN_ATTR_ERROR_CODE = 0x0009,
    STUN_ATTR_UNKNOWN_ATTRIBUTES = 0x000a,
    STUN_ATTR_REALM = 0x0014,
    STUN_ATTR_NONCE = 0x0015,
    STUN_ATTR_XOR_MAPPED_ADDRESS = 0x0020,
    STUN_ATTR_SOFTWARE = 0x8022,
    STUN_ATTR_ALTERNATE_SERVER = 0x8023,
    STUN_ATTR_FINGERPRINT = 0x8028
} StunAttributeType;

// STUN server configuration
typedef struct {
    char hostname[256];
    uint16_t port;
    NetAddress address;
    bool is_primary;
    float priority;
} StunServer;

// NAT traversal result
typedef struct {
    NatType nat_type;
    NetAddress external_address;
    NetAddress local_address;
    bool hairpinning_supported;
    bool filtering_supported;
    uint32_t rtt_ms;
    bool success;
} NatTraversalResult;

// Hole punching session
typedef struct {
    uint32_t session_id;
    NetAddress local_address;
    NetAddress external_address;
    NetAddress peer_address;
    NetAddress peer_external_address;
    uint64_t start_time;
    uint32_t timeout_ms;
    bool is_initiator;
    bool connected;
    uint8_t connection_attempts;
} HolePunchSession;

// NAT traversal system interface
typedef struct NatTraversalSystem {
    // Lifecycle
    bool (*init)(struct NatTraversalSystem* nat);
    void (*shutdown)(struct NatTraversalSystem* nat);
    void (*update)(struct NatTraversalSystem* nat, float delta_time);
    
    // NAT discovery
    bool (*discover_nat_type)(struct NatTraversalSystem* nat);
    NatTraversalResult (*get_nat_result)(struct NatTraversalSystem* nat);
    bool (*is_nat_discovery_complete)(struct NatTraversalSystem* nat);
    
    // STUN operations
    bool (*add_stun_server)(struct NatTraversalSystem* nat, const char* hostname, uint16_t port, bool primary);
    bool (*test_stun_server)(struct NatTraversalSystem* nat, const StunServer* server);
    NetAddress (*get_external_address)(struct NatTraversalSystem* nat);
    
    // Hole punching
    uint32_t (*start_hole_punch)(struct NatTraversalSystem* nat, const NetAddress* peer_address, 
                                bool is_initiator);
    bool (*is_hole_punch_complete)(struct NatTraversalSystem* nat, uint32_t session_id);
    HolePunchSession* (*get_hole_punch_session)(struct NatTraversalSystem* nat, uint32_t session_id);
    bool (*cancel_hole_punch)(struct NatTraversalSystem* nat, uint32_t session_id);
    
    // Relay fallback
    bool (*enable_relay_fallback)(struct NatTraversalSystem* nat, bool enable);
    bool (*connect_via_relay)(struct NatTraversalSystem* nat, const NetAddress* peer_address);
    
    // Statistics
    uint32_t (*get_active_sessions)(struct NatTraversalSystem* nat);
    float (*get_average_rtt)(struct NatTraversalSystem* nat);
    NatType (*get_detected_nat_type)(struct NatTraversalSystem* nat);
    
    // Internal state
    StunServer* stun_servers;
    uint32_t stun_server_count;
    uint32_t stun_server_capacity;
    HolePunchSession* sessions;
    uint32_t session_count;
    uint32_t session_capacity;
    NatTraversalResult nat_result;
    bool nat_discovery_complete;
    bool relay_fallback_enabled;
    uint32_t next_session_id;
    uint64_t last_update;
} NatTraversalSystem;

// Factory functions
NatTraversalSystem* nat_create_system(void);
void nat_destroy_system(NatTraversalSystem* nat);

// Utility functions
const char* nat_type_to_string(NatType type);
bool nat_supports_direct_connection(NatType local_type, NatType remote_type);
bool nat_requires_relay(NatType local_type, NatType remote_type);
uint32_t nat_calculate_connection_priority(NatType local_type, NatType remote_type);

// STUN protocol functions
bool stun_send_binding_request(const StunServer* server, NetAddress* mapped_address);
bool stun_parse_response(const uint8_t* data, size_t length, NetAddress* mapped_address);
bool stun_validate_message(const uint8_t* data, size_t length);
uint16_t stun_generate_transaction_id(uint8_t* transaction_id);

#endif // NAT_TRAVERSAL_H
