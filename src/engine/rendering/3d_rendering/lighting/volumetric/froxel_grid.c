/*
 * froxel_grid.c
 * Froxel volume allocation and management
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "froxel_grid.h"
#include "../../math/vec3.h"
#include "../../math/vec4.h"
#include "../../math/mat4.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LIGHTING_FROXEL_GRID_MAX_COUNT 16
#define LIGHTING_FROXEL_GRID_DEFAULT_RES_X 160
#define LIGHTING_FROXEL_GRID_DEFAULT_RES_Y 90
#define LIGHTING_FROXEL_GRID_DEFAULT_RES_Z 64

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_froxel_grid_data {
    uint32_t resolution_x;
    uint32_t resolution_y;
    uint32_t resolution_z;
    
    float near_plane;
    float far_plane;
    float fov;
    float aspect_ratio;
    
    // Derived values for fast slicing
    float slice_depth_distribution_scale;
    float slice_depth_distribution_bias;
    
    // Simulated Texture IDs (GL/Vulkan handles would be here)
    uint32_t density_texture_id;
    uint32_t scattering_texture_id; // RGB = scattering, A = transmittance
    uint32_t noise_texture_id;
    
    mat4_t view_proj;
    mat4_t inv_view_proj;
    vec3_t cam_pos;
} lighting_froxel_grid_data_t;

typedef struct lighting_froxel_grid_internal {
    uint32_t id;
    uint32_t flags;
    lighting_froxel_grid_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} lighting_froxel_grid_internal_t;

typedef struct lighting_froxel_grid_context {
    lighting_froxel_grid_internal_t items[LIGHTING_FROXEL_GRID_MAX_COUNT];
    uint32_t count;
    bool initialized;
} lighting_froxel_grid_context_t;

static lighting_froxel_grid_context_t g_froxel_grid_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Calculate Z-slice parameters for exponential distribution
// slice = log2(z / near) * scale + bias
static void calculate_slice_distribution(lighting_froxel_grid_data_t* data) {
    if (!data) return;
    
    // We want slice = 0 at z = near
    // We want slice = resolution_z at z = far
    // Linear depth z distribution is bad for fog; use exponential.
    // k = far / near
    // z = near * pow(k, slice / max_slices)
    // slice = max_slices * log(z/near) / log(k)
    
    float k = data->far_plane / data->near_plane;
    float log_k = log2f(k);
    
    if (log_k > 0.0001f) {
        data->slice_depth_distribution_scale = (float)data->resolution_z / log_k;
        data->slice_depth_distribution_bias = -(float)data->resolution_z * log2f(data->near_plane) / log_k;
    } else {
        data->slice_depth_distribution_scale = 1.0f;
        data->slice_depth_distribution_bias = 0.0f;
    }
}

static float get_z_from_slice(const lighting_froxel_grid_data_t* data, float slice) {
    if (data->slice_depth_distribution_scale < 0.0001f) return data->near_plane;
    
    // Inverse of slice logic
    // slice = log2(z) * scale + bias
    // log2(z) = (slice - bias) / scale
    // z = exp2((slice - bias) / scale)
    return exp2f((slice - data->slice_depth_distribution_bias) / data->slice_depth_distribution_scale);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_froxel_grid_init(void) {
    if (g_froxel_grid_ctx.initialized) {
        return 0;
    }

    memset(&g_froxel_grid_ctx, 0, sizeof(g_froxel_grid_ctx));
    g_froxel_grid_ctx.initialized = true;

    return 0;
}

void lighting_froxel_grid_shutdown(void) {
    if (!g_froxel_grid_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < LIGHTING_FROXEL_GRID_MAX_COUNT; i++) {
        if (g_froxel_grid_ctx.items[i].initialized) {
            lighting_froxel_grid_handle_t h = {i};
            lighting_froxel_grid_destroy(h);
        }
    }

    g_froxel_grid_ctx.initialized = false;
    g_froxel_grid_ctx.count = 0; // Reset count
}

int lighting_froxel_grid_create(lighting_froxel_grid_handle_t* out_handle, const lighting_froxel_grid_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_froxel_grid_ctx.initialized) return -2;

    // Find free slot
    int free_index = -1;
    for (int i = 0; i < LIGHTING_FROXEL_GRID_MAX_COUNT; i++) {
        if (!g_froxel_grid_ctx.items[i].initialized) {
            free_index = i;
            break;
        }
    }

    if (free_index == -1) return -3; // No free slots

    lighting_froxel_grid_internal_t* item = &g_froxel_grid_ctx.items[free_index];
    item->id = free_index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(lighting_froxel_grid_data_t));
    if (!item->data) return -4;

    // Set defaults
    item->data->resolution_x = LIGHTING_FROXEL_GRID_DEFAULT_RES_X;
    item->data->resolution_y = LIGHTING_FROXEL_GRID_DEFAULT_RES_Y;
    item->data->resolution_z = LIGHTING_FROXEL_GRID_DEFAULT_RES_Z;
    item->data->near_plane = 0.1f;
    item->data->far_plane = 100.0f;
    
    calculate_slice_distribution(item->data);

    item->initialized = true;
    item->dirty = true;
    g_froxel_grid_ctx.count++;

    out_handle->id = free_index;
    return 0;
}

void lighting_froxel_grid_destroy(lighting_froxel_grid_handle_t handle) {
    if (handle.id >= LIGHTING_FROXEL_GRID_MAX_COUNT) return;
    lighting_froxel_grid_internal_t* item = &g_froxel_grid_ctx.items[handle.id];

    if (item->initialized) {
        if (item->data) {
            free(item->data);
            item->data = NULL;
        }
        item->initialized = false;
        if (g_froxel_grid_ctx.count > 0) g_froxel_grid_ctx.count--;
    }
}

int lighting_froxel_grid_update(lighting_froxel_grid_handle_t handle, const void* data, size_t size) {
    if (handle.id >= LIGHTING_FROXEL_GRID_MAX_COUNT) return -1;
    lighting_froxel_grid_internal_t* item = &g_froxel_grid_ctx.items[handle.id];
    if (!item->initialized) return -2;

    // For now, assume data is a struct with camera info passed from renderer
    // In a real engine, we'd have a specific update struct
    // For this implementation, we just mark dirty
    item->dirty = true;
    
    // Recalculate distribution if planes changed (mockup logic)
    calculate_slice_distribution(item->data);
    
    return 0;
}

bool lighting_froxel_grid_is_valid(lighting_froxel_grid_handle_t handle) {
    if (handle.id >= LIGHTING_FROXEL_GRID_MAX_COUNT) return false;
    return g_froxel_grid_ctx.items[handle.id].initialized;
}

int lighting_froxel_grid_get_info(lighting_froxel_grid_handle_t handle, lighting_froxel_grid_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= LIGHTING_FROXEL_GRID_MAX_COUNT) return -2;

    lighting_froxel_grid_internal_t* item = &g_froxel_grid_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void lighting_froxel_grid_mark_dirty(lighting_froxel_grid_handle_t handle) {
    if (handle.id < LIGHTING_FROXEL_GRID_MAX_COUNT) {
        g_froxel_grid_ctx.items[handle.id].dirty = true;
    }
}

int lighting_froxel_grid_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < LIGHTING_FROXEL_GRID_MAX_COUNT; i++) {
        lighting_froxel_grid_internal_t* item = &g_froxel_grid_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Re-upload GPU buffers if needed
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t lighting_froxel_grid_get_count(void) {
    return g_froxel_grid_ctx.count;
}

size_t lighting_froxel_grid_get_memory_usage(void) {
    size_t total = sizeof(lighting_froxel_grid_context_t);
    for (uint32_t i = 0; i < LIGHTING_FROXEL_GRID_MAX_COUNT; i++) {
        if (g_froxel_grid_ctx.items[i].initialized && g_froxel_grid_ctx.items[i].data) {
            total += sizeof(lighting_froxel_grid_data_t);
            // Add texture memory approximations here
            // W*H*D * 16 bytes (float4) for scattering
            uint32_t w = g_froxel_grid_ctx.items[i].data->resolution_x;
            uint32_t h = g_froxel_grid_ctx.items[i].data->resolution_y;
            uint32_t d = g_froxel_grid_ctx.items[i].data->resolution_z;
            total += w * h * d * 16; // Scattering
            total += w * h * d * 4;  // Density (float)
        }
    }
    return total;
}

void lighting_froxel_grid_debug_print(void) {
    // Debug print
}
