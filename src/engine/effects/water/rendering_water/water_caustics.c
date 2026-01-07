/*
 * water_caustics.c
 * Underwater caustics
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#include "effects/water/rendering_water/water_caustics.h"
#include <math/math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define WATER_WATER_CAUSTICS_MAX_COUNT 32
#define WATER_WATER_CAUSTICS_DEFAULT_CAPACITY 8

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_caustics_data {
    float intensity;
    float focal_depth;
    float spread;
    uint32_t caustics_texture;
    uint32_t resolution;
    
    Vec3 light_direction;
    bool enable_chromatic_dispersion;
} water_caustics_data_t;

typedef struct water_water_caustics_internal {
    uint32_t id;
    uint32_t flags;
    water_caustics_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} water_water_caustics_internal_t;

typedef struct water_water_caustics_context {
    water_water_caustics_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} water_water_caustics_context_t;

static water_water_caustics_context_t g_water_caustics_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool water_water_caustics_validate(const water_water_caustics_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data) return false;
    return true;
}

static void water_water_caustics_cleanup_internal(water_water_caustics_internal_t* item) {
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

int water_water_caustics_init(void) {
    if (g_water_caustics_ctx.initialized) {
        return 0;
    }

    g_water_caustics_ctx.capacity = WATER_WATER_CAUSTICS_DEFAULT_CAPACITY;
    g_water_caustics_ctx.items = calloc(g_water_caustics_ctx.capacity, sizeof(water_water_caustics_internal_t));
    if (!g_water_caustics_ctx.items) {
        return -1;
    }

    g_water_caustics_ctx.count = 0;
    g_water_caustics_ctx.initialized = true;

    return 0;
}

void water_water_caustics_shutdown(void) {
    if (!g_water_caustics_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_water_caustics_ctx.count; i++) {
        water_water_caustics_cleanup_internal(&g_water_caustics_ctx.items[i]);
    }

    free(g_water_caustics_ctx.items);
    g_water_caustics_ctx.items = NULL;
    g_water_caustics_ctx.count = 0;
    g_water_caustics_ctx.capacity = 0;
    g_water_caustics_ctx.initialized = false;
}

int water_water_caustics_create(water_water_caustics_handle_t* out_handle, const water_water_caustics_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_water_caustics_ctx.initialized) {
        return -2;
    }

    if (g_water_caustics_ctx.count >= g_water_caustics_ctx.capacity) {
        uint32_t new_capacity = g_water_caustics_ctx.capacity * 2;
        water_water_caustics_internal_t* new_items = realloc(g_water_caustics_ctx.items, new_capacity * sizeof(water_water_caustics_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_water_caustics_ctx.capacity, 0, (new_capacity - g_water_caustics_ctx.capacity) * sizeof(water_water_caustics_internal_t));
        g_water_caustics_ctx.items = new_items;
        g_water_caustics_ctx.capacity = new_capacity;
    }

    uint32_t index = g_water_caustics_ctx.count++;
    water_water_caustics_internal_t* item = &g_water_caustics_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(water_caustics_data_t));
    if (!item->data) {
        g_water_caustics_ctx.count--;
        return -4;
    }

    item->data->intensity = 1.0f;
    item->data->focal_depth = 5.0f;
    item->data->spread = 0.5f;
    item->data->resolution = 512;
    item->data->light_direction = vec3(0.0f, -1.0f, 0.0f);
    item->data->enable_chromatic_dispersion = true;

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void water_water_caustics_destroy(water_water_caustics_handle_t handle) {
    if (handle.id >= g_water_caustics_ctx.count) {
        return;
    }

    water_water_caustics_cleanup_internal(&g_water_caustics_ctx.items[handle.id]);
}

int water_water_caustics_update(water_water_caustics_handle_t handle, float time) {
    if (handle.id >= g_water_caustics_ctx.count) {
        return -1;
    }

    water_water_caustics_internal_t* item = &g_water_caustics_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // Update caustics texture based on light direction and water surface
    // This is typically done via a compute shader that projects wave focusing patterns
    
    item->frame_updated++;
    item->dirty = false;
    return 0;
}

bool water_water_caustics_is_valid(water_water_caustics_handle_t handle) {
    if (handle.id >= g_water_caustics_ctx.count) {
        return false;
    }
    return g_water_caustics_ctx.items[handle.id].initialized;
}

int water_water_caustics_get_info(water_water_caustics_handle_t handle, water_water_caustics_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_water_caustics_ctx.count) {
        return -2;
    }

    const water_water_caustics_internal_t* item = &g_water_caustics_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void water_water_caustics_mark_dirty(water_water_caustics_handle_t handle) {
    if (handle.id < g_water_caustics_ctx.count) {
        g_water_caustics_ctx.items[handle.id].dirty = true;
    }
}

int water_water_caustics_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_water_caustics_ctx.count; i++) {
        water_water_caustics_internal_t* item = &g_water_caustics_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t water_water_caustics_get_count(void) {
    return g_water_caustics_ctx.count;
}

size_t water_water_caustics_get_memory_usage(void) {
    size_t total = sizeof(g_water_caustics_ctx);
    total += g_water_caustics_ctx.capacity * sizeof(water_water_caustics_internal_t);

    for (uint32_t i = 0; i < g_water_caustics_ctx.count; i++) {
        if (g_water_caustics_ctx.items[i].data) {
            total += sizeof(water_caustics_data_t);
        }
    }

    return total;
}

void water_water_caustics_debug_print(void) {
    // Debug printing implementation
}

/* End of water_caustics.c */
