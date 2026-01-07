/*
 * foliage_interaction.c
 * Player interaction
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement terrain LOD
 * TODO: Add terrain tessellation
 * TODO: Implement heightmap streaming
 * TODO: Add splat map rendering
 * TODO: Implement vegetation instancing
 * TODO: Add grass rendering
 * TODO: Implement procedural terrain
 * TODO: Add erosion simulation
 * TODO: Implement virtual heightmaps
 * TODO: Add terrain holes
 * TODO: Implement foliage interaction initialization
 * TODO: Add foliage interaction cleanup/shutdown
 * TODO: Implement foliage interaction validation
 * TODO: Add foliage interaction error handling
 * TODO: Implement foliage interaction serialization
 * TODO: Add foliage interaction debug output
 * TODO: Implement foliage interaction unit tests
 * TODO: Add foliage interaction performance counters
 * TODO: Implement foliage interaction hot-reload
 * TODO: Add foliage interaction thread safety
 * TODO: Implement foliage interaction memory pooling
 * TODO: Add foliage interaction caching layer
 * TODO: Implement foliage interaction async operations
 * TODO: Add foliage interaction GPU integration
 * TODO: Implement foliage interaction SIMD optimization
 * TODO: Add foliage interaction batch processing
 * TODO: Implement foliage interaction streaming support
 * TODO: Add foliage interaction LOD support
 * TODO: Implement foliage interaction culling integration
 * TODO: Add foliage interaction render graph node
 */

#include "environment/landscape/terrain/landscape/vegetation/foliage_interaction.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LANDSCAPE_FOLIAGE_INTERACTION_MAX_COUNT 4096
#define LANDSCAPE_FOLIAGE_INTERACTION_DEFAULT_CAPACITY 256
#define LANDSCAPE_FOLIAGE_INTERACTION_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_foliage_interaction_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} landscape_foliage_interaction_internal_t;

typedef struct landscape_foliage_interaction_context {
    landscape_foliage_interaction_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} landscape_foliage_interaction_context_t;

static landscape_foliage_interaction_context_t g_foliage_interaction_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

#include <include/math/math.h>

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool landscape_foliage_interaction_validate(const landscape_foliage_interaction_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void landscape_foliage_interaction_cleanup_internal(landscape_foliage_interaction_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

// Internal state
typedef struct foliage_interaction_state {
    float global_range;
    float global_stiffness;
    
    foliage_influencer_t* influencers;
    uint32_t influencer_count;
    uint32_t influencer_capacity;
} foliage_interaction_state_t;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int landscape_foliage_interaction_init(void) {
    if (g_foliage_interaction_ctx.initialized) {
        return 0; // Already initialized
    }

    g_foliage_interaction_ctx.capacity = LANDSCAPE_FOLIAGE_INTERACTION_DEFAULT_CAPACITY;
    g_foliage_interaction_ctx.items = calloc(g_foliage_interaction_ctx.capacity, sizeof(landscape_foliage_interaction_internal_t));
    if (!g_foliage_interaction_ctx.items) {
        return -1;
    }

    g_foliage_interaction_ctx.count = 0;
    g_foliage_interaction_ctx.initialized = true;

    return 0;
}

void landscape_foliage_interaction_shutdown(void) {
    if (!g_foliage_interaction_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_foliage_interaction_ctx.count; i++) {
        landscape_foliage_interaction_cleanup_internal(&g_foliage_interaction_ctx.items[i]);
    }

    free(g_foliage_interaction_ctx.items);
    g_foliage_interaction_ctx.items = NULL;
    g_foliage_interaction_ctx.count = 0;
    g_foliage_interaction_ctx.capacity = 0;
    g_foliage_interaction_ctx.initialized = false;
}

int landscape_foliage_interaction_create(landscape_foliage_interaction_handle_t* out_handle, const landscape_foliage_interaction_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_foliage_interaction_ctx.initialized) {
        return -2;
    }

    if (g_foliage_interaction_ctx.count >= g_foliage_interaction_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_foliage_interaction_ctx.count++;
    landscape_foliage_interaction_internal_t* item = &g_foliage_interaction_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Initialize internal state
    foliage_interaction_state_t* state = malloc(sizeof(foliage_interaction_state_t));
    if (!state) return -4;
    
    state->global_range = desc->global_range > 0.0f ? desc->global_range : 2.0f;
    state->global_stiffness = desc->global_stiffness > 0.0f ? desc->global_stiffness : 0.5f;
    state->influencer_count = 0;
    state->influencer_capacity = 64; // Should be enough for nearby entities
    state->influencers = malloc(state->influencer_capacity * sizeof(foliage_influencer_t));
    
    if (!state->influencers) {
        free(state);
        return -5;
    }

    item->data = state;
    item->data_size = sizeof(foliage_interaction_state_t);
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void landscape_foliage_interaction_destroy(landscape_foliage_interaction_handle_t handle) {
    if (handle.id >= g_foliage_interaction_ctx.count) {
        return;
    }
    
    landscape_foliage_interaction_internal_t* item = &g_foliage_interaction_ctx.items[handle.id];
    foliage_interaction_state_t* state = (foliage_interaction_state_t*)item->data;
    
    if (state) {
        if (state->influencers) free(state->influencers);
    }

    landscape_foliage_interaction_cleanup_internal(item);
}

int landscape_foliage_interaction_set_influencers(
    landscape_foliage_interaction_handle_t handle,
    const foliage_influencer_t* influencers,
    uint32_t count
) {
    if (handle.id >= g_foliage_interaction_ctx.count) return -1;
    landscape_foliage_interaction_internal_t* item = &g_foliage_interaction_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    foliage_interaction_state_t* state = (foliage_interaction_state_t*)item->data;
    
    if (count > state->influencer_capacity) {
        // Simple cap for now, or realloc
        count = state->influencer_capacity;
    }
    
    if (influencers && count > 0) {
        memcpy(state->influencers, influencers, count * sizeof(foliage_influencer_t));
    }
    
    state->influencer_count = count;
    item->dirty = true;
    
    return 0;
}

Vec3 landscape_foliage_interaction_get_displacement(
    landscape_foliage_interaction_handle_t handle,
    const Vec3* position
) {
    Vec3 displacement = {0, 0, 0};
    
    if (handle.id >= g_foliage_interaction_ctx.count) return displacement;
    landscape_foliage_interaction_internal_t* item = &g_foliage_interaction_ctx.items[handle.id];
    if (!item->initialized) return displacement;
    
    foliage_interaction_state_t* state = (foliage_interaction_state_t*)item->data;
    
    // Sum forces from nearby influencers
    for (uint32_t i = 0; i < state->influencer_count; i++) {
        const foliage_influencer_t* inf = &state->influencers[i];
        
        float dx = position->x - inf->position.x;
        float dy = position->y - inf->position.y;
        float dz = position->z - inf->position.z;
        float dist_sq = dx*dx + dy*dy + dz*dz;
        float radius_sq = inf->radius * inf->radius;
        
        if (dist_sq < radius_sq) {
            float dist = sqrtf(dist_sq);
            if (dist > 0.001f) {
                // Push away from center
                float strength = (1.0f - dist / inf->radius) * inf->strength;
                displacement.x += (dx / dist) * strength;
                displacement.z += (dz / dist) * strength;
                // Usually flatten (push down) is better than pushing up/down radially in Y
                // displacement.y += ...
            }
        }
    }
    
    // Clamp or dampen?
    // For now return sum
    
    return displacement;
}

int landscape_foliage_interaction_update(landscape_foliage_interaction_handle_t handle, float delta_time) {
    if (handle.id >= g_foliage_interaction_ctx.count) {
        return -1;
    }

    landscape_foliage_interaction_internal_t* item = &g_foliage_interaction_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    // Nothing to simulate over time really, unless we add spring physics to return to rest
    return 0;
}

int landscape_foliage_interaction_update_internal(landscape_foliage_interaction_handle_t handle, const void* data, size_t size) {
    return 0;
}

bool landscape_foliage_interaction_is_valid(landscape_foliage_interaction_handle_t handle) {
    if (handle.id >= g_foliage_interaction_ctx.count) {
        return false;
    }
    return g_foliage_interaction_ctx.items[handle.id].initialized;
}

int landscape_foliage_interaction_get_info(landscape_foliage_interaction_handle_t handle, landscape_foliage_interaction_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_foliage_interaction_ctx.count) {
        return -2;
    }

    const landscape_foliage_interaction_internal_t* item = &g_foliage_interaction_ctx.items[handle.id];
    const foliage_interaction_state_t* state = (const foliage_interaction_state_t*)item->data;
    
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->active_influencers = state ? state->influencer_count : 0;

    return 0;
}

void landscape_foliage_interaction_mark_dirty(landscape_foliage_interaction_handle_t handle) {
    if (handle.id < g_foliage_interaction_ctx.count) {
        g_foliage_interaction_ctx.items[handle.id].dirty = true;
    }
}

int landscape_foliage_interaction_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_foliage_interaction_ctx.count; i++) {
        landscape_foliage_interaction_internal_t* item = &g_foliage_interaction_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t landscape_foliage_interaction_get_count(void) {
    return g_foliage_interaction_ctx.count;
}

size_t landscape_foliage_interaction_get_memory_usage(void) {
    size_t total = sizeof(g_foliage_interaction_ctx);
    total += g_foliage_interaction_ctx.capacity * sizeof(landscape_foliage_interaction_internal_t);

    for (uint32_t i = 0; i < g_foliage_interaction_ctx.count; i++) {
        landscape_foliage_interaction_internal_t* item = &g_foliage_interaction_ctx.items[i];
        total += item->data_size;
        
        foliage_interaction_state_t* state = (foliage_interaction_state_t*)item->data;
        if (state && state->influencers) {
            total += state->influencer_capacity * sizeof(foliage_influencer_t);
        }
    }

    return total;
}

void landscape_foliage_interaction_debug_print(void) {
    // Debug output
}

/* End of foliage_interaction.c */
