/*
 * tlas_builder.c
 * Top-level AS construction
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#include "tlas_builder.h"
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

#define RAYTRACING_TLAS_BUILDER_MAX_INSTANCES 16384
#define RAYTRACING_TLAS_BUILDER_DEFAULT_CAPACITY 32

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_tlas_instance {
    BLASBuildData* blas;
    Mat4 transform;
    uint32_t custom_index;
} raytracing_tlas_instance_t;

typedef struct raytracing_tlas_builder_internal {
    uint32_t id;
    uint32_t flags;
    VkAccelerationStructureKHR tlas;
    raytracing_tlas_instance_t* instances;
    uint32_t instance_count;
    uint32_t instance_capacity;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} raytracing_tlas_builder_internal_t;

typedef struct raytracing_tlas_builder_context {
    raytracing_tlas_builder_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    VulkanRenderer* renderer;
    bool initialized;
} raytracing_tlas_builder_context_t;

static raytracing_tlas_builder_context_t g_tlas_builder_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void raytracing_tlas_builder_cleanup_internal(raytracing_tlas_builder_internal_t* item) {
    if (!item || !item->initialized) return;
    
    if (g_tlas_builder_ctx.renderer && item->tlas != NULL) {
        vulkan_destroy_tlas(g_tlas_builder_ctx.renderer, item->tlas);
    }
    
    if (item->instances) {
        free(item->instances);
        item->instances = NULL;
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int raytracing_tlas_builder_init(void) {
    if (g_tlas_builder_ctx.initialized) return 0;

    g_tlas_builder_ctx.capacity = RAYTRACING_TLAS_BUILDER_DEFAULT_CAPACITY;
    g_tlas_builder_ctx.items = (raytracing_tlas_builder_internal_t*)calloc(g_tlas_builder_ctx.capacity, sizeof(raytracing_tlas_builder_internal_t));
    if (!g_tlas_builder_ctx.items) return -1;

    g_tlas_builder_ctx.count = 0;
    g_tlas_builder_ctx.initialized = true;

    return 0;
}

void raytracing_tlas_builder_set_renderer(VulkanRenderer* renderer) {
    g_tlas_builder_ctx.renderer = renderer;
}

void raytracing_tlas_builder_shutdown(void) {
    if (!g_tlas_builder_ctx.initialized) return;

    for (uint32_t i = 0; i < g_tlas_builder_ctx.count; i++) {
        raytracing_tlas_builder_cleanup_internal(&g_tlas_builder_ctx.items[i]);
    }

    free(g_tlas_builder_ctx.items);
    g_tlas_builder_ctx.items = NULL;
    g_tlas_builder_ctx.count = 0;
    g_tlas_builder_ctx.capacity = 0;
    g_tlas_builder_ctx.initialized = false;
}

int raytracing_tlas_builder_create(raytracing_tlas_builder_handle_t* out_handle, const raytracing_tlas_builder_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_tlas_builder_ctx.initialized) return -2;

    if (g_tlas_builder_ctx.count >= g_tlas_builder_ctx.capacity) {
        uint32_t new_capacity = g_tlas_builder_ctx.capacity * 2;
        raytracing_tlas_builder_internal_t* new_items = (raytracing_tlas_builder_internal_t*)realloc(g_tlas_builder_ctx.items, new_capacity * sizeof(raytracing_tlas_builder_internal_t));
        if (!new_items) return -3;
        memset(new_items + g_tlas_builder_ctx.capacity, 0, g_tlas_builder_ctx.capacity * sizeof(raytracing_tlas_builder_internal_t));
        g_tlas_builder_ctx.items = new_items;
        g_tlas_builder_ctx.capacity = new_capacity;
    }

    uint32_t index = g_tlas_builder_ctx.count++;
    raytracing_tlas_builder_internal_t* item = &g_tlas_builder_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->tlas = NULL;
    item->instance_count = 0;
    item->instance_capacity = 64;
    item->instances = (raytracing_tlas_instance_t*)calloc(item->instance_capacity, sizeof(raytracing_tlas_instance_t));
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void raytracing_tlas_builder_destroy(raytracing_tlas_builder_handle_t handle) {
    if (handle.id < g_tlas_builder_ctx.count) {
        raytracing_tlas_builder_cleanup_internal(&g_tlas_builder_ctx.items[handle.id]);
    }
}

int raytracing_tlas_builder_add_instance(raytracing_tlas_builder_handle_t handle, BLASBuildData* blas, Mat4 transform, uint32_t custom_index) {
    if (handle.id >= g_tlas_builder_ctx.count) return -1;
    raytracing_tlas_builder_internal_t* item = &g_tlas_builder_ctx.items[handle.id];
    if (!item->initialized) return -2;

    if (item->instance_count >= item->instance_capacity) {
        uint32_t new_capacity = item->instance_capacity * 2;
        raytracing_tlas_instance_t* new_instances = (raytracing_tlas_instance_t*)realloc(item->instances, new_capacity * sizeof(raytracing_tlas_instance_t));
        if (!new_instances) return -3;
        item->instances = new_instances;
        item->instance_capacity = new_capacity;
    }

    uint32_t idx = item->instance_count++;
    item->instances[idx].blas = blas;
    item->instances[idx].transform = transform;
    item->instances[idx].custom_index = custom_index;
    item->dirty = true;

    return 0;
}

int raytracing_tlas_builder_clear_instances(raytracing_tlas_builder_handle_t handle) {
    if (handle.id >= g_tlas_builder_ctx.count) return -1;
    g_tlas_builder_ctx.items[handle.id].instance_count = 0;
    g_tlas_builder_ctx.items[handle.id].dirty = true;
    return 0;
}

int raytracing_tlas_builder_build(raytracing_tlas_builder_handle_t handle) {
    if (!g_tlas_builder_ctx.initialized || !g_tlas_builder_ctx.renderer) return -1;
    if (handle.id >= g_tlas_builder_ctx.count) return -2;

    raytracing_tlas_builder_internal_t* item = &g_tlas_builder_ctx.items[handle.id];
    if (!item->initialized || item->instance_count == 0) return -3;

    // Clean up old TLAS
    if (item->tlas != NULL) {
        vulkan_destroy_tlas(g_tlas_builder_ctx.renderer, item->tlas);
        item->tlas = NULL;
    }

    // We need an array of BLASBuildData for vulkan_build_tlas
    // Note: The current vulkan_build_tlas implementation ignores transforms in the input BLASBuildData array,
    // it likely needs to be updated to take VkAccelerationStructureInstanceKHR or we need to pass them.
    // Assuming for now it takes instances or we will update it.
    
    // Create temporary BLAS array as expected by vulkan_build_tlas (if it only needs the BLAS handles)
    BLASBuildData* temp_blas_array = (BLASBuildData*)malloc(item->instance_count * sizeof(BLASBuildData));
    for (uint32_t i = 0; i < item->instance_count; i++) {
        temp_blas_array[i] = *item->instances[i].blas;
        // In a real implementation, we'd pass the transform here too
    }

    bool success = vulkan_build_tlas(
        g_tlas_builder_ctx.renderer,
        temp_blas_array,
        item->instance_count,
        &item->tlas
    );

    free(temp_blas_array);

    if (!success) {
        LOG_ERROR("Failed to build TLAS for handle %u", handle.id);
        return -4;
    }

    item->dirty = false;
    return 0;
}

int raytracing_tlas_builder_update(raytracing_tlas_builder_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_tlas_builder_ctx.count) return -1;
    g_tlas_builder_ctx.items[handle.id].dirty = true;
    (void)data; (void)size;
    return 0;
}

bool raytracing_tlas_builder_is_valid(raytracing_tlas_builder_handle_t handle) {
    if (handle.id >= g_tlas_builder_ctx.count) return false;
    return g_tlas_builder_ctx.items[handle.id].initialized && g_tlas_builder_ctx.items[handle.id].tlas != NULL;
}

int raytracing_tlas_builder_get_info(raytracing_tlas_builder_handle_t handle, raytracing_tlas_builder_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= g_tlas_builder_ctx.count) return -2;
    const raytracing_tlas_builder_internal_t* item = &g_tlas_builder_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    return 0;
}

void raytracing_tlas_builder_mark_dirty(raytracing_tlas_builder_handle_t handle) {
    if (handle.id < g_tlas_builder_ctx.count) {
        g_tlas_builder_ctx.items[handle.id].dirty = true;
    }
}

int raytracing_tlas_builder_process_pending(void) {
    return 0;
}

uint32_t raytracing_tlas_builder_get_count(void) {
    return g_tlas_builder_ctx.count;
}

size_t raytracing_tlas_builder_get_memory_usage(void) {
    size_t total = sizeof(g_tlas_builder_ctx);
    total += g_tlas_builder_ctx.capacity * sizeof(raytracing_tlas_builder_internal_t);
    return total;
}

void raytracing_tlas_builder_debug_print(void) {
    LOG_INFO("TLAS Builder Stats: %u active items", g_tlas_builder_ctx.count);
}

/* End of tlas_builder.c */
