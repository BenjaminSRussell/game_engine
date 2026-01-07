/*
 * terrain_culling.c
 * Terrain chunk culling
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
 * TODO: Implement terrain culling initialization
 * TODO: Add terrain culling cleanup/shutdown
 * TODO: Implement terrain culling validation
 * TODO: Add terrain culling error handling
 * TODO: Implement terrain culling serialization
 * TODO: Add terrain culling debug output
 * TODO: Implement terrain culling unit tests
 * TODO: Add terrain culling performance counters
 * TODO: Implement terrain culling hot-reload
 * TODO: Add terrain culling thread safety
 * TODO: Implement terrain culling memory pooling
 * TODO: Add terrain culling caching layer
 * TODO: Implement terrain culling async operations
 * TODO: Add terrain culling GPU integration
 * TODO: Implement terrain culling SIMD optimization
 * TODO: Add terrain culling batch processing
 * TODO: Implement terrain culling streaming support
 * TODO: Add terrain culling LOD support
 * TODO: Implement terrain culling culling integration
 * TODO: Add terrain culling render graph node
 */

#include "environment/landscape/terrain/landscape/terrain/terrain_culling.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LANDSCAPE_TERRAIN_CULLING_MAX_COUNT 4096
#define LANDSCAPE_TERRAIN_CULLING_DEFAULT_CAPACITY 256
#define LANDSCAPE_TERRAIN_CULLING_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_terrain_culling_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} landscape_terrain_culling_internal_t;

typedef struct landscape_terrain_culling_context {
    landscape_terrain_culling_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} landscape_terrain_culling_context_t;

static landscape_terrain_culling_context_t g_terrain_culling_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

#include <include/math/math.h>

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool landscape_terrain_culling_validate(const landscape_terrain_culling_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void landscape_terrain_culling_cleanup_internal(landscape_terrain_culling_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

// Internal state
typedef struct terrain_culling_state {
    float max_distance_sq;
    uint32_t last_visible;
} terrain_culling_state_t;

// Frustum plane
typedef struct Plane {
    float x, y, z, w;
} Plane;

static void extract_planes(const float* m, Plane* planes) {
    // Gribb-Hartmann method or similar
    // Left
    planes[0].x = m[3] + m[0];
    planes[0].y = m[7] + m[4];
    planes[0].z = m[11] + m[8];
    planes[0].w = m[15] + m[12];
    // Right
    planes[1].x = m[3] - m[0];
    planes[1].y = m[7] - m[4];
    planes[1].z = m[11] - m[8];
    planes[1].w = m[15] - m[12];
    // Bottom
    planes[2].x = m[3] + m[1];
    planes[2].y = m[7] + m[5];
    planes[2].z = m[11] + m[9];
    planes[2].w = m[15] + m[13];
    // Top
    planes[3].x = m[3] - m[1];
    planes[3].y = m[7] - m[5];
    planes[3].z = m[11] - m[9];
    planes[3].w = m[15] - m[13];
    // Near
    planes[4].x = m[3] + m[2];
    planes[4].y = m[7] + m[6];
    planes[4].z = m[11] + m[10];
    planes[4].w = m[15] + m[14];
    // Far
    planes[5].x = m[3] - m[2];
    planes[5].y = m[7] - m[6];
    planes[5].z = m[11] - m[10];
    planes[5].w = m[15] - m[14];
    
    // Normalize
    for (int i = 0; i < 6; i++) {
        float len = sqrtf(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
        if (len > 0.0f) {
            float inv = 1.0f / len;
            planes[i].x *= inv;
            planes[i].y *= inv;
            planes[i].z *= inv;
            planes[i].w *= inv;
        }
    }
}

// AABB intersection check
static bool frustum_intersects_aabb(const Plane* planes, const terrain_chunk_aabb_t* aabb) {
    for (int i = 0; i < 6; i++) {
        float px = (planes[i].x > 0.0f) ? aabb->max[0] : aabb->min[0];
        float py = (planes[i].y > 0.0f) ? aabb->max[1] : aabb->min[1];
        float pz = (planes[i].z > 0.0f) ? aabb->max[2] : aabb->min[2];
        
        if (planes[i].x * px + planes[i].y * py + planes[i].z * pz + planes[i].w < 0.0f) {
            return false;
        }
    }
    return true;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int landscape_terrain_culling_init(void) {
    if (g_terrain_culling_ctx.initialized) {
        return 0; // Already initialized
    }

    g_terrain_culling_ctx.capacity = LANDSCAPE_TERRAIN_CULLING_DEFAULT_CAPACITY;
    g_terrain_culling_ctx.items = calloc(g_terrain_culling_ctx.capacity, sizeof(landscape_terrain_culling_internal_t));
    if (!g_terrain_culling_ctx.items) {
        return -1;
    }

    g_terrain_culling_ctx.count = 0;
    g_terrain_culling_ctx.initialized = true;

    return 0;
}

void landscape_terrain_culling_shutdown(void) {
    if (!g_terrain_culling_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_terrain_culling_ctx.count; i++) {
        landscape_terrain_culling_cleanup_internal(&g_terrain_culling_ctx.items[i]);
    }

    free(g_terrain_culling_ctx.items);
    g_terrain_culling_ctx.items = NULL;
    g_terrain_culling_ctx.count = 0;
    g_terrain_culling_ctx.capacity = 0;
    g_terrain_culling_ctx.initialized = false;
}

int landscape_terrain_culling_create(landscape_terrain_culling_handle_t* out_handle, const landscape_terrain_culling_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_terrain_culling_ctx.initialized) {
        return -2;
    }

    if (g_terrain_culling_ctx.count >= g_terrain_culling_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_terrain_culling_ctx.count++;
    landscape_terrain_culling_internal_t* item = &g_terrain_culling_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    terrain_culling_state_t* state = malloc(sizeof(terrain_culling_state_t));
    if (!state) return -4;
    
    float dist = desc->max_distance > 0.0f ? desc->max_distance : 1000.0f;
    state->max_distance_sq = dist * dist;
    state->last_visible = 0;
    
    item->data = state;
    item->data_size = sizeof(terrain_culling_state_t);
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void landscape_terrain_culling_destroy(landscape_terrain_culling_handle_t handle) {
    if (handle.id >= g_terrain_culling_ctx.count) {
        return;
    }

    landscape_terrain_culling_cleanup_internal(&g_terrain_culling_ctx.items[handle.id]);
}

uint32_t landscape_terrain_cull_chunks(
    landscape_terrain_culling_handle_t handle,
    const float* camera_pos,
    const float* view_proj_matrix,
    const terrain_chunk_aabb_t* chunks,
    uint32_t chunk_count,
    uint8_t* out_visibility
) {
    if (handle.id >= g_terrain_culling_ctx.count) return 0;
    landscape_terrain_culling_internal_t* item = &g_terrain_culling_ctx.items[handle.id];
    if (!item->initialized) return 0;
    
    terrain_culling_state_t* state = (terrain_culling_state_t*)item->data;
    uint32_t visible_count = 0;
    
    Plane planes[6];
    extract_planes(view_proj_matrix, planes);
    
    for (uint32_t i = 0; i < chunk_count; i++) {
        // Distance check first (using center of AABB)
        float cx = 0.5f * (chunks[i].min[0] + chunks[i].max[0]);
        float cy = 0.5f * (chunks[i].min[1] + chunks[i].max[1]);
        float cz = 0.5f * (chunks[i].min[2] + chunks[i].max[2]);
        
        float dx = cx - camera_pos[0];
        float dy = cy - camera_pos[1];
        float dz = cz - camera_pos[2];
        float dist_sq = dx*dx + dy*dy + dz*dz;
        
        // Use a slightly larger radius for conservative culling if AABB is large
        // Or just closest point? Center + radius is fast.
        // AABB extent
        float ex = 0.5f * (chunks[i].max[0] - chunks[i].min[0]);
        float ey = 0.5f * (chunks[i].max[1] - chunks[i].min[1]);
        float ez = 0.5f * (chunks[i].max[2] - chunks[i].min[2]);
        float radius_sq = ex*ex + ey*ey + ez*ez;
        
        // If dist to center > max_dist + radius, cull? 
        // Simpler: if min_dist > max_dist
        
        if (dist_sq > state->max_distance_sq + radius_sq * 4.0f) { // Very permissive distance check for now
             if (out_visibility) out_visibility[i] = 0;
             continue;
        }

        if (frustum_intersects_aabb(planes, &chunks[i])) {
            if (out_visibility) out_visibility[i] = 1;
            visible_count++;
        } else {
            if (out_visibility) out_visibility[i] = 0;
        }
    }
    
    state->last_visible = visible_count;
    return visible_count;
}


int landscape_terrain_culling_update(landscape_terrain_culling_handle_t handle, const void* data, size_t size) {
    // Update culling parameters
    return 0;
}

bool landscape_terrain_culling_is_valid(landscape_terrain_culling_handle_t handle) {
    if (handle.id >= g_terrain_culling_ctx.count) {
        return false;
    }
    return g_terrain_culling_ctx.items[handle.id].initialized;
}

int landscape_terrain_culling_get_info(landscape_terrain_culling_handle_t handle, landscape_terrain_culling_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_terrain_culling_ctx.count) {
        return -2;
    }

    const landscape_terrain_culling_internal_t* item = &g_terrain_culling_ctx.items[handle.id];
    const terrain_culling_state_t* state = (const terrain_culling_state_t*)item->data;
    
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    if (state) {
        out_info->last_visible_count = state->last_visible;
    }

    return 0;
}

void landscape_terrain_culling_mark_dirty(landscape_terrain_culling_handle_t handle) {
    if (handle.id < g_terrain_culling_ctx.count) {
        g_terrain_culling_ctx.items[handle.id].dirty = true;
    }
}

int landscape_terrain_culling_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_terrain_culling_ctx.count; i++) {
        landscape_terrain_culling_internal_t* item = &g_terrain_culling_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t landscape_terrain_culling_get_count(void) {
    return g_terrain_culling_ctx.count;
}

size_t landscape_terrain_culling_get_memory_usage(void) {
    size_t total = sizeof(g_terrain_culling_ctx);
    total += g_terrain_culling_ctx.capacity * sizeof(landscape_terrain_culling_internal_t);

    for (uint32_t i = 0; i < g_terrain_culling_ctx.count; i++) {
        total += g_terrain_culling_ctx.items[i].data_size;
    }

    return total;
}

void landscape_terrain_culling_debug_print(void) {
    // Debug output
}

/* End of terrain_culling.c */
