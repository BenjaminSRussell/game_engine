#include "network_state_sync.h"
#include "core/common/memory/allocator.h"
#include "core/logger.h"
#include "network_manager.h"
#include <string.h>
#include <stdlib.h>

#define MAX_SYNCED_ENTITIES 4096
#define MAX_STATE_SNAPSHOT_SIZE (1024 * 1024)  // 1MB
#define SYNC_HISTORY_SIZE 60  // 1 second at 60 FPS
#define DELTA_COMPRESSION_THRESHOLD 256

typedef struct state_snapshot {
    uint32_t frame_number;
    double timestamp;
    uint8_t* data;
    size_t data_size;
    uint32_t entity_count;
    bool is_delta;
    uint32_t base_frame;  // For delta compression
} state_snapshot_t;

typedef struct entity_state {
    uint32_t entity_id;
    uint32_t last_updated_frame;
    uint8_t* current_state;
    size_t state_size;
    uint8_t* previous_state;
    bool dirty;
    uint8_t priority;  // 0=low, 1=medium, 2=high
} entity_state_t;

typedef struct state_sync_system {
    entity_state_t entities[MAX_SYNCED_ENTITIES];
    uint32_t entity_count;
    
    state_snapshot_t snapshot_history[SYNC_HISTORY_SIZE];
    uint32_t history_head;
    uint32_t history_count;
    
    uint32_t current_frame;
    double current_time;
    
    // Delta compression
    uint8_t* delta_buffer;
    size_t delta_buffer_size;
    
    // Bandwidth management
    uint32_t bandwidth_budget;
    uint32_t bandwidth_used;
    double last_bandwidth_reset;
    
    // Priority queues
    uint32_t high_priority_entities[MAX_SYNCED_ENTITIES];
    uint32_t high_priority_count;
    uint32_t medium_priority_entities[MAX_SYNCED_ENTITIES];
    uint32_t medium_priority_count;
    uint32_t low_priority_entities[MAX_SYNCED_ENTITIES];
    uint32_t low_priority_count;
    
    bool initialized;
} state_sync_system_t;

static state_sync_system_t g_state_sync = {0};

bool state_sync_init(void) {
    if (g_state_sync.initialized) {
        return true;
    }
    
    memset(&g_state_sync, 0, sizeof(g_state_sync));
    
    g_state_sync.delta_buffer = memory_alloc(MAX_STATE_SNAPSHOT_SIZE);
    if (!g_state_sync.delta_buffer) {
        log_error("Failed to allocate delta buffer for state sync");
        return false;
    }
    g_state_sync.delta_buffer_size = MAX_STATE_SNAPSHOT_SIZE;
    
    g_state_sync.bandwidth_budget = 131072;  // 128KB per frame
    g_state_sync.last_bandwidth_reset = g_state_sync.current_time;
    
    g_state_sync.initialized = true;
    log_info("State synchronization system initialized");
    return true;
}

void state_sync_shutdown(void) {
    if (!g_state_sync.initialized) {
        return;
    }
    
    // Clean up entity states
    for (uint32_t i = 0; i < g_state_sync.entity_count; i++) {
        if (g_state_sync.entities[i].current_state) {
            memory_free(g_state_sync.entities[i].current_state);
        }
        if (g_state_sync.entities[i].previous_state) {
            memory_free(g_state_sync.entities[i].previous_state);
        }
    }
    
    // Clean up snapshot history
    for (uint32_t i = 0; i < g_state_sync.history_count; i++) {
        uint32_t index = (g_state_sync.history_head - i + SYNC_HISTORY_SIZE) % SYNC_HISTORY_SIZE;
        if (g_state_sync.snapshot_history[index].data) {
            memory_free(g_state_sync.snapshot_history[index].data);
        }
    }
    
    if (g_state_sync.delta_buffer) {
        memory_free(g_state_sync.delta_buffer);
    }
    
    memset(&g_state_sync, 0, sizeof(g_state_sync));
    log_info("State synchronization system shutdown");
}

bool state_sync_register_entity(uint32_t entity_id, size_t state_size, uint8_t priority) {
    if (!g_state_sync.initialized || g_state_sync.entity_count >= MAX_SYNCED_ENTITIES) {
        return false;
    }
    
    // Check if entity already exists
    for (uint32_t i = 0; i < g_state_sync.entity_count; i++) {
        if (g_state_sync.entities[i].entity_id == entity_id) {
            log_warn("Entity %u already registered for state sync", entity_id);
            return false;
        }
    }
    
    entity_state_t* entity = &g_state_sync.entities[g_state_sync.entity_count];
    entity->entity_id = entity_id;
    entity->state_size = state_size;
    entity->priority = priority;
    entity->dirty = true;
    entity->last_updated_frame = 0;
    
    entity->current_state = memory_alloc(state_size);
    entity->previous_state = memory_alloc(state_size);
    
    if (!entity->current_state || !entity->previous_state) {
        log_error("Failed to allocate state memory for entity %u", entity_id);
        if (entity->current_state) memory_free(entity->current_state);
        if (entity->previous_state) memory_free(entity->previous_state);
        return false;
    }
    
    memset(entity->current_state, 0, state_size);
    memset(entity->previous_state, 0, state_size);
    
    // Add to appropriate priority queue
    switch (priority) {
        case 2:  // High
            g_state_sync.high_priority_entities[g_state_sync.high_priority_count++] = entity_id;
            break;
        case 1:  // Medium
            g_state_sync.medium_priority_entities[g_state_sync.medium_priority_count++] = entity_id;
            break;
        case 0:  // Low
            g_state_sync.low_priority_entities[g_state_sync.low_priority_count++] = entity_id;
            break;
    }
    
    g_state_sync.entity_count++;
    log_debug("Registered entity %u for state sync with size %zu", entity_id, state_size);
    return true;
}

bool state_sync_unregister_entity(uint32_t entity_id) {
    if (!g_state_sync.initialized) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_state_sync.entity_count; i++) {
        if (g_state_sync.entities[i].entity_id == entity_id) {
            entity_state_t* entity = &g_state_sync.entities[i];
            
            if (entity->current_state) memory_free(entity->current_state);
            if (entity->previous_state) memory_free(entity->previous_state);
            
            // Remove from entity list
            memmove(&g_state_sync.entities[i], &g_state_sync.entities[i + 1],
                    (g_state_sync.entity_count - i - 1) * sizeof(entity_state_t));
            g_state_sync.entity_count--;
            
            // Remove from priority queues
            // Note: This is O(n) but fine for our use case
            for (uint32_t j = 0; j < g_state_sync.high_priority_count; j++) {
                if (g_state_sync.high_priority_entities[j] == entity_id) {
                    memmove(&g_state_sync.high_priority_entities[j],
                            &g_state_sync.high_priority_entities[j + 1],
                            (g_state_sync.high_priority_count - j - 1) * sizeof(uint32_t));
                    g_state_sync.high_priority_count--;
                    break;
                }
            }
            
            log_debug("Unregistered entity %u from state sync", entity_id);
            return true;
        }
    }
    
    return false;
}

bool state_sync_update_entity(uint32_t entity_id, const void* state_data, size_t state_size) {
    if (!g_state_sync.initialized) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_state_sync.entity_count; i++) {
        entity_state_t* entity = &g_state_sync.entities[i];
        if (entity->entity_id == entity_id) {
            if (state_size != entity->state_size) {
                log_error("State size mismatch for entity %u: expected %zu, got %zu",
                         entity_id, entity->state_size, state_size);
                return false;
            }
            
            // Swap previous and current state
            void* temp = entity->previous_state;
            entity->previous_state = entity->current_state;
            entity->current_state = temp;
            
            // Copy new state
            memcpy(entity->current_state, state_data, state_size);
            entity->dirty = true;
            entity->last_updated_frame = g_state_sync.current_frame;
            
            return true;
        }
    }
    
    return false;
}

static size_t compress_delta(const uint8_t* base_data, const uint8_t* new_data,
                           size_t data_size, uint8_t* delta_buffer) {
    size_t delta_size = 0;
    
    for (size_t i = 0; i < data_size; ) {
        // Find first differing byte
        size_t start = i;
        while (i < data_size && base_data[i] == new_data[i]) {
            i++;
        }
        
        if (i >= data_size) break;
        
        // Find run of differences
        size_t diff_start = i;
        while (i < data_size && base_data[i] != new_data[i]) {
            i++;
        }
        size_t diff_length = i - diff_start;
        
        // Write delta entry: [start_offset][length][data...]
        if (delta_size + 8 + diff_length < MAX_STATE_SNAPSHOT_SIZE) {
            // Write start offset (varint)
            uint32_t offset = diff_start;
            while (offset >= 0x80) {
                delta_buffer[delta_size++] = (offset & 0x7F) | 0x80;
                offset >>= 7;
            }
            delta_buffer[delta_size++] = offset;
            
            // Write length (varint)
            uint32_t length = diff_length;
            while (length >= 0x80) {
                delta_buffer[delta_size++] = (length & 0x7F) | 0x80;
                length >>= 7;
            }
            delta_buffer[delta_size++] = length;
            
            // Write diff data
            memcpy(delta_buffer + delta_size, new_data + diff_start, diff_length);
            delta_size += diff_length;
        }
    }
    
    return delta_size;
}

static size_t apply_delta(const uint8_t* base_data, const uint8_t* delta_data,
                         size_t delta_size, uint8_t* output_data, size_t output_size) {
    memcpy(output_data, base_data, output_size);
    
    size_t delta_pos = 0;
    while (delta_pos < delta_size) {
        // Read start offset
        uint32_t offset = 0;
        int shift = 0;
        do {
            if (delta_pos >= delta_size) return 0;
            offset |= (delta_data[delta_pos] & 0x7F) << shift;
            shift += 7;
        } while (delta_data[delta_pos++] & 0x80);
        
        // Read length
        uint32_t length = 0;
        shift = 0;
        do {
            if (delta_pos >= delta_size) return 0;
            length |= (delta_data[delta_pos] & 0x7F) << shift;
            shift += 7;
        } while (delta_data[delta_pos++] & 0x80);
        
        // Apply diff
        if (offset + length > output_size || delta_pos + length > delta_size) {
            return 0;  // Invalid delta
        }
        
        memcpy(output_data + offset, delta_data + delta_pos, length);
        delta_pos += length;
    }
    
    return output_size;
}

static state_snapshot_t* create_snapshot(bool use_delta_compression) {
    if (g_state_sync.entity_count == 0) {
        return NULL;
    }
    
    state_snapshot_t* snapshot = &g_state_sync.snapshot_history[g_state_sync.history_head];
    
    // Clean up previous snapshot data
    if (snapshot->data) {
        memory_free(snapshot->data);
    }
    
    // Calculate total size needed
    size_t total_size = sizeof(uint32_t) * 2;  // entity_count + frame_number
    for (uint32_t i = 0; i < g_state_sync.entity_count; i++) {
        total_size += sizeof(uint32_t) + sizeof(uint32_t) + g_state_sync.entities[i].state_size;
    }
    
    snapshot->data = memory_alloc(total_size);
    if (!snapshot->data) {
        log_error("Failed to allocate snapshot data");
        return NULL;
    }
    
    uint8_t* ptr = snapshot->data;
    
    // Write header
    *(uint32_t*)ptr = g_state_sync.entity_count;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = g_state_sync.current_frame;
    ptr += sizeof(uint32_t);
    
    // Write entity states
    for (uint32_t i = 0; i < g_state_sync.entity_count; i++) {
        entity_state_t* entity = &g_state_sync.entities[i];
        
        *(uint32_t*)ptr = entity->entity_id;
        ptr += sizeof(uint32_t);
        *(uint32_t*)ptr = entity->state_size;
        ptr += sizeof(uint32_t);
        
        if (use_delta_compression && snapshot->is_delta) {
            // Find base snapshot
            state_snapshot_t* base_snapshot = NULL;
            for (uint32_t j = 1; j < g_state_sync.history_count && j <= SYNC_HISTORY_SIZE; j++) {
                uint32_t base_index = (g_state_sync.history_head - j + SYNC_HISTORY_SIZE) % SYNC_HISTORY_SIZE;
                if (g_state_sync.snapshot_history[base_index].frame_number == snapshot->base_frame) {
                    base_snapshot = &g_state_sync.snapshot_history[base_index];
                    break;
                }
            }
            
            if (base_snapshot) {
                // Apply delta compression (simplified - would need proper entity-to-data mapping)
                memcpy(ptr, entity->current_state, entity->state_size);
            } else {
                memcpy(ptr, entity->current_state, entity->state_size);
            }
        } else {
            memcpy(ptr, entity->current_state, entity->state_size);
        }
        
        ptr += entity->state_size;
        entity->dirty = false;
    }
    
    snapshot->frame_number = g_state_sync.current_frame;
    snapshot->timestamp = g_state_sync.current_time;
    snapshot->data_size = total_size;
    snapshot->entity_count = g_state_sync.entity_count;
    snapshot->is_delta = use_delta_compression;
    
    return snapshot;
}

bool state_sync_update(double delta_time) {
    if (!g_state_sync.initialized) {
        return false;
    }
    
    g_state_sync.current_time += delta_time;
    g_state_sync.current_frame++;
    
    // Reset bandwidth budget every second
    if (g_state_sync.current_time - g_state_sync.last_bandwidth_reset >= 1.0) {
        g_state_sync.bandwidth_used = 0;
        g_state_sync.last_bandwidth_reset = g_state_sync.current_time;
    }
    
    return true;
}

bool state_sync_create_snapshot(uint8_t** snapshot_data, size_t* snapshot_size) {
    if (!g_state_sync.initialized || !snapshot_data || !snapshot_size) {
        return false;
    }
    
    // Create new snapshot
    bool use_delta = g_state_sync.history_count > 0;
    state_snapshot_t* snapshot = create_snapshot(use_delta);
    
    if (!snapshot) {
        return false;
    }
    
    // Update history
    g_state_sync.history_head = (g_state_sync.history_head + 1) % SYNC_HISTORY_SIZE;
    if (g_state_sync.history_count < SYNC_HISTORY_SIZE) {
        g_state_sync.history_count++;
    }
    
    *snapshot_data = snapshot->data;
    *snapshot_size = snapshot->data_size;
    
    g_state_sync.bandwidth_used += snapshot->data_size;
    
    return true;
}

bool state_sync_apply_snapshot(const uint8_t* snapshot_data, size_t snapshot_size) {
    if (!g_state_sync.initialized || !snapshot_data || snapshot_size < 8) {
        return false;
    }
    
    const uint8_t* ptr = snapshot_data;
    
    // Read header
    uint32_t entity_count = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    uint32_t frame_number = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    
    if (snapshot_size < 8 + entity_count * (sizeof(uint32_t) * 2)) {
        log_error("Invalid snapshot size");
        return false;
    }
    
    // Apply entity states
    for (uint32_t i = 0; i < entity_count; i++) {
        uint32_t entity_id = *(const uint32_t*)ptr;
        ptr += sizeof(uint32_t);
        uint32_t state_size = *(const uint32_t*)ptr;
        ptr += sizeof(uint32_t);
        
        if (ptr + state_size > snapshot_data + snapshot_size) {
            log_error("Snapshot data overflow");
            return false;
        }
        
        // Find entity and update state
        for (uint32_t j = 0; j < g_state_sync.entity_count; j++) {
            if (g_state_sync.entities[j].entity_id == entity_id) {
                if (state_size != g_state_sync.entities[j].state_size) {
                    log_error("State size mismatch for entity %u in snapshot", entity_id);
                    return false;
                }
                
                memcpy(g_state_sync.entities[j].current_state, ptr, state_size);
                g_state_sync.entities[j].last_updated_frame = frame_number;
                break;
            }
        }
        
        ptr += state_size;
    }
    
    return true;
}

void state_sync_get_statistics(uint32_t* synced_entities, uint32_t* bandwidth_used,
                             uint32_t* snapshot_count) {
    if (!g_state_sync.initialized) {
        return;
    }
    
    if (synced_entities) *synced_entities = g_state_sync.entity_count;
    if (bandwidth_used) *bandwidth_used = g_state_sync.bandwidth_used;
    if (snapshot_count) *snapshot_count = g_state_sync.history_count;
}
