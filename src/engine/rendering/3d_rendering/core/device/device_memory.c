#include "device_memory.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct core_allocation_node {
    void* ptr;
    size_t size;
    struct core_allocation_node* next;
} core_allocation_node_t;

typedef struct core_device_memory_internal {
    core_allocation_node_t* head;
    pthread_mutex_t mutex;
    bool initialized;
} core_device_memory_internal_t;

static core_device_memory_internal_t g_device_memory_ctx = {0};

/* ============================================================================
 * API
 * ============================================================================ */

int core_device_memory_init(void) {
    if (g_device_memory_ctx.initialized) return 0;
    
    pthread_mutex_init(&g_device_memory_ctx.mutex, NULL);
    g_device_memory_ctx.head = NULL;
    g_device_memory_ctx.initialized = true;
    return 0;
}

void core_device_memory_shutdown(void) {
    if (!g_device_memory_ctx.initialized) return;

    pthread_mutex_lock(&g_device_memory_ctx.mutex);
    core_allocation_node_t* curr = g_device_memory_ctx.head;
    while (curr) {
        core_allocation_node_t* next = curr->next;
        free(curr->ptr);
        free(curr);
        curr = next;
    }
    g_device_memory_ctx.head = NULL;
    pthread_mutex_unlock(&g_device_memory_ctx.mutex);

    pthread_mutex_destroy(&g_device_memory_ctx.mutex);
    g_device_memory_ctx.initialized = false;
}

int core_device_memory_allocate(gpu_allocation_t* out_allocation, uint64_t size, uint32_t flags) {
    if (!out_allocation || size == 0) return -1;
    if (!g_device_memory_ctx.initialized) return -2;

    void* ptr = NULL;
    size_t alignment = 256; // Standard GPU alignment for buffers/textures
    
    // posix_memalign requires alignment to be a power of 2 and a multiple of sizeof(void*)
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return -3;
    }

    pthread_mutex_lock(&g_device_memory_ctx.mutex);
    core_allocation_node_t* node = malloc(sizeof(core_allocation_node_t));
    if (!node) {
        free(ptr);
        pthread_mutex_unlock(&g_device_memory_ctx.mutex);
        return -4;
    }
    node->ptr = ptr;
    node->size = size;
    node->next = g_device_memory_ctx.head;
    g_device_memory_ctx.head = node;
    pthread_mutex_unlock(&g_device_memory_ctx.mutex);

    out_allocation->offset = (uint64_t)ptr; 
    out_allocation->size = size;
    out_allocation->heap_index = 0;
    out_allocation->flags = flags;
    out_allocation->mapped_ptr = (flags & GPU_ALLOCATION_FLAG_HOST_VISIBLE) ? ptr : NULL;
    out_allocation->backend_handle = ptr;

    return 0;
}

void core_device_memory_free(gpu_allocation_t* allocation) {
    if (!allocation || !allocation->backend_handle) return;

    pthread_mutex_lock(&g_device_memory_ctx.mutex);
    core_allocation_node_t** curr = &g_device_memory_ctx.head;
    while (*curr) {
        if ((*curr)->ptr == allocation->backend_handle) {
            core_allocation_node_t* entry = *curr;
            *curr = entry->next;
            free(entry->ptr);
            free(entry);
            allocation->backend_handle = NULL;
            allocation->offset = 0;
            break;
        }
        curr = &((*curr)->next);
    }
    pthread_mutex_unlock(&g_device_memory_ctx.mutex);
}

void* core_device_memory_map(gpu_allocation_t* allocation) {
    if (!allocation || !(allocation->flags & GPU_ALLOCATION_FLAG_HOST_VISIBLE)) return NULL;
    return allocation->backend_handle;
}

void core_device_memory_unmap(gpu_allocation_t* allocation) {}

int core_device_memory_create(core_device_memory_handle_t* out_handle, const core_device_memory_desc_t* desc) {
    if (out_handle) out_handle->id = 0;
    return 0;
}

void core_device_memory_destroy(core_device_memory_handle_t handle) {}
int core_device_memory_update(core_device_memory_handle_t handle, const void* data, size_t size) { return 0; }
bool core_device_memory_is_valid(core_device_memory_handle_t handle) { return g_device_memory_ctx.initialized; }

int core_device_memory_get_info(core_device_memory_handle_t handle, core_device_memory_info_t* out_info) {
    if (!out_info) return -1;
    out_info->id = 0;
    out_info->flags = 0;
    out_info->initialized = g_device_memory_ctx.initialized;
    return 0;
}

uint32_t core_device_memory_get_count(void) { return 1; }
size_t core_device_memory_get_memory_usage(void) {
    size_t total = 0;
    pthread_mutex_lock(&g_device_memory_ctx.mutex);
    core_allocation_node_t* curr = g_device_memory_ctx.head;
    while (curr) {
        total += curr->size;
        curr = curr->next;
    }
    pthread_mutex_unlock(&g_device_memory_ctx.mutex);
    return total;
}
void core_device_memory_debug_print(void) {}
void core_device_memory_mark_dirty(core_device_memory_handle_t handle) {}
int core_device_memory_process_pending(void) { return 0; }
