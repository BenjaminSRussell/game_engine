/*
 * vsm_sampling.c
 * VSM shadow sampling
 */

#include "lighting/vsm/virtual_shadow_maps/rendering_vsm/vsm_sampling.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef struct virtual_shadow_maps_vsm_sampling_internal {
    uint32_t id;
    bool initialized;
} virtual_shadow_maps_vsm_sampling_internal_t;

typedef struct virtual_shadow_maps_vsm_sampling_context {
    virtual_shadow_maps_vsm_sampling_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} virtual_shadow_maps_vsm_sampling_context_t;

static virtual_shadow_maps_vsm_sampling_context_t g_vsm_sampling_ctx = {0};

int virtual_shadow_maps_vsm_sampling_init(void) {
    if (g_vsm_sampling_ctx.initialized) return 0;
    g_vsm_sampling_ctx.capacity = 16;
    g_vsm_sampling_ctx.items = calloc(g_vsm_sampling_ctx.capacity, sizeof(virtual_shadow_maps_vsm_sampling_internal_t));
    if (!g_vsm_sampling_ctx.items) return -1;
    g_vsm_sampling_ctx.initialized = true;
    return 0;
}

void virtual_shadow_maps_vsm_sampling_shutdown(void) {
    if (!g_vsm_sampling_ctx.initialized) return;
    free(g_vsm_sampling_ctx.items);
    g_vsm_sampling_ctx.items = NULL;
    g_vsm_sampling_ctx.initialized = false;
}

int virtual_shadow_maps_vsm_sampling_create(virtual_shadow_maps_vsm_sampling_handle_t* out_handle, const virtual_shadow_maps_vsm_sampling_desc_t* desc) {
    if (!out_handle || !g_vsm_sampling_ctx.initialized) return -1;
    if (g_vsm_sampling_ctx.count >= g_vsm_sampling_ctx.capacity) return -2;
    uint32_t index = g_vsm_sampling_ctx.count++;
    virtual_shadow_maps_vsm_sampling_internal_t* samp = &g_vsm_sampling_ctx.items[index];
    samp->id = index;
    samp->initialized = true;
    out_handle->id = index;
    return 0;
}

void virtual_shadow_maps_vsm_sampling_destroy(virtual_shadow_maps_vsm_sampling_handle_t handle) {
    if (handle.id >= g_vsm_sampling_ctx.count) return;
    g_vsm_sampling_ctx.items[handle.id].initialized = false;
}

// Logic for sampling VSM on CPU (e.g. for collision or AI, or just as a reference)
float vsm_sample_depth(virtual_shadow_maps_vsm_sampling_handle_t handle, uint32_t v_x, uint32_t v_y) {
    // TODO: In practical implementation, this might read from back-end resource
    return 1.0f;
}

int virtual_shadow_maps_vsm_sampling_update(virtual_shadow_maps_vsm_sampling_handle_t handle, const void* data, size_t size) { return 0; }
bool virtual_shadow_maps_vsm_sampling_is_valid(virtual_shadow_maps_vsm_sampling_handle_t handle) {
    return handle.id < g_vsm_sampling_ctx.count && g_vsm_sampling_ctx.items[handle.id].initialized;
}
int virtual_shadow_maps_vsm_sampling_get_info(virtual_shadow_maps_vsm_sampling_handle_t handle, virtual_shadow_maps_vsm_sampling_info_t* out_info) { return 0; }
void virtual_shadow_maps_vsm_sampling_mark_dirty(virtual_shadow_maps_vsm_sampling_handle_t handle) {}
int virtual_shadow_maps_vsm_sampling_process_pending(void) { return 0; }
uint32_t virtual_shadow_maps_vsm_sampling_get_count(void) { return g_vsm_sampling_ctx.count; }
size_t virtual_shadow_maps_vsm_sampling_get_memory_usage(void) { return g_vsm_sampling_ctx.count * sizeof(virtual_shadow_maps_vsm_sampling_internal_t); }
void virtual_shadow_maps_vsm_sampling_debug_print(void) {}
