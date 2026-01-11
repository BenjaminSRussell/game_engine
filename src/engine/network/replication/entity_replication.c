#include <network/replication/entity_replication.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Implement a priority-based replication queue for high-bandwidth
// efficiency
// TODO: Add support for Area-of-Interest (AOI) / Relevancy culling based on
// view frustum
// TODO: Implement client-side prediction and server-side reconciliation for
// movement
// TODO: Add support for lag compensation (Backwards reconciliation) for combat
// hits
// TODO: Implement robust packet encryption and authentication for game security
// TODO: Add support for Delta-Compressed property updates with bit-precision
// quantization
// TODO: Implement a network-aware ECS component replication system (NetECS)
// TODO: Research AI-driven packet prioritization based on gameplay state
// TODO: Add support for network-multicast and reliable-ordered message delivery
// TODO: Implement a comprehensive network profiler with latency and jitter
// simulation
// TODO: Add support for sub-stepping and client-side clock synchronization
// TODO: Implement a robust disconnect/reconnect state management system
// TODO: Research and implement P2P fallback and Relay-bridged networking
// TODO: Add support for network-synced animation montages and GAS abilities
// TODO: Implement a virtual "replicated" world state for local prediction
// validation
// TODO: Add logic for bandwidth-limited property updates (dynamic frequency
// scaling)
// TODO: Research and implement voice-over-IP (VoIP) integration with
// replication
// TODO: Implement a robust cheat-detection system using server-side validation
// TODO: Add support for massive entity counts (>10k) using replication-graph
// optimizations
// TODO: Research and implement network-replicated PCG seed synchronization

typedef struct replicated_entity {
  u32 id;
  u32 component_mask;
  u32 owner_client_id;
  u8 priority;
  bool dormant;
  bool active;
  u8 data[256]; // Placeholder for generic component data
  u32 data_size;
  DeltaCompressor *compressor;
  u32 last_acked_sequence;
} replicated_entity_t;

struct ReplicationSystem {
  replicated_entity_t *entities;
  u32 max_entities;
  u32 active_count;

  // Stats
  u32 bytes_sent;
  u32 packets_sent;

  // Relevancy
  f32 relevancy_distance;
  Vec3 reference_position;
};

ReplicationSystem *replication_create(u32 max_entities) {
  ReplicationSystem *system =
      (ReplicationSystem *)calloc(1, sizeof(ReplicationSystem));
  if (!system)
    return NULL;

  system->entities =
      (replicated_entity_t *)calloc(max_entities, sizeof(replicated_entity_t));
  system->max_entities = max_entities;
  return system;
}

void replication_destroy(ReplicationSystem *system) {
  if (!system)
    return;
  free(system->entities);
  free(system);
}

u32 replication_spawn_entity(ReplicationSystem *system, u32 component_mask,
                             u32 owner_client_id) {
  if (!system)
    return 0;

  for (uint32_t i = 0; i < system->max_entities; i++) {
    if (!system->entities[i].active) {
      uint32_t id = i + 1; // 1-based IDs
      system->entities[i].id = id;
      system->entities[i].component_mask = component_mask;
      system->entities[i].owner_client_id = owner_client_id;
      system->entities[i].active = true;
      system->active_count++;
      return id;
    }
  }
  return 0;
}

bool replication_despawn_entity(ReplicationSystem *system, u32 entity_id) {
  if (!system || entity_id == 0 || entity_id > system->max_entities)
    return false;

  uint32_t idx = entity_id - 1;
  if (system->entities[idx].active) {
    system->entities[idx].active = false;
    system->active_count--;
    return true;
  }
  return false;
}

bool replication_update_entity(ReplicationSystem *system, u32 entity_id,
                               const void *component_data, u32 data_size) {
  if (!system || entity_id == 0 || entity_id > system->max_entities)
    return false;

  uint32_t idx = entity_id - 1;
  if (system->entities[idx].active) {
    if (data_size > 256)
      data_size = 256;
    memcpy(system->entities[idx].data, component_data, data_size);
    system->entities[idx].data_size = data_size;
    return true;
  }
  return false;
}

u32 replication_generate_update_packet(ReplicationSystem *system, void *buffer,
                                       u32 buffer_size) {
  if (!system || !buffer || buffer_size < 8)
    return 0;

  uint32_t offset = 0;
  uint8_t *out = (uint8_t *)buffer;

  // Header: entity count
  memcpy(out + offset, &system->active_count, 4);
  offset += 4;

  for (uint32_t i = 0; i < system->max_entities; i++) {
    if (system->entities[i].active && !system->entities[i].dormant) {
      // Lazy init compressor
      if (!system->entities[i].compressor) {
        system->entities[i].compressor = delta_compressor_create();
      }

      // Update state in compressor
      delta_compressor_set_state(system->entities[i].compressor,
                                 system->entities[i].data,
                                 system->entities[i].data_size);

      // Simple relevancy check
      if (system->relevancy_distance > 0.0f) {
        // TODO: Get entity position from ECS
        // For now, assume irrelevant if placeholder logic says so
      }

      // Generate delta if we have an acked sequence
      u32 diff_size = 0;
      u8 diff_buffer[256];
      bool use_delta = false;

      if (system->entities[i].last_acked_sequence > 0) {
        if (delta_compressor_generate_delta(
                system->entities[i].compressor,
                system->entities[i].last_acked_sequence, diff_buffer,
                &diff_size)) {
          use_delta = true;
        }
      }

      // Pack entity ID (3 bytes), flags (1 byte), data size (2 bytes)
      if (offset + 10 +
              (use_delta ? diff_size : system->entities[i].data_size) >
          buffer_size)
        break;

      memcpy(out + offset, &system->entities[i].id, 3);
      offset += 3;

      uint8_t flags = use_delta ? 0x01 : 0x00;
      out[offset++] = flags;

      u32 size_to_pack = use_delta ? diff_size : system->entities[i].data_size;
      memcpy(out + offset, &size_to_pack, 4);
      offset += 4;

      memcpy(out + offset, use_delta ? diff_buffer : system->entities[i].data,
             size_to_pack);
      offset += size_to_pack;

      // Create snapshot for future deltas
      delta_compressor_create_snapshot(system->entities[i].compressor);
      system->packets_sent++;
    }
  }

  system->bytes_sent += offset;
  return offset;
}

bool replication_process_update_packet(ReplicationSystem *system,
                                       const void *buffer, u32 buffer_size) {
  if (!system || !buffer || buffer_size < 4)
    return false;

  const uint8_t *in = (const uint8_t *)buffer;
  uint32_t offset = 0;

  uint32_t entity_count;
  memcpy(&entity_count, in + offset, 4);
  offset += 4;

  for (uint32_t i = 0; i < entity_count; i++) {
    if (offset + 8 > buffer_size)
      break;

    uint32_t entity_id = 0;
    memcpy(&entity_id, in + offset, 3);
    offset += 3;

    uint8_t flags = in[offset++];
    bool is_delta = (flags & 0x01) != 0;

    uint32_t data_size;
    memcpy(&data_size, in + offset, 4);
    offset += 4;

    if (offset + data_size > buffer_size)
      break;

    uint32_t idx = entity_id - 1;
    if (idx < system->max_entities && system->entities[idx].active) {
      if (is_delta) {
        if (system->entities[idx].compressor) {
          delta_compressor_apply_delta(
              system->entities[idx].compressor, in + offset, data_size,
              system->entities[idx].last_acked_sequence);

          delta_compressor_get_state(system->entities[idx].compressor,
                                     system->entities[idx].data,
                                     &system->entities[idx].data_size);
        }
      } else {
        if (data_size <= 256) {
          memcpy(system->entities[idx].data, in + offset, data_size);
          system->entities[idx].data_size = data_size;

          if (!system->entities[idx].compressor) {
            system->entities[idx].compressor = delta_compressor_create();
          }
          delta_compressor_set_state(system->entities[idx].compressor,
                                     system->entities[idx].data, data_size);
        }
      }
    }
    offset += data_size;
  }

  return true;
}

// Stubs for remaining functions
bool replication_set_entity_priority(ReplicationSystem *system, u32 entity_id,
                                     u8 priority) {
  return true;
}
bool replication_set_entity_dormant(ReplicationSystem *system, u32 entity_id,
                                    bool dormant) {
  return true;
}
bool replication_set_relevancy_distance(ReplicationSystem *system,
                                        f32 distance) {
  return true;
}
bool replication_set_reference_position(ReplicationSystem *system,
                                        const Vec3 *position) {
  return true;
}
void replication_get_stats(const ReplicationSystem *system, u32 *entity_count,
                           u32 *bytes_sent, u32 *packets_sent,
                           f32 *avg_time_ms) {
  if (entity_count)
    *entity_count = system->active_count;
  if (bytes_sent)
    *bytes_sent = system->bytes_sent;
  if (packets_sent)
    *packets_sent = system->packets_sent;
}
