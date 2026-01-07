/*
 * decal_sorting.c
 * Decal render order
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement GPU particle system
 * TODO: Add particle collision
 * TODO: Implement ribbon/trail rendering
 * TODO: Add VFX graph system
 * TODO: Implement decal rendering
 * TODO: Add weather effects
 * TODO: Implement particle sorting
 * TODO: Add particle LOD
 * TODO: Implement force fields
 * TODO: Add particle events
 * TODO: Implement decal sorting initialization
 * TODO: Add decal sorting cleanup/shutdown
 * TODO: Implement decal sorting validation
 * TODO: Add decal sorting error handling
 * TODO: Implement decal sorting serialization
 * TODO: Add decal sorting debug output
 * TODO: Implement decal sorting unit tests
 * TODO: Add decal sorting performance counters
 * TODO: Implement decal sorting hot-reload
 * TODO: Add decal sorting thread safety
 * TODO: Implement decal sorting memory pooling
 * TODO: Add decal sorting caching layer
 * TODO: Implement decal sorting async operations
 * TODO: Add decal sorting GPU integration
 * TODO: Implement decal sorting SIMD optimization
 * TODO: Add decal sorting batch processing
 * TODO: Implement decal sorting streaming support
 * TODO: Add decal sorting LOD support
 * TODO: Implement decal sorting culling integration
 * TODO: Add decal sorting render graph node
 */

#include "effects/decals/decal_sorting.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define EFFECTS_DECAL_SORTING_MAX_COUNT 4096
#define EFFECTS_DECAL_SORTING_DEFAULT_CAPACITY 256
#define EFFECTS_DECAL_SORTING_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct decal_sort_entry {
    uint32_t decal_id;
    float depth;
    uint32_t priority;
} decal_sort_entry_t;

typedef struct effects_decal_sorting_internal {
    uint32_t id;
    uint32_t flags;
    decal_sort_entry_t* entries;
    uint32_t entry_count;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} effects_decal_sorting_internal_t;

typedef struct effects_decal_sorting_context {
    effects_decal_sorting_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} effects_decal_sorting_context_t;

static effects_decal_sorting_context_t g_decal_sorting_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool effects_decal_sorting_validate(const effects_decal_sorting_internal_t* item) {
    // TODO: Implement GPU particle system
    // TODO: Add particle collision
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void effects_decal_sorting_cleanup_internal(effects_decal_sorting_internal_t* item) {
    if (!item) return;
    if (item->entries) {
        free(item->entries);
        item->entries = NULL;
    }
    item->entry_count = 0;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int effects_decal_sorting_init(void) {
    // TODO: Implement decal rendering
    // TODO: Add weather effects
    // TODO: Implement particle sorting
    // TODO: Add particle LOD

    if (g_decal_sorting_ctx.initialized) {
        return 0; // Already initialized
    }

    g_decal_sorting_ctx.capacity = EFFECTS_DECAL_SORTING_DEFAULT_CAPACITY;
    g_decal_sorting_ctx.items = calloc(g_decal_sorting_ctx.capacity, sizeof(effects_decal_sorting_internal_t));
    if (!g_decal_sorting_ctx.items) {
        return -1;
    }

    g_decal_sorting_ctx.count = 0;
    g_decal_sorting_ctx.initialized = true;

    return 0;
}

void effects_decal_sorting_shutdown(void) {
    // TODO: Implement force fields
    // TODO: Add particle events
    // TODO: Implement decal sorting initialization
    // TODO: Add decal sorting cleanup/shutdown

    if (!g_decal_sorting_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_decal_sorting_ctx.count; i++) {
        effects_decal_sorting_cleanup_internal(&g_decal_sorting_ctx.items[i]);
    }

    free(g_decal_sorting_ctx.items);
    g_decal_sorting_ctx.items = NULL;
    g_decal_sorting_ctx.count = 0;
    g_decal_sorting_ctx.capacity = 0;
    g_decal_sorting_ctx.initialized = false;
}

int effects_decal_sorting_create(effects_decal_sorting_handle_t* out_handle, const effects_decal_sorting_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_decal_sorting_ctx.initialized) {
        return -2;
    }

    if (g_decal_sorting_ctx.count >= g_decal_sorting_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_decal_sorting_ctx.count++;
    effects_decal_sorting_internal_t* item = &g_decal_sorting_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->entry_count = 0;
    item->entries = calloc(EFFECTS_DECAL_SORTING_MAX_COUNT, sizeof(decal_sort_entry_t));
    if (!item->entries) {
        g_decal_sorting_ctx.count--;
        return -4;
    }

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void effects_decal_sorting_destroy(effects_decal_sorting_handle_t handle) {
    // TODO: Add decal sorting performance counters
    // TODO: Implement decal sorting hot-reload

    if (handle.id >= g_decal_sorting_ctx.count) {
        return;
    }

    effects_decal_sorting_cleanup_internal(&g_decal_sorting_ctx.items[handle.id]);
}

static int decal_compare(const void* a, const void* b) {
    const decal_sort_entry_t* ea = (const decal_sort_entry_t*)a;
    const decal_sort_entry_t* eb = (const decal_sort_entry_t*)b;
    if (ea->priority != eb->priority) return (int)eb->priority - (int)ea->priority;
    return (ea->depth < eb->depth) ? -1 : 1;
}

int effects_decal_sorting_update(effects_decal_sorting_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_decal_sorting_ctx.count) {
        return -1;
    }

    effects_decal_sorting_internal_t* item = &g_decal_sorting_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // data could be raw list of decals to sort
    // For now we assume the data is already in 'entries' or passed here
    // If passed here:
    if (data && size > 0 && size % sizeof(decal_sort_entry_t) == 0) {
        uint32_t count = size / sizeof(decal_sort_entry_t);
        if (count > EFFECTS_DECAL_SORTING_MAX_COUNT) count = EFFECTS_DECAL_SORTING_MAX_COUNT;
        
        memcpy(item->entries, data, count * sizeof(decal_sort_entry_t));
        item->entry_count = count;
    }

    if (item->entry_count > 1) {
        qsort(item->entries, item->entry_count, sizeof(decal_sort_entry_t), decal_compare);
    }

    item->dirty = true;
    return 0;
}

bool effects_decal_sorting_is_valid(effects_decal_sorting_handle_t handle) {
    // TODO: Add decal sorting batch processing
    if (handle.id >= g_decal_sorting_ctx.count) {
        return false;
    }
    return g_decal_sorting_ctx.items[handle.id].initialized;
}

int effects_decal_sorting_get_info(effects_decal_sorting_handle_t handle, effects_decal_sorting_info_t* out_info) {
    // TODO: Implement decal sorting streaming support
    // TODO: Add decal sorting LOD support

    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_decal_sorting_ctx.count) {
        return -2;
    }

    const effects_decal_sorting_internal_t* item = &g_decal_sorting_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void effects_decal_sorting_mark_dirty(effects_decal_sorting_handle_t handle) {
    // TODO: Implement decal sorting culling integration
    if (handle.id < g_decal_sorting_ctx.count) {
        g_decal_sorting_ctx.items[handle.id].dirty = true;
    }
}

int effects_decal_sorting_process_pending(void) {
    // TODO: Add decal sorting render graph node
    // TODO: Implement batch processing

    int processed = 0;
    for (uint32_t i = 0; i < g_decal_sorting_ctx.count; i++) {
        effects_decal_sorting_internal_t* item = &g_decal_sorting_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t effects_decal_sorting_get_count(void) {
    return g_decal_sorting_ctx.count;
}

size_t effects_decal_sorting_get_memory_usage(void) {
    size_t total = sizeof(g_decal_sorting_ctx);
    total += g_decal_sorting_ctx.capacity * sizeof(effects_decal_sorting_internal_t);

    for (uint32_t i = 0; i < g_decal_sorting_ctx.count; i++) {
        total += EFFECTS_DECAL_SORTING_MAX_COUNT * sizeof(decal_sort_entry_t);
    }

    return total;
}

void effects_decal_sorting_debug_print(void) {
    // TODO: Implement debug output
    // Debug printing implementation
}

/* End of decal_sorting.c */
