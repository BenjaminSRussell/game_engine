/*
 * waterfall.c
 * Waterfall effects
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#include "effects/water/rivers/waterfall.h"
#include "include/math/math.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define WATER_WATERFALL_MAX_COUNT 4096
#define WATER_WATERFALL_DEFAULT_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct waterfall_data {
    Vec3 top_position;
    Vec3 bottom_position;
    float width;
    float speed;
    float thickness;
    Vec3 color;
} waterfall_data_t;

typedef struct water_waterfall_internal {
    uint32_t id;
    uint32_t flags;
    waterfall_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} water_waterfall_internal_t;

typedef struct water_waterfall_context {
    water_waterfall_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} water_waterfall_context_t;

static water_waterfall_context_t g_waterfall_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool water_waterfall_validate(const water_waterfall_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data) return false;
    return true;
}

static void water_waterfall_cleanup_internal(water_waterfall_internal_t* item) {
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

int water_waterfall_init(void) {
    if (g_waterfall_ctx.initialized) {
        return 0;
    }

    g_waterfall_ctx.capacity = WATER_WATERFALL_DEFAULT_CAPACITY;
    g_waterfall_ctx.items = calloc(g_waterfall_ctx.capacity, sizeof(water_waterfall_internal_t));
    if (!g_waterfall_ctx.items) {
        return -1;
    }

    g_waterfall_ctx.count = 0;
    g_waterfall_ctx.initialized = true;

    return 0;
}

void water_waterfall_shutdown(void) {
    if (!g_waterfall_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_waterfall_ctx.count; i++) {
        water_waterfall_cleanup_internal(&g_waterfall_ctx.items[i]);
    }

    free(g_waterfall_ctx.items);
    g_waterfall_ctx.items = NULL;
    g_waterfall_ctx.count = 0;
    g_waterfall_ctx.capacity = 0;
    g_waterfall_ctx.initialized = false;
}

int water_waterfall_create(water_waterfall_handle_t* out_handle, const water_waterfall_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_waterfall_ctx.initialized) {
        return -2;
    }

    if (g_waterfall_ctx.count >= g_waterfall_ctx.capacity) {
        uint32_t new_capacity = g_waterfall_ctx.capacity * 2;
        water_waterfall_internal_t* new_items = realloc(g_waterfall_ctx.items, new_capacity * sizeof(water_waterfall_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_waterfall_ctx.capacity, 0, (new_capacity - g_waterfall_ctx.capacity) * sizeof(water_waterfall_internal_t));
        g_waterfall_ctx.items = new_items;
        g_waterfall_ctx.capacity = new_capacity;
    }

    uint32_t index = g_waterfall_ctx.count++;
    water_waterfall_internal_t* item = &g_waterfall_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(waterfall_data_t));
    if (!item->data) {
        g_waterfall_ctx.count--;
        return -4;
    }

    // Default parameters
    item->data->width = 1.0f;
    item->data->speed = 2.0f;
    item->data->thickness = 0.2f;
    item->data->color = vec3(0.8f, 0.9f, 1.0f);

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void water_waterfall_destroy(water_waterfall_handle_t handle) {
    if (handle.id >= g_waterfall_ctx.count) {
        return;
    }

    water_waterfall_cleanup_internal(&g_waterfall_ctx.items[handle.id]);
}

int water_waterfall_update(water_waterfall_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_waterfall_ctx.count) {
        return -1;
    }

    water_waterfall_internal_t* item = &g_waterfall_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    if (data && size == sizeof(waterfall_data_t)) {
        memcpy(item->data, data, size);
        item->dirty = false;
        item->frame_updated++;
    }

    return 0;
}

bool water_waterfall_is_valid(water_waterfall_handle_t handle) {
    if (handle.id >= g_waterfall_ctx.count) {
        return false;
    }
    return g_waterfall_ctx.items[handle.id].initialized;
}

int water_waterfall_get_info(water_waterfall_handle_t handle, water_waterfall_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_waterfall_ctx.count) {
        return -2;
    }

    const water_waterfall_internal_t* item = &g_waterfall_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void water_waterfall_mark_dirty(water_waterfall_handle_t handle) {
    if (handle.id < g_waterfall_ctx.count) {
        g_waterfall_ctx.items[handle.id].dirty = true;
    }
}

int water_waterfall_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_waterfall_ctx.count; i++) {
        water_waterfall_internal_t* item = &g_waterfall_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t water_waterfall_get_count(void) {
    return g_waterfall_ctx.count;
}

size_t water_waterfall_get_memory_usage(void) {
    size_t total = sizeof(g_waterfall_ctx);
    total += g_waterfall_ctx.capacity * sizeof(water_waterfall_internal_t);

    for (uint32_t i = 0; i < g_waterfall_ctx.count; i++) {
        if (g_waterfall_ctx.items[i].data) {
            total += sizeof(waterfall_data_t);
        }
    }

    return total;
}

void water_waterfall_debug_print(void) {
    // Debug printing implementation
}
