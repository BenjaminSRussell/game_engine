/*
 * vsm_page_table.c
 * VSM page table management
 */

#include "vsm_page_table.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define VSM_VIRTUAL_PAGES_X 128
#define VSM_VIRTUAL_PAGES_Y 128
#define VSM_VIRTUAL_PAGE_COUNT (VSM_VIRTUAL_PAGES_X * VSM_VIRTUAL_PAGES_Y)
#define INVALID_PHYSICAL_INDEX 0xFFFFFFFFU

typedef struct virtual_shadow_maps_vsm_page_table_internal {
    uint32_t id;
    uint32_t physical_indices[VSM_VIRTUAL_PAGE_COUNT];
    bool initialized;
} virtual_shadow_maps_vsm_page_table_internal_t;

typedef struct virtual_shadow_maps_vsm_page_table_context {
    virtual_shadow_maps_vsm_page_table_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} virtual_shadow_maps_vsm_page_table_context_t;

static virtual_shadow_maps_vsm_page_table_context_t g_vsm_page_table_ctx = {0};

int virtual_shadow_maps_vsm_page_table_init(void) {
    if (g_vsm_page_table_ctx.initialized) return 0;
    g_vsm_page_table_ctx.capacity = 16;
    g_vsm_page_table_ctx.items = calloc(g_vsm_page_table_ctx.capacity, sizeof(virtual_shadow_maps_vsm_page_table_internal_t));
    if (!g_vsm_page_table_ctx.items) return -1;
    g_vsm_page_table_ctx.initialized = true;
    return 0;
}

void virtual_shadow_maps_vsm_page_table_shutdown(void) {
    if (!g_vsm_page_table_ctx.initialized) return;
    free(g_vsm_page_table_ctx.items);
    g_vsm_page_table_ctx.items = NULL;
    g_vsm_page_table_ctx.initialized = false;
}

int virtual_shadow_maps_vsm_page_table_create(virtual_shadow_maps_vsm_page_table_handle_t* out_handle, const virtual_shadow_maps_vsm_page_table_desc_t* desc) {
    if (!out_handle || !g_vsm_page_table_ctx.initialized) return -1;
    if (g_vsm_page_table_ctx.count >= g_vsm_page_table_ctx.capacity) return -2;
    uint32_t index = g_vsm_page_table_ctx.count++;
    virtual_shadow_maps_vsm_page_table_internal_t* table = &g_vsm_page_table_ctx.items[index];
    table->id = index;
    for (uint32_t i = 0; i < VSM_VIRTUAL_PAGE_COUNT; i++) {
        table->physical_indices[i] = INVALID_PHYSICAL_INDEX;
    }
    table->initialized = true;
    out_handle->id = index;
    return 0;
}

void virtual_shadow_maps_vsm_page_table_destroy(virtual_shadow_maps_vsm_page_table_handle_t handle) {
    if (handle.id >= g_vsm_page_table_ctx.count) return;
    g_vsm_page_table_ctx.items[handle.id].initialized = false;
}

int virtual_shadow_maps_vsm_page_table_update(virtual_shadow_maps_vsm_page_table_handle_t handle, const void* data, size_t size) { return 0; }
bool virtual_shadow_maps_vsm_page_table_is_valid(virtual_shadow_maps_vsm_page_table_handle_t handle) {
    return handle.id < g_vsm_page_table_ctx.count && g_vsm_page_table_ctx.items[handle.id].initialized;
}
int virtual_shadow_maps_vsm_page_table_get_info(virtual_shadow_maps_vsm_page_table_handle_t handle, virtual_shadow_maps_vsm_page_table_info_t* out_info) { return 0; }
void virtual_shadow_maps_vsm_page_table_mark_dirty(virtual_shadow_maps_vsm_page_table_handle_t handle) {}
int virtual_shadow_maps_vsm_page_table_process_pending(void) { return 0; }

uint32_t vsm_page_table_get_physical_index(virtual_shadow_maps_vsm_page_table_handle_t handle, uint32_t v_x, uint32_t v_y) {
    if (handle.id >= g_vsm_page_table_ctx.count || v_x >= VSM_VIRTUAL_PAGES_X || v_y >= VSM_VIRTUAL_PAGES_Y) return INVALID_PHYSICAL_INDEX;
    return g_vsm_page_table_ctx.items[handle.id].physical_indices[v_y * VSM_VIRTUAL_PAGES_X + v_x];
}

void vsm_page_table_set_physical_index(virtual_shadow_maps_vsm_page_table_handle_t handle, uint32_t v_x, uint32_t v_y, uint32_t p_index) {
    if (handle.id >= g_vsm_page_table_ctx.count || v_x >= VSM_VIRTUAL_PAGES_X || v_y >= VSM_VIRTUAL_PAGES_Y) return;
    g_vsm_page_table_ctx.items[handle.id].physical_indices[v_y * VSM_VIRTUAL_PAGES_X + v_x] = p_index;
}

uint32_t virtual_shadow_maps_vsm_page_table_get_count(void) { return g_vsm_page_table_ctx.count; }
size_t virtual_shadow_maps_vsm_page_table_get_memory_usage(void) { return g_vsm_page_table_ctx.count * sizeof(virtual_shadow_maps_vsm_page_table_internal_t); }
void virtual_shadow_maps_vsm_page_table_debug_print(void) {}
