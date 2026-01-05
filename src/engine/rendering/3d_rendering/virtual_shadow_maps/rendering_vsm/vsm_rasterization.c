/*
 * vsm_rasterization.c
 * VSM shadow rasterization
 */

#include "vsm_rasterization.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef struct virtual_shadow_maps_vsm_rasterization_internal {
    uint32_t id;
    bool initialized;
} virtual_shadow_maps_vsm_rasterization_internal_t;

typedef struct virtual_shadow_maps_vsm_rasterization_context {
    virtual_shadow_maps_vsm_rasterization_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} virtual_shadow_maps_vsm_rasterization_context_t;

static virtual_shadow_maps_vsm_rasterization_context_t g_vsm_rasterization_ctx = {0};

int virtual_shadow_maps_vsm_rasterization_init(void) {
    if (g_vsm_rasterization_ctx.initialized) return 0;
    g_vsm_rasterization_ctx.capacity = 16;
    g_vsm_rasterization_ctx.items = calloc(g_vsm_rasterization_ctx.capacity, sizeof(virtual_shadow_maps_vsm_rasterization_internal_t));
    if (!g_vsm_rasterization_ctx.items) return -1;
    g_vsm_rasterization_ctx.initialized = true;
    return 0;
}

void virtual_shadow_maps_vsm_rasterization_shutdown(void) {
    if (!g_vsm_rasterization_ctx.initialized) return;
    free(g_vsm_rasterization_ctx.items);
    g_vsm_rasterization_ctx.items = NULL;
    g_vsm_rasterization_ctx.initialized = false;
}

int virtual_shadow_maps_vsm_rasterization_create(virtual_shadow_maps_vsm_rasterization_handle_t* out_handle, const virtual_shadow_maps_vsm_rasterization_desc_t* desc) {
    if (!out_handle || !g_vsm_rasterization_ctx.initialized) return -1;
    if (g_vsm_rasterization_ctx.count >= g_vsm_rasterization_ctx.capacity) return -2;
    uint32_t index = g_vsm_rasterization_ctx.count++;
    virtual_shadow_maps_vsm_rasterization_internal_t* rast = &g_vsm_rasterization_ctx.items[index];
    rast->id = index;
    rast->initialized = true;
    out_handle->id = index;
    return 0;
}

void virtual_shadow_maps_vsm_rasterization_destroy(virtual_shadow_maps_vsm_rasterization_handle_t handle) {
    if (handle.id >= g_vsm_rasterization_ctx.count) return;
    g_vsm_rasterization_ctx.items[handle.id].initialized = false;
}

void vsm_rasterize_page(virtual_shadow_maps_vsm_rasterization_handle_t handle, uint32_t p_index, uint32_t* objects, uint32_t object_count) {
    if (handle.id >= g_vsm_rasterization_ctx.count) return;
    
    // Set up viewport for physical page p_index
    // p_index = y * 128 + x
    uint32_t px = (p_index % 128) * 128;
    uint32_t py = (p_index / 128) * 128;
    
    // TODO: Call graphics API to set viewport (px, py, 128, 128)
    // TODO: Clear depth for this region
    
    // Render objects
    for (uint32_t i = 0; i < object_count; i++) {
        // TODO: Call graphics API to draw object objects[i]
    }
}

int virtual_shadow_maps_vsm_rasterization_update(virtual_shadow_maps_vsm_rasterization_handle_t handle, const void* data, size_t size) { return 0; }
bool virtual_shadow_maps_vsm_rasterization_is_valid(virtual_shadow_maps_vsm_rasterization_handle_t handle) {
    return handle.id < g_vsm_rasterization_ctx.count && g_vsm_rasterization_ctx.items[handle.id].initialized;
}
int virtual_shadow_maps_vsm_rasterization_get_info(virtual_shadow_maps_vsm_rasterization_handle_t handle, virtual_shadow_maps_vsm_rasterization_info_t* out_info) { return 0; }
void virtual_shadow_maps_vsm_rasterization_mark_dirty(virtual_shadow_maps_vsm_rasterization_handle_t handle) {}
int virtual_shadow_maps_vsm_rasterization_process_pending(void) { return 0; }
uint32_t virtual_shadow_maps_vsm_rasterization_get_count(void) { return g_vsm_rasterization_ctx.count; }
size_t virtual_shadow_maps_vsm_rasterization_get_memory_usage(void) { return g_vsm_rasterization_ctx.count * sizeof(virtual_shadow_maps_vsm_rasterization_internal_t); }
void virtual_shadow_maps_vsm_rasterization_debug_print(void) {}
