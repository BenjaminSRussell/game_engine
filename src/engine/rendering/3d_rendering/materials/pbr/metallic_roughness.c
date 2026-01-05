/*
 * metallic_roughness.c
 * Metallic-roughness workflow
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "metallic_roughness.h"
#include "pbr_parameters.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * WORKFLOW TYPES
 * ============================================================================ */

typedef struct materials_metallic_roughness_internal {
    uint32_t id;
    uint32_t flags;
    materials_pbr_parameters_handle_t params_handle;
    // Potentially add texture handles here (albedo_map, normal_map, etc.)
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} materials_metallic_roughness_internal_t;

typedef struct materials_metallic_roughness_context {
    materials_metallic_roughness_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_metallic_roughness_context_t;

static materials_metallic_roughness_context_t g_metallic_roughness_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_metallic_roughness_init(void) {
    if (g_metallic_roughness_ctx.initialized) {
        return 0;
    }

    g_metallic_roughness_ctx.capacity = 256;
    g_metallic_roughness_ctx.items = calloc(g_metallic_roughness_ctx.capacity, sizeof(materials_metallic_roughness_internal_t));
    if (!g_metallic_roughness_ctx.items) {
        return -1;
    }

    g_metallic_roughness_ctx.count = 0;
    g_metallic_roughness_ctx.initialized = true;

    return 0;
}

void materials_metallic_roughness_shutdown(void) {
    if (!g_metallic_roughness_ctx.initialized) {
        return;
    }

    free(g_metallic_roughness_ctx.items);
    g_metallic_roughness_ctx.items = NULL;
    g_metallic_roughness_ctx.count = 0;
    g_metallic_roughness_ctx.capacity = 0;
    g_metallic_roughness_ctx.initialized = false;
}

int materials_metallic_roughness_create(materials_metallic_roughness_handle_t* out_handle, const materials_metallic_roughness_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_metallic_roughness_ctx.initialized) {
        return -2;
    }

    if (g_metallic_roughness_ctx.count >= g_metallic_roughness_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_metallic_roughness_ctx.count++;
    materials_metallic_roughness_internal_t* item = &g_metallic_roughness_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Create underlying pbr params
    materials_pbr_parameters_desc_t params_desc = {0};
    materials_pbr_parameters_create(&item->params_handle, &params_desc);

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void materials_metallic_roughness_destroy(materials_metallic_roughness_handle_t handle) {
    if (handle.id >= g_metallic_roughness_ctx.count) {
        return;
    }
    
    materials_metallic_roughness_internal_t* item = &g_metallic_roughness_ctx.items[handle.id];
    materials_pbr_parameters_destroy(item->params_handle);
    item->initialized = false;
}

int materials_metallic_roughness_update(materials_metallic_roughness_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_metallic_roughness_ctx.count) {
        return -1;
    }

    materials_metallic_roughness_internal_t* item = &g_metallic_roughness_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // Forward update to pbr parameters
    return materials_pbr_parameters_update(item->params_handle, data, size);
}

bool materials_metallic_roughness_is_valid(materials_metallic_roughness_handle_t handle) {
    if (handle.id >= g_metallic_roughness_ctx.count) {
        return false;
    }
    return g_metallic_roughness_ctx.items[handle.id].initialized;
}

int materials_metallic_roughness_get_info(materials_metallic_roughness_handle_t handle, materials_metallic_roughness_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_metallic_roughness_ctx.count) {
        return -2;
    }

    const materials_metallic_roughness_internal_t* item = &g_metallic_roughness_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void materials_metallic_roughness_mark_dirty(materials_metallic_roughness_handle_t handle) {
    if (handle.id < g_metallic_roughness_ctx.count) {
        g_metallic_roughness_ctx.items[handle.id].dirty = true;
        materials_pbr_parameters_mark_dirty(g_metallic_roughness_ctx.items[handle.id].params_handle);
    }
}

int materials_metallic_roughness_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_metallic_roughness_ctx.count; i++) {
        materials_metallic_roughness_internal_t* item = &g_metallic_roughness_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    // Also process parameters
    processed += materials_pbr_parameters_process_pending();
    return processed;
}

uint32_t materials_metallic_roughness_get_count(void) {
    return g_metallic_roughness_ctx.count;
}

size_t materials_metallic_roughness_get_memory_usage(void) {
    return sizeof(g_metallic_roughness_ctx) + g_metallic_roughness_ctx.capacity * sizeof(materials_metallic_roughness_internal_t);
}

void materials_metallic_roughness_debug_print(void) {
    printf("Metallic-Roughness Stats:\n");
    printf("  Count: %u\n", g_metallic_roughness_ctx.count);
}
