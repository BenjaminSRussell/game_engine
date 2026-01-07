/*
 * diffuse_gi.c
 * Diffuse global illumination
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "diffuse_gi.h"
#include "gi_probe_grid.h"
#include "../lightmaps/lightmap_sampling.h"
#include "../../../../include/common.h"
#include "../../../../include/core/types.h"
#include "../../../../include/math/vec3.h"
#include "../../../../include/math/vec2.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LIGHTING_DIFFUSE_GI_MAX_COUNT 4096
#define LIGHTING_DIFFUSE_GI_DEFAULT_CAPACITY 256
#define LIGHTING_DIFFUSE_GI_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_diffuse_gi_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} lighting_diffuse_gi_internal_t;

typedef struct lighting_diffuse_gi_context {
    lighting_diffuse_gi_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} lighting_diffuse_gi_context_t;

static lighting_diffuse_gi_context_t g_diffuse_gi_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void lighting_diffuse_gi_cleanup_internal(lighting_diffuse_gi_internal_t* item) {
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

int lighting_diffuse_gi_init(void) {
    if (g_diffuse_gi_ctx.initialized) {
        return 0; // Already initialized
    }

    g_diffuse_gi_ctx.capacity = LIGHTING_DIFFUSE_GI_DEFAULT_CAPACITY;
    g_diffuse_gi_ctx.items = calloc(g_diffuse_gi_ctx.capacity, sizeof(lighting_diffuse_gi_internal_t));
    if (!g_diffuse_gi_ctx.items) {
        return -1;
    }

    g_diffuse_gi_ctx.count = 0;
    g_diffuse_gi_ctx.initialized = true;

    return 0;
}

void lighting_diffuse_gi_shutdown(void) {
    if (!g_diffuse_gi_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_diffuse_gi_ctx.count; i++) {
        lighting_diffuse_gi_cleanup_internal(&g_diffuse_gi_ctx.items[i]);
    }

    free(g_diffuse_gi_ctx.items);
    g_diffuse_gi_ctx.items = NULL;
    g_diffuse_gi_ctx.count = 0;
    g_diffuse_gi_ctx.capacity = 0;
    g_diffuse_gi_ctx.initialized = false;
}

int lighting_diffuse_gi_create(lighting_diffuse_gi_handle_t* out_handle, const lighting_diffuse_gi_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_diffuse_gi_ctx.initialized) {
        return -2;
    }

    if (g_diffuse_gi_ctx.count >= g_diffuse_gi_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_diffuse_gi_ctx.count++;
    lighting_diffuse_gi_internal_t* item = &g_diffuse_gi_ctx.items[index];

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

void lighting_diffuse_gi_destroy(lighting_diffuse_gi_handle_t handle) {
    if (handle.id >= g_diffuse_gi_ctx.count) {
        return;
    }

    lighting_diffuse_gi_cleanup_internal(&g_diffuse_gi_ctx.items[handle.id]);
}

Vec3 lighting_diffuse_gi_sample(Vec3 position, Vec3 normal, bool is_static, Vec2 lightmap_uv, lighting_gi_probe_grid_handle_t probe_handle, const lightmap_texture_t* lightmap) {
    if (is_static && lightmap) {
        return lighting_lightmap_sample_bilinear(lightmap, lightmap_uv);
    } else if (lighting_gi_probe_grid_is_valid(probe_handle)) {
        return lighting_gi_probe_grid_sample(probe_handle, position, normal);
    }
    
    return vec3_zero();
}

int lighting_diffuse_gi_evaluate_scene(void) {
    return 0;
}

bool lighting_diffuse_gi_is_valid(lighting_diffuse_gi_handle_t handle) {
    if (handle.id >= g_diffuse_gi_ctx.count) {
        return false;
    }
    return g_diffuse_gi_ctx.items[handle.id].initialized;
}

int lighting_diffuse_gi_get_info(lighting_diffuse_gi_handle_t handle, lighting_diffuse_gi_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_diffuse_gi_ctx.count) {
        return -2;
    }

    const lighting_diffuse_gi_internal_t* item = &g_diffuse_gi_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void lighting_diffuse_gi_mark_dirty(lighting_diffuse_gi_handle_t handle) {
    if (handle.id < g_diffuse_gi_ctx.count) {
        g_diffuse_gi_ctx.items[handle.id].dirty = true;
    }
}

int lighting_diffuse_gi_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_diffuse_gi_ctx.count; i++) {
        lighting_diffuse_gi_internal_t* item = &g_diffuse_gi_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t lighting_diffuse_gi_get_count(void) {
    return g_diffuse_gi_ctx.count;
}

size_t lighting_diffuse_gi_get_memory_usage(void) {
    size_t total = sizeof(g_diffuse_gi_ctx);
    total += g_diffuse_gi_ctx.capacity * sizeof(lighting_diffuse_gi_internal_t);

    for (uint32_t i = 0; i < g_diffuse_gi_ctx.count; i++) {
        total += g_diffuse_gi_ctx.items[i].data_size;
    }

    return total;
}

void lighting_diffuse_gi_debug_print(void) {
}

/* End of diffuse_gi.c */
