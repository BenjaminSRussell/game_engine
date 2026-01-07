/*
 * material_layers.c
 * Layer-based material system
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "material_layers.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MATERIALS_MATERIAL_LAYERS_MAX_LAYERS 4
#define MATERIALS_MATERIAL_LAYERS_DEFAULT_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct {
    uint32_t material_id;
    uint32_t mask_texture_id;
    uint32_t blend_mode; 
    float opacity;
} material_layer_t;

typedef struct materials_material_layers_internal {
    uint32_t id;
    uint32_t flags;
    material_layer_t layers[MATERIALS_MATERIAL_LAYERS_MAX_LAYERS];
    uint32_t layer_count;
    bool initialized;
    bool dirty;
} materials_material_layers_internal_t;

typedef struct materials_material_layers_context {
    materials_material_layers_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_material_layers_context_t;

static materials_material_layers_context_t g_material_layers_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool is_valid_layer_handle(materials_material_layers_handle_t handle) {
    return g_material_layers_ctx.initialized &&
           handle.id < g_material_layers_ctx.capacity &&
           g_material_layers_ctx.items[handle.id].initialized;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_material_layers_init(void) {
    if (g_material_layers_ctx.initialized) return 0;
    
    g_material_layers_ctx.capacity = MATERIALS_MATERIAL_LAYERS_DEFAULT_CAPACITY;
    g_material_layers_ctx.items = calloc(g_material_layers_ctx.capacity, sizeof(materials_material_layers_internal_t));
    if (!g_material_layers_ctx.items) return -1;
    
    g_material_layers_ctx.count = 0;
    g_material_layers_ctx.initialized = true;
    return 0;
}

void materials_material_layers_shutdown(void) {
    if (g_material_layers_ctx.initialized) {
        free(g_material_layers_ctx.items);
        g_material_layers_ctx.items = NULL;
        g_material_layers_ctx.initialized = false;
    }
}

int materials_material_layers_create(materials_material_layers_handle_t* out_handle, const materials_material_layers_desc_t* desc) {
    if (!out_handle) return -1;
    if (!g_material_layers_ctx.initialized) return -2;
    
    if (g_material_layers_ctx.count >= g_material_layers_ctx.capacity) {
        uint32_t new_cap = g_material_layers_ctx.capacity * 2;
        void* ptr = realloc(g_material_layers_ctx.items, new_cap * sizeof(materials_material_layers_internal_t));
        if (!ptr) return -3;
        memset((char*)ptr + g_material_layers_ctx.capacity * sizeof(materials_material_layers_internal_t), 0, (new_cap - g_material_layers_ctx.capacity) * sizeof(materials_material_layers_internal_t));
        g_material_layers_ctx.items = ptr;
        g_material_layers_ctx.capacity = new_cap;
    }
    
    uint32_t index = g_material_layers_ctx.count++; // Simple append
    materials_material_layers_internal_t* item = &g_material_layers_ctx.items[index];
    item->id = index;
    item->initialized = true;
    item->layer_count = 0;
    
    out_handle->id = index;
    return 0;
}

void materials_material_layers_destroy(materials_material_layers_handle_t handle) {
    if (is_valid_layer_handle(handle)) {
        g_material_layers_ctx.items[handle.id].initialized = false;
    }
}

int materials_material_layers_update(materials_material_layers_handle_t handle, const void* data, size_t size) {
    return 0;
}

void materials_material_layers_add_layer(materials_material_layers_handle_t handle, uint32_t material_id, uint32_t mask_id, uint32_t blend_mode) {
    if (!is_valid_layer_handle(handle)) return;
    
    materials_material_layers_internal_t* item = &g_material_layers_ctx.items[handle.id];
    if (item->layer_count < MATERIALS_MATERIAL_LAYERS_MAX_LAYERS) {
        material_layer_t* layer = &item->layers[item->layer_count++];
        layer->material_id = material_id;
        layer->mask_texture_id = mask_id;
        layer->blend_mode = blend_mode;
        layer->opacity = 1.0f;
        item->dirty = true;
    }
}

bool materials_material_layers_is_valid(materials_material_layers_handle_t handle) {
    return is_valid_layer_handle(handle);
}

int materials_material_layers_get_info(materials_material_layers_handle_t handle, materials_material_layers_info_t* out_info) {
    if (out_info && is_valid_layer_handle(handle)) {
        out_info->id = handle.id;
        out_info->flags = 0;
        out_info->initialized = true;
        return 0;
    }
    return -1;
}

void materials_material_layers_mark_dirty(materials_material_layers_handle_t handle) {
    if (is_valid_layer_handle(handle)) {
        g_material_layers_ctx.items[handle.id].dirty = true;
    }
}

int materials_material_layers_process_pending(void) {
    return 0;
}

uint32_t materials_material_layers_get_count(void) {
    return g_material_layers_ctx.count;
}

size_t materials_material_layers_get_memory_usage(void) {
    return g_material_layers_ctx.capacity * sizeof(materials_material_layers_internal_t);
}

void materials_material_layers_debug_print(void) {
    printf("Material Layers: %u layered materials\n", g_material_layers_ctx.count);
}
