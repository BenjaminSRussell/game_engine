// src/engine/core/engine.c
//
// Purpose: Core engine implementation conforming to engine.h interface
//
#include "core/memory/common_pools.h"
#include "core/memory/frame_allocator.h"
#include "core/memory/unified_memory_allocator.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <core/engine.h>
#include <core/game_loop.h>
#include <core/game_module.h>
#include <core/performance.h>
#include <core/string_utils.h>
#include <core/thread_pool.h>
#include <core/window.h>
#include <physics/physics.h>
#include <stdlib.h>
#include <string.h>

// Subsystem headers
#include <core/asset_manager.h>
#include <ecs/ecs.h>
#include <include/platform/input/input.h>
// #include <physics/physics.h>
#include <rendering/renderer.h>
// #include <scripting/script_system.h>
#include <core/resource/vfs/vfs.h>

// New Subsystems
#include <audio/audio_system.h>
// #include <network/reliable_udp.h>
// #include <network/rpc_system.h>

#include <core/hot_reload.h>
#include <rendering/post_processing.h>
#include <scene/scene_system.h>
#include <tools/profiler.h>

// Gameplay system headers
// #include <gameplay/combat/combat_system.h>
// #include <gameplay/crafting/crafting.h>
// #include <gameplay/inventory/inventory.h>
// #include <gameplay/inventory/item_database.h>

// AI advanced subsystems
#include <ai/npc/perception_system.h>
#include <ai/npc_advanced/goap_enhanced.h>
#include <ai/npc_advanced/memory_system.h>
#include <ai/npc_advanced/utility_ai.h>

// #include <ai/npc_advanced/utility_ai.h>

// Animation System
// #include <character/animation/animation_system.h>

// Internal state wrapper if needed
typedef struct {
  Window window;
  GameLoop loop;
} PlatformData;

// Subsystems
VFS g_vfs;

// Subsystem validation state
typedef struct {
  bool vfs_initialized;
  bool input_initialized;
  bool ecs_initialized;
  bool assets_initialized;
  bool renderer_initialized;
  bool physics_initialized;
  bool scene_manager_initialized;
  bool audio_initialized;
  bool post_processing_initialized;
  bool perception_initialized;
  bool memory_initialized;
  bool planner_initialized;
} SubsystemValidationState;

// Forward declarations
static bool engine_init_subsystems(Engine *engine);
static void engine_shutdown_subsystems(Engine *engine);
static void engine_update_callback(void *user_data, f32 delta_time);
static void engine_render_callback(void *user_data, f32 interpolation);
static bool
engine_validate_subsystem_init(const char *name, bool success,
                               SubsystemValidationState *validation);
static void
engine_log_initialization_summary(const SubsystemValidationState *validation);

// -----------------------------------------------------------------------------
// Engine Lifecycle
// -----------------------------------------------------------------------------

bool engine_init(Engine *engine, const EngineConfig *config) {
  if (!engine || !config) {
    LOG_ERROR("Engine init failed: NULL parameters");
    return false;
  }

  // Clear engine struct
  memset(engine, 0, sizeof(Engine));

  // Validate configuration
  if (config->window_width <= 0 || config->window_height <= 0) {
    LOG_ERROR("Engine init failed: Invalid window dimensions");
    return false;
  }

  if (config->window_title == NULL) {
    LOG_ERROR("Engine init failed: Window title cannot be NULL");
    return false;
  }

  // Initialize Profiler
  profiler_init();
  LOG_INFO("Profiler initialized");

  // Copy config
  engine->config = *config;

  // Allocate platform data
  engine->platform_data = calloc(1, sizeof(PlatformData));
  if (!engine->platform_data) {
    LOG_ERROR("Failed to allocate platform data");
    profiler_shutdown();
    return false;
  }
  PlatformData *pdata = (PlatformData *)engine->platform_data;

  LOG_INFO("Initializing Engine Core...");

  // Initialize Window with validation
  LOG_INFO("Creating Window '%s' (%dx%d)...", config->window_title,
           config->window_width, config->window_height);

  if (!window_init(&pdata->window, config->window_width, config->window_height,
                   config->window_title ? config->window_title : "Game Engine",
                   config->fullscreen)) {
    LOG_ERROR("Failed to create window");
    free(engine->platform_data);
    profiler_shutdown();
    return false;
  }

  // Validate window creation
  if (pdata->window.handle == NULL) {
    LOG_ERROR("Window validation failed");
    window_shutdown(&pdata->window);
    free(engine->platform_data);
    profiler_shutdown();
    return false;
  }

  // Initialize Game Loop
  f32 target_fps = 60.0f;
  if (target_fps <= 0.0f) {
    LOG_ERROR("Invalid target FPS: %f", target_fps);
    target_fps = 60.0f; // Default fallback
  }

  game_loop_init(&pdata->loop, 1.0f / target_fps);
  game_loop_set_update_callback(&pdata->loop, engine_update_callback);
  game_loop_set_render_callback(&pdata->loop, engine_render_callback);
  game_loop_set_user_data(&pdata->loop, engine);

  // Initialize Internal Subsystems with validation
  if (!engine_init_subsystems(engine)) {
    LOG_ERROR("Failed to initialize engine subsystems");
    engine_shutdown_subsystems(engine);
    window_shutdown(&pdata->window);
    free(engine->platform_data);
    profiler_shutdown();
    return false;
  }

  // Initialize Hot Reload with validation
  HotReloadConfig hr_config = {
      .enable_code_hot_reload = true,
      .enable_asset_hot_reload = true,
      .watch_path = "." // Current directory for now
  };

  if (!hot_reload_init(hr_config)) {
    LOG_WARN("Hot reload initialization failed, continuing without it");
  }

  engine->state.initialized = true;
  LOG_INFO("Engine initialized successfully");
  return true;
}

void engine_run(Engine *engine, GameModule *game_module) {
  if (!engine || !engine->state.initialized) {
    LOG_ERROR("Cannot run engine: invalid state");
    return;
  }

  engine->game_module = game_module;
  PlatformData *pdata = (PlatformData *)engine->platform_data;

  // Initialize Game Module
  if (engine->game_module && engine->game_module->initialize) {
    if (!engine->game_module->initialize(engine->game_module, engine)) {
      LOG_ERROR("Game module failed to initialize");
      return;
    }
  }

  engine->state.running = true;
  LOG_INFO("Engine Starting Run Loop...");
  game_loop_run(&pdata->loop);
  LOG_INFO("Engine Run Loop Ended");

  // Shutdown Game Module
  if (engine->game_module && engine->game_module->shutdown) {
    engine->game_module->shutdown(engine->game_module);
  }
}

void engine_tick(Engine *engine, f32 delta_time) {
  engine_update_callback(engine, delta_time);
}

void engine_render(Engine *engine) { engine_render_callback(engine, 0.0f); }

void engine_shutdown(Engine *engine) {
  if (!engine || !engine->state.initialized) {
    LOG_WARN("Attempted to shutdown non-initialized engine");
    return;
  }

  LOG_INFO("Shutting down Engine...");

  PlatformData *pdata = (PlatformData *)engine->platform_data;

  // Validate engine state before shutdown
  if (!pdata) {
    LOG_ERROR("Platform data is NULL during shutdown");
    memset(engine, 0, sizeof(Engine));
    profiler_shutdown();
    LOG_INFO("Engine Shutdown Complete (with errors)");
    return;
  }

  // Shutdown subsystems in reverse order with validation
  engine_shutdown_subsystems(engine);

  // Shutdown hot reload
  if (hot_reload_init_watcher(NULL, NULL)) {
    hot_reload_shutdown();
  }

  // Validate and shutdown game loop
  if (pdata->loop.running) {
    game_loop_shutdown(&pdata->loop);
  } else {
    LOG_WARN("Game loop was not properly initialized");
  }

  // Validate and shutdown window
  if (pdata->window.handle != NULL) {
    window_shutdown(&pdata->window);
  } else {
    LOG_WARN("Window was not properly initialized");
  }

  // Free platform data
  if (engine->platform_data) {
    free(engine->platform_data);
    engine->platform_data = NULL;
  }

  // Clear engine state
  memset(engine, 0, sizeof(Engine));

  // Shutdown profiler last
  profiler_shutdown();

  LOG_INFO("Engine Shutdown Complete");
}

// -----------------------------------------------------------------------------
// Callbacks
// -----------------------------------------------------------------------------

static void engine_update_callback(void *user_data, f32 delta_time) {
  // Update Hot Reload
  hot_reload_update();

  Engine *engine = (Engine *)user_data;
  if (!engine) {
    LOG_FATAL("Engine is NULL in update!");
    return;
  }

  PlatformData *pdata = (PlatformData *)engine->platform_data;
  if (!pdata) {
    LOG_FATAL("PlatformData is NULL in update!");
    return;
  }

  // Poll window events
  window_poll_events();

  if (!engine->state.running) {
    game_loop_stop(&pdata->loop);
    return;
  }

  // Update internal state
  engine->state.delta_time = delta_time;
  engine->state.total_time += delta_time;
  engine->state.frame_count++;

  // Update Subsystems
  if (engine->subsystems.input) {
    engine->subsystems.input->update(engine->subsystems.input, delta_time);
  }

  // ... (Asset manager skipped for now)

  if (engine->subsystems.entities) {
    World *world = (World *)engine->subsystems.entities;
    ecs_world_update(world, delta_time);

    // Update combat system
    // combat_system_update(world, delta_time);
  }

  // Update AI Subsystems
  if (engine->subsystems.perception) {
    perception_system_process_frame(engine->subsystems.perception, delta_time);
  }

  if (engine->subsystems.memory) {
    memory_system_update(engine->subsystems.memory, delta_time);
  }

  // Update Animation System
  // animation_system_update(delta_time);

  // Game Module Input & Update
  if (engine->game_module) {
    if (engine->game_module->handle_input) {
      engine->game_module->handle_input(engine->game_module, engine);
    }
    if (engine->game_module->update) {
      engine->game_module->update(engine->game_module, engine, delta_time);
    }
  }
}

static void engine_render_callback(void *user_data, f32 interpolation) {
  Engine *engine = (Engine *)user_data;
  PlatformData *pdata = (PlatformData *)engine->platform_data;
  (void)interpolation;

  if (engine->subsystems.renderer) {
    u32 image_index;
    if (engine->subsystems.renderer->begin_frame(engine->subsystems.renderer,
                                                 &image_index)) {
      // Game Module Render
      if (engine->game_module && engine->game_module->render) {
        engine->game_module->render(engine->game_module, engine);
      }
      engine->subsystems.renderer->end_frame(engine->subsystems.renderer,
                                             image_index);
    }
  }

  window_swap_buffers(&pdata->window);
}

// Manual update for tests and external control
void engine_update(Engine *engine, f32 delta_time) {
  if (!engine || !engine->state.initialized) {
    return;
  }

  // Ensure running state is set (needed for manual updates)
  bool was_running = engine->state.running;
  engine->state.running = true;

  // Directly call the update callback to run one frame
  engine_update_callback(engine, delta_time);

  // Restore previous running state if we changed it
  engine->state.running = was_running;
}

// -----------------------------------------------------------------------------
// Subsystem Management
// -----------------------------------------------------------------------------

static bool engine_init_subsystems(Engine *engine) {
  PlatformData *pdata = (PlatformData *)engine->platform_data;

  // Initialize validation state
  SubsystemValidationState validation = {0};
  bool critical_failure = false;

  // 1. Unified Memory Allocator (CRITICAL - must be FIRST)
  printf("Initializing Unified Memory Allocator...\n");
  MemoryPolicy memory_policy = {
      .global_limit = SIZE_MAX,             // No global limit for now
      .per_pool_limit = 64 * 1024 * 1024,   // 64MB per pool
      .per_stack_limit = 16 * 1024 * 1024,  // 16MB per stack
      .per_arena_limit = 128 * 1024 * 1024, // 128MB per arena
      .max_allocations = 1000000,
      .enable_guard_pages = false,  // Disable for performance
      .enable_canaries = true,      // Enable corruption detection
      .enable_stack_traces = false, // Disable for performance
      .stack_trace_depth = 8,
      .enable_leak_detection = true,
      .enable_fragmentation_check = true};

  if (!unified_memory_init(&memory_policy)) {
    printf("CRITICAL: Failed to initialize unified memory allocator\n");
    return false;
  }
  printf(" Unified Memory Allocator initialized successfully\n");

  // Initialize common memory pools
  if (!common_pools_init()) {
    printf("CRITICAL: Failed to initialize common memory pools\n");
    unified_memory_shutdown();
    return false;
  }

  // Initialize frame allocator
  if (!frame_allocator_init()) {
    printf("CRITICAL: Failed to initialize frame allocator\n");
    common_pools_shutdown();
    unified_memory_shutdown();
    return false;
  }

  // 2. Legacy Memory Tracker (for compatibility)
  if (!memory_tracker_init(1024)) {
    LOG_ERROR("Failed to initialize legacy memory tracker");
    // Non-critical, continue
  }

  // 2. Logging System (CRITICAL - must be second)
  LOG_INFO("Initializing Unified Logging System...");
  if (!logger_init(LOG_LEVEL_DEBUG, LOG_TARGET_CONSOLE, NULL)) {
    LOG_ERROR("Failed to initialize logging system");
    critical_failure = true;
  } else {
    LOG_INFO(" Logging System initialized successfully");
  }

  // 3. Thread Pool (CRITICAL - must be third)
  LOG_INFO("Initializing Thread Pool...");
  if (!thread_pool_init(
          engine->config.max_threads > 0 ? engine->config.max_threads : 4)) {
    LOG_ERROR("Failed to initialize thread pool");
    critical_failure = true;
  } else {
    LOG_INFO(" Thread Pool initialized successfully");
  }

  // 4. VFS
  vfs_init(&g_vfs);
  if (vfs_mount(&g_vfs, "assets", "assets")) {
    validation.vfs_initialized =
        engine_validate_subsystem_init("VFS", true, &validation);
  } else {
    validation.vfs_initialized =
        engine_validate_subsystem_init("VFS", false, &validation);
    critical_failure = true;
  }

  // 5. Input
  if (pdata->window.is_hosted) {
    engine->subsystems.input = create_host_input_system();
  } else {
    engine->subsystems.input = create_glfw_input_system();
  }

  if (engine->subsystems.input) {
    InputConfig input_config = input_create_default_config();
    validation.input_initialized = engine_validate_subsystem_init(
        "Input",
        engine->subsystems.input->init(engine->subsystems.input, &input_config),
        &validation);
    if (!validation.input_initialized) {
      critical_failure = true;
    }
  } else {
    validation.input_initialized =
        engine_validate_subsystem_init("Input", false, &validation);
    critical_failure = true;
  }

  // 6. ECS
  WorldConfig world_config = ecs_world_create_default_config();
  engine->subsystems.entities =
      (EntityManager *)ecs_world_create(&world_config);
  validation.ecs_initialized = engine_validate_subsystem_init(
      "ECS", engine->subsystems.entities != NULL, &validation);
  if (!validation.ecs_initialized) {
    critical_failure = true;
  }

  // 7. Asset Manager (requires ECS)
  if (validation.ecs_initialized) {
    engine->subsystems.assets =
        asset_manager_create(512, (World *)engine->subsystems.entities, &g_vfs);
    if (engine->subsystems.assets) {
      engine->subsystems.assets->vfs = &g_vfs;
      validation.assets_initialized =
          engine_validate_subsystem_init("Asset Manager", true, &validation);
    } else {
      validation.assets_initialized =
          engine_validate_subsystem_init("Asset Manager", false, &validation);
      critical_failure = true;
    }
  } else {
    validation.assets_initialized = false;
    LOG_ERROR("Cannot initialize Asset Manager: ECS not initialized");
    critical_failure = true;
  }

  // 8. Renderer
  engine->subsystems.renderer = renderer_create_with_backend(
      RENDERER_TYPE_VOXEL, engine->config.renderer_backend, &pdata->window);

  if (engine->subsystems.renderer) {
    RendererInitParams render_params = {.window = &pdata->window,
                                        .width = engine->config.window_width,
                                        .height = engine->config.window_height,
                                        .type = RENDERER_TYPE_VOXEL,
                                        .backend =
                                            engine->config.renderer_backend,
                                        .config = NULL};
    validation.renderer_initialized = engine_validate_subsystem_init(
        "Renderer",
        engine->subsystems.renderer->init(engine->subsystems.renderer,
                                          &render_params),
        &validation);
    if (!validation.renderer_initialized) {
      critical_failure = true;
    }
  } else {
    validation.renderer_initialized =
        engine_validate_subsystem_init("Renderer", false, &validation);
    critical_failure = true;
  }

  // 9. Physics (non-critical)
  PhysicsConfig phys_config = {.gravity = {0.0f, -9.81f, 0.0f},
                               .fixed_timestep = 1.0f / 60.0f,
                               .velocity_iterations = 8,
                               .position_iterations = 3};
  engine->subsystems.physics = physics_world_create(phys_config);
  validation.physics_initialized = engine_validate_subsystem_init(
      "Physics", engine->subsystems.physics != NULL, &validation);

  // 10. Scene Manager (non-critical)
  engine->subsystems.scene_manager =
      (SceneManager *)calloc(1, sizeof(SceneManager));
  if (engine->subsystems.scene_manager) {
    validation.scene_manager_initialized = engine_validate_subsystem_init(
        "Scene Manager", scene_manager_init(engine->subsystems.scene_manager),
        &validation);
  } else {
    validation.scene_manager_initialized =
        engine_validate_subsystem_init("Scene Manager", false, &validation);
  }

  // 11. Audio System (non-critical)
  engine->subsystems.audio = (AudioSystem *)calloc(1, sizeof(AudioSystem));
  if (engine->subsystems.audio) {
    audio_system_init(engine->subsystems.audio, 32);
    validation.audio_initialized =
        engine_validate_subsystem_init("Audio", true, &validation);
  } else {
    validation.audio_initialized =
        engine_validate_subsystem_init("Audio", false, &validation);
  }

  // 12. Post Processing (non-critical)
  engine->subsystems.post_processing =
      (PostProcessingPipeline *)calloc(1, sizeof(PostProcessingPipeline));
  if (engine->subsystems.post_processing) {
    PostProcessingConfig pp_config = {0};
    pp_config.enabledEffects = 0;

    bool use_metal_pp = false;
#ifdef GPU_BACKEND_METAL
    use_metal_pp = true;
#endif

    validation.post_processing_initialized = engine_validate_subsystem_init(
        "Post Processing",
        post_process_init(engine->subsystems.post_processing, NULL, &pp_config),
        &validation);
  } else {
    validation.post_processing_initialized =
        engine_validate_subsystem_init("Post Processing", false, &validation);
  }

  // 13. AI Systems (non-critical)
  PerceptionSystemConfig perception_config = {.max_agents = 100,
                                              .max_stimuli_per_frame = 50,
                                              .max_perceived_entities = 20,
                                              .spatial_grid_size = 10.0f,
                                              .enable_occlusion = true,
                                              .memory_decay_time = 30.0,
                                              .debug_mode =
                                                  engine->config.debug_mode};
  engine->subsystems.perception = perception_system_create(&perception_config);
  if (engine->subsystems.perception) {
    validation.perception_initialized = engine_validate_subsystem_init(
        "Perception System",
        perception_system_initialize(engine->subsystems.perception),
        &validation);
  } else {
    validation.perception_initialized =
        engine_validate_subsystem_init("Perception System", false, &validation);
  }

  engine->subsystems.memory = memory_system_create(100);
  validation.memory_initialized = engine_validate_subsystem_init(
      "Memory System", engine->subsystems.memory != NULL, &validation);

  engine->subsystems.planner = goap_planner_create_state(256);
  validation.planner_initialized = engine_validate_subsystem_init(
      "GOAP Planner", engine->subsystems.planner != NULL, &validation);

  // Log initialization summary
  engine_log_initialization_summary(&validation);

  return !critical_failure;
}

static void engine_shutdown_subsystems(Engine *engine) {
  // Shutdown in reverse order of initialization

  // AI Systems Shutdown
  if (engine->subsystems.perception) {
    perception_system_shutdown(engine->subsystems.perception);
    perception_system_destroy(engine->subsystems.perception);
    engine->subsystems.perception = NULL;
  }

  if (engine->subsystems.memory) {
    memory_system_destroy(engine->subsystems.memory);
    engine->subsystems.memory = NULL;
  }

  if (engine->subsystems.planner) {
    goap_planner_destroy_state(engine->subsystems.planner);
    engine->subsystems.planner = NULL;
  }

  /*
  // Gameplay Systems Shutdown
  item_database_shutdown();
  crafting_system_shutdown();
  combat_system_shutdown();
  */

  if (engine->subsystems.post_processing) {
    post_process_shutdown(engine->subsystems.post_processing,
                          (struct VulkanRenderer *)engine->subsystems.renderer);
    free(engine->subsystems.post_processing);
  }

  if (engine->subsystems.scene_manager) {
    scene_manager_shutdown(engine->subsystems.scene_manager);
    free(engine->subsystems.scene_manager);
  }

  if (engine->subsystems.audio) {
    audio_system_free(engine->subsystems.audio);
    free(engine->subsystems.audio);
    engine->subsystems.audio = NULL;
  }

  if (engine->subsystems.physics) {
    // physics_world_destroy(engine->subsystems.physics);
    // Note: physics_world_destroy not fully linked or implemented in stub?
    // If implemented, uncomment.
  }

  if (engine->subsystems.renderer) {
    renderer_destroy(engine->subsystems.renderer);
    engine->subsystems.renderer = NULL;
  }

  if (engine->subsystems.assets) {
    asset_manager_destroy(engine->subsystems.assets);
  }

  if (engine->subsystems.input) {
    engine->subsystems.input->shutdown(engine->subsystems.input);
    free(engine->subsystems.input);
  }

  if (engine->subsystems.entities) {
    ecs_world_destroy((World *)engine->subsystems.entities);
    engine->subsystems.entities = NULL;
  }

  vfs_free(&g_vfs);

  // Critical systems shutdown (reverse order)
  thread_pool_shutdown();
  logger_shutdown();
  memory_tracker_shutdown();

  // Shutdown memory systems (LAST)
  frame_allocator_shutdown();
  common_pools_shutdown();

  // Print final memory statistics
  printf("\n=== Final Memory Statistics ===\n");
  unified_memory_print_stats();

  // Check for leaks
  unified_memory_check_leaks();

  // Shutdown unified memory allocator
  unified_memory_shutdown();
}

// -----------------------------------------------------------------------------
// Utility
// -----------------------------------------------------------------------------

void engine_stop(Engine *engine) {
  if (engine)
    engine->state.running = false;
}

bool engine_is_running(const Engine *engine) {
  return engine ? engine->state.running : false;
}

bool engine_is_paused(const Engine *engine) {
  return engine ? engine->state.paused : false;
}

void engine_pause(Engine *engine) {
  if (engine)
    engine->state.paused = true;
}

void engine_resume(Engine *engine) {
  if (engine)
    engine->state.paused = false;
}

void engine_set_game_module(Engine *engine, GameModule *module) {
  if (engine) {
    engine->game_module = module;
    LOG_INFO("Game module '%s' attached (v%s)",
             module ? module->info.name : "NULL",
             module ? module->info.version : "N/A");
  }
}

// Subsystem access functions
Renderer *engine_get_renderer(Engine *engine) {
  return engine ? engine->subsystems.renderer : NULL;
}
AudioSystem *engine_get_audio(Engine *engine) {
  return engine ? engine->subsystems.audio : NULL;
}
InputSystem *engine_get_input(Engine *engine) {
  return engine ? engine->subsystems.input : NULL;
}
PhysicsWorld *engine_get_physics(Engine *engine) {
  return engine ? engine->subsystems.physics : NULL;
}
AssetManager *engine_get_assets(Engine *engine) {
  return engine ? engine->subsystems.assets : NULL;
}
EntityManager *engine_get_entities(Engine *engine) {
  return engine ? engine->subsystems.entities : NULL;
}
NetworkSystem *engine_get_network(Engine *engine) {
  return engine ? engine->subsystems.network : NULL;
}
UIManager *engine_get_ui(Engine *engine) {
  return engine ? engine->subsystems.ui : NULL;
}
ScriptSystem *engine_get_scripting(Engine *engine) {
  return engine ? engine->subsystems.scripting : NULL;
}

f32 engine_get_delta_time(const Engine *engine) {
  return engine ? engine->state.delta_time : 0.0f;
}

EngineConfig engine_create_default_config(void) {
  EngineConfig config = {0};
  config.window_width = 1280;
  config.window_height = 720;
  config.window_title = "Game Engine";
  config.fullscreen = false;
  config.vsync = true;
  config.render_distance = 100;
  config.fov = 45.0f;
  config.fov = 45.0f;
  config.render_scale = 1.0f;
  config.master_volume = 1.0f;
#if defined(__APPLE__)
  config.renderer_backend = GPU_BACKEND_METAL;
#else
  config.renderer_backend = GPU_BACKEND_VULKAN;
#endif
  return config;
}

f64 engine_get_total_time(const Engine *engine) {
  return engine ? engine->state.total_time : 0.0;
}

u64 engine_get_frame_count(const Engine *engine) {
  return engine ? engine->state.frame_count : 0;
}

// Debug configuration
EngineConfig engine_create_debug_config(void) {
  EngineConfig config = engine_create_default_config();
  config.window_title = "Game Engine (Debug)";
  config.vsync = false; // Disable vsync for debugging
  return config;
}

// Error string conversion
const char *engine_get_error_string(EngineError error) {
  switch (error) {
  case ENGINE_ERROR_NONE:
    return "No error";
  case ENGINE_ERROR_INIT_FAILED:
    return "Engine initialization failed";
  case ENGINE_ERROR_WINDOW_CREATION_FAILED:
    return "Window creation failed";
  case ENGINE_ERROR_RENDERER_INIT_FAILED:
    return "Renderer initialization failed";
  case ENGINE_ERROR_AUDIO_INIT_FAILED:
    return "Audio initialization failed";
  case ENGINE_ERROR_GAME_MODULE_FAILED:
    return "Game module failed";
  default:
    return "Unknown error";
  }
}

// -----------------------------------------------------------------------------
// Validation Helper Functions
// -----------------------------------------------------------------------------

static bool
engine_validate_subsystem_init(const char *name, bool success,
                               SubsystemValidationState *validation) {
  if (success) {
    LOG_INFO(" %s initialized successfully", name);
    return true;
  } else {
    LOG_ERROR(" %s initialization failed", name);
    return false;
  }
}

static void
engine_log_initialization_summary(const SubsystemValidationState *validation) {
  LOG_INFO("=== Engine Initialization Summary ===");

  // Critical systems
  LOG_INFO("Critical Systems:");
  LOG_INFO("  VFS: %s", validation->vfs_initialized ? "" : "");
  LOG_INFO("  Input: %s", validation->input_initialized ? "" : "");
  LOG_INFO("  ECS: %s", validation->ecs_initialized ? "" : "");
  LOG_INFO("  Asset Manager: %s", validation->assets_initialized ? "" : "");
  LOG_INFO("  Renderer: %s", validation->renderer_initialized ? "" : "");

  // Non-critical systems
  LOG_INFO("Non-Critical Systems:");
  LOG_INFO("  Physics: %s", validation->physics_initialized ? "" : "");
  LOG_INFO("  Scene Manager: %s",
           validation->scene_manager_initialized ? "" : "");
  LOG_INFO("  Audio: %s", validation->audio_initialized ? "" : "");
  LOG_INFO("  Post Processing: %s",
           validation->post_processing_initialized ? "" : "");
  LOG_INFO("  Perception System: %s",
           validation->perception_initialized ? "" : "");
  LOG_INFO("  Memory System: %s", validation->memory_initialized ? "" : "");
  LOG_INFO("  GOAP Planner: %s", validation->planner_initialized ? "" : "");

  // Count successful initializations
  int critical_count =
      validation->vfs_initialized + validation->input_initialized +
      validation->ecs_initialized + validation->assets_initialized +
      validation->renderer_initialized;
  int non_critical_count =
      validation->physics_initialized + validation->scene_manager_initialized +
      validation->audio_initialized + validation->post_processing_initialized +
      validation->perception_initialized + validation->memory_initialized +
      validation->planner_initialized;

  LOG_INFO("Critical Systems: %d/5 initialized", critical_count);
  LOG_INFO("Non-Critical Systems: %d/7 initialized", non_critical_count);
  LOG_INFO("=====================================");
}
