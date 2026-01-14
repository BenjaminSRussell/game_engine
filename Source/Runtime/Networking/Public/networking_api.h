/**
 * @file networking_api.h
 * @brief Networking subsystem API
 */

#ifndef VOXELFORGE_NETWORKING_API_H
#define VOXELFORGE_NETWORKING_API_H

#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct NetConnection NetConnection;
typedef struct NetServer NetServer;
typedef struct NetClient NetClient;
typedef struct NetChannel NetChannel;
typedef struct NetPacket NetPacket;

// ============================================================================
// Types
// ============================================================================

typedef u32 NetClientId;
#define NET_CLIENT_INVALID ((NetClientId)0xFFFFFFFF)

typedef enum NetConnectionState {
  NET_STATE_DISCONNECTED = 0,
  NET_STATE_CONNECTING,
  NET_STATE_CONNECTED,
  NET_STATE_DISCONNECTING,
} NetConnectionState;

typedef enum NetChannelType {
  NET_CHANNEL_UNRELIABLE = 0,       // UDP-like
  NET_CHANNEL_UNRELIABLE_SEQUENCED, // Drop out-of-order
  NET_CHANNEL_RELIABLE,             // Guaranteed delivery
  NET_CHANNEL_RELIABLE_ORDERED,     // Guaranteed + in-order
} NetChannelType;

// ============================================================================
// Callbacks
// ============================================================================

typedef void (*NetOnConnectFn)(NetConnection *conn, void *user_data);
typedef void (*NetOnDisconnectFn)(NetConnection *conn, void *user_data);
typedef void (*NetOnReceiveFn)(NetConnection *conn, const void *data,
                               usize size, u8 channel, void *user_data);

typedef struct NetCallbacks {
  NetOnConnectFn on_connect;
  NetOnDisconnectFn on_disconnect;
  NetOnReceiveFn on_receive;
  void *user_data;
} NetCallbacks;

// ============================================================================
// Server
// ============================================================================

typedef struct NetServerConfig {
  u16 port;
  u32 max_clients;
  u32 connection_timeout_ms;
  NetCallbacks callbacks;
} NetServerConfig;

VF_API NetServer *net_server_create(const NetServerConfig *config);
VF_API void net_server_destroy(NetServer *server);
VF_API void net_server_update(NetServer *server);
VF_API void net_server_broadcast(NetServer *server, const void *data,
                                 usize size, u8 channel);
VF_API void net_server_send(NetServer *server, NetClientId client,
                            const void *data, usize size, u8 channel);
VF_API void net_server_disconnect_client(NetServer *server, NetClientId client);
VF_API u32 net_server_get_client_count(NetServer *server);
VF_API void net_server_get_clients(NetServer *server, NetClientId *out_clients,
                                   u32 max_clients);

// ============================================================================
// Client
// ============================================================================

typedef struct NetClientConfig {
  const char *server_address;
  u16 server_port;
  u32 connection_timeout_ms;
  NetCallbacks callbacks;
} NetClientConfig;

VF_API NetClient *net_client_create(const NetClientConfig *config);
VF_API void net_client_destroy(NetClient *client);
VF_API VF_Result net_client_connect(NetClient *client);
VF_API void net_client_disconnect(NetClient *client);
VF_API void net_client_update(NetClient *client);
VF_API void net_client_send(NetClient *client, const void *data, usize size,
                            u8 channel);
VF_API NetConnectionState net_client_get_state(NetClient *client);
VF_API u32 net_client_get_ping(NetClient *client);

// ============================================================================
// Replication
// ============================================================================

typedef struct NetReplicatedObject NetReplicatedObject;

typedef void (*NetSerializeFn)(void *object, void *buffer, usize *size);
typedef void (*NetDeserializeFn)(void *object, const void *buffer, usize size);

typedef struct NetReplicationConfig {
  u32 object_id;
  void *object;
  NetSerializeFn serialize;
  NetDeserializeFn deserialize;
  f32 update_rate; // Updates per second
  b8 reliable;
} NetReplicationConfig;

VF_API NetReplicatedObject *
net_replicate_object(NetServer *server, const NetReplicationConfig *config);
VF_API void net_unreplicate_object(NetReplicatedObject *obj);
VF_API void net_mark_dirty(NetReplicatedObject *obj);

// ============================================================================
// RPC
// ============================================================================

typedef void (*NetRPCFn)(const void *args, usize args_size, void *user_data);

VF_API void net_register_rpc(const char *name, NetRPCFn fn, void *user_data);
VF_API void net_call_rpc_server(NetClient *client, const char *name,
                                const void *args, usize args_size);
VF_API void net_call_rpc_client(NetServer *server, NetClientId client,
                                const char *name, const void *args,
                                usize args_size);
VF_API void net_call_rpc_broadcast(NetServer *server, const char *name,
                                   const void *args, usize args_size);

// ============================================================================
// Statistics
// ============================================================================

typedef struct NetStats {
  u64 bytes_sent;
  u64 bytes_received;
  u64 packets_sent;
  u64 packets_received;
  u64 packets_lost;
  f32 packet_loss_percent;
  u32 rtt_ms;
  u32 jitter_ms;
} NetStats;

VF_API void net_get_stats(NetConnection *conn, NetStats *out_stats);

// ============================================================================
// System
// ============================================================================

VF_API VF_Result networking_init(void);
VF_API void networking_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_NETWORKING_API_H
