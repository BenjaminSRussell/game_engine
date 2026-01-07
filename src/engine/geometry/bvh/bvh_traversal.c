/*
 * bvh_traversal.c
 * CPU BVH traversal for ray casting and frustum testing
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "bvh_traversal.h"
#include "../../core/math/math/vec3.h"
#include "../../core/math/math/aabb.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define BVH_TRAVERSAL_STACK_SIZE 64
#define BVH_TRAVERSAL_DEFAULT_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct bvh_ray {
    vec3_t origin;
    vec3_t direction;
    vec3_t inv_direction;
    float tmin;
    float tmax;
} bvh_ray_t;

typedef struct bvh_node {
    aabb_t bounds;
    uint32_t first_prim;
    uint32_t prim_count;
    uint32_t left_child;
    uint32_t right_child;
    uint8_t split_axis;
} bvh_node_t;

typedef struct bvh_hit_result {
    bool hit;
    float t;
    uint32_t prim_id;
    vec3_t hit_point;
    vec3_t normal;
} bvh_hit_result_t;

typedef struct geometry_bvh_traversal_internal {
    uint32_t id;
    uint32_t flags;
    bvh_node_t* nodes;
    uint32_t* prim_indices;
    uint32_t node_count;
    uint32_t prim_count;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} geometry_bvh_traversal_internal_t;

typedef struct geometry_bvh_traversal_context {
    geometry_bvh_traversal_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} geometry_bvh_traversal_context_t;

static geometry_bvh_traversal_context_t g_bvh_traversal_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static inline bool ray_aabb_intersect(const bvh_ray_t* ray, const aabb_t* bounds, float* out_tmin, float* out_tmax) {
    vec3_t t0 = vec3_set(
        (bounds->min.x - ray->origin.x) * ray->inv_direction.x,
        (bounds->min.y - ray->origin.y) * ray->inv_direction.y,
        (bounds->min.z - ray->origin.z) * ray->inv_direction.z
    );
    vec3_t t1 = vec3_set(
        (bounds->max.x - ray->origin.x) * ray->inv_direction.x,
        (bounds->max.y - ray->origin.y) * ray->inv_direction.y,
        (bounds->max.z - ray->origin.z) * ray->inv_direction.z
    );
    
    float tmin = fmaxf(fmaxf(fminf(t0.x, t1.x), fminf(t0.y, t1.y)), fminf(t0.z, t1.z));
    float tmax = fminf(fminf(fmaxf(t0.x, t1.x), fmaxf(t0.y, t1.y)), fmaxf(t0.z, t1.z));
    
    if (tmax < 0.0f || tmin > tmax) {
        return false;
    }
    
    *out_tmin = tmin;
    *out_tmax = tmax;
    return true;
}

static inline bool frustum_aabb_intersect(const vec4_t* frustum_planes, const aabb_t* bounds) {
    // Test all 6 frustum planes (left, right, top, bottom, near, far)
    for (int i = 0; i < 6; i++) {
        vec3_t plane_normal = vec3_set(frustum_planes[i].x, frustum_planes[i].y, frustum_planes[i].z);
        float plane_dist = ((vec4_t*)&frustum_planes[i])->w;
        
        // Find positive vertex (furthest in direction of normal)
        vec3_t p_vertex;
        p_vertex.x = (plane_normal.x >= 0.0f) ? bounds->max.x : bounds->min.x;
        p_vertex.y = (plane_normal.y >= 0.0f) ? bounds->max.y : bounds->min.y;
        p_vertex.z = (plane_normal.z >= 0.0f) ? bounds->max.z : bounds->min.z;
        
        // If positive vertex is outside, box is completely outside
        if (vec3_dot(plane_normal, p_vertex) + plane_dist < 0.0f) {
            return false;
        }
    }
    
    return true;
}

/* Iterative BVH traversal for ray intersection */
static bool traverse_bvh_ray(
    const geometry_bvh_traversal_internal_t* traversal,
    const bvh_ray_t* ray,
    bvh_hit_result_t* hit
) {
    if (traversal->node_count == 0) {
        return false;
    }
    
    uint32_t stack[BVH_TRAVERSAL_STACK_SIZE];
    int stack_ptr = 0;
    stack[stack_ptr++] = 0; // Start with root node
    
    bool any_hit = false;
    float closest_t = ray->tmax;
    
    while (stack_ptr > 0) {
        uint32_t node_idx = stack[--stack_ptr];
        const bvh_node_t* node = &traversal->nodes[node_idx];
        
        float tmin, tmax;
        if (!ray_aabb_intersect(ray, &node->bounds, &tmin, &tmax)) {
            continue;
        }
        
        // Skip if we already have a closer hit
        if (tmin > closest_t) {
            continue;
        }
        
        // Leaf node - test primitives
        if (node->prim_count > 0) {
            for (uint32_t i = 0; i < node->prim_count; i++) {
                uint32_t prim_idx = traversal->prim_indices[node->first_prim + i];
                
                // TODO: Actual primitive intersection test
                // For now, just register a hit at the AABB intersection
                if (tmin >= ray->tmin && tmin < closest_t) {
                    any_hit = true;
                    closest_t = tmin;
                    hit->t = tmin;
                    hit->prim_id = prim_idx;
                    hit->hit_point = vec3_add(ray->origin, vec3_scale(ray->direction, tmin));
                    hit->normal = vec3_set(0.0f, 1.0f, 0.0f); // Placeholder
                }
            }
        } else {
            // Interior node - traverse children
            // Order traversal by distance for better early termination
            if (stack_ptr + 2 <= BVH_TRAVERSAL_STACK_SIZE) {
                stack[stack_ptr++] = node->left_child;
                stack[stack_ptr++] = node->right_child;
            }
        }
    }
    
    hit->hit = any_hit;
    return any_hit;
}

/* Iterative BVH traversal for frustum culling */
static uint32_t traverse_bvh_frustum(
    const geometry_bvh_traversal_internal_t* traversal,
    const vec4_t* frustum_planes,
    uint32_t* out_visible_prims,
    uint32_t max_prims
) {
    if (traversal->node_count == 0) {
        return 0;
    }
    
    uint32_t stack[BVH_TRAVERSAL_STACK_SIZE];
    int stack_ptr = 0;
    stack[stack_ptr++] = 0;
    
    uint32_t visible_count = 0;
    
    while (stack_ptr > 0 && visible_count < max_prims) {
        uint32_t node_idx = stack[--stack_ptr];
        const bvh_node_t* node = &traversal->nodes[node_idx];
        
        if (!frustum_aabb_intersect(frustum_planes, &node->bounds)) {
            continue;
        }
        
        // Leaf node - add visible primitives
        if (node->prim_count > 0) {
            for (uint32_t i = 0; i < node->prim_count && visible_count < max_prims; i++) {
                out_visible_prims[visible_count++] = traversal->prim_indices[node->first_prim + i];
            }
        } else {
            // Interior node - traverse children
            if (stack_ptr + 2 <= BVH_TRAVERSAL_STACK_SIZE) {
                stack[stack_ptr++] = node->left_child;
                stack[stack_ptr++] = node->right_child;
            }
        }
    }
    
    return visible_count;
}

/* SIMD-optimized frustum culling (4 AABBs at once) */
#include "bvh_simd.h"

static uint32_t traverse_bvh_frustum_simd(
    const geometry_bvh_traversal_internal_t* traversal,
    const vec4_t* frustum_planes,
    uint32_t* out_visible_prims,
    uint32_t max_prims
) {
    if (traversal->node_count == 0) {
        return 0;
    }
    
    // Prepare SIMD frustum once
    bvh_frustum_simd_t simd_frustum;
    bvh_simd_prepare_frustum(frustum_planes, &simd_frustum);
    
    uint32_t stack[BVH_TRAVERSAL_STACK_SIZE];
    int stack_ptr = 0;
    stack[stack_ptr++] = 0;
    
    uint32_t visible_count = 0;
    
    // Batch buffer for testing 4 nodes at once
    aabb_t aabb_buffer[4];
    uint32_t node_buffer[4];
    int batch_count = 0;
    
    while (stack_ptr > 0 && visible_count < max_prims) {
        // Collect up to 4 nodes for batch testing
        batch_count = 0;
        while (stack_ptr > 0 && batch_count < 4) {
            uint32_t node_idx = stack[--stack_ptr];
            const bvh_node_t* node = &traversal->nodes[node_idx];
            
            aabb_buffer[batch_count] = node->bounds;
            node_buffer[batch_count] = node_idx;
            batch_count++;
        }
        
        // Pad batch to 4 if needed (use dummy AABBs that will be culled)
        while (batch_count < 4) {
            aabb_buffer[batch_count].min = vec3_set(FLT_MAX, FLT_MAX, FLT_MAX);
            aabb_buffer[batch_count].max = vec3_set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            node_buffer[batch_count] = UINT32_MAX;
            batch_count++;
        }
        
        // Test 4 AABBs simultaneously
        bvh_aabb_batch_t batch;
        bvh_simd_prepare_aabb_batch(aabb_buffer, &batch);
        bvh_visibility_mask_t visible_mask = bvh_simd_frustum_cull_batch(&simd_frustum, &batch);
        
        // Process visible nodes
        for (int i = 0; i < 4; i++) {
            if (!(visible_mask & (1u << i)) || node_buffer[i] == UINT32_MAX) {
                continue;
            }
            
            const bvh_node_t* node = &traversal->nodes[node_buffer[i]];
            
            // Leaf node - add visible primitives
            if (node->prim_count > 0) {
                for (uint32_t j = 0; j < node->prim_count && visible_count < max_prims; j++) {
                    out_visible_prims[visible_count++] = traversal->prim_indices[node->first_prim + j];
                }
            } else {
                // Interior node - add children to stack
                if (stack_ptr + 2 <= BVH_TRAVERSAL_STACK_SIZE) {
                    stack[stack_ptr++] = node->left_child;
                    stack[stack_ptr++] = node->right_child;
                }
            }
        }
    }
    
    return visible_count;
}


/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool geometry_bvh_traversal_validate(const geometry_bvh_traversal_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void geometry_bvh_traversal_cleanup_internal(geometry_bvh_traversal_internal_t* item) {
    if (!item) return;
    
    if (item->nodes) {
        free(item->nodes);
        item->nodes = NULL;
    }
    if (item->prim_indices) {
        free(item->prim_indices);
        item->prim_indices = NULL;
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_bvh_traversal_init(void) {
    if (g_bvh_traversal_ctx.initialized) {
        return 0;
    }
    
    g_bvh_traversal_ctx.capacity = BVH_TRAVERSAL_DEFAULT_CAPACITY;
    g_bvh_traversal_ctx.items = calloc(g_bvh_traversal_ctx.capacity, sizeof(geometry_bvh_traversal_internal_t));
    if (!g_bvh_traversal_ctx.items) {
        return -1;
    }
    
    g_bvh_traversal_ctx.count = 0;
    g_bvh_traversal_ctx.initialized = true;
    
    return 0;
}

void geometry_bvh_traversal_shutdown(void) {
    if (!g_bvh_traversal_ctx.initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < g_bvh_traversal_ctx.count; i++) {
        geometry_bvh_traversal_cleanup_internal(&g_bvh_traversal_ctx.items[i]);
    }
    
    free(g_bvh_traversal_ctx.items);
    g_bvh_traversal_ctx.items = NULL;
    g_bvh_traversal_ctx.count = 0;
    g_bvh_traversal_ctx.capacity = 0;
    g_bvh_traversal_ctx.initialized = false;
}

int geometry_bvh_traversal_create(geometry_bvh_traversal_handle_t* out_handle, const geometry_bvh_traversal_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_bvh_traversal_ctx.initialized) {
        return -2;
    }
    
    if (g_bvh_traversal_ctx.count >= g_bvh_traversal_ctx.capacity) {
        return -3;
    }
    
    uint32_t index = g_bvh_traversal_ctx.count++;
    geometry_bvh_traversal_internal_t* item = &g_bvh_traversal_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->nodes = NULL;
    item->prim_indices = NULL;
    item->node_count = 0;
    item->prim_count = 0;
    
    out_handle->id = index;
    return 0;
}

void geometry_bvh_traversal_destroy(geometry_bvh_traversal_handle_t handle) {
    if (handle.id >= g_bvh_traversal_ctx.count) {
        return;
    }
    
    geometry_bvh_traversal_cleanup_internal(&g_bvh_traversal_ctx.items[handle.id]);
}

int geometry_bvh_traversal_update(geometry_bvh_traversal_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_bvh_traversal_ctx.count) {
        return -1;
    }
    
    geometry_bvh_traversal_internal_t* item = &g_bvh_traversal_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    // Update BVH data structure
    // data should contain updated node/primitive information
    
    item->dirty = false;
    return 0;
}

bool geometry_bvh_traversal_is_valid(geometry_bvh_traversal_handle_t handle) {
    if (handle.id >= g_bvh_traversal_ctx.count) {
        return false;
    }
    return g_bvh_traversal_ctx.items[handle.id].initialized;
}

int geometry_bvh_traversal_get_info(geometry_bvh_traversal_handle_t handle, geometry_bvh_traversal_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    
    if (handle.id >= g_bvh_traversal_ctx.count) {
        return -2;
    }
    
    const geometry_bvh_traversal_internal_t* item = &g_bvh_traversal_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    return 0;
}

void geometry_bvh_traversal_mark_dirty(geometry_bvh_traversal_handle_t handle) {
    if (handle.id < g_bvh_traversal_ctx.count) {
        g_bvh_traversal_ctx.items[handle.id].dirty = true;
    }
}

int geometry_bvh_traversal_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_bvh_traversal_ctx.count; i++) {
        geometry_bvh_traversal_internal_t* item = &g_bvh_traversal_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    
    return processed;
}

uint32_t geometry_bvh_traversal_get_count(void) {
    return g_bvh_traversal_ctx.count;
}

size_t geometry_bvh_traversal_get_memory_usage(void) {
    size_t total = sizeof(g_bvh_traversal_ctx);
    total += g_bvh_traversal_ctx.capacity * sizeof(geometry_bvh_traversal_internal_t);
    
    for (uint32_t i = 0; i < g_bvh_traversal_ctx.count; i++) {
        const geometry_bvh_traversal_internal_t* item = &g_bvh_traversal_ctx.items[i];
        total += item->node_count * sizeof(bvh_node_t);
        total += item->prim_count * sizeof(uint32_t);
    }
    
    return total;
}

void geometry_bvh_traversal_debug_print(void) {
    // Debug output
}

/* End of bvh_traversal.c */
