/*
 * exposure_adaptation.c
 * Auto exposure
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
 * TODO: Implement exposure adaptation initialization
 * TODO: Add exposure adaptation cleanup/shutdown
 * TODO: Implement exposure adaptation validation
 * TODO: Add exposure adaptation error handling
 * TODO: Implement exposure adaptation serialization
 * TODO: Add exposure adaptation debug output
 * TODO: Implement exposure adaptation unit tests
 * TODO: Add exposure adaptation performance counters
 * TODO: Implement exposure adaptation hot-reload
 * TODO: Add exposure adaptation thread safety
 * TODO: Implement exposure adaptation memory pooling
 * TODO: Add exposure adaptation caching layer
 * TODO: Implement exposure adaptation async operations
 * TODO: Add exposure adaptation GPU integration
 * TODO: Implement exposure adaptation SIMD optimization
 * TODO: Add exposure adaptation batch processing
 * TODO: Implement exposure adaptation streaming support
 * TODO: Add exposure adaptation LOD support
 * TODO: Implement exposure adaptation culling integration
 * TODO: Add exposure adaptation render graph node
 */

#include "exposure_adaptation.h"
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

#define POSTPROCESSING_EXPOSURE_ADAPTATION_MAX_COUNT 4096
#define POSTPROCESSING_EXPOSURE_ADAPTATION_DEFAULT_CAPACITY 256
#define POSTPROCESSING_EXPOSURE_ADAPTATION_ALIGNMENT 16
#define DEFAULT_KEY_VALUE 0.18f // 18% middle gray

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct postprocessing_exposure_adaptation_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    exposure_params_t params;
    float current_exposure;
    
    // History buffer
    float history[64];
    uint32_t history_head;
    uint32_t history_count;
} postprocessing_exposure_adaptation_internal_t;

typedef struct postprocessing_exposure_adaptation_context {
    postprocessing_exposure_adaptation_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} postprocessing_exposure_adaptation_context_t;

static postprocessing_exposure_adaptation_context_t g_exposure_adaptation_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void postprocessing_exposure_adaptation_cleanup_internal(postprocessing_exposure_adaptation_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

static float clamp(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int postprocessing_exposure_adaptation_init(void) {
    if (g_exposure_adaptation_ctx.initialized) {
        return 0; // Already initialized
    }

    g_exposure_adaptation_ctx.capacity = POSTPROCESSING_EXPOSURE_ADAPTATION_DEFAULT_CAPACITY;
    g_exposure_adaptation_ctx.items = calloc(g_exposure_adaptation_ctx.capacity, sizeof(postprocessing_exposure_adaptation_internal_t));
    if (!g_exposure_adaptation_ctx.items) {
        return -1;
    }

    g_exposure_adaptation_ctx.count = 0;
    g_exposure_adaptation_ctx.initialized = true;

    return 0;
}

void postprocessing_exposure_adaptation_shutdown(void) {
    if (!g_exposure_adaptation_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_exposure_adaptation_ctx.count; i++) {
        postprocessing_exposure_adaptation_cleanup_internal(&g_exposure_adaptation_ctx.items[i]);
    }

    free(g_exposure_adaptation_ctx.items);
    g_exposure_adaptation_ctx.items = NULL;
    g_exposure_adaptation_ctx.count = 0;
    g_exposure_adaptation_ctx.capacity = 0;
    g_exposure_adaptation_ctx.initialized = false;
}

int postprocessing_exposure_adaptation_create(postprocessing_exposure_adaptation_handle_t* out_handle, const postprocessing_exposure_adaptation_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_exposure_adaptation_ctx.initialized) {
        return -2;
    }

    if (g_exposure_adaptation_ctx.count >= g_exposure_adaptation_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_exposure_adaptation_ctx.count++;
    postprocessing_exposure_adaptation_internal_t* item = &g_exposure_adaptation_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->params = desc->initial_params;
    item->current_exposure = 1.0f; // Default start
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    item->history_head = 0;
    item->history_count = 0;
    memset(item->history, 0, sizeof(item->history));

    out_handle->id = index;
    return 0;
}

void postprocessing_exposure_adaptation_destroy(postprocessing_exposure_adaptation_handle_t handle) {
    if (handle.id >= g_exposure_adaptation_ctx.count) {
        return;
    }

    postprocessing_exposure_adaptation_cleanup_internal(&g_exposure_adaptation_ctx.items[handle.id]);
}

int postprocessing_exposure_adaptation_update(postprocessing_exposure_adaptation_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_exposure_adaptation_ctx.count) {
        return -1;
    }

    postprocessing_exposure_adaptation_internal_t* item = &g_exposure_adaptation_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

void postprocessing_exposure_adaptation_set_params(postprocessing_exposure_adaptation_handle_t handle, const exposure_params_t* params) {
    if (handle.id >= g_exposure_adaptation_ctx.count || !params) return;
    postprocessing_exposure_adaptation_internal_t* item = &g_exposure_adaptation_ctx.items[handle.id];
    if (item->initialized) {
        item->params = *params;
        item->dirty = true;
    }
}

bool postprocessing_exposure_adaptation_is_valid(postprocessing_exposure_adaptation_handle_t handle) {
    if (handle.id >= g_exposure_adaptation_ctx.count) {
        return false;
    }
    return g_exposure_adaptation_ctx.items[handle.id].initialized;
}

int postprocessing_exposure_adaptation_get_info(postprocessing_exposure_adaptation_handle_t handle, postprocessing_exposure_adaptation_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_exposure_adaptation_ctx.count) {
        return -2;
    }

    const postprocessing_exposure_adaptation_internal_t* item = &g_exposure_adaptation_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->current_params = item->params;
    out_info->current_exposure = item->current_exposure;

    return 0;
}

void postprocessing_exposure_adaptation_mark_dirty(postprocessing_exposure_adaptation_handle_t handle) {
    if (handle.id < g_exposure_adaptation_ctx.count) {
        g_exposure_adaptation_ctx.items[handle.id].dirty = true;
    }
}

int postprocessing_exposure_adaptation_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_exposure_adaptation_ctx.count; i++) {
        postprocessing_exposure_adaptation_internal_t* item = &g_exposure_adaptation_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

float postprocessing_exposure_compute_target(const exposure_params_t* params, float avg_luminance) {
    if (!params) return 1.0f;
    float key_value = params->target_luminance > 0.0001f ? params->target_luminance : DEFAULT_KEY_VALUE;
    
    // Avoid division by zero
    float lum = avg_luminance > 0.0001f ? avg_luminance : 0.0001f;
    
    float target = key_value / lum;
    
    // Clamp
    if (params->use_ev) {
        // Convert EV to linear multiplier: pow(2, EV)
        float min_mult = powf(2.0f, params->min_exposure);
        float max_mult = powf(2.0f, params->max_exposure);
        return clamp(target, min_mult, max_mult);
    } else {
        return clamp(target, params->min_exposure, params->max_exposure);
    }
}

float postprocessing_exposure_adapt(const exposure_params_t* params, float current_exposure, float target_exposure, float dt) {
    if (!params) return target_exposure;
    
    if (params->manual_override) {
        return params->manual_exposure;
    }
    
    // Smooth adaptation
    // exposure = lerp(current, target, 1 - exp(-speed * dt))
    
    float speed = params->adaptation_speed > 0.0f ? params->adaptation_speed : 1.0f;
    float alpha = 1.0f - expf(-speed * dt);
    
    return lerp(current_exposure, target_exposure, alpha);
}

void postprocessing_exposure_set_manual(postprocessing_exposure_adaptation_handle_t handle, bool enabled, float exposure) {
    if (handle.id >= g_exposure_adaptation_ctx.count) return;
    postprocessing_exposure_adaptation_internal_t* item = &g_exposure_adaptation_ctx.items[handle.id];
    item->params.manual_override = enabled;
    item->params.manual_exposure = exposure;
    item->dirty = true;
}

void postprocessing_exposure_push_history(postprocessing_exposure_adaptation_handle_t handle, float exposure) {
    if (handle.id >= g_exposure_adaptation_ctx.count) return;
    postprocessing_exposure_adaptation_internal_t* item = &g_exposure_adaptation_ctx.items[handle.id];
    
    uint32_t size = item->params.history_size;
    if (size == 0) size = 1;
    if (size > 64) size = 64;
    
    item->history[item->history_head] = exposure;
    item->history_head = (item->history_head + 1) % size;
    if (item->history_count < size) item->history_count++;
}

float postprocessing_exposure_get_average_history(postprocessing_exposure_adaptation_handle_t handle) {
    if (handle.id >= g_exposure_adaptation_ctx.count) return 1.0f;
    postprocessing_exposure_adaptation_internal_t* item = &g_exposure_adaptation_ctx.items[handle.id];
    
    if (item->history_count == 0) return item->current_exposure;
    
    float sum = 0.0f;
    for (uint32_t i = 0; i < item->history_count; i++) {
        sum += item->history[i];
    }
    return sum / (float)item->history_count;
}

void postprocessing_exposure_clear_history(postprocessing_exposure_adaptation_handle_t handle) {
    if (handle.id >= g_exposure_adaptation_ctx.count) return;
    postprocessing_exposure_adaptation_internal_t* item = &g_exposure_adaptation_ctx.items[handle.id];
    item->history_count = 0;
    item->history_head = 0;
}

uint32_t postprocessing_exposure_adaptation_get_count(void) {
    return g_exposure_adaptation_ctx.count;
}

size_t postprocessing_exposure_adaptation_get_memory_usage(void) {
    size_t total = sizeof(g_exposure_adaptation_ctx);
    total += g_exposure_adaptation_ctx.capacity * sizeof(postprocessing_exposure_adaptation_internal_t);

    for (uint32_t i = 0; i < g_exposure_adaptation_ctx.count; i++) {
        if (g_exposure_adaptation_ctx.items[i].initialized) {
            total += g_exposure_adaptation_ctx.items[i].data_size;
        }
    }

    return total;
}

void postprocessing_exposure_adaptation_debug_print(void) {
    printf("Exposure Adaptation Context: %u/%u items\n", g_exposure_adaptation_ctx.count, g_exposure_adaptation_ctx.capacity);
}

/* End of exposure_adaptation.c */
