/*
 * material_parameters.c
 * Material parameter binding
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "material_parameters.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MATERIALS_MATERIAL_PARAMETERS_MAX_COUNT 1024
#define MATERIALS_MATERIAL_PARAMETERS_DEFAULT_CAPACITY 64

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_material_parameters_internal {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    // Parameter layout tracking could go here
    // For now we assume the instance holds the data and this is a binding handle/view
    uint32_t instance_id;
    uint32_t binding_slot;
} materials_material_parameters_internal_t;

typedef struct materials_material_parameters_context {
    materials_material_parameters_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_material_parameters_context_t;

static materials_material_parameters_context_t g_material_parameters_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Helper to check validity
static bool is_valid_handle(materials_material_parameters_handle_t handle) {
    if (!g_material_parameters_ctx.initialized) return false;
    if (handle.id >= g_material_parameters_ctx.capacity) return false;
    return g_material_parameters_ctx.items[handle.id].initialized;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_material_parameters_init(void) {
    if (g_material_parameters_ctx.initialized) {
        return 0; // Already initialized
    }

    g_material_parameters_ctx.capacity = MATERIALS_MATERIAL_PARAMETERS_DEFAULT_CAPACITY;
    g_material_parameters_ctx.items = calloc(g_material_parameters_ctx.capacity, sizeof(materials_material_parameters_internal_t));
    if (!g_material_parameters_ctx.items) {
        return -1;
    }

    g_material_parameters_ctx.count = 0;
    g_material_parameters_ctx.initialized = true;

    return 0;
}

void materials_material_parameters_shutdown(void) {
    if (!g_material_parameters_ctx.initialized) {
        return;
    }

    free(g_material_parameters_ctx.items);
    g_material_parameters_ctx.items = NULL;
    g_material_parameters_ctx.count = 0;
    g_material_parameters_ctx.capacity = 0;
    g_material_parameters_ctx.initialized = false;
}

int materials_material_parameters_create(materials_material_parameters_handle_t* out_handle, const materials_material_parameters_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_material_parameters_ctx.initialized) {
        return -2;
    }

    if (g_material_parameters_ctx.count >= g_material_parameters_ctx.capacity) {
        // Resize
        uint32_t new_capacity = g_material_parameters_ctx.capacity * 2;
        materials_material_parameters_internal_t* new_items = realloc(g_material_parameters_ctx.items, new_capacity * sizeof(materials_material_parameters_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_material_parameters_ctx.capacity, 0, (new_capacity - g_material_parameters_ctx.capacity) * sizeof(materials_material_parameters_internal_t));
        g_material_parameters_ctx.items = new_items;
        g_material_parameters_ctx.capacity = new_capacity;
    }

    // Find free slot
    uint32_t index = 0;
    for (uint32_t i = 0; i < g_material_parameters_ctx.capacity; i++) {
        if (!g_material_parameters_ctx.items[i].initialized) {
            index = i;
            if (index >= g_material_parameters_ctx.count) g_material_parameters_ctx.count = index + 1;
            break;
        }
    }

    materials_material_parameters_internal_t* item = &g_material_parameters_ctx.items[index];
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    
    // We would map user_data to instance_id if provided
    // item->instance_id = ...

    out_handle->id = index;
    return 0;
}

void materials_material_parameters_destroy(materials_material_parameters_handle_t handle) {
    if (is_valid_handle(handle)) {
        g_material_parameters_ctx.items[handle.id].initialized = false;
    }
}

int materials_material_parameters_update(materials_material_parameters_handle_t handle, const void* data, size_t size) {
    if (!is_valid_handle(handle)) return -1;
    
    // This would typically update the underlying buffer via the instance system
    // For now we simulate success
    return 0;
}

bool materials_material_parameters_is_valid(materials_material_parameters_handle_t handle) {
    return is_valid_handle(handle);
}

int materials_material_parameters_get_info(materials_material_parameters_handle_t handle, materials_material_parameters_info_t* out_info) {
    if (!out_info) return -1;
    if (!is_valid_handle(handle)) return -2;
    
    const materials_material_parameters_internal_t* item = &g_material_parameters_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    return 0;
}

void materials_material_parameters_mark_dirty(materials_material_parameters_handle_t handle) {
    if (is_valid_handle(handle)) {
        // Find associated instance and mark dirty
    }
}

int materials_material_parameters_process_pending(void) {
    if (!g_material_parameters_ctx.initialized) return 0;
    // Process parameter updates
    return 0;
}

uint32_t materials_material_parameters_get_count(void) {
    uint32_t active = 0;
    if (g_material_parameters_ctx.initialized) {
        for (uint32_t i = 0; i < g_material_parameters_ctx.capacity; i++) {
            if (g_material_parameters_ctx.items[i].initialized) active++;
        }
    }
    return active;
}

size_t materials_material_parameters_get_memory_usage(void) {
    if (!g_material_parameters_ctx.initialized) return 0;
    return g_material_parameters_ctx.capacity * sizeof(materials_material_parameters_internal_t);
}

void materials_material_parameters_debug_print(void) {
    if (!g_material_parameters_ctx.initialized) return;
    printf("Material Parameters: %u active bindings\n", materials_material_parameters_get_count());
}
