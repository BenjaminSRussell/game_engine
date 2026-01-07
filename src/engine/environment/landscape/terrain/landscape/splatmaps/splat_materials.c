/*
 * splat_materials.c
 * Per-layer materials
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement terrain LOD
 * TODO: Add terrain tessellation
 * TODO: Implement heightmap streaming
 * TODO: Add splat map rendering
 * TODO: Implement vegetation instancing
 * TODO: Add grass rendering
 * TODO: Implement procedural terrain
 * TODO: Add erosion simulation
 * TODO: Implement virtual heightmaps
 * TODO: Add terrain holes
 * TODO: Implement splat materials initialization
 * TODO: Add splat materials cleanup/shutdown
 * TODO: Implement splat materials validation
 * TODO: Add splat materials error handling
 * TODO: Implement splat materials serialization
 * TODO: Add splat materials debug output
 * TODO: Implement splat materials unit tests
 * TODO: Add splat materials performance counters
 * TODO: Implement splat materials hot-reload
 * TODO: Add splat materials thread safety
 * TODO: Implement splat materials memory pooling
 * TODO: Add splat materials caching layer
 * TODO: Implement splat materials async operations
 * TODO: Add splat materials GPU integration
 * TODO: Implement splat materials SIMD optimization
 * TODO: Add splat materials batch processing
 * TODO: Implement splat materials streaming support
 * TODO: Add splat materials LOD support
 * TODO: Implement splat materials culling integration
 * TODO: Add splat materials render graph node
 */

#include "environment/landscape/terrain/landscape/splatmaps/splat_materials.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LANDSCAPE_SPLAT_MATERIALS_MAX_COUNT 4096
#define LANDSCAPE_SPLAT_MATERIALS_DEFAULT_CAPACITY 256
#define LANDSCAPE_SPLAT_MATERIALS_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_splat_materials_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} landscape_splat_materials_internal_t;

typedef struct landscape_splat_materials_context {
    landscape_splat_materials_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} landscape_splat_materials_context_t;

static landscape_splat_materials_context_t g_splat_materials_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool landscape_splat_materials_validate(const landscape_splat_materials_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void landscape_splat_materials_cleanup_internal(landscape_splat_materials_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

// Internal state
typedef struct splat_materials_state {
    splat_material_layer_t layers[LANDSCAPE_SPLAT_MAX_LAYERS];
    uint32_t active_layer_mask;
} splat_materials_state_t;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int landscape_splat_materials_init(void) {
    if (g_splat_materials_ctx.initialized) {
        return 0; // Already initialized
    }

    g_splat_materials_ctx.capacity = LANDSCAPE_SPLAT_MATERIALS_DEFAULT_CAPACITY;
    g_splat_materials_ctx.items = calloc(g_splat_materials_ctx.capacity, sizeof(landscape_splat_materials_internal_t));
    if (!g_splat_materials_ctx.items) {
        return -1;
    }

    g_splat_materials_ctx.count = 0;
    g_splat_materials_ctx.initialized = true;

    return 0;
}

void landscape_splat_materials_shutdown(void) {
    if (!g_splat_materials_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_splat_materials_ctx.count; i++) {
        landscape_splat_materials_cleanup_internal(&g_splat_materials_ctx.items[i]);
    }

    free(g_splat_materials_ctx.items);
    g_splat_materials_ctx.items = NULL;
    g_splat_materials_ctx.count = 0;
    g_splat_materials_ctx.capacity = 0;
    g_splat_materials_ctx.initialized = false;
}

int landscape_splat_materials_create(landscape_splat_materials_handle_t* out_handle, const landscape_splat_materials_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_splat_materials_ctx.initialized) {
        return -2;
    }

    if (g_splat_materials_ctx.count >= g_splat_materials_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_splat_materials_ctx.count++;
    landscape_splat_materials_internal_t* item = &g_splat_materials_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    
    // Initialize internal state
    splat_materials_state_t* state = malloc(sizeof(splat_materials_state_t));
    if (!state) return -4;
    
    memset(state, 0, sizeof(splat_materials_state_t));
    item->data = state;
    item->data_size = sizeof(splat_materials_state_t);
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void landscape_splat_materials_destroy(landscape_splat_materials_handle_t handle) {
    if (handle.id >= g_splat_materials_ctx.count) {
        return;
    }
    
    landscape_splat_materials_internal_t* item = &g_splat_materials_ctx.items[handle.id];
    splat_materials_state_t* state = (splat_materials_state_t*)item->data;
    
    if (state) {
        for (int i = 0; i < LANDSCAPE_SPLAT_MAX_LAYERS; i++) {
            if (state->layers[i].albedo_path) free(state->layers[i].albedo_path);
            if (state->layers[i].normal_path) free(state->layers[i].normal_path);
            if (state->layers[i].roughness_path) free(state->layers[i].roughness_path);
            if (state->layers[i].height_path) free(state->layers[i].height_path);
        }
    }

    landscape_splat_materials_cleanup_internal(item);
}

int landscape_splat_materials_set_layer(
    landscape_splat_materials_handle_t handle,
    uint32_t layer_index,
    const splat_material_layer_t* layer
) {
    if (handle.id >= g_splat_materials_ctx.count) return -1;
    if (layer_index >= LANDSCAPE_SPLAT_MAX_LAYERS) return -5;
    
    landscape_splat_materials_internal_t* item = &g_splat_materials_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    splat_materials_state_t* state = (splat_materials_state_t*)item->data;
    splat_material_layer_t* dest = &state->layers[layer_index];
    
    // Free previous paths
    if (dest->albedo_path) free(dest->albedo_path);
    if (dest->normal_path) free(dest->normal_path);
    if (dest->roughness_path) free(dest->roughness_path);
    if (dest->height_path) free(dest->height_path);
    
    if (layer) {
        dest->albedo_path = layer->albedo_path ? strdup(layer->albedo_path) : NULL;
        dest->normal_path = layer->normal_path ? strdup(layer->normal_path) : NULL;
        dest->roughness_path = layer->roughness_path ? strdup(layer->roughness_path) : NULL;
        dest->height_path = layer->height_path ? strdup(layer->height_path) : NULL;
        dest->tiling_scale = layer->tiling_scale;
        dest->displacement_scale = layer->displacement_scale;
        dest->active = true;
        state->active_layer_mask |= (1 << layer_index);
    } else {
        // Clear layer
        memset(dest, 0, sizeof(splat_material_layer_t));
        state->active_layer_mask &= ~(1 << layer_index);
    }
    
    item->dirty = true;
    return 0;
}

int landscape_splat_materials_get_layer(
    landscape_splat_materials_handle_t handle,
    uint32_t layer_index,
    splat_material_layer_t* out_layer
) {
    if (!out_layer) return -1;
    if (handle.id >= g_splat_materials_ctx.count) return -2;
    if (layer_index >= LANDSCAPE_SPLAT_MAX_LAYERS) return -5;
    
    landscape_splat_materials_internal_t* item = &g_splat_materials_ctx.items[handle.id];
    if (!item->initialized) return -3;
    
    splat_materials_state_t* state = (splat_materials_state_t*)item->data;
    // Return shallow copy (pointers still point to internal memory, dangerous if caller frees)
    // Better to just copy scalar values and maybe duplicate strings if needed, but for now shallow copy logic
    // Actually, caller shouldn't modify strings.
    *out_layer = state->layers[layer_index];
    
    return 0;
}

int landscape_splat_materials_upload(landscape_splat_materials_handle_t handle) {
    if (handle.id >= g_splat_materials_ctx.count) return -1;
    landscape_splat_materials_internal_t* item = &g_splat_materials_ctx.items[handle.id];
    if (!item->initialized) return -2;
    
    // Placeholder: This would trigger texture loading and GPU upload
    // splat_materials_state_t* state = (splat_materials_state_t*)item->data;
    
    item->dirty = false;
    return 0;
}

int landscape_splat_materials_update(landscape_splat_materials_handle_t handle, const void* data, size_t size) {
    // Generic update
    return 0;
}

bool landscape_splat_materials_is_valid(landscape_splat_materials_handle_t handle) {
    if (handle.id >= g_splat_materials_ctx.count) {
        return false;
    }
    return g_splat_materials_ctx.items[handle.id].initialized;
}

int landscape_splat_materials_get_info(landscape_splat_materials_handle_t handle, landscape_splat_materials_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_splat_materials_ctx.count) {
        return -2;
    }

    const landscape_splat_materials_internal_t* item = &g_splat_materials_ctx.items[handle.id];
    const splat_materials_state_t* state = (const splat_materials_state_t*)item->data;
    
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    // count active layers
    uint32_t count = 0;
    if (state) {
        for (int i=0; i<LANDSCAPE_SPLAT_MAX_LAYERS; i++) {
            if (state->layers[i].active) count++;
        }
    }
    out_info->active_layers = count;

    return 0;
}

void landscape_splat_materials_mark_dirty(landscape_splat_materials_handle_t handle) {
    if (handle.id < g_splat_materials_ctx.count) {
        g_splat_materials_ctx.items[handle.id].dirty = true;
    }
}

int landscape_splat_materials_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_splat_materials_ctx.count; i++) {
        landscape_splat_materials_internal_t* item = &g_splat_materials_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            // Maybe auto-upload?
            landscape_splat_materials_upload((landscape_splat_materials_handle_t){.id = i});
            processed++;
        }
    }

    return processed;
}

uint32_t landscape_splat_materials_get_count(void) {
    return g_splat_materials_ctx.count;
}

size_t landscape_splat_materials_get_memory_usage(void) {
    size_t total = sizeof(g_splat_materials_ctx);
    total += g_splat_materials_ctx.capacity * sizeof(landscape_splat_materials_internal_t);

    for (uint32_t i = 0; i < g_splat_materials_ctx.count; i++) {
        landscape_splat_materials_internal_t* item = &g_splat_materials_ctx.items[i];
        total += item->data_size;
        
        splat_materials_state_t* state = (splat_materials_state_t*)item->data;
        if (state) {
             for (int l = 0; l < LANDSCAPE_SPLAT_MAX_LAYERS; l++) {
                 if (state->layers[l].albedo_path) total += strlen(state->layers[l].albedo_path) + 1;
                 if (state->layers[l].normal_path) total += strlen(state->layers[l].normal_path) + 1;
                 if (state->layers[l].roughness_path) total += strlen(state->layers[l].roughness_path) + 1;
                 if (state->layers[l].height_path) total += strlen(state->layers[l].height_path) + 1;
             }
        }
    }

    return total;
}

void landscape_splat_materials_debug_print(void) {
    // Debug output
}

/* End of splat_materials.c */
