// src/engine/core/window.h
//
//  Purpose: Cross-platform window management using GLFW
//
#ifndef ENGINE_WINDOW_H
#define ENGINE_WINDOW_H

#include "../../include/common.h"

typedef struct GLFWwindow GLFWwindow;

typedef struct {
  GLFWwindow *handle;
  u32 width;
  u32 height;
  const char *title;
  bool fullscreen;
  bool vsync;
  bool resizable;
  void *user_data;
} Window;

// Window lifecycle
bool window_init(Window *window, u32 width, u32 height, const char *title,
                 bool fullscreen);
void window_shutdown(Window *window);
bool window_should_close(const Window *window);
void window_poll_events(void);
void window_swap_buffers(Window *window);

// Window properties
void window_set_title(Window *window, const char *title);
void window_set_size(Window *window, u32 width, u32 height);
void window_get_size(const Window *window, u32 *width, u32 *height);
void window_set_fullscreen(Window *window, bool fullscreen);
void window_set_vsync(Window *window, bool vsync);
void window_set_user_data(Window *window, void *data);
void *window_get_user_data(const Window *window);

// Vulkan surface creation
void *window_create_vulkan_surface(Window *window, void *instance);

// Callbacks
typedef void (*WindowResizeCallback)(Window *window, u32 width, u32 height);
typedef void (*WindowCloseCallback)(Window *window);
typedef void (*WindowFocusCallback)(Window *window, bool focused);

void window_set_resize_callback(Window *window, WindowResizeCallback callback);
void window_set_close_callback(Window *window, WindowCloseCallback callback);
void window_set_focus_callback(Window *window, WindowFocusCallback callback);

#endif // ENGINE_WINDOW_H
