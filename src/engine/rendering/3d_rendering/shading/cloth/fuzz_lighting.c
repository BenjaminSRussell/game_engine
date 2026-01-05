/*
 * fuzz_lighting.c
 * Fuzz lighting implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "fuzz_lighting.h"
#include "../../math/vec3.h"
#include "../../../include/math/math.h"
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

#define SHADING_FUZZ_LIGHTING_MAX_COUNT 64
#define SHADING_FUZZ_LIGHTING_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct fuzz_params {
    vec3_t fuzz_color;
    float fuzz_strength;
    float fuzz_power; // Controls sharpness of the effect
} fuzz_params_t;

typedef struct shading_fuzz_lighting_internal {
    uint32_t id;
    uint32_t flags;
    fuzz_params_t params;
    bool initialized;
    bool dirty;
} shading_fuzz_lighting_internal_t;

typedef struct shading_fuzz_lighting_context {
    shading_fuzz_lighting_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_fuzz_lighting_context_t;

static shading_fuzz_lighting_context_t g_fuzz_lighting_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_fuzz_lighting_init(void) {
    if (g_fuzz_lighting_ctx.initialized) return 0;

    g_fuzz_lighting_ctx.capacity = SHADING_FUZZ_LIGHTING_DEFAULT_CAPACITY;
    g_fuzz_lighting_ctx.items = calloc(g_fuzz_lighting_ctx.capacity, sizeof(shading_fuzz_lighting_internal_t));
    if (!g_fuzz_lighting_ctx.items) return -1;

    g_fuzz_lighting_ctx.count = 0;
    g_fuzz_lighting_ctx.initialized = true;

    return 0;
}

void shading_fuzz_lighting_shutdown(void) {
    if (!g_fuzz_lighting_ctx.initialized) return;

    free(g_fuzz_lighting_ctx.items);
    g_fuzz_lighting_ctx.items = NULL;
    g_fuzz_lighting_ctx.count = 0;
    g_fuzz_lighting_ctx.capacity = 0;
    g_fuzz_lighting_ctx.initialized = false;
}

int shading_fuzz_lighting_create(shading_fuzz_lighting_handle_t* out_handle, const shading_fuzz_lighting_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_fuzz_lighting_ctx.initialized) return -2;

    if (g_fuzz_lighting_ctx.count >= g_fuzz_lighting_ctx.capacity) {
        uint32_t new_cap = g_fuzz_lighting_ctx.capacity * 2;
        void* new_ptr = realloc(g_fuzz_lighting_ctx.items, new_cap * sizeof(shading_fuzz_lighting_internal_t));
        if (!new_ptr) return -3;
        
        g_fuzz_lighting_ctx.items = new_ptr;
        g_fuzz_lighting_ctx.capacity = new_cap;
    }

    uint32_t index = g_fuzz_lighting_ctx.count++;
    shading_fuzz_lighting_internal_t* item = &g_fuzz_lighting_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Default fuzz
    item->params.fuzz_color = vec3_set(1.0f, 1.0f, 1.0f);
    item->params.fuzz_strength = 1.0f;
    item->params.fuzz_power = 5.0f; // Typical fresnel power
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_fuzz_lighting_destroy(shading_fuzz_lighting_handle_t handle) {
    if (handle.id < g_fuzz_lighting_ctx.count) {
        g_fuzz_lighting_ctx.items[handle.id].initialized = false;
    }
}

int shading_fuzz_lighting_update(shading_fuzz_lighting_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_fuzz_lighting_ctx.count) return -1;
    shading_fuzz_lighting_internal_t* item = &g_fuzz_lighting_ctx.items[handle.id];
    
    if (size == sizeof(fuzz_params_t)) {
        memcpy(&item->params, data, sizeof(fuzz_params_t));
        item->dirty = true;
        return 0;
    }
    return -2;
}

bool shading_fuzz_lighting_is_valid(shading_fuzz_lighting_handle_t handle) {
    return handle.id < g_fuzz_lighting_ctx.count && g_fuzz_lighting_ctx.items[handle.id].initialized;
}

// Evaluate fuzz/rim lighting
void evaluate_fuzz_lighting(shading_fuzz_lighting_handle_t handle,
                           vec3_t N, vec3_t V,
                           vec3_t* out_fuzz) {
    
    if (!g_fuzz_lighting_ctx.initialized || handle.id >= g_fuzz_lighting_ctx.count) return;
    shading_fuzz_lighting_internal_t* item = &g_fuzz_lighting_ctx.items[handle.id];
    
    float NdotV = MAX(vec3_dot(N, V), 0.0f);
    
    // Simple Fresnel-based fuzz
    float fresnel = powf(1.0f - NdotV, item->params.fuzz_power);
    float fuzz_factor = fresnel * item->params.fuzz_strength;
    
    if (out_fuzz) {
        out_fuzz->x = item->params.fuzz_color.x * fuzz_factor;
        out_fuzz->y = item->params.fuzz_color.y * fuzz_factor;
        out_fuzz->z = item->params.fuzz_color.z * fuzz_factor;
    }
}

int shading_fuzz_lighting_get_info(shading_fuzz_lighting_handle_t handle, shading_fuzz_lighting_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= g_fuzz_lighting_ctx.count) return -2;
    
    const shading_fuzz_lighting_internal_t* item = &g_fuzz_lighting_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    return 0;
}

void shading_fuzz_lighting_mark_dirty(shading_fuzz_lighting_handle_t handle) {
    if (handle.id < g_fuzz_lighting_ctx.count) {
        g_fuzz_lighting_ctx.items[handle.id].dirty = true;
    }
}

int shading_fuzz_lighting_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_fuzz_lighting_ctx.count; i++) {
        if (g_fuzz_lighting_ctx.items[i].initialized && g_fuzz_lighting_ctx.items[i].dirty) {
            g_fuzz_lighting_ctx.items[i].dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t shading_fuzz_lighting_get_count(void) {
    return g_fuzz_lighting_ctx.count;
}

size_t shading_fuzz_lighting_get_memory_usage(void) {
    size_t total = sizeof(g_fuzz_lighting_ctx);
    total += g_fuzz_lighting_ctx.capacity * sizeof(shading_fuzz_lighting_internal_t);
    return total;
}

void shading_fuzz_lighting_debug_print(void) {
    printf("Fuzz Lighting Ctx: %u items\n", g_fuzz_lighting_ctx.count);
}
