/*
 * indirect_commands.c
 * Indirect draw/dispatch command building
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#include "core/command/indirect_commands.h"
#include "core/threading/mutex.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CORE_INDIRECT_COMMANDS_MAX_COUNT 4096
#define CORE_INDIRECT_COMMANDS_DEFAULT_CAPACITY 256
#define CORE_INDIRECT_COMMANDS_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_indirect_commands_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    uint32_t lod_level;
    bool visible;
    void* user_data;
} core_indirect_commands_internal_t;

typedef struct core_indirect_commands_context {
    core_indirect_commands_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
    Mutex* mutex;
    size_t total_allocated_memory;
} core_indirect_commands_context_t;

static core_indirect_commands_context_t g_indirect_commands_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void backend_vulkan_process(core_indirect_commands_internal_t* item) {
    /* TODO-30525: Implement Vulkan backend logic */
    (void)item;
}

static void backend_metal_process(core_indirect_commands_internal_t* item) {
    /* TODO-30526: Implement Metal backend logic */
    (void)item;
}

static void backend_d3d12_process(core_indirect_commands_internal_t* item) {
    /* TODO-30527: Implement D3D12 backend logic */
    (void)item;
}

static bool core_indirect_commands_validate(const core_indirect_commands_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void core_indirect_commands_cleanup_internal(core_indirect_commands_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        g_indirect_commands_ctx.total_allocated_memory -= item->data_size;
        item->data = NULL;
    }
    item->data_size = 0;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int core_indirect_commands_init(void) {
    if (g_indirect_commands_ctx.initialized) {
        return CORE_INDIRECT_COMMANDS_SUCCESS;
    }

    g_indirect_commands_ctx.capacity = CORE_INDIRECT_COMMANDS_DEFAULT_CAPACITY;
    g_indirect_commands_ctx.items = calloc(g_indirect_commands_ctx.capacity, sizeof(core_indirect_commands_internal_t));
    if (!g_indirect_commands_ctx.items) {
        return CORE_INDIRECT_COMMANDS_ERROR_OUT_OF_MEMORY;
    }

    g_indirect_commands_ctx.mutex = mutex_create(false, "IndirectCommandsMutex");
    if (!g_indirect_commands_ctx.mutex) {
        free(g_indirect_commands_ctx.items);
        return CORE_INDIRECT_COMMANDS_ERROR_BACKEND_FAILURE;
    }

    g_indirect_commands_ctx.count = 0;
    g_indirect_commands_ctx.total_allocated_memory = sizeof(core_indirect_commands_internal_t) * g_indirect_commands_ctx.capacity;
    g_indirect_commands_ctx.initialized = true;

    return CORE_INDIRECT_COMMANDS_SUCCESS;
}

void core_indirect_commands_shutdown(void) {
    if (!g_indirect_commands_ctx.initialized) {
        return;
    }

    mutex_lock(g_indirect_commands_ctx.mutex);

    for (uint32_t i = 0; i < g_indirect_commands_ctx.capacity; i++) {
        if (g_indirect_commands_ctx.items[i].initialized) {
            core_indirect_commands_cleanup_internal(&g_indirect_commands_ctx.items[i]);
        }
    }

    free(g_indirect_commands_ctx.items);
    g_indirect_commands_ctx.items = NULL;
    g_indirect_commands_ctx.count = 0;
    g_indirect_commands_ctx.capacity = 0;
    g_indirect_commands_ctx.total_allocated_memory = 0;
    g_indirect_commands_ctx.initialized = false;

    mutex_unlock(g_indirect_commands_ctx.mutex);
    mutex_destroy(g_indirect_commands_ctx.mutex);
    g_indirect_commands_ctx.mutex = NULL;
}

int core_indirect_commands_create(core_indirect_commands_handle_t* out_handle, const core_indirect_commands_desc_t* desc) {
    if (!out_handle || !desc) {
        return CORE_INDIRECT_COMMANDS_ERROR_INVALID_ARGS;
    }

    if (!g_indirect_commands_ctx.initialized) {
        return CORE_INDIRECT_COMMANDS_ERROR_NOT_INITIALIZED;
    }

    mutex_lock(g_indirect_commands_ctx.mutex);

    // Find a free slot
    uint32_t index = 0xFFFFFFFF;

    // First, try linearly if there is space before capacity
    if (g_indirect_commands_ctx.count < g_indirect_commands_ctx.capacity) {
        // Simple search for uninitialized
        // Optimization: We could track the first free index, but for now linear scan is acceptable given the constraints.
        // Or since we just incremented count in the previous version, we need to respect that 'count' is tracking # of active items,
        // but we need to find WHERE to put it.
        // Actually, if we use a free-list or just scan, we don't need 'count' to be the high-water mark.
        // Let's iterate to find a free slot.
        for (uint32_t i = 0; i < g_indirect_commands_ctx.capacity; i++) {
            if (!g_indirect_commands_ctx.items[i].initialized) {
                index = i;
                break;
            }
        }
    }

    if (index == 0xFFFFFFFF) {
        // Expand if possible?
        // Current implementation is fixed capacity.
        // If we didn't find a spot, we are full.
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_COMMANDS_ERROR_CAPACITY_REACHED;
    }

    g_indirect_commands_ctx.count++;
    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;

    if (desc->initial_capacity > 0) {
        item->data = malloc(desc->initial_capacity);
        if (item->data) {
            item->data_size = desc->initial_capacity;
            g_indirect_commands_ctx.total_allocated_memory += desc->initial_capacity;
            memset(item->data, 0, desc->initial_capacity);
        }
    }

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->lod_level = 0;
    item->visible = true;
    item->user_data = desc->user_data;

    out_handle->id = index;

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_COMMANDS_SUCCESS;
}

void core_indirect_commands_destroy(core_indirect_commands_handle_t handle) {
    if (!g_indirect_commands_ctx.initialized) return;

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (handle.id < g_indirect_commands_ctx.capacity) {
        if (g_indirect_commands_ctx.items[handle.id].initialized) {
            core_indirect_commands_cleanup_internal(&g_indirect_commands_ctx.items[handle.id]);
            if (g_indirect_commands_ctx.count > 0) {
                g_indirect_commands_ctx.count--;
            }
        }
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
}

int core_indirect_commands_update(core_indirect_commands_handle_t handle, const void* data, size_t size) {
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_COMMANDS_ERROR_NOT_INITIALIZED;

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (handle.id >= g_indirect_commands_ctx.capacity) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_COMMANDS_ERROR_INVALID_HANDLE;
    }

    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];
    if (!item->initialized) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_COMMANDS_ERROR_INVALID_HANDLE;
    }

    // Handle Streaming Flag
    if (item->flags & CORE_INDIRECT_COMMANDS_FLAG_STREAMING) {
        // For streaming, we might not want to keep the data resident all the time,
        // or we handle it differently. For now, we update it as usual.
        // In a real implementation, this might trigger a DMA transfer.
    }

    // Reallocate if size changes or not allocated
    if (item->data_size != size) {
        void* new_data = realloc(item->data, size);
        if (!new_data && size > 0) {
            mutex_unlock(g_indirect_commands_ctx.mutex);
            return CORE_INDIRECT_COMMANDS_ERROR_OUT_OF_MEMORY;
        }
        item->data = new_data;
        g_indirect_commands_ctx.total_allocated_memory -= item->data_size;
        g_indirect_commands_ctx.total_allocated_memory += size;
        item->data_size = size;
    }

    if (data && size > 0) {
        memcpy(item->data, data, size);
    }

    item->dirty = true;

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_COMMANDS_SUCCESS;
}

bool core_indirect_commands_is_valid(core_indirect_commands_handle_t handle) {
    if (!g_indirect_commands_ctx.initialized) return false;

    if (handle.id >= g_indirect_commands_ctx.capacity) {
        return false;
    }

    // We should ideally lock, but simple bool check is often done unlocked for perf in game engines,
    // assuming external sync for destruction. However, since we are adding thread safety, let's respect it if possible.
    // But this function is often used in tight loops. I will keep it lock-free for the read,
    // acknowledging a potential race if destroyed concurrently.
    return g_indirect_commands_ctx.items[handle.id].initialized;
}

int core_indirect_commands_get_info(core_indirect_commands_handle_t handle, core_indirect_commands_info_t* out_info) {
    if (!out_info) return CORE_INDIRECT_COMMANDS_ERROR_INVALID_ARGS;
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_COMMANDS_ERROR_NOT_INITIALIZED;

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (handle.id >= g_indirect_commands_ctx.capacity) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_COMMANDS_ERROR_INVALID_HANDLE;
    }

    const core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];
    if (!item->initialized) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_COMMANDS_ERROR_INVALID_HANDLE;
    }

    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->lod_level = item->lod_level;
    out_info->visible = item->visible;
    out_info->memory_usage = item->data_size;

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_COMMANDS_SUCCESS;
}

void core_indirect_commands_mark_dirty(core_indirect_commands_handle_t handle) {
    if (!g_indirect_commands_ctx.initialized) return;

    mutex_lock(g_indirect_commands_ctx.mutex);
    if (handle.id < g_indirect_commands_ctx.capacity) {
        if (g_indirect_commands_ctx.items[handle.id].initialized) {
            g_indirect_commands_ctx.items[handle.id].dirty = true;
        }
    }
    mutex_unlock(g_indirect_commands_ctx.mutex);
}

int core_indirect_commands_set_lod(core_indirect_commands_handle_t handle, uint32_t lod_level) {
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_COMMANDS_ERROR_NOT_INITIALIZED;

    mutex_lock(g_indirect_commands_ctx.mutex);
    if (handle.id >= g_indirect_commands_ctx.capacity || !g_indirect_commands_ctx.items[handle.id].initialized) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_COMMANDS_ERROR_INVALID_HANDLE;
    }

    if (g_indirect_commands_ctx.items[handle.id].flags & CORE_INDIRECT_COMMANDS_FLAG_LOD) {
        g_indirect_commands_ctx.items[handle.id].lod_level = lod_level;
        g_indirect_commands_ctx.items[handle.id].dirty = true;
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_COMMANDS_SUCCESS;
}

int core_indirect_commands_set_culling_state(core_indirect_commands_handle_t handle, bool visible) {
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_COMMANDS_ERROR_NOT_INITIALIZED;

    mutex_lock(g_indirect_commands_ctx.mutex);
    if (handle.id >= g_indirect_commands_ctx.capacity || !g_indirect_commands_ctx.items[handle.id].initialized) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_COMMANDS_ERROR_INVALID_HANDLE;
    }

    if (g_indirect_commands_ctx.items[handle.id].flags & CORE_INDIRECT_COMMANDS_FLAG_CULLING) {
        g_indirect_commands_ctx.items[handle.id].visible = visible;
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_COMMANDS_SUCCESS;
}

int core_indirect_commands_process_pending(void) {
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_COMMANDS_ERROR_NOT_INITIALIZED;

    mutex_lock(g_indirect_commands_ctx.mutex);

    int processed = 0;
    // Iterate over all potential items since we are using slot reuse
    for (uint32_t i = 0; i < g_indirect_commands_ctx.capacity; i++) {
        core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[i];

        if (!item->initialized) continue;

        // Skip if culled
        if ((item->flags & CORE_INDIRECT_COMMANDS_FLAG_CULLING) && !item->visible) {
            continue;
        }

        if (item->dirty) {
            // Process item: Batch processing logic.
            // In a real engine, we would check the active renderer backend.
            // For now, we invoke the stubs sequentially as we don't have a backend selector.
            // This ensures all backend paths are exercised (in stub form).
            backend_vulkan_process(item);
            backend_metal_process(item);
            backend_d3d12_process(item);

            item->dirty = false;
            processed++;
        }
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return processed;
}

uint32_t core_indirect_commands_get_count(void) {
    return g_indirect_commands_ctx.count;
}

size_t core_indirect_commands_get_memory_usage(void) {
    if (!g_indirect_commands_ctx.initialized) return 0;
    return g_indirect_commands_ctx.total_allocated_memory;
}

void core_indirect_commands_debug_print(void) {
    if (!g_indirect_commands_ctx.initialized) return;

    mutex_lock(g_indirect_commands_ctx.mutex);
    printf("Indirect Commands Context:\n");
    printf("  Count: %u\n", g_indirect_commands_ctx.count);
    printf("  Capacity: %u\n", g_indirect_commands_ctx.capacity);
    printf("  Memory Usage: %zu bytes\n", g_indirect_commands_ctx.total_allocated_memory);

    for (uint32_t i = 0; i < g_indirect_commands_ctx.capacity; i++) {
        core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[i];
        if (item->initialized) {
            printf("  Item %u: Flags=%u, DataSize=%zu, LOD=%u, Visible=%d\n",
                   item->id, item->flags, item->data_size, item->lod_level, item->visible);
        }
    }
    mutex_unlock(g_indirect_commands_ctx.mutex);
}

/* End of indirect_commands.c */
