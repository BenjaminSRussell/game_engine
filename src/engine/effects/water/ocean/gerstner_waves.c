/*
 * gerstner_waves.c
 * Gerstner wave superposition
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#include "effects/water/ocean/gerstner_waves.h"
#include <math/vec2.h>
#include <math/vec3.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define WATER_GERSTNER_WAVES_MAX_COUNT 4096
#define WATER_GERSTNER_WAVES_DEFAULT_CAPACITY 256
#define WATER_GERSTNER_WAVES_ALIGNMENT 16
#define WATER_GERSTNER_WAVES_MAX_PER_SYSTEM 8

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct gerstner_wave_params {
    Vec2 direction;
    float amplitude;
    float frequency;
    float steepness;
    float speed;
    float phase_offset;
} gerstner_wave_params_t;

typedef struct gerstner_waves_data {
    gerstner_wave_params_t waves[WATER_GERSTNER_WAVES_MAX_PER_SYSTEM];
    uint32_t wave_count;
    float time_scale;
} gerstner_waves_data_t;

typedef struct water_gerstner_waves_internal {
    uint32_t id;
    uint32_t flags;
    gerstner_waves_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} water_gerstner_waves_internal_t;

typedef struct water_gerstner_waves_context {
    water_gerstner_waves_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} water_gerstner_waves_context_t;

static water_gerstner_waves_context_t g_gerstner_waves_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool water_gerstner_waves_validate(const water_gerstner_waves_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data) return false;
    return true;
}

static void water_gerstner_waves_cleanup_internal(water_gerstner_waves_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

static Vec3 calculate_gerstner_displacement(const gerstner_waves_data_t* data, Vec2 pos, float time) {
    Vec3 displacement = vec3_zero();
    float t = time * data->time_scale;

    for (uint32_t i = 0; i < data->wave_count; i++) {
        const gerstner_wave_params_t* w = &data->waves[i];
        
        float phase = vec2_dot(w->direction, pos) * w->frequency + t * w->speed + w->phase_offset;
        float c = cosf(phase);
        float s = sinf(phase);

        // Gerstner wave formula:
        // x = x0 + sum(Qi * Ai * Di.x * cos(phase))
        // z = z0 + sum(Qi * Ai * Di.y * cos(phase))
        // y = sum(Ai * sin(phase))
        // Where Qi is steepness, Ai is amplitude, Di is direction

        float q_a = w->steepness * w->amplitude;
        displacement.x += q_a * w->direction.x * c;
        displacement.z += q_a * w->direction.y * c;
        displacement.y += w->amplitude * s;
    }

    return displacement;
}

static Vec3 calculate_gerstner_normal(const gerstner_waves_data_t* data, Vec2 pos, float time) {
    Vec3 normal = vec3(0.0f, 1.0f, 0.0f);
    float t = time * data->time_scale;

    float dx = 0.0f;
    float dz = 0.0f;
    float dy = 0.0f;

    for (uint32_t i = 0; i < data->wave_count; i++) {
        const gerstner_wave_params_t* w = &data->waves[i];
        
        float phase = vec2_dot(w->direction, pos) * w->frequency + t * w->speed + w->phase_offset;
        float c = cosf(phase);
        float s = sinf(phase);

        float wa = w->frequency * w->amplitude;
        float q_wa = w->steepness * wa;

        dx += w->direction.x * wa * c;
        dz += w->direction.y * wa * c;
        dy += q_wa * s;
    }

    // Normal = (-dx, 1 - dy, -dz)
    normal.x = -dx;
    normal.y = 1.0f - dy;
    normal.z = -dz;

    return vec3_normalize(normal);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int water_gerstner_waves_init(void) {
    if (g_gerstner_waves_ctx.initialized) {
        return 0;
    }

    g_gerstner_waves_ctx.capacity = WATER_GERSTNER_WAVES_DEFAULT_CAPACITY;
    g_gerstner_waves_ctx.items = calloc(g_gerstner_waves_ctx.capacity, sizeof(water_gerstner_waves_internal_t));
    if (!g_gerstner_waves_ctx.items) {
        return -1;
    }

    g_gerstner_waves_ctx.count = 0;
    g_gerstner_waves_ctx.initialized = true;

    return 0;
}

void water_gerstner_waves_shutdown(void) {
    if (!g_gerstner_waves_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_gerstner_waves_ctx.count; i++) {
        water_gerstner_waves_cleanup_internal(&g_gerstner_waves_ctx.items[i]);
    }

    free(g_gerstner_waves_ctx.items);
    g_gerstner_waves_ctx.items = NULL;
    g_gerstner_waves_ctx.count = 0;
    g_gerstner_waves_ctx.capacity = 0;
    g_gerstner_waves_ctx.initialized = false;
}

int water_gerstner_waves_create(water_gerstner_waves_handle_t* out_handle, const water_gerstner_waves_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_gerstner_waves_ctx.initialized) {
        return -2;
    }

    if (g_gerstner_waves_ctx.count >= g_gerstner_waves_ctx.capacity) {
        uint32_t new_capacity = g_gerstner_waves_ctx.capacity * 2;
        water_gerstner_waves_internal_t* new_items = realloc(g_gerstner_waves_ctx.items, new_capacity * sizeof(water_gerstner_waves_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_gerstner_waves_ctx.capacity, 0, (new_capacity - g_gerstner_waves_ctx.capacity) * sizeof(water_gerstner_waves_internal_t));
        g_gerstner_waves_ctx.items = new_items;
        g_gerstner_waves_ctx.capacity = new_capacity;
    }

    uint32_t index = g_gerstner_waves_ctx.count++;
    water_gerstner_waves_internal_t* item = &g_gerstner_waves_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(gerstner_waves_data_t));
    if (!item->data) {
        g_gerstner_waves_ctx.count--;
        return -4;
    }

    item->data->time_scale = 1.0f;
    item->data->wave_count = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void water_gerstner_waves_destroy(water_gerstner_waves_handle_t handle) {
    if (handle.id >= g_gerstner_waves_ctx.count) {
        return;
    }

    water_gerstner_waves_cleanup_internal(&g_gerstner_waves_ctx.items[handle.id]);
}

int water_gerstner_waves_update(water_gerstner_waves_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_gerstner_waves_ctx.count) {
        return -1;
    }

    water_gerstner_waves_internal_t* item = &g_gerstner_waves_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    if (data && size <= sizeof(gerstner_waves_data_t)) {
        memcpy(item->data, data, size);
    }

    item->dirty = true;
    return 0;
}

Vec3 water_gerstner_waves_get_displacement(water_gerstner_waves_handle_t handle, Vec2 pos, float time) {
    if (handle.id >= g_gerstner_waves_ctx.count) {
        return vec3_zero();
    }

    water_gerstner_waves_internal_t* item = &g_gerstner_waves_ctx.items[handle.id];
    if (!item->initialized) {
        return vec3_zero();
    }

    return calculate_gerstner_displacement(item->data, pos, time);
}

Vec3 water_gerstner_waves_get_normal(water_gerstner_waves_handle_t handle, Vec2 pos, float time) {
    if (handle.id >= g_gerstner_waves_ctx.count) {
        return vec3(0.0f, 1.0f, 0.0f);
    }

    water_gerstner_waves_internal_t* item = &g_gerstner_waves_ctx.items[handle.id];
    if (!item->initialized) {
        return vec3(0.0f, 1.0f, 0.0f);
    }

    return calculate_gerstner_normal(item->data, pos, time);
}

bool water_gerstner_waves_is_valid(water_gerstner_waves_handle_t handle) {
    if (handle.id >= g_gerstner_waves_ctx.count) {
        return false;
    }
    return g_gerstner_waves_ctx.items[handle.id].initialized;
}

int water_gerstner_waves_get_info(water_gerstner_waves_handle_t handle, water_gerstner_waves_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_gerstner_waves_ctx.count) {
        return -2;
    }

    const water_gerstner_waves_internal_t* item = &g_gerstner_waves_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void water_gerstner_waves_mark_dirty(water_gerstner_waves_handle_t handle) {
    if (handle.id < g_gerstner_waves_ctx.count) {
        g_gerstner_waves_ctx.items[handle.id].dirty = true;
    }
}

int water_gerstner_waves_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_gerstner_waves_ctx.count; i++) {
        water_gerstner_waves_internal_t* item = &g_gerstner_waves_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t water_gerstner_waves_get_count(void) {
    return g_gerstner_waves_ctx.count;
}

size_t water_gerstner_waves_get_memory_usage(void) {
    size_t total = sizeof(g_gerstner_waves_ctx);
    total += g_gerstner_waves_ctx.capacity * sizeof(water_gerstner_waves_internal_t);

    for (uint32_t i = 0; i < g_gerstner_waves_ctx.count; i++) {
        if (g_gerstner_waves_ctx.items[i].data) {
            total += sizeof(gerstner_waves_data_t);
        }
    }

    return total;
}

void water_gerstner_waves_debug_print(void) {
    // Debug printing implementation
}

/* End of gerstner_waves.c */
