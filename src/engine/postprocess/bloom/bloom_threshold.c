/*
 * bloom_threshold.c
 * Bloom threshold filter
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
 * TODO: Implement bloom threshold initialization
 * TODO: Add bloom threshold cleanup/shutdown
 * TODO: Implement bloom threshold validation
 * TODO: Add bloom threshold error handling
 * TODO: Implement bloom threshold serialization
 * TODO: Add bloom threshold debug output
 * TODO: Implement bloom threshold unit tests
 * TODO: Add bloom threshold performance counters
 * TODO: Implement bloom threshold hot-reload
 * TODO: Add bloom threshold thread safety
 * TODO: Implement bloom threshold memory pooling
 * TODO: Add bloom threshold caching layer
 * TODO: Implement bloom threshold async operations
 * TODO: Add bloom threshold GPU integration
 * TODO: Implement bloom threshold SIMD optimization
 * TODO: Add bloom threshold batch processing
 * TODO: Implement bloom threshold streaming support
 * TODO: Add bloom threshold LOD support
 * TODO: Implement bloom threshold culling integration
 * TODO: Add bloom threshold render graph node
 */

#include "postprocess/bloom/bloom_threshold.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define POSTPROCESSING_BLOOM_THRESHOLD_MAX_COUNT 4096
#define POSTPROCESSING_BLOOM_THRESHOLD_DEFAULT_CAPACITY 256
#define POSTPROCESSING_BLOOM_THRESHOLD_ALIGNMENT 16
#define BLOOM_EPSILON 0.0001f

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct postprocessing_bloom_threshold_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    bloom_threshold_params_t params;
} postprocessing_bloom_threshold_internal_t;

typedef struct postprocessing_bloom_threshold_context {
    postprocessing_bloom_threshold_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} postprocessing_bloom_threshold_context_t;

static postprocessing_bloom_threshold_context_t g_bloom_threshold_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void postprocessing_bloom_threshold_cleanup_internal(postprocessing_bloom_threshold_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

static float get_luminance(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

static float max_f(float a, float b) {
    return (a > b) ? a : b;
}

static float clamp_f(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int postprocessing_bloom_threshold_init(void) {
    if (g_bloom_threshold_ctx.initialized) {
        return 0; // Already initialized
    }

    g_bloom_threshold_ctx.capacity = POSTPROCESSING_BLOOM_THRESHOLD_DEFAULT_CAPACITY;
    g_bloom_threshold_ctx.items = calloc(g_bloom_threshold_ctx.capacity, sizeof(postprocessing_bloom_threshold_internal_t));
    if (!g_bloom_threshold_ctx.items) {
        return -1;
    }

    g_bloom_threshold_ctx.count = 0;
    g_bloom_threshold_ctx.initialized = true;

    return 0;
}

void postprocessing_bloom_threshold_shutdown(void) {
    if (!g_bloom_threshold_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_bloom_threshold_ctx.count; i++) {
        postprocessing_bloom_threshold_cleanup_internal(&g_bloom_threshold_ctx.items[i]);
    }

    free(g_bloom_threshold_ctx.items);
    g_bloom_threshold_ctx.items = NULL;
    g_bloom_threshold_ctx.count = 0;
    g_bloom_threshold_ctx.capacity = 0;
    g_bloom_threshold_ctx.initialized = false;
}

int postprocessing_bloom_threshold_create(postprocessing_bloom_threshold_handle_t* out_handle, const postprocessing_bloom_threshold_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_bloom_threshold_ctx.initialized) {
        return -2;
    }

    if (g_bloom_threshold_ctx.count >= g_bloom_threshold_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_bloom_threshold_ctx.count++;
    postprocessing_bloom_threshold_internal_t* item = &g_bloom_threshold_ctx.items[index];

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

void postprocessing_bloom_threshold_destroy(postprocessing_bloom_threshold_handle_t handle) {
    if (handle.id >= g_bloom_threshold_ctx.count) {
        return;
    }

    postprocessing_bloom_threshold_cleanup_internal(&g_bloom_threshold_ctx.items[handle.id]);
}

int postprocessing_bloom_threshold_update(postprocessing_bloom_threshold_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_bloom_threshold_ctx.count) {
        return -1;
    }

    postprocessing_bloom_threshold_internal_t* item = &g_bloom_threshold_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

void postprocessing_bloom_threshold_set_params(postprocessing_bloom_threshold_handle_t handle, const bloom_threshold_params_t* params) {
    if (handle.id >= g_bloom_threshold_ctx.count || !params) return;
    postprocessing_bloom_threshold_internal_t* item = &g_bloom_threshold_ctx.items[handle.id];
    if (item->initialized) {
        item->params = *params;
        item->dirty = true;
    }
}

bool postprocessing_bloom_threshold_is_valid(postprocessing_bloom_threshold_handle_t handle) {
    if (handle.id >= g_bloom_threshold_ctx.count) {
        return false;
    }
    return g_bloom_threshold_ctx.items[handle.id].initialized;
}

int postprocessing_bloom_threshold_get_info(postprocessing_bloom_threshold_handle_t handle, postprocessing_bloom_threshold_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_bloom_threshold_ctx.count) {
        return -2;
    }

    const postprocessing_bloom_threshold_internal_t* item = &g_bloom_threshold_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->current_params = item->params;

    return 0;
}

void postprocessing_bloom_threshold_mark_dirty(postprocessing_bloom_threshold_handle_t handle) {
    if (handle.id < g_bloom_threshold_ctx.count) {
        g_bloom_threshold_ctx.items[handle.id].dirty = true;
    }
}

int postprocessing_bloom_threshold_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_bloom_threshold_ctx.count; i++) {
        postprocessing_bloom_threshold_internal_t* item = &g_bloom_threshold_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

void postprocessing_bloom_apply_threshold(const bloom_threshold_params_t* params, float r, float g, float b, float* out_r, float* out_g, float* out_b) {
    float threshold = params ? params->threshold : 1.0f;
    float knee = params ? params->knee : 0.1f;
    
    float lum = get_luminance(r, g, b);
    
    // Standard quadratic threshold curve (Karis)
    // Partial derivation:
    // (o = output, l = input, t = threshold, k = knee)
    // o = max(l - t, 0) (hard threshold)
    // o = max(l - t + k, 0)^2 / (4 * k) (soft threshold area)
    
    float soft = lum - threshold + knee;
    soft = clamp_f(soft, 0.0f, 2.0f * knee);
    soft = soft * soft / (4.0f * knee + 0.00001f);
    
    float contribution = max_f(soft, lum - threshold);
    contribution /= max_f(lum, 0.00001f);
    
    *out_r = r * contribution;
    *out_g = g * contribution;
    *out_b = b * contribution;
}

void postprocessing_bloom_apply_threshold_buffer(const bloom_threshold_params_t* params, const float* input_rgb, float* output_rgb, size_t pixel_count) {
    for (size_t i = 0; i < pixel_count; i++) {
        postprocessing_bloom_apply_threshold(params, 
            input_rgb[i*3], input_rgb[i*3+1], input_rgb[i*3+2],
            &output_rgb[i*3], &output_rgb[i*3+1], &output_rgb[i*3+2]);
    }
}

uint32_t postprocessing_bloom_threshold_get_count(void) {
    return g_bloom_threshold_ctx.count;
}

size_t postprocessing_bloom_threshold_get_memory_usage(void) {
    size_t total = sizeof(g_bloom_threshold_ctx);
    total += g_bloom_threshold_ctx.capacity * sizeof(postprocessing_bloom_threshold_internal_t);

    for (uint32_t i = 0; i < g_bloom_threshold_ctx.count; i++) {
        total += g_bloom_threshold_ctx.items[i].data_size;
    }

    return total;
}

void postprocessing_bloom_threshold_debug_print(void) {
    printf("Bloom Threshold Context: %u/%u items\n", g_bloom_threshold_ctx.count, g_bloom_threshold_ctx.capacity);
}

/* End of bloom_threshold.c */
