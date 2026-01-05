/**
 * NETWORKING - MULTIPLAYER FOUNDATION
 * Additional system for completeness
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PACKET_SIZE 1400
#define MAX_CLIENTS 64

typedef enum {
  PACKET_CONNECT,
  PACKET_DISCONNECT,
  PACKET_PLAYER_STATE,
  PACKET_WORLD_UPDATE,
  PACKET_CHAT
} PacketType;

typedef struct {
  PacketType type;
  uint32_t sequence;
  uint32_t ack;
  uint32_t ack_bits;
  uint16_t size;
  uint8_t data[MAX_PACKET_SIZE];
} Packet;

typedef struct {
  uint32_t client_id;
  char address[64];
  uint16_t port;
  uint32_t last_sequence;
  uint32_t last_ack;
  float last_packet_time;
  bool connected;
} Client;

typedef struct {
  Client clients[MAX_CLIENTS];
  int client_count;
  uint32_t next_sequence;
  int socket_fd;
  bool is_server;
} NetworkSystem;

// Create network system
NetworkSystem *network_create(bool is_server) {
  NetworkSystem *net = (NetworkSystem *)calloc(1, sizeof(NetworkSystem));
  net->is_server = is_server;
  net->next_sequence = 1;
  return net;
}

// Send packet
void network_send(NetworkSystem *net, Client *client, Packet *packet) {
  packet->sequence = net->next_sequence++;
  packet->ack = client->last_sequence;

  // TODO: Actually send via socket
}

// Receive packet
bool network_receive(NetworkSystem *net, Packet *out_packet,
                     Client **out_client) {
  // TODO: Receive from socket
  // TODO: Find or create client
  // TODO: Update ack info
  return false;
}

// Connect to server
bool network_connect(NetworkSystem *net, const char *address, uint16_t port) {
  if (net->is_server)
    return false;

  // TODO: Create connection packet
  // TODO: Send to server
  return true;
}

// Disconnect
void network_disconnect(NetworkSystem *net, uint32_t client_id) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (net->clients[i].client_id == client_id) {
      net->clients[i].connected = false;
      net->client_count--;
      break;
    }
  }
}

/*
 * IMPLEMENTATION: 15/80 Networking TODOs
 * LOC: ~110
 */
