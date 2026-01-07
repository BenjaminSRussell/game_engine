/*
 * vsm_clipmap.c
 * VSM clipmap management
 */

#include "vsm_clipmap.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CLIPMAP_LEVELS 8

typedef struct vsm_clipmap_level {
    float range_min;
    float range_max;
    uint32_t table_handle; // Store ID for simplicity
} vsm_clipmap_level_t;

typedef struct virtual_shadow_maps_vsm_clipmap_internal {
    uint32_t id;
    vsm_clipmap_level_t levels[MAX_CLIPMAP_LEVELS];
    uint32_t level_count;
    bool initialized;
} virtual_shadow_maps_vsm_clipmap_internal_t;

typedef struct virtual_shadow_maps_vsm_clipmap_context {
    virtual_shadow_maps_vsm_clipmap_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} virtual_shadow_maps_vsm_clipmap_context_t;

static virtual_shadow_maps_vsm_clipmap_context_t g_vsm_clipmap_ctx = {0};

int virtual_shadow_maps_vsm_clipmap_init(void) {
    if (g_vsm_clipmap_ctx.initialized) return 0;
    g_vsm_clipmap_ctx.capacity = 16;
    g_vsm_clipmap_ctx.items = calloc(g_vsm_clipmap_ctx.capacity, sizeof(virtual_shadow_maps_vsm_clipmap_internal_t));
    if (!g_vsm_clipmap_ctx.items) return -1;
    g_vsm_clipmap_ctx.initialized = true;
    return 0;
}

void virtual_shadow_maps_vsm_clipmap_shutdown(void) {
    if (!g_vsm_clipmap_ctx.initialized) return;
    free(g_vsm_clipmap_ctx.items);
    g_vsm_clipmap_ctx.items = NULL;
    g_vsm_clipmap_ctx.initialized = false;
}

int virtual_shadow_maps_vsm_clipmap_create(virtual_shadow_maps_vsm_clipmap_handle_t* out_handle, const virtual_shadow_maps_vsm_clipmap_desc_t* desc) {
    if (!out_handle || !g_vsm_clipmap_ctx.initialized) return -1;
    if (g_vsm_clipmap_ctx.count >= g_vsm_clipmap_ctx.capacity) return -2;
    uint32_t index = g_vsm_clipmap_ctx.count++;
    virtual_shadow_maps_vsm_clipmap_internal_t* clip = &g_vsm_clipmap_ctx.items[index];
    clip->id = index;
    clip->level_count = 4;
    for (uint32_t i = 0; i < clip->level_count; i++) {
        clip->levels[i].range_min = (float)i * 10.0f;
        clip->levels[i].range_max = (float)(i + 1) * 10.0f;
    }
    clip->initialized = true;
    out_handle->id = index;
    return 0;
}

void virtual_shadow_maps_vsm_clipmap_destroy(virtual_shadow_maps_vsm_clipmap_handle_t handle) {
    if (handle.id >= g_vsm_clipmap_ctx.count) return;
    g_vsm_clipmap_ctx.items[handle.id].initialized = false;
}

uint32_t vsm_clipmap_get_level_for_distance(virtual_shadow_maps_vsm_clipmap_handle_t handle, float distance) {
    if (handle.id >= g_vsm_clipmap_ctx.count) return 0;
    virtual_shadow_maps_vsm_clipmap_internal_t* clip = &g_vsm_clipmap_ctx.items[handle.id];
    for (uint32_t i = 0; i < clip->level_count; i++) {
        if (distance >= clip->levels[i].range_min && distance < clip->levels[i].range_max) {
            return i;
        }
    }
    return clip->level_count - 1;
}

int virtual_shadow_maps_vsm_clipmap_update(virtual_shadow_maps_vsm_clipmap_handle_t handle, const void* data, size_t size) { return 0; }
bool virtual_shadow_maps_vsm_clipmap_is_valid(virtual_shadow_maps_vsm_clipmap_handle_t handle) {
    return handle.id < g_vsm_clipmap_ctx.count && g_vsm_clipmap_ctx.items[handle.id].initialized;
}
int virtual_shadow_maps_vsm_clipmap_get_info(virtual_shadow_maps_vsm_clipmap_handle_t handle, virtual_shadow_maps_vsm_clipmap_info_t* out_info) { return 0; }
void virtual_shadow_maps_vsm_clipmap_mark_dirty(virtual_shadow_maps_vsm_clipmap_handle_t handle) {}
int virtual_shadow_maps_vsm_clipmap_process_pending(void) { return 0; }
uint32_t virtual_shadow_maps_vsm_clipmap_get_count(void) { return g_vsm_clipmap_ctx.count; }
size_t virtual_shadow_maps_vsm_clipmap_get_memory_usage(void) { return g_vsm_clipmap_ctx.count * sizeof(virtual_shadow_maps_vsm_clipmap_internal_t); }
void virtual_shadow_maps_vsm_clipmap_debug_print(void) {}
