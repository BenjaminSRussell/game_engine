#ifndef NETWORKING_TYPES_H
#define NETWORKING_TYPES_H

#include "../Public/unified_networking.h"
#include <pthread.h>
#include <time.h>

/* ============================================================================
 * INTERNAL TYPES - NOT EXPOSED TO PUBLIC API
 * ============================================================================
 */

#define MAX_CLIENTS 64
#define MAX_PACKET_SIZE 8192
#define MAX_PACKET_QUEUE 1024
#define MAX_REPLICATED_ENTITIES 1024
#define MAX_REPLICATION_FIELDS 64
#define PACKET_HEADER_SIZE 16
#define HEARTBEAT_INTERVAL_SEC 5.0f
#define CONNECTION_TIMEOUT_SEC 10.0f

// Forward declarations
typedef struct NetSocket NetSocket;
typedef struct NetworkClient NetworkClient;
typedef struct ReplicatedEntity ReplicatedEntity;

/* ============================================================================
 * SOCKET LAYER
 * ============================================================================
 */

struct NetSocket {
#ifdef _WIN32
  void *handle; // SOCKET type
#else
  int handle;
#endif
  uint16_t port;
  bool is_open;
};

/* ============================================================================
 * CONNECTION LAYER
 * ============================================================================
 */

typedef struct {
  NetworkPacket packet;
  uint32_t retry_count;
  struct timespec send_time;
} QueuedPacket;

struct NetworkClient {
  uint32_t client_id;
  NetAddress address;
  char username[64];
  bool connected;
  ConnectionState state;

  // Timing
  float last_heartbeat;
  struct timespec connect_time;
  struct timespec last_activity_time;

  // Sequencing
  uint16_t next_sequence;
  uint16_t expected_sequence;

  // Packet queues
  QueuedPacket sent_queue[MAX_PACKET_QUEUE];
  uint32_t sent_queue_head;
  uint32_t sent_queue_tail;

  NetworkPacket recv_queue[MAX_PACKET_QUEUE];
  uint32_t recv_queue_head;
  uint32_t recv_queue_tail;

  // Statistics
  uint32_t packets_sent;
  uint32_t packets_received;
  uint64_t bytes_sent;
  uint64_t bytes_received;
  float latency_ms;

  pthread_mutex_t lock;
};

/* ============================================================================
 * REPLICATION LAYER
 * ============================================================================
 */

struct ReplicatedEntity {
  uint32_t entity_id;
  uint32_t owner_id;
  uint32_t version;
  float position[3];
  uint32_t field_mask;
  uint16_t data_size;
  uint8_t field_data[MAX_REPLICATION_FIELDS];
  bool changed;
  bool always_replicate;
};

typedef struct {
  uint32_t client_id;
  bool active;
  uint32_t known_versions[MAX_REPLICATED_ENTITIES];
  uint16_t last_sent_sequence;
  uint16_t last_acked_sequence;
} ReplicationClient;

typedef struct {
  uint32_t sequence_number;
  uint32_t timestamp;
  uint32_t entity_count;
  ReplicatedEntity entities[MAX_REPLICATED_ENTITIES];
  bool is_compressed;
  uint32_t compressed_size;
  uint8_t compressed_data[MAX_PACKET_SIZE];
} ReplicationSnapshot;

typedef struct {
  uint32_t entity_id;
  uint32_t last_sequence;
  uint32_t last_timestamp;
  ReplicatedEntity last_state;
  bool is_new;
} ClientEntityState;

/* ============================================================================
 * MAIN CONTEXT
 * ============================================================================
 */

struct NetworkContext {
  // Mode
  NetworkMode mode;
  bool is_running;

  // Socket
  NetSocket *socket;

  // Configuration
  NetworkConfig config;

  // Server state
  NetworkClient clients[MAX_CLIENTS];
  uint32_t client_count;
  uint32_t next_client_id;

  // Client state
  uint32_t local_client_id;
  NetAddress server_address;
  bool connected_to_server;

  // Common state
  float last_heartbeat_sent;
  NetworkStats stats;

  // Replication state
  ReplicatedEntity server_entities[MAX_REPLICATED_ENTITIES];
  uint32_t server_entity_count;
  ReplicationClient replication_clients[MAX_CLIENTS];

  ClientEntityState client_states[MAX_REPLICATED_ENTITIES];
  uint32_t client_state_count;

  uint32_t last_sequence_number;
  float replication_timer;

  // Thread safety
  pthread_mutex_t lock;
};

/* ============================================================================
 * PACKET HEADER
 * ============================================================================
 */

typedef struct {
  PacketType type;
  uint8_t flags;
  uint16_t sequence;
  uint32_t timestamp;
  uint16_t data_size;
  uint16_t checksum;
} PacketHeader;

#endif // NETWORKING_TYPES_H
