#pragma once

#include <core/memory.h>
#include <core/types.h>
#include <math/math.h>

#include "network/replication/delta_compression.h"

typedef struct ReplicationSystem ReplicationSystem;

// Component masks for entity replication
typedef enum {
  COMPONENT_TRANSFORM = (1 << 0),
  COMPONENT_PHYSICS = (1 << 1),
  COMPONENT_RENDER = (1 << 2),
  COMPONENT_AUDIO = (1 << 3),
  COMPONENT_AI = (1 << 4),
  COMPONENT_CUSTOM = (1 << 5)
} ComponentMask;

// Replication priority levels
typedef enum {
  PRIORITY_CRITICAL = 0, // Player, important objects
  PRIORITY_HIGH = 1,     // Enemies, interactive objects
  PRIORITY_MEDIUM = 2,   // Static objects, decorations
  PRIORITY_LOW = 3       // Background elements
} ReplicationPriority;

// System Management
ReplicationSystem *replication_create(u32 max_entities);
void replication_destroy(ReplicationSystem *system);

// Entity Lifecycle
u32 replication_spawn_entity(ReplicationSystem *system, u32 component_mask,
                             u32 owner_client_id);
bool replication_despawn_entity(ReplicationSystem *system, u32 entity_id);
bool replication_update_entity(ReplicationSystem *system, u32 entity_id,
                               const void *component_data, u32 data_size);

// Entity Configuration
bool replication_set_entity_priority(ReplicationSystem *system, u32 entity_id,
                                     u8 priority);
bool replication_set_entity_dormant(ReplicationSystem *system, u32 entity_id,
                                    bool dormant);

// Relevancy Management
bool replication_set_relevancy_distance(ReplicationSystem *system,
                                        f32 distance);
bool replication_set_reference_position(ReplicationSystem *system,
                                        const Vec3 *position);

// Packet Generation/Processing
u32 replication_generate_update_packet(ReplicationSystem *system, void *buffer,
                                       u32 buffer_size);
bool replication_process_update_packet(ReplicationSystem *system,
                                       const void *buffer, u32 buffer_size);

// Statistics
void replication_get_stats(const ReplicationSystem *system, u32 *entity_count,
                           u32 *bytes_sent, u32 *packets_sent,
                           f32 *avg_time_ms);
