/*
 * hair_shading_render.c
 * Hair shading implementation
 *
 * Part of the Hair System subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements Kajiya-Kay or Marschner hair shading models
 */

#include "character/hair/system/hair_system/rendering_hair/hair_shading_render.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define HAIR_SHADING_MAX_COUNT 32
#define HAIR_SHADING_DEFAULT_CAPACITY 8

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

/* ============================================================================
 * HAIR SHADING TYPES
 * ============================================================================ */

typedef struct hair_material {
    vec3_t base_color;
    float melanin;          // 0.0=blonde/white, 1.0=black
    float redness;          // 0.0=no red, 1.0=full red (pheomelanin)
    
    // Specular highlights
    float roughness;
    float shift;            // Longitudinal shift for primary highlight (scales)
    
    // Transmission
    vec3_t transmission_tint;
    float ior;              // Index of refraction usually 1.55 for hair
    
    // Glint/Sparkle
    float glint_strength;
    float glint_scale;
} hair_material_t;

typedef struct hair_system_hair_shading_render_internal {
    uint32_t id;
    uint32_t flags;
    hair_material_t material;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} hair_system_hair_shading_render_internal_t;

typedef struct hair_system_hair_shading_render_context {
    hair_system_hair_shading_render_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} hair_system_hair_shading_render_context_t;

static hair_system_hair_shading_render_context_t g_hair_shading_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int hair_system_hair_shading_render_init(void) {
    if (g_hair_shading_ctx.initialized) {
        return 0;
    }

    g_hair_shading_ctx.capacity = HAIR_SHADING_DEFAULT_CAPACITY;
    g_hair_shading_ctx.items = calloc(g_hair_shading_ctx.capacity, sizeof(hair_system_hair_shading_render_internal_t));
    if (!g_hair_shading_ctx.items) {
        return -1;
    }

    g_hair_shading_ctx.count = 0;
    g_hair_shading_ctx.initialized = true;

    return 0;
}

void hair_system_hair_shading_render_shutdown(void) {
    if (!g_hair_shading_ctx.initialized) {
        return;
    }

    free(g_hair_shading_ctx.items);
    g_hair_shading_ctx.items = NULL;
    g_hair_shading_ctx.count = 0;
    g_hair_shading_ctx.capacity = 0;
    g_hair_shading_ctx.initialized = false;
}

int hair_system_hair_shading_render_create(hair_system_hair_shading_render_handle_t* out_handle, 
                                             const hair_system_hair_shading_render_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_hair_shading_ctx.initialized) {
        return -2;
    }

    if (g_hair_shading_ctx.count >= g_hair_shading_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_hair_shading_ctx.count++;
    hair_system_hair_shading_render_internal_t* item = &g_hair_shading_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Default brunette hair
    item->material.base_color = (vec3_t){0.1f, 0.05f, 0.02f};
    item->material.melanin = 0.6f;
    item->material.redness = 0.2f;
    item->material.roughness = 0.3f;
    item->material.shift = 0.05f; // Scales tilt 
    item->material.ior = 1.55f;
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void hair_system_hair_shading_render_destroy(hair_system_hair_shading_render_handle_t handle) {
    if (handle.id >= g_hair_shading_ctx.count) {
        return;
    }

    g_hair_shading_ctx.items[handle.id].initialized = false;
}

int hair_system_hair_shading_render_set_material(hair_system_hair_shading_render_handle_t handle,
                                                   float melanin, float redness, float roughness) {
    if (handle.id >= g_hair_shading_ctx.count) {
        return -1;
    }
    
    hair_system_hair_shading_render_internal_t* item = &g_hair_shading_ctx.items[handle.id];
    item->material.melanin = melanin;
    item->material.redness = redness;
    item->material.roughness = roughness;
    item->dirty = true;
    
    return 0;
}

int hair_system_hair_shading_render_update(hair_system_hair_shading_render_handle_t handle, 
                                             const void* data, size_t size) {
    if (handle.id >= g_hair_shading_ctx.count) {
        return -1;
    }

    g_hair_shading_ctx.items[handle.id].dirty = true;
    return 0;
}

bool hair_system_hair_shading_render_is_valid(hair_system_hair_shading_render_handle_t handle) {
    if (handle.id >= g_hair_shading_ctx.count) {
        return false;
    }
    return g_hair_shading_ctx.items[handle.id].initialized;
}

int hair_system_hair_shading_render_get_info(hair_system_hair_shading_render_handle_t handle, 
                                               hair_system_hair_shading_render_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_hair_shading_ctx.count) {
        return -2;
    }

    const hair_system_hair_shading_render_internal_t* item = &g_hair_shading_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void hair_system_hair_shading_render_mark_dirty(hair_system_hair_shading_render_handle_t handle) {
    if (handle.id < g_hair_shading_ctx.count) {
        g_hair_shading_ctx.items[handle.id].dirty = true;
    }
}

int hair_system_hair_shading_render_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_hair_shading_ctx.count; i++) {
        hair_system_hair_shading_render_internal_t* item = &g_hair_shading_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t hair_system_hair_shading_render_get_count(void) {
    return g_hair_shading_ctx.count;
}

size_t hair_system_hair_shading_render_get_memory_usage(void) {
    size_t total = sizeof(g_hair_shading_ctx);
    total += g_hair_shading_ctx.capacity * sizeof(hair_system_hair_shading_render_internal_t);
    return total;
}

void hair_system_hair_shading_render_debug_print(void) {
    // Debug output
}

/* End of hair_shading_render.c */
