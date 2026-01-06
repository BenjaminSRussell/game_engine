/*
 * splat_blending.c
 * Splat layer blending
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement terrain LOD
 * TODO: Add terrain tessellation
 * TODO: Implement heightmap streaming
 * TODO: Add splat map rendering
 * TODO: Implement vegetation instancing
 * TODO: Add grass rendering
 * TODO: Implement procedural terrain
 * TODO: Add erosion simulation
 * TODO: Implement virtual heightmaps
 * TODO: Add terrain holes
 * TODO: Implement splat blending initialization
 * TODO: Add splat blending cleanup/shutdown
 * TODO: Implement splat blending validation
 * TODO: Add splat blending error handling
 * TODO: Implement splat blending serialization
 * TODO: Add splat blending debug output
 * TODO: Implement splat blending unit tests
 * TODO: Add splat blending performance counters
 * TODO: Implement splat blending hot-reload
 * TODO: Add splat blending thread safety
 * TODO: Implement splat blending memory pooling
 * TODO: Add splat blending caching layer
 * TODO: Implement splat blending async operations
 * TODO: Add splat blending GPU integration
 * TODO: Implement splat blending SIMD optimization
 * TODO: Add splat blending batch processing
 * TODO: Implement splat blending streaming support
 * TODO: Add splat blending LOD support
 * TODO: Implement splat blending culling integration
 * TODO: Add splat blending render graph node
 */

#include "splat_blending.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LANDSCAPE_SPLAT_BLENDING_MAX_COUNT 4096
#define LANDSCAPE_SPLAT_BLENDING_DEFAULT_CAPACITY 256
#define LANDSCAPE_SPLAT_BLENDING_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_splat_blending_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} landscape_splat_blending_internal_t;

typedef struct landscape_splat_blending_context {
    landscape_splat_blending_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} landscape_splat_blending_context_t;

static landscape_splat_blending_context_t g_splat_blending_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

#include <math.h>

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool landscape_splat_blending_validate(const landscape_splat_blending_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void landscape_splat_blending_cleanup_internal(landscape_splat_blending_internal_t* item) {
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

int landscape_splat_blending_init(void) {
    if (g_splat_blending_ctx.initialized) {
        return 0; // Already initialized
    }

    g_splat_blending_ctx.capacity = LANDSCAPE_SPLAT_BLENDING_DEFAULT_CAPACITY;
    g_splat_blending_ctx.items = calloc(g_splat_blending_ctx.capacity, sizeof(landscape_splat_blending_internal_t));
    if (!g_splat_blending_ctx.items) {
        return -1;
    }

    g_splat_blending_ctx.count = 0;
    g_splat_blending_ctx.initialized = true;

    return 0;
}

void landscape_splat_blending_shutdown(void) {
    if (!g_splat_blending_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_splat_blending_ctx.count; i++) {
        landscape_splat_blending_cleanup_internal(&g_splat_blending_ctx.items[i]);
    }

    free(g_splat_blending_ctx.items);
    g_splat_blending_ctx.items = NULL;
    g_splat_blending_ctx.count = 0;
    g_splat_blending_ctx.capacity = 0;
    g_splat_blending_ctx.initialized = false;
}

int landscape_splat_blending_create(landscape_splat_blending_handle_t* out_handle, const landscape_splat_blending_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_splat_blending_ctx.initialized) {
        return -2;
    }

    if (g_splat_blending_ctx.count >= g_splat_blending_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_splat_blending_ctx.count++;
    landscape_splat_blending_internal_t* item = &g_splat_blending_ctx.items[index];

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

void landscape_splat_blending_destroy(landscape_splat_blending_handle_t handle) {
    if (handle.id >= g_splat_blending_ctx.count) {
        return;
    }

    landscape_splat_blending_cleanup_internal(&g_splat_blending_ctx.items[handle.id]);
}

int landscape_splat_blending_update(landscape_splat_blending_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_splat_blending_ctx.count) {
        return -1;
    }

    landscape_splat_blending_internal_t* item = &g_splat_blending_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool landscape_splat_blending_is_valid(landscape_splat_blending_handle_t handle) {
    if (handle.id >= g_splat_blending_ctx.count) {
        return false;
    }
    return g_splat_blending_ctx.items[handle.id].initialized;
}

int landscape_splat_blending_get_info(landscape_splat_blending_handle_t handle, landscape_splat_blending_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_splat_blending_ctx.count) {
        return -2;
    }

    const landscape_splat_blending_internal_t* item = &g_splat_blending_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void landscape_splat_blending_mark_dirty(landscape_splat_blending_handle_t handle) {
    if (handle.id < g_splat_blending_ctx.count) {
        g_splat_blending_ctx.items[handle.id].dirty = true;
    }
}

int landscape_splat_blending_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_splat_blending_ctx.count; i++) {
        landscape_splat_blending_internal_t* item = &g_splat_blending_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

void landscape_splat_calculate_weights(float* out_weights, const float* layer_heights, const float* layer_alphas, float height_blend_falloff) {
    if (!out_weights || !layer_heights || !layer_alphas) return;

    // Height-based blending logic
    // We modify the layer alpha by the height map value
    
    // 1. Calculate weighted heights
    // Common approach: weight = alpha + height
    // Better approach: max_based blending
    
    float weighted_heights[4];
    float max_height = -1000.0f;
    
    for (int i = 0; i < 4; i++) {
        // Simple approximation: alpha * (1 + height)
        // Or standard height blend: height + factor * alpha?
        // Let's use standard height mix:
        // h = texture_height * alpha
        // But alpha comes from splatmap which is the base blend.
        
        // Using "Height-Based Texture Blending" algorithm
        // depth = 0.2 (falloff)
        // ma = max(h1 + a1, h2 + a2, ...) - depth
        // b1 = max(h1 + a1 - ma, 0)
        
        weighted_heights[i] = layer_heights[i] + layer_alphas[i];
        if (weighted_heights[i] > max_height) {
            max_height = weighted_heights[i];
        }
    }
    
    float sum_weights = 0.0f;
    float transition = fmaxf(0.001f, height_blend_falloff); // depth
    float threshold = max_height - transition;
    
    for (int i = 0; i < 4; i++) {
        float w = fmaxf(weighted_heights[i] - threshold, 0.0f);
        // Apply alpha mask strength to ensure zero-alpha layers stay zero if desired
        // But the algorithm handles it. Just multiply by alpha again if strict masking needed.
        if (layer_alphas[i] <= 0.001f) w = 0.0f; 
        
        out_weights[i] = w;
        sum_weights += w;
    }
    
    // Normalize
    if (sum_weights > 0.0001f) {
        float inv_sum = 1.0f / sum_weights;
        for (int i = 0; i < 4; i++) {
            out_weights[i] *= inv_sum;
        }
    } else {
        // Fallback if all weights zero (shouldn't happen with valid inputs)
        out_weights[0] = 1.0f;
        out_weights[1] = 0.0f;
        out_weights[2] = 0.0f;
        out_weights[3] = 0.0f;
    }
}

uint32_t landscape_splat_blending_get_count(void) {
    return g_splat_blending_ctx.count;
}

size_t landscape_splat_blending_get_memory_usage(void) {
    size_t total = sizeof(g_splat_blending_ctx);
    total += g_splat_blending_ctx.capacity * sizeof(landscape_splat_blending_internal_t);
    for (uint32_t i = 0; i < g_splat_blending_ctx.count; i++) {
        total += g_splat_blending_ctx.items[i].data_size;
    }
    return total;
}

void landscape_splat_blending_debug_print(void) {
    // Debug printing implementation
}

/* End of splat_blending.c */
