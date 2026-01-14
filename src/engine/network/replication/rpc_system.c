#include "network/rpc_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RPC_HANDLERS 256

typedef struct rpc_entry {
  char name[64];
  RPCHandler handler;
} rpc_entry_t;

static rpc_entry_t g_rpc_registry[MAX_RPC_HANDLERS];
static uint32_t g_rpc_count = 0;

void rpc_register(const char *name, RPCHandler handler) {
  if (!name || !handler || g_rpc_count >= MAX_RPC_HANDLERS)
    return;

  // Check for existing
  for (uint32_t i = 0; i < g_rpc_count; i++) {
    if (strcmp(g_rpc_registry[i].name, name) == 0) {
      g_rpc_registry[i].handler = handler;
      return;
    }
  }

  strncpy(g_rpc_registry[g_rpc_count].name, name, 63);
  g_rpc_registry[g_rpc_count].handler = handler;
  g_rpc_count++;
}

void rpc_update(float delta_time) {
  // Stub
}

void rpc_send_reliable(const char *name, void *data, uint32_t size) {
  // Stub
}

uint32_t rpc_call(const char *name, Packet *params, RpcCallback callback, void *user_data) {
  return 0;
}

void rpc_reply(uint32_t target_id, uint32_t request_id, Packet *results) {
}

void rpc_broadcast(const char *name, void *data, uint32_t size) {
}

void rpc_broadcast_reliable(const char *name, void *data, uint32_t size) {
}

void rpc_send_to_client(uint32_t client_id, const char *name, void *data, uint32_t size) {
}

void rpc_send_to_client_reliable(uint32_t client_id, const char *name, void *data, uint32_t size) {
}

void rpc_process_packet(uint32_t sender_id, const void *packet_data, uint16_t packet_size) {
}

void rpc_params_init(Packet *params) {
    if (params) {
        // Simple init
        params->write_pos = 0;
        params->length = 0;
    }
}

void rpc_params_from_data(Packet *params, const void *data, uint32_t size) {
}

void rpc_init(void) {
    g_rpc_count = 0;
}

void rpc_shutdown(void) {
    g_rpc_count = 0;
}

bool rpc_dispatch(const char *name, uint32_t sender_id, uint32_t request_id, void *data,
                  uint32_t size) {
  if (!name)
    return false;

  for (uint32_t i = 0; i < g_rpc_count; i++) {
    if (strcmp(g_rpc_registry[i].name, name) == 0) {
      g_rpc_registry[i].handler(sender_id, request_id, data, size);
      return true;
    }
  }

  return false;
}

void rpc_send(const char *name, void *data, uint32_t size) {
  // In a real implementation:
  // 1. Pack RPC name, sender_id, and data into a packet
  // 2. Send packet through the transport layer (e.g. Reliable UDP)
}
