/*
 * material_sorting.c
 * Sort materials for batching
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "material_sorting.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MATERIALS_MATERIAL_SORTING_DEFAULT_CAPACITY 1024

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct {
    uint64_t key;
    uint32_t material_id;
    uint32_t distance_packed; // for depth sorting
} material_sort_entry_t;

typedef struct materials_material_sorting_context {
    material_sort_entry_t* sort_buffer;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_material_sorting_context_t;

static materials_material_sorting_context_t g_material_sorting_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Simple radix sort or qsort wrapper
static int compare_sort_entries(const void* a, const void* b) {
    const material_sort_entry_t* sa = a;
    const material_sort_entry_t* sb = b;
    if (sa->key < sb->key) return -1;
    if (sa->key > sb->key) return 1;
    return 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_material_sorting_init(void) {
    if (g_material_sorting_ctx.initialized) return 0;
    
    g_material_sorting_ctx.capacity = MATERIALS_MATERIAL_SORTING_DEFAULT_CAPACITY;
    g_material_sorting_ctx.sort_buffer = malloc(g_material_sorting_ctx.capacity * sizeof(material_sort_entry_t));
    if (!g_material_sorting_ctx.sort_buffer) return -1;
    
    g_material_sorting_ctx.count = 0;
    g_material_sorting_ctx.initialized = true;
    return 0;
}

void materials_material_sorting_shutdown(void) {
    if (g_material_sorting_ctx.initialized) {
        free(g_material_sorting_ctx.sort_buffer);
        g_material_sorting_ctx.sort_buffer = NULL;
        g_material_sorting_ctx.initialized = false;
    }
}

int materials_material_sorting_create(materials_material_sorting_handle_t* out_handle, const materials_material_sorting_desc_t* desc) {
    if (!out_handle) return -1;
    // For sorting, the "handle" might just represent a sort bucket or group
    out_handle->id = 0; // Stub
    return 0;
}

void materials_material_sorting_destroy(materials_material_sorting_handle_t handle) {
    // No-op for now
}

int materials_material_sorting_update(materials_material_sorting_handle_t handle, const void* data, size_t size) {
    // Update sort keys?
    return 0;
}

bool materials_material_sorting_is_valid(materials_material_sorting_handle_t handle) {
    return true;
}

int materials_material_sorting_get_info(materials_material_sorting_handle_t handle, materials_material_sorting_info_t* out_info) {
    if (out_info) {
        out_info->id = handle.id;
        out_info->flags = 0;
        out_info->initialized = true;
    }
    return 0;
}

void materials_material_sorting_mark_dirty(materials_material_sorting_handle_t handle) {
    // Trigger re-sort next frame
}

int materials_material_sorting_process_pending(void) {
    // Perform sort here
    if (g_material_sorting_ctx.count > 0) {
        qsort(g_material_sorting_ctx.sort_buffer, g_material_sorting_ctx.count, sizeof(material_sort_entry_t), compare_sort_entries);
    }
    return g_material_sorting_ctx.count;
}

uint32_t materials_material_sorting_get_count(void) {
    return g_material_sorting_ctx.count;
}

size_t materials_material_sorting_get_memory_usage(void) {
    return g_material_sorting_ctx.capacity * sizeof(material_sort_entry_t);
}

void materials_material_sorting_debug_print(void) {
    printf("Material Sorting: %u items sorted\n", g_material_sorting_ctx.count);
}

/* Helper to generate sort key */
uint64_t materials_material_sorting_generate_key(uint32_t shader_id, uint32_t texture_id, uint8_t depth_layer) {
    // Key format: [Depth: 8] [Shader: 24] [Texture: 32]
    uint64_t key = 0;
    key |= ((uint64_t)depth_layer) << 56;
    key |= ((uint64_t)shader_id & 0xFFFFFF) << 32;
    key |= ((uint64_t)texture_id & 0xFFFFFFFF);
    return key;
}

void materials_material_sorting_add_entry(uint32_t material_id, uint64_t sort_key) {
    if (g_material_sorting_ctx.count >= g_material_sorting_ctx.capacity) {
        uint32_t new_cap = g_material_sorting_ctx.capacity * 2;
        void* new_buf = realloc(g_material_sorting_ctx.sort_buffer, new_cap * sizeof(material_sort_entry_t));
        if (new_buf) {
            g_material_sorting_ctx.sort_buffer = new_buf;
            g_material_sorting_ctx.capacity = new_cap;
        } else {
            return; 
        }
    }
    g_material_sorting_ctx.sort_buffer[g_material_sorting_ctx.count].material_id = material_id;
    g_material_sorting_ctx.sort_buffer[g_material_sorting_ctx.count].key = sort_key;
    g_material_sorting_ctx.count++;
}

void materials_material_sorting_clear(void) {
    g_material_sorting_ctx.count = 0;
}
