/*
 * buoyancy.c
 * Object buoyancy
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#include "buoyancy.h"
#include <math/math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define WATER_BUOYANCY_MAX_COUNT 4096
#define WATER_BUOYANCY_DEFAULT_CAPACITY 256
#define GRAVITY 9.81f
#define WATER_DENSITY 1000.0f

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_buoyancy_data {
    float volume;
    float density;
    float drag_coeff;
    Vec3 velocity;
    Vec3 position;
} water_buoyancy_data_t;

typedef struct water_buoyancy_internal {
    uint32_t id;
    uint32_t flags;
    water_buoyancy_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} water_buoyancy_internal_t;

typedef struct water_buoyancy_context {
    water_buoyancy_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} water_buoyancy_context_t;

static water_buoyancy_context_t g_buoyancy_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool water_buoyancy_validate(const water_buoyancy_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data) return false;
    return true;
}

static void water_buoyancy_cleanup_internal(water_buoyancy_internal_t* item) {
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

int water_buoyancy_init(void) {
    if (g_buoyancy_ctx.initialized) {
        return 0;
    }

    g_buoyancy_ctx.capacity = WATER_BUOYANCY_DEFAULT_CAPACITY;
    g_buoyancy_ctx.items = calloc(g_buoyancy_ctx.capacity, sizeof(water_buoyancy_internal_t));
    if (!g_buoyancy_ctx.items) {
        return -1;
    }

    g_buoyancy_ctx.count = 0;
    g_buoyancy_ctx.initialized = true;

    return 0;
}

void water_buoyancy_shutdown(void) {
    if (!g_buoyancy_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_buoyancy_ctx.count; i++) {
        water_buoyancy_cleanup_internal(&g_buoyancy_ctx.items[i]);
    }

    free(g_buoyancy_ctx.items);
    g_buoyancy_ctx.items = NULL;
    g_buoyancy_ctx.count = 0;
    g_buoyancy_ctx.capacity = 0;
    g_buoyancy_ctx.initialized = false;
}

int water_buoyancy_create(water_buoyancy_handle_t* out_handle, const water_buoyancy_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_buoyancy_ctx.initialized) {
        return -2;
    }

    if (g_buoyancy_ctx.count >= g_buoyancy_ctx.capacity) {
        uint32_t new_capacity = g_buoyancy_ctx.capacity * 2;
        water_buoyancy_internal_t* new_items = realloc(g_buoyancy_ctx.items, new_capacity * sizeof(water_buoyancy_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_buoyancy_ctx.capacity, 0, (new_capacity - g_buoyancy_ctx.capacity) * sizeof(water_buoyancy_internal_t));
        g_buoyancy_ctx.items = new_items;
        g_buoyancy_ctx.capacity = new_capacity;
    }

    uint32_t index = g_buoyancy_ctx.count++;
    water_buoyancy_internal_t* item = &g_buoyancy_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(water_buoyancy_data_t));
    if (!item->data) {
        g_buoyancy_ctx.count--;
        return -4;
    }

    // Initialize with default or desc values
    item->data->volume = 1.0f;
    item->data->density = 500.0f; // Less than water -> floats
    item->data->drag_coeff = 0.5f;
    item->data->velocity = vec3_zero();
    item->data->position = vec3_zero();

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void water_buoyancy_destroy(water_buoyancy_handle_t handle) {
    if (handle.id >= g_buoyancy_ctx.count) {
        return;
    }

    water_buoyancy_cleanup_internal(&g_buoyancy_ctx.items[handle.id]);
}

int water_buoyancy_update(water_buoyancy_handle_t handle, float delta_time, float water_level) {
    if (handle.id >= g_buoyancy_ctx.count) {
        return -1;
    }

    water_buoyancy_internal_t* item = &g_buoyancy_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // Simplified buoyancy calculation
    water_buoyancy_data_t* data = item->data;
    
    float merged_volume = 0.0f;
    if (data->position.y < water_level) {
        float difference = water_level - data->position.y;
        merged_volume = difference * 1.0f; // Approximate
        if (merged_volume > data->volume) merged_volume = data->volume;
    }

    if (merged_volume > 0.0f) {
        float buoyancy_force = WATER_DENSITY * merged_volume * GRAVITY;
        Vec3 force = vec3(0.0f, buoyancy_force, 0.0f);
        
        // Apply drag
        Vec3 drag = vec3_mul(data->velocity, -data->drag_coeff * 0.5f * WATER_DENSITY); 
        force = vec3_add(force, drag);
        
        // F = ma -> a = F/m. m = density * volume
        float mass = data->density * data->volume;
        Vec3 accel = vec3_div(force, mass);
        
        data->velocity = vec3_add(data->velocity, vec3_mul(accel, delta_time));
        data->position = vec3_add(data->position, vec3_mul(data->velocity, delta_time));
    }
    
    // Apply gravity always
    data->velocity.y -= GRAVITY * delta_time;
    data->position.y += data->velocity.y * delta_time; // Basic integration

    item->frame_updated++;
    item->dirty = false;
    return 0;
}

bool water_buoyancy_is_valid(water_buoyancy_handle_t handle) {
    if (handle.id >= g_buoyancy_ctx.count) {
        return false;
    }
    return g_buoyancy_ctx.items[handle.id].initialized;
}

int water_buoyancy_get_info(water_buoyancy_handle_t handle, water_buoyancy_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_buoyancy_ctx.count) {
        return -2;
    }

    const water_buoyancy_internal_t* item = &g_buoyancy_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void water_buoyancy_mark_dirty(water_buoyancy_handle_t handle) {
    if (handle.id < g_buoyancy_ctx.count) {
        g_buoyancy_ctx.items[handle.id].dirty = true;
    }
}

int water_buoyancy_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_buoyancy_ctx.count; i++) {
        water_buoyancy_internal_t* item = &g_buoyancy_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t water_buoyancy_get_count(void) {
    return g_buoyancy_ctx.count;
}

size_t water_buoyancy_get_memory_usage(void) {
    size_t total = sizeof(g_buoyancy_ctx);
    total += g_buoyancy_ctx.capacity * sizeof(water_buoyancy_internal_t);

    for (uint32_t i = 0; i < g_buoyancy_ctx.count; i++) {
        if (g_buoyancy_ctx.items[i].data) {
            total += sizeof(water_buoyancy_data_t);
        }
    }

    return total;
}

void water_buoyancy_debug_print(void) {
    // Debug printing implementation
}

/* End of buoyancy.c */
