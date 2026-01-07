/*
 * pbr_parameters.c
 * PBR material parameters
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "pbr_parameters.h"
#include <math/vec3.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * PARAMETER TYPES
 * ============================================================================ */

typedef struct materials_pbr_data {
    vec3_t albedo;
    float roughness;
    float metallic;
    float ao;
    vec3_t emissive;
    float normal_scale;
} materials_pbr_data_t;

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MATERIALS_PBR_PARAMETERS_MAX_COUNT 4096
#define MATERIALS_PBR_PARAMETERS_DEFAULT_CAPACITY 256
#define MATERIALS_PBR_PARAMETERS_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_pbr_parameters_internal {
    uint32_t id;
    uint32_t flags;
    materials_pbr_data_t params;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} materials_pbr_parameters_internal_t;

typedef struct materials_pbr_parameters_context {
    materials_pbr_parameters_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_pbr_parameters_context_t;

static materials_pbr_parameters_context_t g_pbr_parameters_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_pbr_parameters_init(void) {
    if (g_pbr_parameters_ctx.initialized) {
        return 0;
    }

    g_pbr_parameters_ctx.capacity = MATERIALS_PBR_PARAMETERS_DEFAULT_CAPACITY;
    g_pbr_parameters_ctx.items = calloc(g_pbr_parameters_ctx.capacity, sizeof(materials_pbr_parameters_internal_t));
    if (!g_pbr_parameters_ctx.items) {
        return -1;
    }

    g_pbr_parameters_ctx.count = 0;
    g_pbr_parameters_ctx.initialized = true;

    return 0;
}

void materials_pbr_parameters_shutdown(void) {
    if (!g_pbr_parameters_ctx.initialized) {
        return;
    }

    free(g_pbr_parameters_ctx.items);
    g_pbr_parameters_ctx.items = NULL;
    g_pbr_parameters_ctx.count = 0;
    g_pbr_parameters_ctx.capacity = 0;
    g_pbr_parameters_ctx.initialized = false;
}

int materials_pbr_parameters_create(materials_pbr_parameters_handle_t* out_handle, const materials_pbr_parameters_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_pbr_parameters_ctx.initialized) {
        return -2;
    }

    if (g_pbr_parameters_ctx.count >= g_pbr_parameters_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_pbr_parameters_ctx.count++;
    materials_pbr_parameters_internal_t* item = &g_pbr_parameters_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Set defaults
    item->params.albedo = (vec3_t){1.0f, 1.0f, 1.0f};
    item->params.roughness = 0.5f;
    item->params.metallic = 0.0f;
    item->params.ao = 1.0f;
    item->params.emissive = (vec3_t){0.0f, 0.0f, 0.0f};
    item->params.normal_scale = 1.0f;

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void materials_pbr_parameters_destroy(materials_pbr_parameters_handle_t handle) {
    if (handle.id >= g_pbr_parameters_ctx.count) {
        return;
    }
    g_pbr_parameters_ctx.items[handle.id].initialized = false;
}

int materials_pbr_parameters_update(materials_pbr_parameters_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_pbr_parameters_ctx.count) {
        return -1;
    }

    materials_pbr_parameters_internal_t* item = &g_pbr_parameters_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    if (size != sizeof(materials_pbr_data_t)) {
        return -3;
    }

    memcpy(&item->params, data, size);
    item->dirty = true;
    return 0;
}

bool materials_pbr_parameters_is_valid(materials_pbr_parameters_handle_t handle) {
    if (handle.id >= g_pbr_parameters_ctx.count) {
        return false;
    }
    return g_pbr_parameters_ctx.items[handle.id].initialized;
}

int materials_pbr_parameters_get_info(materials_pbr_parameters_handle_t handle, materials_pbr_parameters_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_pbr_parameters_ctx.count) {
        return -2;
    }

    const materials_pbr_parameters_internal_t* item = &g_pbr_parameters_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void materials_pbr_parameters_mark_dirty(materials_pbr_parameters_handle_t handle) {
    if (handle.id < g_pbr_parameters_ctx.count) {
        g_pbr_parameters_ctx.items[handle.id].dirty = true;
    }
}

int materials_pbr_parameters_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_pbr_parameters_ctx.count; i++) {
        materials_pbr_parameters_internal_t* item = &g_pbr_parameters_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t materials_pbr_parameters_get_count(void) {
    return g_pbr_parameters_ctx.count;
}

size_t materials_pbr_parameters_get_memory_usage(void) {
    return sizeof(g_pbr_parameters_ctx) + g_pbr_parameters_ctx.capacity * sizeof(materials_pbr_parameters_internal_t);
}

void materials_pbr_parameters_debug_print(void) {
    printf("PBR Parameters Stats:\n");
    printf("  Count: %u\n", g_pbr_parameters_ctx.count);
}
