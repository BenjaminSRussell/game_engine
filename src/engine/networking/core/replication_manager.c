/**
 * @file replication_manager.c
 * @brief Entity State Replication System.
 *
 * Handles delta compression, state snapshots, and interest management.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <networking/core/replication_manager.h>
#include <string.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct ReplicatedEntity {
  uint32_t network_id;
  uint32_t entity_id;
  uint64_t last_ack_tick;
  uint8_t dirty_mask; // Bitmask of changed components
  void *prev_state;   // For delta compression
} ReplicatedEntity;

typedef struct ReplicationChannel {
  uint32_t connection_id;
  ReplicatedEntity entities[1024];
  uint32_t entity_count;
  float interest_radius;
} ReplicationChannel;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Checks if entity is relevant to the connection.
 * Simple distance check.
 */
bool is_relevant(ReplicationChannel *channel, vec3 entity_pos,
                 vec3 viewer_pos) {
  float dist_sq = vec3_distance_sq(entity_pos, viewer_pos);
  return dist_sq < (channel->interest_radius * channel->interest_radius);
}

/**
 * @brief Serializes entity updates into a packet.
 *
 * Writes: [NetID] [ComponentMask] [Data...]
 */
void serialize_updates(ReplicationChannel *channel, Packet *packet) {
  for (int i = 0; i < channel->entity_count; i++) {
    ReplicatedEntity *rep = &channel->entities[i];

    if (rep->dirty_mask == 0)
      continue;

    packet_write_uint32(packet, rep->network_id);
    packet_write_uint8(packet, rep->dirty_mask);

    // Serialize Components based on mask
    if (rep->dirty_mask & COMP_TRANSFORM) {
      // ... write pos/rot with quantization ...
    }
    if (rep->dirty_mask & COMP_HEALTH) {
      // ... write health ...
    }

    // Reset dirty mask after reliable send (simplified)
    // In real UDP system, clear only on ACK
  }
}

/**
 * @brief Processes incoming state updates on client.
 */
void process_state_update(Packet *packet) {
  while (packet_has_data(packet)) {
    uint32_t net_id = packet_read_uint32(packet);
    uint8_t mask = packet_read_uint8(packet);

    Entity *e = find_entity_by_net_id(net_id);
    if (!e) {
      // Spawn entity if missing
      e = entity_spawn_networked(net_id);
    }

    // Deserialize and apply interpolation target
    if (mask & COMP_TRANSFORM) {
      vec3 target_pos = packet_read_vec3(packet);
      // Add to interpolation buffer
      interpolation_add_snapshot(e, target_pos);
    }
  }
}
