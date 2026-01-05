/*
 * ao_blur.c
 * AO bilateral blur
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement ACES tonemapping
 * TODO: Add physically-based bloom
 * TODO: Implement TAA
 * TODO: Add depth of field
 * TODO: Implement motion blur
 * TODO: Add GTAO
 * TODO: Implement SSR
 * TODO: Add color grading
 * TODO: Implement lens effects
 * TODO: Add film grain
 * TODO: Implement ao blur initialization
 * TODO: Add ao blur cleanup/shutdown
 * TODO: Implement ao blur validation
 * TODO: Add ao blur error handling
 * TODO: Implement ao blur serialization
 * TODO: Add ao blur debug output
 * TODO: Implement ao blur unit tests
 * TODO: Add ao blur performance counters
 * TODO: Implement ao blur hot-reload
 * TODO: Add ao blur thread safety
 * TODO: Implement ao blur memory pooling
 * TODO: Add ao blur caching layer
 * TODO: Implement ao blur async operations
 * TODO: Add ao blur GPU integration
 * TODO: Implement ao blur SIMD optimization
 * TODO: Add ao blur batch processing
 * TODO: Implement ao blur streaming support
 * TODO: Add ao blur LOD support
 * TODO: Implement ao blur culling integration
 * TODO: Add ao blur render graph node
 */

#include "ao_blur.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define POSTPROCESSING_AO_BLUR_MAX_COUNT 4096
#define POSTPROCESSING_AO_BLUR_DEFAULT_CAPACITY 256
#define POSTPROCESSING_AO_BLUR_ALIGNMENT 16

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct postprocessing_ao_blur_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    ao_blur_params_t params;
} postprocessing_ao_blur_internal_t;

typedef struct postprocessing_ao_blur_context {
    postprocessing_ao_blur_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} postprocessing_ao_blur_context_t;

static postprocessing_ao_blur_context_t g_ao_blur_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static float gaussian(float x, float sigma) {
    return expf(-(x*x) / (2.0f * sigma * sigma));
}

static void postprocessing_ao_blur_cleanup_internal(postprocessing_ao_blur_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int postprocessing_ao_blur_init(void) {
    if (g_ao_blur_ctx.initialized) {
        return 0; // Already initialized
    }

    g_ao_blur_ctx.capacity = POSTPROCESSING_AO_BLUR_DEFAULT_CAPACITY;
    g_ao_blur_ctx.items = calloc(g_ao_blur_ctx.capacity, sizeof(postprocessing_ao_blur_internal_t));
    if (!g_ao_blur_ctx.items) {
        return -1;
    }

    g_ao_blur_ctx.count = 0;
    g_ao_blur_ctx.initialized = true;

    return 0;
}

void postprocessing_ao_blur_shutdown(void) {
    if (!g_ao_blur_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_ao_blur_ctx.count; i++) {
        postprocessing_ao_blur_cleanup_internal(&g_ao_blur_ctx.items[i]);
    }

    free(g_ao_blur_ctx.items);
    g_ao_blur_ctx.items = NULL;
    g_ao_blur_ctx.count = 0;
    g_ao_blur_ctx.capacity = 0;
    g_ao_blur_ctx.initialized = false;
}

int postprocessing_ao_blur_create(postprocessing_ao_blur_handle_t* out_handle, const postprocessing_ao_blur_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_ao_blur_ctx.initialized) {
        return -2;
    }

    if (g_ao_blur_ctx.count >= g_ao_blur_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_ao_blur_ctx.count++;
    postprocessing_ao_blur_internal_t* item = &g_ao_blur_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->params = desc->initial_params;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void postprocessing_ao_blur_destroy(postprocessing_ao_blur_handle_t handle) {
    if (handle.id >= g_ao_blur_ctx.count) {
        return;
    }

    postprocessing_ao_blur_cleanup_internal(&g_ao_blur_ctx.items[handle.id]);
}

int postprocessing_ao_blur_update(postprocessing_ao_blur_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_ao_blur_ctx.count) {
        return -1;
    }

    postprocessing_ao_blur_internal_t* item = &g_ao_blur_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

void postprocessing_ao_blur_set_params(postprocessing_ao_blur_handle_t handle, const ao_blur_params_t* params) {
    if (handle.id >= g_ao_blur_ctx.count || !params) return;
    postprocessing_ao_blur_internal_t* item = &g_ao_blur_ctx.items[handle.id];
    if (item->initialized) {
        item->params = *params;
        item->dirty = true;
    }
}

bool postprocessing_ao_blur_is_valid(postprocessing_ao_blur_handle_t handle) {
    if (handle.id >= g_ao_blur_ctx.count) {
        return false;
    }
    return g_ao_blur_ctx.items[handle.id].initialized;
}

int postprocessing_ao_blur_get_info(postprocessing_ao_blur_handle_t handle, postprocessing_ao_blur_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_ao_blur_ctx.count) {
        return -2;
    }

    const postprocessing_ao_blur_internal_t* item = &g_ao_blur_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->current_params = item->params;

    return 0;
}

void postprocessing_ao_blur_mark_dirty(postprocessing_ao_blur_handle_t handle) {
    if (handle.id < g_ao_blur_ctx.count) {
        g_ao_blur_ctx.items[handle.id].dirty = true;
    }
}

int postprocessing_ao_blur_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_ao_blur_ctx.count; i++) {
        postprocessing_ao_blur_internal_t* item = &g_ao_blur_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

void postprocessing_ao_blur_buffer(
    const ao_blur_params_t* params,
    const float* input_ao,
    const float* input_depth,
    const float* input_normals,
    float* output_ao,
    uint32_t width,
    uint32_t height
) {
    if (!input_ao || !input_depth || !output_ao || !params) return;
    
    int radius = params->radius > 0 ? params->radius : 2;
    float sharpness = params->sharpness > 0.0f ? params->sharpness : 40.0f; // Default sharpness
    float sigma = (float)radius / 2.0f; // Spatial sigma
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t center_idx = y * width + x;
            
            float center_depth = input_depth[center_idx];
            float center_ao = input_ao[center_idx];
            
            // Skip background, dont blur across large depth gaps
            if (center_depth > 1000.0f) {
                output_ao[center_idx] = center_ao;
                continue;
            }
            
            float weight_sum = 0.0f;
            float ao_sum = 0.0f;
            
            // Get center normal
            float nx = 0.0f, ny = 0.0f, nz = 1.0f;
            if (params->use_normals && input_normals) {
                nx = input_normals[center_idx*3 + 0];
                ny = input_normals[center_idx*3 + 1];
                nz = input_normals[center_idx*3 + 2];
            }
            
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int nx_coord = (int)x + dx;
                    int ny_coord = (int)y + dy;
                    
                    if (nx_coord < 0 || nx_coord >= (int)width || ny_coord < 0 || ny_coord >= (int)height) {
                        continue;
                    }
                    
                    uint32_t neighbor_idx = ny_coord * width + nx_coord;
                    float neighbor_depth = input_depth[neighbor_idx];
                    
                    // Spatial weight
                    float w_s = gaussian((float)dx, sigma) * gaussian((float)dy, sigma);
                    
                    // Range weight (depth difference)
                    float depth_diff = fabsf(center_depth - neighbor_depth);
                    float w_r = expf(-(depth_diff * depth_diff * sharpness));
                    
                    // Normal weight
                    float w_n = 1.0f;
                    if (params->use_normals && input_normals) {
                        float nnx = input_normals[neighbor_idx*3 + 0];
                        float nny = input_normals[neighbor_idx*3 + 1];
                        float nnz = input_normals[neighbor_idx*3 + 2];
                        float dot = nx*nnx + ny*nny + nz*nnz;
                        if (dot < 0.0f) dot = 0.0f;
                        w_n = powf(dot, 4.0f); // Power ensures strict adherence to planar surfaces
                    }
                    
                    float w = w_s * w_r * w_n;
                    
                    ao_sum += input_ao[neighbor_idx] * w;
                    weight_sum += w;
                }
            }
            
            if (weight_sum > 0.0001f) {
                output_ao[center_idx] = ao_sum / weight_sum;
            } else {
                output_ao[center_idx] = center_ao;
            }
        }
    }
}

uint32_t postprocessing_ao_blur_get_count(void) {
    return g_ao_blur_ctx.count;
}

size_t postprocessing_ao_blur_get_memory_usage(void) {
    size_t total = sizeof(g_ao_blur_ctx);
    total += g_ao_blur_ctx.capacity * sizeof(postprocessing_ao_blur_internal_t);

    for (uint32_t i = 0; i < g_ao_blur_ctx.count; i++) {
        total += g_ao_blur_ctx.items[i].data_size;
    }

    return total;
}

void postprocessing_ao_blur_debug_print(void) {
    printf("AO Blur Context: %u/%u items\n", g_ao_blur_ctx.count, g_ao_blur_ctx.capacity);
}

/* End of ao_blur.c */
