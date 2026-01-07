/*
 * bloom_downsample.c
 * Bloom downsample chain
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
 * TODO: Implement bloom downsample initialization
 * TODO: Add bloom downsample cleanup/shutdown
 * TODO: Implement bloom downsample validation
 * TODO: Add bloom downsample error handling
 * TODO: Implement bloom downsample serialization
 * TODO: Add bloom downsample debug output
 * TODO: Implement bloom downsample unit tests
 * TODO: Add bloom downsample performance counters
 * TODO: Implement bloom downsample hot-reload
 * TODO: Add bloom downsample thread safety
 * TODO: Implement bloom downsample memory pooling
 * TODO: Add bloom downsample caching layer
 * TODO: Implement bloom downsample async operations
 * TODO: Add bloom downsample GPU integration
 * TODO: Implement bloom downsample SIMD optimization
 * TODO: Add bloom downsample batch processing
 * TODO: Implement bloom downsample streaming support
 * TODO: Add bloom downsample LOD support
 * TODO: Implement bloom downsample culling integration
 * TODO: Add bloom downsample render graph node
 */

#include "postprocess/bloom/bloom_downsample.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define POSTPROCESSING_BLOOM_DOWNSAMPLE_MAX_COUNT 4096
#define POSTPROCESSING_BLOOM_DOWNSAMPLE_DEFAULT_CAPACITY 256
#define POSTPROCESSING_BLOOM_DOWNSAMPLE_ALIGNMENT 16

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct postprocessing_bloom_downsample_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} postprocessing_bloom_downsample_internal_t;

typedef struct postprocessing_bloom_downsample_context {
    postprocessing_bloom_downsample_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} postprocessing_bloom_downsample_context_t;

static postprocessing_bloom_downsample_context_t g_bloom_downsample_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void postprocessing_bloom_downsample_cleanup_internal(postprocessing_bloom_downsample_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

static void sample_pixel(const float* input, uint32_t w, uint32_t h, int x, int y, float* r, float* g, float* b) {
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

static void sample_bilinear(const float* input, uint32_t w, uint32_t h, float u, float v, float* r, float* g, float* b) {
    // u, v are in pixel coordinates (0..w, 0..h)
    // Center at -0.5 for pixel centers? Assume u,v are exact coordinates where integer is pixel center.
    // If integer is center, then floor(u - 0.5) is x0.
    
    // Standard texture coordinate logic: (0.5, 0.5) is center of pixel (0,0).
    // Let's assume u,v are 0-indexed pixel coordinates.
    
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
    
    sample_pixel(input, w, h, x0, y0, &r00, &g00, &b00);
    sample_pixel(input, w, h, x1, y0, &r10, &g10, &b10);
    sample_pixel(input, w, h, x0, y1, &r01, &g01, &b01);
    sample_pixel(input, w, h, x1, y1, &r11, &g11, &b11);
    
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

int postprocessing_bloom_downsample_init(void) {
    if (g_bloom_downsample_ctx.initialized) {
        return 0; // Already initialized
    }

    g_bloom_downsample_ctx.capacity = POSTPROCESSING_BLOOM_DOWNSAMPLE_DEFAULT_CAPACITY;
    g_bloom_downsample_ctx.items = calloc(g_bloom_downsample_ctx.capacity, sizeof(postprocessing_bloom_downsample_internal_t));
    if (!g_bloom_downsample_ctx.items) {
        return -1;
    }

    g_bloom_downsample_ctx.count = 0;
    g_bloom_downsample_ctx.initialized = true;

    return 0;
}

void postprocessing_bloom_downsample_shutdown(void) {
    if (!g_bloom_downsample_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_bloom_downsample_ctx.count; i++) {
        postprocessing_bloom_downsample_cleanup_internal(&g_bloom_downsample_ctx.items[i]);
    }

    free(g_bloom_downsample_ctx.items);
    g_bloom_downsample_ctx.items = NULL;
    g_bloom_downsample_ctx.count = 0;
    g_bloom_downsample_ctx.capacity = 0;
    g_bloom_downsample_ctx.initialized = false;
}

int postprocessing_bloom_downsample_create(postprocessing_bloom_downsample_handle_t* out_handle, const postprocessing_bloom_downsample_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_bloom_downsample_ctx.initialized) {
        return -2;
    }

    if (g_bloom_downsample_ctx.count >= g_bloom_downsample_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_bloom_downsample_ctx.count++;
    postprocessing_bloom_downsample_internal_t* item = &g_bloom_downsample_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void postprocessing_bloom_downsample_destroy(postprocessing_bloom_downsample_handle_t handle) {
    if (handle.id >= g_bloom_downsample_ctx.count) {
        return;
    }

    postprocessing_bloom_downsample_cleanup_internal(&g_bloom_downsample_ctx.items[handle.id]);
}

int postprocessing_bloom_downsample_update(postprocessing_bloom_downsample_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_bloom_downsample_ctx.count) {
        return -1;
    }

    postprocessing_bloom_downsample_internal_t* item = &g_bloom_downsample_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool postprocessing_bloom_downsample_is_valid(postprocessing_bloom_downsample_handle_t handle) {
    if (handle.id >= g_bloom_downsample_ctx.count) {
        return false;
    }
    return g_bloom_downsample_ctx.items[handle.id].initialized;
}

int postprocessing_bloom_downsample_get_info(postprocessing_bloom_downsample_handle_t handle, postprocessing_bloom_downsample_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_bloom_downsample_ctx.count) {
        return -2;
    }

    const postprocessing_bloom_downsample_internal_t* item = &g_bloom_downsample_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void postprocessing_bloom_downsample_mark_dirty(postprocessing_bloom_downsample_handle_t handle) {
    if (handle.id < g_bloom_downsample_ctx.count) {
        g_bloom_downsample_ctx.items[handle.id].dirty = true;
    }
}

int postprocessing_bloom_downsample_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_bloom_downsample_ctx.count; i++) {
        postprocessing_bloom_downsample_internal_t* item = &g_bloom_downsample_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

void postprocessing_bloom_downsample_buffer(const float* input_rgb, uint32_t in_width, uint32_t in_height, float* output_rgb, uint32_t out_width, uint32_t out_height) {
    if (!input_rgb || !output_rgb) return;
    
    // Scale factors
    float scale_x = (float)in_width / out_width;
    float scale_y = (float)in_height / out_height;
    
    for (uint32_t y = 0; y < out_height; y++) {
        for (uint32_t x = 0; x < out_width; x++) {
            // Source center coordinates in input buffer
            float u = (x + 0.5f) * scale_x;
            float v = (y + 0.5f) * scale_y; // Center of pixel in input coords?
            // Actually usually we sample at (u,v).
            
            // 5-tap (13-sample) filter pattern
            // A . B
            // . C .
            // D . E
            
            // Groups of 4 pixels (bilinear)
            // A = (-2, -2) offset? No, usually spacing is 1 unit in output space or source space?
            // Karis: "Source mip resolution".
            
            // Assumes standard downsample:
            // Group Center: (u, v)
            // Group TL: (u - 2, v - 2) ?
            
            // Let's use offset = 1.0 (one texel radius)
            // A = sample(u - 2, v - 2)
            // B = sample(u + 2, v - 2)
            // ...
            // M = sample(u, v)
            
            // Wait, Karis partial average:
            // "Groups": A (top-left), B (top-right), C (bottom-left), D (bottom-right), E (center)
            // E = sample(0, 0)
            // A = sample(-2, -2)? Or (-1, -1)?
            // Using (-1, -1) from center logic relative to output pixel projected to input.
            
            // Let's assume standard unity/unreal bloom downsample offsets in source texels:
            // Center (0,0)
            // TL (-2, -2) ??
            // Offsets are usually (dx, dy) where dx = 1.0 / width.
            
            // For simple CPU sim, let's just do:
            // Center
            // 4 diagonal neighbors at distance 1 texel?
            
            float r_sum = 0, g_sum = 0, b_sum = 0;
            
            float r_samp, g_samp, b_samp;
            
            // Center
            sample_bilinear(input_rgb, in_width, in_height, u, v, &r_samp, &g_samp, &b_samp);
            r_sum += r_samp * 0.5f; g_sum += g_samp * 0.5f; b_sum += b_samp * 0.5f;
            
            // TL (-2, -2) input texels? or output?
            // Downsample usually implies taking a wider area.
            // Let's use offset 1.0 input texels.
            
            float off = 1.0f; // offset radius
            
            // TL
            sample_bilinear(input_rgb, in_width, in_height, u - off, v - off, &r_samp, &g_samp, &b_samp);
            r_sum += r_samp * 0.125f; g_sum += g_samp * 0.125f; b_sum += b_samp * 0.125f;
            
            // TR
            sample_bilinear(input_rgb, in_width, in_height, u + off, v - off, &r_samp, &g_samp, &b_samp);
            r_sum += r_samp * 0.125f; g_sum += g_samp * 0.125f; b_sum += b_samp * 0.125f;
            
            // BL
            sample_bilinear(input_rgb, in_width, in_height, u - off, v + off, &r_samp, &g_samp, &b_samp);
            r_sum += r_samp * 0.125f; g_sum += g_samp * 0.125f; b_sum += b_samp * 0.125f;
            
            // BR
            sample_bilinear(input_rgb, in_width, in_height, u + off, v + off, &r_samp, &g_samp, &b_samp);
            r_sum += r_samp * 0.125f; g_sum += g_samp * 0.125f; b_sum += b_samp * 0.125f;
            
            uint32_t out_idx = (y * out_width + x) * 3;
            output_rgb[out_idx] = r_sum;
            output_rgb[out_idx+1] = g_sum;
            output_rgb[out_idx+2] = b_sum;
        }
    }
}

uint32_t postprocessing_bloom_downsample_get_count(void) {
    return g_bloom_downsample_ctx.count;
}

size_t postprocessing_bloom_downsample_get_memory_usage(void) {
    size_t total = sizeof(g_bloom_downsample_ctx);
    total += g_bloom_downsample_ctx.capacity * sizeof(postprocessing_bloom_downsample_internal_t);

    for (uint32_t i = 0; i < g_bloom_downsample_ctx.count; i++) {
        total += g_bloom_downsample_ctx.items[i].data_size;
    }

    return total;
}

void postprocessing_bloom_downsample_debug_print(void) {
    printf("Bloom Downsample Context: %u/%u items\n", g_bloom_downsample_ctx.count, g_bloom_downsample_ctx.capacity);
}

/* End of bloom_downsample.c */
