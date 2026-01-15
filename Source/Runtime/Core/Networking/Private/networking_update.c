#include "networking_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

extern int socket_receive(NetSocket *sock, NetAddress *from, void *buffer,
                          uint32_t buffer_size);
extern NetworkClient *
network_connection_find_by_address(NetworkContext *ctx, const NetAddress *addr);
extern NetworkClient *network_connection_find_free(NetworkContext *ctx);
extern bool network_connection_enqueue_packet(NetworkClient *client,
                                              const NetworkPacket *packet);
extern void network_connection_update_activity(NetworkClient *client);
extern uint64_t network_connection_get_timestamp_ms(void);

extern void network_replication_server_update(NetworkContext *ctx);
extern void network_replication_process_snapshot(NetworkContext *ctx,
                                                 const uint8_t *packet_data,
                                                 uint32_t packet_size);

extern int network_server_send_to_client(NetworkContext *context,
                                         uint32_t client_id, PacketType type,
                                         const void *data, uint32_t size);
extern int network_server_broadcast(NetworkContext *context, PacketType type,
                                    const void *data, uint32_t size);
extern int network_client_send(NetworkContext *context, PacketType type,
                               const void *data, uint32_t size);

/* ============================================================================
 * PACKET HANDLING
 * ============================================================================
 */

static void deserialize_packet_header(const uint8_t *buffer,
                                      PacketHeader *header) {
  memcpy(&header->type, &buffer[0], 2);
  memcpy(&header->flags, &buffer[2], 1);
  memcpy(&header->sequence, &buffer[3], 2);
  memcpy(&header->timestamp, &buffer[5], 4);
  memcpy(&header->data_size, &buffer[9], 2);
  memcpy(&header->checksum, &buffer[11], 2);
}

static void handle_server_packet(NetworkContext *ctx, const NetAddress *from,
                                 const uint8_t *data, uint32_t size) {
  if (size < PACKET_HEADER_SIZE)
    return;

  PacketHeader header;
  deserialize_packet_header(data, &header);

  switch (header.type) {
  case PACKET_TYPE_CONNECT: {
    // Find free client slot
    NetworkClient *client = network_connection_find_free(ctx);
    if (!client) {
      LOG_WARN(LOG_CAT_GENERAL,
               "No free client slots for connection from %u:%u", from->host,
               from->port);
      return;
    }

    client->address = *from;
    client->client_id = ctx->next_client_id++;

    // Extract username from packet data
    const char *username = (const char *)&data[PACKET_HEADER_SIZE];
    strncpy(client->username, username, sizeof(client->username) - 1);
    client->username[sizeof(client->username) - 1] = '\0';

    client->connected = true;
    client->state = CONNECTION_STATE_CONNECTED;
    clock_gettime(CLOCK_MONOTONIC, &client->connect_time);
    network_connection_update_activity(client);
    ctx->client_count++;

    // Send auth response
    uint8_t response[32];
    memcpy(&response[0], &client->client_id, 4);
    uint8_t success = 1;
    response[4] = success;
    strcpy((char *)&response[5], "Connected");

    network_server_send_to_client(ctx, client->client_id,
                                  PACKET_TYPE_AUTH_RESPONSE, response,
                                  strlen("Connected") + 6);

    LOG_INFO(LOG_CAT_GENERAL, "Client connected: %s (ID: %u)", client->username,
             client->client_id);
    break;
  }

  case PACKET_TYPE_DISCONNECT: {
    NetworkClient *client = network_connection_find_by_address(ctx, from);
    if (client) {
      LOG_INFO(LOG_CAT_GENERAL, "Client disconnected: %s (ID: %u)",
               client->username, client->client_id);
      client->connected = false;
      client->state = CONNECTION_STATE_DISCONNECTED;
      ctx->client_count--;
    }
    break;
  }

  case PACKET_TYPE_HEARTBEAT: {
    NetworkClient *client = network_connection_find_by_address(ctx, from);
    if (client) {
      network_connection_update_activity(client);
    }
    break;
  }

  default:
    break;
  }
}

static void handle_client_packet(NetworkContext *ctx, const NetAddress *from,
                                 const uint8_t *data, uint32_t size) {
  (void)from;
  if (size < PACKET_HEADER_SIZE)
    return;

  PacketHeader header;
  deserialize_packet_header(data, &header);

  switch (header.type) {
  case PACKET_TYPE_AUTH_RESPONSE: {
    if (size >= PACKET_HEADER_SIZE + 5) {
      uint32_t client_id;
      memcpy(&client_id, &data[PACKET_HEADER_SIZE], 4);
      uint8_t success = data[PACKET_HEADER_SIZE + 4];

      if (success) {
        ctx->local_client_id = client_id;
        ctx->connected_to_server = true;
        ctx->clients[0].state = CONNECTION_STATE_CONNECTED;
        LOG_INFO(LOG_CAT_GENERAL, "Connected to server (ID: %u)", client_id);
      } else {
        LOG_ERROR(LOG_CAT_GENERAL, "Connection rejected by server");
      }
    }
    break;
  }

  case PACKET_TYPE_SNAPSHOT: {
    network_replication_process_snapshot(ctx, &data[PACKET_HEADER_SIZE],
                                         size - PACKET_HEADER_SIZE);
    break;
  }

  case PACKET_TYPE_DISCONNECT: {
    ctx->connected_to_server = false;
    ctx->clients[0].state = CONNECTION_STATE_DISCONNECTED;
    LOG_INFO(LOG_CAT_GENERAL, "Server disconnected");
    break;
  }

  case PACKET_TYPE_HEARTBEAT: {
    network_connection_update_activity(&ctx->clients[0]);
    break;
  }

  default:
    break;
  }
}

/* ============================================================================
 * MAIN UPDATE LOOP
 * ============================================================================
 */

int network_update(NetworkContext *context, float delta_time) {
  if (!context || !context->is_running || !context->socket) {
    return -1;
  }

  uint8_t buffer[MAX_PACKET_SIZE];
  NetAddress from;
  int bytes_received;

  // Process incoming packets
  while ((bytes_received = socket_receive(context->socket, &from, buffer,
                                          sizeof(buffer))) > 0) {
    context->stats.packets_received++;
    context->stats.bytes_received += bytes_received;

    if (context->mode == NETWORK_MODE_SERVER) {
      handle_server_packet(context, &from, buffer, bytes_received);
    } else {
      handle_client_packet(context, &from, buffer, bytes_received);
    }
  }

  // Handle heartbeats
  context->last_heartbeat_sent += delta_time;
  if (context->last_heartbeat_sent >= HEARTBEAT_INTERVAL_SEC) {
    if (context->mode == NETWORK_MODE_SERVER) {
      network_server_broadcast(context, PACKET_TYPE_HEARTBEAT, NULL, 0);
    } else if (context->connected_to_server) {
      network_client_send(context, PACKET_TYPE_HEARTBEAT, NULL, 0);
    }
    context->last_heartbeat_sent = 0.0f;
  }

  // Handle replication (server-side)
  if (context->mode == NETWORK_MODE_SERVER) {
    context->replication_timer += delta_time;
    float replication_interval = 1.0f / context->config.replication_rate_hz;

    if (context->replication_timer >= replication_interval) {
      network_replication_server_update(context);
      context->replication_timer = 0.0f;
    }
  }

  return 0;
}
