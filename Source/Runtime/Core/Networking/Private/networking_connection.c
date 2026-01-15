#include "networking_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>
#include <time.h>

/* ============================================================================
 * CONNECTION MANAGEMENT
 * Handles packet queuing and connection state
 * ============================================================================
 */

static bool enqueue_received_packet(NetworkClient *client,
                                    const NetworkPacket *packet) {
  if (!client || !packet)
    return false;

  pthread_mutex_lock(&client->lock);

  uint32_t next_tail = (client->recv_queue_tail + 1) % MAX_PACKET_QUEUE;
  if (next_tail == client->recv_queue_head) {
    pthread_mutex_unlock(&client->lock);
    return false; // Queue full
  }

  // Copy packet
  NetworkPacket *queued = &client->recv_queue[client->recv_queue_tail];
  queued->type = packet->type;
  queued->delivery_type = packet->delivery_type;
  queued->sequence_number = packet->sequence_number;
  queued->timestamp = packet->timestamp;
  queued->size = packet->size;

  if (packet->data && packet->size > 0) {
    queued->data = UNIFIED_ALLOC(packet->size);
    if (!queued->data) {
      pthread_mutex_unlock(&client->lock);
      return false;
    }
    memcpy(queued->data, packet->data, packet->size);
  } else {
    queued->data = NULL;
  }

  client->recv_queue_tail = next_tail;
  client->packets_received++;

  pthread_mutex_unlock(&client->lock);
  return true;
}

static bool dequeue_received_packet(NetworkClient *client,
                                    NetworkPacket *packet) {
  if (!client || !packet)
    return false;

  pthread_mutex_lock(&client->lock);

  if (client->recv_queue_head == client->recv_queue_tail) {
    pthread_mutex_unlock(&client->lock);
    return false; // Queue empty
  }

  *packet = client->recv_queue[client->recv_queue_head];
  client->recv_queue_head = (client->recv_queue_head + 1) % MAX_PACKET_QUEUE;

  pthread_mutex_unlock(&client->lock);
  return true;
}

void connection_free_packet(NetworkPacket *packet) {
  if (!packet)
    return;
  if (packet->data) {
    UNIFIED_FREE(packet->data);
    packet->data = NULL;
  }
}

static void connection_init_client(NetworkClient *client) {
  if (!client)
    return;
  memset(client, 0, sizeof(NetworkClient));
  client->state = CONNECTION_STATE_DISCONNECTED;
}

static NetworkClient *connection_find_free_slot(NetworkContext *ctx) {
  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    if (!ctx->clients[i].connected &&
        ctx->clients[i].state == CONNECTION_STATE_DISCONNECTED) {
      return &ctx->clients[i];
    }
  }
  return NULL;
}

static NetworkClient *connection_find_by_id(NetworkContext *ctx,
                                            uint32_t client_id) {
  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    if (ctx->clients[i].connected && ctx->clients[i].client_id == client_id) {
      return &ctx->clients[i];
    }
  }
  return NULL;
}

static NetworkClient *connection_find_by_address(NetworkContext *ctx,
                                                 const NetAddress *addr) {
  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    if (ctx->clients[i].connected &&
        ctx->clients[i].address.host == addr->host &&
        ctx->clients[i].address.port == addr->port) {
      return &ctx->clients[i];
    }
  }
  return NULL;
}

static uint64_t get_timestamp_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static void connection_update_activity(NetworkClient *client) {
  if (!client)
    return;
  clock_gettime(CLOCK_MONOTONIC, &client->last_activity_time);
}

// Export symbols for use in other files
bool network_connection_enqueue_packet(NetworkClient *client,
                                       const NetworkPacket *packet) {
  return enqueue_received_packet(client, packet);
}

bool network_connection_dequeue_packet(NetworkClient *client,
                                       NetworkPacket *packet) {
  return dequeue_received_packet(client, packet);
}

void network_connection_init(NetworkClient *client) {
  connection_init_client(client);
}

NetworkClient *network_connection_find_free(NetworkContext *ctx) {
  return connection_find_free_slot(ctx);
}

NetworkClient *network_connection_find_by_id(NetworkContext *ctx,
                                             uint32_t client_id) {
  return connection_find_by_id(ctx, client_id);
}

NetworkClient *network_connection_find_by_address(NetworkContext *ctx,
                                                  const NetAddress *addr) {
  return connection_find_by_address(ctx, addr);
}

uint64_t network_connection_get_timestamp_ms(void) {
  return get_timestamp_ms();
}

void network_connection_update_activity(NetworkClient *client) {
  connection_update_activity(client);
}
