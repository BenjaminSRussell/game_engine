// Source/Runtime/Core/Engine/Private/engine_subsystems.c
#include "../Private/engine_private.h"
#include <ai/npc/perception_system.h>
#include <ai/npc_advanced/goap_enhanced.h>
#include <ai/npc_advanced/memory_system.h>
#include <audio/audio_system.h>
#include <core/asset_manager.h>
#include <core/logger.h>
#include <core/memory.h>
#include <core/resource/vfs/vfs.h>
#include <core/thread_pool.h>
#include <ecs/ecs.h>
#include <include/platform/input/input.h>
#include <physics/physics.h>
#include <rendering/post_processing.h>
#include <rendering/renderer.h>
#include <scene/scene_system.h>
#include <stdlib.h>

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

extern VFS g_vfs; // defined in engine.c or needs to be moved?
// For now, let's assume we can reference it or move definition here if it's
// subsystem related. Actually g_vfs was in engine.c. I should probably declare
// it extern in a header or define it here if appropriate. I will define it here
// if it's used primarily for subsystems, but main engine might need it. Let's
// keep definition in engine.c for now and use extern.

static bool
engine_validate_subsystem_init(const char *name, bool success,
                               SubsystemValidationState *validation) {
  if (success) {
    LOG_INFO("✓ %s initialized successfully", name);
    return true;
  } else {
    LOG_ERROR("✗ Failed to initialize %s", name);
    return false;
  }
}

static void
engine_log_initialization_summary(const SubsystemValidationState *validation) {
  LOG_INFO("=== Initialization Summary ===");
  LOG_INFO("VFS: %s", validation->vfs_initialized ? "OK" : "FAILED");
  LOG_INFO("Input: %s", validation->input_initialized ? "OK" : "FAILED");
  LOG_INFO("ECS: %s", validation->ecs_initialized ? "OK" : "FAILED");
  LOG_INFO("Assets: %s", validation->assets_initialized ? "OK" : "FAILED");
  LOG_INFO("Renderer: %s", validation->renderer_initialized ? "OK" : "FAILED");
  // ... complete logging
}

bool engine_init_subsystems(Engine *engine) {
  PlatformData *pdata =
      (PlatformData *)engine
          ->platform_data; // Needed? PlatformData needs compilation visibility.
  // PlatformData was a private struct in engine.c. I need to move it to
  // engine_private.h or engine.h? It's cast from void* platform_data. I need
  // the definition of struct PlatformData. Re-defining it here is risky if it
  // changes. I should move PlatformData definition to engine_private.h

  // Initialize validation state
  SubsystemValidationState validation = {0};
  bool critical_failure = false;

  // 1. Memory Allocator (CRITICAL)
  LOG_INFO("Initializing Memory Allocator...");
  if (!memory_tracker_init(1024)) {
    LOG_ERROR("Failed to initialize memory allocator");
    critical_failure = true;
  } else {
    LOG_INFO("✓ Memory Allocator initialized successfully");
  }

  // 2. Logging System (CRITICAL)
  LOG_INFO("Initializing Unified Logging System...");
  if (!logger_init(LOG_LEVEL_DEBUG, LOG_TARGET_CONSOLE, NULL)) {
    LOG_ERROR("Failed to initialize logging system");
    critical_failure = true;
  } else {
    LOG_INFO("✓ Logging System initialized successfully");
  }

  // 3. Thread Pool (CRITICAL)
  LOG_INFO("Initializing Thread Pool...");
  if (!thread_pool_init(
          engine->config.max_threads > 0 ? engine->config.max_threads : 4)) {
    LOG_ERROR("Failed to initialize thread pool");
    critical_failure = true;
  } else {
    LOG_INFO("✓ Thread Pool initialized successfully");
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

  // 7. Asset Manager
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

  // 9. Physics
  PhysicsConfig phys_config = {.gravity = {0.0f, -9.81f, 0.0f},
                               .fixed_timestep = 1.0f / 60.0f,
                               .velocity_iterations = 8,
                               .position_iterations = 3};
  engine->subsystems.physics = physics_world_create(phys_config);
  validation.physics_initialized = engine_validate_subsystem_init(
      "Physics", engine->subsystems.physics != NULL, &validation);

  // 10. Scene Manager
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

  // 11. Audio
  engine->subsystems.audio = (AudioSystem *)calloc(1, sizeof(AudioSystem));
  if (engine->subsystems.audio) {
    audio_system_init(engine->subsystems.audio, 32);
    validation.audio_initialized =
        engine_validate_subsystem_init("Audio", true, &validation);
  } else {
    validation.audio_initialized =
        engine_validate_subsystem_init("Audio", false, &validation);
  }

  // 12. Post Processing
  engine->subsystems.post_processing =
      (PostProcessingPipeline *)calloc(1, sizeof(PostProcessingPipeline));
  if (engine->subsystems.post_processing) {
    PostProcessingConfig pp_config = {0};
    pp_config.enabledEffects = 0;

    validation.post_processing_initialized = engine_validate_subsystem_init(
        "Post Processing",
        post_process_init(engine->subsystems.post_processing, NULL, &pp_config),
        &validation);
  } else {
    validation.post_processing_initialized =
        engine_validate_subsystem_init("Post Processing", false, &validation);
  }

  // 13. AI Systems
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

  engine_log_initialization_summary(&validation);

  return !critical_failure;
}

void engine_shutdown_subsystems(Engine *engine) {
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

  // Physics destroy...

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

  thread_pool_shutdown();
  logger_shutdown();
  memory_tracker_shutdown();
}
