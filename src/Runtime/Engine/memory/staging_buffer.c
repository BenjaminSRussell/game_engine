#include "core/memory/staging_buffer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

#define CORE_STAGING_BUFFER_DEFAULT_CAPACITY 16
#define CORE_STAGING_BUFFER_SIZE (16 * 1024 * 1024) // 16MB default

typedef struct core_staging_buffer_internal {
    uint8_t* buffer;
    size_t size;
    size_t head;
    size_t tail;
    pthread_mutex_t mutex;
    bool initialized;
} core_staging_buffer_internal_t;

typedef struct core_staging_buffer_context {
    core_staging_buffer_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    pthread_mutex_t global_mutex;
    bool initialized;
} core_staging_buffer_context_t;

static core_staging_buffer_context_t g_staging_buffer_ctx = {0};

/* ============================================================================
 * API
 * ============================================================================ */

int core_staging_buffer_init(void) {
    if (g_staging_buffer_ctx.initialized) return 0;

    pthread_mutex_init(&g_staging_buffer_ctx.global_mutex, NULL);
    g_staging_buffer_ctx.capacity = CORE_STAGING_BUFFER_DEFAULT_CAPACITY;
    g_staging_buffer_ctx.items = calloc(g_staging_buffer_ctx.capacity, sizeof(core_staging_buffer_internal_t));
    if (!g_staging_buffer_ctx.items) return -1;

    g_staging_buffer_ctx.count = 0;
    g_staging_buffer_ctx.initialized = true;
    return 0;
}

void core_staging_buffer_shutdown(void) {
    if (!g_staging_buffer_ctx.initialized) return;

    for (uint32_t i = 0; i < g_staging_buffer_ctx.count; i++) {
        if (g_staging_buffer_ctx.items[i].buffer) {
            free(g_staging_buffer_ctx.items[i].buffer);
            pthread_mutex_destroy(&g_staging_buffer_ctx.items[i].mutex);
        }
    }

    free(g_staging_buffer_ctx.items);
    g_staging_buffer_ctx.items = NULL;
    g_staging_buffer_ctx.count = 0;
    g_staging_buffer_ctx.capacity = 0;

    pthread_mutex_destroy(&g_staging_buffer_ctx.global_mutex);
    g_staging_buffer_ctx.initialized = false;
}

int core_staging_buffer_create(core_staging_buffer_handle_t* out_handle, const core_staging_buffer_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_staging_buffer_ctx.initialized) return -2;

    pthread_mutex_lock(&g_staging_buffer_ctx.global_mutex);
    if (g_staging_buffer_ctx.count >= g_staging_buffer_ctx.capacity) {
        pthread_mutex_unlock(&g_staging_buffer_ctx.global_mutex);
        return -3;
    }

    uint32_t index = g_staging_buffer_ctx.count++;
    core_staging_buffer_internal_t* item = &g_staging_buffer_ctx.items[index];
    
    item->size = CORE_STAGING_BUFFER_SIZE;
    item->buffer = malloc(item->size);
    item->head = 0;
    item->tail = 0;
    pthread_mutex_init(&item->mutex, NULL);
    item->initialized = true;
    pthread_mutex_unlock(&g_staging_buffer_ctx.global_mutex);

    out_handle->id = index;
    return 0;
}

void core_staging_buffer_destroy(core_staging_buffer_handle_t handle) {
    if (handle.id < g_staging_buffer_ctx.count) {
        pthread_mutex_lock(&g_staging_buffer_ctx.items[handle.id].mutex);
        if (g_staging_buffer_ctx.items[handle.id].buffer) {
            free(g_staging_buffer_ctx.items[handle.id].buffer);
            g_staging_buffer_ctx.items[handle.id].buffer = NULL;
        }
        g_staging_buffer_ctx.items[handle.id].initialized = false;
        pthread_mutex_unlock(&g_staging_buffer_ctx.items[handle.id].mutex);
    }
}

int core_staging_buffer_push(core_staging_buffer_handle_t handle, const void* data, uint64_t size, uint64_t* out_offset) {
    if (handle.id >= g_staging_buffer_ctx.count || !data || size == 0) return -1;

    core_staging_buffer_internal_t* item = &g_staging_buffer_ctx.items[handle.id];
    if (!item->initialized) return -2;

    pthread_mutex_lock(&item->mutex);
    
    // Check if there's enough space in the ring buffer
    size_t free_space;
    if (item->head >= item->tail) {
        free_space = item->size - (item->head - item->tail) - 1;
    } else {
        free_space = item->tail - item->head - 1;
    }

    if (size > free_space) {
        pthread_mutex_unlock(&item->mutex);
        return -3; // Buffer full
    }

    if (out_offset) *out_offset = (uint64_t)item->head;

    // Circular copy
    size_t space_to_end = item->size - item->head;
    if (size <= (uint64_t)space_to_end) {
        memcpy(item->buffer + item->head, data, (size_t)size);
        item->head = (item->head + (size_t)size) % item->size;
    } else {
        memcpy(item->buffer + item->head, data, space_to_end);
        memcpy(item->buffer, (uint8_t*)data + space_to_end, (size_t)(size - space_to_end));
        item->head = (size_t)(size - space_to_end);
    }

    pthread_mutex_unlock(&item->mutex);
    return 0;
}

int core_staging_buffer_update(core_staging_buffer_handle_t handle, const void* data, size_t size) { return 0; }
bool core_staging_buffer_is_valid(core_staging_buffer_handle_t handle) {
    if (handle.id >= g_staging_buffer_ctx.count) return false;
    return g_staging_buffer_ctx.items[handle.id].initialized;
}

int core_staging_buffer_get_info(core_staging_buffer_handle_t handle, core_staging_buffer_info_t* out_info) {
    if (!out_info || handle.id >= g_staging_buffer_ctx.count) return -1;
    out_info->id = handle.id;
    out_info->initialized = g_staging_buffer_ctx.items[handle.id].initialized;
    return 0;
}

uint32_t core_staging_buffer_get_count(void) { return g_staging_buffer_ctx.count; }
size_t core_staging_buffer_get_memory_usage(void) {
    size_t total = 0;
    pthread_mutex_lock(&g_staging_buffer_ctx.global_mutex);
    for (uint32_t i = 0; i < g_staging_buffer_ctx.count; i++) {
        if (g_staging_buffer_ctx.items[i].initialized) {
            total += g_staging_buffer_ctx.items[i].size;
        }
    }
    pthread_mutex_unlock(&g_staging_buffer_ctx.global_mutex);
    return total;
}
void core_staging_buffer_debug_print(void) {}
void core_staging_buffer_mark_dirty(core_staging_buffer_handle_t handle) {}
int core_staging_buffer_process_pending(void) { return 0; }
