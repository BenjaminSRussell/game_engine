#include "networking_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

/* ============================================================================
 * FORWARD DECLARATIONS FROM OTHER FILES
 * ============================================================================
 */

extern NetSocket *socket_create(uint16_t port);
extern void socket_close(NetSocket *sock);
extern bool socket_send(NetSocket *sock, const NetAddress *addr,
                        const void *data, uint32_t size);
extern int socket_receive(NetSocket *sock, NetAddress *from, void *buffer,
                          uint32_t buffer_size);

/* ============================================================================
 * CORE IMPLEMENTATION
 * ============================================================================
 */

NetworkConfig network_get_default_config(void) {
  NetworkConfig config = {.max_connections = 32,
                          .max_packet_size = MAX_PACKET_SIZE,
                          .timeout_ms =
                              (uint32_t)(CONNECTION_TIMEOUT_SEC * 1000),
                          .enable_compression = true,
                          .replication_rate_hz = 20,
                          .relevancy_distance = 1000.0f};
  return config;
}

NetworkContext *network_init(NetworkMode mode, const NetworkConfig *config) {
  if (mode == NETWORK_MODE_NONE) {
    LOG_ERROR(LOG_CAT_GENERAL, "Invalid network mode");
    return NULL;
  }

  NetworkContext *ctx = UNIFIED_ALLOC(sizeof(NetworkContext));
  if (!ctx) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate network context");
    return NULL;
  }
  memset(ctx, 0, sizeof(NetworkContext));

  ctx->mode = mode;
  ctx->is_running = false;

  // Apply configuration
  if (config) {
    ctx->config = *config;
  } else {
    ctx->config = network_get_default_config();
  }

  // Initialize mutexes
  pthread_mutex_init(&ctx->lock, NULL);

  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    pthread_mutex_init(&ctx->clients[i].lock, NULL);
  }

  LOG_INFO(LOG_CAT_GENERAL, "Network context initialized (mode: %s)",
           mode == NETWORK_MODE_SERVER ? "SERVER" : "CLIENT");

  return ctx;
}

void network_shutdown(NetworkContext *context) {
  if (!context)
    return;

  LOG_INFO(LOG_CAT_GENERAL, "Shutting down network context");

  // Close socket if open
  if (context->socket) {
    socket_close(context->socket);
    context->socket = NULL;
  }

  // Destroy mutexes
  pthread_mutex_destroy(&context->lock);

  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    pthread_mutex_destroy(&context->clients[i].lock);
  }

  UNIFIED_FREE(context);
}

int network_get_stats(NetworkContext *context, NetworkStats *stats) {
  if (!context || !stats)
    return -1;

  pthread_mutex_lock(&context->lock);
  *stats = context->stats;
  pthread_mutex_unlock(&context->lock);

  return 0;
}

bool network_parse_address(const char *address_str, uint16_t port,
                           NetAddress *out_address) {
  if (!address_str || !out_address)
    return false;

  uint32_t addr = inet_addr(address_str);
  if (addr == INADDR_NONE) {
    return false;
  }

  out_address->host = addr;
  out_address->port = port;
  return true;
}

const char *network_get_error(void) {
  extern const char *socket_get_error(void);
  return socket_get_error();
}
