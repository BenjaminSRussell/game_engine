/**
 * =================================================================================================
 *                          RPC SYSTEM
 *                          Phase 5: Networking & Multiplayer
 * =================================================================================================
 *
 * PURPOSE: Remote Procedure Call registration and dispatching
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MAX_RPCS 256
#define MAX_NAME_LEN 64

// RPC Handler function pointer
typedef void (*RPCHandler)(uint32_t sender_id, void *data, uint32_t size);

typedef struct {
  char name[MAX_NAME_LEN];
  RPCHandler handler;
} RPCEntry;

static RPCEntry rpc_table[MAX_RPCS];
static int rpc_count = 0;

void rpc_register(const char *name, RPCHandler handler) {
  if (rpc_count >= MAX_RPCS)
    return;

  // Check duplicate
  for (int i = 0; i < rpc_count; i++) {
    if (strcmp(rpc_table[i].name, name) == 0)
      return;
  }

  strncpy(rpc_table[rpc_count].name, name, MAX_NAME_LEN - 1);
  rpc_table[rpc_count].handler = handler;
  rpc_count++;
}

bool rpc_dispatch(const char *name, uint32_t sender_id, void *data,
                  uint32_t size) {
  for (int i = 0; i < rpc_count; i++) {
    if (strcmp(rpc_table[i].name, name) == 0) {
      if (rpc_table[i].handler) {
        rpc_table[i].handler(sender_id, data, size);
        return true;
      }
    }
  }
  return false;
}

// Serialize RPC call (Stub)
// Would write [RPC_ID][DATA_LEN][DATA] to packet
void rpc_send(const char *name, void *data, uint32_t size) {
  // 1. Find ID for name
  // 2. Write packet
  // 3. Send via network transport
}
