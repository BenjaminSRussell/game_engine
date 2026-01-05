/*
 * bloom_upsample.c
 * Bloom upsample blend
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
 * TODO: Implement bloom upsample initialization
 * TODO: Add bloom upsample cleanup/shutdown
 * TODO: Implement bloom upsample validation
 * TODO: Add bloom upsample error handling
 * TODO: Implement bloom upsample serialization
 * TODO: Add bloom upsample debug output
 * TODO: Implement bloom upsample unit tests
 * TODO: Add bloom upsample performance counters
 * TODO: Implement bloom upsample hot-reload
 * TODO: Add bloom upsample thread safety
 * TODO: Implement bloom upsample memory pooling
 * TODO: Add bloom upsample caching layer
 * TODO: Implement bloom upsample async operations
 * TODO: Add bloom upsample GPU integration
 * TODO: Implement bloom upsample SIMD optimization
 * TODO: Add bloom upsample batch processing
 * TODO: Implement bloom upsample streaming support
 * TODO: Add bloom upsample LOD support
 * TODO: Implement bloom upsample culling integration
 * TODO: Add bloom upsample render graph node
 */

#include "bloom_upsample.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define POSTPROCESSING_BLOOM_UPSAMPLE_MAX_COUNT 4096
#define POSTPROCESSING_BLOOM_UPSAMPLE_DEFAULT_CAPACITY 256
#define POSTPROCESSING_BLOOM_UPSAMPLE_ALIGNMENT 16

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct postprocessing_bloom_upsample_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    bloom_upsample_params_t params;
} postprocessing_bloom_upsample_internal_t;

typedef struct postprocessing_bloom_upsample_context {
    postprocessing_bloom_upsample_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} postprocessing_bloom_upsample_context_t;

static postprocessing_bloom_upsample_context_t g_bloom_upsample_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void postprocessing_bloom_upsample_cleanup_internal(postprocessing_bloom_upsample_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

static void sample_pixel_u(const float* input, uint32_t w, uint32_t h, int x, int y, float* r, float* g, float* b) {
    // Clamp to border
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (int)w) x = (int)w - 1;
    if (y >= (int)h) y = (int)h - 1;
    
    uint32_t idx = (y * w + x) * 3;
    *r = input[idx];
    *g = input[idx+1];
    *b = input[idx+2];
}

static void sample_bilinear_u(const float* input, uint32_t w, uint32_t h, float u, float v, float* r, float* g, float* b) {
    float x = u - 0.5f;
    float y = v - 0.5f;
    
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    float wx = x - x0;
    float wy = y - y0;
    
    float r00, g00, b00;
    float r10, g10, b10;
    float r01, g01, b01;
    float r11, g11, b11;
    
    sample_pixel_u(input, w, h, x0, y0, &r00, &g00, &b00);
    sample_pixel_u(input, w, h, x1, y0, &r10, &g10, &b10);
    sample_pixel_u(input, w, h, x0, y1, &r01, &g01, &b01);
    sample_pixel_u(input, w, h, x1, y1, &r11, &g11, &b11);
    
    // Interpolate
    float r0 = r00 * (1.0f - wx) + r10 * wx;
    float g0 = g00 * (1.0f - wx) + g10 * wx;
    float b0 = b00 * (1.0f - wx) + b10 * wx;
    
    float r1 = r01 * (1.0f - wx) + r11 * wx;
    float g1 = g01 * (1.0f - wx) + g11 * wx;
    float b1 = b01 * (1.0f - wx) + b11 * wx;
    
    *r = r0 * (1.0f - wy) + r1 * wy;
    *g = g0 * (1.0f - wy) + g1 * wy;
    *b = b0 * (1.0f - wy) + b1 * wy;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int postprocessing_bloom_upsample_init(void) {
    if (g_bloom_upsample_ctx.initialized) {
        return 0; // Already initialized
    }

    g_bloom_upsample_ctx.capacity = POSTPROCESSING_BLOOM_UPSAMPLE_DEFAULT_CAPACITY;
    g_bloom_upsample_ctx.items = calloc(g_bloom_upsample_ctx.capacity, sizeof(postprocessing_bloom_upsample_internal_t));
    if (!g_bloom_upsample_ctx.items) {
        return -1;
    }

    g_bloom_upsample_ctx.count = 0;
    g_bloom_upsample_ctx.initialized = true;

    return 0;
}

void postprocessing_bloom_upsample_shutdown(void) {
    if (!g_bloom_upsample_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_bloom_upsample_ctx.count; i++) {
        postprocessing_bloom_upsample_cleanup_internal(&g_bloom_upsample_ctx.items[i]);
    }

    free(g_bloom_upsample_ctx.items);
    g_bloom_upsample_ctx.items = NULL;
    g_bloom_upsample_ctx.count = 0;
    g_bloom_upsample_ctx.capacity = 0;
    g_bloom_upsample_ctx.initialized = false;
}

int postprocessing_bloom_upsample_create(postprocessing_bloom_upsample_handle_t* out_handle, const postprocessing_bloom_upsample_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_bloom_upsample_ctx.initialized) {
        return -2;
    }

    if (g_bloom_upsample_ctx.count >= g_bloom_upsample_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_bloom_upsample_ctx.count++;
    postprocessing_bloom_upsample_internal_t* item = &g_bloom_upsample_ctx.items[index];

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

void postprocessing_bloom_upsample_destroy(postprocessing_bloom_upsample_handle_t handle) {
    if (handle.id >= g_bloom_upsample_ctx.count) {
        return;
    }

    postprocessing_bloom_upsample_cleanup_internal(&g_bloom_upsample_ctx.items[handle.id]);
}

int postprocessing_bloom_upsample_update(postprocessing_bloom_upsample_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_bloom_upsample_ctx.count) {
        return -1;
    }

    postprocessing_bloom_upsample_internal_t* item = &g_bloom_upsample_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

void postprocessing_bloom_upsample_set_params(postprocessing_bloom_upsample_handle_t handle, const bloom_upsample_params_t* params) {
    if (handle.id >= g_bloom_upsample_ctx.count || !params) return;
    postprocessing_bloom_upsample_internal_t* item = &g_bloom_upsample_ctx.items[handle.id];
    if (item->initialized) {
        item->params = *params;
        item->dirty = true;
    }
}

bool postprocessing_bloom_upsample_is_valid(postprocessing_bloom_upsample_handle_t handle) {
    if (handle.id >= g_bloom_upsample_ctx.count) {
        return false;
    }
    return g_bloom_upsample_ctx.items[handle.id].initialized;
}

int postprocessing_bloom_upsample_get_info(postprocessing_bloom_upsample_handle_t handle, postprocessing_bloom_upsample_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_bloom_upsample_ctx.count) {
        return -2;
    }

    const postprocessing_bloom_upsample_internal_t* item = &g_bloom_upsample_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->current_params = item->params;

    return 0;
}

void postprocessing_bloom_upsample_mark_dirty(postprocessing_bloom_upsample_handle_t handle) {
    if (handle.id < g_bloom_upsample_ctx.count) {
        g_bloom_upsample_ctx.items[handle.id].dirty = true;
    }
}

int postprocessing_bloom_upsample_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_bloom_upsample_ctx.count; i++) {
        postprocessing_bloom_upsample_internal_t* item = &g_bloom_upsample_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

void postprocessing_bloom_upsample_buffer(const bloom_upsample_params_t* params, const float* input_rgb, uint32_t in_width, uint32_t in_height, float* output_rgb, uint32_t out_width, uint32_t out_height) {
    if (!input_rgb || !output_rgb) return;
    
    // Scale factors
    float scale_x = (float)in_width / out_width;
    float scale_y = (float)in_height / out_height;
    
    float radius = params ? params->filter_radius : 0.005f; // Radius in UV space or texels?
    // Let's assume params->filter_radius is a scalar multiplier for typical offset (1 texel).
    // If radius is "1.0", we sample at +/- 1 texel offset.
    
    float off_u = 1.0f; 
    float off_v = 1.0f;
    
    if (params && params->filter_radius > 0.0001f) {
        // Interpret radius as multiplier?
        off_u = params->filter_radius * in_width; // if radius is UV
        // Let's just assume radius is multiplier: 1.0 = standard tent.
        off_u = params->filter_radius;
        off_v = params->filter_radius;
    } else {
        off_u = 1.0f;
        off_v = 1.0f;
    }

    for (uint32_t y = 0; y < out_height; y++) {
        for (uint32_t x = 0; x < out_width; x++) {
            float u = (x + 0.5f) * scale_x;
            float v = (y + 0.5f) * scale_y;
            
            // 3x3 Tent Filter
            // Center sample: weight 4
            // 4 orthogonal: weight 2
            // 4 diagonal: weight 1
            // Total weight: 16
            
            float r_sum = 0, g_sum = 0, b_sum = 0;
            float r, g, b;
            
            // Center
            sample_bilinear_u(input_rgb, in_width, in_height, u, v, &r, &g, &b);
            r_sum += r * 4.0f; g_sum += g * 4.0f; b_sum += b * 4.0f;
            
            // Orthogonal (+-1, 0) and (0, +-1)
            // Left
            sample_bilinear_u(input_rgb, in_width, in_height, u - off_u, v, &r, &g, &b);
            r_sum += r * 2.0f; g_sum += g * 2.0f; b_sum += b * 2.0f;
            // Right
            sample_bilinear_u(input_rgb, in_width, in_height, u + off_u, v, &r, &g, &b);
            r_sum += r * 2.0f; g_sum += g * 2.0f; b_sum += b * 2.0f;
            // Top
            sample_bilinear_u(input_rgb, in_width, in_height, u, v - off_v, &r, &g, &b);
            r_sum += r * 2.0f; g_sum += g * 2.0f; b_sum += b * 2.0f;
            // Bottom
            sample_bilinear_u(input_rgb, in_width, in_height, u, v + off_v, &r, &g, &b);
            r_sum += r * 2.0f; g_sum += g * 2.0f; b_sum += b * 2.0f;
            
            // Diagonal (+-1, +-1)
            // TL
            sample_bilinear_u(input_rgb, in_width, in_height, u - off_u, v - off_v, &r, &g, &b);
            r_sum += r * 1.0f; g_sum += g * 1.0f; b_sum += b * 1.0f;
            // TR
            sample_bilinear_u(input_rgb, in_width, in_height, u + off_u, v - off_v, &r, &g, &b);
            r_sum += r * 1.0f; g_sum += g * 1.0f; b_sum += b * 1.0f;
            // BL
            sample_bilinear_u(input_rgb, in_width, in_height, u - off_u, v + off_v, &r, &g, &b);
            r_sum += r * 1.0f; g_sum += g * 1.0f; b_sum += b * 1.0f;
            // BR
            sample_bilinear_u(input_rgb, in_width, in_height, u + off_u, v + off_v, &r, &g, &b);
            r_sum += r * 1.0f; g_sum += g * 1.0f; b_sum += b * 1.0f;
            
            uint32_t out_idx = (y * out_width + x) * 3;
            output_rgb[out_idx] = r_sum * 0.0625f;   // / 16.0
            output_rgb[out_idx+1] = g_sum * 0.0625f;
            output_rgb[out_idx+2] = b_sum * 0.0625f;
        }
    }
}

uint32_t postprocessing_bloom_upsample_get_count(void) {
    return g_bloom_upsample_ctx.count;
}

size_t postprocessing_bloom_upsample_get_memory_usage(void) {
    size_t total = sizeof(g_bloom_upsample_ctx);
    total += g_bloom_upsample_ctx.capacity * sizeof(postprocessing_bloom_upsample_internal_t);

    for (uint32_t i = 0; i < g_bloom_upsample_ctx.count; i++) {
        total += g_bloom_upsample_ctx.items[i].data_size;
    }

    return total;
}

void postprocessing_bloom_upsample_debug_print(void) {
    printf("Bloom Upsample Context: %u/%u items\n", g_bloom_upsample_ctx.count, g_bloom_upsample_ctx.capacity);
}

/* End of bloom_upsample.c */
