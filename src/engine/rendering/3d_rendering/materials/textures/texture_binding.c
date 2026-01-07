/*
 * texture_binding.c
 * Bind textures to material slots
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "texture_binding.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MATERIALS_TEXTURE_BINDING_MAX_SLOTS 16
#define MATERIALS_TEXTURE_BINDING_DEFAULT_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_texture_binding_internal {
    uint32_t id;
    uint32_t flags;
    uint32_t texture_ids[MATERIALS_TEXTURE_BINDING_MAX_SLOTS];
    uint32_t sampler_ids[MATERIALS_TEXTURE_BINDING_MAX_SLOTS];
    uint32_t active_slots_mask; // Bitmask of valid slots
    bool initialized;
    bool dirty;
} materials_texture_binding_internal_t;

typedef struct materials_texture_binding_context {
    materials_texture_binding_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_texture_binding_context_t;

static materials_texture_binding_context_t g_texture_binding_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool is_valid_binding(materials_texture_binding_handle_t handle) {
    return g_texture_binding_ctx.initialized && 
           handle.id < g_texture_binding_ctx.capacity && 
           g_texture_binding_ctx.items[handle.id].initialized;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_texture_binding_init(void) {
    if (g_texture_binding_ctx.initialized) return 0;
    
    g_texture_binding_ctx.capacity = MATERIALS_TEXTURE_BINDING_DEFAULT_CAPACITY;
    g_texture_binding_ctx.items = calloc(g_texture_binding_ctx.capacity, sizeof(materials_texture_binding_internal_t));
    if (!g_texture_binding_ctx.items) return -1;
    
    g_texture_binding_ctx.count = 0;
    g_texture_binding_ctx.initialized = true;
    return 0;
}

void materials_texture_binding_shutdown(void) {
    if (g_texture_binding_ctx.initialized) {
        free(g_texture_binding_ctx.items);
        g_texture_binding_ctx.items = NULL;
        g_texture_binding_ctx.initialized = false;
    }
}

int materials_texture_binding_create(materials_texture_binding_handle_t* out_handle, const materials_texture_binding_desc_t* desc) {
    if (!out_handle) return -1;
    if (!g_texture_binding_ctx.initialized) return -2;
    
    if (g_texture_binding_ctx.count >= g_texture_binding_ctx.capacity) {
        uint32_t new_cap = g_texture_binding_ctx.capacity * 2;
        void* ptr = realloc(g_texture_binding_ctx.items, new_cap * sizeof(materials_texture_binding_internal_t));
        if (!ptr) return -3;
        memset((char*)ptr + g_texture_binding_ctx.capacity * sizeof(materials_texture_binding_internal_t), 0, (new_cap - g_texture_binding_ctx.capacity) * sizeof(materials_texture_binding_internal_t));
        g_texture_binding_ctx.items = ptr;
        g_texture_binding_ctx.capacity = new_cap;
    }
    
    uint32_t index = 0;
    // Find free slot
    while(index < g_texture_binding_ctx.capacity && g_texture_binding_ctx.items[index].initialized) {
        index++;
    }
    if (index >= g_texture_binding_ctx.capacity) index = g_texture_binding_ctx.count; // fallback if realloc happened but loop failed? should satisfy invariant though
    
    if (index >= g_texture_binding_ctx.count) g_texture_binding_ctx.count = index + 1;
    
    materials_texture_binding_internal_t* item = &g_texture_binding_ctx.items[index];
    item->id = index;
    item->initialized = true;
    item->active_slots_mask = 0;
    item->dirty = true;
    
    out_handle->id = index;
    return 0;
}

void materials_texture_binding_destroy(materials_texture_binding_handle_t handle) {
    if (is_valid_binding(handle)) {
        g_texture_binding_ctx.items[handle.id].initialized = false;
    }
}

int materials_texture_binding_update(materials_texture_binding_handle_t handle, const void* data, size_t size) {
    // Generic update unused, specialized setters are better
    return 0;
}

void materials_texture_binding_set_texture(materials_texture_binding_handle_t handle, uint32_t slot, uint32_t texture_id) {
    if (!is_valid_binding(handle)) return;
    if (slot >= MATERIALS_TEXTURE_BINDING_MAX_SLOTS) return;
    
    materials_texture_binding_internal_t* item = &g_texture_binding_ctx.items[handle.id];
    item->texture_ids[slot] = texture_id;
    item->active_slots_mask |= (1 << slot);
    item->dirty = true;
}

bool materials_texture_binding_is_valid(materials_texture_binding_handle_t handle) {
    return is_valid_binding(handle);
}

int materials_texture_binding_get_info(materials_texture_binding_handle_t handle, materials_texture_binding_info_t* out_info) {
    if (out_info && is_valid_binding(handle)) {
        out_info->id = handle.id;
        out_info->flags = g_texture_binding_ctx.items[handle.id].flags;
        out_info->initialized = true;
        return 0;
    }
    return -1;
}

void materials_texture_binding_mark_dirty(materials_texture_binding_handle_t handle) {
    if (is_valid_binding(handle)) {
        g_texture_binding_ctx.items[handle.id].dirty = true;
    }
}

int materials_texture_binding_process_pending(void) {
    if (!g_texture_binding_ctx.initialized) return 0;
    
    int processed = 0;
    // Prepare bindless descriptors or descriptor sets here
    for (uint32_t i = 0; i < g_texture_binding_ctx.capacity; i++) {
        if (g_texture_binding_ctx.items[i].initialized && g_texture_binding_ctx.items[i].dirty) {
             g_texture_binding_ctx.items[i].dirty = false;
             processed++;
        }
    }
    return processed;
}

uint32_t materials_texture_binding_get_count(void) {
    uint32_t count = 0;
    if(g_texture_binding_ctx.initialized) {
        for(uint32_t i=0; i<g_texture_binding_ctx.capacity; i++) {
            if(g_texture_binding_ctx.items[i].initialized) count++;
        }
    }
    return count;
}

size_t materials_texture_binding_get_memory_usage(void) {
    return g_texture_binding_ctx.capacity * sizeof(materials_texture_binding_internal_t);
}

void materials_texture_binding_debug_print(void) {
    printf("Texture Binding: %u active bindings\n", materials_texture_binding_get_count());
}
