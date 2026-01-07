/*
 * river_foam.c
 * River foam/rapids
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#include "river_foam.h"
#include "../../../../include/math/math.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define WATER_RIVER_FOAM_MAX_COUNT 4096
#define WATER_RIVER_FOAM_DEFAULT_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct river_foam_data {
    float foam_intensity;
    float decay_rate;
    float bubble_density;
    Vec3 foam_color;
} river_foam_data_t;

typedef struct water_river_foam_internal {
    uint32_t id;
    uint32_t flags;
    river_foam_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} water_river_foam_internal_t;

typedef struct water_river_foam_context {
    water_river_foam_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} water_river_foam_context_t;

static water_river_foam_context_t g_river_foam_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool water_river_foam_validate(const water_river_foam_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data) return false;
    return true;
}

static void water_river_foam_cleanup_internal(water_river_foam_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int water_river_foam_init(void) {
    if (g_river_foam_ctx.initialized) {
        return 0;
    }

    g_river_foam_ctx.capacity = WATER_RIVER_FOAM_DEFAULT_CAPACITY;
    g_river_foam_ctx.items = calloc(g_river_foam_ctx.capacity, sizeof(water_river_foam_internal_t));
    if (!g_river_foam_ctx.items) {
        return -1;
    }

    g_river_foam_ctx.count = 0;
    g_river_foam_ctx.initialized = true;

    return 0;
}

void water_river_foam_shutdown(void) {
    if (!g_river_foam_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_river_foam_ctx.count; i++) {
        water_river_foam_cleanup_internal(&g_river_foam_ctx.items[i]);
    }

    free(g_river_foam_ctx.items);
    g_river_foam_ctx.items = NULL;
    g_river_foam_ctx.count = 0;
    g_river_foam_ctx.capacity = 0;
    g_river_foam_ctx.initialized = false;
}

int water_river_foam_create(water_river_foam_handle_t* out_handle, const water_river_foam_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_river_foam_ctx.initialized) {
        return -2;
    }

    if (g_river_foam_ctx.count >= g_river_foam_ctx.capacity) {
        uint32_t new_capacity = g_river_foam_ctx.capacity * 2;
        water_river_foam_internal_t* new_items = realloc(g_river_foam_ctx.items, new_capacity * sizeof(water_river_foam_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_river_foam_ctx.capacity, 0, (new_capacity - g_river_foam_ctx.capacity) * sizeof(water_river_foam_internal_t));
        g_river_foam_ctx.items = new_items;
        g_river_foam_ctx.capacity = new_capacity;
    }

    uint32_t index = g_river_foam_ctx.count++;
    water_river_foam_internal_t* item = &g_river_foam_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(river_foam_data_t));
    if (!item->data) {
        g_river_foam_ctx.count--;
        return -4;
    }

    // Default parameters
    item->data->foam_intensity = 1.0f;
    item->data->decay_rate = 0.5f;
    item->data->bubble_density = 10.0f;
    item->data->foam_color = vec3(1.0f, 1.0f, 1.0f);

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void water_river_foam_destroy(water_river_foam_handle_t handle) {
    if (handle.id >= g_river_foam_ctx.count) {
        return;
    }

    water_river_foam_cleanup_internal(&g_river_foam_ctx.items[handle.id]);
}

int water_river_foam_update(water_river_foam_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_river_foam_ctx.count) {
        return -1;
    }

    water_river_foam_internal_t* item = &g_river_foam_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    if (data && size == sizeof(river_foam_data_t)) {
        memcpy(item->data, data, size);
        item->dirty = false;
        item->frame_updated++;
    }

    return 0;
}

bool water_river_foam_is_valid(water_river_foam_handle_t handle) {
    if (handle.id >= g_river_foam_ctx.count) {
        return false;
    }
    return g_river_foam_ctx.items[handle.id].initialized;
}

int water_river_foam_get_info(water_river_foam_handle_t handle, water_river_foam_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_river_foam_ctx.count) {
        return -2;
    }

    const water_river_foam_internal_t* item = &g_river_foam_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void water_river_foam_mark_dirty(water_river_foam_handle_t handle) {
    if (handle.id < g_river_foam_ctx.count) {
        g_river_foam_ctx.items[handle.id].dirty = true;
    }
}

int water_river_foam_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_river_foam_ctx.count; i++) {
        water_river_foam_internal_t* item = &g_river_foam_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t water_river_foam_get_count(void) {
    return g_river_foam_ctx.count;
}

size_t water_river_foam_get_memory_usage(void) {
    size_t total = sizeof(g_river_foam_ctx);
    total += g_river_foam_ctx.capacity * sizeof(water_river_foam_internal_t);

    for (uint32_t i = 0; i < g_river_foam_ctx.count; i++) {
        if (g_river_foam_ctx.items[i].data) {
            total += sizeof(river_foam_data_t);
        }
    }

    return total;
}

void water_river_foam_debug_print(void) {
    // Debug printing implementation
}
