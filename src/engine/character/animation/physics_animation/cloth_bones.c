/*
 * cloth_bones.c
 * Cloth bone simulation
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#include "character/animation/physics_animation/cloth_bones.h"
#include <core/threading.h>
#include <core/logger.h>
#include <core/performance.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_CLOTH_BONES_MAX_COUNT 4096
#define ANIMATION_CLOTH_BONES_DEFAULT_CAPACITY 256
#define ANIMATION_CLOTH_BONES_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_cloth_bones_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} animation_cloth_bones_internal_t;

typedef struct animation_cloth_bones_context {
    animation_cloth_bones_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    Mutex* lock;
    animation_cloth_bones_stats_t stats;
    bool initialized;
} animation_cloth_bones_context_t;

static animation_cloth_bones_context_t g_cloth_bones_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool animation_cloth_bones_validate(const animation_cloth_bones_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void animation_cloth_bones_cleanup_internal(animation_cloth_bones_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
    item->data_size = 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_cloth_bones_init(void) {
    if (g_cloth_bones_ctx.initialized) {
        LOG_WARN(LOG_CAT_ANIMATION, "Cloth bones system already initialized");
        return 0; // Already initialized
    }

    g_cloth_bones_ctx.lock = mutex_create();
    if (!g_cloth_bones_ctx.lock) {
        LOG_FATAL(LOG_CAT_ANIMATION, "Failed to create mutex for cloth bones system");
        return -1;
    }

    mutex_lock(g_cloth_bones_ctx.lock);

    g_cloth_bones_ctx.capacity = ANIMATION_CLOTH_BONES_DEFAULT_CAPACITY;
    g_cloth_bones_ctx.items = calloc(g_cloth_bones_ctx.capacity, sizeof(animation_cloth_bones_internal_t));
    if (!g_cloth_bones_ctx.items) {
        mutex_unlock(g_cloth_bones_ctx.lock);
        mutex_destroy(g_cloth_bones_ctx.lock);
        g_cloth_bones_ctx.lock = NULL;
        LOG_FATAL(LOG_CAT_ANIMATION, "Failed to allocate memory for cloth bones items");
        return -1;
    }

    g_cloth_bones_ctx.count = 0;

    // Reset stats
    memset(&g_cloth_bones_ctx.stats, 0, sizeof(animation_cloth_bones_stats_t));

    g_cloth_bones_ctx.initialized = true;

    mutex_unlock(g_cloth_bones_ctx.lock);
    LOG_INFO(LOG_CAT_ANIMATION, "Cloth bones system initialized");

    return 0;
}

void animation_cloth_bones_shutdown(void) {
    if (!g_cloth_bones_ctx.initialized) {
        return;
    }

    mutex_lock(g_cloth_bones_ctx.lock);

    for (uint32_t i = 0; i < g_cloth_bones_ctx.count; i++) {
        animation_cloth_bones_cleanup_internal(&g_cloth_bones_ctx.items[i]);
    }

    free(g_cloth_bones_ctx.items);
    g_cloth_bones_ctx.items = NULL;
    g_cloth_bones_ctx.count = 0;
    g_cloth_bones_ctx.capacity = 0;
    g_cloth_bones_ctx.initialized = false;
    memset(&g_cloth_bones_ctx.stats, 0, sizeof(animation_cloth_bones_stats_t));

    mutex_unlock(g_cloth_bones_ctx.lock);
    mutex_destroy(g_cloth_bones_ctx.lock);
    g_cloth_bones_ctx.lock = NULL;

    LOG_INFO(LOG_CAT_ANIMATION, "Cloth bones system shutdown");
}

int animation_cloth_bones_create(animation_cloth_bones_handle_t* out_handle, const animation_cloth_bones_desc_t* desc) {
    if (!out_handle || !desc) {
        LOG_ERROR(LOG_CAT_ANIMATION, "Invalid arguments for cloth bones creation");
        return -1;
    }

    if (!g_cloth_bones_ctx.initialized) {
        LOG_ERROR(LOG_CAT_ANIMATION, "Cloth bones system not initialized");
        return -2;
    }

    mutex_lock(g_cloth_bones_ctx.lock);

    if (g_cloth_bones_ctx.count >= g_cloth_bones_ctx.capacity) {
        // Resize logic could go here, for now failing
        LOG_ERROR(LOG_CAT_ANIMATION, "Cloth bones capacity reached");
        mutex_unlock(g_cloth_bones_ctx.lock);
        return -3;
    }

    uint32_t index = g_cloth_bones_ctx.count++;
    animation_cloth_bones_internal_t* item = &g_cloth_bones_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL; // User data not stored internally yet, might change with deeper implementation
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;

    g_cloth_bones_ctx.stats.active_simulations++;
    g_cloth_bones_ctx.stats.memory_usage = animation_cloth_bones_get_memory_usage();

    mutex_unlock(g_cloth_bones_ctx.lock);

    return 0;
}

void animation_cloth_bones_destroy(animation_cloth_bones_handle_t handle) {
    if (!g_cloth_bones_ctx.initialized) return;

    mutex_lock(g_cloth_bones_ctx.lock);

    if (handle.id >= g_cloth_bones_ctx.count) {
        LOG_WARN(LOG_CAT_ANIMATION, "Attempt to destroy invalid cloth bones handle %u", handle.id);
        mutex_unlock(g_cloth_bones_ctx.lock);
        return;
    }

    if (g_cloth_bones_ctx.items[handle.id].initialized) {
        animation_cloth_bones_cleanup_internal(&g_cloth_bones_ctx.items[handle.id]);
        if (g_cloth_bones_ctx.stats.active_simulations > 0) {
            g_cloth_bones_ctx.stats.active_simulations--;
        }
    }
    g_cloth_bones_ctx.stats.memory_usage = animation_cloth_bones_get_memory_usage();

    mutex_unlock(g_cloth_bones_ctx.lock);
}

int animation_cloth_bones_update(animation_cloth_bones_handle_t handle, const void* data, size_t size) {
    if (!g_cloth_bones_ctx.initialized) return -1;

    // Performance timer
    Timer* timer = perf_timer_create("cloth_bones_update");
    timer_start(timer);

    mutex_lock(g_cloth_bones_ctx.lock);

    if (handle.id >= g_cloth_bones_ctx.count) {
        mutex_unlock(g_cloth_bones_ctx.lock);
        timer_stop(timer);
        timer_destroy(timer);
        return -1;
    }

    animation_cloth_bones_internal_t* item = &g_cloth_bones_ctx.items[handle.id];
    if (!item->initialized) {
        mutex_unlock(g_cloth_bones_ctx.lock);
        timer_stop(timer);
        timer_destroy(timer);
        return -2;
    }

    // Basic data update simulation
    // Ideally we copy data here if we were storing it
    item->dirty = true;
    item->frame_updated++; // Assuming this increments per update call for now

    // Update stats
    g_cloth_bones_ctx.stats.updates_per_frame++;

    mutex_unlock(g_cloth_bones_ctx.lock);

    timer_stop(timer);

    // Simple moving average for stats (very rough approximation)
    g_cloth_bones_ctx.stats.average_update_time_ms =
        (g_cloth_bones_ctx.stats.average_update_time_ms * 0.95f) +
        ((float)timer_get_elapsed(timer) * 1000.0f * 0.05f);

    timer_destroy(timer);

    return 0;
}

bool animation_cloth_bones_is_valid(animation_cloth_bones_handle_t handle) {
    if (!g_cloth_bones_ctx.initialized) return false;

    // We avoid locking for simple read checks if acceptable, but for strict thread safety:
    mutex_lock(g_cloth_bones_ctx.lock);
    bool valid = false;
    if (handle.id < g_cloth_bones_ctx.count) {
        valid = g_cloth_bones_ctx.items[handle.id].initialized;
    }
    mutex_unlock(g_cloth_bones_ctx.lock);
    return valid;
}

int animation_cloth_bones_get_info(animation_cloth_bones_handle_t handle, animation_cloth_bones_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (!g_cloth_bones_ctx.initialized) return -2;

    mutex_lock(g_cloth_bones_ctx.lock);

    if (handle.id >= g_cloth_bones_ctx.count) {
        mutex_unlock(g_cloth_bones_ctx.lock);
        return -2;
    }

    const animation_cloth_bones_internal_t* item = &g_cloth_bones_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    mutex_unlock(g_cloth_bones_ctx.lock);
    return 0;
}

void animation_cloth_bones_mark_dirty(animation_cloth_bones_handle_t handle) {
    if (!g_cloth_bones_ctx.initialized) return;

    mutex_lock(g_cloth_bones_ctx.lock);
    if (handle.id < g_cloth_bones_ctx.count) {
        g_cloth_bones_ctx.items[handle.id].dirty = true;
    }
    mutex_unlock(g_cloth_bones_ctx.lock);
}

int animation_cloth_bones_process_pending(void) {
    if (!g_cloth_bones_ctx.initialized) return 0;

    mutex_lock(g_cloth_bones_ctx.lock);

    int processed = 0;
    for (uint32_t i = 0; i < g_cloth_bones_ctx.count; i++) {
        animation_cloth_bones_internal_t* item = &g_cloth_bones_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item logic would go here
            item->dirty = false;
            processed++;
        }
    }

    // Reset frame stats
    g_cloth_bones_ctx.stats.updates_per_frame = 0;

    mutex_unlock(g_cloth_bones_ctx.lock);
    return processed;
}

uint32_t animation_cloth_bones_get_count(void) {
    return g_cloth_bones_ctx.count;
}

size_t animation_cloth_bones_get_memory_usage(void) {
    // Basic calculation, potentially unsafe if not locked, but acceptable for stats
    size_t total = sizeof(g_cloth_bones_ctx);
    if (g_cloth_bones_ctx.items) {
        total += g_cloth_bones_ctx.capacity * sizeof(animation_cloth_bones_internal_t);
        for (uint32_t i = 0; i < g_cloth_bones_ctx.count; i++) {
            total += g_cloth_bones_ctx.items[i].data_size;
        }
    }
    return total;
}

int animation_cloth_bones_get_stats(animation_cloth_bones_stats_t* out_stats) {
    if (!out_stats || !g_cloth_bones_ctx.initialized) return -1;

    mutex_lock(g_cloth_bones_ctx.lock);
    *out_stats = g_cloth_bones_ctx.stats;
    mutex_unlock(g_cloth_bones_ctx.lock);

    return 0;
}

int animation_cloth_bones_serialize(animation_cloth_bones_handle_t handle, void* buffer, size_t size, size_t* out_written) {
    if (!g_cloth_bones_ctx.initialized) return -1;

    mutex_lock(g_cloth_bones_ctx.lock);

    if (handle.id >= g_cloth_bones_ctx.count || !g_cloth_bones_ctx.items[handle.id].initialized) {
        mutex_unlock(g_cloth_bones_ctx.lock);
        return -2;
    }

    animation_cloth_bones_internal_t* item = &g_cloth_bones_ctx.items[handle.id];
    size_t required_size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(size_t) + item->data_size;

    if (!buffer) {
        if (out_written) *out_written = required_size;
        mutex_unlock(g_cloth_bones_ctx.lock);
        return 0;
    }

    if (size < required_size) {
        mutex_unlock(g_cloth_bones_ctx.lock);
        return -3; // Buffer too small
    }

    uint8_t* ptr = (uint8_t*)buffer;

    // Serialize ID
    memcpy(ptr, &item->id, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    // Serialize Flags
    memcpy(ptr, &item->flags, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    // Serialize Data Size
    memcpy(ptr, &item->data_size, sizeof(size_t));
    ptr += sizeof(size_t);

    // Serialize Data
    if (item->data_size > 0 && item->data) {
        memcpy(ptr, item->data, item->data_size);
        ptr += item->data_size;
    }

    if (out_written) *out_written = required_size;

    mutex_unlock(g_cloth_bones_ctx.lock);
    return 0;
}

int animation_cloth_bones_deserialize(animation_cloth_bones_handle_t handle, const void* buffer, size_t size) {
    if (!g_cloth_bones_ctx.initialized || !buffer) return -1;

    mutex_lock(g_cloth_bones_ctx.lock);

    if (handle.id >= g_cloth_bones_ctx.count || !g_cloth_bones_ctx.items[handle.id].initialized) {
        mutex_unlock(g_cloth_bones_ctx.lock);
        return -2;
    }

    animation_cloth_bones_internal_t* item = &g_cloth_bones_ctx.items[handle.id];
    const uint8_t* ptr = (const uint8_t*)buffer;
    size_t read_bytes = 0;

    // Read ID (verify)
    uint32_t id;
    if (size < sizeof(uint32_t)) { mutex_unlock(g_cloth_bones_ctx.lock); return -3; }
    memcpy(&id, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    read_bytes += sizeof(uint32_t);

    if (id != item->id) {
        // ID mismatch, warning
        LOG_WARN(LOG_CAT_ANIMATION, "Deserialization ID mismatch: expected %u, got %u", item->id, id);
    }

    // Read Flags
    if (size - read_bytes < sizeof(uint32_t)) { mutex_unlock(g_cloth_bones_ctx.lock); return -3; }
    memcpy(&item->flags, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    read_bytes += sizeof(uint32_t);

    // Read Data Size
    size_t data_size;
    if (size - read_bytes < sizeof(size_t)) { mutex_unlock(g_cloth_bones_ctx.lock); return -3; }
    memcpy(&data_size, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    read_bytes += sizeof(size_t);

    // Read Data
    if (data_size > 0) {
        if (size - read_bytes < data_size) { mutex_unlock(g_cloth_bones_ctx.lock); return -3; }

        if (item->data) free(item->data);
        item->data = malloc(data_size);
        if (item->data) {
            memcpy(item->data, ptr, data_size);
            item->data_size = data_size;
        } else {
             LOG_ERROR(LOG_CAT_ANIMATION, "Failed to allocate memory for deserialized data");
             item->data_size = 0;
        }
    } else {
        if (item->data) free(item->data);
        item->data = NULL;
        item->data_size = 0;
    }

    item->dirty = true;
    mutex_unlock(g_cloth_bones_ctx.lock);
    return 0;
}

void animation_cloth_bones_debug_print(void) {
    if (!g_cloth_bones_ctx.initialized) return;

    mutex_lock(g_cloth_bones_ctx.lock);

    LOG_INFO(LOG_CAT_ANIMATION, "Cloth Bones Debug Info:");
    LOG_INFO(LOG_CAT_ANIMATION, "  Count: %u", g_cloth_bones_ctx.count);
    LOG_INFO(LOG_CAT_ANIMATION, "  Capacity: %u", g_cloth_bones_ctx.capacity);
    LOG_INFO(LOG_CAT_ANIMATION, "  Active Sims: %u", g_cloth_bones_ctx.stats.active_simulations);
    LOG_INFO(LOG_CAT_ANIMATION, "  Memory Usage: %zu bytes", g_cloth_bones_ctx.stats.memory_usage);
    LOG_INFO(LOG_CAT_ANIMATION, "  Avg Update Time: %.3f ms", g_cloth_bones_ctx.stats.average_update_time_ms);

    mutex_unlock(g_cloth_bones_ctx.lock);
}

/* End of cloth_bones.c */
