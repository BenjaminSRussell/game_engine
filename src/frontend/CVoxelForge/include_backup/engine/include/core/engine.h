// include/engine/engine.h
//
// Purpose: Core engine interface that provides game-independent functionality.
// This header defines the main engine API that games can use without being
// coupled to specific implementations of rendering, physics, audio, etc.
//
// Key Features:
// - Game-agnostic engine systems
// - Plugin-style architecture for different backends
// - Clean separation between engine and game logic
// - Support for multiple game modules
//
// Usage:
// Games should only include this header and use the engine API.
// Direct access to subsystems (Vulkan, physics, etc.) should be avoided.
//
#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include "../math/math.h"

// Forward declarations for engine subsystems
typedef struct Engine Engine;
struct IRenderer;
typedef struct IRenderer Renderer;
typedef struct AudioSystem AudioSystem;
typedef struct InputSystem InputSystem;
typedef struct PhysicsWorld PhysicsWorld;
typedef struct AssetManager AssetManager;
typedef struct EntityManager EntityManager;
typedef struct NetworkSystem NetworkSystem;
typedef struct UIManager UIManager;
typedef struct ScriptSystem ScriptSystem;
typedef struct SceneManager SceneManager;
typedef struct PostProcessingPipeline PostProcessingPipeline;
typedef struct QuestSystem QuestSystem;

// AI Subsystems
typedef struct PerceptionSystem PerceptionSystem;
typedef struct MemorySystem MemorySystem;
typedef struct GoapPlannerState GoapPlannerState;
typedef struct UtilityAgent UtilityAgent;

// Engine configuration
typedef struct {
  // Window settings
  u32 window_width;
  u32 window_height;
  const char *window_title;
  bool fullscreen;
  bool vsync;

  // Rendering settings
  u32 render_distance;
  bool shadows_enabled;
  bool ray_tracing_enabled;
  f32 render_scale; // Resolution scale (0.5 - 2.0)
  f32 fov;

  // Audio settings
  f32 master_volume;
  f32 music_volume;
  f32 sfx_volume;

  // System settings
  u32 max_threads;
  u64 memory_limit;
  bool debug_mode;
  const char *log_level;
} EngineConfig;

// Forward declaration - full definition in game_module.h
typedef struct GameModule GameModule;

// Engine state
typedef struct {
  bool initialized;
  bool running;
  bool paused;
  f32 delta_time;
  u64 frame_count;
  f64 total_time;
} EngineState;

// Engine subsystem access
typedef struct {
  Renderer *renderer;
  AudioSystem *audio;
  InputSystem *input;
  PhysicsWorld *physics;
  AssetManager *assets;
  EntityManager *entities;
  NetworkSystem *network;
  UIManager *ui;
  ScriptSystem *scripting;
  SceneManager *scene_manager;
  PostProcessingPipeline *post_processing;
  QuestSystem *quests;

  // AI Subsystems
  PerceptionSystem *perception;
  MemorySystem *memory;
  GoapPlannerState *planner;
} EngineSubsystems;

// Main engine structure
struct Engine {
  EngineConfig config;
  EngineState state;
  EngineSubsystems subsystems;
  GameModule *game_module;
  void *platform_data; // Platform-specific data (window, etc.)
};

// Engine lifecycle functions
bool engine_init(Engine *engine, const EngineConfig *config);
void engine_shutdown(Engine *engine);
void engine_run(Engine *engine, GameModule *game_module);
void engine_update(Engine *engine, f32 delta_time);
void engine_render(Engine *engine);

// Engine control functions
void engine_set_game_module(Engine *engine, GameModule *module);
void engine_pause(Engine *engine);
void engine_resume(Engine *engine);
void engine_stop(Engine *engine);
bool engine_is_running(const Engine *engine);
bool engine_is_paused(const Engine *engine);

// Subsystem access functions
Renderer *engine_get_renderer(Engine *engine);
AudioSystem *engine_get_audio(Engine *engine);
InputSystem *engine_get_input(Engine *engine);
PhysicsWorld *engine_get_physics(Engine *engine);
AssetManager *engine_get_assets(Engine *engine);
EntityManager *engine_get_entities(Engine *engine);
NetworkSystem *engine_get_network(Engine *engine);
UIManager *engine_get_ui(Engine *engine);
ScriptSystem *engine_get_scripting(Engine *engine);

// Engine utility functions
f32 engine_get_delta_time(const Engine *engine);
f64 engine_get_total_time(const Engine *engine);
u64 engine_get_frame_count(const Engine *engine);
const EngineConfig *engine_get_config(const Engine *engine);

// Engine factory functions for different configurations
EngineConfig engine_create_default_config(void);
EngineConfig engine_create_low_end_config(void);
EngineConfig engine_create_high_end_config(void);
EngineConfig engine_create_debug_config(void);

// Engine validation and debugging
bool engine_validate_config(const EngineConfig *config);
void engine_print_info(const Engine *engine);
void engine_print_performance_stats(const Engine *engine);

// Memory management
void *engine_allocate(Engine *engine, size_t size);
void engine_deallocate(Engine *engine, void *ptr);
void *engine_reallocate(Engine *engine, void *ptr, size_t size);

// Error handling
typedef enum {
  ENGINE_ERROR_NONE = 0,
  ENGINE_ERROR_INIT_FAILED,
  ENGINE_ERROR_WINDOW_CREATION_FAILED,
  ENGINE_ERROR_RENDERER_INIT_FAILED,
  ENGINE_ERROR_AUDIO_INIT_FAILED,
  ENGINE_ERROR_OUT_OF_MEMORY,
  ENGINE_ERROR_INVALID_CONFIG,
  ENGINE_ERROR_SUBSYSTEM_FAILED,
  ENGINE_ERROR_GAME_MODULE_FAILED,
  ENGINE_ERROR_PLATFORM_ERROR
} EngineError;

const char *engine_get_error_string(EngineError error);

#endif // ENGINE_H
