#ifndef ENGINE_INIT_H
#define ENGINE_INIT_H

#include <stdbool.h>
#include <stdint.h>

// Platform definitions
typedef enum {
  ENGINE_PLATFORM_DESKTOP,
  ENGINE_PLATFORM_WEB,
  ENGINE_PLATFORM_MOBILE,
  ENGINE_PLATFORM_CONSOLE
} EnginePlatform;

// Renderer Backend definitions
typedef enum {
  RENDERER_BACKEND_VULKAN,
  RENDERER_BACKEND_METAL,
  RENDERER_BACKEND_OPENGL,
  RENDERER_BACKEND_DIRECTX
} RendererBackend;

// Window Configuration
typedef struct {
  const char *title;
  uint32_t width;
  uint32_t height;
  bool fullscreen;
  bool resizable;
  bool vsync;
} WindowConfig;

// Main Engine Configuration
typedef struct {
  EnginePlatform platform;
  RendererBackend renderer_backend;
  WindowConfig window;

  // Memory settings
  uint64_t main_memory_size;
  uint64_t temp_memory_size;

  // Threading settings
  uint32_t thread_count;

  // Debug settings
  bool enable_validation_layers;
  bool enable_debug_logging;
} EngineConfig;

// Initialization Result Codes
typedef enum {
  ENGINE_INIT_SUCCESS = 0,
  ENGINE_INIT_FAILURE_UNKNOWN = -1,
  ENGINE_INIT_FAILURE_MEMORY = -2,
  ENGINE_INIT_FAILURE_WINDOW = -3,
  ENGINE_INIT_FAILURE_RENDERER = -4,
  ENGINE_INIT_FAILURE_AUDIO = -5,
  ENGINE_INIT_FAILURE_SUBSYSTEM = -6
} EngineInitResult;

// Public API
/**
 * @brief Initialize the engine with the given configuration.
 *
 * @param config Pointer to configuration structure.
 * @return EngineInitResult SUCCESS on success, error code otherwise.
 */
EngineInitResult engine_init(const EngineConfig *config);

/**
 * @brief Shutdown the engine and release all resources.
 */
void engine_shutdown(void);

/**
 * @brief Check if the engine is currently running.
 *
 * @return true if running, false otherwise.
 */
bool engine_is_running(void);

/**
 * @brief Register a critical error handler.
 *
 * @param handler Function pointer to handler.
 */
void engine_register_error_handler(void (*handler)(const char *message));

#endif // ENGINE_INIT_H
