#pragma once

#include "core/core.h"
#include "engine/include/core/memory.h"

typedef struct RPCSystem RPCSystem;

// RPC parameter types
typedef enum {
    RPC_TYPE_BOOL = 1,
    RPC_TYPE_U8 = 2,
    RPC_TYPE_U16 = 3,
    RPC_TYPE_U32 = 4,
    RPC_TYPE_U64 = 5,
    RPC_TYPE_S8 = 6,
    RPC_TYPE_S16 = 7,
    RPC_TYPE_S32 = 8,
    RPC_TYPE_S64 = 9,
    RPC_TYPE_F32 = 10,
    RPC_TYPE_F64 = 11,
    RPC_TYPE_STRING = 12,
    RPC_TYPE_VECTOR3 = 13,
    RPC_TYPE_ENTITY_ID = 14
} RPCParamType;

// System Management
RPCSystem* rpc_system_create(void);
void rpc_system_destroy(RPCSystem* system);

// RPC Registration
bool rpc_register(RPCSystem* system, const char* name, void* function_ptr, 
                  const u8* param_types, u8 param_count, bool server_to_client, 
                  bool client_to_server, bool multicast, u8 reliability_mode, 
                  u32 rate_limit_ms, u8 priority);

// RPC Calls
bool rpc_call_server(RPCSystem* system, u32 rpc_id, const void* params, u32 param_size, u32 sender_client_id);
bool rpc_call_client(RPCSystem* system, u32 rpc_id, const void* params, u32 param_size, u32 target_client_id);
bool rpc_call_multicast(RPCSystem* system, u32 rpc_id, const void* params, u32 param_size);

// Queue Processing
void rpc_process_queue(RPCSystem* system);

// Packet Generation/Processing
u32 rpc_generate_call_packet(RPCSystem* system, void* buffer, u32 buffer_size);
bool rpc_process_call_packet(RPCSystem* system, const void* buffer, u32 buffer_size);

// Statistics
void rpc_get_stats(const RPCSystem* system, u32* registered, u32* called, u32* executed, 
                   u32* failed, u32* rate_limited, f32* avg_time_ms);
