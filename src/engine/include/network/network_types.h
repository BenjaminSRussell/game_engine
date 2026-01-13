#ifndef NETWORK_TYPES_H
#define NETWORK_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Network address type
typedef enum {
    NET_ADDR_IPV4 = 0,
    NET_ADDR_IPV6 = 1
} NetAddressType;

// Network address structure
typedef struct {
  union {
      uint32_t host; // IPv4 address in network byte order (legacy access)
      uint32_t ip4;
      uint8_t ip6[16];
  };
  uint16_t port; // Port in network byte order
  uint8_t type;  // NetAddressType
} NetAddress;

// Packet types
typedef enum {
    PACKET_TYPE_CONNECT = 0,
    PACKET_TYPE_DISCONNECT = 1,
    PACKET_TYPE_HEARTBEAT = 2,
    PACKET_TYPE_CHAT = 3,
    PACKET_TYPE_PLAYER_UPDATE = 4,
    PACKET_TYPE_WORLD_UPDATE = 5,
    PACKET_TYPE_ENTITY_SPAWN = 6,
    PACKET_TYPE_ENTITY_DESTROY = 7,
    PACKET_TYPE_ENTITY_UPDATE = 8,
    PACKET_TYPE_BLOCK_CHANGE = 9,
    PACKET_TYPE_CHUNK_DATA = 10,
    PACKET_TYPE_SERVER_INFO = 11,
    PACKET_TYPE_CLIENT_INFO = 12,
    PACKET_TYPE_AUTH_REQUEST = 13,
    PACKET_TYPE_AUTH_RESPONSE = 14,
    PACKET_TYPE_RPC = 15,

    // Legacy/Alias support if needed, but preferred to use above
    PACKET_TYPE_PING = 16,
    PACKET_TYPE_PONG = 17,
    PACKET_TYPE_SNAPSHOT = 18,
    PACKET_TYPE_INPUT = 19,
    PACKET_TYPE_RPC_ACK = 20,

    PACKET_TYPE_COUNT
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
