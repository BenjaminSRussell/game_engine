#include "networking_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

/* ============================================================================
 * REPLICATION SYSTEM
 * ============================================================================
 */

// Forward declarations
extern int network_server_send_to_client(NetworkContext *context,
                                         uint32_t client_id, PacketType type,
                                         const void *data, uint32_t size);
extern int network_server_get_clients(NetworkContext *context,
                                      uint32_t *out_client_ids,
                                      uint32_t max_count);

static ReplicatedEntity *find_server_entity(NetworkContext *ctx,
                                            uint32_t entity_id) {
  for (uint32_t i = 0; i < ctx->server_entity_count; i++) {
    if (ctx->server_entities[i].entity_id == entity_id) {
      return &ctx->server_entities[i];
    }
  }
  return NULL;
}

static ClientEntityState *find_client_state(NetworkContext *ctx,
                                            uint32_t entity_id) {
  for (uint32_t i = 0; i < ctx->client_state_count; i++) {
    if (ctx->client_states[i].entity_id == entity_id) {
      return &ctx->client_states[i];
    }
  }
  return NULL;
}

static ReplicationClient *get_replication_client(NetworkContext *ctx,
                                                 uint32_t client_id) {
  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    if (ctx->replication_clients[i].active &&
        ctx->replication_clients[i].client_id == client_id) {
      return &ctx->replication_clients[i];
    }
  }
  // Find free slot
  for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
    if (!ctx->replication_clients[i].active) {
      ctx->replication_clients[i].active = true;
      ctx->replication_clients[i].client_id = client_id;
      memset(ctx->replication_clients[i].known_versions, 0,
             sizeof(ctx->replication_clients[i].known_versions));
      return &ctx->replication_clients[i];
    }
  }
  return NULL;
}

int network_replication_add_entity(NetworkContext *context, uint32_t entity_id,
                                   bool always_replicate) {
  if (!context || context->mode != NETWORK_MODE_SERVER)
    return -1;
  if (context->server_entity_count >= MAX_REPLICATED_ENTITIES)
    return -1;

  ReplicatedEntity *entity =
      &context->server_entities[context->server_entity_count++];
  memset(entity, 0, sizeof(ReplicatedEntity));
  entity->entity_id = entity_id;
  entity->always_replicate = always_replicate;
  entity->changed = true;
  entity->version = 1;

  return 0;
}

void network_replication_remove_entity(NetworkContext *context,
                                       uint32_t entity_id) {
  if (!context || context->mode != NETWORK_MODE_SERVER)
    return;

  for (uint32_t i = 0; i < context->server_entity_count; i++) {
    if (context->server_entities[i].entity_id == entity_id) {
      // Swap with last
      if (i < context->server_entity_count - 1) {
        context->server_entities[i] =
            context->server_entities[context->server_entity_count - 1];
      }
      context->server_entity_count--;
      return;
    }
  }
}

void network_replication_update_entity(NetworkContext *context,
                                       uint32_t entity_id, uint32_t field_mask,
                                       const void *field_data,
                                       uint16_t data_size) {
  if (!context || context->mode != NETWORK_MODE_SERVER)
    return;

  ReplicatedEntity *entity = find_server_entity(context, entity_id);
  if (!entity)
    return;

  entity->field_mask |= field_mask;
  if (field_data && data_size > 0 && data_size <= MAX_REPLICATION_FIELDS) {
    memcpy(entity->field_data, field_data, data_size);
    entity->data_size = data_size;
  }
  entity->changed = true;
  entity->version++;
}

void network_replication_set_entity_position(NetworkContext *context,
                                             uint32_t entity_id, float x,
                                             float y, float z) {
  if (!context || context->mode != NETWORK_MODE_SERVER)
    return;

  ReplicatedEntity *entity = find_server_entity(context, entity_id);
  if (!entity)
    return;

  entity->position[0] = x;
  entity->position[1] = y;
  entity->position[2] = z;
}

void network_replication_set_owner(NetworkContext *context, uint32_t entity_id,
                                   uint32_t owner_client_id) {
  if (!context || context->mode != NETWORK_MODE_SERVER)
    return;

  ReplicatedEntity *entity = find_server_entity(context, entity_id);
  if (!entity)
    return;

  entity->owner_id = owner_client_id;
  entity->version++;
}

uint32_t network_replication_get_owner(NetworkContext *context,
                                       uint32_t entity_id) {
  if (!context)
    return 0;

  if (context->mode == NETWORK_MODE_SERVER) {
    ReplicatedEntity *entity = find_server_entity(context, entity_id);
    return entity ? entity->owner_id : 0;
  } else {
    ClientEntityState *state = find_client_state(context, entity_id);
    return state ? state->last_state.owner_id : 0;
  }
}

static void compress_snapshot(ReplicationSnapshot *snapshot) {
  if (!snapshot || snapshot->entity_count == 0)
    return;

  uint8_t *compressed_data = snapshot->compressed_data;
  uint32_t compressed_size = 0;

  for (uint32_t i = 0; i < snapshot->entity_count; i++) {
    ReplicatedEntity *entity = &snapshot->entities[i];

    // Pack entity data (ID + owner + version + position + mask + data_size +
    // data)
    memcpy(&compressed_data[compressed_size], &entity->entity_id, 4);
    compressed_size += 4;
    memcpy(&compressed_data[compressed_size], &entity->owner_id, 4);
    compressed_size += 4;
    memcpy(&compressed_data[compressed_size], &entity->version, 4);
    compressed_size += 4;
    memcpy(&compressed_data[compressed_size], entity->position,
           sizeof(float) * 3);
    compressed_size += 12;
    memcpy(&compressed_data[compressed_size], &entity->field_mask, 4);
    compressed_size += 4;
    memcpy(&compressed_data[compressed_size], &entity->data_size, 2);
    compressed_size += 2;

    if (entity->data_size > 0 && entity->data_size <= MAX_REPLICATION_FIELDS) {
      memcpy(&compressed_data[compressed_size], entity->field_data,
             entity->data_size);
      compressed_size += entity->data_size;
    }
  }

  snapshot->compressed_size = compressed_size;
  snapshot->is_compressed = (compressed_size < MAX_PACKET_SIZE);
}

static bool decompress_snapshot(ReplicationSnapshot *snapshot,
                                const uint8_t *compressed_data) {
  if (!snapshot || !compressed_data)
    return false;

  uint32_t offset = 0;
  while (offset < snapshot->compressed_size &&
         snapshot->entity_count < MAX_REPLICATED_ENTITIES) {
    ReplicatedEntity *entity = &snapshot->entities[snapshot->entity_count++];

    memcpy(&entity->entity_id, &compressed_data[offset], 4);
    offset += 4;
    memcpy(&entity->owner_id, &compressed_data[offset], 4);
    offset += 4;
    memcpy(&entity->version, &compressed_data[offset], 4);
    offset += 4;
    memcpy(entity->position, &compressed_data[offset], 12);
    offset += 12;
    memcpy(&entity->field_mask, &compressed_data[offset], 4);
    offset += 4;
    memcpy(&entity->data_size, &compressed_data[offset], 2);
    offset += 2;

    if (entity->data_size > 0) {
      memcpy(entity->field_data, &compressed_data[offset], entity->data_size);
      offset += entity->data_size;
    }
  }

  return true;
}

void network_replication_server_update(NetworkContext *ctx) {
  if (!ctx || ctx->mode != NETWORK_MODE_SERVER)
    return;

  uint32_t client_ids[MAX_CLIENTS];
  int client_count = network_server_get_clients(ctx, client_ids, MAX_CLIENTS);

  ctx->last_sequence_number++;

  for (int c = 0; c < client_count; c++) {
    uint32_t client_id = client_ids[c];
    ReplicationClient *rep_client = get_replication_client(ctx, client_id);
    if (!rep_client)
      continue;

    ReplicationSnapshot snapshot;
    memset(&snapshot, 0, sizeof(ReplicationSnapshot));
    snapshot.sequence_number = ctx->last_sequence_number;
    snapshot.timestamp = (uint32_t)(time(NULL) * 1000);

    // Collect entities for this client
    for (uint32_t i = 0; i < ctx->server_entity_count; i++) {
      ReplicatedEntity *entity = &ctx->server_entities[i];

      // Version-based delta
      if (entity->version > rep_client->known_versions[i]) {
        if (snapshot.entity_count < MAX_REPLICATED_ENTITIES) {
          snapshot.entities[snapshot.entity_count++] = *entity;
          rep_client->known_versions[i] = entity->version;
        }
      }
    }

    if (snapshot.entity_count > 0) {
      if (ctx->config.enable_compression) {
        compress_snapshot(&snapshot);
      }

      uint8_t packet_data[MAX_PACKET_SIZE];
      uint32_t packet_size =
          sizeof(uint32_t) * 3; // sequence + timestamp + entity_count

      memcpy(&packet_data[0], &snapshot.sequence_number, 4);
      memcpy(&packet_data[4], &snapshot.timestamp, 4);
      memcpy(&packet_data[8], &snapshot.entity_count, 4);

      if (snapshot.is_compressed) {
        memcpy(&packet_data[packet_size], snapshot.compressed_data,
               snapshot.compressed_size);
        packet_size += snapshot.compressed_size;
      } else {
        for (uint32_t i = 0; i < snapshot.entity_count; i++) {
          memcpy(&packet_data[packet_size], &snapshot.entities[i],
                 sizeof(ReplicatedEntity));
          packet_size += sizeof(ReplicatedEntity);
        }
      }

      network_server_send_to_client(ctx, client_id, PACKET_TYPE_SNAPSHOT,
                                    packet_data, packet_size);
    }
  }
}

void network_replication_process_snapshot(NetworkContext *ctx,
                                          const uint8_t *packet_data,
                                          uint32_t packet_size) {
  if (!ctx || !packet_data || packet_size < 12)
    return;

  ReplicationSnapshot snapshot;
  memset(&snapshot, 0, sizeof(ReplicationSnapshot));

  memcpy(&snapshot.sequence_number, &packet_data[0], 4);
  memcpy(&snapshot.timestamp, &packet_data[4], 4);
  memcpy(&snapshot.entity_count, &packet_data[8], 4);

  const uint8_t *data_ptr = &packet_data[12];

  if (ctx->config.enable_compression) {
    snapshot.compressed_size = packet_size - 12;
    decompress_snapshot(&snapshot, data_ptr);
  } else {
    for (uint32_t i = 0; i < snapshot.entity_count; i++) {
      memcpy(&snapshot.entities[i], data_ptr, sizeof(ReplicatedEntity));
      data_ptr += sizeof(ReplicatedEntity);
    }
  }

  // Apply to client state
  for (uint32_t i = 0; i < snapshot.entity_count; i++) {
    const ReplicatedEntity *replicated_entity = &snapshot.entities[i];
    ClientEntityState *client_state =
        find_client_state(ctx, replicated_entity->entity_id);

    if (!client_state) {
      if (ctx->client_state_count < MAX_REPLICATED_ENTITIES) {
        client_state = &ctx->client_states[ctx->client_state_count++];
        memset(client_state, 0, sizeof(ClientEntityState));
        client_state->entity_id = replicated_entity->entity_id;
        client_state->is_new = true;
      }
    }

    if (client_state &&
        snapshot.sequence_number > client_state->last_sequence) {
      client_state->last_sequence = snapshot.sequence_number;
      client_state->last_timestamp = snapshot.timestamp;
      client_state->last_state = *replicated_entity;
      client_state->is_new = false;
    }
  }
}
