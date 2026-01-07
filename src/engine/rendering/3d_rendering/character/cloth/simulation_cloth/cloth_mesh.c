/*
 * cloth_mesh.c
 * Cloth mesh generation
 *
 * Part of the Cloth System subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements procedural cloth mesh generation for simulation
 */

#include "cloth_mesh.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CLOTH_MESH_MAX_COUNT 32
#define CLOTH_MESH_DEFAULT_CAPACITY 8
#define CLOTH_MAX_VERTICES 16384
#define CLOTH_MAX_INDICES 98304

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec2 {
    float x, y;
} vec2_t;

typedef struct vec3 {
    float x, y, z;
} vec3_t;

/* ============================================================================
 * CLOTH MESH TYPES
 * ============================================================================ */

typedef struct cloth_vertex {
    vec3_t position;
    vec3_t normal;
    vec2_t texcoord;
} cloth_vertex_t;

typedef struct cloth_system_cloth_mesh_internal {
    uint32_t id;
    uint32_t flags;
    
    // Mesh data (host copy)
    cloth_vertex_t* vertices;
    uint32_t vertex_count;
    
    uint32_t* indices;
    uint32_t index_count;
    
    // Grid settings
    uint32_t width_segments;
    uint32_t height_segments;
    float width;
    float height;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} cloth_system_cloth_mesh_internal_t;

typedef struct cloth_system_cloth_mesh_context {
    cloth_system_cloth_mesh_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} cloth_system_cloth_mesh_context_t;

static cloth_system_cloth_mesh_context_t g_cloth_mesh_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int cloth_system_cloth_mesh_init(void) {
    if (g_cloth_mesh_ctx.initialized) {
        return 0;
    }

    g_cloth_mesh_ctx.capacity = CLOTH_MESH_DEFAULT_CAPACITY;
    g_cloth_mesh_ctx.items = calloc(g_cloth_mesh_ctx.capacity, sizeof(cloth_system_cloth_mesh_internal_t));
    if (!g_cloth_mesh_ctx.items) {
        return -1;
    }

    g_cloth_mesh_ctx.count = 0;
    g_cloth_mesh_ctx.initialized = true;

    return 0;
}

void cloth_system_cloth_mesh_shutdown(void) {
    if (!g_cloth_mesh_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_cloth_mesh_ctx.count; i++) {
        cloth_system_cloth_mesh_internal_t* item = &g_cloth_mesh_ctx.items[i];
        if (item->vertices) free(item->vertices);
        if (item->indices) free(item->indices);
    }

    free(g_cloth_mesh_ctx.items);
    g_cloth_mesh_ctx.items = NULL;
    g_cloth_mesh_ctx.count = 0;
    g_cloth_mesh_ctx.capacity = 0;
    g_cloth_mesh_ctx.initialized = false;
}

int cloth_system_cloth_mesh_create(cloth_system_cloth_mesh_handle_t* out_handle, 
                                     const cloth_system_cloth_mesh_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_cloth_mesh_ctx.initialized) {
        return -2;
    }

    if (g_cloth_mesh_ctx.count >= g_cloth_mesh_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_cloth_mesh_ctx.count++;
    cloth_system_cloth_mesh_internal_t* item = &g_cloth_mesh_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->vertices = NULL;
    item->indices = NULL;
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void cloth_system_cloth_mesh_destroy(cloth_system_cloth_mesh_handle_t handle) {
    if (handle.id >= g_cloth_mesh_ctx.count) {
        return;
    }

    cloth_system_cloth_mesh_internal_t* item = &g_cloth_mesh_ctx.items[handle.id];
    if (item->vertices) free(item->vertices);
    if (item->indices) free(item->indices);
    
    item->initialized = false;
}

int cloth_system_cloth_mesh_generate_plane(cloth_system_cloth_mesh_handle_t handle,
                                             float width, float height,
                                             uint32_t width_segments, uint32_t height_segments) {
    if (handle.id >= g_cloth_mesh_ctx.count) return -1;
    
    cloth_system_cloth_mesh_internal_t* item = &g_cloth_mesh_ctx.items[handle.id];
    
    uint32_t vertex_count = (width_segments + 1) * (height_segments + 1);
    uint32_t index_count = width_segments * height_segments * 6;
    
    if (vertex_count > CLOTH_MAX_VERTICES) return -2;
    
    if (item->vertex_count != vertex_count) {
        if (item->vertices) free(item->vertices);
        item->vertices = calloc(vertex_count, sizeof(cloth_vertex_t));
        item->vertex_count = vertex_count;
    }
    
    if (item->index_count != index_count) {
        if (item->indices) free(item->indices);
        item->indices = calloc(index_count, sizeof(uint32_t));
        item->index_count = index_count;
    }
    
    if (!item->vertices || !item->indices) return -3;
    
    // Generate vertices
    float dx = width / width_segments;
    float dy = height / height_segments;
    float half_width = width * 0.5f;
    float half_height = height * 0.5f;
    
    for (uint32_t y = 0; y <= height_segments; y++) {
        for (uint32_t x = 0; x <= width_segments; x++) {
            uint32_t i = y * (width_segments + 1) + x;
            
            item->vertices[i].position.x = x * dx - half_width;
            item->vertices[i].position.y = 0.0f;
            item->vertices[i].position.z = y * dy - half_height;
            
            item->vertices[i].normal = (vec3_t){0, 1, 0};
            
            item->vertices[i].texcoord.x = (float)x / width_segments;
            item->vertices[i].texcoord.y = (float)y / height_segments;
        }
    }
    
    // Generate indices
    uint32_t i = 0;
    for (uint32_t y = 0; y < height_segments; y++) {
        for (uint32_t x = 0; x < width_segments; x++) {
            uint32_t row1 = y * (width_segments + 1);
            uint32_t row2 = (y + 1) * (width_segments + 1);
            
            item->indices[i++] = row1 + x;
            item->indices[i++] = row2 + x;
            item->indices[i++] = row1 + x + 1;
            
            item->indices[i++] = row1 + x + 1;
            item->indices[i++] = row2 + x;
            item->indices[i++] = row2 + x + 1;
        }
    }
    
    item->dirty = true;
    return 0;
}

int cloth_system_cloth_mesh_update(cloth_system_cloth_mesh_handle_t handle, 
                                     const void* data, size_t size) {
    if (handle.id >= g_cloth_mesh_ctx.count) {
        return -1;
    }

    g_cloth_mesh_ctx.items[handle.id].dirty = true;
    return 0;
}

bool cloth_system_cloth_mesh_is_valid(cloth_system_cloth_mesh_handle_t handle) {
    if (handle.id >= g_cloth_mesh_ctx.count) {
        return false;
    }
    return g_cloth_mesh_ctx.items[handle.id].initialized;
}

int cloth_system_cloth_mesh_get_info(cloth_system_cloth_mesh_handle_t handle, 
                                       cloth_system_cloth_mesh_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_cloth_mesh_ctx.count) {
        return -2;
    }

    const cloth_system_cloth_mesh_internal_t* item = &g_cloth_mesh_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void cloth_system_cloth_mesh_mark_dirty(cloth_system_cloth_mesh_handle_t handle) {
    if (handle.id < g_cloth_mesh_ctx.count) {
        g_cloth_mesh_ctx.items[handle.id].dirty = true;
    }
}

int cloth_system_cloth_mesh_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_cloth_mesh_ctx.count; i++) {
        cloth_system_cloth_mesh_internal_t* item = &g_cloth_mesh_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t cloth_system_cloth_mesh_get_count(void) {
    return g_cloth_mesh_ctx.count;
}

size_t cloth_system_cloth_mesh_get_memory_usage(void) {
    size_t total = sizeof(g_cloth_mesh_ctx);
    for (uint32_t i = 0; i < g_cloth_mesh_ctx.count; i++) {
        cloth_system_cloth_mesh_internal_t* item = &g_cloth_mesh_ctx.items[i];
        if (item->vertices) total += item->vertex_count * sizeof(cloth_vertex_t);
        if (item->indices) total += item->index_count * sizeof(uint32_t);
    }
    return total;
}

void cloth_system_cloth_mesh_debug_print(void) {
    // Debug output
}

/* End of cloth_mesh.c */
