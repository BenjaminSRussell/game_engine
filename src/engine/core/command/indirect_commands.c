/*
 * indirect_commands.c
 * Indirect draw/dispatch command building
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#include "core/command/indirect_commands.h"
#include "core/logging/unified_logger.h"
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
#define CORE_INDIRECT_LOG_CAT LOG_CAT_RENDERER

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
    void* gpu_resource; // Placeholder for GPU integration
} core_indirect_commands_internal_t;

typedef struct core_indirect_commands_context {
    core_indirect_commands_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;

    // Thread safety
    Mutex* mutex;

    // Statistics
    core_indirect_commands_stats_t stats;
} core_indirect_commands_context_t;

static core_indirect_commands_context_t g_indirect_commands_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool core_indirect_commands_validate(const core_indirect_commands_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    // Add more validation logic here, e.g. checking data alignment
    if (item->data_size > 0 && item->data == NULL) return false;
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
    item->dirty = false;
    item->gpu_resource = NULL;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int core_indirect_commands_init(void) {
    if (g_indirect_commands_ctx.initialized) {
        LOG_WARN(CORE_INDIRECT_LOG_CAT, "Indirect commands system already initialized");
        return CORE_INDIRECT_SUCCESS;
    }

    g_indirect_commands_ctx.capacity = CORE_INDIRECT_COMMANDS_DEFAULT_CAPACITY;
    g_indirect_commands_ctx.items = calloc(g_indirect_commands_ctx.capacity, sizeof(core_indirect_commands_internal_t));
    if (!g_indirect_commands_ctx.items) {
        LOG_ERROR(CORE_INDIRECT_LOG_CAT, "Failed to allocate memory for indirect commands");
        return CORE_INDIRECT_ERROR_OUT_OF_MEMORY;
    }

    g_indirect_commands_ctx.mutex = mutex_create(false, "IndirectCommandsMutex");
    if (!g_indirect_commands_ctx.mutex) {
        free(g_indirect_commands_ctx.items);
        g_indirect_commands_ctx.items = NULL;
        LOG_ERROR(CORE_INDIRECT_LOG_CAT, "Failed to create mutex for indirect commands");
        return CORE_INDIRECT_ERROR_INTERNAL;
    }

    g_indirect_commands_ctx.count = 0;
    memset(&g_indirect_commands_ctx.stats, 0, sizeof(g_indirect_commands_ctx.stats));
    g_indirect_commands_ctx.initialized = true;

    LOG_INFO(CORE_INDIRECT_LOG_CAT, "Indirect commands system initialized");
    return CORE_INDIRECT_SUCCESS;
}

void core_indirect_commands_shutdown(void) {
    if (!g_indirect_commands_ctx.initialized) {
        return;
    }

    // We don't need to lock here assuming shutdown is single-threaded and final
    for (uint32_t i = 0; i < g_indirect_commands_ctx.count; i++) {
        core_indirect_commands_cleanup_internal(&g_indirect_commands_ctx.items[i]);
    }

    free(g_indirect_commands_ctx.items);
    g_indirect_commands_ctx.items = NULL;

    if (g_indirect_commands_ctx.mutex) {
        mutex_destroy(g_indirect_commands_ctx.mutex);
        g_indirect_commands_ctx.mutex = NULL;
    }

    g_indirect_commands_ctx.count = 0;
    g_indirect_commands_ctx.capacity = 0;
    g_indirect_commands_ctx.initialized = false;

    LOG_INFO(CORE_INDIRECT_LOG_CAT, "Indirect commands system shutdown");
}

int core_indirect_commands_create(core_indirect_commands_handle_t* out_handle, const core_indirect_commands_desc_t* desc) {
    if (!out_handle || !desc) {
        return CORE_INDIRECT_ERROR_INVALID_ARGS;
    }

    if (!g_indirect_commands_ctx.initialized) {
        return CORE_INDIRECT_ERROR_NOT_INITIALIZED;
    }

    mutex_lock(g_indirect_commands_ctx.mutex);

    // Find a free slot
    int free_index = -1;
    for (uint32_t i = 0; i < g_indirect_commands_ctx.count; i++) {
        if (!g_indirect_commands_ctx.items[i].initialized) {
            free_index = (int)i;
            break;
        }
    }

    if (free_index == -1) {
        // No free slot found, check capacity
        if (g_indirect_commands_ctx.count >= g_indirect_commands_ctx.capacity) {
            uint32_t new_capacity = g_indirect_commands_ctx.capacity * 2;
            if (new_capacity > CORE_INDIRECT_COMMANDS_MAX_COUNT) {
                 // Try to stick to max count but check if we really are full
                 if (g_indirect_commands_ctx.count >= CORE_INDIRECT_COMMANDS_MAX_COUNT) {
                     mutex_unlock(g_indirect_commands_ctx.mutex);
                     LOG_ERROR(CORE_INDIRECT_LOG_CAT, "Indirect commands capacity reached");
                     return CORE_INDIRECT_ERROR_OUT_OF_MEMORY;
                 }
                 new_capacity = CORE_INDIRECT_COMMANDS_MAX_COUNT;
            }

            core_indirect_commands_internal_t* new_items = realloc(g_indirect_commands_ctx.items, new_capacity * sizeof(core_indirect_commands_internal_t));
            if (!new_items) {
                mutex_unlock(g_indirect_commands_ctx.mutex);
                return CORE_INDIRECT_ERROR_OUT_OF_MEMORY;
            }
            g_indirect_commands_ctx.items = new_items;

            // Initialize new items
            memset(&g_indirect_commands_ctx.items[g_indirect_commands_ctx.capacity], 0,
                   (new_capacity - g_indirect_commands_ctx.capacity) * sizeof(core_indirect_commands_internal_t));

            g_indirect_commands_ctx.capacity = new_capacity;
        }
        free_index = (int)g_indirect_commands_ctx.count++;
    }

    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[free_index];

    item->id = (uint32_t)free_index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->lod_level = desc->lod_level;
    item->gpu_resource = NULL;

    out_handle->id = (uint32_t)free_index;

    g_indirect_commands_ctx.stats.total_created++;
    g_indirect_commands_ctx.stats.active_commands++;

    mutex_unlock(g_indirect_commands_ctx.mutex);

    return CORE_INDIRECT_SUCCESS;
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
        g_indirect_commands_ctx.stats.memory_usage -= item->data_size;
        core_indirect_commands_cleanup_internal(item);
        g_indirect_commands_ctx.stats.active_commands--;
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
}

int core_indirect_commands_update(core_indirect_commands_handle_t handle, const void* data, size_t size) {
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_ERROR_NOT_INITIALIZED;
    if (!data && size > 0) return CORE_INDIRECT_ERROR_INVALID_ARGS;

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (handle.id >= g_indirect_commands_ctx.count) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_HANDLE;
    }

    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];
    if (!item->initialized) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_HANDLE;
    }

    // Resize if necessary
    if (item->data_size != size) {
        void* new_data = realloc(item->data, size);
        if (!new_data && size > 0) {
            mutex_unlock(g_indirect_commands_ctx.mutex);
            return CORE_INDIRECT_ERROR_OUT_OF_MEMORY;
        }
        item->data = new_data;
        g_indirect_commands_ctx.stats.memory_usage = g_indirect_commands_ctx.stats.memory_usage - item->data_size + size;
        item->data_size = size;
    }

    if (size > 0) {
        memcpy(item->data, data, size);
    }

    item->dirty = true;
    g_indirect_commands_ctx.stats.total_updates++;

    if (!core_indirect_commands_validate(item)) {
        g_indirect_commands_ctx.stats.validation_failures++;
        LOG_WARN(CORE_INDIRECT_LOG_CAT, "Validation failed for indirect command %u", handle.id);
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_SUCCESS;
}

int core_indirect_commands_update_partial(core_indirect_commands_handle_t handle, uint32_t offset, const void* data, size_t size) {
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_ERROR_NOT_INITIALIZED;
    if (!data) return CORE_INDIRECT_ERROR_INVALID_ARGS;

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (handle.id >= g_indirect_commands_ctx.count) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_HANDLE;
    }

    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];
    if (!item->initialized) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_HANDLE;
    }

    if (offset + size > item->data_size) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_BUFFER_TOO_SMALL;
    }

    memcpy((char*)item->data + offset, data, size);
    item->dirty = true;
    g_indirect_commands_ctx.stats.total_updates++;

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_SUCCESS;
}

bool core_indirect_commands_is_valid(core_indirect_commands_handle_t handle) {
    if (!g_indirect_commands_ctx.initialized) return false;

    mutex_lock(g_indirect_commands_ctx.mutex);
    bool valid = false;
    if (handle.id < g_indirect_commands_ctx.count) {
        valid = core_indirect_commands_validate(&g_indirect_commands_ctx.items[handle.id]);
    }
    mutex_unlock(g_indirect_commands_ctx.mutex);
    return valid;
}

int core_indirect_commands_get_info(core_indirect_commands_handle_t handle, core_indirect_commands_info_t* out_info) {
    if (!out_info) return CORE_INDIRECT_ERROR_INVALID_ARGS;
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_ERROR_NOT_INITIALIZED;

    mutex_lock(g_indirect_commands_ctx.mutex);

    if (handle.id >= g_indirect_commands_ctx.count) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_HANDLE;
    }

    const core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->lod_level = item->lod_level;

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_SUCCESS;
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

    mutex_lock(g_indirect_commands_ctx.mutex);

    int processed = 0;
    // Batch processing loop
    // Potential for SIMD optimization here if we were processing raw data arrays directly
    // Since we are iterating over structs, basic loop unrolling could help but compiler usually does it.
    for (uint32_t i = 0; i < g_indirect_commands_ctx.count; i++) {
        core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Placeholder for processing logic (e.g. uploading to GPU)
            // if (item->gpu_resource) update_gpu_resource(...)

            item->dirty = false;
            processed++;
        }
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return processed;
}

int core_indirect_commands_reload(core_indirect_commands_handle_t handle) {
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_ERROR_NOT_INITIALIZED;

    mutex_lock(g_indirect_commands_ctx.mutex);
    if (handle.id >= g_indirect_commands_ctx.count || !g_indirect_commands_ctx.items[handle.id].initialized) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_HANDLE;
    }

    // Force re-validation and dirty
    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];
    if (!core_indirect_commands_validate(item)) {
        LOG_WARN(CORE_INDIRECT_LOG_CAT, "Reload failed validation for command %u", handle.id);
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_ARGS; // Or validation error
    }

    item->dirty = true;
    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_SUCCESS;
}

int core_indirect_commands_serialize(core_indirect_commands_handle_t handle, void* buffer, size_t buffer_size, size_t* out_size) {
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_ERROR_NOT_INITIALIZED;
    if (!out_size) return CORE_INDIRECT_ERROR_INVALID_ARGS;

    mutex_lock(g_indirect_commands_ctx.mutex);
    if (handle.id >= g_indirect_commands_ctx.count || !g_indirect_commands_ctx.items[handle.id].initialized) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_INVALID_HANDLE;
    }

    core_indirect_commands_internal_t* item = &g_indirect_commands_ctx.items[handle.id];

    // Format: [id:4][flags:4][lod:4][size:8][data...]
    size_t needed = sizeof(uint32_t) * 3 + sizeof(size_t) + item->data_size;
    *out_size = needed;

    if (buffer == NULL) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_SUCCESS; // Just querying size
    }

    if (buffer_size < needed) {
        mutex_unlock(g_indirect_commands_ctx.mutex);
        return CORE_INDIRECT_ERROR_BUFFER_TOO_SMALL;
    }

    uint8_t* ptr = buffer;
    memcpy(ptr, &item->id, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(ptr, &item->flags, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(ptr, &item->lod_level, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(ptr, &item->data_size, sizeof(size_t)); ptr += sizeof(size_t);
    if (item->data_size > 0) {
        memcpy(ptr, item->data, item->data_size);
    }

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return CORE_INDIRECT_SUCCESS;
}

int core_indirect_commands_deserialize(const void* buffer, size_t buffer_size, core_indirect_commands_handle_t* out_handle) {
    if (!g_indirect_commands_ctx.initialized) return CORE_INDIRECT_ERROR_NOT_INITIALIZED;
    if (!buffer || !out_handle) return CORE_INDIRECT_ERROR_INVALID_ARGS;

    // Header check
    if (buffer_size < sizeof(uint32_t) * 3 + sizeof(size_t)) {
        return CORE_INDIRECT_ERROR_INVALID_ARGS;
    }

    const uint8_t* ptr = buffer;
    uint32_t id_dummy;
    uint32_t flags;
    uint32_t lod;
    size_t data_size;

    memcpy(&id_dummy, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(&flags, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(&lod, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(&data_size, ptr, sizeof(size_t)); ptr += sizeof(size_t);

    if (buffer_size < (ptr - (const uint8_t*)buffer) + data_size) {
        return CORE_INDIRECT_ERROR_INVALID_ARGS;
    }

    core_indirect_commands_desc_t desc = {
        .flags = flags,
        .lod_level = lod,
        .user_data = NULL
    };

    core_indirect_commands_handle_t handle;
    int res = core_indirect_commands_create(&handle, &desc);
    if (res != CORE_INDIRECT_SUCCESS) {
        return res;
    }

    if (data_size > 0) {
        res = core_indirect_commands_update(handle, ptr, data_size);
        if (res != CORE_INDIRECT_SUCCESS) {
            core_indirect_commands_destroy(handle);
            return res;
        }
    }

    *out_handle = handle;
    return CORE_INDIRECT_SUCCESS;
}

uint32_t core_indirect_commands_get_count(void) {
    if (!g_indirect_commands_ctx.initialized) return 0;
    // Reads of primitive types are atomic on most platforms, but strictly for thread safety we should lock or use atomics.
    // Locking just to read a count might be heavy, but let's be safe.
    mutex_lock(g_indirect_commands_ctx.mutex);
    uint32_t count = g_indirect_commands_ctx.count;
    mutex_unlock(g_indirect_commands_ctx.mutex);
    return count;
}

size_t core_indirect_commands_get_memory_usage(void) {
    if (!g_indirect_commands_ctx.initialized) return 0;

    mutex_lock(g_indirect_commands_ctx.mutex);

    size_t total = sizeof(g_indirect_commands_ctx);
    total += g_indirect_commands_ctx.capacity * sizeof(core_indirect_commands_internal_t);
    total += g_indirect_commands_ctx.stats.memory_usage;

    mutex_unlock(g_indirect_commands_ctx.mutex);
    return total;
}

void core_indirect_commands_get_stats(core_indirect_commands_stats_t* out_stats) {
    if (!out_stats || !g_indirect_commands_ctx.initialized) return;

    mutex_lock(g_indirect_commands_ctx.mutex);
    *out_stats = g_indirect_commands_ctx.stats;
    mutex_unlock(g_indirect_commands_ctx.mutex);
}

void core_indirect_commands_debug_print(void) {
    if (!g_indirect_commands_ctx.initialized) return;

    core_indirect_commands_stats_t stats;
    core_indirect_commands_get_stats(&stats);

    LOG_INFO(CORE_INDIRECT_LOG_CAT, "Indirect Commands Stats:");
    LOG_INFO(CORE_INDIRECT_LOG_CAT, "  Active Commands: %u", stats.active_commands);
    LOG_INFO(CORE_INDIRECT_LOG_CAT, "  Total Created: %u", stats.total_created);
    LOG_INFO(CORE_INDIRECT_LOG_CAT, "  Total Updates: %u", stats.total_updates);
    LOG_INFO(CORE_INDIRECT_LOG_CAT, "  Memory Usage: %zu bytes", stats.memory_usage);
}
