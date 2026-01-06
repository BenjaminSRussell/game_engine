/*
 * blas_builder.c
 * Bottom-level AS construction
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#include "blas_builder.h"
#include <renderer/vulkan.h>
#include <renderer/vulkan_raytracing.h>
#include <renderer/mesh.h>
#include <core/logger.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define RAYTRACING_BLAS_BUILDER_MAX_COUNT 4096
#define RAYTRACING_BLAS_BUILDER_DEFAULT_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_blas_builder_internal {
    uint32_t id;
    uint32_t flags;
    BLASBuildData vk_data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} raytracing_blas_builder_internal_t;

typedef struct raytracing_blas_builder_context {
    raytracing_blas_builder_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    VulkanRenderer* renderer;
    bool initialized;
} raytracing_blas_builder_context_t;

static raytracing_blas_builder_context_t g_blas_builder_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void raytracing_blas_builder_cleanup_internal(raytracing_blas_builder_internal_t* item) {
    if (!item || !item->initialized) return;
    
    if (g_blas_builder_ctx.renderer) {
        vulkan_destroy_blas(g_blas_builder_ctx.renderer, &item->vk_data);
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int raytracing_blas_builder_init(void) {
    if (g_blas_builder_ctx.initialized) {
        return 0; // Already initialized
    }

    g_blas_builder_ctx.capacity = RAYTRACING_BLAS_BUILDER_DEFAULT_CAPACITY;
    g_blas_builder_ctx.items = (raytracing_blas_builder_internal_t*)calloc(g_blas_builder_ctx.capacity, sizeof(raytracing_blas_builder_internal_t));
    if (!g_blas_builder_ctx.items) {
        return -1;
    }
    
    g_blas_builder_ctx.count = 0;
    g_blas_builder_ctx.initialized = true;

    return 0;
}

void raytracing_blas_builder_set_renderer(VulkanRenderer* renderer) {
    g_blas_builder_ctx.renderer = renderer;
}

void raytracing_blas_builder_shutdown(void) {
    if (!g_blas_builder_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_blas_builder_ctx.count; i++) {
        raytracing_blas_builder_cleanup_internal(&g_blas_builder_ctx.items[i]);
    }

    free(g_blas_builder_ctx.items);
    g_blas_builder_ctx.items = NULL;
    g_blas_builder_ctx.count = 0;
    g_blas_builder_ctx.capacity = 0;
    g_blas_builder_ctx.initialized = false;
}

int raytracing_blas_builder_create(raytracing_blas_builder_handle_t* out_handle, const raytracing_blas_builder_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_blas_builder_ctx.initialized) {
        return -2;
    }

    if (g_blas_builder_ctx.count >= g_blas_builder_ctx.capacity) {
        uint32_t new_capacity = g_blas_builder_ctx.capacity * 2;
        raytracing_blas_builder_internal_t* new_items = (raytracing_blas_builder_internal_t*)realloc(g_blas_builder_ctx.items, new_capacity * sizeof(raytracing_blas_builder_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_blas_builder_ctx.capacity, 0, g_blas_builder_ctx.capacity * sizeof(raytracing_blas_builder_internal_t));
        g_blas_builder_ctx.items = new_items;
        g_blas_builder_ctx.capacity = new_capacity;
    }

    uint32_t index = g_blas_builder_ctx.count++;
    raytracing_blas_builder_internal_t* item = &g_blas_builder_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    memset(&item->vk_data, 0, sizeof(BLASBuildData));
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void raytracing_blas_builder_destroy(raytracing_blas_builder_handle_t handle) {
    if (handle.id >= g_blas_builder_ctx.count) {
        return;
    }

    raytracing_blas_builder_cleanup_internal(&g_blas_builder_ctx.items[handle.id]);
}

int raytracing_blas_builder_build_from_mesh(raytracing_blas_builder_handle_t handle, Mesh* mesh) {
    if (!g_blas_builder_ctx.initialized || !g_blas_builder_ctx.renderer) {
        return -1;
    }

    if (handle.id >= g_blas_builder_ctx.count) {
        return -2;
    }

    raytracing_blas_builder_internal_t* item = &g_blas_builder_ctx.items[handle.id];
    if (!item->initialized) {
        return -3;
    }

    if (item->vk_data.blas != NULL) {
        vulkan_destroy_blas(g_blas_builder_ctx.renderer, &item->vk_data);
    }

    bool success = vulkan_build_blas(
        g_blas_builder_ctx.renderer,
        mesh->vertices,
        mesh->vertex_count,
        mesh->indices,
        mesh->index_count,
        0, // VK_FORMAT_R32G32B32_SFLOAT - placeholder
        &item->vk_data
    );

    if (!success) {
        LOG_ERROR("Failed to build BLAS for handle %u", handle.id);
        return -4;
    }

    item->dirty = false;
    return 0;
}

int raytracing_blas_builder_update(raytracing_blas_builder_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_blas_builder_ctx.count) {
        return -1;
    }

    raytracing_blas_builder_internal_t* item = &g_blas_builder_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    (void)data; (void)size;
    return 0;
}

bool raytracing_blas_builder_is_valid(raytracing_blas_builder_handle_t handle) {
    if (handle.id >= g_blas_builder_ctx.count) {
        return false;
    }
    return g_blas_builder_ctx.items[handle.id].initialized && g_blas_builder_ctx.items[handle.id].vk_data.blas != NULL;
}

int raytracing_blas_builder_get_info(raytracing_blas_builder_handle_t handle, raytracing_blas_builder_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= g_blas_builder_ctx.count) return -2;
    const raytracing_blas_builder_internal_t* item = &g_blas_builder_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    return 0;
}

void raytracing_blas_builder_mark_dirty(raytracing_blas_builder_handle_t handle) {
    if (handle.id < g_blas_builder_ctx.count) {
        g_blas_builder_ctx.items[handle.id].dirty = true;
    }
}

int raytracing_blas_builder_process_pending(void) {
    int processed = 0;
    // Potentially perform batched builds here
    return processed;
}

uint32_t raytracing_blas_builder_get_count(void) {
    return g_blas_builder_ctx.count;
}

size_t raytracing_blas_builder_get_memory_usage(void) {
    size_t total = sizeof(g_blas_builder_ctx);
    total += g_blas_builder_ctx.capacity * sizeof(raytracing_blas_builder_internal_t);
    // Add memory from Vulkan buffers in each handle
    return total;
}

void raytracing_blas_builder_debug_print(void) {
    LOG_INFO("BLAS Builder Stats: %u active items, %u capacity", g_blas_builder_ctx.count, g_blas_builder_ctx.capacity);
}

/* End of blas_builder.c */
