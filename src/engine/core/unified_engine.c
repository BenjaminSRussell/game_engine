// src/core/unified_engine.c
//
// Purpose: Unified engine implementation integrating all systems
//
#include "../include/core/unified_engine.h"
#include "../include/chunk/chunk.h"
#include "../include/core/asset_manager.h"
#include "../include/core/logger.h"
#include "../include/core/memory.h"
#include "../include/core/performance.h"
#include "../include/ecs/ecs.h"
#include "include/audio/audio.h"
#include "core/game_module.h"
#include "include/platform/input/input.h"
#include "include/rendering/renderer.h"
#include "include/ecs/components/npc.h"
#include "../include/physics/physics.h"
#include "core/threading/job.h"
#include "core/resource/vfs/vfs.h"
#include "../include/world/generator.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

// High-resolution timer
static f64 get_high_res_time(void) {
#ifdef _WIN32
  static LARGE_INTEGER frequency = {0};
  if (frequency.QuadPart == 0) {
    QueryPerformanceFrequency(&frequency);
  }
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (f64)counter.QuadPart / (f64)frequency.QuadPart;
#elif defined(__APPLE__)
  static mach_timebase_info_data_t timebase = {0};
  if (timebase.denom == 0) {
    mach_timebase_info(&timebase);
  }
  return (f64)mach_absolute_time() * (f64)timebase.numer / (f64)timebase.denom /
         1e9;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1e9;
#endif
}

// Default configuration
EngineConfig engine_create_default_config(void) {
  EngineConfig config = {0};
  config.window_width = 1920;
  config.window_height = 1080;
  config.window_title = "Minecraft Engine";
  config.fullscreen = false;
  config.vsync = true;
  config.resizable = true;

  config.render_distance = 16;
  config.shadows_enabled = true;
  config.ray_tracing_enabled = false;
  config.fov = 70.0f;
  config.near_plane = 0.1f;
  config.far_plane = 1000.0f;

  config.master_volume = 1.0f;
  config.music_volume = 0.7f;
  config.sfx_volume = 1.0f;
  config.audio_sample_rate = 44100;
  config.audio_channels = 2;

  config.physics_timestep = 1.0f / 60.0f;
  config.physics_iterations = 10;
  config.gravity = (Vec3){0, -9.81f, 0};

  config.chunk_size = 32;
  config.max_chunks_loaded = 1024;
  config.world_seed = 0;

  config.max_threads = 4;
  config.memory_limit = 2ULL * 1024 * 1024 * 1024; // 2GB
  config.debug_mode = false;
  config.log_level = "INFO";
  config.config_path = "config.ini";

  config.server_port = 25565;
  config.max_clients = 32;
  config.enable_networking = false;

  return config;
}

EngineConfig engine_create_low_end_config(void) {
  EngineConfig config = engine_create_default_config();
  config.window_width = 1280;
  config.window_height = 720;
  config.render_distance = 8;
  config.shadows_enabled = false;
  config.max_chunks_loaded = 512;
  config.max_threads = 2;
  return config;
}

EngineConfig engine_create_high_end_config(void) {
  EngineConfig config = engine_create_default_config();
  config.window_width = 2560;
  config.window_height = 1440;
  config.render_distance = 32;
  config.shadows_enabled = true;
  config.ray_tracing_enabled = true;
  config.max_chunks_loaded = 2048;
  config.max_threads = 8;
  return config;
}

// Engine initialization
bool engine_unified_init(Engine *engine, const EngineConfig *config) {
  if (!engine || !config) {
    LOG_ERROR("Engine init failed: null parameters");
    return false;
  }

  memset(engine, 0, sizeof(Engine));
  engine->config = *config;
  engine->state = ENGINE_STATE_INITIALIZING;
  engine->time_scale = 1.0f;
  engine->paused = false;

  // Initialize time
  engine->start_time = get_high_res_time();
  engine->current_time = engine->start_time;
  engine->last_frame_time = engine->start_time;
  engine->delta_time = 0.0f;
  engine->fixed_delta_time = config->physics_timestep;

  LOG_INFO("Initializing unified engine...");

  // Initialize VFS first (needed by other systems)
  engine->vfs = (VFS *)calloc(1, sizeof(VFS));
  if (!engine->vfs) {
    LOG_ERROR("Failed to allocate VFS");
    return false;
  }
  vfs_init(engine->vfs);

  // Initialize thread pool
  engine->thread_pool = (ThreadPool *)calloc(1, sizeof(ThreadPool));
  if (!engine->thread_pool) {
    LOG_ERROR("Failed to allocate thread pool");
    return false;
  }
  if (!thread_pool_init(engine->thread_pool, config->max_threads)) {
    LOG_ERROR("Failed to initialize thread pool");
    return false;
  }

  // Initialize ECS first (Asset Manager may need it)
  engine->ecs = (ECSWorld *)calloc(1, sizeof(ECSWorld));
  if (!engine->ecs) {
    LOG_ERROR("Failed to allocate ECS world");
    return false;
  }
  ecs_world_init(engine->ecs, 65536, 256, 64);

  // Initialize asset manager
  engine->assets = asset_manager_create(256, (World *)engine->ecs);
  if (!engine->assets) {
    LOG_ERROR("Failed to initialize asset manager");
    return false;
  }

  // Initialize physics
  engine->physics = (PhysicsWorld *)calloc(1, sizeof(PhysicsWorld));
  if (!engine->physics) {
    LOG_ERROR("Failed to allocate physics world");
    return false;
  }
  physics_world_init(engine->physics, &config->gravity);

  // Initialize renderer (placeholder - will be implemented)
  // engine->renderer = renderer_create(...);

  // Initialize audio (placeholder)
  // engine->audio = audio_system_create(...);

  // Initialize input (placeholder)
  // engine->input = input_system_create(...);

  // Initialize network (if enabled)
  if (config->enable_networking) {
    // engine->network = network_system_create(...);
  }

  // Initialize UI (placeholder)
  // engine->ui = ui_manager_create(...);

  // Initialize chunk manager
  engine->chunk_manager = (ChunkManager *)calloc(1, sizeof(ChunkManager));
  if (!engine->chunk_manager) {
    LOG_ERROR("Failed to allocate chunk manager");
    return false;
  }
  chunk_manager_init(engine->chunk_manager, config->max_chunks_loaded);

  // Initialize NPC system
  engine->npc_system = (NPCSystem *)calloc(1, sizeof(NPCSystem));
  if (!engine->npc_system) {
    LOG_ERROR("Failed to allocate NPC system");
    return false;
  }
  npc_system_init(engine->npc_system, engine->ecs, engine->physics);

  // Initialize world generator
  engine->world_generator = (WorldGenerator *)calloc(1, sizeof(WorldGenerator));
  if (!engine->world_generator) {
    LOG_ERROR("Failed to allocate world generator");
    return false;
  }

  // Create generation context
  GenerationContext *gen_ctx =
      (GenerationContext *)calloc(1, sizeof(GenerationContext));
  if (!gen_ctx) {
    LOG_ERROR("Failed to allocate generation context");
    return false;
  }
  gen_ctx->chunk_manager = engine->chunk_manager;
  gen_ctx->npc_system = engine->npc_system;

  world_generator_init(engine->world_generator, config->world_seed, gen_ctx);

  // Initialize statistics
  memset(&engine->stats, 0, sizeof(EngineStats));

  engine->state = ENGINE_STATE_RUNNING;

  // Integrate all systems
  extern bool engine_integrate_all_systems(Engine * engine);
  if (!engine_integrate_all_systems(engine)) {
    LOG_WARN("Some systems failed to integrate, continuing anyway");
  }

  LOG_INFO("Engine initialized successfully");
  return true;
}

void engine_unified_shutdown(Engine *engine) {
  if (!engine)
    return;

  if (engine->state == ENGINE_STATE_SHUTDOWN)
    return;

  engine->state = ENGINE_STATE_SHUTTING_DOWN;
  LOG_INFO("Shutting down engine...");

  // Shutdown game module
  if (engine->game_shutdown) {
    engine->game_shutdown(engine);
  }

  // Shutdown callbacks
  if (engine->on_shutdown) {
    engine->on_shutdown(engine);
  }

  // Shutdown subsystems
  if (engine->world_generator) {
    if (engine->world_generator->context) {
      free(engine->world_generator->context);
    }
    world_generator_free(engine->world_generator);
    free(engine->world_generator);
  }

  if (engine->npc_system) {
    npc_system_free(engine->npc_system);
    free(engine->npc_system);
  }

  if (engine->chunk_manager) {
    chunk_manager_free(engine->chunk_manager);
    free(engine->chunk_manager);
  }

  if (engine->ecs) {
    ecs_world_free(engine->ecs);
    free(engine->ecs);
  }

  if (engine->physics) {
    physics_world_free(engine->physics);
    free(engine->physics);
  }

  if (engine->assets) {
    asset_manager_destroy(engine->assets);
  }

  if (engine->thread_pool) {
    thread_pool_free(engine->thread_pool);
    free(engine->thread_pool);
  }

  if (engine->vfs) {
    vfs_free(engine->vfs);
    free(engine->vfs);
  }

  // Shutdown integrated systems
  extern void engine_shutdown_integrated_systems(Engine * engine);
  engine_shutdown_integrated_systems(engine);

  engine->state = ENGINE_STATE_SHUTDOWN;
  LOG_INFO("Engine shutdown complete");
}

void engine_unified_update(Engine *engine) {
  if (!engine || engine->state != ENGINE_STATE_RUNNING || engine->paused) {
    return;
  }

  f64 update_start = get_high_res_time();

  // Update time
  engine->current_time = get_high_res_time();
  engine->delta_time = (f32)(engine->current_time - engine->last_frame_time) *
                       engine->time_scale;
  engine->last_frame_time = engine->current_time;

  // Clamp delta time to prevent spiral of death
  if (engine->delta_time > 0.25f) {
    engine->delta_time = 0.25f;
  }

  // Update statistics
  engine->stats.frame_count++;
  engine->stats.fps = 1.0f / engine->delta_time;
  engine->stats.frame_time_ms = engine->delta_time * 1000.0f;

  // Update integrated systems
  extern void engine_update_integrated_systems(Engine * engine, f32 delta_time);
  engine_update_integrated_systems(engine, engine->delta_time);

  // Fixed timestep physics
  f32 accumulator = engine->delta_time;
  const f32 fixed_dt = engine->fixed_delta_time;
  while (accumulator >= fixed_dt) {
    if (engine->physics) {
      physics_world_step(engine->physics, fixed_dt);
    }
    accumulator -= fixed_dt;
  }

  // Update ECS systems
  if (engine->ecs) {
    ecs_update_systems(engine->ecs, engine->delta_time);
  }

  // Update game module
  if (engine->game_update) {
    engine->game_update(engine, engine->delta_time);
  }

  // Update NPC system
  if (engine->npc_system) {
    npc_update(engine->npc_system, engine->delta_time);
  }

  // Update callbacks
  if (engine->on_update) {
    engine->on_update(engine, engine->delta_time);
  }

  f64 update_end = get_high_res_time();
  engine->stats.update_time_ms = (f32)(update_end - update_start) * 1000.0f;
}

void engine_unified_render(Engine *engine) {
  if (!engine || engine->state != ENGINE_STATE_RUNNING) {
    return;
  }

  f64 render_start = get_high_res_time();

  // Render integrated systems
  extern void engine_render_integrated_systems(Engine * engine);
  engine_render_integrated_systems(engine);

  // Render callbacks
  if (engine->on_render) {
    engine->on_render(engine);
  }

  f64 render_end = get_high_res_time();
  engine->stats.render_time_ms = (f32)(render_end - render_start) * 1000.0f;
}

void engine_unified_run(Engine *engine) {
  if (!engine || engine->state != ENGINE_STATE_RUNNING) {
    return;
  }

  // Initialize game module
  if (engine->game_init && !engine->game_init(engine)) {
    LOG_ERROR("Game module initialization failed");
    return;
  }

  // Initialize callbacks
  if (engine->on_init) {
    engine->on_init(engine);
  }

  LOG_INFO("Starting engine main loop");

  while (engine->state == ENGINE_STATE_RUNNING) {
    engine_unified_update(engine);
    engine_unified_render(engine);

    // Handle platform events (window messages, etc.)
    // This would be platform-specific
  }

  LOG_INFO("Engine main loop ended");
}

// Control functions
void engine_pause(Engine *engine) {
  if (engine) {
    engine->paused = true;
  }
}

void engine_resume(Engine *engine) {
  if (engine) {
    engine->paused = false;
  }
}

void engine_stop(Engine *engine) {
  if (engine) {
    engine->state = ENGINE_STATE_SHUTTING_DOWN;
  }
}

bool engine_is_running(const Engine *engine) {
  return engine && engine->state == ENGINE_STATE_RUNNING;
}

bool engine_is_paused(const Engine *engine) { return engine && engine->paused; }

// Time functions
f32 engine_get_delta_time(const Engine *engine) {
  return engine ? engine->delta_time : 0.0f;
}

f64 engine_get_time(const Engine *engine) {
  return engine ? engine->current_time : 0.0;
}

f64 engine_get_total_time(const Engine *engine) {
  return engine ? (engine->current_time - engine->start_time) : 0.0;
}

void engine_set_time_scale(Engine *engine, f32 scale) {
  if (engine) {
    engine->time_scale = scale;
  }
}

// Statistics
const EngineStats *engine_get_stats(const Engine *engine) {
  return engine ? &engine->stats : NULL;
}

void engine_reset_stats(Engine *engine) {
  if (engine) {
    memset(&engine->stats, 0, sizeof(EngineStats));
  }
}

// Subsystem access
Renderer *engine_get_renderer(Engine *engine) {
  return engine ? engine->renderer : NULL;
}
AudioSystem *engine_get_audio(Engine *engine) {
  return engine ? engine->audio : NULL;
}
InputSystem *engine_get_input(Engine *engine) {
  return engine ? engine->input : NULL;
}
PhysicsWorld *engine_get_physics(Engine *engine) {
  return engine ? engine->physics : NULL;
}
AssetManager *engine_get_assets(Engine *engine) {
  return engine ? engine->assets : NULL;
}
NetworkSystem *engine_get_network(Engine *engine) {
  return engine ? engine->network : NULL;
}
UIManager *engine_get_ui(Engine *engine) { return engine ? engine->ui : NULL; }
ECSWorld *engine_get_ecs(Engine *engine) { return engine ? engine->ecs : NULL; }
ChunkManager *engine_get_chunk_manager(Engine *engine) {
  return engine ? engine->chunk_manager : NULL;
}
WorldGenerator *engine_get_world_generator(Engine *engine) {
  return engine ? engine->world_generator : NULL;
}
ThreadPool *engine_get_thread_pool(Engine *engine) {
  return engine ? engine->thread_pool : NULL;
}
VFS *engine_get_vfs(Engine *engine) { return engine ? engine->vfs : NULL; }
NPCSystem *engine_get_npc_system(Engine *engine) {
  return engine ? engine->npc_system : NULL;
}

// Platform events
void engine_handle_window_resize(Engine *engine, u32 width, u32 height) {
  if (!engine)
    return;
  engine->config.window_width = width;
  engine->config.window_height = height;
  if (engine->renderer) {
    // renderer_resize(engine->renderer, width, height);
  }
}

void engine_handle_window_focus(Engine *engine, bool focused) {
  if (!engine)
    return;
  // Handle focus events
}

void engine_handle_window_close(Engine *engine) {
  if (engine) {
    engine_stop(engine);
  }
}
