#include "perf_overlay.h"
#include "../../statistics/stats_tracker.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct perf_overlay_item {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} perf_overlay_item_t;

#define MAX_PERF_ITEMS 16
static perf_overlay_item_t g_perf_items[MAX_PERF_ITEMS];
static uint32_t g_perf_count = 0;
static bool g_perf_initialized = false;

int profiling_perf_overlay_init(void) {
    if (g_perf_initialized) return 0;
    memset(g_perf_items, 0, sizeof(g_perf_items));
    g_perf_count = 0;
    g_perf_initialized = true;
    return 0;
}

void profiling_perf_overlay_shutdown(void) {
    g_perf_initialized = false;
    g_perf_count = 0;
}

int profiling_perf_overlay_create(profiling_perf_overlay_handle_t* out_handle, const profiling_perf_overlay_desc_t* desc) {
    if (g_perf_count >= MAX_PERF_ITEMS) return -1;
    
    uint32_t index = g_perf_count++;
    g_perf_items[index].id = index;
    g_perf_items[index].flags = desc->flags;
    g_perf_items[index].initialized = true;
    
    out_handle->id = index;
    return 0;
}

void profiling_perf_overlay_destroy(profiling_perf_overlay_handle_t handle) {
    if (handle.id < g_perf_count) {
        g_perf_items[handle.id].initialized = false;
    }
}

int profiling_perf_overlay_update(profiling_perf_overlay_handle_t handle, const void* data, size_t size) {
    (void)data; (void)size;
    if (handle.id >= g_perf_count || !g_perf_items[handle.id].initialized) return -1;
    return 0;
}

bool profiling_perf_overlay_is_valid(profiling_perf_overlay_handle_t handle) {
    return handle.id < g_perf_count && g_perf_items[handle.id].initialized;
}

int profiling_perf_overlay_get_info(profiling_perf_overlay_handle_t handle, profiling_perf_overlay_info_t* out_info) {
    if (handle.id >= g_perf_count) return -1;
    out_info->id = g_perf_items[handle.id].id;
    out_info->flags = g_perf_items[handle.id].flags;
    out_info->initialized = g_perf_items[handle.id].initialized;
    return 0;
}

void profiling_perf_overlay_mark_dirty(profiling_perf_overlay_handle_t handle) {
    (void)handle;
}

int profiling_perf_overlay_process_pending(void) {
    return 0;
}

uint32_t profiling_perf_overlay_get_count(void) {
    return g_perf_count;
}

size_t profiling_perf_overlay_get_memory_usage(void) {
    return sizeof(g_perf_items);
}

void profiling_perf_overlay_debug_print(void) {
    printf("Perf Overlay: %u active items\n", g_perf_count);
}
