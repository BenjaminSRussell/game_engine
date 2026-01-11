// src/engine/core/engine.c
//
// Purpose: Core engine implementation conforming to engine.h interface
//
#include <core/engine.h>
#include <core/game_loop.h>
#include <core/game_module.h>
#include <core/logger.h>
#include <core/memory.h>
#include <core/performance.h>
#include <core/string_utils.h>
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

// Forward declarations
static bool engine_init_subsystems(Engine *engine);
static void engine_shutdown_subsystems(Engine *engine);
static void engine_update_callback(void *user_data, f32 delta_time);
static void engine_render_callback(void *user_data, f32 interpolation);

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

  // Initialize Profiler
  profiler_init();

  // Copy config
  engine->config = *config;

  // Allocate platform data
  engine->platform_data = calloc(1, sizeof(PlatformData));
  PlatformData *pdata = (PlatformData *)engine->platform_data;

  LOG_INFO("Initializing Engine Core...");

  // Initialize Window
  LOG_INFO("Creating Window '%s' (%dx%d)...", config->window_title,
           config->window_width, config->window_height);

  if (!window_init(&pdata->window, config->window_width, config->window_height,
                   config->window_title ? config->window_title : "Game Engine",
                   config->fullscreen)) {
    LOG_ERROR("Failed to create window");
    return false;
  }

  // Initialize Game Loop
  // Use fixed timestep of 60 FPS typically
  f32 target_fps = 60.0f;
  game_loop_init(&pdata->loop, 1.0f / target_fps);
  game_loop_set_update_callback(&pdata->loop, engine_update_callback);
  game_loop_set_render_callback(&pdata->loop, engine_render_callback);
  game_loop_set_user_data(&pdata->loop, engine);

  // Initialize Internal Subsystems (Asset Manager, ECS, etc.)
  if (!engine_init_subsystems(engine)) {
    LOG_ERROR("Failed to initialize engine subsystems");
    // window_shutdown(&pdata->window);
    return false;
  }

  // Initialize Hot Reload with default config
  HotReloadConfig hr_config = {
      .enable_code_hot_reload = true,
      .enable_asset_hot_reload = true,
      .watch_path = "." // Current directory for now
  };
  hot_reload_init(hr_config);

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
  if (!engine || !engine->state.initialized)
    return;

  LOG_INFO("Shutting down Engine...");

  PlatformData *pdata = (PlatformData *)engine->platform_data;

  engine_shutdown_subsystems(engine);

  hot_reload_shutdown(); // Shutdown hot reload

  game_loop_shutdown(&pdata->loop);
  // window_shutdown(&pdata->window);

  if (engine->platform_data) {
    free(engine->platform_data);
  }

  memset(engine, 0, sizeof(Engine));

  profiler_shutdown(); // Shutdown profiler last

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

  // 1. VFS
  vfs_init(&g_vfs);
  vfs_mount(&g_vfs, "assets", "assets");
  LOG_INFO("VFS initialized and 'assets' mounted");

  // 2. Input
  if (pdata->window.is_hosted) {
    engine->subsystems.input = create_host_input_system();
  } else {
    engine->subsystems.input = create_glfw_input_system();
  }

  if (!engine->subsystems.input) {
    LOG_ERROR("Failed to create input system");
    return false;
  }
  InputConfig input_config = input_create_default_config();
  if (!engine->subsystems.input->init(engine->subsystems.input,
                                      &input_config)) {
    LOG_ERROR("Input initialization failed");
    return false;
  }
  LOG_INFO("Input System initialized");

  // 3. ECS
  WorldConfig world_config = ecs_world_create_default_config();
  engine->subsystems.entities =
      (EntityManager *)ecs_world_create(&world_config);
  if (!engine->subsystems.entities) {
    LOG_ERROR("ECS initialization failed");
    return false;
  }
  LOG_INFO("ECS initialized");

  // 2. Asset Manager (requires ECS)
  engine->subsystems.assets =
      asset_manager_create(512, (World *)engine->subsystems.entities, &g_vfs);
  if (!engine->subsystems.assets) {
    LOG_ERROR("Asset Manager initialization failed");
    return false;
  }
  engine->subsystems.assets->vfs = &g_vfs;
  LOG_INFO("Asset Manager initialized");

  // 4. Renderer
  engine->subsystems.renderer = renderer_create_with_backend(
      RENDERER_TYPE_VOXEL, engine->config.renderer_backend);
  if (!engine->subsystems.renderer) {
    LOG_ERROR("Failed to create renderer");
    return false;
  }

  RendererInitParams render_params = {.window = &pdata->window,
                                      .width = engine->config.window_width,
                                      .height = engine->config.window_height,
                                      .type = RENDERER_TYPE_VOXEL,
                                      .backend =
                                          engine->config.renderer_backend,
                                      .config = NULL};
  if (!engine->subsystems.renderer->init(engine->subsystems.renderer,
                                         &render_params)) {
    LOG_ERROR("Failed to init renderer");
    return false;
  }

  // 5. Physics
  PhysicsConfig phys_config = {.gravity = {0.0f, -9.81f, 0.0f},
                               .fixed_timestep = 1.0f / 60.0f,
                               .velocity_iterations = 8,
                               .position_iterations = 3};
  engine->subsystems.physics = physics_world_create(phys_config);
  if (!engine->subsystems.physics) {
    LOG_ERROR("Failed to initialize physics subsystem");
    // continue anyway, maybe fallback?
  } else {
    LOG_INFO("Physics System initialized");
  }

  // 6. Scene Manager
  engine->subsystems.scene_manager =
      (SceneManager *)calloc(1, sizeof(SceneManager));
  if (scene_manager_init(engine->subsystems.scene_manager)) {
    LOG_INFO("Scene Manager initialized");
  } else {
    LOG_ERROR("Scene Manager initialization failed");
  }

  // 10. Audio System
  engine->subsystems.audio = (AudioSystem *)calloc(1, sizeof(AudioSystem));
  if (engine->subsystems.audio) {
    audio_system_init(engine->subsystems.audio, 32); // 32 channels
    LOG_INFO("Audio System initialized");
  }

  // 11. Scripting System
  /*
  engine->subsystems.scripting =
      (ScriptSystem *)calloc(1, sizeof(ScriptSystem));
  if (engine->subsystems.scripting) {
    if (ScriptSystem_Init(engine->subsystems.scripting)) {
      LOG_INFO("Scripting System initialized");
    } else {
      LOG_ERROR("Scripting System initialization failed");
    }
  }
  */

  // 12. Network System (Stub for now, but structures exist)
  // engine->subsystems.network = ...

  // 7. Post Processing
  engine->subsystems.post_processing =
      (PostProcessingPipeline *)calloc(1, sizeof(PostProcessingPipeline));
  PostProcessingConfig pp_config = {0};
  pp_config.enabledEffects = 0; // Default off

  // Note: post_process_init requires VulkanRenderer*, but we have IRenderer*.
  // Post-processing is currently Vulkan-only. If using Metal, skip it or use
  // Metal-native path.
  bool use_metal_pp = false;
#ifdef GPU_BACKEND_METAL
  use_metal_pp = true;
#endif

  if (use_metal_pp) {
    LOG_INFO("Post Processing: Metal path detected, using Metal-native pp");
    if (post_process_init(engine->subsystems.post_processing,
                          NULL, // No Vulkan instance needed
                          &pp_config)) {
      LOG_INFO("Post Processing initialized (Metal)");
    } else {
      LOG_INFO("Post Processing initialization skipped or failed (Metal)");
    }
  } else {
    if (post_process_init(engine->subsystems.post_processing,
                          NULL, // Pass Vulkan context if available
                          &pp_config)) {
      LOG_INFO("Post Processing initialized (Vulkan)");
    } else {
      LOG_INFO("Post Processing skipped (Vulkan renderer not available)");
    }
  }

  // 8. Gameplay Systems Integration
  /*
  World *world = (World *)engine->subsystems.entities;
  if (world) {
    // Combat System
    if (combat_system_init(world)) {
      LOG_INFO("Combat System initialized");
    } else {
      LOG_ERROR("Combat System initialization failed");
    }

    // Inventory System
    if (item_database_init(1000)) {
      item_database_register_defaults();
      LOG_INFO("Inventory/Item Database initialized");
    } else {
      LOG_ERROR("Inventory System initialization failed");
    }

    // Crafting System
    if (crafting_system_init(1000)) {

      crafting_register_default_recipes();
      LOG_INFO("Crafting System initialized");
    } else {
      LOG_ERROR("Crafting System initialization failed");
    }
  }
  */

  // 9. AI Systems
  // Perception System
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
    perception_system_initialize(engine->subsystems.perception);
    LOG_INFO("Perception System initialized");
  } else {
    LOG_ERROR("Perception System initialization failed");
  }

  // Memory System
  engine->subsystems.memory = memory_system_create(100);
  if (engine->subsystems.memory) {
    LOG_INFO("Memory System initialized");
  } else {
    LOG_ERROR("Memory System initialization failed");
  }

  // GOAP Planner
  engine->subsystems.planner = goap_planner_create_state(256);
  if (engine->subsystems.planner) {
    LOG_INFO("GOAP Planner initialized");
  } else {
    LOG_ERROR("GOAP Planner initialization failed");
  }

  return true;
}

static void engine_shutdown_subsystems(Engine *engine) {
  if (engine->subsystems.entities) {
    ecs_world_destroy((World *)engine->subsystems.entities);
    engine->subsystems.entities = NULL;
  }

  if (engine->subsystems.assets) {
    asset_manager_destroy(engine->subsystems.assets);
  }

  if (engine->subsystems.input) {
    engine->subsystems.input->shutdown(engine->subsystems.input);
    free(engine->subsystems.input);
  }

  if (engine->subsystems.renderer) {
    renderer_destroy(engine->subsystems.renderer);
    engine->subsystems.renderer = NULL;
  }

  if (engine->subsystems.physics) {
    // physics_world_destroy(engine->subsystems.physics);
    // Note: physics_world_destroy not fully linked or implemented in stub?
    // If implemented, uncomment.
  }

  if (engine->subsystems.audio) {
    audio_system_free(engine->subsystems.audio);
    free(engine->subsystems.audio);
    engine->subsystems.audio = NULL;
  }

  /*
  if (engine->subsystems.scripting) {
    ScriptSystem_Shutdown(engine->subsystems.scripting);
    free(engine->subsystems.scripting);
    engine->subsystems.scripting = NULL;
  }
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

  vfs_free(&g_vfs);
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
