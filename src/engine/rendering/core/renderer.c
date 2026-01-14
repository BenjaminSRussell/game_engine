// src/engine/rendering/core/renderer.c
// Core Renderer System - Main rendering pipeline orchestrator

#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <core/time_system.h>
#include <core/window.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../framebuffer.h"
#include "../render_pipeline.h"
#include "backend/metal/mtl_command_buffer.h"
#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_encoder.h"

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
  uint64_t frame_start_time_ns;
  float total_frame_time_ms;

  // Render targets
  uint32_t width;
  uint32_t height;

  // Configuration
  bool enable_debug;
  bool enable_stats;
  bool is_initialized;

  // Validation state
  bool validation_enabled;
  uint64_t validation_errors;
  uint64_t initialization_time;
  uint64_t last_validation_time;
} Renderer;

static Renderer *g_renderer = NULL;

// Forward declarations for validation functions
static bool renderer_validate_dimensions(uint32_t width, uint32_t height);
static bool renderer_validate_window(Window *window);
static bool renderer_validate_device(metal_device_t *device);
static bool renderer_validate_framebuffer(Framebuffer *framebuffer);
static void renderer_log_validation_error(Renderer *renderer,
                                          const char *error);
static uint64_t renderer_get_timestamp(void);
void renderer_resize(Renderer *renderer, uint32_t width, uint32_t height);

// ============================================================================
// Renderer API
// ============================================================================

Renderer *renderer_create(uint32_t width, uint32_t height) {
  // Validate dimensions
  if (!renderer_validate_dimensions(width, height)) {
    LOG_ERROR("Invalid renderer dimensions: %ux%u", width, height);
    return NULL;
  }

  Renderer *renderer = calloc(1, sizeof(Renderer));
  if (!renderer) {
    LOG_ERROR("Failed to allocate renderer");
    return NULL;
  }

  // Initialize validation state
  renderer->validation_enabled = true;
  renderer->validation_errors = 0;
  renderer->initialization_time = renderer_get_timestamp();
  renderer->last_validation_time = renderer->initialization_time;

  // Create window
  renderer->window = calloc(1, sizeof(Window));
  if (!renderer->window) {
    LOG_ERROR("Failed to allocate window structure");
    free(renderer);
    return NULL;
  }

  if (!window_init(renderer->window, width, height, "VoxelForge Engine",
                   false)) {
    LOG_ERROR("Failed to create window");
    free(renderer->window);
    free(renderer);
    return NULL;
  }

  // Validate window
  if (!renderer_validate_window(renderer->window)) {
    renderer_log_validation_error(renderer, "Window validation failed");
    window_shutdown(renderer->window);
    free(renderer->window);
    free(renderer);
    return NULL;
  }

  // Create Metal device
  renderer->device = metal_device_create_system_default();
  if (!renderer->device) {
    LOG_ERROR("Failed to create Metal device");
    renderer_log_validation_error(renderer, "Metal device creation failed");
    window_shutdown(renderer->window);
    free(renderer->window);
    free(renderer);
    return NULL;
  }

  // Validate device
  if (!renderer_validate_device(renderer->device)) {
    renderer_log_validation_error(renderer, "Metal device validation failed");
    metal_device_destroy(renderer->device);
    window_shutdown(renderer->window);
    free(renderer->window);
    free(renderer);
    return NULL;
  }

  // Create main framebuffer
  renderer->main_framebuffer = framebuffer_create(width, height);
  if (!renderer->main_framebuffer) {
    LOG_ERROR("Failed to create main framebuffer");
    renderer_log_validation_error(renderer, "Main framebuffer creation failed");
    metal_device_destroy(renderer->device);
    window_shutdown(renderer->window);
    free(renderer->window);
    free(renderer);
    return NULL;
  }

  // Validate framebuffer
  if (!renderer_validate_framebuffer(renderer->main_framebuffer)) {
    renderer_log_validation_error(renderer,
                                  "Main framebuffer validation failed");
    framebuffer_destroy(renderer->main_framebuffer);
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

  LOG_INFO("Renderer created successfully (%ux%u) with validation enabled",
           width, height);
  return renderer;
}

void renderer_destroy(Renderer *renderer) {
  if (!renderer)
    return;

  // Report validation statistics before destruction
  if (renderer->validation_enabled) {
    LOG_INFO("Renderer Validation Statistics:");
    LOG_INFO("  Validation errors: %lu", renderer->validation_errors);
    LOG_INFO("  Uptime: %lu seconds",
             (renderer_get_timestamp() - renderer->initialization_time) /
                 1000000000ULL);
    LOG_INFO("  Frames rendered: %u", renderer->stats.frames_rendered);

    if (renderer->stats.frames_rendered > 0) {
      LOG_INFO("  Average frame time: %.2f ms",
               renderer->total_frame_time_ms / renderer->stats.frames_rendered);
    }
  }

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

static void renderer_window_resize_callback(Window *window, u32 width,
                                            u32 height) {
  if (g_renderer) {
    renderer_resize(g_renderer, width, height);
  }
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
  window_set_resize_callback(renderer->window, renderer_window_resize_callback);

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
  renderer->stats.frame_time_ms = 0.0f;
  renderer->stats.cpu_time_ms = 0.0f;
  renderer->stats.gpu_time_ms = 0.0f;
  renderer->frame_start_time_ns = get_time_nanos();

  renderer->frame_index++;
}

void renderer_end_frame(Renderer *renderer) {
  if (!renderer)
    return;

  // End frame timing
  if (renderer->frame_start_time_ns > 0) {
    uint64_t current_time = get_time_nanos();
    float frame_time_ms =
        nanos_to_ms(current_time - renderer->frame_start_time_ns);
    renderer->stats.frame_time_ms = frame_time_ms;
    renderer->stats.cpu_time_ms = frame_time_ms;
    renderer->stats.gpu_time_ms = 0.0f;
    renderer->total_frame_time_ms += frame_time_ms;
  }

  renderer->stats.frames_rendered++;

  if (renderer->enable_stats) {
    LOG_DEBUG("Frame %u: %u draw calls, %u triangles, %.2f ms",
              renderer->frame_index, renderer->stats.draw_calls,
              renderer->stats.triangles_drawn, renderer->stats.frame_time_ms);
  }
}

void renderer_set_vsync(Renderer *renderer, bool enabled) {
  if (!renderer)
    return;

  renderer->vsync_enabled = enabled;
  if (renderer->window) {
    window_set_vsync(renderer->window, enabled);
  }

  LOG_INFO("VSync %s", enabled ? "enabled" : "disabled");
}

void renderer_resize(Renderer *renderer, uint32_t width, uint32_t height) {
  if (!renderer || width == 0 || height == 0)
    return;

  uint32_t old_width = renderer->width;
  uint32_t old_height = renderer->height;

  renderer->width = width;
  renderer->height = height;

  if (renderer->main_framebuffer) {
    if (!framebuffer_resize(renderer->main_framebuffer, width, height)) {
      LOG_ERROR("Failed to resize main framebuffer to %ux%u", width, height);
    }
  }

  if (renderer->gbuffer) {
    if (!framebuffer_resize(renderer->gbuffer, width, height)) {
      LOG_ERROR("Failed to resize gbuffer to %ux%u", width, height);
    }
  }

  LOG_INFO("Framebuffer resized from %ux%u to %ux%u", old_width, old_height,
           width, height);

  // Update projection matrices for new aspect ratio
  // update_projection_matrices(renderer, width, height);

  LOG_INFO("Renderer resized to %ux%u", width, height);
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

void renderer_get_dimensions(Renderer *renderer, uint32_t *width,
                             uint32_t *height) {
  if (!renderer)
    return;

  if (width)
    *width = renderer->width;
  if (height)
    *height = renderer->height;
}

// ============================================================================
// Global Accessors
// ============================================================================

Renderer *renderer_get_global(void) { return g_renderer; }

bool renderer_is_initialized(void) {
  return g_renderer != NULL && g_renderer->is_initialized;
}

Window *renderer_get_window(Renderer *renderer) {
  if (!renderer)
    return NULL;

  return renderer->window;
}

bool renderer_should_close(Renderer *renderer) {
  if (!renderer || !renderer->window)
    return true;

  return window_should_close(renderer->window);
}

void renderer_poll_events(Renderer *renderer) {
  if (renderer) {
    window_poll_events();
  }
}

void renderer_present(Renderer *renderer) {
  if (!renderer || !renderer->window)
    return;

  window_swap_buffers(renderer->window);
}

// -----------------------------------------------------------------------------
// Renderer Validation Implementation
// -----------------------------------------------------------------------------

static bool renderer_validate_dimensions(uint32_t width, uint32_t height) {
  // Check minimum dimensions
  if (width < 64 || height < 64) {
    LOG_ERROR("Renderer dimensions too small: %ux%u (minimum 64x64)", width,
              height);
    return false;
  }

  // Check maximum dimensions (reasonable limits)
  if (width > 16384 || height > 16384) {
    LOG_ERROR("Renderer dimensions too large: %ux%u (maximum 16384x16384)",
              width, height);
    return false;
  }

  // Check for power-of-two dimensions (optional but recommended for some GPUs)
  // This is just a warning, not an error
  if ((width & (width - 1)) != 0 || (height & (height - 1)) != 0) {
    LOG_WARN("Renderer dimensions are not power-of-two: %ux%u", width, height);
  }

  return true;
}

static bool renderer_validate_window(Window *window) {
  if (!window) {
    LOG_ERROR("Window is NULL");
    return false;
  }

  // Check if window is properly initialized
  if (!window->is_valid) {
    LOG_ERROR("Window is not marked as valid");
    return false;
  }

  // Check window dimensions
  if (window->width == 0 || window->height == 0) {
    LOG_ERROR("Window has invalid dimensions: %ux%u", window->width,
              window->height);
    return false;
  }

  return true;
}

static bool renderer_validate_device(metal_device_t *device) {
  if (!device) {
    LOG_ERROR("Metal device is NULL");
    return false;
  }

  // Try to get device properties to validate it's working
  // This is a basic validation - in a real implementation,
  // you would check specific device capabilities
  return true;
}

static bool renderer_validate_framebuffer(Framebuffer *framebuffer) {
  if (!framebuffer) {
    LOG_ERROR("Framebuffer is NULL");
    return false;
  }

  // Check framebuffer dimensions
  if (framebuffer->width == 0 || framebuffer->height == 0) {
    LOG_ERROR("Framebuffer has invalid dimensions: %ux%u", framebuffer->width,
              framebuffer->height);
    return false;
  }

  return true;
}

static void renderer_log_validation_error(Renderer *renderer,
                                          const char *error) {
  if (!renderer || !error)
    return;

  renderer->validation_errors++;
  LOG_ERROR("Renderer Validation Error [%lu]: %s", renderer->validation_errors,
            error);
}

static uint64_t renderer_get_timestamp(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Public validation API
bool renderer_validate_state(Renderer *renderer) {
  if (!renderer || !renderer->validation_enabled) {
    return true;
  }

  bool valid = true;
  uint64_t current_time = renderer_get_timestamp();

  // Validate window
  if (!renderer_validate_window(renderer->window)) {
    renderer_log_validation_error(
        renderer, "Window validation failed during state check");
    valid = false;
  }

  // Validate device
  if (!renderer_validate_device(renderer->device)) {
    renderer_log_validation_error(
        renderer, "Device validation failed during state check");
    valid = false;
  }

  // Validate framebuffer
  if (!renderer_validate_framebuffer(renderer->main_framebuffer)) {
    renderer_log_validation_error(
        renderer, "Framebuffer validation failed during state check");
    valid = false;
  }

  // Check for excessive validation errors
  if (renderer->validation_errors > 100) {
    renderer_log_validation_error(renderer,
                                  "Excessive validation errors detected");
    valid = false;
  }

  // Check for stale state (no frames rendered for extended period)
  uint64_t time_since_init = current_time - renderer->initialization_time;
  if (time_since_init > 60000000000ULL &&
      renderer->stats.frames_rendered == 0) { // 1 minute
    renderer_log_validation_error(renderer,
                                  "No frames rendered in first minute");
    valid = false;
  }

  renderer->last_validation_time = current_time;
  return valid;
}

void renderer_enable_validation(Renderer *renderer, bool enabled) {
  if (!renderer)
    return;

  renderer->validation_enabled = enabled;
  renderer->last_validation_time = renderer_get_timestamp();
  LOG_INFO("Renderer validation %s", enabled ? "enabled" : "disabled");
}

uint64_t renderer_get_validation_errors(Renderer *renderer) {
  return renderer ? renderer->validation_errors : 0;
}

void renderer_get_validation_stats(Renderer *renderer,
                                   uint64_t *validation_errors,
                                   uint64_t *initialization_time,
                                   uint64_t *last_validation_time) {
  if (!renderer)
    return;

  if (validation_errors)
    *validation_errors = renderer->validation_errors;
  if (initialization_time)
    *initialization_time = renderer->initialization_time;
  if (last_validation_time)
    *last_validation_time = renderer->last_validation_time;
}

void renderer_reset_validation_stats(Renderer *renderer) {
  if (!renderer)
    return;

  renderer->validation_errors = 0;
  renderer->last_validation_time = renderer_get_timestamp();
  LOG_INFO("Renderer validation statistics reset");
}
