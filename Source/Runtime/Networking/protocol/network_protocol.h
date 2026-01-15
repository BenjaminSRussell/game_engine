#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include "include/math/math_all.h"
#include <stdbool.h>
#include <stdint.h>

// Protocol version
#define NETWORK_PROTOCOL_VERSION 1
#define MAX_PACKET_SIZE 1400 // UDP safe packet size
#define MAX_MESSAGE_TYPES 64

// Message types
typedef enum {
  MSG_NONE = 0,
  MSG_CONNECT = 1,
  MSG_DISCONNECT = 2,
  MSG_HEARTBEAT = 3,
  MSG_CLIENT_INPUT = 4,
  MSG_SERVER_STATE = 5,
  MSG_ENTITY_CREATE = 6,
  MSG_ENTITY_UPDATE = 7,
  MSG_ENTITY_DESTROY = 8,
  MSG_PLAYER_JOIN = 9,
  MSG_PLAYER_LEAVE = 10,
  MSG_CHAT_MESSAGE = 11,
  MSG_RPC_CALL = 12,
  MSG_RPC_RESPONSE = 13,
  MSG_CUSTOM_START = 100
} MessageType;

#include <core/math/types.h>

// Packet flags are defined in core/math/types.h

// Network address
typedef struct {
  uint32_t address; // IPv4 address
  uint16_t port;
} NetworkAddress;

// Packet header
typedef struct {
  uint8_t protocol_version;
  uint8_t message_type;
  uint16_t sequence_number;
  uint16_t ack_number;
  uint32_t timestamp;
  uint8_t flags;
  uint16_t data_size;
  uint16_t checksum;
} PacketHeader;

// Network packet
typedef struct {
  PacketHeader header;
  uint8_t data[MAX_PACKET_SIZE - sizeof(PacketHeader)];
  NetworkAddress from_address;
  NetworkAddress to_address;
  float receive_time;
} NetworkPacket;

// Connection state
typedef enum {
  CONN_DISCONNECTED,
  CONN_CONNECTING,
  CONN_CONNECTED,
  CONN_DISCONNECTING
} ConnectionState;

// Connection info
typedef struct {
  NetworkAddress address;
  ConnectionState state;
  uint32_t client_id;
  uint16_t last_sequence;
  uint16_t remote_sequence;
  uint32_t last_acked_sequence;
  float last_heartbeat_time;
  float rtt; // Round-trip time
  float packet_loss;
  uint32_t packets_sent;
  uint32_t packets_received;
  uint32_t packets_lost;
} NetworkConnection;

// Protocol statistics
typedef struct {
  uint32_t packets_sent;
  uint32_t packets_received;
  uint32_t packets_lost;
  uint32_t bytes_sent;
  uint32_t bytes_received;
  float average_rtt;
  float packet_loss_rate;
  uint32_t connections_active;
} ProtocolStats;

// Message handlers
typedef bool (*MessageHandler)(const NetworkPacket *packet, void *user_data);

// Protocol configuration
typedef struct {
  uint16_t port;
  uint32_t max_connections;
  float heartbeat_interval;
  float connection_timeout;
  float rtt_smoothing_factor;
  bool enable_compression;
  bool enable_encryption;
  uint8_t encryption_key[32];
} ProtocolConfig;

// Protocol instance
typedef struct {
  ProtocolConfig config;
  NetworkConnection *connections;
  uint32_t connection_count;
  uint32_t max_connections;
  MessageHandler message_handlers[MAX_MESSAGE_TYPES];
  void *handler_user_data[MAX_MESSAGE_TYPES];
  ProtocolStats stats;
  uint16_t next_sequence;
  bool is_server;
  bool is_initialized;
} NetworkProtocol;

// Function declarations
NetworkProtocol *protocol_create(const ProtocolConfig *config, bool is_server);
void protocol_destroy(NetworkProtocol *protocol);
bool protocol_initialize(NetworkProtocol *protocol);
void protocol_shutdown(NetworkProtocol *protocol);
void protocol_update(NetworkProtocol *protocol, float current_time);

// Connection management
uint32_t protocol_add_connection(NetworkProtocol *protocol,
                                 const NetworkAddress *address);
bool protocol_remove_connection(NetworkProtocol *protocol,
                                uint32_t connection_id);
NetworkConnection *protocol_get_connection(NetworkProtocol *protocol,
                                           uint32_t connection_id);
NetworkConnection *
protocol_find_connection_by_address(NetworkProtocol *protocol,
                                    const NetworkAddress *address);

// Packet sending/receiving
bool protocol_send_packet(NetworkProtocol *protocol, uint32_t connection_id,
                          MessageType type, const void *data,
                          uint16_t data_size, uint8_t flags);
bool protocol_broadcast_packet(NetworkProtocol *protocol, MessageType type,
                               const void *data, uint16_t data_size,
                               uint8_t flags);
bool protocol_receive_packet(NetworkProtocol *protocol,
                             NetworkPacket *out_packet);
void protocol_process_packet(NetworkProtocol *protocol,
                             const NetworkPacket *packet);

// Message handlers
void protocol_register_handler(NetworkProtocol *protocol, MessageType type,
                               MessageHandler handler, void *user_data);
void protocol_unregister_handler(NetworkProtocol *protocol, MessageType type);

// Utility functions
uint16_t calculate_checksum(const void *data, size_t size);
bool validate_packet(const NetworkPacket *packet);
void serialize_packet_header(const PacketHeader *header, uint8_t *buffer);
bool deserialize_packet_header(const uint8_t *buffer, PacketHeader *header);
NetworkAddress parse_address(const char *address_string);
bool address_equals(const NetworkAddress *a, const NetworkAddress *b);
uint32_t address_hash(const NetworkAddress *address);

// Statistics
void protocol_get_stats(const NetworkProtocol *protocol,
                        ProtocolStats *out_stats);
void protocol_reset_stats(NetworkProtocol *protocol);

// Reliable transport
bool protocol_send_reliable(NetworkProtocol *protocol, uint32_t connection_id,
                            MessageType type, const void *data,
                            uint16_t data_size);
bool protocol_send_unreliable(NetworkProtocol *protocol, uint32_t connection_id,
                              MessageType type, const void *data,
                              uint16_t data_size);

// Fragmentation (for large messages)
bool protocol_send_fragmented(NetworkProtocol *protocol, uint32_t connection_id,
                              MessageType type, const void *data,
                              uint32_t data_size);
bool protocol_reassemble_fragments(NetworkProtocol *protocol,
                                   const NetworkPacket *packet, void **out_data,
                                   uint32_t *out_size);

#endif // NETWORK_PROTOCOL_H
