/*
 * hair_scattering.c
 * Hair multiple scattering implementation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#include "shading/hair/hair_scattering.h"
#include "include/math/vec3.h"
#include "include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SHADING_HAIR_SCATTERING_MAX_COUNT 64
#define SHADING_HAIR_SCATTERING_DEFAULT_CAPACITY 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct scattering_params {
    vec3_t scatter_color;
    float global_scatter_strength;
    float density_scale;
} scattering_params_t;

typedef struct shading_hair_scattering_internal {
    uint32_t id;
    uint32_t flags;
    scattering_params_t params;
    bool initialized;
    bool dirty;
} shading_hair_scattering_internal_t;

typedef struct shading_hair_scattering_context {
    shading_hair_scattering_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shading_hair_scattering_context_t;

static shading_hair_scattering_context_t g_hair_scattering_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int shading_hair_scattering_init(void) {
    if (g_hair_scattering_ctx.initialized) return 0;

    g_hair_scattering_ctx.capacity = SHADING_HAIR_SCATTERING_DEFAULT_CAPACITY;
    g_hair_scattering_ctx.items = calloc(g_hair_scattering_ctx.capacity, sizeof(shading_hair_scattering_internal_t));
    
    if (!g_hair_scattering_ctx.items) return -1;
    
    g_hair_scattering_ctx.count = 0;
    g_hair_scattering_ctx.initialized = true;
    
    return 0;
}

void shading_hair_scattering_shutdown(void) {
    if (!g_hair_scattering_ctx.initialized) return;
    
    free(g_hair_scattering_ctx.items);
    g_hair_scattering_ctx.items = NULL;
    g_hair_scattering_ctx.count = 0;
    g_hair_scattering_ctx.capacity = 0;
    g_hair_scattering_ctx.initialized = false;
}

int shading_hair_scattering_create(shading_hair_scattering_handle_t* out_handle, const shading_hair_scattering_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_hair_scattering_ctx.initialized) return -2;
    
    if (g_hair_scattering_ctx.count >= g_hair_scattering_ctx.capacity) {
        uint32_t new_capacity = g_hair_scattering_ctx.capacity * 2;
        if (new_capacity > SHADING_HAIR_SCATTERING_MAX_COUNT) new_capacity = SHADING_HAIR_SCATTERING_MAX_COUNT;
        
        if (new_capacity == g_hair_scattering_ctx.capacity) return -3;
        
        void* new_items = realloc(g_hair_scattering_ctx.items, new_capacity * sizeof(shading_hair_scattering_internal_t));
        if (!new_items) return -4;
        
        g_hair_scattering_ctx.items = new_items;
        g_hair_scattering_ctx.capacity = new_capacity;
    }
    
    uint32_t index = g_hair_scattering_ctx.count++;
    shading_hair_scattering_internal_t* item = &g_hair_scattering_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    
    // Default params
    item->params.scatter_color = vec3_set(0.8f, 0.6f, 0.4f);
    item->params.global_scatter_strength = 1.0f;
    item->params.density_scale = 1.0f;
    
    item->initialized = true;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void shading_hair_scattering_destroy(shading_hair_scattering_handle_t handle) {
    if (handle.id >= g_hair_scattering_ctx.count) return;
    g_hair_scattering_ctx.items[handle.id].initialized = false;
}

int shading_hair_scattering_update(shading_hair_scattering_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_hair_scattering_ctx.count) return -1;
    
    shading_hair_scattering_internal_t* item = &g_hair_scattering_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    if (size == sizeof(scattering_params_t)) {
        memcpy(&item->params, data, sizeof(scattering_params_t));
        item->dirty = true;
    }
    
    return 0;
}

bool shading_hair_scattering_is_valid(shading_hair_scattering_handle_t handle) {
    if (handle.id >= g_hair_scattering_ctx.count) return false;
    return g_hair_scattering_ctx.items[handle.id].initialized;
}

// Logic for Dual Scattering approximation
// Simulates global scattering in hair volume
void evaluate_hair_scattering(shading_hair_scattering_handle_t handle,
                             float NdotL, float NdotV, 
                             float depth, // depth or thickness of hair volume
                             vec3_t* out_scatter) {
    
    if (!g_hair_scattering_ctx.initialized || handle.id >= g_hair_scattering_ctx.count) return;
    shading_hair_scattering_internal_t* item = &g_hair_scattering_ctx.items[handle.id];

    // Dual scattering approximation:
    // Global scattering spread (f_spread) and local scattering (f_local)
    
    // Simple forward scattering through volume based on Beer's law
    float dist = depth * item->params.density_scale;
    float transmittance = expf(-dist);
    
    // Add forward scattering glow
    // Stronger when looking towards light through hair
    float forward_scatter = 0.0f;
    if (NdotL < 0.0f) { // Backlighting
        forward_scatter = powf(fabsf(NdotL), 2.0f) * 0.5f;
    }
    
    // Global scatter contribution (ambient-like)
    float global_term = transmittance * item->params.global_scatter_strength;
    
    if (out_scatter) {
        out_scatter->x = item->params.scatter_color.x * (global_term + forward_scatter);
        out_scatter->y = item->params.scatter_color.y * (global_term + forward_scatter);
        out_scatter->z = item->params.scatter_color.z * (global_term + forward_scatter);
    }
}

uint32_t shading_hair_scattering_get_count(void) {
    return g_hair_scattering_ctx.count;
}

size_t shading_hair_scattering_get_memory_usage(void) {
    size_t total = sizeof(g_hair_scattering_ctx);
    total += g_hair_scattering_ctx.capacity * sizeof(shading_hair_scattering_internal_t);
    return total;
}

void shading_hair_scattering_debug_print(void) {
    // Debug print
}
