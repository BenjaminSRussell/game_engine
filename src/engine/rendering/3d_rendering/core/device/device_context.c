#include "device_context.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

#define CORE_DEVICE_CONTEXT_DEFAULT_CAPACITY 16

typedef struct core_device_context_internal {
    render_device_t* devices;
    uint32_t count;
    uint32_t capacity;
    pthread_mutex_t mutex;
    bool initialized;
} core_device_context_internal_t;

static core_device_context_internal_t g_device_ctx = {0};

/* ============================================================================
 * API
 * ============================================================================ */

int core_device_context_init(void) {
    if (g_device_ctx.initialized) return 0;

    pthread_mutex_init(&g_device_ctx.mutex, NULL);
    g_device_ctx.capacity = CORE_DEVICE_CONTEXT_DEFAULT_CAPACITY;
    g_device_ctx.devices = calloc(g_device_ctx.capacity, sizeof(render_device_t));
    if (!g_device_ctx.devices) return -1;

    g_device_ctx.count = 0;
    g_device_ctx.initialized = true;
    return 0;
}

void core_device_context_shutdown(void) {
    if (!g_device_ctx.initialized) return;

    pthread_mutex_lock(&g_device_ctx.mutex);
    free(g_device_ctx.devices);
    g_device_ctx.devices = NULL;
    g_device_ctx.count = 0;
    g_device_ctx.capacity = 0;
    pthread_mutex_unlock(&g_device_ctx.mutex);
    
    pthread_mutex_destroy(&g_device_ctx.mutex);
    g_device_ctx.initialized = false;
}

int core_device_context_create(core_device_context_handle_t* out_handle, const core_device_context_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_device_ctx.initialized) return -2;

    pthread_mutex_lock(&g_device_ctx.mutex);
    if (g_device_ctx.count >= g_device_ctx.capacity) {
        pthread_mutex_unlock(&g_device_ctx.mutex);
        return -3;
    }

    uint32_t index = g_device_ctx.count++;
    render_device_t* device = &g_device_ctx.devices[index];
    memset(device, 0, sizeof(render_device_t));
    
    // Placeholder handle
    device->backend_handle = (void*)(uintptr_t)(index + 1);
    
    pthread_mutex_unlock(&g_device_ctx.mutex);

    out_handle->id = index;
    return 0;
}

void core_device_context_destroy(core_device_context_handle_t handle) {
    if (!g_device_ctx.initialized || handle.id >= g_device_ctx.count) return;
    // Logical destroy - currently just ignoring it as we don't have complex backend handles yet
}

int core_device_context_update(core_device_context_handle_t handle, const void* data, size_t size) { return 0; }
bool core_device_context_is_valid(core_device_context_handle_t handle) {
    return g_device_ctx.initialized && handle.id < g_device_ctx.count;
}

int core_device_context_get_info(core_device_context_handle_t handle, core_device_context_info_t* out_info) {
    if (!out_info || handle.id >= g_device_ctx.count) return -1;
    out_info->id = handle.id;
    out_info->initialized = g_device_ctx.initialized;
    return 0;
}

void core_device_context_mark_dirty(core_device_context_handle_t handle) {}
int core_device_context_process_pending(void) { return 0; }

uint32_t core_device_context_get_count(void) { return g_device_ctx.count; }
size_t core_device_context_get_memory_usage(void) {
    return g_device_ctx.capacity * sizeof(render_device_t);
}
void core_device_context_debug_print(void) {}
