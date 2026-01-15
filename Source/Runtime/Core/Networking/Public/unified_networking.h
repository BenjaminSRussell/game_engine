#ifndef UNIFIED_NETWORKING_H
#define UNIFIED_NETWORKING_H

#include "core/types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * NETWORKING SYSTEM - PUBLIC API
 * ============================================================================
 */

// Forward declarations (opaque types)
typedef struct NetworkContext NetworkContext;
typedef struct NetworkConnection NetworkConnection;

/* ============================================================================
 * TYPES AND ENUMS
 * ============================================================================
 */

typedef enum {
  NETWORK_MODE_NONE = 0,
  NETWORK_MODE_SERVER,
  NETWORK_MODE_CLIENT
} NetworkMode;

typedef enum {
  CONNECTION_STATE_DISCONNECTED = 0,
  CONNECTION_STATE_CONNECTING,
  CONNECTION_STATE_CONNECTED,
  CONNECTION_STATE_DISCONNECTING
} ConnectionState;

typedef enum {
  PACKET_TYPE_CONNECT = 0,
  PACKET_TYPE_DISCONNECT,
  PACKET_TYPE_HEARTBEAT,
  PACKET_TYPE_AUTH_REQUEST,
  PACKET_TYPE_AUTH_RESPONSE,
  PACKET_TYPE_RPC,
  PACKET_TYPE_SNAPSHOT,
  PACKET_TYPE_SNAPSHOT_ACK,
  PACKET_TYPE_CUSTOM
} PacketType;

typedef enum {
  DELIVERY_TYPE_UNRELIABLE = 0,
  DELIVERY_TYPE_RELIABLE
} DeliveryType;

// Network address
typedef struct {
  uint32_t host;
  uint16_t port;
} NetAddress;

// Packet structure
typedef struct {
  PacketType type;
  DeliveryType delivery_type;
  uint32_t sequence_number;
  uint32_t timestamp;
  uint8_t *data;
  uint32_t size;
} NetworkPacket;

// Network statistics
typedef struct {
  uint64_t packets_sent;
  uint64_t packets_received;
  uint64_t bytes_sent;
  uint64_t bytes_received;
  uint32_t active_connections;
  float average_latency_ms;
} NetworkStats;

// Configuration
typedef struct {
  uint16_t max_connections;
  uint32_t max_packet_size;
  uint32_t timeout_ms;
  bool enable_compression;
  uint32_t replication_rate_hz;
  float relevancy_distance;
} NetworkConfig;

/* ============================================================================
 * CORE API
 * ============================================================================
 */

/**
 * Initialize the networking system
 * @param mode Server or client mode
 * @param config Configuration parameters
 * @return Network context on success, NULL on failure
 */
NetworkContext *network_init(NetworkMode mode, const NetworkConfig *config);

/**
 * Shutdown the networking system
 * @param context Network context to shutdown
 */
void network_shutdown(NetworkContext *context);

/**
 * Update the networking system (call each frame)
 * @param context Network context
 * @param delta_time Time since last update in seconds
 * @return 0 on success, negative on error
 */
int network_update(NetworkContext *context, float delta_time);

/**
 * Get networking statistics
 * @param context Network context
 * @param stats Output statistics structure
 * @return 0 on success, negative on error
 */
int network_get_stats(NetworkContext *context, NetworkStats *stats);

/* ============================================================================
 * SERVER API
 * ============================================================================
 */

/**
 * Start a network server
 * @param context Network context
 * @param port Port to bind to
 * @param server_name Optional server name
 * @return 0 on success, negative on error
 */
int network_server_start(NetworkContext *context, uint16_t port,
                         const char *server_name);

/**
 * Stop the network server
 * @param context Network context
 * @return 0 on success, negative on error
 */
int network_server_stop(NetworkContext *context);

/**
 * Send packet to a specific client
 * @param context Network context
 * @param client_id Target client ID
 * @param type Packet type
 * @param data Packet data
 * @param size Data size
 * @return 0 on success, negative on error
 */
int network_server_send_to_client(NetworkContext *context, uint32_t client_id,
                                  PacketType type, const void *data,
                                  uint32_t size);

/**
 * Broadcast packet to all clients
 * @param context Network context
 * @param type Packet type
 * @param data Packet data
 * @param size Data size
 * @return Number of clients sent to, negative on error
 */
int network_server_broadcast(NetworkContext *context, PacketType type,
                             const void *data, uint32_t size);

/**
 * Get active client count
 * @param context Network context
 * @return Number of active clients
 */
uint32_t network_server_get_client_count(NetworkContext *context);

/**
 * Get list of connected client IDs
 * @param context Network context
 * @param out_client_ids Output array
 * @param max_count Maximum clients to return
 * @return Actual number of clients returned
 */
int network_server_get_clients(NetworkContext *context,
                               uint32_t *out_client_ids, uint32_t max_count);

/* ============================================================================
 * CLIENT API
 * ============================================================================
 */

/**
 * Connect to a server
 * @param context Network context
 * @param address Server address
 * @param port Server port
 * @param username Client username
 * @return 0 on success, negative on error
 */
int network_client_connect(NetworkContext *context, const char *address,
                           uint16_t port, const char *username);

/**
 * Disconnect from server
 * @param context Network context
 * @return 0 on success, negative on error
 */
int network_client_disconnect(NetworkContext *context);

/**
 * Send packet to server
 * @param context Network context
 * @param type Packet type
 * @param data Packet data
 * @param size Data size
 * @return 0 on success, negative on error
 */
int network_client_send(NetworkContext *context, PacketType type,
                        const void *data, uint32_t size);

/**
 * Check if connected to server
 * @param context Network context
 * @return true if connected, false otherwise
 */
bool network_client_is_connected(NetworkContext *context);

/**
 * Get local client ID
 * @param context Network context
 * @return Client ID
 */
uint32_t network_client_get_id(NetworkContext *context);

/* ============================================================================
 * REPLICATION API
 * ============================================================================
 */

/**
 * Register an entity for replication
 * @param context Network context
 * @param entity_id Unique entity ID
 * @param always_replicate Always replicate regardless of relevancy
 * @return 0 on success, negative on error
 */
int network_replication_add_entity(NetworkContext *context, uint32_t entity_id,
                                   bool always_replicate);

/**
 * Unregister an entity from replication
 * @param context Network context
 * @param entity_id Entity ID to remove
 */
void network_replication_remove_entity(NetworkContext *context,
                                       uint32_t entity_id);

/**
 * Update entity replication data
 * @param context Network context
 * @param entity_id Entity ID
 * @param field_mask Bitmask of changed fields
 * @param field_data Field data buffer
 * @param data_size Size of field data
 */
void network_replication_update_entity(NetworkContext *context,
                                       uint32_t entity_id, uint32_t field_mask,
                                       const void *field_data,
                                       uint16_t data_size);

/**
 * Set entity position for relevancy checks
 * @param context Network context
 * @param entity_id Entity ID
 * @param x, y, z Position coordinates
 */
void network_replication_set_entity_position(NetworkContext *context,
                                             uint32_t entity_id, float x,
                                             float y, float z);

/**
 * Set entity owner (for ownership-based replication)
 * @param context Network context
 * @param entity_id Entity ID
 * @param owner_client_id Owning client ID
 */
void network_replication_set_owner(NetworkContext *context, uint32_t entity_id,
                                   uint32_t owner_client_id);

/**
 * Get entity owner
 * @param context Network context
 * @param entity_id Entity ID
 * @return Owner client ID, 0 if no owner
 */
uint32_t network_replication_get_owner(NetworkContext *context,
                                       uint32_t entity_id);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

/**
 * Get default network configuration
 * @return Default configuration structure
 */
NetworkConfig network_get_default_config(void);

/**
 * Convert address string to NetAddress
 * @param address_str Address string (e.g., "127.0.0.1")
 * @param port Port number
 * @param out_address Output NetAddress
 * @return true on success, false on failure
 */
bool network_parse_address(const char *address_str, uint16_t port,
                           NetAddress *out_address);

/**
 * Get error string for last network error
 * @return Error string
 */
const char *network_get_error(void);

#ifdef __cplusplus
}
#endif

#endif // UNIFIED_NETWORKING_H
