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
    CoreBudgetBackend backend;
    size_t current_usage;
    size_t peak_usage;
    size_t limit;
    void* user_data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} core_budget_tracker_internal_t;

typedef struct core_budget_tracker_context {
    core_budget_tracker_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    Mutex* mutex;
    bool initialized;
} core_budget_tracker_context_t;

static core_budget_tracker_context_t g_budget_tracker_ctx = {0};

/* ============================================================================
 * BACKEND STUBS
 * ============================================================================ */

static size_t backend_vulkan_get_usage(void* user_data) {
    // In a real implementation, this would query the Vulkan Allocator
    return 0;
}

static size_t backend_metal_get_usage(void* user_data) {
    // Metal specific query
    return 0;
}

static size_t backend_d3d12_get_usage(void* user_data) {
    // D3D12 specific query
    return 0;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool core_budget_tracker_validate(const core_budget_tracker_internal_t* item) {
    if (!item) return false;

    // Backend specific validation
    switch (item->backend) {
        case CORE_BUDGET_BACKEND_VULKAN:
            // Check if Vulkan is available?
            break;
        case CORE_BUDGET_BACKEND_METAL:
            // Check if Metal is available?
            break;
        case CORE_BUDGET_BACKEND_D3D12:
            // Check if D3D12 is available?
            break;
        default:
            break;
    }

    return true;
}

static void core_budget_tracker_cleanup_internal(core_budget_tracker_internal_t* item) {
    if (!item) return;

    // Reset stats
    item->current_usage = 0;
    item->peak_usage = 0;
    item->user_data = NULL;
    item->initialized = false;
}

static void core_budget_tracker_debug_print_internal(void) {
    printf("Budget Tracker Debug Info:\n");
    printf("  Items: %u / %u\n", g_budget_tracker_ctx.count, g_budget_tracker_ctx.capacity);

    for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
        const core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[i];
        if (item->initialized) {
            const char* backend_str = "GENERIC";
            if (item->backend == CORE_BUDGET_BACKEND_VULKAN) backend_str = "VULKAN";
            else if (item->backend == CORE_BUDGET_BACKEND_METAL) backend_str = "METAL";
            else if (item->backend == CORE_BUDGET_BACKEND_D3D12) backend_str = "D3D12";

            printf("  [%u] %s: Usage %zu / %zu (Peak: %zu)\n",
                   item->id, backend_str, item->current_usage, item->limit, item->peak_usage);
        }
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int core_budget_tracker_init(void) {
    if (g_budget_tracker_ctx.initialized) {
        return 0; // Already initialized
    }

    g_budget_tracker_ctx.mutex = mutex_create();
    if (!g_budget_tracker_ctx.mutex) {
        return -1; // Failed to create mutex
    }

    mutex_lock(g_budget_tracker_ctx.mutex);

    g_budget_tracker_ctx.capacity = CORE_BUDGET_TRACKER_DEFAULT_CAPACITY;
    g_budget_tracker_ctx.items = calloc(g_budget_tracker_ctx.capacity, sizeof(core_budget_tracker_internal_t));
    if (!g_budget_tracker_ctx.items) {
        mutex_unlock(g_budget_tracker_ctx.mutex);
        mutex_destroy(g_budget_tracker_ctx.mutex);
        g_budget_tracker_ctx.mutex = NULL;
        return -2; // Allocation failure
    }

    g_budget_tracker_ctx.count = 0;
    g_budget_tracker_ctx.initialized = true;

    mutex_unlock(g_budget_tracker_ctx.mutex);

    return 0;
}

void core_budget_tracker_shutdown(void) {
    if (!g_budget_tracker_ctx.initialized) {
        return;
    }

    mutex_lock(g_budget_tracker_ctx.mutex);

    // Leak detection
    size_t leaked_items = 0;
    for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
        if (g_budget_tracker_ctx.items[i].initialized) {
            leaked_items++;
        }
    }

    if (leaked_items > 0) {
        printf("WARNING: Budget Tracker detected %zu leaked budget items on shutdown.\n", leaked_items);
        core_budget_tracker_debug_print_internal();
    }

    for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
        core_budget_tracker_cleanup_internal(&g_budget_tracker_ctx.items[i]);
    }

    free(g_budget_tracker_ctx.items);
    g_budget_tracker_ctx.items = NULL;
    g_budget_tracker_ctx.count = 0;
    g_budget_tracker_ctx.capacity = 0;
    g_budget_tracker_ctx.initialized = false;

    mutex_unlock(g_budget_tracker_ctx.mutex);
    mutex_destroy(g_budget_tracker_ctx.mutex);
    g_budget_tracker_ctx.mutex = NULL;
}

int core_budget_tracker_create(core_budget_tracker_handle_t* out_handle, const core_budget_tracker_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1; // Invalid arguments
    }

    if (!g_budget_tracker_ctx.initialized) {
        return -2; // Not initialized
    }

    mutex_lock(g_budget_tracker_ctx.mutex);

    // Reuse free slot if available
    int32_t index = -1;
    for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
        if (!g_budget_tracker_ctx.items[i].initialized) {
            index = i;
            break;
        }
    }

    // No free slot, expand if possible
    if (index == -1) {
        if (g_budget_tracker_ctx.count >= g_budget_tracker_ctx.capacity) {
            // Expand capacity
            uint32_t new_capacity = g_budget_tracker_ctx.capacity * 2;
            if (new_capacity > CORE_BUDGET_TRACKER_MAX_COUNT) {
                new_capacity = CORE_BUDGET_TRACKER_MAX_COUNT;
            }

            if (g_budget_tracker_ctx.count >= new_capacity) {
                mutex_unlock(g_budget_tracker_ctx.mutex);
                return -3; // Max capacity reached
            }

            core_budget_tracker_internal_t* new_items = realloc(g_budget_tracker_ctx.items, new_capacity * sizeof(core_budget_tracker_internal_t));
            if (!new_items) {
                mutex_unlock(g_budget_tracker_ctx.mutex);
                return -4; // Realloc failed
            }

            // Initialize new memory
            memset(new_items + g_budget_tracker_ctx.capacity, 0, (new_capacity - g_budget_tracker_ctx.capacity) * sizeof(core_budget_tracker_internal_t));

            g_budget_tracker_ctx.items = new_items;
            g_budget_tracker_ctx.capacity = new_capacity;
        }
        index = g_budget_tracker_ctx.count++;
    }

    core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->backend = desc->backend;
    item->limit = desc->limit;
    item->user_data = desc->user_data;
    item->current_usage = 0;
    item->peak_usage = 0;
    item->initialized = true; // Mark as initialized so validation passes if it checks this
    item->dirty = true;
    item->frame_updated = 0;

    if (!core_budget_tracker_validate(item)) {
        item->initialized = false;
        mutex_unlock(g_budget_tracker_ctx.mutex);
        return -5; // Validation failed
    }

    out_handle->id = index;

    mutex_unlock(g_budget_tracker_ctx.mutex);
    return 0;
}

void core_budget_tracker_destroy(core_budget_tracker_handle_t handle) {
    if (!g_budget_tracker_ctx.initialized) return;

    mutex_lock(g_budget_tracker_ctx.mutex);

    if (handle.id >= g_budget_tracker_ctx.count) {
        mutex_unlock(g_budget_tracker_ctx.mutex);
        return;
    }

    core_budget_tracker_cleanup_internal(&g_budget_tracker_ctx.items[handle.id]);

    mutex_unlock(g_budget_tracker_ctx.mutex);
}

int core_budget_tracker_update(core_budget_tracker_handle_t handle, const void* data, size_t size) {
    if (!g_budget_tracker_ctx.initialized) return -1;

    mutex_lock(g_budget_tracker_ctx.mutex);

    if (handle.id >= g_budget_tracker_ctx.count) {
        mutex_unlock(g_budget_tracker_ctx.mutex);
        return -2;
    }

    core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[handle.id];
    if (!item->initialized) {
        mutex_unlock(g_budget_tracker_ctx.mutex);
        return -3;
    }

    // Backend specific logic
    switch (item->backend) {
        case CORE_BUDGET_BACKEND_VULKAN:
            // For Vulkan backend, if data is NULL, we might query internally if supported
            if (size == 0 && item->user_data) {
                size = backend_vulkan_get_usage(item->user_data);
            }
            break;
        case CORE_BUDGET_BACKEND_METAL:
             if (size == 0 && item->user_data) {
                size = backend_metal_get_usage(item->user_data);
            }
            break;
        case CORE_BUDGET_BACKEND_D3D12:
             if (size == 0 && item->user_data) {
                size = backend_d3d12_get_usage(item->user_data);
            }
            break;
        default:
            break;
    }

    item->current_usage = size;
    if (item->current_usage > item->peak_usage) {
        item->peak_usage = item->current_usage;
    }

    item->dirty = true;

    // Check limit
    int result = 0;
    if (item->limit > 0 && item->current_usage > item->limit) {
        result = 1; // Over budget
    }

    mutex_unlock(g_budget_tracker_ctx.mutex);
    return result;
}

bool core_budget_tracker_is_valid(core_budget_tracker_handle_t handle) {
    if (!g_budget_tracker_ctx.initialized) return false;

    bool valid = false;
    // To avoid contention on hot path validation, we rely on stable array pointer if no resize happens often.
    // But since we have realloc, we MUST lock.
    mutex_lock(g_budget_tracker_ctx.mutex);
    if (handle.id < g_budget_tracker_ctx.count) {
        valid = g_budget_tracker_ctx.items[handle.id].initialized;
    }
    mutex_unlock(g_budget_tracker_ctx.mutex);
    return valid;
}

int core_budget_tracker_get_info(core_budget_tracker_handle_t handle, core_budget_tracker_info_t* out_info) {
    if (!out_info) return -1;
    if (!g_budget_tracker_ctx.initialized) return -2;

    mutex_lock(g_budget_tracker_ctx.mutex);

    if (handle.id >= g_budget_tracker_ctx.count) {
        mutex_unlock(g_budget_tracker_ctx.mutex);
        return -3;
    }

    const core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->backend = item->backend;
    out_info->current_usage = item->current_usage;
    out_info->peak_usage = item->peak_usage;
    out_info->limit = item->limit;
    out_info->initialized = item->initialized;

    mutex_unlock(g_budget_tracker_ctx.mutex);
    return 0;
}

void core_budget_tracker_mark_dirty(core_budget_tracker_handle_t handle) {
    if (!g_budget_tracker_ctx.initialized) return;

    mutex_lock(g_budget_tracker_ctx.mutex);
    if (handle.id < g_budget_tracker_ctx.count) {
        g_budget_tracker_ctx.items[handle.id].dirty = true;
    }
    mutex_unlock(g_budget_tracker_ctx.mutex);
}

int core_budget_tracker_process_pending(void) {
    if (!g_budget_tracker_ctx.initialized) return 0;

    int processed = 0;
    mutex_lock(g_budget_tracker_ctx.mutex);
    for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
        core_budget_tracker_internal_t* item = &g_budget_tracker_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Validate limits or perform callbacks if we had them
            if (item->limit > 0 && item->current_usage > item->limit) {
                // Could trigger an event here
            }
            item->dirty = false;
            processed++;
        }
    }
    mutex_unlock(g_budget_tracker_ctx.mutex);

    return processed;
}

uint32_t core_budget_tracker_get_count(void) {
    return g_budget_tracker_ctx.count;
}

size_t core_budget_tracker_get_memory_usage(void) {
    if (!g_budget_tracker_ctx.initialized) return 0;

    size_t total = sizeof(g_budget_tracker_ctx);

    mutex_lock(g_budget_tracker_ctx.mutex);
    total += g_budget_tracker_ctx.capacity * sizeof(core_budget_tracker_internal_t);

    for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
        // Sum up the tracked usage
        if (g_budget_tracker_ctx.items[i].initialized) {
            total += g_budget_tracker_ctx.items[i].current_usage;
        }
    }
    mutex_unlock(g_budget_tracker_ctx.mutex);

    return total;
}

void core_budget_tracker_debug_print(void) {
    if (!g_budget_tracker_ctx.initialized) return;

    mutex_lock(g_budget_tracker_ctx.mutex);
    core_budget_tracker_debug_print_internal();
    mutex_unlock(g_budget_tracker_ctx.mutex);
}
