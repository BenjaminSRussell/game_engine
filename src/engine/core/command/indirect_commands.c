/*
 * indirect_commands.c
 * Indirect draw/dispatch command building
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#include "core/command/indirect_commands.h"
#include "core/logger/unified_logger.h"
#include "core/threading/mutex.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CORE_INDIRECT_COMMANDS_MAX_COUNT 4096
#define CORE_INDIRECT_COMMANDS_DEFAULT_CAPACITY 256
#define CORE_INDIRECT_COMMANDS_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum {
    CORE_INDIRECT_STATE_UNINITIALIZED = 0,
    CORE_INDIRECT_STATE_READY,
    CORE_INDIRECT_STATE_RECORDING,
    CORE_INDIRECT_STATE_EXECUTING,
    CORE_INDIRECT_STATE_INVALID
} core_indirect_state_t;

typedef struct core_indirect_commands_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    core_indirect_state_t state;
} core_indirect_commands_internal_t;

typedef struct core_indirect_commands_context {
    core_indirect_commands_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    Mutex* mutex;
    bool initialized;
    size_t total_memory_usage;
} core_indirect_commands_context_t;

static core_indirect_commands_context_t g_indirect_commands_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool core_indirect_commands_validate(const core_indirect_commands_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->state == CORE_INDIRECT_STATE_INVALID) return false;
    return true;
}

static void core_indirect_commands_cleanup_internal(core_indirect_commands_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->data_size = 0;
    item->initialized = false;
    item->state = CORE_INDIRECT_STATE_UNINITIALIZED;
}

// Simulated validation layer check
static void core_indirect_commands_validation_check(const core_indirect_commands_internal_t* item, const char* operation) {
    if (!item) {
        LOG_GRAPHICS_ERROR("Validation Layer: Null item in operation %s", operation);
        return;
    }
    if (item->state == CORE_INDIRECT_STATE_INVALID) {
        LOG_GRAPHICS_ERROR("Validation Layer: Invalid state for item %u in operation %s", item->id, operation);
    }
    // Additional validation checks could go here
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int core_indirect_commands_init(void) {
    if (g_indirect_commands_ctx.initialized) {
        LOG_GRAPHICS_WARN("Indirect commands system already initialized");
        return CORE_INDIRECT_ERROR_NONE;
    }

    g_indirect_commands_ctx.mutex = mutex_create(false, "IndirectCommandsMutex");
    if (!g_indirect_commands_ctx.mutex) {
        LOG_GRAPHICS_FATAL("Failed to create mutex for indirect commands");
        return CORE_INDIRECT_ERROR_INVALID_STATE;
    }

    g_indirect_commands_ctx.capacity = CORE_INDIRECT_COMMANDS_DEFAULT_CAPACITY;
    g_indirect_commands_ctx.items = calloc(g_indirect_commands_ctx.capacity, sizeof(core_indirect_commands_internal_t));
    if (!g_indirect_commands_ctx.items) {
        mutex_destroy(g_indirect_commands_ctx.mutex);
        LOG_GRAPHICS_FATAL("Failed to allocate memory for indirect commands");
        return CORE_INDIRECT_ERROR_OUT_OF_MEMORY;
    }

    g_indirect_commands_ctx.count = 0;
    g_indirect_commands_ctx.total_memory_usage = g_indirect_commands_ctx.capacity * sizeof(core_indirect_commands_internal_t);
    g_indirect_commands_ctx.initialized = true;

    LOG_GRAPHICS_INFO("Indirect commands system initialized");
    return CORE_INDIRECT_ERROR_NONE;
}

void core_indirect_commands_shutdown(void) {
    if (!g_indirect_commands_ctx.initialized) {
        return;
    }

    mutex_lock(g_indirect_commands_ctx.mutex);

    for (uint32_t i = 0; i < g_indirect_commands_ctx.count; i++) {
        core_indirect_commands_cleanup_internal(&g_indirect_commands_ctx.items[i]);
    }

    free(g_indirect_commands_ctx.items);
    g_indirect_commands_ctx.items = NULL;
    g_indirect_commands_ctx.count = 0;
    g_indirect_commands_ctx.capacity = 0;
    g_indirect_commands_ctx.total_memory_usage = 0;
    g_indirect_commands_ctx.initialized = false;

    mutex_unlock(g_indirect_commands_ctx.mutex);
    mutex_destroy(g_indirect_commands_ctx.mutex);
    g_indirect_commands_ctx.mutex = NULL;

    LOG_GRAPHICS_INFO("Indirect commands system shutdown");
}

int core_indirect_commands_create(core_indirect_commands_handle_t* out_handle, const core_indirect_commands_desc_t* desc) {
    if (!out_handle || !desc) {
        return CORE_INDIRECT_ERROR_INVALID_ARGS;
    }

    if (!g_indirect_commands_ctx.initialized) {
        return CORE_INDIRECT_ERROR_NOT_INITIALIZED;
    }

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (g_indirect_commands_ctx.count >= g_indirect_commands_ctx.capacity) {
        // Simple growth strategy
        uint32_t new_capacity = g_indirect_commands_ctx.capacity * 2;
        if (new_capacity > CORE_INDIRECT_COMMANDS_MAX_COUNT) {
             mutex_unlock(g_indirect_commands_ctx.mutex);
             LOG_GRAPHICS_ERROR("Max indirect commands count reached");
             return CORE_INDIRECT_ERROR_OUT_OF_MEMORY;
        }

        core_indirect_commands_internal_t* new_items = realloc(g_indirect_commands_ctx.items, new_capacity * sizeof(core_indirect_commands_internal_t));
        if (!new_items) {
            mutex_unlock(g_indirect_commands_ctx.mutex);
            return CORE_INDIRECT_ERROR_OUT_OF_MEMORY;
        }

        // Zero out new memory
        memset(new_items + g_indirect_commands_ctx.capacity, 0, (new_capacity - g_indirect_commands_ctx.capacity) * sizeof(core_indirect_commands_internal_t));

        g_indirect_commands_ctx.items = new_items;
        g_indirect_commands_ctx.capacity = new_capacity;
        g_indirect_commands_ctx.total_memory_usage = g_indirect_commands_ctx.capacity * sizeof(core_indirect_commands_internal_t);
        // Add existing data sizes
        for(uint32_t i=0; i<g_indirect_commands_ctx.count; ++i) {
             g_indirect_commands_ctx.total_memory_usage += g_indirect_commands_ctx.items[i].data_size;
        }
    }

    uint32_t index = g_indirect_commands_ctx.count++;
    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->state = CORE_INDIRECT_STATE_READY;

    out_handle->id = index;

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_ERROR_NONE;
}

void core_indirect_commands_destroy(core_indirect_commands_handle_t handle) {
    if (!g_indirect_commands_ctx.initialized) return;

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (handle.id >= g_indirect_commands_ctx.count) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return;
    }

    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];
    if (item->initialized) {
        g_indirect_commands_ctx.total_memory_usage -= item->data_size;
        core_indirect_commands_cleanup_internal(item);
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
}

int core_indirect_commands_update(core_indirect_commands_handle_t handle, const void* data, size_t size) {
    if (!g_indirect_commands_ctx.initialized) {
        return CORE_INDIRECT_ERROR_NOT_INITIALIZED;
    }

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (handle.id >= g_indirect_commands_ctx.count) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_HANDLE;
    }

    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];
    if (!item->initialized) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_STATE;
    }

    // Validation layer integration
    core_indirect_commands_validation_check(item, "update");

    // Internal validation
    if (!core_indirect_commands_validate(item)) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_STATE;
    }

    if (item->data_size != size) {
        void* new_data = realloc(item->data, size);
        if (!new_data && size > 0) {
            mutex_unlock(g_indirect_commands_ctx.mutex);
            return CORE_INDIRECT_ERROR_OUT_OF_MEMORY;
        }
        item->data = new_data;
        g_indirect_commands_ctx.total_memory_usage -= item->data_size;
        g_indirect_commands_ctx.total_memory_usage += size;
        item->data_size = size;
    }

    if (data && size > 0) {
        memcpy(item->data, data, size);
    }

    item->dirty = true;
    item->state = CORE_INDIRECT_STATE_RECORDING; // State transition

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_ERROR_NONE;
}

bool core_indirect_commands_is_valid(core_indirect_commands_handle_t handle) {
    if (!g_indirect_commands_ctx.initialized) return false;

    // Quick check without lock for atomic-like fields if possible, but safe with lock
    mutex_lock(g_indirect_commands_ctx.mutex);
    if (handle.id >= g_indirect_commands_ctx.count) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return false;
    }
    bool result = g_indirect_commands_ctx.items[handle.id].initialized;
    mutex_unlock(g_indirect_commands_ctx.mutex);
    return result;
}

int core_indirect_commands_get_info(core_indirect_commands_handle_t handle, core_indirect_commands_info_t* out_info) {
    if (!out_info) {
        return CORE_INDIRECT_ERROR_INVALID_ARGS;
    }

    if (!g_indirect_commands_ctx.initialized) {
        return CORE_INDIRECT_ERROR_NOT_INITIALIZED;
    }

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (handle.id >= g_indirect_commands_ctx.count) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_HANDLE;
    }

    const core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_ERROR_NONE;
}

void core_indirect_commands_mark_dirty(core_indirect_commands_handle_t handle) {
    if (!g_indirect_commands_ctx.initialized) return;

    mutex_lock(g_indirect_commands_ctx.mutex);
    if (handle.id < g_indirect_commands_ctx.count) {
        g_indirect_commands_ctx.items[handle.id].dirty = true;
    }
    mutex_unlock(g_indirect_commands_ctx.mutex);
}

int core_indirect_commands_process_pending(void) {
    if (!g_indirect_commands_ctx.initialized) return 0;

    int processed = 0;
    mutex_lock(g_indirect_commands_ctx.mutex);

    for (uint32_t i = 0; i < g_indirect_commands_ctx.count; i++) {
        core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            // Logic to sync with GPU or prepare buffer would go here
            item->dirty = false;
            item->state = CORE_INDIRECT_STATE_READY;
            processed++;
        }
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return processed;
}

int core_indirect_commands_reload(core_indirect_commands_handle_t handle) {
    // Hot-reload support: Mark as dirty and force reprocessing/reupload
    if (!g_indirect_commands_ctx.initialized) {
        return CORE_INDIRECT_ERROR_NOT_INITIALIZED;
    }

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (handle.id >= g_indirect_commands_ctx.count) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_HANDLE;
    }

    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];
    if (!item->initialized) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_STATE;
    }

    item->dirty = true;
    LOG_GRAPHICS_INFO("Hot-reloaded indirect commands handle %u", handle.id);

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_ERROR_NONE;
}

uint32_t core_indirect_commands_get_count(void) {
    if (!g_indirect_commands_ctx.initialized) return 0;

    // mutex_lock(g_indirect_commands_ctx.mutex); // Reading a uint32 might be atomic enough but let's be safe if count changes
    // But since it's just a getter, and typically accessed from main thread or just for stats
    // We can assume eventual consistency or lock. Let's lock.

    // Warning: recursive lock if called from within another locked function?
    // The current mutex implementation is non-recursive by default in init(false).
    // All internal calls seem to avoid calling public APIs that lock.
    // So this should be fine.

    mutex_lock(g_indirect_commands_ctx.mutex);
    uint32_t count = g_indirect_commands_ctx.count;
    mutex_unlock(g_indirect_commands_ctx.mutex);
    return count;
}

size_t core_indirect_commands_get_memory_usage(void) {
    if (!g_indirect_commands_ctx.initialized) return 0;

    mutex_lock(g_indirect_commands_ctx.mutex);
    size_t usage = g_indirect_commands_ctx.total_memory_usage;
    mutex_unlock(g_indirect_commands_ctx.mutex);
    return usage;
}

void core_indirect_commands_debug_print(void) {
    if (!g_indirect_commands_ctx.initialized) return;

    mutex_lock(g_indirect_commands_ctx.mutex);

    LOG_GRAPHICS_INFO("Indirect Commands Debug Info:");
    LOG_GRAPHICS_INFO("  Count: %u", g_indirect_commands_ctx.count);
    LOG_GRAPHICS_INFO("  Capacity: %u", g_indirect_commands_ctx.capacity);
    LOG_GRAPHICS_INFO("  Memory Usage: %zu bytes", g_indirect_commands_ctx.total_memory_usage);

    for (uint32_t i = 0; i < g_indirect_commands_ctx.count; i++) {
        const core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[i];
        if (item->initialized) {
            LOG_GRAPHICS_INFO("  Item %u: Size=%zu, Dirty=%s, State=%d",
                item->id, item->data_size, item->dirty ? "Yes" : "No", item->state);
        }
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
}

/* End of indirect_commands.c */
