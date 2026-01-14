#include "core/memory/memory_pool.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

#define CORE_MEMORY_POOL_DEFAULT_CAPACITY 256

typedef struct core_memory_pool_internal {
    void* buffer;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t free_index;
    uint32_t* next_free;
    pthread_mutex_t mutex;
    bool initialized;
} core_memory_pool_internal_t;

typedef struct core_memory_pool_context {
    core_memory_pool_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    pthread_mutex_t global_mutex;
    bool initialized;
} core_memory_pool_context_t;

static core_memory_pool_context_t g_memory_pool_ctx = {0};

/* ============================================================================
 * API
 * ============================================================================ */

int core_memory_pool_init(void) {
    if (g_memory_pool_ctx.initialized) return 0;

    pthread_mutex_init(&g_memory_pool_ctx.global_mutex, NULL);
    g_memory_pool_ctx.capacity = CORE_MEMORY_POOL_DEFAULT_CAPACITY;
    g_memory_pool_ctx.items = calloc(g_memory_pool_ctx.capacity, sizeof(core_memory_pool_internal_t));
    if (!g_memory_pool_ctx.items) return -1;

    g_memory_pool_ctx.count = 0;
    g_memory_pool_ctx.initialized = true;
    return 0;
}

void core_memory_pool_shutdown(void) {
    if (!g_memory_pool_ctx.initialized) return;

    for (uint32_t i = 0; i < g_memory_pool_ctx.count; i++) {
        if (g_memory_pool_ctx.items[i].buffer) {
            free(g_memory_pool_ctx.items[i].buffer);
            free(g_memory_pool_ctx.items[i].next_free);
            pthread_mutex_destroy(&g_memory_pool_ctx.items[i].mutex);
        }
    }

    free(g_memory_pool_ctx.items);
    g_memory_pool_ctx.items = NULL;
    g_memory_pool_ctx.count = 0;
    g_memory_pool_ctx.capacity = 0;

    pthread_mutex_destroy(&g_memory_pool_ctx.global_mutex);
    g_memory_pool_ctx.initialized = false;
}

int core_memory_pool_create(core_memory_pool_handle_t* out_handle, const core_memory_pool_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_memory_pool_ctx.initialized) return -2;

    pthread_mutex_lock(&g_memory_pool_ctx.global_mutex);
    if (g_memory_pool_ctx.count >= g_memory_pool_ctx.capacity) {
        pthread_mutex_unlock(&g_memory_pool_ctx.global_mutex);
        return -3;
    }

    uint32_t index = g_memory_pool_ctx.count++;
    core_memory_pool_internal_t* item = &g_memory_pool_ctx.items[index];
    
    // Default: 1024 blocks of 1KB
    item->block_size = 1024;
    item->block_count = 1024;
    item->buffer = malloc(item->block_size * item->block_count);
    item->next_free = malloc(sizeof(uint32_t) * item->block_count);
    for (uint32_t i = 0; i < item->block_count; i++) {
        item->next_free[i] = i + 1;
    }
    item->next_free[item->block_count - 1] = 0xFFFFFFFF;
    item->free_index = 0;
    pthread_mutex_init(&item->mutex, NULL);
    item->initialized = true;
    pthread_mutex_unlock(&g_memory_pool_ctx.global_mutex);

    out_handle->id = index;
    return 0;
}

void core_memory_pool_destroy(core_memory_pool_handle_t handle) {
    if (handle.id < g_memory_pool_ctx.count) {
        pthread_mutex_lock(&g_memory_pool_ctx.items[handle.id].mutex);
        if (g_memory_pool_ctx.items[handle.id].buffer) {
            free(g_memory_pool_ctx.items[handle.id].buffer);
            g_memory_pool_ctx.items[handle.id].buffer = NULL;
        }
        if (g_memory_pool_ctx.items[handle.id].next_free) {
            free(g_memory_pool_ctx.items[handle.id].next_free);
            g_memory_pool_ctx.items[handle.id].next_free = NULL;
        }
        g_memory_pool_ctx.items[handle.id].initialized = false;
        pthread_mutex_unlock(&g_memory_pool_ctx.items[handle.id].mutex);
    }
}

void* core_memory_pool_alloc(core_memory_pool_handle_t handle) {
    if (handle.id >= g_memory_pool_ctx.count) return NULL;
    
    core_memory_pool_internal_t* item = &g_memory_pool_ctx.items[handle.id];
    if (!item->initialized) return NULL;

    pthread_mutex_lock(&item->mutex);
    if (item->free_index == 0xFFFFFFFF) {
        pthread_mutex_unlock(&item->mutex);
        return NULL;
    }

    uint32_t index = item->free_index;
    item->free_index = item->next_free[index];
    pthread_mutex_unlock(&item->mutex);

    return (uint8_t*)item->buffer + (index * item->block_size);
}

void core_memory_pool_free(core_memory_pool_handle_t handle, void* ptr) {
    if (handle.id >= g_memory_pool_ctx.count || !ptr) return;

    core_memory_pool_internal_t* item = &g_memory_pool_ctx.items[handle.id];
    if (!item->initialized) return;

    pthread_mutex_lock(&item->mutex);
    uint32_t index = (uint32_t)(((uint8_t*)ptr - (uint8_t*)item->buffer) / item->block_size);
    item->next_free[index] = item->free_index;
    item->free_index = index;
    pthread_mutex_unlock(&item->mutex);
}

int core_memory_pool_update(core_memory_pool_handle_t handle, const void* data, size_t size) { return 0; }
bool core_memory_pool_is_valid(core_memory_pool_handle_t handle) {
    if (handle.id >= g_memory_pool_ctx.count) return false;
    return g_memory_pool_ctx.items[handle.id].initialized;
}

int core_memory_pool_get_info(core_memory_pool_handle_t handle, core_memory_pool_info_t* out_info) {
    if (!out_info || handle.id >= g_memory_pool_ctx.count) return -1;
    out_info->id = handle.id;
    out_info->initialized = g_memory_pool_ctx.items[handle.id].initialized;
    return 0;
}

uint32_t core_memory_pool_get_count(void) { return g_memory_pool_ctx.count; }
size_t core_memory_pool_get_memory_usage(void) {
    size_t total = 0;
    pthread_mutex_lock(&g_memory_pool_ctx.global_mutex);
    total += g_memory_pool_ctx.capacity * sizeof(core_memory_pool_internal_t);
    for (uint32_t i = 0; i < g_memory_pool_ctx.count; i++) {
        if (g_memory_pool_ctx.items[i].initialized) {
            total += g_memory_pool_ctx.items[i].block_size * g_memory_pool_ctx.items[i].block_count;
            total += sizeof(uint32_t) * g_memory_pool_ctx.items[i].block_count;
        }
    }
    pthread_mutex_unlock(&g_memory_pool_ctx.global_mutex);
    return total;
}
void core_memory_pool_debug_print(void) {}
void core_memory_pool_mark_dirty(core_memory_pool_handle_t handle) {}
int core_memory_pool_process_pending(void) { return 0; }
