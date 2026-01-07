#ifndef NETWORK_TYPES_H
#define NETWORK_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Network address structure
typedef struct {
  uint32_t host; // IPv4 address in network byte order
  uint16_t port; // Port in network byte order
} NetAddress;

// Packet types
typedef enum {
  PACKET_CONNECT = 0,
  PACKET_DISCONNECT,
  PACKET_PING,
  PACKET_PONG,
  PACKET_SNAPSHOT, // Entity state snapshot
  PACKET_INPUT,    // Player input
  PACKET_SPAWN_ENTITY,
  PACKET_DESTROY_ENTITY,
  PACKET_CHAT_MESSAGE,
  PACKET_COUNT
} PacketType;

// Packet header (sent with every packet)
typedef struct {
  uint8_t type;
  uint8_t flags;
  uint16_t sequence;  // Packet sequence number
  uint32_t timestamp; // Milliseconds since connection
  uint16_t data_size; // Size of payload
} PacketHeader;

// Network configuration
typedef struct {
  uint16_t port;
  uint32_t max_connections;
  uint32_t tick_rate;     // Server ticks per second
  uint32_t snapshot_rate; // Snapshots per second
  float timeout_seconds;
} NetworkConfig;

// Connection state
typedef enum {
  CONNECTION_DISCONNECTED = 0,
  CONNECTION_CONNECTING,
  CONNECTION_CONNECTED,
  CONNECTION_TIMEOUT
} ConnectionState;

#endif // NETWORK_TYPES_H
