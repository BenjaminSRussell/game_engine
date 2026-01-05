/*
 * ocean_renderer.c
 * Ocean surface rendering
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#include "ocean_renderer.h"
#include "../../../../include/math/math.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define WATER_OCEAN_RENDERER_MAX_COUNT 4096
#define WATER_OCEAN_RENDERER_DEFAULT_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ocean_renderer_data {
    Vec3 ocean_color;
    float roughness;
    float metallic;
    float subsurface_scattering;
    float wave_height_scale;
    float chopness;
    bool enable_caustics;
    bool enable_foam;
} ocean_renderer_data_t;

typedef struct water_ocean_renderer_internal {
    uint32_t id;
    uint32_t flags;
    ocean_renderer_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} water_ocean_renderer_internal_t;

typedef struct water_ocean_renderer_context {
    water_ocean_renderer_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} water_ocean_renderer_context_t;

static water_ocean_renderer_context_t g_ocean_renderer_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool water_ocean_renderer_validate(const water_ocean_renderer_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data) return false;
    return true;
}

static void water_ocean_renderer_cleanup_internal(water_ocean_renderer_internal_t* item) {
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

int water_ocean_renderer_init(void) {
    if (g_ocean_renderer_ctx.initialized) {
        return 0;
    }

    g_ocean_renderer_ctx.capacity = WATER_OCEAN_RENDERER_DEFAULT_CAPACITY;
    g_ocean_renderer_ctx.items = calloc(g_ocean_renderer_ctx.capacity, sizeof(water_ocean_renderer_internal_t));
    if (!g_ocean_renderer_ctx.items) {
        return -1;
    }

    g_ocean_renderer_ctx.count = 0;
    g_ocean_renderer_ctx.initialized = true;

    return 0;
}

void water_ocean_renderer_shutdown(void) {
    if (!g_ocean_renderer_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_ocean_renderer_ctx.count; i++) {
        water_ocean_renderer_cleanup_internal(&g_ocean_renderer_ctx.items[i]);
    }

    free(g_ocean_renderer_ctx.items);
    g_ocean_renderer_ctx.items = NULL;
    g_ocean_renderer_ctx.count = 0;
    g_ocean_renderer_ctx.capacity = 0;
    g_ocean_renderer_ctx.initialized = false;
}

int water_ocean_renderer_create(water_ocean_renderer_handle_t* out_handle, const water_ocean_renderer_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_ocean_renderer_ctx.initialized) {
        return -2;
    }

    if (g_ocean_renderer_ctx.count >= g_ocean_renderer_ctx.capacity) {
        uint32_t new_capacity = g_ocean_renderer_ctx.capacity * 2;
        water_ocean_renderer_internal_t* new_items = realloc(g_ocean_renderer_ctx.items, new_capacity * sizeof(water_ocean_renderer_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_ocean_renderer_ctx.capacity, 0, (new_capacity - g_ocean_renderer_ctx.capacity) * sizeof(water_ocean_renderer_internal_t));
        g_ocean_renderer_ctx.items = new_items;
        g_ocean_renderer_ctx.capacity = new_capacity;
    }

    uint32_t index = g_ocean_renderer_ctx.count++;
    water_ocean_renderer_internal_t* item = &g_ocean_renderer_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(ocean_renderer_data_t));
    if (!item->data) {
        g_ocean_renderer_ctx.count--;
        return -4;
    }

    // Default parameters
    item->data->ocean_color = vec3(0.0f, 0.1f, 0.3f);
    item->data->roughness = 0.2f;
    item->data->metallic = 0.0f;
    item->data->subsurface_scattering = 0.5f;
    item->data->wave_height_scale = 1.0f;
    item->data->chopness = 1.0f;
    item->data->enable_caustics = true;
    item->data->enable_foam = true;

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void water_ocean_renderer_destroy(water_ocean_renderer_handle_t handle) {
    if (handle.id >= g_ocean_renderer_ctx.count) {
        return;
    }

    water_ocean_renderer_cleanup_internal(&g_ocean_renderer_ctx.items[handle.id]);
}

int water_ocean_renderer_update(water_ocean_renderer_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_ocean_renderer_ctx.count) {
        return -1;
    }

    water_ocean_renderer_internal_t* item = &g_ocean_renderer_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    if (data && size == sizeof(ocean_renderer_data_t)) {
        memcpy(item->data, data, size);
        item->dirty = false;
        item->frame_updated++;
    }

    return 0;
}

bool water_ocean_renderer_is_valid(water_ocean_renderer_handle_t handle) {
    if (handle.id >= g_ocean_renderer_ctx.count) {
        return false;
    }
    return g_ocean_renderer_ctx.items[handle.id].initialized;
}

int water_ocean_renderer_get_info(water_ocean_renderer_handle_t handle, water_ocean_renderer_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_ocean_renderer_ctx.count) {
        return -2;
    }

    const water_ocean_renderer_internal_t* item = &g_ocean_renderer_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void water_ocean_renderer_mark_dirty(water_ocean_renderer_handle_t handle) {
    if (handle.id < g_ocean_renderer_ctx.count) {
        g_ocean_renderer_ctx.items[handle.id].dirty = true;
    }
}

int water_ocean_renderer_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_ocean_renderer_ctx.count; i++) {
        water_ocean_renderer_internal_t* item = &g_ocean_renderer_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t water_ocean_renderer_get_count(void) {
    return g_ocean_renderer_ctx.count;
}

size_t water_ocean_renderer_get_memory_usage(void) {
    size_t total = sizeof(g_ocean_renderer_ctx);
    total += g_ocean_renderer_ctx.capacity * sizeof(water_ocean_renderer_internal_t);

    for (uint32_t i = 0; i < g_ocean_renderer_ctx.count; i++) {
        if (g_ocean_renderer_ctx.items[i].data) {
            total += sizeof(ocean_renderer_data_t);
        }
    }

    return total;
}

void water_ocean_renderer_debug_print(void) {
    // Debug printing implementation
}
