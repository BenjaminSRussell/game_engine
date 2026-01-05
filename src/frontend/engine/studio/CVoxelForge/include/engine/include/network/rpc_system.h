/**
 * =================================================================================================
 *                          RPC SYSTEM HEADER
 *                          Phase 5: Networking
 * =================================================================================================
 */

#ifndef RPC_SYSTEM_H
#define RPC_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*RPCHandler)(uint32_t sender_id, void *data, uint32_t size);

void rpc_register(const char *name, RPCHandler handler);
bool rpc_dispatch(const char *name, uint32_t sender_id, void *data,
                  uint32_t size);
void rpc_send(const char *name, void *data, uint32_t size);

#endif // RPC_SYSTEM_H
