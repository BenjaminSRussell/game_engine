/*
 * material_lod.c
 * Simplify materials at distance
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "material_lod.h"
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

#define MATERIALS_MATERIAL_LOD_LEVEL_COUNT 4

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_material_lod_internal {
    uint32_t id;
    uint32_t flags;
    float distances[MATERIALS_MATERIAL_LOD_LEVEL_COUNT]; 
    uint32_t material_ids[MATERIALS_MATERIAL_LOD_LEVEL_COUNT];
    bool initialized;
} materials_material_lod_internal_t;

typedef struct materials_material_lod_context {
    materials_material_lod_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_material_lod_context_t;

static materials_material_lod_context_t g_material_lod_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool is_valid_lod(materials_material_lod_handle_t handle) {
    return g_material_lod_ctx.initialized && handle.id < g_material_lod_ctx.capacity && g_material_lod_ctx.items[handle.id].initialized;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_material_lod_init(void) {
    if (g_material_lod_ctx.initialized) return 0;
    
    g_material_lod_ctx.capacity = 256;
    g_material_lod_ctx.items = calloc(g_material_lod_ctx.capacity, sizeof(materials_material_lod_internal_t));
    if (!g_material_lod_ctx.items) return -1;
    
    g_material_lod_ctx.count = 0;
    g_material_lod_ctx.initialized = true;
    return 0;
}

void materials_material_lod_shutdown(void) {
    if (g_material_lod_ctx.initialized) {
        free(g_material_lod_ctx.items);
        g_material_lod_ctx.items = NULL;
        g_material_lod_ctx.initialized = false;
    }
}

int materials_material_lod_create(materials_material_lod_handle_t* out_handle, const materials_material_lod_desc_t* desc) {
    if (!out_handle) return -1;
    if (!g_material_lod_ctx.initialized) return -2;
    
    if (g_material_lod_ctx.count >= g_material_lod_ctx.capacity) {
        // Expand
        uint32_t new_cap = g_material_lod_ctx.capacity * 2;
        void* ptr = realloc(g_material_lod_ctx.items, new_cap * sizeof(materials_material_lod_internal_t));
        if (!ptr) return -3;
        memset((char*)ptr + g_material_lod_ctx.capacity * sizeof(materials_material_lod_internal_t), 0, (new_cap - g_material_lod_ctx.capacity) * sizeof(materials_material_lod_internal_t));
        g_material_lod_ctx.items = ptr;
        g_material_lod_ctx.capacity = new_cap;
    }
    
    uint32_t index = g_material_lod_ctx.count++; // Append logic for simplicity
    materials_material_lod_internal_t* item = &g_material_lod_ctx.items[index];
    item->id = index;
    item->initialized = true;
    
    // Default distances
    item->distances[0] = 10.0f;
    item->distances[1] = 50.0f;
    item->distances[2] = 100.0f;
    item->distances[3] = 500.0f;
    
    out_handle->id = index;
    return 0;
}

void materials_material_lod_destroy(materials_material_lod_handle_t handle) {
    if (is_valid_lod(handle)) {
        g_material_lod_ctx.items[handle.id].initialized = false;
    }
}

int materials_material_lod_update(materials_material_lod_handle_t handle, const void* data, size_t size) {
    if (!is_valid_lod(handle)) return -1;
    if (size == sizeof(float) * MATERIALS_MATERIAL_LOD_LEVEL_COUNT) {
        memcpy(g_material_lod_ctx.items[handle.id].distances, data, size);
        return 0;
    }
    return -2;
}

bool materials_material_lod_is_valid(materials_material_lod_handle_t handle) {
    return is_valid_lod(handle);
}

int materials_material_lod_get_info(materials_material_lod_handle_t handle, materials_material_lod_info_t* out_info) {
    if (out_info && is_valid_lod(handle)) {
        out_info->id = handle.id;
        out_info->flags = 0;
        out_info->initialized = true;
        return 0;
    }
    return -1;
}

void materials_material_lod_mark_dirty(materials_material_lod_handle_t handle) {
    // LOD usually doesn't need dirty marking unless GPU data needs update
}

int materials_material_lod_process_pending(void) {
    return 0;
}

uint32_t materials_material_lod_get_current_lod(materials_material_lod_handle_t handle, float distance) {
    if (!is_valid_lod(handle)) return 0;
    
    materials_material_lod_internal_t* item = &g_material_lod_ctx.items[handle.id];
    for (uint32_t i = 0; i < MATERIALS_MATERIAL_LOD_LEVEL_COUNT; i++) {
        if (distance < item->distances[i]) {
            return i;
        }
    }
    return MATERIALS_MATERIAL_LOD_LEVEL_COUNT - 1;
}

uint32_t materials_material_lod_get_count(void) {
    return g_material_lod_ctx.count;
}

size_t materials_material_lod_get_memory_usage(void) {
    return g_material_lod_ctx.capacity * sizeof(materials_material_lod_internal_t);
}

void materials_material_lod_debug_print(void) {
    printf("Material LOD: %u objects managed\n", g_material_lod_ctx.count);
}
