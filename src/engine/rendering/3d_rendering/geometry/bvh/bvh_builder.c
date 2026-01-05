/*
 * bvh_builder.c
 * BVH tree construction with SAH (Surface Area Heuristic) splitting
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "bvh_builder.h"
#include "../../math/vec3.h"
#include "../../math/aabb.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define BVH_MAX_LEAF_PRIMITIVES 4
#define BVH_SAH_BINS 16
#define BVH_DEFAULT_CAPACITY 256
#define BVH_STACK_SIZE 64

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct bvh_primitive {
    aabb_t bounds;
    vec3_t centroid;
    uint32_t id;
} bvh_primitive_t;

typedef struct bvh_node {
    aabb_t bounds;
    uint32_t first_prim;    // For leaves: first primitive index
    uint32_t prim_count;    // For leaves: primitive count, 0 for interior nodes
    uint32_t left_child;    // For interior: left child node index
    uint32_t right_child;   // For interior: right child node index
    uint8_t split_axis;     // 0=X, 1=Y, 2=Z
} bvh_node_t;

typedef struct bvh_build_context {
    bvh_node_t* nodes;
    uint32_t* prim_indices;
    bvh_primitive_t* primitives;
    uint32_t node_count;
    uint32_t node_capacity;
    uint32_t prim_count;
} bvh_build_context_t;

typedef struct geometry_bvh_builder_internal {
    uint32_t id;
    uint32_t flags;
    bvh_build_context_t build_ctx;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} geometry_bvh_builder_internal_t;

typedef struct geometry_bvh_builder_context {
    geometry_bvh_builder_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} geometry_bvh_builder_context_t;

static geometry_bvh_builder_context_t g_bvh_builder_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static inline float aabb_surface_area(const aabb_t* bounds) {
    vec3_t extent = vec3_sub(bounds->max, bounds->min);
    return 2.0f * (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x);
}

static inline aabb_t aabb_union(const aabb_t* a, const aabb_t* b) {
    aabb_t result;
    result.min = vec3_min(a->min, b->min);
    result.max = vec3_max(a->max, b->max);
    return result;
}

static inline aabb_t compute_centroid_bounds(const bvh_primitive_t* prims, const uint32_t* indices, uint32_t count) {
    aabb_t bounds;
    bounds.min = vec3_set(FLT_MAX, FLT_MAX, FLT_MAX);
    bounds.max = vec3_set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    
    for (uint32_t i = 0; i < count; i++) {
        vec3_t c = prims[indices[i]].centroid;
        bounds.min = vec3_min(bounds.min, c);
        bounds.max = vec3_max(bounds.max, c);
    }
    return bounds;
}

static inline uint32_t find_best_split_axis(const aabb_t* centroid_bounds) {
    vec3_t extent = vec3_sub(centroid_bounds->max, centroid_bounds->min);
    if (extent.x > extent.y && extent.x > extent.z) return 0;
    if (extent.y > extent.z) return 1;
    return 2;
}

/* SAH (Surface Area Heuristic) based split finding */
static float evaluate_sah_split(
    const bvh_primitive_t* prims,
    const uint32_t* indices,
    uint32_t start,
    uint32_t end,
    uint32_t axis,
    float* out_split_pos
) {
    aabb_t centroid_bounds = compute_centroid_bounds(prims, indices + start, end - start);
    vec3_t extent = vec3_sub(centroid_bounds.max, centroid_bounds.min);
    float axis_extent = axis == 0 ? extent.x : (axis == 1 ? extent.y : extent.z);
    
    if (axis_extent < 1e-6f) {
        *out_split_pos = axis == 0 ? centroid_bounds.min.x : 
                         (axis == 1 ? centroid_bounds.min.y : centroid_bounds.min.z);
        return FLT_MAX;
    }
    
    // Initialize bins
    struct {
        aabb_t bounds;
        uint32_t count;
    } bins[BVH_SAH_BINS] = {0};
    
    float bin_min = axis == 0 ? centroid_bounds.min.x : 
                    (axis == 1 ? centroid_bounds.min.y : centroid_bounds.min.z);
    float bin_scale = BVH_SAH_BINS / axis_extent;
    
    // Populate bins
    for (uint32_t i = start; i < end; i++) {
        const bvh_primitive_t* prim = &prims[indices[i]];
        float centroid_val = axis == 0 ? prim->centroid.x : 
                            (axis == 1 ? prim->centroid.y : prim->centroid.z);
        int bin_idx = (int)((centroid_val - bin_min) * bin_scale);
        bin_idx = bin_idx < 0 ? 0 : (bin_idx >= BVH_SAH_BINS ? BVH_SAH_BINS - 1 : bin_idx);
        
        if (bins[bin_idx].count == 0) {
            bins[bin_idx].bounds = prim->bounds;
        } else {
            bins[bin_idx].bounds = aabb_union(&bins[bin_idx].bounds, &prim->bounds);
        }
        bins[bin_idx].count++;
    }
    
    // Compute sweep costs
    float cost_left[BVH_SAH_BINS - 1];
    float cost_right[BVH_SAH_BINS - 1];
    aabb_t bounds_left[BVH_SAH_BINS - 1];
    aabb_t bounds_right[BVH_SAH_BINS - 1];
    uint32_t count_left[BVH_SAH_BINS - 1];
    uint32_t count_right[BVH_SAH_BINS - 1];
    
    // Left sweep
    aabb_t accum_bounds = bins[0].bounds;
    uint32_t accum_count = bins[0].count;
    for (int i = 0; i < BVH_SAH_BINS - 1; i++) {
        bounds_left[i] = accum_bounds;
        count_left[i] = accum_count;
        cost_left[i] = aabb_surface_area(&accum_bounds) * accum_count;
        
        if (bins[i + 1].count > 0) {
            accum_bounds = aabb_union(&accum_bounds, &bins[i + 1].bounds);
            accum_count += bins[i + 1].count;
        }
    }
    
    // Right sweep
    accum_bounds = bins[BVH_SAH_BINS - 1].bounds;
    accum_count = bins[BVH_SAH_BINS - 1].count;
    for (int i = BVH_SAH_BINS - 2; i >= 0; i--) {
        bounds_right[i] = accum_bounds;
        count_right[i] = accum_count;
        cost_right[i] = aabb_surface_area(&accum_bounds) * accum_count;
        
        if (bins[i].count > 0) {
            accum_bounds = aabb_union(&accum_bounds, &bins[i].bounds);
            accum_count += bins[i].count;
        }
    }
    
    // Find minimum SAH cost
    int best_bin = -1;
    float best_cost = FLT_MAX;
    for (int i = 0; i < BVH_SAH_BINS - 1; i++) {
        float cost = cost_left[i] + cost_right[i];
        if (cost < best_cost && count_left[i] > 0 && count_right[i] > 0) {
            best_cost = cost;
            best_bin = i;
        }
    }
    
    if (best_bin < 0) {
        *out_split_pos = bin_min + axis_extent * 0.5f;
        return FLT_MAX;
    }
    
    *out_split_pos = bin_min + axis_extent * ((best_bin + 1) / (float)BVH_SAH_BINS);
    return best_cost;
}

/* Partition primitives around split position */
static uint32_t partition_primitives(
    uint32_t* indices,
    const bvh_primitive_t* prims,
    uint32_t start,
    uint32_t end,
    uint32_t axis,
    float split_pos
) {
    uint32_t i = start;
    uint32_t j = end - 1;
    
    while (i <= j) {
        float centroid_val = axis == 0 ? prims[indices[i]].centroid.x :
                            (axis == 1 ? prims[indices[i]].centroid.y : prims[indices[i]].centroid.z);
        
        if (centroid_val < split_pos) {
            i++;
        } else {
            uint32_t temp = indices[i];
            indices[i] = indices[j];
            indices[j] = temp;
            if (j == 0) break;
            j--;
        }
    }
    
    return i > start ? i : start + 1;
}

/* Recursive BVH construction */
static uint32_t build_bvh_recursive(
    bvh_build_context_t* ctx,
    uint32_t start,
    uint32_t end,
    uint32_t depth
) {
    if (ctx->node_count >= ctx->node_capacity) {
        return UINT32_MAX;
    }
    
    uint32_t node_idx = ctx->node_count++;
    bvh_node_t* node = &ctx->nodes[node_idx];
    
    // Compute bounding box for this node
    node->bounds.min = vec3_set(FLT_MAX, FLT_MAX, FLT_MAX);
    node->bounds.max = vec3_set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    
    for (uint32_t i = start; i < end; i++) {
        const aabb_t* prim_bounds = &ctx->primitives[ctx->prim_indices[i]].bounds;
        node->bounds = aabb_union(&node->bounds, prim_bounds);
    }
    
    uint32_t prim_count = end - start;
    
    // Create leaf node
    if (prim_count <= BVH_MAX_LEAF_PRIMITIVES || depth > 32) {
        node->first_prim = start;
        node->prim_count = prim_count;
        node->left_child = UINT32_MAX;
        node->right_child = UINT32_MAX;
        return node_idx;
    }
    
    // Find best split using SAH
    aabb_t centroid_bounds = compute_centroid_bounds(ctx->primitives, ctx->prim_indices + start, prim_count);
    uint32_t best_axis = find_best_split_axis(&centroid_bounds);
    
    float split_pos;
    float sah_cost = evaluate_sah_split(ctx->primitives, ctx->prim_indices, start, end, best_axis, &split_pos);
    
    // Calculate leaf cost
    float leaf_cost = aabb_surface_area(&node->bounds) * prim_count;
    
    // If SAH prefers leaf, create leaf
    if (sah_cost >= leaf_cost || sah_cost == FLT_MAX) {
        node->first_prim = start;
        node->prim_count = prim_count;
        node->left_child = UINT32_MAX;
        node->right_child = UINT32_MAX;
        return node_idx;
    }
    
    // Partition primitives
    uint32_t mid = partition_primitives(ctx->prim_indices, ctx->primitives, start, end, best_axis, split_pos);
    
    // Ensure both sides have at least one primitive
    if (mid == start || mid >= end) {
        mid = start + prim_count / 2;
    }
    
    // Create interior node
    node->split_axis = best_axis;
    node->prim_count = 0;
    node->first_prim = 0;
    
    // Build children
    uint32_t left_idx = build_bvh_recursive(ctx, start, mid, depth + 1);
    uint32_t right_idx = build_bvh_recursive(ctx, mid, end, depth + 1);
    
    // Update node (pointer may have changed)
    ctx->nodes[node_idx].left_child = left_idx;
    ctx->nodes[node_idx].right_child = right_idx;
    
    return node_idx;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool geometry_bvh_builder_validate(const geometry_bvh_builder_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void geometry_bvh_builder_cleanup_internal(geometry_bvh_builder_internal_t* item) {
    if (!item) return;
    
    if (item->build_ctx.nodes) {
        free(item->build_ctx.nodes);
        item->build_ctx.nodes = NULL;
    }
    if (item->build_ctx.prim_indices) {
        free(item->build_ctx.prim_indices);
        item->build_ctx.prim_indices = NULL;
    }
    if (item->build_ctx.primitives) {
        free(item->build_ctx.primitives);
        item->build_ctx.primitives = NULL;
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_bvh_builder_init(void) {
    if (g_bvh_builder_ctx.initialized) {
        return 0;
    }
    
    g_bvh_builder_ctx.capacity = BVH_DEFAULT_CAPACITY;
    g_bvh_builder_ctx.items = calloc(g_bvh_builder_ctx.capacity, sizeof(geometry_bvh_builder_internal_t));
    if (!g_bvh_builder_ctx.items) {
        return -1;
    }
    
    g_bvh_builder_ctx.count = 0;
    g_bvh_builder_ctx.initialized = true;
    
    return 0;
}

void geometry_bvh_builder_shutdown(void) {
    if (!g_bvh_builder_ctx.initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < g_bvh_builder_ctx.count; i++) {
        geometry_bvh_builder_cleanup_internal(&g_bvh_builder_ctx.items[i]);
    }
    
    free(g_bvh_builder_ctx.items);
    g_bvh_builder_ctx.items = NULL;
    g_bvh_builder_ctx.count = 0;
    g_bvh_builder_ctx.capacity = 0;
    g_bvh_builder_ctx.initialized = false;
}

int geometry_bvh_builder_create(geometry_bvh_builder_handle_t* out_handle, const geometry_bvh_builder_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_bvh_builder_ctx.initialized) {
        return -2;
    }
    
    if (g_bvh_builder_ctx.count >= g_bvh_builder_ctx.capacity) {
        return -3;
    }
    
    uint32_t index = g_bvh_builder_ctx.count++;
    geometry_bvh_builder_internal_t* item = &g_bvh_builder_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    memset(&item->build_ctx, 0, sizeof(bvh_build_context_t));
    
    out_handle->id = index;
    return 0;
}

void geometry_bvh_builder_destroy(geometry_bvh_builder_handle_t handle) {
    if (handle.id >= g_bvh_builder_ctx.count) {
        return;
    }
    
    geometry_bvh_builder_cleanup_internal(&g_bvh_builder_ctx.items[handle.id]);
}

int geometry_bvh_builder_update(geometry_bvh_builder_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_bvh_builder_ctx.count) {
        return -1;
    }
    
    geometry_bvh_builder_internal_t* item = &g_bvh_builder_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    // Build BVH from input primitives
    // data should point to an array of primitives
    const bvh_primitive_t* input_prims = (const bvh_primitive_t*)data;
    uint32_t prim_count = size / sizeof(bvh_primitive_t);
    
    if (prim_count == 0) {
        return -3;
    }
    
    // Allocate build structures
    bvh_build_context_t* ctx = &item->build_ctx;
    
    ctx->prim_count = prim_count;
    ctx->node_capacity = prim_count * 2;
    ctx->node_count = 0;
    
    ctx->primitives = malloc(prim_count * sizeof(bvh_primitive_t));
    ctx->prim_indices = malloc(prim_count * sizeof(uint32_t));
    ctx->nodes = malloc(ctx->node_capacity * sizeof(bvh_node_t));
    
    if (!ctx->primitives || !ctx->prim_indices || !ctx->nodes) {
        geometry_bvh_builder_cleanup_internal(item);
        return -4;
    }
    
    memcpy(ctx->primitives, input_prims, prim_count * sizeof(bvh_primitive_t));
    
    for (uint32_t i = 0; i < prim_count; i++) {
        ctx->prim_indices[i] = i;
    }
    
    // Build BVH tree
    build_bvh_recursive(ctx, 0, prim_count, 0);
    
    item->dirty = false;
    return 0;
}

bool geometry_bvh_builder_is_valid(geometry_bvh_builder_handle_t handle) {
    if (handle.id >= g_bvh_builder_ctx.count) {
        return false;
    }
    return g_bvh_builder_ctx.items[handle.id].initialized;
}

int geometry_bvh_builder_get_info(geometry_bvh_builder_handle_t handle, geometry_bvh_builder_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    
    if (handle.id >= g_bvh_builder_ctx.count) {
        return -2;
    }
    
    const geometry_bvh_builder_internal_t* item = &g_bvh_builder_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    return 0;
}

void geometry_bvh_builder_mark_dirty(geometry_bvh_builder_handle_t handle) {
    if (handle.id < g_bvh_builder_ctx.count) {
        g_bvh_builder_ctx.items[handle.id].dirty = true;
    }
}

int geometry_bvh_builder_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_bvh_builder_ctx.count; i++) {
        geometry_bvh_builder_internal_t* item = &g_bvh_builder_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    
    return processed;
}

uint32_t geometry_bvh_builder_get_count(void) {
    return g_bvh_builder_ctx.count;
}

size_t geometry_bvh_builder_get_memory_usage(void) {
    size_t total = sizeof(g_bvh_builder_ctx);
    total += g_bvh_builder_ctx.capacity * sizeof(geometry_bvh_builder_internal_t);
    
    for (uint32_t i = 0; i < g_bvh_builder_ctx.count; i++) {
        const bvh_build_context_t* ctx = &g_bvh_builder_ctx.items[i].build_ctx;
        total += ctx->node_capacity * sizeof(bvh_node_t);
        total += ctx->prim_count * sizeof(bvh_primitive_t);
        total += ctx->prim_count * sizeof(uint32_t);
    }
    
    return total;
}

void geometry_bvh_builder_debug_print(void) {
    // TODO: Implement debug output
}

/* End of bvh_builder.c */
