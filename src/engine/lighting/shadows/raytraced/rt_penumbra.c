/*
 * rt_penumbra.c
 * RT soft penumbra calculation
 */

#include "lighting/shadows/raytraced/rt_penumbra.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef struct shadows_advanced_rt_penumbra_internal {
    uint32_t id;
    bool initialized;
} shadows_advanced_rt_penumbra_internal_t;

typedef struct shadows_advanced_rt_penumbra_context {
    shadows_advanced_rt_penumbra_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} shadows_advanced_rt_penumbra_context_t;

static shadows_advanced_rt_penumbra_context_t g_rt_penumbra_ctx = {0};

int shadows_advanced_rt_penumbra_init(void) {
    if (g_rt_penumbra_ctx.initialized) return 0;
    g_rt_penumbra_ctx.capacity = 16;
    g_rt_penumbra_ctx.items = calloc(g_rt_penumbra_ctx.capacity, sizeof(shadows_advanced_rt_penumbra_internal_t));
    if (!g_rt_penumbra_ctx.items) return -1;
    g_rt_penumbra_ctx.initialized = true;
    return 0;
}

void shadows_advanced_rt_penumbra_shutdown(void) {
    if (!g_rt_penumbra_ctx.initialized) return;
    free(g_rt_penumbra_ctx.items);
    g_rt_penumbra_ctx.items = NULL;
    g_rt_penumbra_ctx.initialized = false;
}

int shadows_advanced_rt_penumbra_create(shadows_advanced_rt_penumbra_handle_t* out_handle, const shadows_advanced_rt_penumbra_desc_t* desc) {
    if (!out_handle || !g_rt_penumbra_ctx.initialized) return -1;
    if (g_rt_penumbra_ctx.count >= g_rt_penumbra_ctx.capacity) return -2;
    uint32_t index = g_rt_penumbra_ctx.count++;
    shadows_advanced_rt_penumbra_internal_t* pen = &g_rt_penumbra_ctx.items[index];
    pen->id = index;
    pen->initialized = true;
    out_handle->id = index;
    return 0;
}

void shadows_advanced_rt_penumbra_destroy(shadows_advanced_rt_penumbra_handle_t handle) {
    if (handle.id >= g_rt_penumbra_ctx.count) return;
    g_rt_penumbra_ctx.items[handle.id].initialized = false;
}

float rt_calculate_penumbra_width(float receiver_dist, float occluder_dist, float light_size) {
    if (occluder_dist <= 0.0f) return 0.0f;
    // Standard geometric penumbra formula: (d_receiver - d_occluder) / d_occluder * light_size
    return (receiver_dist - occluder_dist) / occluder_dist * light_size;
}

int shadows_advanced_rt_penumbra_update(shadows_advanced_rt_penumbra_handle_t handle, const void* data, size_t size) { return 0; }
bool shadows_advanced_rt_penumbra_is_valid(shadows_advanced_rt_penumbra_handle_t handle) {
    return handle.id < g_rt_penumbra_ctx.count && g_rt_penumbra_ctx.items[handle.id].initialized;
}
int shadows_advanced_rt_penumbra_get_info(shadows_advanced_rt_penumbra_handle_t handle, shadows_advanced_rt_penumbra_info_t* out_info) { return 0; }
void shadows_advanced_rt_penumbra_mark_dirty(shadows_advanced_rt_penumbra_handle_t handle) {}
int shadows_advanced_rt_penumbra_process_pending(void) { return 0; }
uint32_t shadows_advanced_rt_penumbra_get_count(void) { return g_rt_penumbra_ctx.count; }
size_t shadows_advanced_rt_penumbra_get_memory_usage(void) { return g_rt_penumbra_ctx.count * sizeof(shadows_advanced_rt_penumbra_internal_t); }
void shadows_advanced_rt_penumbra_debug_print(void) {}
