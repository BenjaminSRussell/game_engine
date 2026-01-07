/*
 * rt_shadow_rays.c
 * Ray-traced shadow rays
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/raytracing/shadows_rt/rt_shadow_rays.h"
#include <rendering/vulkan.h>
#include <rendering/vulkan_raytracing.h>
#include <rendering/lighting.h>
#include <core/logger.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define RAYTRACING_RT_SHADOW_RAYS_DEFAULT_CAPACITY 64

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_rt_shadow_rays_internal {
    uint32_t id;
    uint32_t flags;
    VkPipeline pipeline;
    VkDescriptorSet descriptor_set;
    bool initialized;
    bool dirty;
} raytracing_rt_shadow_rays_internal_t;

typedef struct raytracing_rt_shadow_rays_context {
    raytracing_rt_shadow_rays_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    VulkanRenderer* renderer;
    bool initialized;
} raytracing_rt_shadow_rays_context_t;

static raytracing_rt_shadow_rays_context_t g_rt_shadow_rays_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void raytracing_rt_shadow_rays_cleanup_internal(raytracing_rt_shadow_rays_internal_t* item) {
    if (!item || !item->initialized) return;
    
    // Pipelines and descriptor sets are usually managed by the renderer
    // but if we owned them, we'd destroy them here.
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int raytracing_rt_shadow_rays_init(void) {
    if (g_rt_shadow_rays_ctx.initialized) return 0;

    g_rt_shadow_rays_ctx.capacity = RAYTRACING_RT_SHADOW_RAYS_DEFAULT_CAPACITY;
    g_rt_shadow_rays_ctx.items = (raytracing_rt_shadow_rays_internal_t*)calloc(g_rt_shadow_rays_ctx.capacity, sizeof(raytracing_rt_shadow_rays_internal_t));
    if (!g_rt_shadow_rays_ctx.items) return -1;

    g_rt_shadow_rays_ctx.count = 0;
    g_rt_shadow_rays_ctx.initialized = true;

    return 0;
}

void raytracing_rt_shadow_rays_set_renderer(VulkanRenderer* renderer) {
    g_rt_shadow_rays_ctx.renderer = renderer;
}

void raytracing_rt_shadow_rays_shutdown(void) {
    if (!g_rt_shadow_rays_ctx.initialized) return;

    for (uint32_t i = 0; i < g_rt_shadow_rays_ctx.count; i++) {
        raytracing_rt_shadow_rays_cleanup_internal(&g_rt_shadow_rays_ctx.items[i]);
    }

    free(g_rt_shadow_rays_ctx.items);
    g_rt_shadow_rays_ctx.items = NULL;
    g_rt_shadow_rays_ctx.count = 0;
    g_rt_shadow_rays_ctx.capacity = 0;
    g_rt_shadow_rays_ctx.initialized = false;
}

int raytracing_rt_shadow_rays_create(raytracing_rt_shadow_rays_handle_t* out_handle, const raytracing_rt_shadow_rays_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_rt_shadow_rays_ctx.initialized) return -2;

    if (g_rt_shadow_rays_ctx.count >= g_rt_shadow_rays_ctx.capacity) {
        uint32_t new_capacity = g_rt_shadow_rays_ctx.capacity * 2;
        raytracing_rt_shadow_rays_internal_t* new_items = (raytracing_rt_shadow_rays_internal_t*)realloc(g_rt_shadow_rays_ctx.items, new_capacity * sizeof(raytracing_rt_shadow_rays_internal_t));
        if (!new_items) return -3;
        memset(new_items + g_rt_shadow_rays_ctx.capacity, 0, g_rt_shadow_rays_ctx.capacity * sizeof(raytracing_rt_shadow_rays_internal_t));
        g_rt_shadow_rays_ctx.items = new_items;
        g_rt_shadow_rays_ctx.capacity = new_capacity;
    }

    uint32_t index = g_rt_shadow_rays_ctx.count++;
    raytracing_rt_shadow_rays_internal_t* item = &g_rt_shadow_rays_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->pipeline = NULL;
    item->descriptor_set = NULL;
    item->initialized = true;
    item->dirty = true;

    out_handle->id = index;
    return 0;
}

void raytracing_rt_shadow_rays_destroy(raytracing_rt_shadow_rays_handle_t handle) {
    if (handle.id < g_rt_shadow_rays_ctx.count) {
        raytracing_rt_shadow_rays_cleanup_internal(&g_rt_shadow_rays_ctx.items[handle.id]);
    }
}

int raytracing_rt_shadow_rays_dispatch(raytracing_rt_shadow_rays_handle_t handle, VkCommandBuffer cmd, VkAccelerationStructureKHR tlas, uint32_t width, uint32_t height) {
    if (!g_rt_shadow_rays_ctx.renderer) return -1;
    if (handle.id >= g_rt_shadow_rays_ctx.count) return -2;
    
    raytracing_rt_shadow_rays_internal_t* item = &g_rt_shadow_rays_ctx.items[handle.id];
    if (!item->initialized || !item->pipeline) return -3;

    // Dispatch shadow rays
    vulkan_trace_rays(
        g_rt_shadow_rays_ctx.renderer,
        cmd,
        item->pipeline,
        item->descriptor_set,
        width,
        height
    );

    item->dirty = false;
    return 0;
}

int raytracing_rt_shadow_rays_update(raytracing_rt_shadow_rays_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_rt_shadow_rays_ctx.count) return -1;
    g_rt_shadow_rays_ctx.items[handle.id].dirty = true;
    (void)data; (void)size;
    return 0;
}

bool raytracing_rt_shadow_rays_is_valid(raytracing_rt_shadow_rays_handle_t handle) {
    if (handle.id >= g_rt_shadow_rays_ctx.count) return false;
    return g_rt_shadow_rays_ctx.items[handle.id].initialized;
}

int raytracing_rt_shadow_rays_get_info(raytracing_rt_shadow_rays_handle_t handle, raytracing_rt_shadow_rays_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= g_rt_shadow_rays_ctx.count) return -2;
    const raytracing_rt_shadow_rays_internal_t* item = &g_rt_shadow_rays_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    return 0;
}

void raytracing_rt_shadow_rays_mark_dirty(raytracing_rt_shadow_rays_handle_t handle) {
    if (handle.id < g_rt_shadow_rays_ctx.count) {
        g_rt_shadow_rays_ctx.items[handle.id].dirty = true;
    }
}

int raytracing_rt_shadow_rays_process_pending(void) {
    return 0;
}

uint32_t raytracing_rt_shadow_rays_get_count(void) {
    return g_rt_shadow_rays_ctx.count;
}

size_t raytracing_rt_shadow_rays_get_memory_usage(void) {
    size_t total = sizeof(g_rt_shadow_rays_ctx);
    total += g_rt_shadow_rays_ctx.capacity * sizeof(raytracing_rt_shadow_rays_internal_t);
    return total;
}

void raytracing_rt_shadow_rays_debug_print(void) {
    LOG_INFO("RT Shadow Rays Stats: %u active items", g_rt_shadow_rays_ctx.count);
}

/* End of rt_shadow_rays.c */
