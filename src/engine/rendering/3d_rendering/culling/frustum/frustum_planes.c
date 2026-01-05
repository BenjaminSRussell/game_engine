/*
 * frustum_planes.c
 * Frustum plane extraction from view-projection matrix
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#include "frustum_planes.h"
#include "../../math/vec3.h"
#include "../../math/vec4.h"
#include "../../math/mat4.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct frustum_planes {
    vec4_t planes[6];  // left, right, bottom, top, near, far
    bool normalized;
} frustum_planes_t;

typedef struct culling_frustum_planes_internal {
    uint32_t id;
    uint32_t flags;
    frustum_planes_t frustum;
    bool initialized;
    bool dirty;
} culling_frustum_planes_internal_t;

typedef struct culling_frustum_planes_context {
    culling_frustum_planes_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} culling_frustum_planes_context_t;

static culling_frustum_planes_context_t g_frustum_planes_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static inline vec4_t normalize_plane(vec4_t plane) {
    float length = sqrtf(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
    if (length < 1e-6f) {
        return plane;
    }
    float inv_length = 1.0f / length;
    return (vec4_t){
        .x = plane.x * inv_length,
        .y = plane.y * inv_length,
        .z = plane.z * inv_length,
        .w = plane.w * inv_length
    };
}

// Extract frustum planes from view-projection matrix (Gribb-Hartmann method)
static void extract_frustum_planes(const mat4_t* vp_matrix, frustum_planes_t* frustum) {
    const float* m = (const float*)vp_matrix;
    
    // Left plane: m[3] + m[0]
    frustum->planes[0] = (vec4_t){
        .x = m[3] + m[0],
        .y = m[7] + m[4],
        .z = m[11] + m[8],
        .w = m[15] + m[12]
    };
    
    // Right plane: m[3] - m[0]
    frustum->planes[1] = (vec4_t){
        .x = m[3] - m[0],
        .y = m[7] - m[4],
        .z = m[11] - m[8],
        .w = m[15] - m[12]
    };
    
    // Bottom plane: m[3] + m[1]
    frustum->planes[2] = (vec4_t){
        .x = m[3] + m[1],
        .y = m[7] + m[5],
        .z = m[11] + m[9],
        .w = m[15] + m[13]
    };
    
    // Top plane: m[3] - m[1]
    frustum->planes[3] = (vec4_t){
        .x = m[3] - m[1],
        .y = m[7] - m[5],
        .z = m[11] - m[9],
        .w = m[15] - m[13]
    };
    
    // Near plane: m[3] + m[2]
    frustum->planes[4] = (vec4_t){
        .x = m[3] + m[2],
        .y = m[7] + m[6],
        .z = m[11] + m[10],
        .w = m[15] + m[14]
    };
    
    // Far plane: m[3] - m[2]
    frustum->planes[5] = (vec4_t){
        .x = m[3] - m[2],
        .y = m[7] - m[6],
        .z = m[11] - m[10],
        .w = m[15] - m[14]
    };
    
    // Normalize all planes
    for (int i = 0; i < 6; i++) {
        frustum->planes[i] = normalize_plane(frustum->planes[i]);
    }
    
    frustum->normalized = true;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int culling_frustum_planes_init(void) {
    if (g_frustum_planes_ctx.initialized) {
        return 0;
    }
    
    g_frustum_planes_ctx.capacity = 64;
    g_frustum_planes_ctx.items = calloc(g_frustum_planes_ctx.capacity,
                                        sizeof(culling_frustum_planes_internal_t));
    if (!g_frustum_planes_ctx.items) {
        return -1;
    }
    
    g_frustum_planes_ctx.count = 0;
    g_frustum_planes_ctx.initialized = true;
    
    return 0;
}

void culling_frustum_planes_shutdown(void) {
    if (!g_frustum_planes_ctx.initialized) {
        return;
    }
    
    free(g_frustum_planes_ctx.items);
    g_frustum_planes_ctx.items = NULL;
    g_frustum_planes_ctx.count = 0;
    g_frustum_planes_ctx.capacity = 0;
    g_frustum_planes_ctx.initialized = false;
}

int culling_frustum_planes_create(culling_frustum_planes_handle_t* out_handle,
                                   const culling_frustum_planes_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_frustum_planes_ctx.initialized) {
        return -2;
    }
    
    if (g_frustum_planes_ctx.count >= g_frustum_planes_ctx.capacity) {
        return -3;
    }
    
    uint32_t index = g_frustum_planes_ctx.count++;
    culling_frustum_planes_internal_t* item = &g_frustum_planes_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    memset(&item->frustum, 0, sizeof(frustum_planes_t));
    
    out_handle->id = index;
    return 0;
}

void culling_frustum_planes_destroy(culling_frustum_planes_handle_t handle) {
    if (handle.id >= g_frustum_planes_ctx.count) {
        return;
    }
    
    g_frustum_planes_ctx.items[handle.id].initialized = false;
}

int culling_frustum_planes_extract(culling_frustum_planes_handle_t handle,
                                    const mat4_t* view_projection_matrix) {
    if (handle.id >= g_frustum_planes_ctx.count) {
        return -1;
    }
    
    if (!view_projection_matrix) {
        return -2;
    }
    
    culling_frustum_planes_internal_t* item = &g_frustum_planes_ctx.items[handle.id];
    if (!item->initialized) {
        return -3;
    }
    
    extract_frustum_planes(view_projection_matrix, &item->frustum);
    item->dirty = false;
    
    return 0;
}

int culling_frustum_planes_get_planes(culling_frustum_planes_handle_t handle,
                                       vec4_t* out_planes, uint32_t max_planes) {
    if (!out_planes || max_planes < 6) {
        return -1;
    }
    
    if (handle.id >= g_frustum_planes_ctx.count) {
        return -2;
    }
    
    const culling_frustum_planes_internal_t* item = &g_frustum_planes_ctx.items[handle.id];
    if (!item->initialized) {
        return -3;
    }
    
    memcpy(out_planes, item->frustum.planes, 6 * sizeof(vec4_t));
    return 0;
}

bool culling_frustum_planes_is_valid(culling_frustum_planes_handle_t handle) {
    if (handle.id >= g_frustum_planes_ctx.count) {
        return false;
    }
    return g_frustum_planes_ctx.items[handle.id].initialized;
}

uint32_t culling_frustum_planes_get_count(void) {
    return g_frustum_planes_ctx.count;
}

void culling_frustum_planes_debug_print(void) {
    printf("[Frustum Planes] Total frustums: %u\n", g_frustum_planes_ctx.count);
}

/* End of frustum_planes.c */
