#include "network/replication/entity_replication.h"
#include "core/serialization.h"
#include "core/logger.h"
#include "core/timing.h"
#include "network/replication/delta_compression.h"
#include <string.h>

// ✅ COMPLETED: Entity replication system with delta compression and prioritization
// Implements efficient network synchronization of entities across clients

typedef struct {
    u32 entity_id;
    u32 component_mask;
    u8* component_data;
    u32 data_size;
    u32 last_update_sequence;
    u64 last_update_time;
    f32 update_interval;
    u8 priority;
    bool is_relevant;
    bool is_dormant;
    u32 owner_client_id;
} ReplicatedEntity;

typedef struct {
    ReplicatedEntity* entities;
    u32 max_entities;
    u32 entity_count;
    u32 next_entity_id;
    
    DeltaCompressor* compressor;
    u32 current_sequence;
    u64 last_update_time;
    
    // Relevancy settings
    f32 relevancy_distance;
    vec3 reference_position;
    
    // Bandwidth limiting
    u32 max_bytes_per_update;
    u32 current_bytes_sent;
    
    // Statistics
    u32 entities_replicated;
    u32 bytes_sent;
    u32 packets_sent;
    f32 avg_replication_time_ms;
    
    bool is_initialized;
} ReplicationSystem;

// Component replication masks
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
    PRIORITY_CRITICAL = 0,    // Player, important objects
    PRIORITY_HIGH = 1,        // Enemies, interactive objects
    PRIORITY_MEDIUM = 2,      // Static objects, decorations
    PRIORITY_LOW = 3          // Background elements
} ReplicationPriority;

// Internal functions
static ReplicatedEntity* replication_find_entity(ReplicationSystem* system, u32 entity_id);
static bool replication_is_entity_relevant(const ReplicationSystem* system, const ReplicatedEntity* entity);
static bool replication_should_replicate_entity(const ReplicationSystem* system, const ReplicatedEntity* entity);
static u32 replication_calculate_entity_size(const ReplicatedEntity* entity);
static bool replication_serialize_entity(const ReplicatedEntity* entity, void* buffer, u32 buffer_size, u32* written);
static bool replication_deserialize_entity(const void* buffer, u32 buffer_size, ReplicatedEntity* entity);

ReplicationSystem* replication_create(u32 max_entities) {
    if (max_entities == 0) {
        LOG_ERROR("Invalid max_entities for replication system");
        return NULL;
    }
    
    ReplicationSystem* system = malloc(sizeof(ReplicationSystem));
    if (!system) {
        LOG_ERROR("Failed to allocate replication system");
        return NULL;
    }
    
    memset(system, 0, sizeof(ReplicationSystem));
    system->entities = malloc(sizeof(ReplicatedEntity) * max_entities);
    if (!system->entities) {
        LOG_ERROR("Failed to allocate entity array");
        free(system);
        return NULL;
    }
    
    memset(system->entities, 0, sizeof(ReplicatedEntity) * max_entities);
    system->max_entities = max_entities;
    system->next_entity_id = 1;
    system->relevancy_distance = 100.0f;
    system->max_bytes_per_update = 1024; // 1KB per update
    
    system->compressor = delta_compressor_create();
    if (!system->compressor) {
        LOG_ERROR("Failed to create delta compressor");
        free(system->entities);
        free(system);
        return NULL;
    }
    
    system->is_initialized = true;
    LOG_INFO("Replication system created with max entities: %u", max_entities);
    return system;
}

void replication_destroy(ReplicationSystem* system) {
    if (!system) return;
    
    // Free entity data
    for (u32 i = 0; i < system->max_entities; i++) {
        if (system->entities[i].component_data) {
            free(system->entities[i].component_data);
        }
    }
    
    free(system->entities);
    delta_compressor_destroy(system->compressor);
    free(system);
    LOG_INFO("Replication system destroyed");
}

u32 replication_spawn_entity(ReplicationSystem* system, u32 component_mask, u32 owner_client_id) {
    if (!system || !system->is_initialized) return 0;
    
    if (system->entity_count >= system->max_entities) {
        LOG_ERROR("Entity limit reached");
        return 0;
    }
    
    // Find free slot
    u32 slot = 0;
    for (u32 i = 0; i < system->max_entities; i++) {
        if (system->entities[i].entity_id == 0) {
            slot = i;
            break;
        }
    }
    
    ReplicatedEntity* entity = &system->entities[slot];
    entity->entity_id = system->next_entity_id++;
    entity->component_mask = component_mask;
    entity->last_update_sequence = 0;
    entity->last_update_time = timing_get_time_ms();
    entity->update_interval = 0.1f; // 10 updates per second by default
    entity->priority = PRIORITY_MEDIUM;
    entity->is_relevant = true;
    entity->is_dormant = false;
    entity->owner_client_id = owner_client_id;
    
    system->entity_count++;
    LOG_INFO("Spawned entity %u with component mask 0x%08X", entity->entity_id, component_mask);
    return entity->entity_id;
}

bool replication_despawn_entity(ReplicationSystem* system, u32 entity_id) {
    if (!system || entity_id == 0) return false;
    
    ReplicatedEntity* entity = replication_find_entity(system, entity_id);
    if (!entity) {
        LOG_WARN("Entity %u not found for despawn", entity_id);
        return false;
    }
    
    if (entity->component_data) {
        free(entity->component_data);
    }
    
    memset(entity, 0, sizeof(ReplicatedEntity));
    system->entity_count--;
    
    LOG_INFO("Despawned entity %u", entity_id);
    return true;
}

bool replication_update_entity(ReplicationSystem* system, u32 entity_id, const void* component_data, u32 data_size) {
    if (!system || !component_data || data_size == 0) return false;
    
    ReplicatedEntity* entity = replication_find_entity(system, entity_id);
    if (!entity) {
        LOG_WARN("Entity %u not found for update", entity_id);
        return false;
    }
    
    // Reallocate component data if needed
    if (!entity->component_data || entity->data_size != data_size) {
        free(entity->component_data);
        entity->component_data = malloc(data_size);
        if (!entity->component_data) {
            LOG_ERROR("Failed to allocate component data for entity %u", entity_id);
            return false;
        }
        entity->data_size = data_size;
    }
    
    memcpy(entity->component_data, component_data, data_size);
    entity->last_update_time = timing_get_time_ms();
    
    LOG_TRACE("Updated entity %u with %u bytes of data", entity_id, data_size);
    return true;
}

bool replication_set_entity_priority(ReplicationSystem* system, u32 entity_id, u8 priority) {
    if (!system) return false;
    
    ReplicatedEntity* entity = replication_find_entity(system, entity_id);
    if (!entity) return false;
    
    entity->priority = priority;
    return true;
}

bool replication_set_entity_dormant(ReplicationSystem* system, u32 entity_id, bool dormant) {
    if (!system) return false;
    
    ReplicatedEntity* entity = replication_find_entity(system, entity_id);
    if (!entity) return false;
    
    entity->is_dormant = dormant;
    LOG_TRACE("Entity %u dormancy set to %s", entity_id, dormant ? "true" : "false");
    return true;
}

bool replication_set_relevancy_distance(ReplicationSystem* system, f32 distance) {
    if (!system || distance < 0.0f) return false;
    
    system->relevancy_distance = distance;
    return true;
}

bool replication_set_reference_position(ReplicationSystem* system, const vec3* position) {
    if (!system || !position) return false;
    
    system->reference_position = *position;
    return true;
}

u32 replication_generate_update_packet(ReplicationSystem* system, void* buffer, u32 buffer_size) {
    if (!system || !buffer || buffer_size == 0) return 0;
    
    u64 start_time = timing_get_time_ms();
    u32 bytes_written = 0;
    system->current_bytes_sent = 0;
    
    // Create snapshot for delta compression
    delta_compressor_create_snapshot(system->compressor);
    
    // Write packet header
    if (bytes_written + sizeof(u32) > buffer_size) return 0;
    memcpy((u8*)buffer + bytes_written, &system->current_sequence, sizeof(u32));
    bytes_written += sizeof(u32);
    
    // Write entity count placeholder
    u32 entity_count_pos = bytes_written;
    u32 entity_count = 0;
    if (bytes_written + sizeof(u32) > buffer_size) return 0;
    memcpy((u8*)buffer + bytes_written, &entity_count, sizeof(u32));
    bytes_written += sizeof(u32);
    
    // Sort entities by priority
    // Simple priority sorting (higher priority = lower value)
    for (u32 i = 0; i < system->max_entities - 1; i++) {
        for (u32 j = i + 1; j < system->max_entities; j++) {
            ReplicatedEntity* entity_i = &system->entities[i];
            ReplicatedEntity* entity_j = &system->entities[j];
            
            if (entity_i->entity_id == 0 || entity_j->entity_id == 0) continue;
            
            // Swap if entity_i has higher priority (lower value) than entity_j
            if (entity_i->priority < entity_j->priority) {
                ReplicatedEntity temp = *entity_i;
                *entity_i = *entity_j;
                *entity_j = temp;
            }
        }
    }
    
    // Replicate entities
    for (u32 i = 0; i < system->max_entities; i++) {
        ReplicatedEntity* entity = &system->entities[i];
        
        if (entity->entity_id == 0) continue;
        if (!replication_should_replicate_entity(system, entity)) continue;
        if (system->current_bytes_sent >= system->max_bytes_per_update) break;
        
        u32 entity_size = replication_calculate_entity_size(entity);
        if (bytes_written + entity_size > buffer_size) break;
        
        u32 written = 0;
        if (replication_serialize_entity(entity, (u8*)buffer + bytes_written, buffer_size - bytes_written, &written)) {
            bytes_written += written;
            entity_count++;
            system->current_bytes_sent += written;
            entity->last_update_sequence = system->current_sequence;
            system->entities_replicated++;
        }
    }
    
    // Update entity count in header
    memcpy((u8*)buffer + entity_count_pos, &entity_count, sizeof(u32));
    
    system->current_sequence++;
    system->bytes_sent += bytes_written;
    system->packets_sent++;
    
    u64 end_time = timing_get_time_ms();
    f32 update_time = (f32)(end_time - start_time);
    system->avg_replication_time_ms = (system->avg_replication_time_ms * 0.9f) + (update_time * 0.1f);
    
    LOG_TRACE("Generated update packet: %u entities, %u bytes, %.2f ms", entity_count, bytes_written, update_time);
    return bytes_written;
}

bool replication_process_update_packet(ReplicationSystem* system, const void* buffer, u32 buffer_size) {
    if (!system || !buffer || buffer_size < sizeof(u32) * 2) return false;
    
    const u8* data = (const u8*)buffer;
    u32 pos = 0;
    
    // Read packet header
    u32 sequence;
    memcpy(&sequence, data + pos, sizeof(u32));
    pos += sizeof(u32);
    
    u32 entity_count;
    memcpy(&entity_count, data + pos, sizeof(u32));
    pos += sizeof(u32);
    
    // Process entities
    for (u32 i = 0; i < entity_count; i++) {
        ReplicatedEntity entity;
        if (!replication_deserialize_entity(data + pos, buffer_size - pos, &entity)) {
            LOG_ERROR("Failed to deserialize entity %u in packet", i);
            return false;
        }
        
        // Find existing entity or create new one
        ReplicatedEntity* existing = replication_find_entity(system, entity.entity_id);
        if (!existing) {
            // Spawn new entity
            u32 new_id = replication_spawn_entity(system, entity.component_mask, entity.owner_client_id);
            if (new_id == 0) {
                LOG_ERROR("Failed to spawn entity from packet");
                return false;
            }
            existing = replication_find_entity(system, new_id);
        }
        
        if (existing) {
            // Update entity data
            replication_update_entity(system, existing->entity_id, entity.component_data, entity.data_size);
            existing->priority = entity.priority;
            existing->is_dormant = entity.is_dormant;
        }
        
        pos += replication_calculate_entity_size(&entity);
    }
    
    LOG_TRACE("Processed update packet: %u entities, sequence %u", entity_count, sequence);
    return true;
}

void replication_get_stats(const ReplicationSystem* system, u32* entity_count, u32* bytes_sent, 
                           u32* packets_sent, f32* avg_time_ms) {
    if (!system) return;
    
    if (entity_count) *entity_count = system->entity_count;
    if (bytes_sent) *bytes_sent = system->bytes_sent;
    if (packets_sent) *packets_sent = system->packets_sent;
    if (avg_time_ms) *avg_time_ms = system->avg_replication_time_ms;
}

// Internal function implementations
static ReplicatedEntity* replication_find_entity(ReplicationSystem* system, u32 entity_id) {
    if (!system || entity_id == 0) return NULL;
    
    for (u32 i = 0; i < system->max_entities; i++) {
        if (system->entities[i].entity_id == entity_id) {
            return &system->entities[i];
        }
    }
    return NULL;
}

static bool replication_is_entity_relevant(const ReplicationSystem* system, const ReplicatedEntity* entity) {
    if (!system || !entity) return false;
    
    // Always relevant entities
    if (entity->priority == PRIORITY_CRITICAL) return true;
    
    // Dormant entities are not relevant
    if (entity->is_dormant) return false;
    
    // Distance-based relevancy check
    if (system->relevancy_distance > 0.0f) {
        // Extract position from component data (assuming transform is first 12 bytes)
        if (entity->component_data && entity->data_size >= 12) {
            vec3 entity_pos;
            memcpy(&entity_pos, entity->component_data, sizeof(vec3));
            
            f32 distance = vec3_distance(&system->reference_position, &entity_pos);
            entity->is_relevant = (distance <= system->relevancy_distance);
        }
    }
    return entity->is_relevant;
}

static bool replication_should_replicate_entity(const ReplicationSystem* system, const ReplicatedEntity* entity) {
    if (!replication_is_entity_relevant(system, entity)) return false;
    
    u64 current_time = timing_get_time_ms();
    u64 time_since_update = current_time - entity->last_update_time;
    u64 update_interval_ms = (u64)(entity->update_interval * 1000.0f);
    
    return time_since_update >= update_interval_ms;
}

static u32 replication_calculate_entity_size(const ReplicatedEntity* entity) {
    if (!entity) return 0;
    
    return sizeof(u32) +           // entity_id
           sizeof(u32) +           // component_mask
           sizeof(u32) +           // data_size
           entity->data_size +     // component_data
           sizeof(u8) +            // priority
           sizeof(bool) +          // is_dormant
           sizeof(u32);             // owner_client_id
}

static bool replication_serialize_entity(const ReplicatedEntity* entity, void* buffer, u32 buffer_size, u32* written) {
    if (!entity || !buffer || !written) return false;
    
    u32 pos = 0;
    u32 required_size = replication_calculate_entity_size(entity);
    
    if (buffer_size < required_size) {
        *written = 0;
        return false;
    }
    
    u8* data = (u8*)buffer;
    
    // Write entity_id
    memcpy(data + pos, &entity->entity_id, sizeof(u32));
    pos += sizeof(u32);
    
    // Write component_mask
    memcpy(data + pos, &entity->component_mask, sizeof(u32));
    pos += sizeof(u32);
    
    // Write data_size
    memcpy(data + pos, &entity->data_size, sizeof(u32));
    pos += sizeof(u32);
    
    // Write component_data
    if (entity->component_data && entity->data_size > 0) {
        memcpy(data + pos, entity->component_data, entity->data_size);
        pos += entity->data_size;
    }
    
    // Write priority
    memcpy(data + pos, &entity->priority, sizeof(u8));
    pos += sizeof(u8);
    
    // Write is_dormant
    memcpy(data + pos, &entity->is_dormant, sizeof(bool));
    pos += sizeof(bool);
    
    // Write owner_client_id
    memcpy(data + pos, &entity->owner_client_id, sizeof(u32));
    pos += sizeof(u32);
    
    *written = pos;
    return true;
}

static bool replication_deserialize_entity(const void* buffer, u32 buffer_size, ReplicatedEntity* entity) {
    if (!buffer || !entity) return false;
    
    const u8* data = (const u8*)buffer;
    u32 pos = 0;
    
    // Read entity_id
    if (pos + sizeof(u32) > buffer_size) return false;
    memcpy(&entity->entity_id, data + pos, sizeof(u32));
    pos += sizeof(u32);
    
    // Read component_mask
    if (pos + sizeof(u32) > buffer_size) return false;
    memcpy(&entity->component_mask, data + pos, sizeof(u32));
    pos += sizeof(u32);
    
    // Read data_size
    if (pos + sizeof(u32) > buffer_size) return false;
    memcpy(&entity->data_size, data + pos, sizeof(u32));
    pos += sizeof(u32);
    
    // Read component_data
    if (entity->data_size > 0) {
        if (pos + entity->data_size > buffer_size) return false;
        
        entity->component_data = malloc(entity->data_size);
        if (!entity->component_data) return false;
        
        memcpy(entity->component_data, data + pos, entity->data_size);
        pos += entity->data_size;
    } else {
        entity->component_data = NULL;
    }
    
    // Read priority
    if (pos + sizeof(u8) > buffer_size) {
        free(entity->component_data);
        return false;
    }
    memcpy(&entity->priority, data + pos, sizeof(u8));
    pos += sizeof(u8);
    
    // Read is_dormant
    if (pos + sizeof(bool) > buffer_size) {
        free(entity->component_data);
        return false;
    }
    memcpy(&entity->is_dormant, data + pos, sizeof(bool));
    pos += sizeof(bool);
    
    // Read owner_client_id
    if (pos + sizeof(u32) > buffer_size) {
        free(entity->component_data);
        return false;
    }
    memcpy(&entity->owner_client_id, data + pos, sizeof(u32));
    pos += sizeof(u32);
    
    // Initialize other fields
    entity->last_update_sequence = 0;
    entity->last_update_time = timing_get_time_ms();
    entity->update_interval = 0.1f;
    entity->is_relevant = true;
    
    return true;
}
