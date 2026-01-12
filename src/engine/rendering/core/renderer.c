// src/engine/rendering/core/renderer.c
// Core Renderer System - Main rendering pipeline orchestrator

#include <core/logger.h>
#include <core/memory.h>
#include <core/window.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_command_buffer.h"
#include "backend/metal/mtl_encoder.h"
#include "../framebuffer.h"
#include "../render_pipeline.h"

// ============================================================================
// Renderer Types
// ============================================================================

typedef struct {
    uint32_t draw_calls;
    uint32_t triangles_drawn;
    uint32_t frames_rendered;
    float frame_time_ms;
    float cpu_time_ms;
    float gpu_time_ms;
} RenderStats;

typedef struct {
    metal_device_t *device;
    Window *window;
    Framebuffer *main_framebuffer;
    Framebuffer *gbuffer;
    
    RenderStats stats;
    bool vsync_enabled;
    uint32_t frame_index;
    
    // Render targets
    uint32_t width;
    uint32_t height;
    
    // Configuration
    bool enable_debug;
    bool enable_stats;
    bool is_initialized;
} Renderer;

static Renderer *g_renderer = NULL;

// ============================================================================
// Renderer API
// ============================================================================

Renderer *renderer_create(uint32_t width, uint32_t height) {
    Renderer *renderer = calloc(1, sizeof(Renderer));
    if (!renderer) {
        LOG_ERROR("Failed to allocate renderer");
        return NULL;
    }
    
    // Create window
    renderer->window = calloc(1, sizeof(Window));
    if (!renderer->window || !window_init(renderer->window, width, height, "VoxelForge Engine", false)) {
        LOG_ERROR("Failed to create window");
        free(renderer->window);
        free(renderer);
        return NULL;
    }
    
    // Create Metal device
    renderer->device = metal_device_create_system_default();
    if (!renderer->device) {
        LOG_ERROR("Failed to create Metal device");
        window_shutdown(renderer->window);
        free(renderer->window);
        free(renderer);
        return NULL;
    }
    
    // Create main framebuffer
    renderer->main_framebuffer = framebuffer_create(width, height);
    if (!renderer->main_framebuffer) {
        LOG_ERROR("Failed to create main framebuffer");
        metal_device_destroy(renderer->device);
        window_shutdown(renderer->window);
        free(renderer->window);
        free(renderer);
        return NULL;
    }
    
    renderer->width = width;
    renderer->height = height;
    renderer->vsync_enabled = true;
    renderer->enable_debug = false;
    renderer->enable_stats = true;
    renderer->frame_index = 0;
    renderer->is_initialized = false;
    
    LOG_INFO("Renderer created successfully (%ux%u)", width, height);
    return renderer;
}

void renderer_destroy(Renderer *renderer) {
    if (!renderer)
        return;
    
    if (renderer->main_framebuffer) {
        framebuffer_destroy(renderer->main_framebuffer);
    }
    
    if (renderer->gbuffer) {
        framebuffer_destroy(renderer->gbuffer);
    }
    
    if (renderer->device) {
        metal_device_destroy(renderer->device);
    }
    
    if (renderer->window) {
        window_shutdown(renderer->window);
        free(renderer->window);
    }
    
    free(renderer);
    
    if (g_renderer == renderer) {
        g_renderer = NULL;
    }
    
    LOG_INFO("Renderer destroyed");
}

bool renderer_initialize(Renderer *renderer) {
    if (!renderer)
        return false;
    
    if (renderer->is_initialized) {
        LOG_WARNING("Renderer already initialized");
        return true;
    }
    
    g_renderer = renderer;
    
    // Initialize window vsync
    window_set_vsync(renderer->window, renderer->vsync_enabled);
    
    // Set up window resize callback
    window_set_resize_callback(renderer->window, [](Window *window, u32 width, u32 height) {
        if (g_renderer) {
            renderer_resize(g_renderer, width, height);
        }
    });
    
    // Initialize render pipeline components
    // TODO: Initialize shader systems
    // TODO: Initialize material systems  
    // TODO: Initialize geometry systems
    
    renderer->is_initialized = true;
    LOG_INFO("Renderer initialized successfully");
    return true;
}

void renderer_begin_frame(Renderer *renderer) {
    if (!renderer)
        return;
    
    // Reset frame stats
    renderer->stats.draw_calls = 0;
    renderer->stats.triangles_drawn = 0;
    
    // Begin frame timing
    // TODO: Implement frame timing
    
    renderer->frame_index++;
}

void renderer_end_frame(Renderer *renderer) {
    if (!renderer)
        return;
    
    // End frame timing
    // TODO: Calculate frame times
    
    renderer->stats.frames_rendered++;
    
    if (renderer->enable_stats) {
        LOG_DEBUG("Frame %u: %u draw calls, %u triangles, %.2f ms",
                 renderer->frame_index,
                 renderer->stats.draw_calls,
                 renderer->stats.triangles_drawn,
                 renderer->stats.frame_time_ms);
    }
}

void renderer_set_vsync(Renderer *renderer, bool enabled) {
    if (!renderer)
        return;
    
    renderer->vsync_enabled = enabled;
    // TODO: Implement vsync control
}

void renderer_resize(Renderer *renderer, uint32_t width, uint32_t height) {
    if (!renderer || width == 0 || height == 0)
        return;
    
    renderer->width = width;
    renderer->height = height;
    
    // Resize framebuffers
    if (renderer->main_framebuffer) {
        // TODO: Implement framebuffer resize
        LOG_INFO("Renderer resized to %ux%u", width, height);
    }
}

void renderer_set_debug_mode(Renderer *renderer, bool enabled) {
    if (!renderer)
        return;
    
    renderer->enable_debug = enabled;
    LOG_INFO("Debug mode %s", enabled ? "enabled" : "disabled");
}

RenderStats renderer_get_stats(Renderer *renderer) {
    if (!renderer)
        return (RenderStats){0};
    
    return renderer->stats;
}

uint32_t renderer_get_frame_index(Renderer *renderer) {
    if (!renderer)
        return 0;
    
    return renderer->frame_index;
}

void renderer_get_dimensions(Renderer *renderer, uint32_t *width, uint32_t *height) {
    if (!renderer)
        return;
    
    if (width) *width = renderer->width;
    if (height) *height = renderer->height;
}

// ============================================================================
// Global Accessors
// ============================================================================

Renderer *renderer_get_global(void) {
    return g_renderer;
}

bool renderer_is_initialized(void) {
    return g_renderer != NULL;
}
