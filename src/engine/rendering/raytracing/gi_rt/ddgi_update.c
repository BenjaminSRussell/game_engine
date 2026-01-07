/*
 * ddgi_update.c
 * DDGI probe update
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/raytracing/gi_rt/ddgi_update.h"
#include <rendering/vulkan.h>
#include <rendering/vulkan_raytracing.h>
#include <core/logger.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define RAYTRACING_DDGI_UPDATE_DEFAULT_CAPACITY 8

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_ddgi_update_internal {
    uint32_t id;
    uint32_t flags;
    VkPipeline pipeline;
    VkDescriptorSet descriptor_set;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} raytracing_ddgi_update_internal_t;

typedef struct raytracing_ddgi_update_context {
    raytracing_ddgi_update_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    VulkanRenderer* renderer;
    bool initialized;
} raytracing_ddgi_update_context_t;

static raytracing_ddgi_update_context_t g_ddgi_update_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void raytracing_ddgi_update_cleanup_internal(raytracing_ddgi_update_internal_t* item) {
    if (!item || !item->initialized) return;

    // Pipeline destruction would go here
    item->pipeline = VK_NULL_HANDLE;
    item->descriptor_set = VK_NULL_HANDLE;

    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int raytracing_ddgi_update_init(void) {
    if (g_ddgi_update_ctx.initialized) return 0;

    g_ddgi_update_ctx.capacity = RAYTRACING_DDGI_UPDATE_DEFAULT_CAPACITY;
    g_ddgi_update_ctx.items = (raytracing_ddgi_update_internal_t*)calloc(g_ddgi_update_ctx.capacity, sizeof(raytracing_ddgi_update_internal_t));
    if (!g_ddgi_update_ctx.items) return -1;

    g_ddgi_update_ctx.count = 0;
    g_ddgi_update_ctx.initialized = true;

    return 0;
}

void raytracing_ddgi_update_set_renderer(VulkanRenderer* renderer) {
    g_ddgi_update_ctx.renderer = renderer;
}

void raytracing_ddgi_update_shutdown(void) {
    if (!g_ddgi_update_ctx.initialized) return;

    for (uint32_t i = 0; i < g_ddgi_update_ctx.count; i++) {
        raytracing_ddgi_update_cleanup_internal(&g_ddgi_update_ctx.items[i]);
    }

    free(g_ddgi_update_ctx.items);
    g_ddgi_update_ctx.items = NULL;
    g_ddgi_update_ctx.count = 0;
    g_ddgi_update_ctx.capacity = 0;
    g_ddgi_update_ctx.initialized = false;
}

int raytracing_ddgi_update_create(raytracing_ddgi_update_handle_t* out_handle, const raytracing_ddgi_update_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    if (!g_ddgi_update_ctx.initialized) return -2;

    if (g_ddgi_update_ctx.count >= g_ddgi_update_ctx.capacity) {
        uint32_t new_capacity = g_ddgi_update_ctx.capacity * 2;
        raytracing_ddgi_update_internal_t* new_items = (raytracing_ddgi_update_internal_t*)realloc(g_ddgi_update_ctx.items, new_capacity * sizeof(raytracing_ddgi_update_internal_t));
        if (!new_items) return -3;
        memset(new_items + g_ddgi_update_ctx.capacity, 0, g_ddgi_update_ctx.capacity * sizeof(raytracing_ddgi_update_internal_t));
        g_ddgi_update_ctx.items = new_items;
        g_ddgi_update_ctx.capacity = new_capacity;
    }

    uint32_t index = g_ddgi_update_ctx.count++;
    raytracing_ddgi_update_internal_t* item = &g_ddgi_update_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->pipeline = VK_NULL_HANDLE;
    item->descriptor_set = VK_NULL_HANDLE;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    LOG_INFO("Created DDGI update context for item %u", index);
    return 0;
}

void raytracing_ddgi_update_destroy(raytracing_ddgi_update_handle_t handle) {
    if (handle.id < g_ddgi_update_ctx.count) {
        raytracing_ddgi_update_cleanup_internal(&g_ddgi_update_ctx.items[handle.id]);
    }
}

int raytracing_ddgi_update_update(raytracing_ddgi_update_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_ddgi_update_ctx.count) return -1;
    
    raytracing_ddgi_update_internal_t* item = &g_ddgi_update_ctx.items[handle.id];
    if (!item->initialized) return -2;

    item->dirty = true;
    return 0;
}

int raytracing_ddgi_update_dispatch(raytracing_ddgi_update_handle_t handle, VkCommandBuffer cmd, uint32_t probe_count) {
    if (!g_ddgi_update_ctx.renderer) return -1;
    if (handle.id >= g_ddgi_update_ctx.count) return -2;
    
    raytracing_ddgi_update_internal_t* item = &g_ddgi_update_ctx.items[handle.id];
    if (!item->initialized) return -3;

    // Dispatch update rays (irradiance + visibility + distance)
    // For now we assume a simple 1D dispatch over total probes, or 3D if optimized
    vulkan_trace_rays(
        g_ddgi_update_ctx.renderer,
        cmd,
        item->pipeline,
        item->descriptor_set,
        probe_count, 
        1
    );

    return 0;
}

bool raytracing_ddgi_update_is_valid(raytracing_ddgi_update_handle_t handle) {
    if (handle.id >= g_ddgi_update_ctx.count) return false;
    return g_ddgi_update_ctx.items[handle.id].initialized;
}

int raytracing_ddgi_update_get_info(raytracing_ddgi_update_handle_t handle, raytracing_ddgi_update_info_t* out_info) {
    if (!out_info) return -1;
    if (handle.id >= g_ddgi_update_ctx.count) return -2;
    
    const raytracing_ddgi_update_internal_t* item = &g_ddgi_update_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    return 0;
}

void raytracing_ddgi_update_mark_dirty(raytracing_ddgi_update_handle_t handle) {
    if (handle.id < g_ddgi_update_ctx.count) {
        g_ddgi_update_ctx.items[handle.id].dirty = true;
    }
}

int raytracing_ddgi_update_process_pending(void) {
    return 0;
}

uint32_t raytracing_ddgi_update_get_count(void) {
    return g_ddgi_update_ctx.count;
}

size_t raytracing_ddgi_update_get_memory_usage(void) {
    size_t total = sizeof(g_ddgi_update_ctx);
    total += g_ddgi_update_ctx.capacity * sizeof(raytracing_ddgi_update_internal_t);
    return total;
}

void raytracing_ddgi_update_debug_print(void) {
    LOG_INFO("DDGI Update Stats: %u active contexts", g_ddgi_update_ctx.count);
}

/* End of ddgi_update.c */
