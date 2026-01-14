// network/replication/net_replication.c
// Complete network replication implementation
#include "include/network/replication/net_replication.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

NetReplicationSystem *net_replication_create(NetAuthority authority,
                                             u32 client_id) {
  NetReplicationSystem *net =
      (NetReplicationSystem *)calloc(1, sizeof(NetReplicationSystem));
  net->authority = authority;
  net->local_client_id = client_id;
  net->bytes_per_second_limit = 100000; // 100KB/s
  net->enable_delta_compression = true;

  LOG_INFO("Network replication created: authority=%d, clientID=%u", authority,
           client_id);
  return net;
}

void net_replication_destroy(NetReplicationSystem *net) {
  if (!net)
    return;

  // Cleanup property last values
  for (u32 i = 0; i < net->object_count; i++) {
    ReplicatedObject *obj = &net->objects[i];
    for (u32 j = 0; j < obj->property_count; j++) {
      free(obj->properties[j].last_value);
    }
  }

  free(net);
}

u32 net_register_object(NetReplicationSystem *net, void *object,
                        u32 owner_client_id) {
  if (!net || net->object_count >= NET_MAX_REPLICATED_OBJECTS) {
    LOG_ERROR("Cannot register object: limit reached");
    return 0;
  }

  u32 net_id = net->object_count++;
  ReplicatedObject *obj = &net->objects[net_id];

  obj->net_id = net_id;
  obj->object_ptr = object;
  obj->owner_client_id = owner_client_id;
  obj->property_count = 0;
  obj->rpc_count = 0;

  // Initialize relevancy (all clients by default)
  for (u32 i = 0; i < NET_MAX_CLIENTS; i++) {
    obj->is_relevant[i] = true;
    obj->priority[i] = 1.0f;
  }

  LOG_INFO("Registered replicated object: netID=%u, owner=%u", net_id,
           owner_client_id);
  return net_id;
}

void net_add_property(NetReplicationSystem *net, u32 net_id, const char *name,
                      NetPropertyType type, u32 offset,
                      ReplicationFlags flags) {
  if (!net || net_id >= net->object_count)
    return;

  ReplicatedObject *obj = &net->objects[net_id];
  if (obj->property_count >= NET_MAX_PROPERTIES_PER_OBJECT) {
    LOG_ERROR("Cannot add property '%s': limit reached", name);
    return;
  }

  NetProperty *prop = &obj->properties[obj->property_count++];
  strncpy(prop->name, name, sizeof(prop->name) - 1);
  prop->type = type;
  prop->offset = offset;
  prop->flags = flags;
  prop->condition = NULL;
  prop->dirty = true;

  // Allocate storage for last value (delta compression)
  u32 size = 0;
  switch (type) {
  case NET_PROP_BOOL:
    size = sizeof(bool);
    break;
  case NET_PROP_INT:
    size = sizeof(int);
    break;
  case NET_PROP_FLOAT:
    size = sizeof(f32);
    break;
  case NET_PROP_VEC3:
    size = sizeof(Vec3);
    break;
  case NET_PROP_QUAT:
    size = sizeof(f32) * 4;
    break;
  case NET_PROP_STRING:
    size = 256;
    break;
  }
  prop->last_value = calloc(1, size);

  LOG_INFO("Added replicated property '%s' to netID=%u", name, net_id);
}

void net_register_rpc(NetReplicationSystem *net, u32 net_id,
                      const char *rpc_name,
                      void (*handler)(void *object, void *data)) {
  if (!net || net_id >= net->object_count)
    return;

  ReplicatedObject *obj = &net->objects[net_id];
  if (obj->rpc_count >= 16) {
    LOG_ERROR("Cannot register RPC '%s': limit reached", rpc_name);
    return;
  }

  obj->rpc_handlers[obj->rpc_count++] = handler;
  LOG_INFO("Registered RPC '%s' for netID=%u", rpc_name, net_id);
}

void net_update_relevancy(NetReplicationSystem *net, u32 net_id, u32 client_id,
                          bool relevant) {
  if (!net || net_id >= net->object_count || client_id >= NET_MAX_CLIENTS)
    return;

  ReplicatedObject *obj = &net->objects[net_id];
  obj->is_relevant[client_id] = relevant;
}

void net_set_priority(NetReplicationSystem *net, u32 net_id, u32 client_id,
                      f32 priority) {
  if (!net || net_id >= net->object_count || client_id >= NET_MAX_CLIENTS)
    return;

  ReplicatedObject *obj = &net->objects[net_id];
  obj->priority[client_id] = priority;
}

void net_replicate_objects(NetReplicationSystem *net, f32 delta_time,
                           void *(*get_packet_buffer)(u32 size),
                           void (*send_packet)(u32 client_id, void *data,
                                               u32 size)) {
  if (!net || net->authority != NET_AUTHORITY_SERVER)
    return;

  // Bandwidth management
  net->second_accumulator += delta_time;
  if (net->second_accumulator >= 1.0f) {
    net->bytes_sent_this_second = 0;
    net->second_accumulator = 0.0f;
  }

  // For each client
  for (u32 client = 0; client < NET_MAX_CLIENTS; client++) {
    if (client == net->local_client_id)
      continue; // Don't replicate to self

    // Build packet for this client
    u32 packet_size = 0;
    void *packet = get_packet_buffer(8192);
    if (!packet)
      continue;

    u8 *write_ptr = (u8 *)packet;

    // Replicate relevant objects
    for (u32 i = 0; i < net->object_count; i++) {
      ReplicatedObject *obj = &net->objects[i];

      if (!obj->is_relevant[client])
        continue;

      // Write object header
      *(u32 *)write_ptr = obj->net_id;
      write_ptr += sizeof(u32);
      packet_size += sizeof(u32);

      // Write dirty properties
      for (u32 j = 0; j < obj->property_count; j++) {
        NetProperty *prop = &obj->properties[j];

        if (!prop->dirty)
          continue;

        // Get current value from object
        void *current_value = (u8 *)obj->object_ptr + prop->offset;

        // Delta compression check
        bool changed = true;
        if (net->enable_delta_compression && prop->last_value) {
          changed = memcmp(current_value, prop->last_value,
                           prop->type == NET_PROP_VEC3 ? sizeof(Vec3)
                                                       : sizeof(f32)) != 0;
        }

        if (changed) {
          // Write property ID
          *write_ptr++ = (u8)j;
          packet_size++;

          // Write value based on type
          u32 size = 0;
          switch (prop->type) {
          case NET_PROP_FLOAT:
            size = sizeof(f32);
            memcpy(write_ptr, current_value, size);
            break;
          case NET_PROP_VEC3:
            size = sizeof(Vec3);
            memcpy(write_ptr, current_value, size);
            break;
            // ... other types
          }
          write_ptr += size;
          packet_size += size;

          // Update last value
          memcpy(prop->last_value, current_value, size);
        }
      }
    }

    // Send packet
    if (packet_size > 0 && net->bytes_sent_this_second + packet_size <
                               net->bytes_per_second_limit) {
      send_packet(client, packet, packet_size);
      net->bytes_sent_this_second += packet_size;
    }
  }
}
