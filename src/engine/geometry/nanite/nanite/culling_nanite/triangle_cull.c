/*
 * triangle_cull.c
 * Triangle-level culling
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#include "triangle_cull.h"
#include "../../3d_rendering.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define NANITE_TRIANGLE_CULL_MAX_COUNT 4096
#define NANITE_TRIANGLE_CULL_DEFAULT_CAPACITY 256
#define NANITE_TRIANGLE_CULL_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_triangle_cull_internal {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} nanite_triangle_cull_internal_t;

typedef struct nanite_triangle_cull_context {
    nanite_triangle_cull_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} nanite_triangle_cull_context_t;

static nanite_triangle_cull_context_t g_triangle_cull_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool triangle_backface_cull(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 view_dir) {
    Vec3 e0 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vec3 e1 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    
    // Cross product
    Vec3 normal = {
        e0.y * e1.z - e0.z * e1.y,
        e0.z * e1.x - e0.x * e1.z,
        e0.x * e1.y - e0.y * e1.x
    };
    
    // Dot product with view direction
    float dot = normal.x * view_dir.x + normal.y * view_dir.y + normal.z * view_dir.z;
    return dot <= 0.0f;
}

static bool triangle_small_cull(Vec2 p0, Vec2 p1, Vec2 p2, float min_size) {
    float min_x = fminf(fminf(p0.x, p1.x), p2.x);
    float max_x = fmaxf(fmaxf(p0.x, p1.x), p2.x);
    float min_y = fminf(fminf(p0.y, p1.y), p2.y);
    float max_y = fmaxf(fmaxf(p0.y, p1.y), p2.y);
    
    return (max_x - min_x) < min_size || (max_y - min_y) < min_size;
}

static void nanite_triangle_cull_cleanup_internal(nanite_triangle_cull_internal_t* item) {
    if (!item) return;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int nanite_triangle_cull_init(void) {
    if (g_triangle_cull_ctx.initialized) {
        return 0;
    }

    g_triangle_cull_ctx.capacity = NANITE_TRIANGLE_CULL_DEFAULT_CAPACITY;
    g_triangle_cull_ctx.items = calloc(g_triangle_cull_ctx.capacity, sizeof(nanite_triangle_cull_internal_t));
    if (!g_triangle_cull_ctx.items) {
        return -1;
    }

    g_triangle_cull_ctx.count = 0;
    g_triangle_cull_ctx.initialized = true;

    return 0;
}

void nanite_triangle_cull_shutdown(void) {
    if (!g_triangle_cull_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_triangle_cull_ctx.count; i++) {
        nanite_triangle_cull_cleanup_internal(&g_triangle_cull_ctx.items[i]);
    }

    free(g_triangle_cull_ctx.items);
    g_triangle_cull_ctx.items = NULL;
    g_triangle_cull_ctx.count = 0;
    g_triangle_cull_ctx.capacity = 0;
    g_triangle_cull_ctx.initialized = false;
}

int nanite_triangle_cull_create(nanite_triangle_cull_handle_t* out_handle, const nanite_triangle_cull_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_triangle_cull_ctx.initialized) {
        return -2;
    }

    if (g_triangle_cull_ctx.count >= g_triangle_cull_ctx.capacity) {
        uint32_t new_capacity = g_triangle_cull_ctx.capacity * 2;
        nanite_triangle_cull_internal_t* new_items = realloc(g_triangle_cull_ctx.items, new_capacity * sizeof(nanite_triangle_cull_internal_t));
        if (!new_items) return -3;
        g_triangle_cull_ctx.items = new_items;
        g_triangle_cull_ctx.capacity = new_capacity;
    }

    uint32_t index = g_triangle_cull_ctx.count++;
    nanite_triangle_cull_internal_t* item = &g_triangle_cull_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void nanite_triangle_cull_destroy(nanite_triangle_cull_handle_t handle) {
    if (handle.id >= g_triangle_cull_ctx.count) {
        return;
    }

    nanite_triangle_cull_cleanup_internal(&g_triangle_cull_ctx.items[handle.id]);
}

int nanite_triangle_cull_update(nanite_triangle_cull_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_triangle_cull_ctx.count) {
        return -1;
    }

    nanite_triangle_cull_internal_t* item = &g_triangle_cull_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool nanite_triangle_cull_is_valid(nanite_triangle_cull_handle_t handle) {
    if (handle.id >= g_triangle_cull_ctx.count) {
        return false;
    }
    return g_triangle_cull_ctx.items[handle.id].initialized;
}

int nanite_triangle_cull_get_info(nanite_triangle_cull_handle_t handle, nanite_triangle_cull_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_triangle_cull_ctx.count) {
        return -2;
    }

    const nanite_triangle_cull_internal_t* item = &g_triangle_cull_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void nanite_triangle_cull_mark_dirty(nanite_triangle_cull_handle_t handle) {
    if (handle.id < g_triangle_cull_ctx.count) {
        g_triangle_cull_ctx.items[handle.id].dirty = true;
    }
}

int nanite_triangle_cull_process_pending(void) {
    if (!g_triangle_cull_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_triangle_cull_ctx.count; i++) {
        nanite_triangle_cull_internal_t* item = &g_triangle_cull_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t nanite_triangle_cull_get_count(void) {
    return g_triangle_cull_ctx.count;
}

size_t nanite_triangle_cull_get_memory_usage(void) {
    size_t total = sizeof(g_triangle_cull_ctx);
    total += g_triangle_cull_ctx.capacity * sizeof(nanite_triangle_cull_internal_t);
    return total;
}

void nanite_triangle_cull_debug_print(void) {
    if (!g_triangle_cull_ctx.initialized) return;
    
    printf("Nanite Triangle Cull Status:\n");
    printf("  Count: %u / %u\n", g_triangle_cull_ctx.count, g_triangle_cull_ctx.capacity);
    printf("  Memory Usage: %zu bytes\n", nanite_triangle_cull_get_memory_usage());
}

/* End of triangle_cull.c */
