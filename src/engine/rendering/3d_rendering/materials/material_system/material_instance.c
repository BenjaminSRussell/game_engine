/*
 * material_instance.c
 * Material instance creation
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "material_instance.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MATERIALS_MATERIAL_INSTANCE_MAX_COUNT 4096
#define MATERIALS_MATERIAL_INSTANCE_DEFAULT_CAPACITY 256
#define MATERIALS_MATERIAL_INSTANCE_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_material_instance_internal {
    uint32_t id;
    uint32_t flags;
    void* data;              /* CPU-side parameter data */
    size_t data_size;
    uint32_t buffer_handle;  /* GPU uniform buffer handle (mock) */
    uint32_t template_id;    /* ID of the material template */
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    uint32_t ref_count;
} materials_material_instance_internal_t;

typedef struct materials_material_instance_context {
    materials_material_instance_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    size_t total_memory_usage;
} materials_material_instance_context_t;

static materials_material_instance_context_t g_material_instance_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool materials_material_instance_validate(const materials_material_instance_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    // Validate data size matches expected size for template (if we had template lookup)
    return true;
}

static void materials_material_instance_cleanup_internal(materials_material_instance_internal_t* item) {
    if (!item) return;
    
    // Free parameter data
    if (item->data) {
        free(item->data);
        item->data = NULL;
        if (g_material_instance_ctx.total_memory_usage >= item->data_size) {
            g_material_instance_ctx.total_memory_usage -= item->data_size;
        }
    }
    
    // Release GPU resources (mock)
    if (item->buffer_handle != 0) {
        // buffer_destroy(item->buffer_handle);
        item->buffer_handle = 0;
    }

    item->initialized = false;
    item->data_size = 0;
    item->ref_count = 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_material_instance_init(void) {
    if (g_material_instance_ctx.initialized) {
        return 0; // Already initialized
    }

    g_material_instance_ctx.capacity = MATERIALS_MATERIAL_INSTANCE_DEFAULT_CAPACITY;
    g_material_instance_ctx.items = calloc(g_material_instance_ctx.capacity, sizeof(materials_material_instance_internal_t));
    if (!g_material_instance_ctx.items) {
        return -1;
    }

    g_material_instance_ctx.count = 0;
    g_material_instance_ctx.total_memory_usage = g_material_instance_ctx.capacity * sizeof(materials_material_instance_internal_t);
    g_material_instance_ctx.initialized = true;

    return 0;
}

void materials_material_instance_shutdown(void) {
    if (!g_material_instance_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_material_instance_ctx.count; i++) {
        materials_material_instance_cleanup_internal(&g_material_instance_ctx.items[i]);
    }

    free(g_material_instance_ctx.items);
    g_material_instance_ctx.items = NULL;
    g_material_instance_ctx.count = 0;
    g_material_instance_ctx.capacity = 0;
    g_material_instance_ctx.total_memory_usage = 0;
    g_material_instance_ctx.initialized = false;
}

int materials_material_instance_create(materials_material_instance_handle_t* out_handle, const materials_material_instance_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_material_instance_ctx.initialized) {
        return -2;
    }

    // Resize if needed
    if (g_material_instance_ctx.count >= g_material_instance_ctx.capacity) {
        uint32_t new_capacity = g_material_instance_ctx.capacity * 2;
        materials_material_instance_internal_t* new_items = realloc(g_material_instance_ctx.items, new_capacity * sizeof(materials_material_instance_internal_t));
        if (!new_items) {
            return -3;
        }
        
        // Zero out new memory
        memset(new_items + g_material_instance_ctx.capacity, 0, (new_capacity - g_material_instance_ctx.capacity) * sizeof(materials_material_instance_internal_t));
        
        g_material_instance_ctx.items = new_items;
        g_material_instance_ctx.capacity = new_capacity;
        g_material_instance_ctx.total_memory_usage += (new_capacity - g_material_instance_ctx.capacity) * sizeof(materials_material_instance_internal_t);
    }

    // Find free slot
    uint32_t index = g_material_instance_ctx.count;
    
    // Check for recycled slot?
    for (uint32_t i = 0; i < g_material_instance_ctx.capacity; i++) {
        if (!g_material_instance_ctx.items[i].initialized) {
            index = i;
            if (index >= g_material_instance_ctx.count) g_material_instance_ctx.count = index + 1;
            break;
        }
    }

    materials_material_instance_internal_t* item = &g_material_instance_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->template_id = 0; // TODO: Pass template ID in desc or default
    item->data_size = 256; // Default parameter block size, should come from template
    item->data = calloc(1, item->data_size);
    if (!item->data) {
        return -4;
    }
    g_material_instance_ctx.total_memory_usage += item->data_size;
    
    // Create GPU buffer (mock)
    // item->buffer_handle = buffer_create(item->data_size, BUFFER_UNIFORM);
    item->buffer_handle = index + 1; // Fake handle

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->ref_count = 1;

    out_handle->id = index;
    return 0;
}

void materials_material_instance_destroy(materials_material_instance_handle_t handle) {
    if (handle.id >= g_material_instance_ctx.capacity) {
        return;
    }

    materials_material_instance_internal_t* item = &g_material_instance_ctx.items[handle.id];
    if (item->initialized) {
        if (item->ref_count > 0) {
            item->ref_count--;
        }
        
        if (item->ref_count == 0) {
            materials_material_instance_cleanup_internal(item);
        }
    }
}

int materials_material_instance_update(materials_material_instance_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_material_instance_ctx.capacity) {
        return -1;
    }

    materials_material_instance_internal_t* item = &g_material_instance_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    if (size > item->data_size) {
        // Reallocate if strictly necessary, but usually we warn on template mismatch
        void* new_data = realloc(item->data, size);
        if (!new_data) return -3;
        item->data = new_data;
        g_material_instance_ctx.total_memory_usage += (size - item->data_size);
        item->data_size = size;
        // Recreate buffer? 
        // buffer_resize(item->buffer_handle, size);
    }
    
    // Only copy if data provided
    if (data) {
        memcpy(item->data, data, size);
    }

    item->dirty = true;
    return 0;
}

bool materials_material_instance_is_valid(materials_material_instance_handle_t handle) {
    if (handle.id >= g_material_instance_ctx.capacity) {
        return false;
    }
    return g_material_instance_ctx.items[handle.id].initialized;
}

int materials_material_instance_get_info(materials_material_instance_handle_t handle, materials_material_instance_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_material_instance_ctx.capacity) {
        return -2;
    }

    const materials_material_instance_internal_t* item = &g_material_instance_ctx.items[handle.id];
    if (!item->initialized) {
        return -3;
    }

    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void materials_material_instance_mark_dirty(materials_material_instance_handle_t handle) {
    if (handle.id < g_material_instance_ctx.capacity) {
        if (g_material_instance_ctx.items[handle.id].initialized) {
            g_material_instance_ctx.items[handle.id].dirty = true;
        }
    }
}

int materials_material_instance_process_pending(void) {
    if (!g_material_instance_ctx.initialized) return 0;

    int processed = 0;
    // Determine current frame (mock)
    static uint64_t current_frame = 0;
    current_frame++;

    for (uint32_t i = 0; i < g_material_instance_ctx.capacity; i++) {
        materials_material_instance_internal_t* item = &g_material_instance_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Upload to GPU
            // buffer_update(item->buffer_handle, item->data, item->data_size);
            
            item->dirty = false;
            item->frame_updated = current_frame;
            processed++;
        }
    }

    return processed;
}

uint32_t materials_material_instance_get_count(void) {
    // Count actual active instances
    uint32_t active_count = 0;
    if (g_material_instance_ctx.initialized) {
        for (uint32_t i = 0; i < g_material_instance_ctx.capacity; i++) {
            if (g_material_instance_ctx.items[i].initialized) {
                active_count++;
            }
        }
    }
    return active_count;
}

size_t materials_material_instance_get_memory_usage(void) {
    return g_material_instance_ctx.total_memory_usage;
}

void materials_material_instance_debug_print(void) {
    if (!g_material_instance_ctx.initialized) {
        printf("Material System: Not initialized\n");
        return;
    }
    
    printf("Material System Status:\n");
    printf("  Capacity: %u\n", g_material_instance_ctx.capacity);
    printf("  Active Instances: %u\n", materials_material_instance_get_count());
    printf("  Memory Usage: %zu bytes\n", g_material_instance_ctx.total_memory_usage);
}
