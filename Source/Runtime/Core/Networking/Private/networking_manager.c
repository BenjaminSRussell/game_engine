#include "networking_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

extern NetSocket *socket_create(uint16_t port);
extern void socket_close(NetSocket *sock);
extern bool socket_send(NetSocket *sock, const NetAddress *addr,
                        const void *data, uint32_t size);
extern int socket_receive(NetSocket *sock, NetAddress *from, void *buffer,
                          uint32_t buffer_size);

extern NetworkClient *network_connection_find_free(NetworkContext *ctx);
extern NetworkClient *network_connection_find_by_id(NetworkContext *ctx,
                                                    uint32_t client_id);
extern NetworkClient *
network_connection_find_by_address(NetworkContext *ctx, const NetAddress *addr);
extern bool network_connection_enqueue_packet(NetworkClient *client,
                                              const NetworkPacket *packet);
extern uint64_t network_connection_get_timestamp_ms(void);
extern void network_connection_update_activity(NetworkClient *client);

/* ============================================================================
 * PACKET SERIALIZATION
 * ============================================================================
 */

static void serialize_packet_header(uint8_t *buffer,
                                    const PacketHeader *header) {
  memcpy(&buffer[0], &header->type, 2);
  memcpy(&buffer[2], &header->flags, 1);
  memcpy(&buffer[3], &header->sequence, 2);
  memcpy(&buffer[5], &header->timestamp, 4);
  memcpy(&buffer[9], &header->data_size, 2);
  memcpy(&buffer[11], &header->checksum, 2);
}

/* ============================================================================
 * SERVER IMPLEMENTATION
 * ============================================================================
 */

int network_server_start(NetworkContext *context, uint16_t port,
                         const char *server_name) {
  if (!context || context->mode != NETWORK_MODE_SERVER) {
    LOG_ERROR(LOG_CAT_GENERAL, "Invalid context or mode for server start");
    return -1;
  }

  if (context->is_running) {
    LOG_ERROR(LOG_CAT_GENERAL, "Server already running");
    return -1;
  }

  context->socket = socket_create(port);
  if (!context->socket) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to create server socket");
    return -1;
  }

  context->is_running = true;
  context->client_count = 0;
  context->next_client_id = 1;

  LOG_INFO(LOG_CAT_GENERAL, "Network server started on port %d%s%s", port,
           server_name ? " (name: " : "", server_name ? server_name : "");

  return 0;
}

int network_server_stop(NetworkContext *context) {
  if (!context || !context->is_running ||
      context->mode != NETWORK_MODE_SERVER) {
    return -1;
  }

  // Send disconnect to all clients
  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    if (context->clients[i].connected) {
      uint8_t buffer[PACKET_HEADER_SIZE + 4];
      PacketHeader header = {
          .type = PACKET_TYPE_DISCONNECT,
          .flags = 0,
          .sequence = context->clients[i].next_sequence++,
          .timestamp = (uint32_t)network_connection_get_timestamp_ms(),
          .data_size = 4,
          .checksum = 0};
      serialize_packet_header(buffer, &header);
      memcpy(&buffer[PACKET_HEADER_SIZE], &context->clients[i].client_id, 4);

      socket_send(context->socket, &context->clients[i].address, buffer,
                  sizeof(buffer));
    }
  }

  socket_close(context->socket);
  context->socket = NULL;
  context->is_running = false;

  LOG_INFO(LOG_CAT_GENERAL, "Network server stopped");
  return 0;
}

int network_server_send_to_client(NetworkContext *context, uint32_t client_id,
                                  PacketType type, const void *data,
                                  uint32_t size) {
  if (!context || !context->is_running ||
      context->mode != NETWORK_MODE_SERVER) {
    return -1;
  }

  NetworkClient *client = network_connection_find_by_id(context, client_id);
  if (!client) {
    LOG_ERROR(LOG_CAT_GENERAL, "Client %u not found", client_id);
    return -1;
  }

  uint8_t buffer[MAX_PACKET_SIZE];
  PacketHeader header = {.type = type,
                         .flags = 0,
                         .sequence = client->next_sequence++,
                         .timestamp =
                             (uint32_t)network_connection_get_timestamp_ms(),
                         .data_size = (uint16_t)size,
                         .checksum = 0};

  serialize_packet_header(buffer, &header);
  if (data && size > 0) {
    memcpy(&buffer[PACKET_HEADER_SIZE], data, size);
  }

  uint32_t total_size = PACKET_HEADER_SIZE + size;
  if (socket_send(context->socket, &client->address, buffer, total_size)) {
    context->stats.packets_sent++;
    context->stats.bytes_sent += total_size;
    return 0;
  }

  return -1;
}

int network_server_broadcast(NetworkContext *context, PacketType type,
                             const void *data, uint32_t size) {
  if (!context || !context->is_running ||
      context->mode != NETWORK_MODE_SERVER) {
    return -1;
  }

  int sent_count = 0;
  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    if (context->clients[i].connected) {
      if (network_server_send_to_client(context, context->clients[i].client_id,
                                        type, data, size) == 0) {
        sent_count++;
      }
    }
  }

  return sent_count;
}

uint32_t network_server_get_client_count(NetworkContext *context) {
  if (!context)
    return 0;
  return context->client_count;
}

int network_server_get_clients(NetworkContext *context,
                               uint32_t *out_client_ids, uint32_t max_count) {
  if (!context || !out_client_ids || max_count == 0)
    return 0;

  uint32_t count = 0;
  for (uint32_t i = 0; i < MAX_CLIENTS && count < max_count; i++) {
    if (context->clients[i].connected) {
      out_client_ids[count++] = context->clients[i].client_id;
    }
  }

  return count;
}

/* ============================================================================
 * CLIENT IMPLEMENTATION
 * ============================================================================
 */

int network_client_connect(NetworkContext *context, const char *address,
                           uint16_t port, const char *username) {
  if (!context || context->mode != NETWORK_MODE_CLIENT || !address ||
      !username) {
    LOG_ERROR(LOG_CAT_GENERAL, "Invalid parameters for client connect");
    return -1;
  }

  if (context->is_running) {
    LOG_ERROR(LOG_CAT_GENERAL, "Network already running");
    return -1;
  }

  // Create client socket
  context->socket = socket_create(0); // Bind to any port
  if (!context->socket) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to create client socket");
    return -1;
  }

  // Parse server address
  if (!network_parse_address(address, port, &context->server_address)) {
    LOG_ERROR(LOG_CAT_GENERAL, "Invalid server address: %s", address);
    socket_close(context->socket);
    return -1;
  }

  context->is_running = true;
  context->connected_to_server = false;

  // Store username in first client slot (used for local tracking)
  strncpy(context->clients[0].username, username,
          sizeof(context->clients[0].username) - 1);
  context->clients[0].username[sizeof(context->clients[0].username) - 1] = '\0';

  // Send connection request
  uint8_t buffer[MAX_PACKET_SIZE];
  PacketHeader header = {.type = PACKET_TYPE_CONNECT,
                         .flags = 0,
                         .sequence = 0,
                         .timestamp =
                             (uint32_t)network_connection_get_timestamp_ms(),
                         .data_size = (uint16_t)strlen(username) + 1,
                         .checksum = 0};

  serialize_packet_header(buffer, &header);
  strcpy((char *)&buffer[PACKET_HEADER_SIZE], username);

  uint32_t total_size = PACKET_HEADER_SIZE + header.data_size;
  if (socket_send(context->socket, &context->server_address, buffer,
                  total_size)) {
    LOG_INFO(LOG_CAT_GENERAL, "Sent connection request to %s:%d", address,
             port);
    return 0;
  }

  socket_close(context->socket);
  context->is_running = false;
  return -1;
}

int network_client_disconnect(NetworkContext *context) {
  if (!context || !context->is_running ||
      context->mode != NETWORK_MODE_CLIENT) {
    return -1;
  }

  if (context->connected_to_server) {
    uint8_t buffer[PACKET_HEADER_SIZE + 4];
    PacketHeader header = {.type = PACKET_TYPE_DISCONNECT,
                           .flags = 0,
                           .sequence = context->clients[0].next_sequence++,
                           .timestamp =
                               (uint32_t)network_connection_get_timestamp_ms(),
                           .data_size = 4,
                           .checksum = 0};
    serialize_packet_header(buffer, &header);
    memcpy(&buffer[PACKET_HEADER_SIZE], &context->local_client_id, 4);

    socket_send(context->socket, &context->server_address, buffer,
                sizeof(buffer));
  }

  socket_close(context->socket);
  context->socket = NULL;
  context->is_running = false;
  context->connected_to_server = false;

  LOG_INFO(LOG_CAT_GENERAL, "Disconnected from server");
  return 0;
}

int network_client_send(NetworkContext *context, PacketType type,
                        const void *data, uint32_t size) {
  if (!context || !context->is_running ||
      context->mode != NETWORK_MODE_CLIENT || !context->connected_to_server) {
    return -1;
  }

  uint8_t buffer[MAX_PACKET_SIZE];
  PacketHeader header = {.type = type,
                         .flags = 0,
                         .sequence = context->clients[0].next_sequence++,
                         .timestamp =
                             (uint32_t)network_connection_get_timestamp_ms(),
                         .data_size = (uint16_t)size,
                         .checksum = 0};

  serialize_packet_header(buffer, &header);
  if (data && size > 0) {
    memcpy(&buffer[PACKET_HEADER_SIZE], data, size);
  }

  uint32_t total_size = PACKET_HEADER_SIZE + size;
  if (socket_send(context->socket, &context->server_address, buffer,
                  total_size)) {
    context->stats.packets_sent++;
    context->stats.bytes_sent += total_size;
    return 0;
  }

  return -1;
}

bool network_client_is_connected(NetworkContext *context) {
  if (!context)
    return false;
  return context->is_running && context->mode == NETWORK_MODE_CLIENT &&
         context->connected_to_server;
}

uint32_t network_client_get_id(NetworkContext *context) {
  if (!context)
    return 0;
  return context->local_client_id;
}
