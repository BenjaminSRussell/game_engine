#ifndef NETWORK_TYPES_H
#define NETWORK_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Network address type. IPv4 stays 0 so zero-initialized addresses remain IPv4.
typedef enum {
  NET_ADDR_IPV4 = 0,
  NET_ADDR_IPV6 = 1
} NetAddressType;

// Network address structure. host and ip4 alias the first four bytes.
typedef struct {
  union {
    uint32_t host; // IPv4 address in network byte order (legacy access)
    uint32_t ip4;
    uint8_t ip6[16];
  };
  uint16_t port; // Port in host byte order (socket layer applies htons)
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
  PACKET_TYPE_PING = 16,
  PACKET_TYPE_PONG = 17,
  PACKET_TYPE_SNAPSHOT = 18,
  PACKET_TYPE_INPUT = 19,
  PACKET_TYPE_ACK = 20,
  PACKET_TYPE_FRAGMENT = 21,
  PACKET_TYPE_SNAPSHOT_ACK = 22,
  // Transport ACK/fragment/snapshot ACK stay 20/21/22. RPC ACKs use a free id.
  PACKET_TYPE_RPC_ACK = 23,

  PACKET_TYPE_COUNT
} PacketType;

// Packet header (sent with every packet)
typedef struct {
  uint32_t packet_id; // Unique ID for this packet
  uint8_t type;       // PacketType
  uint8_t flags;      // Optional flags
  uint16_t sequence;  // Sequence number for reliability
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
  CONNECTION_TIMEOUT,
  CONNECTION_RECONNECTING,
  CONNECTION_DISCONNECTING
} ConnectionState;

#endif // NETWORK_TYPES_H
