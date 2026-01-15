#pragma once

#include <stdbool.h>
#include <stdint.h>

// Basic type definitions if not using core/types.h
#ifndef u32
typedef uint32_t u32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint64_t u64;
typedef int32_t i32;
typedef float f32;
typedef double f64;
#endif

// Platform Subsystems
typedef enum {
  PLATFORM_SUBSYSTEM_WINDOW = 1 << 0,
  PLATFORM_SUBSYSTEM_INPUT = 1 << 1,
  PLATFORM_SUBSYSTEM_FILE = 1 << 2,
  PLATFORM_SUBSYSTEM_ALL = 0xFF
} PlatformSubsystem;

// Opaque handles
typedef struct PlatformWindow PlatformWindow;
typedef struct PlatformContext PlatformContext;

// Initialization
PlatformContext *platform_init(PlatformSubsystem subsystems);
void platform_shutdown(PlatformContext *ctx);
void platform_update(PlatformContext *ctx);

// Window Management
typedef struct {
  const char *title;
  u32 width;
  u32 height;
  bool fullscreen;
  bool vsync;
  bool resizable;
  void *native_handle; // Optional, for hosted mode
} WindowConfig;

PlatformWindow *platform_window_create(PlatformContext *ctx,
                                       const WindowConfig *config);
void platform_window_destroy(PlatformWindow *window);
bool platform_window_should_close(PlatformWindow *window);
void platform_window_poll_events(void);
void platform_window_swap_buffers(PlatformWindow *window);

// Window Accessors
void *platform_window_get_native_handle(PlatformWindow *window);
void platform_window_get_size(PlatformWindow *window, u32 *width, u32 *height);
void platform_window_set_title(PlatformWindow *window, const char *title);
void platform_window_set_fullscreen(PlatformWindow *window, bool fullscreen);

// Input System (Basic Interface - detailed in unified_input.h if separate)
// ... Input API definitions will go here in the next steps ...

// Filesystem (Basic Interface)
// ... Filesystem API definitions will go here ...
