/*
 * bvh_update.c
 * Dynamic BVH updates for moving objects
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "geometry/bvh/bvh_update.h"
#include "include/math/vec3.h"
#include "include/math/aabb.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define BVH_UPDATE_DEFAULT_CAPACITY 256
#define BVH_UPDATE_MOVEMENT_THRESHOLD 0.1f  // Minimum movement to trigger update
#define BVH_UPDATE_REBUILD_THRESHOLD 2.0f   // Movement threshold for rebuild
#define BVH_MAX_REFIT_DEPTH 32

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct bvh_node {
    aabb_t bounds;
    aabb_t original_bounds;  // For tracking expansion
    uint32_t first_prim;
    uint32_t prim_count;
    uint32_t left_child;
    uint32_t right_child;
    uint32_t parent;         // For bottom-up refitting
    uint8_t split_axis;
    bool dirty;
} bvh_node_t;

typedef struct bvh_object_state {
    aabb_t bounds;
    aabb_t prev_bounds;
    vec3_t velocity;         // For predictive updates
    uint32_t primitive_id;
    uint32_t leaf_node;      // Which leaf contains this object
    bool moved;
    bool needs_rebuild;
} bvh_object_state_t;

typedef struct geometry_bvh_update_internal {
    uint32_t id;
    uint32_t flags;
    bvh_node_t* nodes;
    bvh_object_state_t* objects;
    uint32_t* dirty_nodes;   // Stack of dirty node indices
    uint32_t node_count;
    uint32_t object_count;
    uint32_t dirty_count;
    float movement_threshold;
    float rebuild_threshold;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} geometry_bvh_update_internal_t;

typedef struct geometry_bvh_update_context {
    geometry_bvh_update_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} geometry_bvh_update_context_t;

static geometry_bvh_update_context_t g_bvh_update_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static inline aabb_t aabb_union(const aabb_t* a, const aabb_t* b) {
    aabb_t result;
    result.min = vec3_min(a->min, b->min);
    result.max = vec3_max(a->max, b->max);
    return result;
}

static inline float aabb_surface_area(const aabb_t* bounds) {
    vec3_t extent = vec3_sub(bounds->max, bounds->min);
    return 2.0f * (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x);
}

static inline float vec3_distance(vec3_t a, vec3_t b) {
    vec3_t diff = vec3_sub(a, b);
    return vec3_length(diff);
}

/* Refit AABB for a node based on its children */
static void refit_node_bounds(geometry_bvh_update_internal_t* update, uint32_t node_idx) {
    if (node_idx >= update->node_count) return;
    
    bvh_node_t* node = &update->nodes[node_idx];
    
    // Leaf node - bounds come from objects
    if (node->prim_count > 0) {
        node->bounds.min = vec3_set(FLT_MAX, FLT_MAX, FLT_MAX);
        node->bounds.max = vec3_set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        
        for (uint32_t i = 0; i < node->prim_count; i++) {
            uint32_t obj_idx = node->first_prim + i;
            if (obj_idx < update->object_count) {
                node->bounds = aabb_union(&node->bounds, &update->objects[obj_idx].bounds);
            }
        }
        return;
    }
    
    // Interior node - union of children
    if (node->left_child < update->node_count && node->right_child < update->node_count) {
        const aabb_t* left_bounds = &update->nodes[node->left_child].bounds;
        const aabb_t* right_bounds = &update->nodes[node->right_child].bounds;
        node->bounds = aabb_union(left_bounds, right_bounds);
    }
}

/* Mark node and all ancestors as dirty */
static void mark_node_dirty(geometry_bvh_update_internal_t* update, uint32_t node_idx) {
    while (node_idx < update->node_count) {
        bvh_node_t* node = &update->nodes[node_idx];
        
        if (node->dirty) {
            break;  // Already dirty, ancestors are too
        }
        
        node->dirty = true;
        
        // Add to dirty stack if not already there
        if (update->dirty_count < update->node_count) {
            update->dirty_nodes[update->dirty_count++] = node_idx;
        }
        
        // Move to parent
        if (node->parent == UINT32_MAX) {
            break;
        }
        node_idx = node->parent;
    }
}

/* Incremental bottom-up refitting */
static void refit_dirty_nodes(geometry_bvh_update_internal_t* update) {
    // Process dirty nodes from leaves to root
    for (uint32_t i = 0; i < update->dirty_count; i++) {
        uint32_t node_idx = update->dirty_nodes[i];
        refit_node_bounds(update, node_idx);
        update->nodes[node_idx].dirty = false;
    }
    
    update->dirty_count = 0;
}

/* Check if object needs rebuild (moved too far) */
static bool needs_rebuild(const bvh_object_state_t* obj, float threshold) {
    vec3_t center_old = vec3_scale(vec3_add(obj->prev_bounds.min, obj->prev_bounds.max), 0.5f);
    vec3_t center_new = vec3_scale(vec3_add(obj->bounds.min, obj->bounds.max), 0.5f);
    
    float dist = vec3_distance(center_old, center_new);
    return dist > threshold;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool geometry_bvh_update_validate(const geometry_bvh_update_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void geometry_bvh_update_cleanup_internal(geometry_bvh_update_internal_t* item) {
    if (!item) return;
    
    if (item->nodes) {
        free(item->nodes);
        item->nodes = NULL;
    }
    if (item->objects) {
        free(item->objects);
        item->objects = NULL;
    }
    if (item->dirty_nodes) {
        free(item->dirty_nodes);
        item->dirty_nodes = NULL;
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_bvh_update_init(void) {
    if (g_bvh_update_ctx.initialized) {
        return 0;
    }
    
    g_bvh_update_ctx.capacity = BVH_UPDATE_DEFAULT_CAPACITY;
    g_bvh_update_ctx.items = calloc(g_bvh_update_ctx.capacity, sizeof(geometry_bvh_update_internal_t));
    if (!g_bvh_update_ctx.items) {
        return -1;
    }
    
    g_bvh_update_ctx.count = 0;
    g_bvh_update_ctx.initialized = true;
    
    return 0;
}

void geometry_bvh_update_shutdown(void) {
    if (!g_bvh_update_ctx.initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < g_bvh_update_ctx.count; i++) {
        geometry_bvh_update_cleanup_internal(&g_bvh_update_ctx.items[i]);
    }
    
    free(g_bvh_update_ctx.items);
    g_bvh_update_ctx.items = NULL;
    g_bvh_update_ctx.count = 0;
    g_bvh_update_ctx.capacity = 0;
    g_bvh_update_ctx.initialized = false;
}

int geometry_bvh_update_create(geometry_bvh_update_handle_t* out_handle, const geometry_bvh_update_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_bvh_update_ctx.initialized) {
        return -2;
    }
    
    if (g_bvh_update_ctx.count >= g_bvh_update_ctx.capacity) {
        return -3;
    }
    
    uint32_t index = g_bvh_update_ctx.count++;
    geometry_bvh_update_internal_t* item = &g_bvh_update_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    item->nodes = NULL;
    item->objects = NULL;
    item->dirty_nodes = NULL;
    item->node_count = 0;
    item->object_count = 0;
    item->dirty_count = 0;
    item->movement_threshold = BVH_UPDATE_MOVEMENT_THRESHOLD;
    item->rebuild_threshold = BVH_UPDATE_REBUILD_THRESHOLD;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    out_handle->id = index;
    return 0;
}

void geometry_bvh_update_destroy(geometry_bvh_update_handle_t handle) {
    if (handle.id >= g_bvh_update_ctx.count) {
        return;
    }
    
    geometry_bvh_update_cleanup_internal(&g_bvh_update_ctx.items[handle.id]);
}

int geometry_bvh_update_update(geometry_bvh_update_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_bvh_update_ctx.count) {
        return -1;
    }
    
    geometry_bvh_update_internal_t* item = &g_bvh_update_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    // data should contain updated object bounds
    // For now, just mark as dirty
    item->dirty = true;
    
    return 0;
}

int geometry_bvh_update_move_object(
    geometry_bvh_update_handle_t handle,
    uint32_t object_id,
    const aabb_t* new_bounds
) {
    if (handle.id >= g_bvh_update_ctx.count || !new_bounds) {
        return -1;
    }
    
    geometry_bvh_update_internal_t* item = &g_bvh_update_ctx.items[handle.id];
    if (!item->initialized || object_id >= item->object_count) {
        return -2;
    }
    
    bvh_object_state_t* obj = &item->objects[object_id];
    
    // Check if object actually moved
    vec3_t center_old = vec3_scale(vec3_add(obj->bounds.min, obj->bounds.max), 0.5f);
    vec3_t center_new = vec3_scale(vec3_add(new_bounds->min, new_bounds->max), 0.5f);
    float dist = vec3_distance(center_old, center_new);
    
    if (dist < item->movement_threshold) {
        return 0;  // Movement too small, ignore
    }
    
    // Update velocity for predictive updates
    obj->velocity = vec3_sub(center_new, center_old);
    
    // Store previous bounds
    obj->prev_bounds = obj->bounds;
    obj->bounds = *new_bounds;
    obj->moved = true;
    
    // Check if needs full rebuild
    if (needs_rebuild(obj, item->rebuild_threshold)) {
        obj->needs_rebuild = true;
        item->dirty = true;
        return 1;  // Needs rebuild
    }
    
    // Mark leaf node as dirty for refitting
    if (obj->leaf_node < item->node_count) {
        mark_node_dirty(item, obj->leaf_node);
    }
    
    return 0;
}

int geometry_bvh_update_process_pending(void) {
    int processed = 0;
    
    for (uint32_t i = 0; i < g_bvh_update_ctx.count; i++) {
        geometry_bvh_update_internal_t* item = &g_bvh_update_ctx.items[i];
        if (!item->initialized) continue;
        
        // Refit dirty nodes
        if (item->dirty_count > 0) {
            refit_dirty_nodes(item);
            processed++;
        }
        
        // Check if any objects need rebuild
        bool needs_full_rebuild = false;
        for (uint32_t j = 0; j < item->object_count; j++) {
            if (item->objects[j].needs_rebuild) {
                needs_full_rebuild = true;
                break;
            }
        }
        
        if (needs_full_rebuild) {
            // TODO: Trigger partial or full BVH rebuild
            item->dirty = true;
        }
        
        // Reset moved flags
        for (uint32_t j = 0; j < item->object_count; j++) {
            item->objects[j].moved = false;
            item->objects[j].needs_rebuild = false;
        }
    }
    
    return processed;
}

bool geometry_bvh_update_is_valid(geometry_bvh_update_handle_t handle) {
    if (handle.id >= g_bvh_update_ctx.count) {
        return false;
    }
    return g_bvh_update_ctx.items[handle.id].initialized;
}

int geometry_bvh_update_get_info(geometry_bvh_update_handle_t handle, geometry_bvh_update_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    
    if (handle.id >= g_bvh_update_ctx.count) {
        return -2;
    }
    
    const geometry_bvh_update_internal_t* item = &g_bvh_update_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    return 0;
}

void geometry_bvh_update_mark_dirty(geometry_bvh_update_handle_t handle) {
    if (handle.id < g_bvh_update_ctx.count) {
        g_bvh_update_ctx.items[handle.id].dirty = true;
    }
}

uint32_t geometry_bvh_update_get_count(void) {
    return g_bvh_update_ctx.count;
}

size_t geometry_bvh_update_get_memory_usage(void) {
    size_t total = sizeof(g_bvh_update_ctx);
    total += g_bvh_update_ctx.capacity * sizeof(geometry_bvh_update_internal_t);
    
    for (uint32_t i = 0; i < g_bvh_update_ctx.count; i++) {
        const geometry_bvh_update_internal_t* item = &g_bvh_update_ctx.items[i];
        total += item->node_count * sizeof(bvh_node_t);
        total += item->object_count * sizeof(bvh_object_state_t);
        total += item->node_count * sizeof(uint32_t);  // dirty_nodes
    }
    
    return total;
}

void geometry_bvh_update_debug_print(void) {
    // TODO: Implement debug output
}

/* End of bvh_update.c */
