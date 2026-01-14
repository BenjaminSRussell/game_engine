/**
 * @file physics_networking.h
 * @brief Physics networking system
 *
 * Provides comprehensive networking support for physics systems including
 * client-side prediction, server-side authority, lag compensation,
 * and delta compression for multiplayer physics synchronization.
 */

#ifndef PHYSICS_NETWORKING_H
#define PHYSICS_NETWORKING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct PhysicsWorld PhysicsWorld;
typedef struct PhysicsObject PhysicsObject;

// ========================================
// Network Roles
// ========================================

typedef enum NetworkRole {
    NETWORK_ROLE_NONE,                // No networking
    NETWORK_ROLE_CLIENT,              // Client role
    NETWORK_ROLE_SERVER,              // Server role
    NETWORK_ROLE_PEER                 // Peer-to-peer role
} NetworkRole;

// ========================================
// Network Message Types
// ========================================

typedef enum NetworkMessageType {
    NET_MSG_WORLD_STATE,              // World state update
    NET_MSG_OBJECT_UPDATE,            // Object update
    NET_MSG_OBJECT_CREATE,            // Object creation
    NET_MSG_OBJECT_DESTROY,           // Object destruction
    NET_MSG_COLLISION_EVENT,          // Collision event
    NET_MSG_FORCE_APPLY,              // Force application
    NET_MSG_MATERIAL_CHANGE,          // Material change
    NET_MSG_CONSTRAINT_UPDATE,        // Constraint update
    NET_MSG_SIMULATION_SYNC,          // Simulation synchronization
    NET_MSG_CLIENT_INPUT,             // Client input
    NET_MSG_CLIENT_PREDICTION,        // Client prediction correction
    NET_MSG_SERVER_AUTHORITY,         // Server authority update
    NET_MSG_LAG_COMPENSATION,         // Lag compensation data
    NET_MSG_DELTA_COMPRESSION,         // Delta compression data
    NET_MSG_HEARTBEAT,                // Heartbeat
    NET_MSG_CONNECT,                  // Connection request
    NET_MSG_DISCONNECT,               // Disconnection
    NET_MSG_ACK,                      // Acknowledgment
    NET_MSG_NACK,                     // Negative acknowledgment
    NET_MSG_CUSTOM = 1000             // Custom messages
} NetworkMessageType;

// ========================================
// Network Flags
// ========================================

typedef enum NetworkFlags {
    NET_FLAG_NONE = 0x00,
    NET_FLAG_RELIABLE = 0x01,         // Reliable delivery
    NET_FLAG_ORDERED = 0x02,           // Ordered delivery
    NET_FLAG_SEQUENCED = 0x04,        // Sequenced delivery
    NET_FLAG_COMPRESSED = 0x08,        // Compressed data
    NET_FLAG_ENCRYPTED = 0x10,        // Encrypted data
    NET_FLAG_PRIORITY_HIGH = 0x20,     // High priority
    NET_FLAG_IMMEDIATE = 0x40,        // Immediate delivery
    NET_FLAG_BROADCAST = 0x80         // Broadcast to all
} NetworkFlags;

// ========================================
// Network Connection
// ========================================

typedef struct NetworkConnection {
    uint32_t connection_id;           // Connection ID
    char address[64];                 // Network address
    uint16_t port;                    // Network port
    bool connected;                   // Connection status
    bool authenticated;               // Authentication status
    uint64_t last_heartbeat;          // Last heartbeat time
    uint32_t ping_ms;                 // Ping in milliseconds
    float packet_loss;                // Packet loss rate
    uint64_t bytes_sent;              // Bytes sent
    uint64_t bytes_received;          // Bytes received
    uint32_t packets_sent;            // Packets sent
    uint32_t packets_received;        // Packets received
    uint32_t packets_dropped;         // Packets dropped
    float bandwidth_up;               // Upload bandwidth
    float bandwidth_down;             // Download bandwidth
    void *user_data;                 // User data pointer
} NetworkConnection;

// ========================================
// Network Message
// ========================================

typedef struct NetworkMessage {
    uint32_t message_id;              // Message ID
    NetworkMessageType type;           // Message type
    uint32_t flags;                   // Message flags
    uint64_t timestamp;               // Message timestamp
    uint32_t source_id;               // Source connection ID
    uint32_t target_id;               // Target connection ID (0 for broadcast)
    uint16_t sequence_number;         // Sequence number
    uint16_t ack_number;             // Acknowledgment number
    
    // Message data
    void *data;                       // Message data
    size_t data_size;                 // Data size
    size_t compressed_size;          // Compressed size
    
    // Metadata
    bool processed;                   // Message processed
    bool reliable;                    // Reliable delivery required
    bool ordered;                     // Ordered delivery required
    uint64_t send_time;               // Send time
    uint64_t receive_time;            // Receive time
    uint32_t retry_count;             // Retry count
    
} NetworkMessage;

// ========================================
// Object Network State
// ========================================

typedef struct ObjectNetworkState {
    uint64_t object_id;               // Object ID
    uint32_t network_id;              // Network ID
    bool network_owned;               // Network owned object
    bool client_predicted;            // Client predicted object
    bool server_authoritative;        // Server authoritative object
    
    // Position state
    float position[3];                // Current position
    float velocity[3];                // Current velocity
    float acceleration[3];            // Current acceleration
    uint64_t position_timestamp;      // Position timestamp
    
    // Rotation state
    float rotation[4];                // Current rotation (quaternion)
    float angular_velocity[3];        // Angular velocity
    uint64_t rotation_timestamp;      // Rotation timestamp
    
    // Physics state
    bool active;                      // Active state
    bool sleeping;                    // Sleeping state
    float mass;                       // Mass
    uint32_t material_id;             // Material ID
    
    // Prediction state
    float predicted_position[3];      // Predicted position
    float predicted_rotation[4];      // Predicted rotation
    float prediction_error;           // Prediction error
    uint64_t last_prediction_time;    // Last prediction time
    
    // Synchronization state
    uint32_t update_sequence;        // Update sequence number
    uint64_t last_update_time;       // Last update time
    uint32_t update_frequency;        // Update frequency
    bool needs_update;               // Needs update
    float priority;                  // Update priority
    
} ObjectNetworkState;

// ========================================
// Client Prediction State
// ========================================

typedef struct ClientPredictionState {
    uint64_t input_sequence;          // Input sequence number
    float input_values[16];           // Input values
    uint64_t input_timestamp;         // Input timestamp
    bool processed;                  // Input processed
    
    float predicted_position[3];      // Predicted position
    float predicted_rotation[4];      // Predicted rotation
    float predicted_velocity[3];      // Predicted velocity
    
    float server_position[3];         // Server position
    float server_rotation[4];         // Server rotation
    float server_velocity[3];         // Server velocity
    
    float position_error;             // Position error
    float rotation_error;             // Rotation error
    float velocity_error;             // Velocity error
    
    bool needs_correction;            // Needs correction
    uint64_t correction_sequence;     // Correction sequence
    float correction_strength;        // Correction strength
    
} ClientPredictionState;

// ========================================
// Lag Compensation Data
// ========================================

typedef struct LagCompensationData {
    uint32_t client_id;               // Client ID
    uint64_t client_time;             // Client time
    uint64_t server_time;             // Server time
    float time_difference;            // Time difference
    float interpolation_delay;         // Interpolation delay
    float extrapolation_limit;         // Extrapolation limit
    
    // Historical positions
    float *position_history;          // Position history buffer
    float *rotation_history;          // Rotation history buffer
    uint64_t *timestamp_history;      // Timestamp history buffer
    int history_size;                 // History buffer size
    int history_count;                // History count
    int history_index;               // Current history index
    
    // Compensation state
    bool compensation_enabled;         // Compensation enabled
    float compensation_factor;         // Compensation factor
    uint64_t last_compensation_time;  // Last compensation time
    
} LagCompensationData;

// ========================================
// Delta Compression
// ========================================

typedef struct DeltaCompressionData {
    uint32_t base_sequence;           // Base sequence number
    uint32_t target_sequence;         // Target sequence number
    
    // Changed objects
    uint64_t *changed_objects;        // Changed object IDs
    int changed_count;                // Number of changed objects
    
    // Delta data
    void *delta_data;                 // Delta data buffer
    size_t delta_size;                // Delta data size
    
    // Compression stats
    size_t original_size;             // Original data size
    size_t compressed_size;           // Compressed size
    float compression_ratio;          // Compression ratio
    
    bool valid;                       // Delta is valid
    uint64_t creation_time;           // Creation time
    
} DeltaCompressionData;

// ========================================
// Physics Network System
// ========================================

typedef struct PhysicsNetworkSystem {
    // Network configuration
    NetworkRole role;                 // Network role
    bool enabled;                     // System enabled
    bool authoritative_server;         // Authoritative server mode
    bool client_prediction;           // Client prediction enabled
    bool lag_compensation;            // Lag compensation enabled
    bool delta_compression;           // Delta compression enabled
    
    // Network connections
    NetworkConnection *connections;    // Connection array
    int connection_count;             // Number of connections
    int connection_capacity;          // Connection capacity
    uint32_t next_connection_id;       // Next connection ID
    
    // Message queues
    NetworkMessage *outgoing_queue;    // Outgoing message queue
    NetworkMessage *incoming_queue;    // Incoming message queue
    int outgoing_capacity;            // Outgoing queue capacity
    int incoming_capacity;            // Incoming queue capacity
    int outgoing_size;                // Outgoing queue size
    int incoming_size;                // Incoming queue size
    
    // Object network states
    ObjectNetworkState *object_states; // Object state array
    int object_count;                 // Number of objects
    int object_capacity;              // Object capacity
    
    // Client prediction
    ClientPredictionState *prediction_states; // Prediction states
    int prediction_count;             // Number of prediction states
    int prediction_capacity;          // Prediction capacity
    
    // Lag compensation
    LagCompensationData *lag_compensation; // Lag compensation data
    int lag_compensation_count;       // Number of clients
    int lag_compensation_capacity;    // Lag compensation capacity
    
    // Delta compression
    DeltaCompressionData *delta_data; // Delta compression data
    int delta_count;                  // Number of delta states
    int delta_capacity;               // Delta capacity
    
    // Synchronization
    float network_tick_rate;          // Network tick rate
    float interpolation_delay;        // Interpolation delay
    float extrapolation_limit;        // Extrapolation limit
    float position_tolerance;         // Position tolerance
    float rotation_tolerance;         // Rotation tolerance
    float velocity_tolerance;         // Velocity tolerance
    
    // Performance
    uint64_t messages_sent;           // Messages sent
    uint64_t messages_received;       // Messages received
    uint64_t bytes_sent;              // Bytes sent
    uint64_t bytes_received;          // Bytes received
    float average_latency;            // Average latency
    float packet_loss_rate;           // Packet loss rate
    
    // Timing
    uint64_t last_update_time;        // Last update time
    uint64_t last_sync_time;          // Last sync time
    float update_accumulator;          // Update accumulator
    
    // User data
    void *user_data;                 // User data pointer
    
} PhysicsNetworkSystem;

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create physics network system
 * @param role Network role
 * @param max_connections Maximum connections
 * @param max_objects Maximum objects
 * @return Network system or NULL on failure
 */
PhysicsNetworkSystem* physics_network_create(NetworkRole role, 
                                             int max_connections, int max_objects);

/**
 * Destroy physics network system
 * @param system Network system
 */
void physics_network_destroy(PhysicsNetworkSystem *system);

// ========================================
// System Control
// ========================================

/**
 * Enable/disable network system
 * @param system Network system
 * @param enabled Enable system
 */
void physics_network_set_enabled(PhysicsNetworkSystem *system, bool enabled);

/**
 * Set network role
 * @param system Network system
 * @param role Network role
 */
void physics_network_set_role(PhysicsNetworkSystem *system, NetworkRole role);

/**
 * Update network system
 * @param system Network system
 * @param delta_time Time delta
 */
void physics_network_update(PhysicsNetworkSystem *system, float delta_time);

// ========================================
// Connection Management
// ========================================

/**
 * Connect to server
 * @param system Network system
 * @param address Server address
 * @param port Server port
 * @return Connection ID or 0 on failure
 */
uint32_t physics_network_connect(PhysicsNetworkSystem *system, const char *address, uint16_t port);

/**
 * Disconnect from server
 * @param system Network system
 * @param connection_id Connection ID
 * @return True if successful
 */
bool physics_network_disconnect(PhysicsNetworkSystem *system, uint32_t connection_id);

/**
 * Accept connection
 * @param system Network system
 * @param connection_id Connection ID
 * @return True if successful
 */
bool physics_network_accept_connection(PhysicsNetworkSystem *system, uint32_t connection_id);

/**
 * Get connection info
 * @param system Network system
 * @param connection_id Connection ID
 * @return Connection info or NULL if not found
 */
NetworkConnection* physics_network_get_connection(PhysicsNetworkSystem *system, uint32_t connection_id);

// ========================================
// Object Synchronization
// ========================================

/**
 * Register object for networking
 * @param system Network system
 * @param object_id Object ID
 * @param network_id Network ID
 * @param network_owned Network owned
 * @param client_predicted Client predicted
 * @param server_authoritative Server authoritative
 * @return True if successful
 */
bool physics_network_register_object(PhysicsNetworkSystem *system, uint64_t object_id,
                                      uint32_t network_id, bool network_owned,
                                      bool client_predicted, bool server_authoritative);

/**
 * Unregister object from networking
 * @param system Network system
 * @param object_id Object ID
 * @return True if successful
 */
bool physics_network_unregister_object(PhysicsNetworkSystem *system, uint64_t object_id);

/**
 * Update object network state
 * @param system Network system
 * @param object_id Object ID
 * @param position Object position
 * @param rotation Object rotation
 * @param velocity Object velocity
 * @param active Active state
 * @param sleeping Sleeping state
 * @return True if successful
 */
bool physics_network_update_object_state(PhysicsNetworkSystem *system, uint64_t object_id,
                                         const float *position, const float *rotation,
                                         const float *velocity, bool active, bool sleeping);

/**
 * Get object network state
 * @param system Network system
 * @param object_id Object ID
 * @return Network state or NULL if not found
 */
ObjectNetworkState* physics_network_get_object_state(PhysicsNetworkSystem *system, uint64_t object_id);

// ========================================
// Client Prediction
// ========================================

/**
 * Enable/disable client prediction
 * @param system Network system
 * @param enabled Enable prediction
 */
void physics_network_set_client_prediction(PhysicsNetworkSystem *system, bool enabled);

/**
 * Add client input
 * @param system Network system
 * @param client_id Client ID
 * @param input_values Input values
 * @param input_count Number of input values
 * @return Input sequence number
 */
uint64_t physics_network_add_client_input(PhysicsNetworkSystem *system, uint32_t client_id,
                                          const float *input_values, int input_count);

/**
 * Apply server correction
 * @param system Network system
 * @param client_id Client ID
 * @param server_position Server position
 * @param server_rotation Server rotation
 * @param server_velocity Server velocity
 * @param sequence_number Sequence number
 * @return True if successful
 */
bool physics_network_apply_server_correction(PhysicsNetworkSystem *system, uint32_t client_id,
                                             const float *server_position, const float *server_rotation,
                                             const float *server_velocity, uint64_t sequence_number);

// ========================================
// Lag Compensation
// ========================================

/**
 * Enable/disable lag compensation
 * @param system Network system
 * @param enabled Enable compensation
 */
void physics_network_set_lag_compensation(PhysicsNetworkSystem *system, bool enabled);

/**
 * Set interpolation delay
 * @param system Network system
 * @param delay Interpolation delay
 */
void physics_network_set_interpolation_delay(PhysicsNetworkSystem *system, float delay);

/**
 * Set extrapolation limit
 * @param system Network system
 * @param limit Extrapolation limit
 */
void physics_network_set_extrapolation_limit(PhysicsNetworkSystem *system, float limit);

/**
 * Get compensated position
 * @param system Network system
 * @param client_id Client ID
 * @param object_id Object ID
 * @param client_time Client time
 * @param position Output compensated position
 * @return True if successful
 */
bool physics_network_get_compensated_position(PhysicsNetworkSystem *system, uint32_t client_id,
                                               uint64_t object_id, uint64_t client_time,
                                               float *position);

// ========================================
// Delta Compression
// ========================================

/**
 * Enable/disable delta compression
 * @param system Network system
 * @param enabled Enable compression
 */
void physics_network_set_delta_compression(PhysicsNetworkSystem *system, bool enabled);

/**
 * Create delta compression
 * @param system Network system
 * @param base_sequence Base sequence
 * @param target_sequence Target sequence
 * @return Delta data or NULL on failure
 */
DeltaCompressionData* physics_network_create_delta(PhysicsNetworkSystem *system,
                                                  uint32_t base_sequence, uint32_t target_sequence);

/**
 * Apply delta compression
 * @param system Network system
 * @param delta_data Delta data
 * @return True if successful
 */
bool physics_network_apply_delta(PhysicsNetworkSystem *system, const DeltaCompressionData *delta_data);

// ========================================
// Message Handling
// ========================================

/**
 * Send message
 * @param system Network system
 * @param message_type Message type
 * @param data Message data
 * @param data_size Data size
 * @param target_id Target connection ID (0 for broadcast)
 * @param flags Message flags
 * @return Message ID or 0 on failure
 */
uint32_t physics_network_send_message(PhysicsNetworkSystem *system, NetworkMessageType message_type,
                                       const void *data, size_t data_size,
                                       uint32_t target_id, uint32_t flags);

/**
 * Receive message
 * @param system Network system
 * @param message Output message
 * @return True if message received
 */
bool physics_network_receive_message(PhysicsNetworkSystem *system, NetworkMessage *message);

/**
 * Broadcast message
 * @param system Network system
 * @param message_type Message type
 * @param data Message data
 * @param data_size Data size
 * @param flags Message flags
 * @return Message ID or 0 on failure
 */
uint32_t physics_network_broadcast_message(PhysicsNetworkSystem *system, NetworkMessageType message_type,
                                           const void *data, size_t data_size, uint32_t flags);

// ========================================
// Statistics
// ========================================

/**
 * Get network statistics
 * @param system Network system
 * @param messages_sent Messages sent
 * @param messages_received Messages received
 * @param bytes_sent Bytes sent
 * @param bytes_received Bytes received
 * @param average_latency Average latency
 * @param packet_loss_rate Packet loss rate
 */
void physics_network_get_statistics(PhysicsNetworkSystem *system,
                                     uint64_t *messages_sent, uint64_t *messages_received,
                                     uint64_t *bytes_sent, uint64_t *bytes_received,
                                     float *average_latency, float *packet_loss_rate);

/**
 * Get connection statistics
 * @param system Network system
 * @param connection_id Connection ID
 * @param ping_ms Ping in milliseconds
 * @param packet_loss Packet loss rate
 * @param bandwidth_up Upload bandwidth
 * @param bandwidth_down Download bandwidth
 * @return True if successful
 */
bool physics_network_get_connection_stats(PhysicsNetworkSystem *system, uint32_t connection_id,
                                          uint32_t *ping_ms, float *packet_loss,
                                          float *bandwidth_up, float *bandwidth_down);

// ========================================
// Configuration
// ========================================

/**
 * Set network tick rate
 * @param system Network system
 * @param tick_rate Tick rate in Hz
 */
void physics_network_set_tick_rate(PhysicsNetworkSystem *system, float tick_rate);

/**
 * Set position tolerance
 * @param system Network system
 * @param tolerance Position tolerance
 */
void physics_network_set_position_tolerance(PhysicsNetworkSystem *system, float tolerance);

/**
 * Set rotation tolerance
 * @param system Network system
 * @param tolerance Rotation tolerance
 */
void physics_network_set_rotation_tolerance(PhysicsNetworkSystem *system, float tolerance);

/**
 * Set velocity tolerance
 * @param system Network system
 * @param tolerance Velocity tolerance
 */
void physics_network_set_velocity_tolerance(PhysicsNetworkSystem *system, float tolerance);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_NETWORKING_H */
