// include/core/unified_engine.h
//
// Purpose: Unified engine core that integrates all systems for 6-month
// development timeline This is the main entry point that coordinates all
// subsystems
//
#ifndef UNIFIED_ENGINE_H
#define UNIFIED_ENGINE_H

#include "include/common.h"
#include <math/mat4.h>
#include <math/vec3.h>
#include "include/core/gpu_acceleration.h"

// Forward declarations
typedef struct Engine Engine;
typedef struct Renderer Renderer;
typedef struct AudioSystem AudioSystem;
typedef struct InputSystem InputSystem;
typedef struct PhysicsWorld PhysicsWorld;
typedef struct AssetManager AssetManager;
typedef struct NetworkSystem NetworkSystem;
typedef struct UIManager UIManager;
typedef struct ECSWorld ECSWorld;
typedef struct ChunkManager ChunkManager;
typedef struct WorldGenerator WorldGenerator;
typedef struct ThreadPool ThreadPool;
typedef struct VFS VFS;
typedef struct NPCSystem NPCSystem;

// Engine state machine
typedef enum {
  ENGINE_STATE_UNINITIALIZED,
  ENGINE_STATE_INITIALIZING,
  ENGINE_STATE_RUNNING,
  ENGINE_STATE_PAUSED,
  ENGINE_STATE_SHUTTING_DOWN,
  ENGINE_STATE_SHUTDOWN
} EngineState;

// Engine configuration
typedef struct {
  // Window
  u32 window_width;
  u32 window_height;
  const char *window_title;
  bool fullscreen;
  bool vsync;
  bool resizable;

  // Rendering
  u32 render_distance;
  bool shadows_enabled;
  bool ray_tracing_enabled;
  f32 fov;
  f32 near_plane;
  f32 far_plane;
  GPUBackend renderer_backend;

  // Audio
  f32 master_volume;
  f32 music_volume;
  f32 sfx_volume;
  u32 audio_sample_rate;
  u32 audio_channels;

  // Physics
  f32 physics_timestep;
  u32 physics_iterations;
  Vec3 gravity;

  // World
  u32 chunk_size;
  u32 max_chunks_loaded;
  u32 world_seed;

  // System
  u32 max_threads;
  u64 memory_limit;
  bool debug_mode;
  const char *log_level;
  const char *config_path;

  // Network
  u16 server_port;
  u32 max_clients;
  bool enable_networking;
} EngineConfig;

// Engine statistics
typedef struct {
  f32 fps;
  f32 frame_time_ms;
  f32 update_time_ms;
  f32 render_time_ms;
  u64 frame_count;
  u64 draw_calls;
  u64 triangles_rendered;
  u64 memory_used;
  u64 memory_peak;
  u32 active_entities;
  u32 active_chunks;
} EngineStats;

// Main engine structure
struct Engine {
  // Core state
  EngineState state;
  EngineConfig config;
  EngineStats stats;

  // Time management
  f64 start_time;
  f64 current_time;
  f64 last_frame_time;
  f32 delta_time;
  f32 fixed_delta_time;
  f32 time_scale;
  bool paused;

  // Subsystems
  Renderer *renderer;
  AudioSystem *audio;
  InputSystem *input;
  PhysicsWorld *physics;
  AssetManager *assets;
  NetworkSystem *network;
  UIManager *ui;
  ECSWorld *ecs;
  ChunkManager *chunk_manager;
  WorldGenerator *world_generator;
  NPCSystem *npc_system;
  ThreadPool *thread_pool;
  VFS *vfs;

  // Platform
  void *window;
  void *platform_data;

  // Game module
  void *game_module;
  bool (*game_init)(Engine *engine);
  void (*game_update)(Engine *engine, f32 delta_time);
  void (*game_render)(Engine *engine);
  void (*game_shutdown)(Engine *engine);

  // Callbacks
  void (*on_init)(Engine *engine);
  void (*on_update)(Engine *engine, f32 delta_time);
  void (*on_render)(Engine *engine);
  void (*on_shutdown)(Engine *engine);
};

// Engine lifecycle
bool engine_unified_init(Engine *engine, const EngineConfig *config);
void engine_unified_shutdown(Engine *engine);
void engine_unified_run(Engine *engine);
void engine_unified_update(Engine *engine);
void engine_unified_render(Engine *engine);

// Engine control
void engine_pause(Engine *engine);
void engine_resume(Engine *engine);
void engine_stop(Engine *engine);
bool engine_is_running(const Engine *engine);
bool engine_is_paused(const Engine *engine);

// Single tick/render for hosted modes
void engine_tick(Engine *engine, f32 delta_time);
void engine_render(Engine *engine);

// Time management
f32 engine_get_delta_time(const Engine *engine);
f64 engine_get_time(const Engine *engine);
f64 engine_get_total_time(const Engine *engine);
void engine_set_time_scale(Engine *engine, f32 scale);

// Statistics
const EngineStats *engine_get_stats(const Engine *engine);
void engine_reset_stats(Engine *engine);

// Subsystem access
Renderer *engine_get_renderer(Engine *engine);
AudioSystem *engine_get_audio(Engine *engine);
InputSystem *engine_get_input(Engine *engine);
PhysicsWorld *engine_get_physics(Engine *engine);
AssetManager *engine_get_assets(Engine *engine);
NetworkSystem *engine_get_network(Engine *engine);
UIManager *engine_get_ui(Engine *engine);
ECSWorld *engine_get_ecs(Engine *engine);
ChunkManager *engine_get_chunk_manager(Engine *engine);
WorldGenerator *engine_get_world_generator(Engine *engine);
ThreadPool *engine_get_thread_pool(Engine *engine);
VFS *engine_get_vfs(Engine *engine);
NPCSystem *engine_get_npc_system(Engine *engine);

// Configuration
EngineConfig engine_create_default_config(void);
EngineConfig engine_create_low_end_config(void);
EngineConfig engine_create_high_end_config(void);
bool engine_load_config(Engine *engine, const char *path);
bool engine_save_config(const Engine *engine, const char *path);

// Platform events
void engine_handle_window_resize(Engine *engine, u32 width, u32 height);
void engine_handle_window_focus(Engine *engine, bool focused);
void engine_handle_window_close(Engine *engine);

#endif // UNIFIED_ENGINE_H
