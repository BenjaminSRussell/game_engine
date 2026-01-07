/*
 * vsm_page_pool.c
 * VSM physical page pool management
 */

#include "lighting/vsm/virtual_shadow_maps/page_management/vsm_page_pool.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define VSM_PAGE_POOL_SIZE (128 * 128)
#define VSM_PAGE_SIZE 128
#define INVALID_PAGE_INDEX 0xFFFFFFFFU

typedef struct vsm_page_metadata {
    uint32_t virtual_coord_x;
    uint32_t virtual_coord_y;
    uint64_t last_used_frame;
    bool allocated;
    bool dirty;
} vsm_page_metadata_t;

typedef struct virtual_shadow_maps_vsm_page_pool_internal {
    uint32_t id;
    vsm_page_metadata_t metadata[VSM_PAGE_POOL_SIZE];
    uint32_t free_stack[VSM_PAGE_POOL_SIZE];
    uint32_t free_stack_top;
    uint32_t allocated_count;
    bool initialized;
} virtual_shadow_maps_vsm_page_pool_internal_t;

typedef struct virtual_shadow_maps_vsm_page_pool_context {
    virtual_shadow_maps_vsm_page_pool_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} virtual_shadow_maps_vsm_page_pool_context_t;

static virtual_shadow_maps_vsm_page_pool_context_t g_vsm_page_pool_ctx = {0};

int virtual_shadow_maps_vsm_page_pool_init(void) {
    if (g_vsm_page_pool_ctx.initialized) return 0;
    g_vsm_page_pool_ctx.capacity = 16;
    g_vsm_page_pool_ctx.items = calloc(g_vsm_page_pool_ctx.capacity, sizeof(virtual_shadow_maps_vsm_page_pool_internal_t));
    if (!g_vsm_page_pool_ctx.items) return -1;
    g_vsm_page_pool_ctx.initialized = true;
    return 0;
}

void virtual_shadow_maps_vsm_page_pool_shutdown(void) {
    if (!g_vsm_page_pool_ctx.initialized) return;
    free(g_vsm_page_pool_ctx.items);
    g_vsm_page_pool_ctx.items = NULL;
    g_vsm_page_pool_ctx.initialized = false;
}

int virtual_shadow_maps_vsm_page_pool_create(virtual_shadow_maps_vsm_page_pool_handle_t* out_handle, const virtual_shadow_maps_vsm_page_pool_desc_t* desc) {
    if (!out_handle || !g_vsm_page_pool_ctx.initialized) return -1;
    if (g_vsm_page_pool_ctx.count >= g_vsm_page_pool_ctx.capacity) return -2;
    uint32_t index = g_vsm_page_pool_ctx.count++;
    virtual_shadow_maps_vsm_page_pool_internal_t* pool = &g_vsm_page_pool_ctx.items[index];
    pool->id = index;
    pool->free_stack_top = VSM_PAGE_POOL_SIZE;
    for (uint32_t i = 0; i < VSM_PAGE_POOL_SIZE; i++) {
        pool->free_stack[i] = (VSM_PAGE_POOL_SIZE - 1) - i;
        pool->metadata[i].allocated = false;
    }
    pool->initialized = true;
    out_handle->id = index;
    return 0;
}

void virtual_shadow_maps_vsm_page_pool_destroy(virtual_shadow_maps_vsm_page_pool_handle_t handle) {
    if (handle.id >= g_vsm_page_pool_ctx.count) return;
    g_vsm_page_pool_ctx.items[handle.id].initialized = false;
}

uint32_t vsm_page_pool_alloc(virtual_shadow_maps_vsm_page_pool_handle_t handle, uint32_t v_x, uint32_t v_y, uint64_t frame) {
    if (handle.id >= g_vsm_page_pool_ctx.count) return INVALID_PAGE_INDEX;
    virtual_shadow_maps_vsm_page_pool_internal_t* pool = &g_vsm_page_pool_ctx.items[handle.id];
    if (pool->free_stack_top == 0) return INVALID_PAGE_INDEX;
    uint32_t physical_index = pool->free_stack[--pool->free_stack_top];
    pool->metadata[physical_index].allocated = true;
    pool->metadata[physical_index].virtual_coord_x = v_x;
    pool->metadata[physical_index].virtual_coord_y = v_y;
    pool->metadata[physical_index].last_used_frame = frame;
    pool->metadata[physical_index].dirty = true;
    pool->allocated_count++;
    return physical_index;
}

void vsm_page_pool_free(virtual_shadow_maps_vsm_page_pool_handle_t handle, uint32_t physical_index) {
    if (handle.id >= g_vsm_page_pool_ctx.count || physical_index >= VSM_PAGE_POOL_SIZE) return;
    virtual_shadow_maps_vsm_page_pool_internal_t* pool = &g_vsm_page_pool_ctx.items[handle.id];
    if (!pool->metadata[physical_index].allocated) return;
    pool->metadata[physical_index].allocated = false;
    pool->free_stack[pool->free_stack_top++] = physical_index;
    pool->allocated_count--;
}

void vsm_page_pool_touch(virtual_shadow_maps_vsm_page_pool_handle_t handle, uint32_t physical_index, uint64_t frame) {
    if (handle.id >= g_vsm_page_pool_ctx.count || physical_index >= VSM_PAGE_POOL_SIZE) return;
    g_vsm_page_pool_ctx.items[handle.id].metadata[physical_index].last_used_frame = frame;
}

int virtual_shadow_maps_vsm_page_pool_update(virtual_shadow_maps_vsm_page_pool_handle_t handle, const void* data, size_t size) { return 0; }
bool virtual_shadow_maps_vsm_page_pool_is_valid(virtual_shadow_maps_vsm_page_pool_handle_t handle) {
    return handle.id < g_vsm_page_pool_ctx.count && g_vsm_page_pool_ctx.items[handle.id].initialized;
}
int virtual_shadow_maps_vsm_page_pool_get_info(virtual_shadow_maps_vsm_page_pool_handle_t handle, virtual_shadow_maps_vsm_page_pool_info_t* out_info) { return 0; }
void virtual_shadow_maps_vsm_page_pool_mark_dirty(virtual_shadow_maps_vsm_page_pool_handle_t handle) {}
int virtual_shadow_maps_vsm_page_pool_process_pending(void) { return 0; }
uint32_t virtual_shadow_maps_vsm_page_pool_get_count(void) { return g_vsm_page_pool_ctx.count; }
size_t virtual_shadow_maps_vsm_page_pool_get_memory_usage(void) { return g_vsm_page_pool_ctx.count * sizeof(virtual_shadow_maps_vsm_page_pool_internal_t); }
void virtual_shadow_maps_vsm_page_pool_debug_print(void) {}
