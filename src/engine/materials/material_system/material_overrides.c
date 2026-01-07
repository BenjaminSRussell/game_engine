/*
 * material_overrides.c
 * Per-object material overrides
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "materials/material_system/material_overrides.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MATERIALS_MATERIAL_OVERRIDES_MAX_COUNT 2048
#define MATERIALS_MATERIAL_OVERRIDES_DEFAULT_CAPACITY 64

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_material_overrides_internal {
    uint32_t id;
    uint32_t flags;
    uint32_t parent_material_id;
    void* override_data;
    size_t override_data_size;
    bool initialized;
    bool dirty;
} materials_material_overrides_internal_t;

typedef struct materials_material_overrides_context {
    materials_material_overrides_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_material_overrides_context_t;

static materials_material_overrides_context_t g_material_overrides_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool is_valid_override(materials_material_overrides_handle_t handle) {
    if (!g_material_overrides_ctx.initialized) return false;
    if (handle.id >= g_material_overrides_ctx.capacity) return false;
    return g_material_overrides_ctx.items[handle.id].initialized;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_material_overrides_init(void) {
    if (g_material_overrides_ctx.initialized) return 0;

    g_material_overrides_ctx.capacity = MATERIALS_MATERIAL_OVERRIDES_DEFAULT_CAPACITY;
    g_material_overrides_ctx.items = calloc(g_material_overrides_ctx.capacity, sizeof(materials_material_overrides_internal_t));
    if (!g_material_overrides_ctx.items) return -1;

    g_material_overrides_ctx.count = 0;
    g_material_overrides_ctx.initialized = true;
    return 0;
}

void materials_material_overrides_shutdown(void) {
    if (!g_material_overrides_ctx.initialized) return;
    
    for(uint32_t i=0; i<g_material_overrides_ctx.capacity; i++) {
        if(g_material_overrides_ctx.items[i].override_data) {
            free(g_material_overrides_ctx.items[i].override_data);
        }
    }
    
    free(g_material_overrides_ctx.items);
    g_material_overrides_ctx.items = NULL;
    g_material_overrides_ctx.capacity = 0;
    g_material_overrides_ctx.initialized = false;
}

int materials_material_overrides_create(materials_material_overrides_handle_t* out_handle, const materials_material_overrides_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_material_overrides_ctx.initialized) return -2;

    if (g_material_overrides_ctx.count >= g_material_overrides_ctx.capacity) {
        uint32_t new_cap = g_material_overrides_ctx.capacity * 2;
        void* new_ptr = realloc(g_material_overrides_ctx.items, new_cap * sizeof(materials_material_overrides_internal_t));
        if (!new_ptr) return -3;
        
        memset((char*)new_ptr + g_material_overrides_ctx.capacity * sizeof(materials_material_overrides_internal_t), 
               0, 
               (new_cap - g_material_overrides_ctx.capacity) * sizeof(materials_material_overrides_internal_t));
        
        g_material_overrides_ctx.items = new_ptr;
        g_material_overrides_ctx.capacity = new_cap;
    }

    uint32_t index = 0;
    for (uint32_t i=0; i < g_material_overrides_ctx.capacity; i++) {
        if (!g_material_overrides_ctx.items[i].initialized) {
            index = i;
            if (index >= g_material_overrides_ctx.count) g_material_overrides_ctx.count = index + 1;
            break;
        }
    }

    materials_material_overrides_internal_t* item = &g_material_overrides_ctx.items[index];
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->override_data = NULL;
    item->override_data_size = 0;

    out_handle->id = index;
    return 0;
}

void materials_material_overrides_destroy(materials_material_overrides_handle_t handle) {
    if (is_valid_override(handle)) {
        materials_material_overrides_internal_t* item = &g_material_overrides_ctx.items[handle.id];
        if (item->override_data) {
            free(item->override_data);
            item->override_data = NULL;
        }
        item->initialized = false;
    }
}

int materials_material_overrides_update(materials_material_overrides_handle_t handle, const void* data, size_t size) {
    if (!is_valid_override(handle)) return -1;
    
    materials_material_overrides_internal_t* item = &g_material_overrides_ctx.items[handle.id];
    
    if (size > item->override_data_size) {
        void* new_data = realloc(item->override_data, size);
        if (!new_data) return -2;
        item->override_data = new_data;
        item->override_data_size = size;
    }
    
    if (data) {
        memcpy(item->override_data, data, size);
    }
    
    item->dirty = true;
    return 0;
}

bool materials_material_overrides_is_valid(materials_material_overrides_handle_t handle) {
    return is_valid_override(handle);
}

int materials_material_overrides_get_info(materials_material_overrides_handle_t handle, materials_material_overrides_info_t* out_info) {
    if (!out_info) return -1;
    if (!is_valid_override(handle)) return -2;
    
    materials_material_overrides_internal_t* item = &g_material_overrides_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    return 0;
}

void materials_material_overrides_mark_dirty(materials_material_overrides_handle_t handle) {
    if (is_valid_override(handle)) {
        g_material_overrides_ctx.items[handle.id].dirty = true;
    }
}

int materials_material_overrides_process_pending(void) {
    if (!g_material_overrides_ctx.initialized) return 0;
    int processed = 0;
    for(uint32_t i=0; i<g_material_overrides_ctx.capacity; i++) {
        if(g_material_overrides_ctx.items[i].initialized && g_material_overrides_ctx.items[i].dirty) {
            // Apply overrides?
            g_material_overrides_ctx.items[i].dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t materials_material_overrides_get_count(void) {
    uint32_t count = 0;
    if (g_material_overrides_ctx.initialized) {
        for(uint32_t i=0; i<g_material_overrides_ctx.capacity; i++) {
            if(g_material_overrides_ctx.items[i].initialized) count++;
        }
    }
    return count;
}

size_t materials_material_overrides_get_memory_usage(void) {
    if (!g_material_overrides_ctx.initialized) return 0;
    size_t size = g_material_overrides_ctx.capacity * sizeof(materials_material_overrides_internal_t);
    for(uint32_t i=0; i<g_material_overrides_ctx.capacity; i++) {
        if(g_material_overrides_ctx.items[i].initialized) {
            size += g_material_overrides_ctx.items[i].override_data_size;
        }
    }
    return size;
}

void materials_material_overrides_debug_print(void) {
    if(g_material_overrides_ctx.initialized) {
        printf("Material Overrides: %u active\n", materials_material_overrides_get_count());
    }
}
