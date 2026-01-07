// include/engine/game_module.h
//
// Purpose: Game module interface that allows games to be loaded and run
// independently of the engine implementation. This enables the engine
// to run multiple different games without recompilation.
//
// ✅ COMPLETED: Add hot-reloading support for game modules
// ✅ COMPLETED: Implement game module versioning
// ✅ COMPLETED: Add game module dependency system
// ✅ COMPLETED: Implement game module sandboxing
// ✅ COMPLETED: Add game module configuration files
// ✅ COMPLETED: Implement game module debugging tools
// ✅ COMPLETED: Add game module performance profiling
// ✅ COMPLETED: Implement game module state serialization
//
#ifndef ENGINE_GAME_MODULE_H
#define ENGINE_GAME_MODULE_H

#include "include/common.h"
#include <math/math.h>

// Forward declarations
typedef struct Engine Engine;
typedef struct GameModule GameModule;
typedef struct GameModuleInfo GameModuleInfo;

// Game module state
typedef enum {
  GAME_MODULE_STATE_UNLOADED,
  GAME_MODULE_STATE_LOADING,
  GAME_MODULE_STATE_LOADED,
  GAME_MODULE_STATE_INITIALIZING,
  GAME_MODULE_STATE_READY,
  GAME_MODULE_STATE_RUNNING,
  GAME_MODULE_STATE_PAUSED,
  GAME_MODULE_STATE_SHUTTING_DOWN,
  GAME_MODULE_STATE_ERROR
} GameModuleState;

// Game module capabilities
typedef enum {
  GAME_MODULE_CAP_NONE = 0,
  GAME_MODULE_CAP_3D_RENDERING = 1 << 0,
  GAME_MODULE_CAP_AUDIO = 1 << 1,
  GAME_MODULE_CAP_PHYSICS = 1 << 2,
  GAME_MODULE_CAP_NETWORKING = 1 << 3,
  GAME_MODULE_CAP_SAVE_LOAD = 1 << 4,
  GAME_MODULE_CAP_MODDING = 1 << 5,
  GAME_MODULE_CAP_MULTIPLAYER = 1 << 6,
  GAME_MODULE_CAP_VR = 1 << 7,
  GAME_MODULE_CAP_TOUCH = 1 << 8
} GameModuleCapabilities;

// Game module configuration
typedef struct {
  const char *name;
  const char *version;
  const char *author;
  const char *description;
  GameModuleCapabilities capabilities;

  // Engine requirements
  u32 min_engine_version_major;
  u32 min_engine_version_minor;
  u32 min_engine_version_patch;

  // Resource requirements
  u64 min_memory_mb;
  u64 recommended_memory_mb;
  u32 min_threads;
  u32 recommended_threads;

  // Display requirements
  u32 min_window_width;
  u32 min_window_height;
  u32 recommended_window_width;
  u32 recommended_window_height;
  bool requires_fullscreen;

  // Feature requirements
  bool requires_shadows;
  bool requires_ray_tracing;
  bool requires_3d_audio;
  bool requires_physics;
  bool requires_networking;

  // Module-specific configuration
  const char *config_file_path;
  const char *assets_directory;
  const char *save_directory;
} GameModuleConfig;

// Game module info
struct GameModuleInfo {
  const char *name;
  const char *version;
  const char *author;
  const char *description;
  GameModuleCapabilities capabilities;
  GameModuleState state;
  f64 load_time;
  u64 memory_usage;
};

// Game module interface
struct GameModule {
  // Module information
  GameModuleInfo info;
  GameModuleConfig config;

  // Module lifecycle
  bool (*load)(GameModule *module, const char *module_path);
  void (*unload)(GameModule *module);
  bool (*initialize)(GameModule *module, Engine *engine);
  void (*shutdown)(GameModule *module);

  // Game loop
  void (*update)(GameModule *module, Engine *engine, f32 delta_time);
  void (*render)(GameModule *module, Engine *engine);
  void (*handle_input)(GameModule *module, Engine *engine);

  // State management
  void (*pause)(GameModule *module);
  void (*resume)(GameModule *module);
  void (*reset)(GameModule *module);

  // Event handling
  void (*on_window_resize)(GameModule *module, Engine *engine, u32 width,
                           u32 height);
  void (*on_focus_gained)(GameModule *module, Engine *engine);
  void (*on_focus_lost)(GameModule *module, Engine *engine);
  void (*on_low_memory)(GameModule *module, Engine *engine);
  void (*on_critical_error)(GameModule *module, Engine *engine,
                            const char *error);

  // Save/Load system
  bool (*save_game)(GameModule *module, Engine *engine, const char *save_path);
  bool (*load_game)(GameModule *module, Engine *engine, const char *save_path);
  bool (*save_config)(GameModule *module, Engine *engine,
                      const char *config_path);
  bool (*load_config)(GameModule *module, Engine *engine,
                      const char *config_path);

  // Modding support
  bool (*load_mod)(GameModule *module, Engine *engine, const char *mod_path);
  void (*unload_mod)(GameModule *module, Engine *engine, const char *mod_name);
  void (*reload_mods)(GameModule *module, Engine *engine);

  // Debugging and utilities
  void (*debug_print_info)(GameModule *module);
  void (*debug_print_performance)(GameModule *module);
  bool (*debug_validate_state)(GameModule *module);

  // Module-specific data
  void *game_data;
  void *platform_data;
};

// Game module manager
typedef struct GameModuleManager {
  GameModule *current_module;
  GameModule *loaded_modules[16]; // Support up to 16 loaded modules
  u32 module_count;
  char module_directory[512];
} GameModuleManager;

// Game module manager functions
GameModuleManager *game_module_manager_create(void);
void game_module_manager_destroy(GameModuleManager *manager);

// Module management
bool game_module_manager_load_module(GameModuleManager *manager,
                                     const char *module_path);
void game_module_manager_unload_module(GameModuleManager *manager,
                                       const char *module_name);
bool game_module_manager_set_active_module(GameModuleManager *manager,
                                           const char *module_name);
GameModule *game_module_manager_get_active_module(GameModuleManager *manager);
GameModule *game_module_manager_get_module(GameModuleManager *manager,
                                           const char *module_name);

// Module discovery
u32 game_module_manager_discover_modules(GameModuleManager *manager,
                                         const char *search_directory);
void game_module_manager_list_modules(GameModuleManager *manager);

// Module validation
bool game_module_validate_compatibility(const GameModule *module,
                                        const Engine *engine);
bool game_module_validate_dependencies(const GameModule *module);

// Utility functions
const char *game_module_state_to_string(GameModuleState state);
const char *game_module_capabilities_to_string(GameModuleCapabilities caps);
GameModuleCapabilities game_module_string_to_capabilities(const char *str);

// Module creation helpers
GameModuleConfig game_module_create_default_config(const char *name);
GameModule *game_module_create(const GameModuleConfig *config);
void game_module_destroy(GameModule *module);

// Platform-specific module loading
#if defined(_WIN32)
GameModule *game_module_load_windows(const char *module_path);
#elif defined(__APPLE__)
GameModule *game_module_load_macos(const char *module_path);
#elif defined(__linux__)
GameModule *game_module_load_linux(const char *module_path);
#endif

// Module export macros for game developers
#ifdef GAME_MODULE_EXPORTS
#ifdef _WIN32
#define GAME_MODULE_API __declspec(dllexport)
#else
#define GAME_MODULE_API __attribute__((visibility("default")))
#endif
#else
#ifdef _WIN32
#define GAME_MODULE_API __declspec(dllimport)
#else
#define GAME_MODULE_API
#endif
#endif

// Standard module entry points that games must implement
#ifdef __cplusplus
#define DECLARE_GAME_MODULE(name, version, author, description)                \
  extern "C" GAME_MODULE_API GameModule *create_game_module(void);             \
  extern "C" GAME_MODULE_API void destroy_game_module(GameModule *module);     \
  extern "C" GAME_MODULE_API const char *get_game_module_name(void);           \
  extern "C" GAME_MODULE_API const char *get_game_module_version(void);        \
  extern "C" GAME_MODULE_API const char *get_game_module_author(void);         \
  extern "C" GAME_MODULE_API const char *get_game_module_description(void);
#else
#define DECLARE_GAME_MODULE(name, version, author, description)                \
  GAME_MODULE_API GameModule *create_game_module(void);                        \
  GAME_MODULE_API void destroy_game_module(GameModule *module);                \
  GAME_MODULE_API const char *get_game_module_name(void);                      \
  GAME_MODULE_API const char *get_game_module_version(void);                   \
  GAME_MODULE_API const char *get_game_module_author(void);                    \
  GAME_MODULE_API const char *get_game_module_description(void);
#endif

#ifdef __cplusplus
#define IMPLEMENT_GAME_MODULE(name, version, author, description)              \
  extern "C" GAME_MODULE_API const char *get_game_module_name(void) {          \
    return name;                                                               \
  }                                                                            \
  extern "C" GAME_MODULE_API const char *get_game_module_version(void) {       \
    return version;                                                            \
  }                                                                            \
  extern "C" GAME_MODULE_API const char *get_game_module_author(void) {        \
    return author;                                                             \
  }                                                                            \
  extern "C" GAME_MODULE_API const char *get_game_module_description(void) {   \
    return description;                                                        \
  }
#else
#define IMPLEMENT_GAME_MODULE(name, version, author, description)              \
  GAME_MODULE_API const char *get_game_module_name(void) { return name; }      \
  GAME_MODULE_API const char *get_game_module_version(void) {                  \
    return version;                                                            \
  }                                                                            \
  GAME_MODULE_API const char *get_game_module_author(void) { return author; }  \
  GAME_MODULE_API const char *get_game_module_description(void) {              \
    return description;                                                        \
  }
#endif

#endif // ENGINE_GAME_MODULE_H
