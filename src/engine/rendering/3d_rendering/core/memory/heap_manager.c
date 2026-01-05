#include "heap_manager.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

#define CORE_HEAP_MANAGER_DEFAULT_CAPACITY 256

typedef struct core_heap_manager_internal {
    uint64_t total_size;
    uint64_t allocated_size;
    pthread_mutex_t mutex;
    bool initialized;
} core_heap_manager_internal_t;

typedef struct core_heap_manager_context {
    core_heap_manager_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    pthread_mutex_t global_mutex;
    bool initialized;
} core_heap_manager_context_t;

static core_heap_manager_context_t g_heap_manager_ctx = {0};

/* ============================================================================
 * API
 * ============================================================================ */

int core_heap_manager_init(void) {
    if (g_heap_manager_ctx.initialized) return 0;

    pthread_mutex_init(&g_heap_manager_ctx.global_mutex, NULL);
    g_heap_manager_ctx.capacity = CORE_HEAP_MANAGER_DEFAULT_CAPACITY;
    g_heap_manager_ctx.items = calloc(g_heap_manager_ctx.capacity, sizeof(core_heap_manager_internal_t));
    if (!g_heap_manager_ctx.items) return -1;

    g_heap_manager_ctx.count = 0;
    g_heap_manager_ctx.initialized = true;
    return 0;
}

void core_heap_manager_shutdown(void) {
    if (!g_heap_manager_ctx.initialized) return;

    for (uint32_t i = 0; i < g_heap_manager_ctx.count; i++) {
        if (g_heap_manager_ctx.items[i].initialized) {
            pthread_mutex_destroy(&g_heap_manager_ctx.items[i].mutex);
        }
    }

    free(g_heap_manager_ctx.items);
    g_heap_manager_ctx.items = NULL;
    g_heap_manager_ctx.count = 0;
    g_heap_manager_ctx.capacity = 0;

    pthread_mutex_destroy(&g_heap_manager_ctx.global_mutex);
    g_heap_manager_ctx.initialized = false;
}

int core_heap_manager_create(core_heap_manager_handle_t* out_handle, const core_heap_manager_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_heap_manager_ctx.initialized) return -2;

    pthread_mutex_lock(&g_heap_manager_ctx.global_mutex);
    if (g_heap_manager_ctx.count >= g_heap_manager_ctx.capacity) {
        pthread_mutex_unlock(&g_heap_manager_ctx.global_mutex);
        return -3;
    }

    uint32_t index = g_heap_manager_ctx.count++;
    core_heap_manager_internal_t* item = &g_heap_manager_ctx.items[index];
    
    item->total_size = 0;
    item->allocated_size = 0;
    pthread_mutex_init(&item->mutex, NULL);
    item->initialized = true;
    pthread_mutex_unlock(&g_heap_manager_ctx.global_mutex);

    out_handle->id = index;
    return 0;
}

void core_heap_manager_destroy(core_heap_manager_handle_t handle) {
    if (handle.id < g_heap_manager_ctx.count) {
        pthread_mutex_lock(&g_heap_manager_ctx.items[handle.id].mutex);
        g_heap_manager_ctx.items[handle.id].initialized = false;
        pthread_mutex_unlock(&g_heap_manager_ctx.items[handle.id].mutex);
    }
}

int core_heap_manager_add_heap(core_heap_manager_handle_t handle, uint64_t size, uint32_t flags) {
    if (handle.id >= g_heap_manager_ctx.count) return -1;
    
    core_heap_manager_internal_t* item = &g_heap_manager_ctx.items[handle.id];
    if (!item->initialized) return -2;

    pthread_mutex_lock(&item->mutex);
    item->total_size += size;
    pthread_mutex_unlock(&item->mutex);

    return 0;
}

int core_heap_manager_update(core_heap_manager_handle_t handle, const void* data, size_t size) { return 0; }
bool core_heap_manager_is_valid(core_heap_manager_handle_t handle) {
    if (handle.id >= g_heap_manager_ctx.count) return false;
    return g_heap_manager_ctx.items[handle.id].initialized;
}

int core_heap_manager_get_info(core_heap_manager_handle_t handle, core_heap_manager_info_t* out_info) {
    if (!out_info || handle.id >= g_heap_manager_ctx.count) return -1;
    out_info->id = handle.id;
    out_info->initialized = g_heap_manager_ctx.items[handle.id].initialized;
    return 0;
}

uint32_t core_heap_manager_get_count(void) { return g_heap_manager_ctx.count; }
size_t core_heap_manager_get_memory_usage(void) {
    size_t total = 0;
    pthread_mutex_lock(&g_heap_manager_ctx.global_mutex);
    total += g_heap_manager_ctx.capacity * sizeof(core_heap_manager_internal_t);
    for (uint32_t i = 0; i < g_heap_manager_ctx.count; i++) {
        if (g_heap_manager_ctx.items[i].initialized) total += g_heap_manager_ctx.items[i].allocated_size;
    }
    pthread_mutex_unlock(&g_heap_manager_ctx.global_mutex);
    return total;
}
void core_heap_manager_debug_print(void) {}
void core_heap_manager_mark_dirty(core_heap_manager_handle_t handle) {}
int core_heap_manager_process_pending(void) { return 0; }
