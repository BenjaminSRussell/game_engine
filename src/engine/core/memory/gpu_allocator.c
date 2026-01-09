#include "core/memory/gpu_allocator.h"
#include "core/device/device_memory.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

#define CORE_GPU_ALLOCATOR_DEFAULT_CAPACITY 16

typedef struct core_gpu_allocator_internal {
    render_memory_allocator_t allocator;
    pthread_mutex_t mutex;
    bool initialized;
} core_gpu_allocator_internal_t;

typedef struct core_gpu_allocator_context {
    core_gpu_allocator_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    pthread_mutex_t global_mutex;
    bool initialized;
} core_gpu_allocator_context_t;

static core_gpu_allocator_context_t g_gpu_allocator_ctx = {0};

/* ============================================================================
 * API
 * ============================================================================ */

int core_gpu_allocator_init(void) {
    if (g_gpu_allocator_ctx.initialized) return 0;

    pthread_mutex_init(&g_gpu_allocator_ctx.global_mutex, NULL);
    g_gpu_allocator_ctx.capacity = CORE_GPU_ALLOCATOR_DEFAULT_CAPACITY;
    g_gpu_allocator_ctx.items = calloc(g_gpu_allocator_ctx.capacity, sizeof(core_gpu_allocator_internal_t));
    if (!g_gpu_allocator_ctx.items) return -1;

    g_gpu_allocator_ctx.count = 0;
    g_gpu_allocator_ctx.initialized = true;
    return 0;
}

void core_gpu_allocator_shutdown(void) {
    if (!g_gpu_allocator_ctx.initialized) return;

    for (uint32_t i = 0; i < g_gpu_allocator_ctx.count; i++) {
        if (g_gpu_allocator_ctx.items[i].initialized) {
            pthread_mutex_destroy(&g_gpu_allocator_ctx.items[i].mutex);
        }
    }

    free(g_gpu_allocator_ctx.items);
    g_gpu_allocator_ctx.items = NULL;
    g_gpu_allocator_ctx.count = 0;
    g_gpu_allocator_ctx.capacity = 0;
    
    pthread_mutex_destroy(&g_gpu_allocator_ctx.global_mutex);
    g_gpu_allocator_ctx.initialized = false;
}

int core_gpu_allocator_create(core_gpu_allocator_handle_t* out_handle, const core_gpu_allocator_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_gpu_allocator_ctx.initialized) return -2;

    pthread_mutex_lock(&g_gpu_allocator_ctx.global_mutex);
    if (g_gpu_allocator_ctx.count >= g_gpu_allocator_ctx.capacity) {
        pthread_mutex_unlock(&g_gpu_allocator_ctx.global_mutex);
        return -3;
    }

    uint32_t index = g_gpu_allocator_ctx.count++;
    core_gpu_allocator_internal_t* item = &g_gpu_allocator_ctx.items[index];
    
    memset(&item->allocator, 0, sizeof(render_memory_allocator_t));
    pthread_mutex_init(&item->mutex, NULL);
    item->initialized = true;
    pthread_mutex_unlock(&g_gpu_allocator_ctx.global_mutex);

    out_handle->id = index;
    return 0;
}

void core_gpu_allocator_destroy(core_gpu_allocator_handle_t handle) {
    if (handle.id < g_gpu_allocator_ctx.count) {
        pthread_mutex_lock(&g_gpu_allocator_ctx.items[handle.id].mutex);
        g_gpu_allocator_ctx.items[handle.id].initialized = false;
        pthread_mutex_unlock(&g_gpu_allocator_ctx.items[handle.id].mutex);
    }
}

int core_gpu_allocator_alloc(core_gpu_allocator_handle_t handle, uint64_t size, uint32_t alignment, gpu_allocation_t* out_allocation) {
    if (handle.id >= g_gpu_allocator_ctx.count || !out_allocation) return -1;
    
    core_gpu_allocator_internal_t* item = &g_gpu_allocator_ctx.items[handle.id];
    if (!item->initialized) return -2;

    pthread_mutex_lock(&item->mutex);
    // Note: alignment is ignored for now as core_device_memory_allocate uses hardcoded 256
    int res = core_device_memory_allocate(out_allocation, size, out_allocation->flags);
    if (res == 0) {
        item->allocator.allocated_size += size;
        item->allocator.allocation_count++;
    }
    pthread_mutex_unlock(&item->mutex);

    return res;
}

void core_gpu_allocator_free(core_gpu_allocator_handle_t handle, gpu_allocation_t* allocation) {
    if (handle.id >= g_gpu_allocator_ctx.count || !allocation) return;

    core_gpu_allocator_internal_t* item = &g_gpu_allocator_ctx.items[handle.id];
    if (!item->initialized) return;

    pthread_mutex_lock(&item->mutex);
    uint64_t size = allocation->size;
    core_device_memory_free(allocation);
    item->allocator.allocated_size -= size;
    item->allocator.allocation_count--;
    pthread_mutex_unlock(&item->mutex);
}

int core_gpu_allocator_update(core_gpu_allocator_handle_t handle, const void* data, size_t size) { return 0; }
bool core_gpu_allocator_is_valid(core_gpu_allocator_handle_t handle) {
    if (handle.id >= g_gpu_allocator_ctx.count) return false;
    return g_gpu_allocator_ctx.items[handle.id].initialized;
}

int core_gpu_allocator_get_info(core_gpu_allocator_handle_t handle, core_gpu_allocator_info_t* out_info) {
    if (!out_info || handle.id >= g_gpu_allocator_ctx.count) return -1;
    out_info->id = handle.id;
    out_info->initialized = g_gpu_allocator_ctx.items[handle.id].initialized;
    return 0;
}

uint32_t core_gpu_allocator_get_count(void) { return g_gpu_allocator_ctx.count; }
size_t core_gpu_allocator_get_memory_usage(void) {
    size_t total = 0;
    pthread_mutex_lock(&g_gpu_allocator_ctx.global_mutex);
    for (uint32_t i = 0; i < g_gpu_allocator_ctx.count; i++) {
        if (g_gpu_allocator_ctx.items[i].initialized) {
            total += (size_t)g_gpu_allocator_ctx.items[i].allocator.allocated_size;
        }
    }
    pthread_mutex_unlock(&g_gpu_allocator_ctx.global_mutex);
    return total;
}
void core_gpu_allocator_debug_print(void) {}
void core_gpu_allocator_mark_dirty(core_gpu_allocator_handle_t handle) {}
int core_gpu_allocator_process_pending(void) { return 0; }
