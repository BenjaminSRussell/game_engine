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

// Core RPC functions
void rpc_init(void);
void rpc_shutdown(void);
void rpc_register(const char *name, RPCHandler handler);
bool rpc_dispatch(const char *name, uint32_t sender_id, void *data, uint32_t size);

// Client-side functions
void rpc_send(const char *name, void *data, uint32_t size);

// Server-side functions
void rpc_broadcast(const char *name, void *data, uint32_t size);
void rpc_send_to_client(uint32_t client_id, const char *name, void *data, uint32_t size);

// Packet processing
void rpc_process_packet(uint32_t sender_id, const void *packet_data, uint16_t packet_size);

#endif // RPC_SYSTEM_H
