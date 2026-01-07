/*
 * bloom_lens_dirt.c
 * Lens dirt overlay
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
 * TODO: Implement bloom lens dirt initialization
 * TODO: Add bloom lens dirt cleanup/shutdown
 * TODO: Implement bloom lens dirt validation
 * TODO: Add bloom lens dirt error handling
 * TODO: Implement bloom lens dirt serialization
 * TODO: Add bloom lens dirt debug output
 * TODO: Implement bloom lens dirt unit tests
 * TODO: Add bloom lens dirt performance counters
 * TODO: Implement bloom lens dirt hot-reload
 * TODO: Add bloom lens dirt thread safety
 * TODO: Implement bloom lens dirt memory pooling
 * TODO: Add bloom lens dirt caching layer
 * TODO: Implement bloom lens dirt async operations
 * TODO: Add bloom lens dirt GPU integration
 * TODO: Implement bloom lens dirt SIMD optimization
 * TODO: Add bloom lens dirt batch processing
 * TODO: Implement bloom lens dirt streaming support
 * TODO: Add bloom lens dirt LOD support
 * TODO: Implement bloom lens dirt culling integration
 * TODO: Add bloom lens dirt render graph node
 */

#include "postprocess/bloom/bloom_lens_dirt.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define POSTPROCESSING_BLOOM_LENS_DIRT_MAX_COUNT 4096
#define POSTPROCESSING_BLOOM_LENS_DIRT_DEFAULT_CAPACITY 256
#define POSTPROCESSING_BLOOM_LENS_DIRT_ALIGNMENT 16

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct postprocessing_bloom_lens_dirt_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    bloom_lens_dirt_params_t params;
} postprocessing_bloom_lens_dirt_internal_t;

typedef struct postprocessing_bloom_lens_dirt_context {
    postprocessing_bloom_lens_dirt_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} postprocessing_bloom_lens_dirt_context_t;

static postprocessing_bloom_lens_dirt_context_t g_bloom_lens_dirt_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void postprocessing_bloom_lens_dirt_cleanup_internal(postprocessing_bloom_lens_dirt_internal_t* item) {
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

int postprocessing_bloom_lens_dirt_init(void) {
    if (g_bloom_lens_dirt_ctx.initialized) {
        return 0; // Already initialized
    }

    g_bloom_lens_dirt_ctx.capacity = POSTPROCESSING_BLOOM_LENS_DIRT_DEFAULT_CAPACITY;
    g_bloom_lens_dirt_ctx.items = calloc(g_bloom_lens_dirt_ctx.capacity, sizeof(postprocessing_bloom_lens_dirt_internal_t));
    if (!g_bloom_lens_dirt_ctx.items) {
        return -1;
    }

    g_bloom_lens_dirt_ctx.count = 0;
    g_bloom_lens_dirt_ctx.initialized = true;

    return 0;
}

void postprocessing_bloom_lens_dirt_shutdown(void) {
    if (!g_bloom_lens_dirt_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_bloom_lens_dirt_ctx.count; i++) {
        postprocessing_bloom_lens_dirt_cleanup_internal(&g_bloom_lens_dirt_ctx.items[i]);
    }

    free(g_bloom_lens_dirt_ctx.items);
    g_bloom_lens_dirt_ctx.items = NULL;
    g_bloom_lens_dirt_ctx.count = 0;
    g_bloom_lens_dirt_ctx.capacity = 0;
    g_bloom_lens_dirt_ctx.initialized = false;
}

int postprocessing_bloom_lens_dirt_create(postprocessing_bloom_lens_dirt_handle_t* out_handle, const postprocessing_bloom_lens_dirt_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_bloom_lens_dirt_ctx.initialized) {
        return -2;
    }

    if (g_bloom_lens_dirt_ctx.count >= g_bloom_lens_dirt_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_bloom_lens_dirt_ctx.count++;
    postprocessing_bloom_lens_dirt_internal_t* item = &g_bloom_lens_dirt_ctx.items[index];

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

void postprocessing_bloom_lens_dirt_destroy(postprocessing_bloom_lens_dirt_handle_t handle) {
    if (handle.id >= g_bloom_lens_dirt_ctx.count) {
        return;
    }

    postprocessing_bloom_lens_dirt_cleanup_internal(&g_bloom_lens_dirt_ctx.items[handle.id]);
}

int postprocessing_bloom_lens_dirt_update(postprocessing_bloom_lens_dirt_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_bloom_lens_dirt_ctx.count) {
        return -1;
    }

    postprocessing_bloom_lens_dirt_internal_t* item = &g_bloom_lens_dirt_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

void postprocessing_bloom_lens_dirt_set_params(postprocessing_bloom_lens_dirt_handle_t handle, const bloom_lens_dirt_params_t* params) {
    if (handle.id >= g_bloom_lens_dirt_ctx.count || !params) return;
    postprocessing_bloom_lens_dirt_internal_t* item = &g_bloom_lens_dirt_ctx.items[handle.id];
    if (item->initialized) {
        item->params = *params;
        item->dirty = true;
    }
}

bool postprocessing_bloom_lens_dirt_is_valid(postprocessing_bloom_lens_dirt_handle_t handle) {
    if (handle.id >= g_bloom_lens_dirt_ctx.count) {
        return false;
    }
    return g_bloom_lens_dirt_ctx.items[handle.id].initialized;
}

int postprocessing_bloom_lens_dirt_get_info(postprocessing_bloom_lens_dirt_handle_t handle, postprocessing_bloom_lens_dirt_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_bloom_lens_dirt_ctx.count) {
        return -2;
    }

    const postprocessing_bloom_lens_dirt_internal_t* item = &g_bloom_lens_dirt_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->current_params = item->params;

    return 0;
}

void postprocessing_bloom_lens_dirt_mark_dirty(postprocessing_bloom_lens_dirt_handle_t handle) {
    if (handle.id < g_bloom_lens_dirt_ctx.count) {
        g_bloom_lens_dirt_ctx.items[handle.id].dirty = true;
    }
}

int postprocessing_bloom_lens_dirt_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_bloom_lens_dirt_ctx.count; i++) {
        postprocessing_bloom_lens_dirt_internal_t* item = &g_bloom_lens_dirt_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

void postprocessing_bloom_lens_dirt_apply(const bloom_lens_dirt_params_t* params, const float* bloom_buffer, const float* dirt_texture, float* output_buffer, size_t pixel_count) {
    if (!bloom_buffer || !output_buffer) return;
    
    // Default pass-through if no dirt texture or zero intensity
    float intensity = params ? params->intensity : 0.0f;
    
    if (intensity < 0.0001f || !dirt_texture) {
        // Just copy bloom to output if not already same buffer
        if (bloom_buffer != output_buffer) {
            memcpy(output_buffer, bloom_buffer, pixel_count * 3 * sizeof(float));
        }
        return;
    }
    
    for (size_t i = 0; i < pixel_count * 3; i++) {
        float bloom = bloom_buffer[i];
        float dirt = dirt_texture[i]; // Assumes dirt texture is same size as bloom buffer
        
        // Add dirt: bloom + bloom * dirt * intensity
        output_buffer[i] = bloom + (bloom * dirt * intensity);
    }
}

uint32_t postprocessing_bloom_lens_dirt_get_count(void) {
    return g_bloom_lens_dirt_ctx.count;
}

size_t postprocessing_bloom_lens_dirt_get_memory_usage(void) {
    size_t total = sizeof(g_bloom_lens_dirt_ctx);
    total += g_bloom_lens_dirt_ctx.capacity * sizeof(postprocessing_bloom_lens_dirt_internal_t);

    for (uint32_t i = 0; i < g_bloom_lens_dirt_ctx.count; i++) {
        total += g_bloom_lens_dirt_ctx.items[i].data_size;
    }

    return total;
}

void postprocessing_bloom_lens_dirt_debug_print(void) {
    printf("Bloom Lens Dirt Context: %u/%u items\n", g_bloom_lens_dirt_ctx.count, g_bloom_lens_dirt_ctx.capacity);
}

/* End of bloom_lens_dirt.c */
