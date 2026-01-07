/*
 * vsm_invalidation.c
 * VSM page invalidation
 */

#include "lighting/vsm/virtual_shadow_maps/page_management/vsm_invalidation.h"
#include "lighting/vsm/virtual_shadow_maps/page_management/vsm_page_table.h"
#include "lighting/vsm/virtual_shadow_maps/page_management/vsm_page_pool.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef struct virtual_shadow_maps_vsm_invalidation_internal {
    uint32_t id;
    bool initialized;
} virtual_shadow_maps_vsm_invalidation_internal_t;

typedef struct virtual_shadow_maps_vsm_invalidation_context {
    virtual_shadow_maps_vsm_invalidation_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} virtual_shadow_maps_vsm_invalidation_context_t;

static virtual_shadow_maps_vsm_invalidation_context_t g_vsm_invalidation_ctx = {0};

int virtual_shadow_maps_vsm_invalidation_init(void) {
    if (g_vsm_invalidation_ctx.initialized) return 0;
    g_vsm_invalidation_ctx.capacity = 16;
    g_vsm_invalidation_ctx.items = calloc(g_vsm_invalidation_ctx.capacity, sizeof(virtual_shadow_maps_vsm_invalidation_internal_t));
    if (!g_vsm_invalidation_ctx.items) return -1;
    g_vsm_invalidation_ctx.initialized = true;
    return 0;
}

void virtual_shadow_maps_vsm_invalidation_shutdown(void) {
    if (!g_vsm_invalidation_ctx.initialized) return;
    free(g_vsm_invalidation_ctx.items);
    g_vsm_invalidation_ctx.items = NULL;
    g_vsm_invalidation_ctx.initialized = false;
}

int virtual_shadow_maps_vsm_invalidation_create(virtual_shadow_maps_vsm_invalidation_handle_t* out_handle, const virtual_shadow_maps_vsm_invalidation_desc_t* desc) {
    if (!out_handle || !g_vsm_invalidation_ctx.initialized) return -1;
    if (g_vsm_invalidation_ctx.count >= g_vsm_invalidation_ctx.capacity) return -2;
    uint32_t index = g_vsm_invalidation_ctx.count++;
    virtual_shadow_maps_vsm_invalidation_internal_t* inv = &g_vsm_invalidation_ctx.items[index];
    inv->id = index;
    inv->initialized = true;
    out_handle->id = index;
    return 0;
}

void virtual_shadow_maps_vsm_invalidation_destroy(virtual_shadow_maps_vsm_invalidation_handle_t handle) {
    if (handle.id >= g_vsm_invalidation_ctx.count) return;
    g_vsm_invalidation_ctx.items[handle.id].initialized = false;
}

void vsm_invalidate_page(virtual_shadow_maps_vsm_page_table_handle_t table, virtual_shadow_maps_vsm_page_pool_handle_t pool, uint32_t v_x, uint32_t v_y) {
    uint32_t p_index = vsm_page_table_get_physical_index(table, v_x, v_y);
    if (p_index != 0xFFFFFFFFU) {
        vsm_page_pool_free(pool, p_index);
        vsm_page_table_set_physical_index(table, v_x, v_y, 0xFFFFFFFFU);
    }
}

int virtual_shadow_maps_vsm_invalidation_update(virtual_shadow_maps_vsm_invalidation_handle_t handle, const void* data, size_t size) { return 0; }
bool virtual_shadow_maps_vsm_invalidation_is_valid(virtual_shadow_maps_vsm_invalidation_handle_t handle) {
    return handle.id < g_vsm_invalidation_ctx.count && g_vsm_invalidation_ctx.items[handle.id].initialized;
}
int virtual_shadow_maps_vsm_invalidation_get_info(virtual_shadow_maps_vsm_invalidation_handle_t handle, virtual_shadow_maps_vsm_invalidation_info_t* out_info) { return 0; }
void virtual_shadow_maps_vsm_invalidation_mark_dirty(virtual_shadow_maps_vsm_invalidation_handle_t handle) {}
int virtual_shadow_maps_vsm_invalidation_process_pending(void) { return 0; }
uint32_t virtual_shadow_maps_vsm_invalidation_get_count(void) { return g_vsm_invalidation_ctx.count; }
size_t virtual_shadow_maps_vsm_invalidation_get_memory_usage(void) { return g_vsm_invalidation_ctx.count * sizeof(virtual_shadow_maps_vsm_invalidation_internal_t); }
void virtual_shadow_maps_vsm_invalidation_debug_print(void) {}
