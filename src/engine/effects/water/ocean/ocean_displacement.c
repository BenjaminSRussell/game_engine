/*
 * ocean_displacement.c
 * Vertex displacement
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#include "effects/water/ocean/ocean_displacement.h"
#include "effects/water/ocean/fft_waves.h"
#include "effects/water/ocean/gerstner_waves.h"
#include <math/math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define WATER_OCEAN_DISPLACEMENT_MAX_COUNT 1024
#define WATER_OCEAN_DISPLACEMENT_DEFAULT_CAPACITY 64

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum ocean_displacement_mode {
    OCEAN_DISPLACEMENT_MODE_FFT,
    OCEAN_DISPLACEMENT_MODE_GERSTNER,
    OCEAN_DISPLACEMENT_MODE_COMBINED
} ocean_displacement_mode_t;

typedef struct ocean_displacement_data {
    ocean_displacement_mode_t mode;
    water_fft_waves_handle_t fft_handle;
    water_gerstner_waves_handle_t gerstner_handle;
    
    float strength;
    float chopiness;
    Vec2 tiling;
    Vec2 offset;
} ocean_displacement_data_t;

typedef struct water_ocean_displacement_internal {
    uint32_t id;
    uint32_t flags;
    ocean_displacement_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} water_ocean_displacement_internal_t;

typedef struct water_ocean_displacement_context {
    water_ocean_displacement_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} water_ocean_displacement_context_t;

static water_ocean_displacement_context_t g_ocean_displacement_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool water_ocean_displacement_validate(const water_ocean_displacement_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data) return false;
    return true;
}

static void water_ocean_displacement_cleanup_internal(water_ocean_displacement_internal_t* item) {
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

int water_ocean_displacement_init(void) {
    if (g_ocean_displacement_ctx.initialized) {
        return 0;
    }

    g_ocean_displacement_ctx.capacity = WATER_OCEAN_DISPLACEMENT_DEFAULT_CAPACITY;
    g_ocean_displacement_ctx.items = calloc(g_ocean_displacement_ctx.capacity, sizeof(water_ocean_displacement_internal_t));
    if (!g_ocean_displacement_ctx.items) {
        return -1;
    }

    g_ocean_displacement_ctx.count = 0;
    g_ocean_displacement_ctx.initialized = true;

    return 0;
}

void water_ocean_displacement_shutdown(void) {
    if (!g_ocean_displacement_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_ocean_displacement_ctx.count; i++) {
        water_ocean_displacement_cleanup_internal(&g_ocean_displacement_ctx.items[i]);
    }

    free(g_ocean_displacement_ctx.items);
    g_ocean_displacement_ctx.items = NULL;
    g_ocean_displacement_ctx.count = 0;
    g_ocean_displacement_ctx.capacity = 0;
    g_ocean_displacement_ctx.initialized = false;
}

int water_ocean_displacement_create(water_ocean_displacement_handle_t* out_handle, const water_ocean_displacement_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_ocean_displacement_ctx.initialized) {
        return -2;
    }

    if (g_ocean_displacement_ctx.count >= g_ocean_displacement_ctx.capacity) {
        uint32_t new_capacity = g_ocean_displacement_ctx.capacity * 2;
        water_ocean_displacement_internal_t* new_items = realloc(g_ocean_displacement_ctx.items, new_capacity * sizeof(water_ocean_displacement_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_ocean_displacement_ctx.capacity, 0, (new_capacity - g_ocean_displacement_ctx.capacity) * sizeof(water_ocean_displacement_internal_t));
        g_ocean_displacement_ctx.items = new_items;
        g_ocean_displacement_ctx.capacity = new_capacity;
    }

    uint32_t index = g_ocean_displacement_ctx.count++;
    water_ocean_displacement_internal_t* item = &g_ocean_displacement_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(ocean_displacement_data_t));
    if (!item->data) {
        g_ocean_displacement_ctx.count--;
        return -4;
    }

    item->data->mode = OCEAN_DISPLACEMENT_MODE_GERSTNER;
    item->data->strength = 1.0f;
    item->data->chopiness = 1.0f;
    item->data->tiling = vec2(1.0f, 1.0f);
    item->data->offset = vec2(0.0f, 0.0f);

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void water_ocean_displacement_destroy(water_ocean_displacement_handle_t handle) {
    if (handle.id >= g_ocean_displacement_ctx.count) {
        return;
    }

    water_ocean_displacement_cleanup_internal(&g_ocean_displacement_ctx.items[handle.id]);
}

int water_ocean_displacement_update(water_ocean_displacement_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_ocean_displacement_ctx.count) {
        return -1;
    }

    water_ocean_displacement_internal_t* item = &g_ocean_displacement_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    if (data && size <= sizeof(ocean_displacement_data_t)) {
        memcpy(item->data, data, size);
    }

    item->dirty = true;
    return 0;
}

Vec3 water_ocean_displacement_sample(water_ocean_displacement_handle_t handle, Vec2 world_pos, float time) {
    if (handle.id >= g_ocean_displacement_ctx.count) {
        return vec3_zero();
    }

    water_ocean_displacement_internal_t* item = &g_ocean_displacement_ctx.items[handle.id];
    if (!item->initialized) {
        return vec3_zero();
    }

    ocean_displacement_data_t* data = item->data;
    Vec3 displacement = vec3_zero();

    // Transform world position
    Vec2 uv = vec2_add(vec2(world_pos.x * data->tiling.x, world_pos.y * data->tiling.y), data->offset);

    switch (data->mode) {
        case OCEAN_DISPLACEMENT_MODE_GERSTNER: {
            displacement = water_gerstner_waves_get_displacement(data->gerstner_handle, uv, time);
            break;
        }
        case OCEAN_DISPLACEMENT_MODE_FFT: {
            // In a real implementation, this would sample a displacement texture produced by FFT
            // For now, we return zero or a simple placeholder
            break;
        }
        case OCEAN_DISPLACEMENT_MODE_COMBINED: {
            Vec3 g = water_gerstner_waves_get_displacement(data->gerstner_handle, uv, time);
            // Add FFT component
            displacement = g;
            break;
        }
    }

    return vec3_mul(displacement, data->strength);
}

bool water_ocean_displacement_is_valid(water_ocean_displacement_handle_t handle) {
    if (handle.id >= g_ocean_displacement_ctx.count) {
        return false;
    }
    return g_ocean_displacement_ctx.items[handle.id].initialized;
}

int water_ocean_displacement_get_info(water_ocean_displacement_handle_t handle, water_ocean_displacement_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_ocean_displacement_ctx.count) {
        return -2;
    }

    const water_ocean_displacement_internal_t* item = &g_ocean_displacement_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void water_ocean_displacement_mark_dirty(water_ocean_displacement_handle_t handle) {
    if (handle.id < g_ocean_displacement_ctx.count) {
        g_ocean_displacement_ctx.items[handle.id].dirty = true;
    }
}

int water_ocean_displacement_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_ocean_displacement_ctx.count; i++) {
        water_ocean_displacement_internal_t* item = &g_ocean_displacement_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t water_ocean_displacement_get_count(void) {
    return g_ocean_displacement_ctx.count;
}

size_t water_ocean_displacement_get_memory_usage(void) {
    size_t total = sizeof(g_ocean_displacement_ctx);
    total += g_ocean_displacement_ctx.capacity * sizeof(water_ocean_displacement_internal_t);

    for (uint32_t i = 0; i < g_ocean_displacement_ctx.count; i++) {
        if (g_ocean_displacement_ctx.items[i].data) {
            total += sizeof(ocean_displacement_data_t);
        }
    }

    return total;
}

void water_ocean_displacement_debug_print(void) {
    // Debug printing implementation
}

/* End of ocean_displacement.c */
