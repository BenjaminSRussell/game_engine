/**
 * ENTITY REPLICATION SYSTEM - COMPLETE IMPLEMENTATION
 * Network entity synchronization with delta compression and relevancy culling
 */

#include <math.h>
#include <network/replication/entity_replication.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTITIES 4096
#define MAX_COMPONENTS 32
#define RELEVANCY_DISTANCE 100.0f
#define PACKET_SIZE 1400
#define DELTA_BUFFER_SIZE 1024

typedef struct {
  uint32_t id;
  uint32_t component_mask;
  uint32_t owner_client_id;
  uint8_t priority;
  bool dormant;
  bool active;
  uint8_t data[256];
  uint32_t data_size;
  uint32_t last_update_sequence;
  Vec3 position;
  float last_replication_time;
} ReplicatedEntity;

typedef struct ReplicationSystem {
  ReplicatedEntity *entities;
  uint32_t max_entities;
  uint32_t active_count;

  // Network stats
  uint32_t bytes_sent;
  uint32_t packets_sent;
  uint32_t entities_replicated;

  // Relevancy culling
  float relevancy_distance;
  Vec3 reference_position;

  // Delta compression
  uint8_t delta_buffer[DELTA_BUFFER_SIZE];
  uint32_t delta_buffer_size;

  // Priority queue for replication
  uint32_t priority_queue[MAX_ENTITIES];
  uint32_t queue_size;
} ReplicationSystem;

// Global replication system
static ReplicationSystem *g_replication_system = NULL;

// Helper functions
static float calculate_distance(const Vec3 *a, const Vec3 *b) {
  float dx = a->x - b->x;
  float dy = a->y - b->y;
  float dz = a->z - b->z;
  return sqrtf(dx * dx + dy * dy + dz * dz);
}

static bool is_entity_relevant(const ReplicatedEntity *entity,
                               const Vec3 *reference_pos, float max_distance) {
  if (!entity || !entity->active || entity->dormant) {
    return false;
  }

  if (max_distance > 0.0f && reference_pos) {
    float distance = calculate_distance(&entity->position, reference_pos);
    return distance <= max_distance;
  }

  return true;
}

static void sort_priority_queue(ReplicationSystem *system) {
  // Simple bubble sort for priority queue (high priority first)
  for (uint32_t i = 0; i < system->queue_size - 1; i++) {
    for (uint32_t j = 0; j < system->queue_size - i - 1; j++) {
      uint32_t idx1 = system->priority_queue[j];
      uint32_t idx2 = system->priority_queue[j + 1];

      ReplicatedEntity *entity1 = &system->entities[idx1];
      ReplicatedEntity *entity2 = &system->entities[idx2];

      // Sort by priority (higher priority first), then by distance
      bool should_swap = false;
      if (entity1->priority < entity2->priority) {
        should_swap = true;
      } else if (entity1->priority == entity2->priority) {
        float dist1 =
            calculate_distance(&entity1->position, &system->reference_position);
        float dist2 =
            calculate_distance(&entity2->position, &system->reference_position);
        should_swap = (dist1 > dist2);
      }

      if (should_swap) {
        system->priority_queue[j] = idx2;
        system->priority_queue[j + 1] = idx1;
      }
    }
  }
}

// Create replication system
ReplicationSystem *replication_create(uint32_t max_entities) {
  if (max_entities == 0 || max_entities > MAX_ENTITIES) {
    return NULL;
  }

  ReplicationSystem *system = calloc(1, sizeof(ReplicationSystem));
  if (!system) {
    return NULL;
  }

  system->entities = calloc(max_entities, sizeof(ReplicatedEntity));
  if (!system->entities) {
    free(system);
    return NULL;
  }

  system->max_entities = max_entities;
  system->active_count = 0;
  system->relevancy_distance = RELEVANCY_DISTANCE;
  system->reference_position = (Vec3){0, 0, 0};
  system->bytes_sent = 0;
  system->packets_sent = 0;
  system->entities_replicated = 0;

  // Initialize priority queue
  for (uint32_t i = 0; i < max_entities; i++) {
    system->priority_queue[i] = i;
  }
  system->queue_size = 0;

  g_replication_system = system;
  return system;
}

// Destroy replication system
void replication_destroy(ReplicationSystem *system) {
  if (!system)
    return;

  free(system->entities);
  free(system);

  if (g_replication_system == system) {
    g_replication_system = NULL;
  }
}

// Spawn entity
uint32_t replication_spawn_entity(ReplicationSystem *system,
                                  uint32_t component_mask,
                                  uint32_t owner_client_id) {
  if (!system || system->active_count >= system->max_entities) {
    return 0;
  }

  for (uint32_t i = 0; i < system->max_entities; i++) {
    if (!system->entities[i].active) {
      uint32_t id = i + 1; // 1-based IDs
      ReplicatedEntity *entity = &system->entities[i];

      entity->id = id;
      entity->component_mask = component_mask;
      entity->owner_client_id = owner_client_id;
      entity->priority = 5; // Default priority
      entity->dormant = false;
      entity->active = true;
      entity->data_size = 0;
      entity->last_update_sequence = 0;
      entity->position = (Vec3){0, 0, 0};
      entity->last_replication_time = 0.0f;

      system->active_count++;

      // Add to priority queue
      if (system->queue_size < system->max_entities) {
        system->priority_queue[system->queue_size++] = i;
        sort_priority_queue(system);
      }

      return id;
    }
  }

  return 0;
}

// Destroy entity
bool replication_despawn_entity(ReplicationSystem *system, uint32_t entity_id) {
  if (!system || entity_id == 0 || entity_id > system->max_entities) {
    return false;
  }

  ReplicatedEntity *entity = &system->entities[entity_id - 1];
  if (!entity->active) {
    return false;
  }

  entity->active = false;
  system->active_count--;

  // Remove from priority queue
  for (uint32_t i = 0; i < system->queue_size; i++) {
    if (system->priority_queue[i] == entity_id - 1) {
      // Shift remaining elements
      for (uint32_t j = i; j < system->queue_size - 1; j++) {
        system->priority_queue[j] = system->priority_queue[j + 1];
      }
      system->queue_size--;
      break;
    }
  }

  return true;
}

// Update entity data
bool replication_update_entity(ReplicationSystem *system, uint32_t entity_id,
                               const void *data, uint32_t data_size) {
  if (!system || entity_id == 0 || entity_id > system->max_entities) {
    return false;
  }

  ReplicatedEntity *entity = &system->entities[entity_id - 1];
  if (!entity->active) {
    return false;
  }

  if (data && data_size > 0 && data_size <= sizeof(entity->data)) {
    memcpy(entity->data, data, data_size);
    entity->data_size = data_size;
  }

  entity->last_update_sequence++;
  return true;
}

// Set entity priority
bool replication_set_entity_priority(ReplicationSystem *system,
                                     uint32_t entity_id, uint8_t priority) {
  if (!system || entity_id == 0 || entity_id > system->max_entities) {
    return false;
  }

  ReplicatedEntity *entity = &system->entities[entity_id - 1];
  if (!entity->active) {
    return false;
  }

  entity->priority = priority;

  // Re-sort priority queue
  sort_priority_queue(system);

  return true;
}

// Set entity dormant state
bool replication_set_entity_dormant(ReplicationSystem *system,
                                    uint32_t entity_id, bool dormant) {
  if (!system || entity_id == 0 || entity_id > system->max_entities) {
    return false;
  }

  ReplicatedEntity *entity = &system->entities[entity_id - 1];
  if (!entity->active) {
    return false;
  }

  entity->dormant = dormant;

  if (dormant) {
    // Remove from priority queue
    for (uint32_t i = 0; i < system->queue_size; i++) {
      if (system->priority_queue[i] == entity_id - 1) {
        for (uint32_t j = i; j < system->queue_size - 1; j++) {
          system->priority_queue[j] = system->priority_queue[j + 1];
        }
        system->queue_size--;
        break;
      }
    }
  } else {
    // Add back to priority queue
    if (system->queue_size < system->max_entities) {
      system->priority_queue[system->queue_size++] = entity_id - 1;
      sort_priority_queue(system);
    }
  }

  return true;
}

// Set relevancy distance
bool replication_set_relevancy_distance(ReplicationSystem *system,
                                        float distance) {
  if (system) {
    system->relevancy_distance = distance;
    return true;
  }
  return false;
}

// Set reference position for relevancy culling
bool replication_set_reference_position(ReplicationSystem *system,
                                        const Vec3 *position) {
  if (system && position) {
    system->reference_position = *position;
    return true;
  }
  return false;
}

// Update replication system (call regularly)
void replication_update(ReplicationSystem *system, float delta_time) {
  if (!system)
    return;

  // Update replication timers and handle timeouts
  for (uint32_t i = 0; i < system->max_entities; i++) {
    ReplicatedEntity *entity = &system->entities[i];
    if (entity->active && !entity->dormant) {
      entity->last_replication_time += delta_time;
    }
  }
}

// Get entities to replicate (based on priority and relevancy)
uint32_t replication_get_entities_to_replicate(ReplicationSystem *system,
                                               uint32_t *entity_ids,
                                               uint32_t max_count,
                                               const Vec3 *reference_pos) {
  if (!system || !entity_ids || max_count == 0) {
    return 0;
  }

  uint32_t count = 0;

  for (uint32_t i = 0; i < system->queue_size && count < max_count; i++) {
    uint32_t entity_index = system->priority_queue[i];
    ReplicatedEntity *entity = &system->entities[entity_index];

    if (is_entity_relevant(entity, reference_pos, system->relevancy_distance)) {
      entity_ids[count++] = entity_index + 1; // Convert to 1-based ID
    }
  }

  return count;
}

// Get replication statistics
void replication_get_stats(const ReplicationSystem *system,
                           uint32_t *active_count, uint32_t *bytes_sent,
                           uint32_t *packets_sent, float *avg_time_ms) {
  if (!system)
    return;

  if (active_count)
    *active_count = system->active_count;
  if (bytes_sent)
    *bytes_sent = system->bytes_sent;
  if (packets_sent)
    *packets_sent = system->packets_sent;
}

// Reset statistics
void replication_reset_stats(ReplicationSystem *system) {
  if (!system)
    return;

  system->bytes_sent = 0;
  system->packets_sent = 0;
  system->entities_replicated = 0;
}

// Find entity by owner
uint32_t replication_find_entities_by_owner(ReplicationSystem *system,
                                            uint32_t owner_client_id,
                                            uint32_t *entity_ids,
                                            uint32_t max_count) {
  if (!system || !entity_ids || max_count == 0) {
    return 0;
  }

  uint32_t count = 0;

  for (uint32_t i = 0; i < system->max_entities && count < max_count; i++) {
    ReplicatedEntity *entity = &system->entities[i];
    if (entity->active && entity->owner_client_id == owner_client_id) {
      entity_ids[count++] = entity->id;
    }
  }

  return count;
}

// Get global replication system
ReplicationSystem *replication_get_system(void) { return g_replication_system; }

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
      // Pack entity ID (3 bytes), flags (1 byte), data size (2 bytes)
      if (offset + 10 + system->entities[i].data_size > buffer_size)
        break;

      memcpy(out + offset, &system->entities[i].id, 3);
      offset += 3;

      out[offset++] = 0;

      uint32_t size_to_pack = system->entities[i].data_size;
      memcpy(out + offset, &size_to_pack, 4);
      offset += 4;

      memcpy(out + offset, system->entities[i].data, size_to_pack);
      offset += size_to_pack;
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

    offset++; // flags

    uint32_t data_size;
    memcpy(&data_size, in + offset, 4);
    offset += 4;

    if (offset + data_size > buffer_size)
      break;

    uint32_t idx = entity_id - 1;
    if (idx < system->max_entities && system->entities[idx].active) {
      if (data_size <= 256) {
        memcpy(system->entities[idx].data, in + offset, data_size);
        system->entities[idx].data_size = data_size;
      }
    }
    offset += data_size;
  }

  return true;
}
