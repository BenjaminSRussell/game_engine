// network/replication/net_replication.h
// Network replication system (Unreal-style property replication)
#ifndef NET_REPLICATION_H
#define NET_REPLICATION_H

#include <common.h>
#include "math/vec3.h"

#define NET_MAX_REPLICATED_OBJECTS 10000
#define NET_MAX_PROPERTIES_PER_OBJECT 64
#define NET_MAX_CLIENTS 64

// Replication flags
typedef enum {
  REPLICATE_ALWAYS = 1 << 0,
  REPLICATE_OWNER_ONLY = 1 << 1,
  REPLICATE_SIMULATED = 1 << 2,
  REPLICATE_INITIAL_ONLY = 1 << 3
} ReplicationFlags;

// Property type
typedef enum {
  NET_PROP_BOOL,
  NET_PROP_INT,
  NET_PROP_FLOAT,
  NET_PROP_VEC3,
  NET_PROP_QUAT,
  NET_PROP_STRING
} NetPropertyType;

// Replicated property
typedef struct {
  char name[64];
  NetPropertyType type;
  u32 offset; // Offset in struct
  ReplicationFlags flags;

  // Condition (when to replicate)
  bool (*condition)(void *object);

  // Delta compression
  void *last_value;
  bool dirty;
} NetProperty;

// Replicated object
typedef struct {
  u32 net_id;
  u32 owner_client_id;
  void *object_ptr;

  NetProperty properties[NET_MAX_PROPERTIES_PER_OBJECT];
  u32 property_count;

  bool is_relevant[NET_MAX_CLIENTS]; // Per-client relevancy
  f32 priority[NET_MAX_CLIENTS];     // Replication priority

  // RPC support
  void (*rpc_handlers[16])(void *object, void *data);
  u32 rpc_count;

} ReplicatedObject;

// Network authority
typedef enum {
  NET_AUTHORITY_SERVER,
  NET_AUTHORITY_CLIENT_PREDICTED,
  NET_AUTHORITY_CLIENT_SIMULATED
} NetAuthority;

typedef struct {
  ReplicatedObject objects[NET_MAX_REPLICATED_OBJECTS];
  u32 object_count;

  NetAuthority authority;
  u32 local_client_id;

  // Bandwidth management
  u32 bytes_per_second_limit;
  u32 bytes_sent_this_second;
  f32 second_accumulator;

  // Delta compression
  bool enable_delta_compression;

} NetReplicationSystem;

#ifdef __cplusplus
extern "C" {
#endif

// System lifecycle
NetReplicationSystem *net_replication_create(NetAuthority authority,
                                             u32 client_id);
void net_replication_destroy(NetReplicationSystem *net);

// Object registration
u32 net_register_object(NetReplicationSystem *net, void *object,
                        u32 owner_client_id);
void net_unregister_object(NetReplicationSystem *net, u32 net_id);

// Property setup
void net_add_property(NetReplicationSystem *net, u32 net_id, const char *name,
                      NetPropertyType type, u32 offset, ReplicationFlags flags);

// RPC (Remote Procedure Call)
void net_register_rpc(NetReplicationSystem *net, u32 net_id,
                      const char *rpc_name,
                      void (*handler)(void *object, void *data));

void net_call_rpc(NetReplicationSystem *net, u32 net_id, const char *rpc_name,
                  void *data, u32 data_size, bool reliable, u32 *target_clients,
                  u32 client_count);

// Relevancy (which objects each client should receive)
void net_update_relevancy(NetReplicationSystem *net, u32 net_id, u32 client_id,
                          bool relevant);
void net_set_priority(NetReplicationSystem *net, u32 net_id, u32 client_id,
                      f32 priority);

// Per-frame replication
void net_replicate_objects(NetReplicationSystem *net, f32 delta_time,
                           void *(*get_packet_buffer)(u32 size),
                           void (*send_packet)(u32 client_id, void *data,
                                               u32 size));

// Receive replication data
void net_receive_replication(NetReplicationSystem *net, void *data, u32 size);

#ifdef __cplusplus
}
#endif

#endif // NET_REPLICATION_H
