/*
 * instance_culler.c
 * Per-Instance Culling Logic
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "geometry/instancing/instance_culler.h"
#include "math/frustum.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arm_neon.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define INSTANCE_CULLER_DEFAULT_CAPACITY 128
#define SIMD_BATCH_SIZE 4  // Process 4 AABBs at once with NEON

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct instance_culler_internal {
    uint32_t id;
    uint32_t flags;
    uint32_t* visible_indices;
    uint32_t visible_capacity;
    uint32_t visible_count;
    bool initialized;
} instance_culler_internal_t;

typedef struct instance_culler_context {
    instance_culler_internal_t* cullers;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} instance_culler_context_t;

static instance_culler_context_t g_instance_culler_ctx = {0};

/* ============================================================================
 * SIMD FRUSTUM CULLING (ARM NEON)
 * ============================================================================ */

// Test single AABB against frustum using NEON
static inline bool simd_test_aabb_frustum(const aabb_t* aabb, const frustum_t* frustum) {
    // Load AABB min/max (vec3_t has x, y, z components)
    float32x4_t aabb_min = {aabb->min.x, aabb->min.y, aabb->min.z, 0.0f};
    float32x4_t aabb_max = {aabb->max.x, aabb->max.y, aabb->max.z, 0.0f};
    
    // Test against all 6 frustum planes
    for (int i = 0; i < 6; i++) {
        const plane_t* plane = &frustum->planes[i];
        
        // Load plane normal and distance
        float32x4_t normal = {plane->normal.x, plane->normal.y, plane->normal.z, 0.0f};
        float32_t distance = plane->distance;
        
        // Compute positive/negative vertices
        // p_vertex = (normal.x > 0) ? max.x : min.x for each component
        uint32x4_t mask = vcgtq_f32(normal, vdupq_n_f32(0.0f));
        float32x4_t p_vertex = vbslq_f32(mask, aabb_max, aabb_min);
        
        // Dot product: p_vertex · normal
        float32x4_t dot = vmulq_f32(p_vertex, normal);
        float32x2_t sum_low = vget_low_f32(dot);
        float32x2_t sum_high = vget_high_f32(dot);
        float32x2_t sum = vadd_f32(sum_low, sum_high);
        float32x2_t final_sum = vpadd_f32(sum, sum);
        float result = vget_lane_f32(final_sum, 0);
        
        // If p_vertex is outside (behind) plane, AABB is outside frustum
        if (result + distance < 0.0f) {
            return false;
        }
    }
    
    return true;
}

// Batch frustum cull using SIMD
static uint32_t simd_frustum_cull_batch(const aabb_t* aabbs,
                                        uint32_t count,
                                        const frustum_t* frustum,
                                        uint32_t* out_visible_indices,
                                        uint32_t max_visible) {
    uint32_t visible_count = 0;
    
    for (uint32_t i = 0; i < count && visible_count < max_visible; i++) {
        if (simd_test_aabb_frustum(&aabbs[i], frustum)) {
            out_visible_indices[visible_count++] = i;
        }
    }
    
    return visible_count;
}

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static bool allocate_visible_buffer(instance_culler_internal_t* culler, uint32_t capacity) {
    culler->visible_indices = calloc(capacity, sizeof(uint32_t));
    if (!culler->visible_indices) {
        return false;
    }
    
    culler->visible_capacity = capacity;
    culler->visible_count = 0;
    
    return true;
}

static void free_visible_buffer(instance_culler_internal_t* culler) {
    if (culler->visible_indices) {
        free(culler->visible_indices);
        culler->visible_indices = NULL;
    }
    culler->visible_capacity = 0;
    culler->visible_count = 0;
}

static bool resize_visible_buffer(instance_culler_internal_t* culler, uint32_t new_capacity) {
    if (new_capacity <= culler->visible_capacity) {
        return true;
    }
    
    uint32_t* new_indices = realloc(culler->visible_indices, new_capacity * sizeof(uint32_t));
    if (!new_indices) {
        return false;
    }
    
    culler->visible_indices = new_indices;
    culler->visible_capacity = new_capacity;
    
    return true;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_instance_culler_init(void) {
    if (g_instance_culler_ctx.initialized) {
        return 0;
    }
    
    g_instance_culler_ctx.capacity = INSTANCE_CULLER_DEFAULT_CAPACITY;
    g_instance_culler_ctx.cullers = calloc(g_instance_culler_ctx.capacity,
                                           sizeof(instance_culler_internal_t));
    if (!g_instance_culler_ctx.cullers) {
        return -1;
    }
    
    g_instance_culler_ctx.count = 0;
    g_instance_culler_ctx.initialized = true;
    
    return 0;
}

void geometry_instance_culler_shutdown(void) {
    if (!g_instance_culler_ctx.initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < g_instance_culler_ctx.count; i++) {
        free_visible_buffer(&g_instance_culler_ctx.cullers[i]);
    }
    
    free(g_instance_culler_ctx.cullers);
    g_instance_culler_ctx.cullers = NULL;
    g_instance_culler_ctx.count = 0;
    g_instance_culler_ctx.capacity = 0;
    g_instance_culler_ctx.initialized = false;
}

int geometry_instance_culler_create(geometry_instance_culler_handle_t* out_handle,
                                    const geometry_instance_culler_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_instance_culler_ctx.initialized) {
        return -2;
    }
    
    if (g_instance_culler_ctx.count >= g_instance_culler_ctx.capacity) {
        return -3;
    }
    
    uint32_t index = g_instance_culler_ctx.count++;
    instance_culler_internal_t* culler = &g_instance_culler_ctx.cullers[index];
    
    culler->id = index;
    culler->flags = desc->flags;
    culler->initialized = true;
    
    uint32_t initial_capacity = desc->max_instances > 0 ? desc->max_instances : 1024;
    if (!allocate_visible_buffer(culler, initial_capacity)) {
        culler->initialized = false;
        g_instance_culler_ctx.count--;
        return -4;
    }
    
    out_handle->id = index;
    return 0;
}

void geometry_instance_culler_destroy(geometry_instance_culler_handle_t handle) {
    if (handle.id >= g_instance_culler_ctx.count) {
        return;
    }
    
    instance_culler_internal_t* culler = &g_instance_culler_ctx.cullers[handle.id];
    free_visible_buffer(culler);
    culler->initialized = false;
}

int geometry_instance_culler_cull_instances(geometry_instance_culler_handle_t handle,
                                            const aabb_t* instance_aabbs,
                                            uint32_t instance_count,
                                            const frustum_t* frustum,
                                            uint32_t** out_visible_indices,
                                            uint32_t* out_visible_count) {
    if (handle.id >= g_instance_culler_ctx.count) {
        return -1;
    }
    
    instance_culler_internal_t* culler = &g_instance_culler_ctx.cullers[handle.id];
    if (!culler->initialized) {
        return -2;
    }
    
    if (!instance_aabbs || !frustum || !out_visible_indices || !out_visible_count) {
        return -3;
    }
    
    // Resize buffer if needed
    if (instance_count > culler->visible_capacity) {
        if (!resize_visible_buffer(culler, instance_count)) {
            return -4;
        }
    }
    
    // Perform SIMD frustum culling
    culler->visible_count = simd_frustum_cull_batch(
        instance_aabbs,
        instance_count,
        frustum,
        culler->visible_indices,
        culler->visible_capacity
    );
    
    *out_visible_indices = culler->visible_indices;
    *out_visible_count = culler->visible_count;
    
    return 0;
}

int geometry_instance_culler_update(geometry_instance_culler_handle_t handle,
                                    const void* data, size_t size) {
    if (handle.id >= g_instance_culler_ctx.count) {
        return -1;
    }
    
    instance_culler_internal_t* culler = &g_instance_culler_ctx.cullers[handle.id];
    if (!culler->initialized) {
        return -2;
    }
    
    return 0;
}

bool geometry_instance_culler_is_valid(geometry_instance_culler_handle_t handle) {
    if (handle.id >= g_instance_culler_ctx.count) {
        return false;
    }
    return g_instance_culler_ctx.cullers[handle.id].initialized;
}

int geometry_instance_culler_get_info(geometry_instance_culler_handle_t handle,
                                      geometry_instance_culler_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    
    if (handle.id >= g_instance_culler_ctx.count) {
        return -2;
    }
    
    const instance_culler_internal_t* culler = &g_instance_culler_ctx.cullers[handle.id];
    out_info->id = culler->id;
    out_info->flags = culler->flags;
    out_info->initialized = culler->initialized;
    out_info->visible_count = culler->visible_count;
    
    return 0;
}

void geometry_instance_culler_mark_dirty(geometry_instance_culler_handle_t handle) {
    if (handle.id < g_instance_culler_ctx.count) {
        // Mark for re-culling if needed
    }
}

int geometry_instance_culler_process_pending(void) {
    // Process any pending GPU culling operations
    return 0;
}

uint32_t geometry_instance_culler_get_count(void) {
    return g_instance_culler_ctx.count;
}

size_t geometry_instance_culler_get_memory_usage(void) {
    size_t total = sizeof(g_instance_culler_ctx);
    total += g_instance_culler_ctx.capacity * sizeof(instance_culler_internal_t);
    
    for (uint32_t i = 0; i < g_instance_culler_ctx.count; i++) {
        const instance_culler_internal_t* culler = &g_instance_culler_ctx.cullers[i];
        total += culler->visible_capacity * sizeof(uint32_t);
    }
    
    return total;
}

void geometry_instance_culler_debug_print(void) {
    printf("[Instance Culler] Total cullers: %u\n", g_instance_culler_ctx.count);
    
    for (uint32_t i = 0; i < g_instance_culler_ctx.count; i++) {
        const instance_culler_internal_t* culler = &g_instance_culler_ctx.cullers[i];
        if (culler->initialized) {
            printf("  Culler %u: %u visible instances\n", i, culler->visible_count);
        }
    }
}

/* End of instance_culler.c */
