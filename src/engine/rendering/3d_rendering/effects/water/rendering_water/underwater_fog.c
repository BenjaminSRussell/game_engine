/*
 * underwater_fog.c
 * Underwater fog/scattering
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#include "underwater_fog.h"
#include <math/math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define WATER_UNDERWATER_FOG_MAX_COUNT 32
#define WATER_UNDERWATER_FOG_DEFAULT_CAPACITY 8

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct underwater_fog_data {
    Vec3 fog_color;
    float density;
    float start_distance;
    float end_distance;
    
    float absorption_depth;
    Vec3 deep_water_color;
} underwater_fog_data_t;

typedef struct water_underwater_fog_internal {
    uint32_t id;
    uint32_t flags;
    underwater_fog_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} water_underwater_fog_internal_t;

typedef struct water_underwater_fog_context {
    water_underwater_fog_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} water_underwater_fog_context_t;

static water_underwater_fog_context_t g_underwater_fog_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool water_underwater_fog_validate(const water_underwater_fog_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data) return false;
    return true;
}

static void water_underwater_fog_cleanup_internal(water_underwater_fog_internal_t* item) {
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

int water_underwater_fog_init(void) {
    if (g_underwater_fog_ctx.initialized) {
        return 0;
    }

    g_underwater_fog_ctx.capacity = WATER_UNDERWATER_FOG_DEFAULT_CAPACITY;
    g_underwater_fog_ctx.items = calloc(g_underwater_fog_ctx.capacity, sizeof(water_underwater_fog_internal_t));
    if (!g_underwater_fog_ctx.items) {
        return -1;
    }

    g_underwater_fog_ctx.count = 0;
    g_underwater_fog_ctx.initialized = true;

    return 0;
}

void water_underwater_fog_shutdown(void) {
    if (!g_underwater_fog_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_underwater_fog_ctx.count; i++) {
        water_underwater_fog_cleanup_internal(&g_underwater_fog_ctx.items[i]);
    }

    free(g_underwater_fog_ctx.items);
    g_underwater_fog_ctx.items = NULL;
    g_underwater_fog_ctx.count = 0;
    g_underwater_fog_ctx.capacity = 0;
    g_underwater_fog_ctx.initialized = false;
}

int water_underwater_fog_create(water_underwater_fog_handle_t* out_handle, const water_underwater_fog_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_underwater_fog_ctx.initialized) {
        return -2;
    }

    if (g_underwater_fog_ctx.count >= g_underwater_fog_ctx.capacity) {
        uint32_t new_capacity = g_underwater_fog_ctx.capacity * 2;
        water_underwater_fog_internal_t* new_items = realloc(g_underwater_fog_ctx.items, new_capacity * sizeof(water_underwater_fog_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_underwater_fog_ctx.capacity, 0, (new_capacity - g_underwater_fog_ctx.capacity) * sizeof(water_underwater_fog_internal_t));
        g_underwater_fog_ctx.items = new_items;
        g_underwater_fog_ctx.capacity = new_capacity;
    }

    uint32_t index = g_underwater_fog_ctx.count++;
    water_underwater_fog_internal_t* item = &g_underwater_fog_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(underwater_fog_data_t));
    if (!item->data) {
        g_underwater_fog_ctx.count--;
        return -4;
    }

    item->data->fog_color = vec3(0.0f, 0.5f, 0.6f);
    item->data->density = 0.05f;
    item->data->start_distance = 0.0f;
    item->data->end_distance = 100.0f;
    item->data->absorption_depth = 10.0f;
    item->data->deep_water_color = vec3(0.0f, 0.1f, 0.2f);

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void water_underwater_fog_destroy(water_underwater_fog_handle_t handle) {
    if (handle.id >= g_underwater_fog_ctx.count) {
        return;
    }

    water_underwater_fog_cleanup_internal(&g_underwater_fog_ctx.items[handle.id]);
}

int water_underwater_fog_update(water_underwater_fog_handle_t handle, float time) {
    if (handle.id >= g_underwater_fog_ctx.count) {
        return -1;
    }

    water_underwater_fog_internal_t* item = &g_underwater_fog_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // Update fog uniforms for shader
    
    item->frame_updated++;
    item->dirty = false;
    return 0;
}

bool water_underwater_fog_is_valid(water_underwater_fog_handle_t handle) {
    if (handle.id >= g_underwater_fog_ctx.count) {
        return false;
    }
    return g_underwater_fog_ctx.items[handle.id].initialized;
}

int water_underwater_fog_get_info(water_underwater_fog_handle_t handle, water_underwater_fog_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_underwater_fog_ctx.count) {
        return -2;
    }

    const water_underwater_fog_internal_t* item = &g_underwater_fog_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void water_underwater_fog_mark_dirty(water_underwater_fog_handle_t handle) {
    if (handle.id < g_underwater_fog_ctx.count) {
        g_underwater_fog_ctx.items[handle.id].dirty = true;
    }
}

int water_underwater_fog_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_underwater_fog_ctx.count; i++) {
        water_underwater_fog_internal_t* item = &g_underwater_fog_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t water_underwater_fog_get_count(void) {
    return g_underwater_fog_ctx.count;
}

size_t water_underwater_fog_get_memory_usage(void) {
    size_t total = sizeof(g_underwater_fog_ctx);
    total += g_underwater_fog_ctx.capacity * sizeof(water_underwater_fog_internal_t);

    for (uint32_t i = 0; i < g_underwater_fog_ctx.count; i++) {
        if (g_underwater_fog_ctx.items[i].data) {
            total += sizeof(underwater_fog_data_t);
        }
    }

    return total;
}

void water_underwater_fog_debug_print(void) {
    // Debug printing implementation
}

/* End of underwater_fog.c */
