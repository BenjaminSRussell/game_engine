/**
 * =================================================================================================
 *                          PLATFORM ABSTRACTION LAYER (HAL)
 *                          Phase 9: Cross-Platform Support
 * =================================================================================================
 *
 * PURPOSE: Unified interface for OS-specific functionality
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Type Definitions
// -----------------------------------------------------------------------------

typedef struct PlatformWindow PlatformWindow;
typedef void *PlatformHandle;

// Window settings
typedef struct {
  const char *title;
  int width;
  int height;
  bool fullscreen;
  bool resizable;
  bool vsync;
} WindowConfig;

// Input types
typedef enum {
  KEY_UNKNOWN = -1,
  KEY_SPACE = 32,
  KEY_APOSTROPHE = 39,
  KEY_COMMA = 44,
  KEY_MINUS = 45,
  KEY_PERIOD = 46,
  KEY_SLASH = 47,
  KEY_0 = 48,
  KEY_A = 65,
  KEY_ESCAPE = 256,
  KEY_ENTER = 257,
  KEY_TAB = 258,
  KEY_BACKSPACE = 259,
  KEY_INSERT = 260,
  KEY_DELETE = 261,
  KEY_RIGHT = 262,
  KEY_LEFT = 263,
  KEY_DOWN = 264,
  KEY_UP = 265,
  // ... maps to standard scancodes (e.g. GLFW/SDL)
} KeyCode;

typedef enum {
  MOUSE_BUTTON_LEFT,
  MOUSE_BUTTON_RIGHT,
  MOUSE_BUTTON_MIDDLE
} MouseButton;

// File System
typedef enum {
  FILE_MODE_READ,
  FILE_MODE_WRITE,
  FILE_MODE_APPEND,
  FILE_MODE_READ_BINARY,
  FILE_MODE_WRITE_BINARY
} FileMode;

// Threading
typedef struct PlatformThread PlatformThread;
typedef struct PlatformMutex PlatformMutex;
typedef void (*ThreadFunc)(void *arg);

// -----------------------------------------------------------------------------
// Platform Interface
// -----------------------------------------------------------------------------

// System
// bool platform_init(void);
// void platform_shutdown(void);
double platform_get_time(
    void); // Keeping this one as it has ifdef implementation later
void platform_sleep(uint32_t milliseconds);
const char *platform_get_name(void);
int platform_get_cpu_count(void);

// Windowing
PlatformWindow *platform_window_create(const WindowConfig *config);
void platform_window_destroy(PlatformWindow *window);
bool platform_window_should_close(PlatformWindow *window);
void platform_window_poll_events(void);
void platform_window_swap_buffers(PlatformWindow *window);
void platform_window_set_title(PlatformWindow *window, const char *title);
void platform_window_get_size(PlatformWindow *window, int *width, int *height);
void *platform_get_native_window_handle(PlatformWindow *window);

// Input
bool platform_input_get_key(KeyCode key);
bool platform_input_get_key_down(KeyCode key);
bool platform_input_get_mouse_button(MouseButton button);
void platform_input_get_mouse_pos(double *x, double *y);
void platform_input_set_cursor_mode(bool locked);

// File System
PlatformHandle platform_file_open(const char *path, FileMode mode);
void platform_file_close(PlatformHandle file);
size_t platform_file_read(PlatformHandle file, void *buffer, size_t size);
size_t platform_file_write(PlatformHandle file, const void *buffer,
                           size_t size);
bool platform_file_exists(const char *path);
size_t platform_file_size(const char *path);

// Threading
PlatformThread *platform_thread_create(ThreadFunc func, void *arg);
void platform_thread_join(PlatformThread *thread);
void platform_thread_destroy(PlatformThread *thread);
PlatformMutex *platform_mutex_create(void);
void platform_mutex_lock(PlatformMutex *mutex);
void platform_mutex_unlock(PlatformMutex *mutex);
void platform_mutex_destroy(PlatformMutex *mutex);

// -----------------------------------------------------------------------------
// Platform Implementation (Stub for macOS/Posix)
// -----------------------------------------------------------------------------

#ifdef __APPLE__
#include <time.h>
#include <unistd.h>
// In a real engine, this would include Cocoa/NSWindow code or wrap GLFW/SDL
#endif

// Stub implementation for compilation
static inline bool platform_init(void) { return true; }
static inline void platform_shutdown(void) {}

#ifdef __APPLE__
static inline double platform_get_time(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
#else
static inline double platform_get_time(void) { return 0.0; }
#endif

// ... Other implementations would go here ...
// This file serves as the header/interface definition mainly.
