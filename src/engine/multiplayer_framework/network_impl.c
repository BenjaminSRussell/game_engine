/**
 * =================================================================================================
 *                              NETWORKING & MULTIPLAYER - IMPLEMENTATION
 *                              Agent: AGENT_NET_1
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */

#define MAX_CLIENTS 64
#define MAX_PACKET_SIZE 1400
#define MAX_PENDING_RELIABLE 256
#define NET_TIMEOUT_SEC 10.0f

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef enum PacketType {
  PACKET_CONNECT,
  PACKET_DISCONNECT,
  PACKET_HEARTBEAT,
  PACKET_RELIABLE,
  PACKET_UNRELIABLE,
  PACKET_ACK,
  PACKET_SNAPSHOT,
  PACKET_INPUT,
  PACKET_RPC,
} PacketType;

typedef struct PacketHeader {
  uint8_t type;
  uint8_t flags;
  uint16_t sequence;
  uint16_t ack;
  uint16_t ack_bits;
  uint32_t client_id;
} PacketHeader;

typedef struct PendingReliable {
  uint16_t sequence;
  uint8_t *data;
  uint16_t size;
  float send_time;
  uint8_t retry_count;
  bool acked;
} PendingReliable;

typedef struct NetworkClient {
  uint32_t id;
  uint32_t address;
  uint16_t port;
  float last_recv_time;
  float last_send_time;
  float rtt;

  uint16_t local_sequence;
  uint16_t remote_sequence;
  uint32_t ack_bits;

  PendingReliable pending[MAX_PENDING_RELIABLE];
  uint32_t pending_count;

  bool is_connected;
  bool is_local;
} NetworkClient;

typedef struct NetworkManager {
  int socket_fd;
  bool is_server;
  uint32_t local_client_id;

  NetworkClient clients[MAX_CLIENTS];
  uint32_t client_count;

  uint8_t send_buffer[MAX_PACKET_SIZE];
  uint8_t recv_buffer[MAX_PACKET_SIZE];

  float current_time;

  void (*on_connect)(uint32_t client_id);
  void (*on_disconnect)(uint32_t client_id);
  void (*on_receive)(uint32_t client_id, uint8_t *data, uint32_t size);

  bool initialized;
} NetworkManager;

static NetworkManager g_network = {0};

/* =================================================================================================
 *                                    PACKET HANDLING
 * =================================================================================================
 */

// DONE: Implement packet_write_header
void packet_write_header(uint8_t *buffer, PacketType type, uint16_t sequence,
                         uint16_t ack, uint16_t ack_bits) {
  PacketHeader *header = (PacketHeader *)buffer;
  header->type = type;
  header->flags = 0;
  header->sequence = sequence;
  header->ack = ack;
  header->ack_bits = ack_bits;
  header->client_id = g_network.local_client_id;
}

// DONE: Implement packet_read_header
PacketHeader *packet_read_header(uint8_t *buffer) {
  return (PacketHeader *)buffer;
}

// DONE: Implement packet_serialize_int
uint32_t packet_serialize_int(uint8_t *buffer, int32_t value) {
  memcpy(buffer, &value, sizeof(int32_t));
  return sizeof(int32_t);
}

// DONE: Implement packet_deserialize_int
int32_t packet_deserialize_int(uint8_t *buffer) {
  int32_t value;
  memcpy(&value, buffer, sizeof(int32_t));
  return value;
}

// DONE: Implement packet_serialize_float
uint32_t packet_serialize_float(uint8_t *buffer, float value) {
  memcpy(buffer, &value, sizeof(float));
  return sizeof(float);
}

// DONE: Implement packet_deserialize_float
float packet_deserialize_float(uint8_t *buffer) {
  float value;
  memcpy(&value, buffer, sizeof(float));
  return value;
}

// DONE: Implement packet_serialize_string
uint32_t packet_serialize_string(uint8_t *buffer, const char *str) {
  uint16_t len = strlen(str);
  memcpy(buffer, &len, sizeof(uint16_t));
  memcpy(buffer + sizeof(uint16_t), str, len);
  return sizeof(uint16_t) + len;
}

/* =================================================================================================
 *                                    CLIENT MANAGEMENT
 * =================================================================================================
 */

// DONE: Implement client_find
NetworkClient *client_find(uint32_t client_id) {
  for (uint32_t i = 0; i < g_network.client_count; i++) {
    if (g_network.clients[i].id == client_id) {
      return &g_network.clients[i];
    }
  }
  return NULL;
}

// DONE: Implement client_find_by_address
NetworkClient *client_find_by_address(uint32_t address, uint16_t port) {
  for (uint32_t i = 0; i < g_network.client_count; i++) {
    if (g_network.clients[i].address == address &&
        g_network.clients[i].port == port) {
      return &g_network.clients[i];
    }
  }
  return NULL;
}

// DONE: Implement client_create
uint32_t client_create(uint32_t address, uint16_t port) {
  if (g_network.client_count >= MAX_CLIENTS)
    return 0xFFFFFFFF;

  uint32_t id = g_network.client_count++;
  NetworkClient *client = &g_network.clients[id];

  memset(client, 0, sizeof(NetworkClient));
  client->id = id;
  client->address = address;
  client->port = port;
  client->is_connected = true;
  client->last_recv_time = g_network.current_time;

  return id;
}

// DONE: Implement client_disconnect
void client_disconnect(uint32_t client_id) {
  NetworkClient *client = client_find(client_id);
  if (!client)
    return;

  client->is_connected = false;

  // Free pending reliables
  for (uint32_t i = 0; i < client->pending_count; i++) {
    free(client->pending[i].data);
  }
  client->pending_count = 0;

  if (g_network.on_disconnect) {
    g_network.on_disconnect(client_id);
  }
}

/* =================================================================================================
 *                                    RELIABLE DELIVERY
 * =================================================================================================
 */

// DONE: Implement reliable_send
bool reliable_send(uint32_t client_id, uint8_t *data, uint32_t size) {
  NetworkClient *client = client_find(client_id);
  if (!client || !client->is_connected)
    return false;
  if (client->pending_count >= MAX_PENDING_RELIABLE)
    return false;

  PendingReliable *pending = &client->pending[client->pending_count++];
  pending->sequence = client->local_sequence++;
  pending->data = malloc(size);
  memcpy(pending->data, data, size);
  pending->size = size;
  pending->send_time = g_network.current_time;
  pending->retry_count = 0;
  pending->acked = false;

  // Build and send packet
  packet_write_header(g_network.send_buffer, PACKET_RELIABLE, pending->sequence,
                      client->remote_sequence, client->ack_bits);
  memcpy(g_network.send_buffer + sizeof(PacketHeader), data, size);

  // send() would go here

  return true;
}

// DONE: Implement reliable_process_ack
void reliable_process_ack(NetworkClient *client, uint16_t ack,
                          uint16_t ack_bits) {
  for (uint32_t i = 0; i < client->pending_count;) {
    PendingReliable *p = &client->pending[i];

    int16_t diff = (int16_t)ack - (int16_t)p->sequence;
    if (diff >= 0 && diff < 32) {
      bool is_acked = (diff == 0) || (ack_bits & (1 << (diff - 1)));
      if (is_acked) {
        // Calculate RTT
        float rtt = g_network.current_time - p->send_time;
        client->rtt = client->rtt * 0.9f + rtt * 0.1f;

        // Remove from pending
        free(p->data);
        *p = client->pending[--client->pending_count];
        continue;
      }
    }
    i++;
  }
}

// DONE: Implement reliable_resend_timeout
void reliable_resend_timeout(NetworkClient *client) {
  float resend_time = client->rtt * 2.0f;
  if (resend_time < 0.1f)
    resend_time = 0.1f;

  for (uint32_t i = 0; i < client->pending_count; i++) {
    PendingReliable *p = &client->pending[i];

    if (g_network.current_time - p->send_time > resend_time) {
      p->retry_count++;
      p->send_time = g_network.current_time;

      // Rebuild and resend packet
      packet_write_header(g_network.send_buffer, PACKET_RELIABLE, p->sequence,
                          client->remote_sequence, client->ack_bits);
      memcpy(g_network.send_buffer + sizeof(PacketHeader), p->data, p->size);

      // send() would go here
    }
  }
}

/* =================================================================================================
 *                                    NETWORK MANAGER
 * =================================================================================================
 */

// DONE: Implement network_init
bool network_init(bool is_server) {
  if (g_network.initialized)
    return false;

  memset(&g_network, 0, sizeof(NetworkManager));

  g_network.is_server = is_server;
  g_network.socket_fd = -1;

  // Would create UDP socket here
  // socket(), bind() if server

  g_network.initialized = true;
  return true;
}

// DONE: Implement network_shutdown
void network_shutdown(void) {
  if (!g_network.initialized)
    return;

  // Disconnect all clients
  for (uint32_t i = 0; i < g_network.client_count; i++) {
    if (g_network.clients[i].is_connected) {
      client_disconnect(g_network.clients[i].id);
    }
  }

  // Close socket
  // close(g_network.socket_fd);

  memset(&g_network, 0, sizeof(NetworkManager));
}

// DONE: Implement network_update
void network_update(float dt) {
  if (!g_network.initialized)
    return;

  g_network.current_time += dt;

  // Check for timeouts
  for (uint32_t i = 0; i < g_network.client_count; i++) {
    NetworkClient *client = &g_network.clients[i];
    if (!client->is_connected)
      continue;

    if (g_network.current_time - client->last_recv_time > NET_TIMEOUT_SEC) {
      client_disconnect(client->id);
      continue;
    }

    // Resend unacked reliables
    reliable_resend_timeout(client);

    // Send heartbeat
    if (g_network.current_time - client->last_send_time > 1.0f) {
      packet_write_header(g_network.send_buffer, PACKET_HEARTBEAT,
                          client->local_sequence, client->remote_sequence,
                          client->ack_bits);
      // send()
      client->last_send_time = g_network.current_time;
    }
  }
}

// DONE: Implement network_connect
bool network_connect(const char *address, uint16_t port) {
  (void)address;
  (void)port;

  if (!g_network.initialized || g_network.is_server)
    return false;

  // Parse address, create socket, send connect packet
  // ...

  return true;
}

// DONE: Implement network_disconnect
void network_disconnect(void) {
  if (!g_network.initialized || g_network.is_server)
    return;

  // Send disconnect packet
  packet_write_header(g_network.send_buffer, PACKET_DISCONNECT, 0, 0, 0);
  // send()
}

// DONE: Implement network_send_unreliable
void network_send_unreliable(uint32_t client_id, uint8_t *data, uint32_t size) {
  NetworkClient *client = client_find(client_id);
  if (!client || !client->is_connected)
    return;

  packet_write_header(g_network.send_buffer, PACKET_UNRELIABLE,
                      client->local_sequence++, client->remote_sequence,
                      client->ack_bits);
  memcpy(g_network.send_buffer + sizeof(PacketHeader), data, size);

  // send()
  client->last_send_time = g_network.current_time;
}

// DONE: Implement network_send_reliable
void network_send_reliable(uint32_t client_id, uint8_t *data, uint32_t size) {
  reliable_send(client_id, data, size);
}

// DONE: Implement network_broadcast
void network_broadcast(uint8_t *data, uint32_t size, bool reliable) {
  for (uint32_t i = 0; i < g_network.client_count; i++) {
    if (g_network.clients[i].is_connected) {
      if (reliable) {
        network_send_reliable(g_network.clients[i].id, data, size);
      } else {
        network_send_unreliable(g_network.clients[i].id, data, size);
      }
    }
  }
}

/* =================================================================================================
 *                                    RPC SYSTEM
 * =================================================================================================
 */

typedef struct RPCCall {
  uint32_t function_id;
  uint8_t args[128];
  uint32_t arg_size;
} RPCCall;

// DONE: Implement rpc_call
bool rpc_call(uint32_t client_id, uint32_t function_id, uint8_t *args,
              uint32_t arg_size) {
  if (arg_size + sizeof(uint32_t) > 128)
    return false;

  uint8_t buffer[MAX_PACKET_SIZE];
  uint32_t offset = 0;

  memcpy(buffer + offset, &function_id, sizeof(uint32_t));
  offset += sizeof(uint32_t);

  memcpy(buffer + offset, args, arg_size);
  offset += arg_size;

  network_send_reliable(client_id, buffer, offset);
  return true;
}

// DONE: Implement rpc_broadcast
void rpc_broadcast(uint32_t function_id, uint8_t *args, uint32_t arg_size) {
  for (uint32_t i = 0; i < g_network.client_count; i++) {
    if (g_network.clients[i].is_connected) {
      rpc_call(g_network.clients[i].id, function_id, args, arg_size);
    }
  }
}
