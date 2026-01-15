#pragma once

#include <stdbool.h>
#include <stdint.h>

// Basic type definitions
#ifndef u32
typedef uint32_t u32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint64_t u64;
typedef int32_t i32;
typedef float f32;
typedef double f64;
#endif

// Forward declarations
typedef struct RenderContext RenderContext;
typedef struct RenderDevice RenderDevice;
typedef struct RenderSwapchain RenderSwapchain;

// Render backend types
typedef enum {
  RENDER_BACKEND_VULKAN,
  RENDER_BACKEND_METAL,
  RENDER_BACKEND_D3D12,
  RENDER_BACKEND_OPENGL,
  RENDER_BACKEND_AUTO // Auto-select based on platform
} RenderBackend;

// Resource formats
typedef enum {
  RENDER_FORMAT_UNDEFINED,
  RENDER_FORMAT_R8_UNORM,
  RENDER_FORMAT_R8G8B8A8_UNORM,
  RENDER_FORMAT_R8G8B8A8_SRGB,
  RENDER_FORMAT_B8G8R8A8_UNORM,
  RENDER_FORMAT_B8G8R8A8_SRGB,
  RENDER_FORMAT_R16G16B16A16_FLOAT,
  RENDER_FORMAT_R32G32B32A32_FLOAT,
  RENDER_FORMAT_D24_UNORM_S8_UINT,
  RENDER_FORMAT_D32_FLOAT,
  RENDER_FORMAT_COUNT
} RenderFormat;

// Present modes
typedef enum {
  RENDER_PRESENT_MODE_IMMEDIATE, // No VSync
  RENDER_PRESENT_MODE_FIFO,      // VSync (60Hz)
  RENDER_PRESENT_MODE_MAILBOX    // VSync with triple buffering
} RenderPresentMode;

// Renderer configuration
typedef struct {
  RenderBackend backend;
  void *window_handle; // Platform-specific window (from Platform System)

  // Swapchain config
  u32 width;
  u32 height;
  RenderFormat format;
  RenderPresentMode present_mode;
  u32 swapchain_image_count; // 2-3 recommended

  // Features
  bool enable_validation; // Debug validation layers
  bool enable_profiling;
  bool enable_vsync;
} RenderConfig;

// Renderer statistics
typedef struct {
  u64 frame_count;
  f32 frame_time_ms;
  f32 gpu_time_ms;
  u32 draw_calls;
  u32 triangles;
  u64 gpu_memory_used;
  u64 gpu_memory_total;
} RenderStats;

// Initialization
RenderContext *render_init(const RenderConfig *config);
void render_shutdown(RenderContext *ctx);

// Frame lifecycle
void render_begin_frame(RenderContext *ctx);
void render_end_frame(RenderContext *ctx);
void render_present(RenderContext *ctx);

// State queries
RenderDevice *render_get_device(RenderContext *ctx);
RenderSwapchain *render_get_swapchain(RenderContext *ctx);
void render_get_stats(RenderContext *ctx, RenderStats *out_stats);
bool render_is_initialized(RenderContext *ctx);

// Configuration
void render_resize(RenderContext *ctx, u32 width, u32 height);
void render_set_vsync(RenderContext *ctx, bool enabled);
