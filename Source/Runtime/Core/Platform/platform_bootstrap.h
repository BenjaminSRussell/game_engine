#ifndef PLATFORM_BOOTSTRAP_H
#define PLATFORM_BOOTSTRAP_H

#include <stdbool.h>
#include <stdint.h>

// Initialize platform systems (Window, Input layer, OS services)
// Returns true on success
bool platform_init(int width, int height, const char *title);

// Shutdown platform systems
void platform_shutdown(void);

// Poll for OS events (input, window resize, etc.)
void platform_poll_events(void);

// Get high-resolution time in seconds
double platform_get_time(void);

// Check if window should close
bool platform_window_should_close(void);

// Get internal window handle (void* to avoid GLFW dependency in header if
// possible, though we usually need it for renderer init)
void *platform_get_window_handle(void);

// Types for window sizing
typedef struct {
  int width;
  int height;
} WindowSize;

WindowSize platform_get_window_size(void);

#endif
