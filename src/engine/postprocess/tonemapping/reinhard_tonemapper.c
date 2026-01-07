/*
 * reinhard_tonemapper.c
 * Reinhard tonemapping
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
 * TODO: Implement reinhard tonemapper initialization
 * TODO: Add reinhard tonemapper cleanup/shutdown
 * TODO: Implement reinhard tonemapper validation
 * TODO: Add reinhard tonemapper error handling
 * TODO: Implement reinhard tonemapper serialization
 * TODO: Add reinhard tonemapper debug output
 * TODO: Implement reinhard tonemapper unit tests
 * TODO: Add reinhard tonemapper performance counters
 * TODO: Implement reinhard tonemapper hot-reload
 * TODO: Add reinhard tonemapper thread safety
 * TODO: Implement reinhard tonemapper memory pooling
 * TODO: Add reinhard tonemapper caching layer
 * TODO: Implement reinhard tonemapper async operations
 * TODO: Add reinhard tonemapper GPU integration
 * TODO: Implement reinhard tonemapper SIMD optimization
 * TODO: Add reinhard tonemapper batch processing
 * TODO: Implement reinhard tonemapper streaming support
 * TODO: Add reinhard tonemapper LOD support
 * TODO: Implement reinhard tonemapper culling integration
 * TODO: Add reinhard tonemapper render graph node
 */

#include "postprocess/tonemapping/reinhard_tonemapper.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define POSTPROCESSING_REINHARD_TONEMAPPER_MAX_COUNT 4096
#define POSTPROCESSING_REINHARD_TONEMAPPER_DEFAULT_CAPACITY 256
#define POSTPROCESSING_REINHARD_TONEMAPPER_ALIGNMENT 16

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct postprocessing_reinhard_tonemapper_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    reinhard_tonemap_params_t params;
} postprocessing_reinhard_tonemapper_internal_t;

typedef struct postprocessing_reinhard_tonemapper_context {
    postprocessing_reinhard_tonemapper_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} postprocessing_reinhard_tonemapper_context_t;

static postprocessing_reinhard_tonemapper_context_t g_reinhard_tonemapper_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void postprocessing_reinhard_tonemapper_cleanup_internal(postprocessing_reinhard_tonemapper_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

// Helper to get luminance
static float get_luminance(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int postprocessing_reinhard_tonemapper_init(void) {
    if (g_reinhard_tonemapper_ctx.initialized) {
        return 0; // Already initialized
    }

    g_reinhard_tonemapper_ctx.capacity = POSTPROCESSING_REINHARD_TONEMAPPER_DEFAULT_CAPACITY;
    g_reinhard_tonemapper_ctx.items = calloc(g_reinhard_tonemapper_ctx.capacity, sizeof(postprocessing_reinhard_tonemapper_internal_t));
    if (!g_reinhard_tonemapper_ctx.items) {
        return -1;
    }

    g_reinhard_tonemapper_ctx.count = 0;
    g_reinhard_tonemapper_ctx.initialized = true;

    return 0;
}

void postprocessing_reinhard_tonemapper_shutdown(void) {
    if (!g_reinhard_tonemapper_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_reinhard_tonemapper_ctx.count; i++) {
        postprocessing_reinhard_tonemapper_cleanup_internal(&g_reinhard_tonemapper_ctx.items[i]);
    }

    free(g_reinhard_tonemapper_ctx.items);
    g_reinhard_tonemapper_ctx.items = NULL;
    g_reinhard_tonemapper_ctx.count = 0;
    g_reinhard_tonemapper_ctx.capacity = 0;
    g_reinhard_tonemapper_ctx.initialized = false;
}

int postprocessing_reinhard_tonemapper_create(postprocessing_reinhard_tonemapper_handle_t* out_handle, const postprocessing_reinhard_tonemapper_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_reinhard_tonemapper_ctx.initialized) {
        return -2;
    }

    if (g_reinhard_tonemapper_ctx.count >= g_reinhard_tonemapper_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_reinhard_tonemapper_ctx.count++;
    postprocessing_reinhard_tonemapper_internal_t* item = &g_reinhard_tonemapper_ctx.items[index];

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

void postprocessing_reinhard_tonemapper_destroy(postprocessing_reinhard_tonemapper_handle_t handle) {
    if (handle.id >= g_reinhard_tonemapper_ctx.count) {
        return;
    }

    postprocessing_reinhard_tonemapper_cleanup_internal(&g_reinhard_tonemapper_ctx.items[handle.id]);
}

int postprocessing_reinhard_tonemapper_update(postprocessing_reinhard_tonemapper_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_reinhard_tonemapper_ctx.count) {
        return -1;
    }

    postprocessing_reinhard_tonemapper_internal_t* item = &g_reinhard_tonemapper_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

void postprocessing_reinhard_tonemapper_set_params(postprocessing_reinhard_tonemapper_handle_t handle, const reinhard_tonemap_params_t* params) {
    if (handle.id >= g_reinhard_tonemapper_ctx.count || !params) return;
    postprocessing_reinhard_tonemapper_internal_t* item = &g_reinhard_tonemapper_ctx.items[handle.id];
    if (item->initialized) {
        item->params = *params;
        item->dirty = true;
    }
}

bool postprocessing_reinhard_tonemapper_is_valid(postprocessing_reinhard_tonemapper_handle_t handle) {
    if (handle.id >= g_reinhard_tonemapper_ctx.count) {
        return false;
    }
    return g_reinhard_tonemapper_ctx.items[handle.id].initialized;
}

int postprocessing_reinhard_tonemapper_get_info(postprocessing_reinhard_tonemapper_handle_t handle, postprocessing_reinhard_tonemapper_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_reinhard_tonemapper_ctx.count) {
        return -2;
    }

    const postprocessing_reinhard_tonemapper_internal_t* item = &g_reinhard_tonemapper_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->current_params = item->params;

    return 0;
}

void postprocessing_reinhard_tonemapper_mark_dirty(postprocessing_reinhard_tonemapper_handle_t handle) {
    if (handle.id < g_reinhard_tonemapper_ctx.count) {
        g_reinhard_tonemapper_ctx.items[handle.id].dirty = true;
    }
}

int postprocessing_reinhard_tonemapper_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_reinhard_tonemapper_ctx.count; i++) {
        postprocessing_reinhard_tonemapper_internal_t* item = &g_reinhard_tonemapper_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

void postprocessing_reinhard_tonemap_color(const reinhard_tonemap_params_t* params, float r, float g, float b, float* out_r, float* out_g, float* out_b) {
    float exposure = params ? params->exposure : 1.0f;
    float white_sq = params ? params->white_point : 100.0f; // Default high white point
    
    // Safety check for white point
    if (white_sq < 0.001f) white_sq = 100.0f;

    // Apply exposure
    r *= exposure;
    g *= exposure;
    b *= exposure;

    float l_old = get_luminance(r, g, b);
    
    // Reinhard extended:
    // L_new = (L_old * (1 + L_old / L_white^2)) / (1 + L_old)
    float l_new = (l_old * (1.0f + (l_old / white_sq))) / (1.0f + l_old);
    
    // Color scaling
    // c_new = c_old * (L_new / L_old)
    float scale = (l_old > 1e-6f) ? (l_new / l_old) : 1.0f;
    
    *out_r = r * scale;
    *out_g = g * scale;
    *out_b = b * scale;
}

void postprocessing_reinhard_tonemap_buffer(const reinhard_tonemap_params_t* params, const float* input_rgb, float* output_rgb, size_t pixel_count) {
    for (size_t i = 0; i < pixel_count; i++) {
        postprocessing_reinhard_tonemap_color(params, 
            input_rgb[i*3], input_rgb[i*3+1], input_rgb[i*3+2],
            &output_rgb[i*3], &output_rgb[i*3+1], &output_rgb[i*3+2]);
    }
}

uint32_t postprocessing_reinhard_tonemapper_get_count(void) {
    return g_reinhard_tonemapper_ctx.count;
}

size_t postprocessing_reinhard_tonemapper_get_memory_usage(void) {
    size_t total = sizeof(g_reinhard_tonemapper_ctx);
    total += g_reinhard_tonemapper_ctx.capacity * sizeof(postprocessing_reinhard_tonemapper_internal_t);

    for (uint32_t i = 0; i < g_reinhard_tonemapper_ctx.count; i++) {
        if (g_reinhard_tonemapper_ctx.items[i].initialized) {
            total += g_reinhard_tonemapper_ctx.items[i].data_size;
        }
    }

    return total;
}

void postprocessing_reinhard_tonemapper_debug_print(void) {
    printf("Reinhard Tonemapper Context: %u/%u items\n", g_reinhard_tonemapper_ctx.count, g_reinhard_tonemapper_ctx.capacity);
}

/* End of reinhard_tonemapper.c */
