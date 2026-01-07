/*
 * indirect_lighting.c
 * Indirect light accumulation
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <include/math/math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lighting/global_illumination/indirect_lighting.h"
#include "lighting/global_illumination/diffuse_gi.h"
#include "lighting/global_illumination/specular_gi.h"
#include "lighting/global_illumination/gi_probe_grid.h"
#include "lighting/lightmaps/lightmap_sampling.h"
#include "include/common.h"
#include "include/core/types.h"
#include "include/math/vec3.h"
#include "include/math/vec2.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LIGHTING_INDIRECT_LIGHTING_MAX_COUNT 4096
#define LIGHTING_INDIRECT_LIGHTING_DEFAULT_CAPACITY 256
#define LIGHTING_INDIRECT_LIGHTING_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_indirect_lighting_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} lighting_indirect_lighting_internal_t;

typedef struct lighting_indirect_lighting_context {
    lighting_indirect_lighting_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} lighting_indirect_lighting_context_t;

static lighting_indirect_lighting_context_t g_indirect_lighting_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool lighting_indirect_lighting_validate(const lighting_indirect_lighting_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void lighting_indirect_lighting_cleanup_internal(lighting_indirect_lighting_internal_t* item) {
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

int lighting_indirect_lighting_init(void) {
    if (g_indirect_lighting_ctx.initialized) {
        return 0; // Already initialized
    }

    g_indirect_lighting_ctx.capacity = LIGHTING_INDIRECT_LIGHTING_DEFAULT_CAPACITY;
    g_indirect_lighting_ctx.items = calloc(g_indirect_lighting_ctx.capacity, sizeof(lighting_indirect_lighting_internal_t));
    if (!g_indirect_lighting_ctx.items) {
        return -1;
    }

    g_indirect_lighting_ctx.count = 0;
    g_indirect_lighting_ctx.initialized = true;

    return 0;
}

void lighting_indirect_lighting_shutdown(void) {
    if (!g_indirect_lighting_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_indirect_lighting_ctx.count; i++) {
        lighting_indirect_lighting_cleanup_internal(&g_indirect_lighting_ctx.items[i]);
    }

    free(g_indirect_lighting_ctx.items);
    g_indirect_lighting_ctx.items = NULL;
    g_indirect_lighting_ctx.count = 0;
    g_indirect_lighting_ctx.capacity = 0;
    g_indirect_lighting_ctx.initialized = false;
}

int lighting_indirect_lighting_create(lighting_indirect_lighting_handle_t* out_handle, const lighting_indirect_lighting_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_indirect_lighting_ctx.initialized) {
        return -2;
    }

    if (g_indirect_lighting_ctx.count >= g_indirect_lighting_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_indirect_lighting_ctx.count++;
    lighting_indirect_lighting_internal_t* item = &g_indirect_lighting_ctx.items[index];

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

void lighting_indirect_lighting_destroy(lighting_indirect_lighting_handle_t handle) {
    if (handle.id >= g_indirect_lighting_ctx.count) {
        return;
    }

    lighting_indirect_lighting_cleanup_internal(&g_indirect_lighting_ctx.items[handle.id]);
}

Vec3 lighting_indirect_lighting_evaluate(Vec3 position, Vec3 normal, Vec3 view_dir, float roughness, bool is_static, Vec2 lightmap_uv, lighting_gi_probe_grid_handle_t probe_handle, const lightmap_texture_t* lightmap) {
    Vec3 diffuse = lighting_diffuse_gi_sample(position, normal, is_static, lightmap_uv, probe_handle, lightmap);
    Vec3 specular = lighting_specular_gi_sample(position, normal, view_dir, roughness);
    
    // Combine diffuse and specular
    return vec3_add(diffuse, specular);
}

int lighting_indirect_lighting_update_system(void) {
    return 0;
}

bool lighting_indirect_lighting_is_valid(lighting_indirect_lighting_handle_t handle) {
    if (handle.id >= g_indirect_lighting_ctx.count) {
        return false;
    }
    return g_indirect_lighting_ctx.items[handle.id].initialized;
}

int lighting_indirect_lighting_get_info(lighting_indirect_lighting_handle_t handle, lighting_indirect_lighting_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_indirect_lighting_ctx.count) {
        return -2;
    }

    const lighting_indirect_lighting_internal_t* item = &g_indirect_lighting_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void lighting_indirect_lighting_mark_dirty(lighting_indirect_lighting_handle_t handle) {
    if (handle.id < g_indirect_lighting_ctx.count) {
        g_indirect_lighting_ctx.items[handle.id].dirty = true;
    }
}

int lighting_indirect_lighting_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_indirect_lighting_ctx.count; i++) {
        lighting_indirect_lighting_internal_t* item = &g_indirect_lighting_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t lighting_indirect_lighting_get_count(void) {
    return g_indirect_lighting_ctx.count;
}

size_t lighting_indirect_lighting_get_memory_usage(void) {
    size_t total = sizeof(g_indirect_lighting_ctx);
    total += g_indirect_lighting_ctx.capacity * sizeof(lighting_indirect_lighting_internal_t);

    for (uint32_t i = 0; i < g_indirect_lighting_ctx.count; i++) {
        total += g_indirect_lighting_ctx.items[i].data_size;
    }

    return total;
}

void lighting_indirect_lighting_debug_print(void) {
}

/* End of indirect_lighting.c */
