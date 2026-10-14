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

static u32 net_property_wire_size(NetPropertyType type) {
  switch (type) {
  case NET_PROP_BOOL:
    return (u32)sizeof(bool);
  case NET_PROP_INT:
    return (u32)sizeof(int);
  case NET_PROP_FLOAT:
    return (u32)sizeof(f32);
  case NET_PROP_VEC3:
    return (u32)sizeof(Vec3);
  case NET_PROP_QUAT:
    return (u32)sizeof(f32) * 4;
  case NET_PROP_STRING:
    return 256;
  default:
    return 0;
  }
}

void net_add_property(NetReplicationSystem *net, u32 net_id, const char *name,
                      NetPropertyType type, u32 offset,
                      ReplicationFlags flags) {
  u32 size;
  if (!net || !name || net_id >= net->object_count)
    return;

  ReplicatedObject *obj = &net->objects[net_id];
  if (obj->property_count >= NET_MAX_PROPERTIES_PER_OBJECT) {
    LOG_ERROR("Cannot add property '%s': limit reached", name);
    return;
  }

  NetProperty *prop = &obj->properties[obj->property_count++];
  strncpy(prop->name, name, sizeof(prop->name) - 1);
  prop->name[sizeof(prop->name) - 1] = '\0';
  prop->type = type;
  prop->offset = offset;
  prop->flags = flags;
  prop->condition = NULL;
  prop->dirty = true;

  size = net_property_wire_size(type);
  prop->last_value = size > 0 ? calloc(1, size) : NULL;

  LOG_INFO("Added replicated property '%s' to netID=%u", name, net_id);
}

void net_register_rpc(NetReplicationSystem *net, u32 net_id,
                      const char *rpc_name,
                      void (*handler)(void *object, void *data)) {
  if (!net || !rpc_name || net_id >= net->object_count)
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
  bool *changed = NULL;
  bool any_sent = false;
  enum { kPacketCap = 8192 };

  if (!net || net->authority != NET_AUTHORITY_SERVER)
    return;
  if (!get_packet_buffer || !send_packet)
    return;

  net->second_accumulator += delta_time;
  if (net->second_accumulator >= 1.0f) {
    net->bytes_sent_this_second = 0;
    net->second_accumulator = 0.0f;
  }

  if (net->object_count > 0) {
    changed = (bool *)calloc(
        (size_t)net->object_count * NET_MAX_PROPERTIES_PER_OBJECT, sizeof(bool));
    if (!changed)
      return;
  }

  for (u32 i = 0; i < net->object_count; i++) {
    ReplicatedObject *obj = &net->objects[i];
    if (!obj->object_ptr)
      continue;
    for (u32 j = 0; j < obj->property_count; j++) {
      NetProperty *prop = &obj->properties[j];
      u32 size = net_property_wire_size(prop->type);
      void *current = (u8 *)obj->object_ptr + prop->offset;
      bool send = true;
      if (size == 0)
        continue;
      /* Dirty forces the first send, including values that match the zero baseline. */
      if (net->enable_delta_compression && !prop->dirty && prop->last_value)
        send = memcmp(current, prop->last_value, size) != 0;
      changed[(size_t)i * NET_MAX_PROPERTIES_PER_OBJECT + j] = send;
    }
  }

  for (u32 client = 0; client < NET_MAX_CLIENTS; client++) {
    u32 packet_size = 0;
    void *packet;
    u8 *write_ptr;

    if (client == net->local_client_id)
      continue;

    packet = get_packet_buffer(kPacketCap);
    if (!packet)
      continue;
    write_ptr = (u8 *)packet;

    for (u32 i = 0; i < net->object_count; i++) {
      ReplicatedObject *obj = &net->objects[i];
      u8 *header;
      u8 *cursor;
      u32 payload = 0;

      if (!obj->is_relevant[client] || !obj->object_ptr)
        continue;
      if (packet_size + sizeof(u32) > kPacketCap)
        break;

      header = write_ptr;
      cursor = write_ptr + sizeof(u32);
      for (u32 j = 0; j < obj->property_count; j++) {
        u32 size;
        if (!changed[(size_t)i * NET_MAX_PROPERTIES_PER_OBJECT + j])
          continue;
        size = net_property_wire_size(obj->properties[j].type);
        if (packet_size + sizeof(u32) + payload + 1u + size > kPacketCap)
          break;
        *cursor++ = (u8)j;
        memcpy(cursor, (u8 *)obj->object_ptr + obj->properties[j].offset, size);
        cursor += size;
        payload += 1u + size;
      }
      if (payload == 0)
        continue;
      memcpy(header, &obj->net_id, sizeof(obj->net_id));
      write_ptr = cursor;
      packet_size += (u32)sizeof(u32) + payload;
    }

    if (packet_size > 0 && net->bytes_sent_this_second + packet_size <=
                               net->bytes_per_second_limit) {
      send_packet(client, packet, packet_size);
      net->bytes_sent_this_second += packet_size;
      any_sent = true;
    }
  }

  /* One baseline per tick. Updating it inside the client loop hid the delta from everyone else. */
  if (any_sent) {
    for (u32 i = 0; i < net->object_count; i++) {
      ReplicatedObject *obj = &net->objects[i];
      if (!obj->object_ptr)
        continue;
      for (u32 j = 0; j < obj->property_count; j++) {
        NetProperty *prop = &obj->properties[j];
        u32 size;
        if (!changed[(size_t)i * NET_MAX_PROPERTIES_PER_OBJECT + j])
          continue;
        size = net_property_wire_size(prop->type);
        if (prop->last_value && size > 0)
          memcpy(prop->last_value, (u8 *)obj->object_ptr + prop->offset, size);
        prop->dirty = false;
      }
    }
  }

  free(changed);
}
