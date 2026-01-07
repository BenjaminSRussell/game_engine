/*
 * car_paint.c
 * Car paint model implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "shading/clearcoat/car_paint.h"
#include "include/math/vec3.h"
#include "include/math/math.h"
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

#define SHADING_CAR_PAINT_MAX_COUNT 64
#define SHADING_CAR_PAINT_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct car_paint_params {
    vec3_t base_color;
    vec3_t flake_color;
    float flake_density;
    float flake_roughness;
    float flake_scale;
    float coat_strength;
    float coat_roughness;
} car_paint_params_t;

typedef struct shading_car_paint_internal {
    uint32_t id;
    uint32_t flags;
    car_paint_params_t params;
    bool initialized;
    bool dirty;
} shading_car_paint_internal_t;

typedef struct shading_car_paint_context {
    shading_car_paint_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_car_paint_context_t;

static shading_car_paint_context_t g_car_paint_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Pseudo-random noise for flake storage simulation
static float flake_noise(float x, float y, float z) {
    // Simple hash
    float n = sinf(x * 12.9898f + y * 78.233f + z * 54.53f) * 43758.5453f;
    return n - floorf(n);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_car_paint_init(void) {
    if (g_car_paint_ctx.initialized) return 0;

    g_car_paint_ctx.capacity = SHADING_CAR_PAINT_DEFAULT_CAPACITY;
    g_car_paint_ctx.items = calloc(g_car_paint_ctx.capacity, sizeof(shading_car_paint_internal_t));
    if (!g_car_paint_ctx.items) return -1;

    g_car_paint_ctx.count = 0;
    g_car_paint_ctx.initialized = true;

    return 0;
}

void shading_car_paint_shutdown(void) {
    if (!g_car_paint_ctx.initialized) return;

    free(g_car_paint_ctx.items);
    g_car_paint_ctx.items = NULL;
    g_car_paint_ctx.count = 0;
    g_car_paint_ctx.capacity = 0;
    g_car_paint_ctx.initialized = false;
}

int shading_car_paint_create(shading_car_paint_handle_t* out_handle, const shading_car_paint_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_car_paint_ctx.initialized) return -2;

    if (g_car_paint_ctx.count >= g_car_paint_ctx.capacity) {
        uint32_t new_cap = g_car_paint_ctx.capacity * 2;
        void* new_ptr = realloc(g_car_paint_ctx.items, new_cap * sizeof(shading_car_paint_internal_t));
        if (!new_ptr) return -3;
        
        g_car_paint_ctx.items = new_ptr;
        g_car_paint_ctx.capacity = new_cap;
    }

    uint32_t index = g_car_paint_ctx.count++;
    shading_car_paint_internal_t* item = &g_car_paint_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Default blue metallic
    item->params.base_color = vec3_set(0.0f, 0.1f, 0.4f);
    item->params.flake_color = vec3_set(0.8f, 0.9f, 1.0f);
    item->params.flake_density = 1.0f;
    item->params.flake_roughness = 0.2f;
    item->params.flake_scale = 100.0f;
    item->params.coat_strength = 1.0f;
    item->params.coat_roughness = 0.04f;
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_car_paint_destroy(shading_car_paint_handle_t handle) {
    if (handle.id < g_car_paint_ctx.count) {
        g_car_paint_ctx.items[handle.id].initialized = false;
    }
}

int shading_car_paint_update(shading_car_paint_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_car_paint_ctx.count) return -1;
    shading_car_paint_internal_t* item = &g_car_paint_ctx.items[handle.id];
    
    if (size == sizeof(car_paint_params_t)) {
        memcpy(&item->params, data, sizeof(car_paint_params_t));
        item->dirty = true;
        return 0;
    }
    return -2;
}

bool shading_car_paint_is_valid(shading_car_paint_handle_t handle) {
    return handle.id < g_car_paint_ctx.count && g_car_paint_ctx.items[handle.id].initialized;
}

// Get effective normal for flakes
void evaluate_car_paint_flakes(shading_car_paint_handle_t handle,
                              vec3_t world_pos, vec3_t geometric_normal,
                              vec3_t* out_flake_normal) {
                                  
    if (!g_car_paint_ctx.initialized || handle.id >= g_car_paint_ctx.count) return;
    shading_car_paint_internal_t* item = &g_car_paint_ctx.items[handle.id];
    
    if (out_flake_normal) {
        // Simple procedural noise to perturb normal
        // Real implementation would sample a dedicated flake normal map
        float scale = item->params.flake_scale;
        float nx = flake_noise(world_pos.x * scale, world_pos.y * scale, world_pos.z * scale);
        float ny = flake_noise(world_pos.x * scale + 13.0f, world_pos.y * scale + 2.0f, world_pos.z * scale + 5.0f);
        float nz = flake_noise(world_pos.x * scale - 4.0f, world_pos.y * scale + 8.0f, world_pos.z * scale - 1.0f);
        
        vec3_t perturbation = vec3_set(nx - 0.5f, ny - 0.5f, nz - 0.5f);
        perturbation = vec3_mul_scalar(perturbation, item->params.flake_density * 0.5f);
        
        vec3_t final_normal = vec3_add(geometric_normal, perturbation);
        *out_flake_normal = vec3_normalize(final_normal);
    }
}

int shading_car_paint_get_info(shading_car_paint_handle_t handle, shading_car_paint_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= g_car_paint_ctx.count) return -2;
    
    const shading_car_paint_internal_t* item = &g_car_paint_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    return 0;
}

void shading_car_paint_mark_dirty(shading_car_paint_handle_t handle) {
    if (handle.id < g_car_paint_ctx.count) {
        g_car_paint_ctx.items[handle.id].dirty = true;
    }
}

int shading_car_paint_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_car_paint_ctx.count; i++) {
        if (g_car_paint_ctx.items[i].initialized && g_car_paint_ctx.items[i].dirty) {
            g_car_paint_ctx.items[i].dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t shading_car_paint_get_count(void) {
    return g_car_paint_ctx.count;
}

size_t shading_car_paint_get_memory_usage(void) {
    size_t total = sizeof(g_car_paint_ctx);
    total += g_car_paint_ctx.capacity * sizeof(shading_car_paint_internal_t);
    return total;
}

void shading_car_paint_debug_print(void) {
    printf("Car Paint Ctx: %u items\n", g_car_paint_ctx.count);
}
