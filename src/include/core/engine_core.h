// include/core/engine_core.h
//
// Purpose: Core engine abstraction layer that separates the engine from
// game-specific code. This provides a unified API for creating 2.5D and 3D
// games efficiently, abstracting away Minecraft-specific concepts.
//
// Public APIs:
// - EngineCore: Main engine context managing all subsystems
// - EngineConfig: Configuration for engine initialization
// - RenderingMode: 3D voxel, 2.5D isometric, or 2D sprite rendering
// - High-level API for game creation
//
// Ownership: EngineCore owns all subsystems (renderer, physics, ECS, etc.)
//
// Invariants:
// - Engine must be initialized before use
// - Rendering mode can be switched at runtime
// - All subsystems are properly initialized and cleaned up
//
#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include "include/common.h"
#include "include/ecs/ecs.h"
#include <math/mat4.h>
#include <math/vec3.h>
#include <include/rendering/renderer.h>

// Forward declarations
struct Camera;
struct PhysicsWorld;
struct ThreadPool;
struct VFS;
struct AudioSystem;

// Rendering modes
typedef enum {
  RENDERING_MODE_3D_VOXEL,       // Full 3D voxel rendering (Minecraft-style)
  RENDERING_MODE_2_5D_ISOMETRIC, // 2.5D isometric rendering (diagonal top-down)
  RENDERING_MODE_2_5D_ORTHO,     // 2.5D orthographic (side-scroller with depth)
  RENDERING_MODE_2D_SPRITE,      // Pure 2D sprite rendering
  RENDERING_MODE_COUNT
} RenderingMode;

// Engine configuration
typedef struct {
  u32 window_width;
  u32 window_height;
  const char *window_title;
  RenderingMode rendering_mode;
  bool enable_physics;
  bool enable_audio;
  bool enable_ray_tracing;
  u32 max_entities;
  u32 max_components;
  f32 physics_fixed_dt;
  f32 physics_max_frame_time;
} EngineCoreConfig;

// Engine core structure
typedef struct EngineCore {
  // Configuration
  EngineCoreConfig config;

  // Window
  void *window;
  u32 window_width;
  u32 window_height;

  // Rendering
  IRenderer *renderer;
  RenderingMode current_rendering_mode;
  struct Camera *camera;

  // Core systems
  World ecs_world;
  struct PhysicsWorld *physics_world;
  struct ThreadPool *thread_pool;
  struct VFS *vfs;
  struct AudioSystem *audio_system;

  // State
  bool initialized;
  bool running;
  f32 delta_time;
  u64 last_frame_time;

  // Scene management (see scene.h)
  void *active_scene; // Scene pointer

  // Callbacks
  void (*on_init)(struct EngineCore *engine);
  void (*on_update)(struct EngineCore *engine, f32 delta_time);
  void (*on_render)(struct EngineCore *engine);
  void (*on_shutdown)(struct EngineCore *engine);
} EngineCore;

// Engine lifecycle
bool engine_core_init(EngineCore *engine, const EngineCoreConfig *config);
void engine_core_shutdown(EngineCore *engine);
void engine_core_update(EngineCore *engine);
void engine_core_render(EngineCore *engine);

// Configuration helpers
void engine_config_set_defaults(EngineCoreConfig *config);
EngineCoreConfig engine_config_create_2_5d(u32 width, u32 height);
EngineCoreConfig engine_config_create_3d(u32 width, u32 height);
EngineCoreConfig engine_config_create_2d(u32 width, u32 height);

// Rendering mode switching
bool engine_set_rendering_mode(EngineCore *engine, RenderingMode mode);
RenderingMode engine_get_rendering_mode(EngineCore *engine);

// Window management
void engine_set_window_size(EngineCore *engine, u32 width, u32 height);
void engine_get_window_size(EngineCore *engine, u32 *width, u32 *height);

// Callback registration
void engine_set_init_callback(EngineCore *engine,
                              void (*callback)(EngineCore *));
void engine_set_update_callback(EngineCore *engine,
                                void (*callback)(EngineCore *, f32));
void engine_set_render_callback(EngineCore *engine,
                                void (*callback)(EngineCore *));
void engine_set_shutdown_callback(EngineCore *engine,
                                  void (*callback)(EngineCore *));

// High-level game creation API
typedef struct {
  const char *name;
  EngineCoreConfig config;
  void (*game_init)(EngineCore *engine);
  void (*game_update)(EngineCore *engine, f32 delta_time);
  void (*game_render)(EngineCore *engine);
  void (*game_shutdown)(EngineCore *engine);
} GameDefinition;

bool engine_run_game(const GameDefinition *game);

#endif // ENGINE_CORE_H
