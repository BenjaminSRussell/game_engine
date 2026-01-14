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
#include "include/network/packet.h"

// RPC Flags
#define RPC_FLAG_RELIABLE (1 << 0)
#define RPC_FLAG_RESPONSE (1 << 1)

// Handler signature
// request_id: if non-zero, the caller expects a response via rpc_reply
typedef void (*RPCHandler)(uint32_t sender_id, uint32_t request_id, void *data, uint32_t size);

// Callback for RPC responses
// response: Packet containing the return values
typedef void (*RpcCallback)(uint32_t request_id, Packet *response, void *user_data);

// Core RPC functions
void rpc_init(void);
void rpc_shutdown(void);
void rpc_update(float delta_time);

void rpc_register(const char *name, RPCHandler handler);
bool rpc_dispatch(const char *name, uint32_t sender_id, uint32_t request_id, void *data, uint32_t size);

// Client-side functions
void rpc_send(const char *name, void *data, uint32_t size); // Unreliable, fire-and-forget
void rpc_send_reliable(const char *name, void *data, uint32_t size); // Reliable, fire-and-forget

// Advanced RPC calls
// Returns request_id used for the call
uint32_t rpc_call(const char *name, Packet *params, RpcCallback callback, void *user_data);
// target_id: The ID of the client to reply to (0 for server if we are client, or client_id if we are server)
void rpc_reply(uint32_t target_id, uint32_t request_id, Packet *results);

// Server-side functions
void rpc_broadcast(const char *name, void *data, uint32_t size);
void rpc_broadcast_reliable(const char *name, void *data, uint32_t size);
void rpc_send_to_client(uint32_t client_id, const char *name, void *data, uint32_t size);
void rpc_send_to_client_reliable(uint32_t client_id, const char *name, void *data, uint32_t size);

// Packet processing
void rpc_process_packet(uint32_t sender_id, const void *packet_data, uint16_t packet_size);

// Helper to initialize a packet for RPC parameters
void rpc_params_init(Packet *params);

// Helper to prepare a packet for reading RPC parameters from data received
void rpc_params_from_data(Packet *params, const void *data, uint32_t size);

#endif // RPC_SYSTEM_H
