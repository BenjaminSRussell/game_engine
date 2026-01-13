/*
 * budget_tracker.c
 * Memory budget monitoring
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#include "core/memory/budget_tracker.h"
#include "core/threading.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CORE_BUDGET_TRACKER_MAX_COUNT 4096
#define CORE_BUDGET_TRACKER_DEFAULT_CAPACITY 256
#define CORE_BUDGET_TRACKER_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_budget_tracker_internal {
    uint32_t id;
    uint32_t flags;
    void* data;         /* Optional associated data (not managed by tracker) */
    size_t data_size;   /* Size of associated data (if any) */

    char name[64];
    size_t limit;
    size_t current_usage;
    size_t peak_usage;
    uint32_t allocation_count;

    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} core_budget_tracker_internal_t;

typedef struct core_budget_tracker_context {
    core_budget_tracker_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    Mutex* lock;
    bool initialized;
} core_budget_tracker_context_t;

static core_budget_tracker_context_t g_budget_tracker_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void core_budget_tracker_cleanup_internal(core_budget_tracker_internal_t* item) {
    if (!item) return;
    /* We do not own item->data, so we don't free it */
    item->data = NULL;
    item->initialized = false;
    item->current_usage = 0;
    item->allocation_count = 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int core_budget_tracker_init(void) {
    if (g_budget_tracker_ctx.initialized) {
        return CORE_BUDGET_SUCCESS;
    }

    g_budget_tracker_ctx.capacity = CORE_BUDGET_TRACKER_DEFAULT_CAPACITY;
    g_budget_tracker_ctx.items = calloc(g_budget_tracker_ctx.capacity, sizeof(core_budget_tracker_internal_t));
    if (!g_budget_tracker_ctx.items) {
        return CORE_BUDGET_ERROR_OUT_OF_MEMORY;
    }

    g_budget_tracker_ctx.lock = mutex_create();
    if (!g_budget_tracker_ctx.lock) {
        free(g_budget_tracker_ctx.items);
        g_budget_tracker_ctx.items = NULL;
        return CORE_BUDGET_ERROR_INTERNAL;
    }

    g_budget_tracker_ctx.count = 0;
    g_budget_tracker_ctx.initialized = true;

    return CORE_BUDGET_SUCCESS;
}

void core_budget_tracker_shutdown(void) {
    if (!g_budget_tracker_ctx.initialized) {
        return;
    }

    mutex_lock(g_budget_tracker_ctx.lock);

    /* Leak detection / Validation on shutdown */
    for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
        if (g_budget_tracker_ctx.items[i].initialized && g_budget_tracker_ctx.items[i].current_usage > 0) {
            fprintf(stderr, "[BudgetTracker] Warning: Tracker '%s' (ID %u) has %zu bytes still allocated at shutdown.\n",
                g_budget_tracker_ctx.items[i].name, i, g_budget_tracker_ctx.items[i].current_usage);
        }
        core_budget_tracker_cleanup_internal(&g_budget_tracker_ctx.items[i]);
    }

    free(g_budget_tracker_ctx.items);
    g_budget_tracker_ctx.items = NULL;
    g_budget_tracker_ctx.count = 0;
    g_budget_tracker_ctx.capacity = 0;
    g_budget_tracker_ctx.initialized = false;

    mutex_unlock(g_budget_tracker_ctx.lock);
    mutex_destroy(g_budget_tracker_ctx.lock);
    g_budget_tracker_ctx.lock = NULL;
}

int core_budget_tracker_create(core_budget_tracker_handle_t* out_handle, const core_budget_tracker_desc_t* desc) {
    if (!out_handle || !desc) {
        return CORE_BUDGET_ERROR_INVALID_ARGS;
    }

    if (!g_budget_tracker_ctx.initialized) {
        return CORE_BUDGET_ERROR_NOT_INITIALIZED;
    }

    mutex_lock(g_budget_tracker_ctx.lock);

    if (g_budget_tracker_ctx.count >= g_budget_tracker_ctx.capacity) {
        /* Expand capacity */
        uint32_t new_capacity = g_budget_tracker_ctx.capacity * 2;
        if (new_capacity > CORE_BUDGET_TRACKER_MAX_COUNT) {
            new_capacity = CORE_BUDGET_TRACKER_MAX_COUNT;
        }
        if (g_budget_tracker_ctx.count >= new_capacity) {
             mutex_unlock(g_budget_tracker_ctx.lock);
             return CORE_BUDGET_ERROR_OUT_OF_MEMORY;
        }

        core_budget_tracker_internal_t* new_items = realloc(g_budget_tracker_ctx.items, new_capacity * sizeof(core_budget_tracker_internal_t));
        if (!new_items) {
            mutex_unlock(g_budget_tracker_ctx.lock);
            return CORE_BUDGET_ERROR_OUT_OF_MEMORY;
        }
        /* Zero out new memory */
        memset(new_items + g_budget_tracker_ctx.capacity, 0, (new_capacity - g_budget_tracker_ctx.capacity) * sizeof(core_budget_tracker_internal_t));

        g_budget_tracker_ctx.items = new_items;
        g_budget_tracker_ctx.capacity = new_capacity;
    }

    uint32_t index = g_budget_tracker_ctx.count++;
    core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->limit = desc->limit;
    item->current_usage = 0;
    item->peak_usage = 0;
    item->allocation_count = 0;

    if (desc->name) {
        strncpy(item->name, desc->name, sizeof(item->name) - 1);
        item->name[sizeof(item->name) - 1] = '\0';
    } else {
        snprintf(item->name, sizeof(item->name), "Tracker_%u", index);
    }

    out_handle->id = index;

    mutex_unlock(g_budget_tracker_ctx.lock);
    return CORE_BUDGET_SUCCESS;
}

void core_budget_tracker_destroy(core_budget_tracker_handle_t handle) {
    if (!g_budget_tracker_ctx.initialized) return;

    mutex_lock(g_budget_tracker_ctx.lock);
    if (handle.id < g_budget_tracker_ctx.count) {
        core_budget_tracker_cleanup_internal(&g_budget_tracker_ctx.items[handle.id]);
    }
    mutex_unlock(g_budget_tracker_ctx.lock);
}

int core_budget_tracker_allocate(core_budget_tracker_handle_t handle, size_t size) {
    if (!g_budget_tracker_ctx.initialized) return CORE_BUDGET_ERROR_NOT_INITIALIZED;

    int result = CORE_BUDGET_SUCCESS;
    mutex_lock(g_budget_tracker_ctx.lock);

    if (handle.id >= g_budget_tracker_ctx.count || !g_budget_tracker_ctx.items[handle.id].initialized) {
        result = CORE_BUDGET_ERROR_NOT_FOUND;
        goto done;
    }

    core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[handle.id];

    if (item->limit > 0 && (item->current_usage + size > item->limit)) {
        result = CORE_BUDGET_ERROR_LIMIT_EXCEEDED;
        goto done;
    }

    item->current_usage += size;
    if (item->current_usage > item->peak_usage) {
        item->peak_usage = item->current_usage;
    }
    item->allocation_count++;
    item->dirty = true;

done:
    mutex_unlock(g_budget_tracker_ctx.lock);
    return result;
}

int core_budget_tracker_deallocate(core_budget_tracker_handle_t handle, size_t size) {
    if (!g_budget_tracker_ctx.initialized) return CORE_BUDGET_ERROR_NOT_INITIALIZED;

    int result = CORE_BUDGET_SUCCESS;
    mutex_lock(g_budget_tracker_ctx.lock);

    if (handle.id >= g_budget_tracker_ctx.count || !g_budget_tracker_ctx.items[handle.id].initialized) {
        result = CORE_BUDGET_ERROR_NOT_FOUND;
        goto done;
    }

    core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[handle.id];

    if (item->current_usage < size) {
        /* Underflow? Just set to 0 and warn? */
        item->current_usage = 0;
    } else {
        item->current_usage -= size;
    }

    if (item->allocation_count > 0) {
        item->allocation_count--;
    }
    item->dirty = true;

done:
    mutex_unlock(g_budget_tracker_ctx.lock);
    return result;
}

int core_budget_tracker_update(core_budget_tracker_handle_t handle, const void* data, size_t size) {
    if (!g_budget_tracker_ctx.initialized) return CORE_BUDGET_ERROR_NOT_INITIALIZED;

    int result = CORE_BUDGET_SUCCESS;
    mutex_lock(g_budget_tracker_ctx.lock);

    if (handle.id >= g_budget_tracker_ctx.count) {
        result = CORE_BUDGET_ERROR_NOT_FOUND;
        goto done;
    }

    core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[handle.id];
    if (!item->initialized) {
        result = CORE_BUDGET_ERROR_NOT_FOUND;
        goto done;
    }

    /* Update associated data pointer/size but don't change usage (unless implied?) */
    /* Note: original implementation didn't copy data. */
    /* We treat this as updating metadata. */
    item->data = (void*)data;
    item->data_size = size;
    item->dirty = true;

done:
    mutex_unlock(g_budget_tracker_ctx.lock);
    return result;
}

bool core_budget_tracker_is_valid(core_budget_tracker_handle_t handle) {
    if (!g_budget_tracker_ctx.initialized) return false;

    /* Read without lock for speed, risk of tearing is low for boolean,
       but strictly should lock. For now, atomic read of count/init is assumed safe enough for is_valid */
    if (handle.id >= g_budget_tracker_ctx.count) return false;
    return g_budget_tracker_ctx.items[handle.id].initialized;
}

int core_budget_tracker_get_info(core_budget_tracker_handle_t handle, core_budget_tracker_info_t* out_info) {
    if (!out_info) return CORE_BUDGET_ERROR_INVALID_ARGS;
    if (!g_budget_tracker_ctx.initialized) return CORE_BUDGET_ERROR_NOT_INITIALIZED;

    int result = CORE_BUDGET_SUCCESS;
    mutex_lock(g_budget_tracker_ctx.lock);

    if (handle.id >= g_budget_tracker_ctx.count || !g_budget_tracker_ctx.items[handle.id].initialized) {
        result = CORE_BUDGET_ERROR_NOT_FOUND;
        goto done;
    }

    const core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    strncpy(out_info->name, item->name, sizeof(out_info->name));

done:
    mutex_unlock(g_budget_tracker_ctx.lock);
    return result;
}

int core_budget_tracker_get_status(core_budget_tracker_handle_t handle, core_budget_tracker_status_t* out_status) {
    if (!out_status) return CORE_BUDGET_ERROR_INVALID_ARGS;
    if (!g_budget_tracker_ctx.initialized) return CORE_BUDGET_ERROR_NOT_INITIALIZED;

    int result = CORE_BUDGET_SUCCESS;
    mutex_lock(g_budget_tracker_ctx.lock);

    if (handle.id >= g_budget_tracker_ctx.count || !g_budget_tracker_ctx.items[handle.id].initialized) {
        result = CORE_BUDGET_ERROR_NOT_FOUND;
        goto done;
    }

    const core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[handle.id];
    out_status->current_usage = item->current_usage;
    out_status->peak_usage = item->peak_usage;
    out_status->limit = item->limit;
    out_status->allocation_count = item->allocation_count;
    strncpy(out_status->name, item->name, sizeof(out_status->name));

done:
    mutex_unlock(g_budget_tracker_ctx.lock);
    return result;
}

void core_budget_tracker_mark_dirty(core_budget_tracker_handle_t handle) {
    if (!g_budget_tracker_ctx.initialized) return;

    mutex_lock(g_budget_tracker_ctx.lock);
    if (handle.id < g_budget_tracker_ctx.count) {
        g_budget_tracker_ctx.items[handle.id].dirty = true;
    }
    mutex_unlock(g_budget_tracker_ctx.lock);
}

int core_budget_tracker_process_pending(void) {
    if (!g_budget_tracker_ctx.initialized) return 0;

    mutex_lock(g_budget_tracker_ctx.lock);
    int processed = 0;
    for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
        core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    mutex_unlock(g_budget_tracker_ctx.lock);
    return processed;
}

uint32_t core_budget_tracker_get_count(void) {
    return g_budget_tracker_ctx.count;
}

size_t core_budget_tracker_get_memory_usage(void) {
    if (!g_budget_tracker_ctx.initialized) return 0;

    mutex_lock(g_budget_tracker_ctx.lock);

    size_t total = sizeof(g_budget_tracker_ctx);
    total += g_budget_tracker_ctx.capacity * sizeof(core_budget_tracker_internal_t);

    for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
        if (g_budget_tracker_ctx.items[i].initialized) {
            /* This counts the usage of resources tracked */
            total += g_budget_tracker_ctx.items[i].current_usage;
        }
    }

    mutex_unlock(g_budget_tracker_ctx.lock);
    return total;
}

void core_budget_tracker_debug_print(void) {
    if (!g_budget_tracker_ctx.initialized) {
        printf("[BudgetTracker] Not initialized.\n");
        return;
    }

    mutex_lock(g_budget_tracker_ctx.lock);

    printf("=== Budget Tracker Status ===\n");
    printf("Total Trackers: %u\n", g_budget_tracker_ctx.count);

    for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
        const core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[i];
        if (item->initialized) {
            printf("[%u] %s: %zu / %zu bytes (Peak: %zu, Allocations: %u)\n",
                item->id,
                item->name,
                item->current_usage,
                item->limit == 0 ? (size_t)-1 : item->limit,
                item->peak_usage,
                item->allocation_count);
        }
    }
    printf("=============================\n");

    mutex_unlock(g_budget_tracker_ctx.lock);
}
