/*
 * sss_transmission.c
 * SSS transmission
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "sss_transmission.h"
#include "../../math/vec3.h"
#include "../../../include/math/math.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_SSS_TRANSMISSION_MAX_COUNT 64
#define SHADING_SSS_TRANSMISSION_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct sss_transmission_data {
    vec3_t transmission_color;
    float thickness_scale;
    float distortion; // Distorts light direction
    float power;      // Power falloff
    float scale;      // Intensity scale
} sss_transmission_data_t;

typedef struct shading_sss_transmission_internal {
    uint32_t id;
    uint32_t flags;
    sss_transmission_data_t data;
    bool initialized;
    bool dirty;
} shading_sss_transmission_internal_t;

typedef struct shading_sss_transmission_context {
    shading_sss_transmission_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_sss_transmission_context_t;

static shading_sss_transmission_context_t g_sss_transmission_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_sss_transmission_init(void) {
    if (g_sss_transmission_ctx.initialized) return 0;

    g_sss_transmission_ctx.capacity = SHADING_SSS_TRANSMISSION_DEFAULT_CAPACITY;
    g_sss_transmission_ctx.items = calloc(g_sss_transmission_ctx.capacity, sizeof(shading_sss_transmission_internal_t));
    if (!g_sss_transmission_ctx.items) return -1;

    g_sss_transmission_ctx.count = 0;
    g_sss_transmission_ctx.initialized = true;

    return 0;
}

void shading_sss_transmission_shutdown(void) {
    if (!g_sss_transmission_ctx.initialized) return;

    free(g_sss_transmission_ctx.items);
    g_sss_transmission_ctx.items = NULL;
    g_sss_transmission_ctx.count = 0;
    g_sss_transmission_ctx.capacity = 0;
    g_sss_transmission_ctx.initialized = false;
}

int shading_sss_transmission_create(shading_sss_transmission_handle_t* out_handle, const shading_sss_transmission_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_sss_transmission_ctx.initialized) return -2;

    if (g_sss_transmission_ctx.count >= g_sss_transmission_ctx.capacity) {
        uint32_t new_cap = g_sss_transmission_ctx.capacity * 2;
        void* new_ptr = realloc(g_sss_transmission_ctx.items, new_cap * sizeof(shading_sss_transmission_internal_t));
        if (!new_ptr) return -3;
        
        g_sss_transmission_ctx.items = new_ptr;
        g_sss_transmission_ctx.capacity = new_cap;
    }

    uint32_t index = g_sss_transmission_ctx.count++;
    shading_sss_transmission_internal_t* item = &g_sss_transmission_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Default transmission values
    item->data.transmission_color = vec3_set(1.0f, 0.4f, 0.2f); // Default reddish transmission
    item->data.thickness_scale = 1.0f;
    item->data.distortion = 0.5f;
    item->data.power = 2.0f;
    item->data.scale = 1.0f;
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_sss_transmission_destroy(shading_sss_transmission_handle_t handle) {
    if (handle.id < g_sss_transmission_ctx.count) {
        g_sss_transmission_ctx.items[handle.id].initialized = false;
    }
}

int shading_sss_transmission_update(shading_sss_transmission_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_sss_transmission_ctx.count) return -1;
    shading_sss_transmission_internal_t* item = &g_sss_transmission_ctx.items[handle.id];
    
    if (size == sizeof(sss_transmission_data_t)) {
        memcpy(&item->data, data, sizeof(sss_transmission_data_t));
        item->dirty = true;
        return 0;
    }
    return -2;
}

bool shading_sss_transmission_is_valid(shading_sss_transmission_handle_t handle) {
    return handle.id < g_sss_transmission_ctx.count && g_sss_transmission_ctx.items[handle.id].initialized;
}

int shading_sss_transmission_get_info(shading_sss_transmission_handle_t handle, shading_sss_transmission_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= g_sss_transmission_ctx.count) return -2;
    
    const shading_sss_transmission_internal_t* item = &g_sss_transmission_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    return 0;
}

void shading_sss_transmission_mark_dirty(shading_sss_transmission_handle_t handle) {
    if (handle.id < g_sss_transmission_ctx.count) {
        g_sss_transmission_ctx.items[handle.id].dirty = true;
    }
}

int shading_sss_transmission_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_sss_transmission_ctx.count; i++) {
        shading_sss_transmission_internal_t* item = &g_sss_transmission_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t shading_sss_transmission_get_count(void) {
    return g_sss_transmission_ctx.count;
}

size_t shading_sss_transmission_get_memory_usage(void) {
    size_t total = sizeof(shading_sss_transmission_context_t);
    total += g_sss_transmission_ctx.capacity * sizeof(shading_sss_transmission_internal_t);
    return total;
}

void shading_sss_transmission_debug_print(void) {
    printf("SSS Transmission Context: %u items\n", g_sss_transmission_ctx.count);
}
