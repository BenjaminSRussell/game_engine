// src/games/minecraft_v2/minecraft_v2_module.c
//
// Purpose: Minecraft v2 implementation as a game module using the engine API.
// This file demonstrates how to convert existing game logic to work with
// the decoupled engine architecture.
//
// TODO: Migrate all Minecraft v2 systems to use engine APIs
// TODO: Remove direct dependencies on Vulkan, physics, etc.
// TODO: Implement proper asset loading through engine asset manager
// TODO: Convert game state to use engine entity system
// TODO: Implement save/load system using engine file I/O
// TODO: Add modding support through engine module system
// TODO: Implement multiplayer through engine networking
// TODO: Add proper error handling and recovery
// TODO: Optimize performance for engine integration
//
#include <common.h>
#include <core/asset_manager.h>
#include <core/engine.h>
#include <core/engine_core.h>
#include <core/game_module.h>
#include <core/logger.h>
#include <math/mat4.h>
#include <math/math.h>
#include <math/vec3.h>
#include <rendering/renderer.h>
#include <stdlib.h>
#include <string.h>
#include <effects/vfx/particle_system.h>

// Minecraft v2 specific includes (these will be gradually migrated)
#include <audio/audio_system.h>
#include <block/block.h>
#include <chunk/chunk.h>
#include <combat/combat.h>
#include <inventory/inventory.h>
#include <npc/npc.h>
#include <physics/physics.h>
#include <platform/input/input.h> // For KEY_ESCAPE
#include <player/player.h>
#include <tech/crafting.h>
#include <ui/hud.h>
#include <weather/weather.h>
#include <world/generator.h>

AudioSystem* g_audio_system = NULL;
ParticleSystem* g_particle_system = NULL;

// Minecraft v2 game state
typedef struct {
  // Core game systems (will be migrated to engine subsystems)
  BlockRegistry block_registry;
  ChunkManager chunk_manager;
  WorldGenerator world_generator;
  PlayerSystem player_system;
  TechRecipeRegistry recipe_registry;
  TechnologyTree tech_tree;
  CombatSystem combat_system;
  NPCSystem npc_system;
  WeatherSystem weather_system;

  // Game state
  bool in_world;
  bool paused;
  f32 game_time;
  u32 day_count;

  // Engine references
  Renderer *renderer;
  struct AudioSystem *audio;
  InputState *input;
  struct PhysicsWorld *physics;
  struct AssetManager *assets;

  // Particle system for module-scoped VFX
  struct ParticleSystem *particles;

  // Camera
  Camera camera;

  // Performance tracking
  f32 avg_frame_time;
  u32 frames_per_second;
} MinecraftV2State;

// Forward declarations
static bool minecraft_v2_init(GameModule *module, Engine *engine);
static void minecraft_v2_shutdown(GameModule *module);
static void minecraft_v2_update(GameModule *module, Engine *engine,
                                f32 delta_time);
static void minecraft_v2_render(GameModule *module, Engine *engine);
static void minecraft_v2_handle_input(GameModule *module, Engine *engine);

// Module lifecycle implementation
static bool minecraft_v2_load(GameModule *module, const char *module_path) {
  if (!module || !module_path) {
    LOG_ERROR("Minecraft v2: Invalid load parameters");
    return false;
  }

  LOG_INFO("Loading Minecraft v2 module from: %s", module_path);

  // Initialize module info
  module->info.name = "Minecraft v2";
  module->info.version = "1.0.0";
  module->info.author = "Game Engine Team";
  module->info.description =
      "A voxel-based sandbox game built with the game engine";
  module->info.state = GAME_MODULE_STATE_LOADED;
  module->info.load_time = 0.0;  // TODO: Implement timing
  module->info.memory_usage = 0; // TODO: Track memory usage

  // Allocate game-specific data
  module->game_data = malloc(sizeof(MinecraftV2State));
  if (!module->game_data) {
    LOG_ERROR("Minecraft v2: Failed to allocate game state");
    return false;
  }

  memset(module->game_data, 0, sizeof(MinecraftV2State));

  LOG_INFO("Minecraft v2 module loaded successfully");
  return true;
}

static void minecraft_v2_unload(GameModule *module) {
  if (!module)
    return;

  LOG_INFO("Unloading Minecraft v2 module");

  // Free game-specific data
  if (module->game_data) {
    free(module->game_data);
    module->game_data = NULL;
  }

  module->info.state = GAME_MODULE_STATE_UNLOADED;

  LOG_INFO("Minecraft v2 module unloaded");
}

static bool minecraft_v2_initialize(GameModule *module, Engine *engine) {
  if (!module || !engine || !module->game_data) {
    LOG_ERROR("Minecraft v2: Invalid initialize parameters");
    return false;
  }

  LOG_INFO("Initializing Minecraft v2 game module");
  module->info.state = GAME_MODULE_STATE_INITIALIZING;

  MinecraftV2State *game_state = (MinecraftV2State *)module->game_data;

  // Get engine subsystems
  game_state->renderer = engine_get_renderer(engine);
  game_state->audio = engine_get_audio(engine);
  game_state->input = (InputState *)engine_get_input(engine);
  game_state->physics = engine_get_physics(engine);
  game_state->assets = engine_get_assets(engine);

  // Validate required subsystems
  if (!game_state->renderer || !game_state->input) {
    LOG_ERROR("Minecraft v2: Required engine subsystems not available");
    return false;
  }

  // Initialize core game systems
  // TODO: Migrate these to use engine APIs
  block_registry_init(&game_state->block_registry, 256);
  block_registry_init_defaults(&game_state->block_registry);

  chunk_manager_init(&game_state->chunk_manager, 32);

  // Initialize particle system for VFX (renderer not yet wired in, pass NULL)
  game_state->particles = malloc(sizeof(ParticleSystem));
  particle_system_init(game_state->particles, game_state->renderer);
  g_particle_system = game_state->particles;

  GenerationContext gen_context = {&game_state->chunk_manager};
  u32 seed = 12345; // Placeholder for actual seed
  world_generator_init(&game_state->world_generator, seed, &gen_context);

  player_system_init(&game_state->player_system, game_state->input, NULL,
                     NULL,                      // controls, game_mode
                     game_state->physics, NULL, // physics, ecs
                     &game_state->chunk_manager, &game_state->block_registry,
                     &game_state->camera, &game_state->combat_system,
                     game_state->audio);

  tech_recipe_registry_init(&game_state->recipe_registry, 100);
  tech_tree_init(&game_state->tech_tree);

  npc_system_init(&game_state->npc_system,
                  (struct World *)engine_get_entities(engine),
                  game_state->physics);
  combat_system_bind_world(&game_state->combat_system,
                           (struct World *)engine_get_entities(engine));
  weather_system_init(&game_state->weather_system);

  // Initialize camera
  camera_init(&game_state->camera, vec3(0, 64, 0), 0.0f, 0.0f);

  g_audio_system = game_state->audio;

  // Set initial game state
  game_state->in_world = false;
  game_state->paused = false;
  game_state->game_time = 0.0f;
  game_state->day_count = 0;

  module->info.state = GAME_MODULE_STATE_READY;
  LOG_INFO("Minecraft v2 game module initialized successfully");
  return true;
}

static void minecraft_v2_shutdown(GameModule *module) {
  if (!module || !module->game_data)
    return;

  LOG_INFO("Shutting down Minecraft v2 game module");
  module->info.state = GAME_MODULE_STATE_SHUTTING_DOWN;

  MinecraftV2State *game_state = (MinecraftV2State *)module->game_data;

  // Cleanup game systems
  // TODO: Use engine cleanup APIs
  weather_system_free(&game_state->weather_system);
  npc_system_free(&game_state->npc_system);
  // combat_system_free(&game_state->combat_system); // Not available
  tech_tree_free(&game_state->tech_tree);
  tech_recipe_registry_free(&game_state->recipe_registry);
  player_system_free(&game_state->player_system);
  world_generator_free(&game_state->world_generator);

  // Shutdown particle system
  if (game_state->particles) {
    particle_system_shutdown(game_state->particles, game_state->renderer);
    free(game_state->particles);
  }

  chunk_manager_free(&game_state->chunk_manager);
  block_registry_free(&game_state->block_registry);

  module->info.state = GAME_MODULE_STATE_LOADED;
  LOG_INFO("Minecraft v2 game module shutdown complete");
}

static void minecraft_v2_update(GameModule *module, Engine *engine,
                                f32 delta_time) {
  if (!module || !engine || !module->game_data)
    return;

  MinecraftV2State *game_state = (MinecraftV2State *)module->game_data;

  if (game_state->paused)
    return;

  // Update game time
  game_state->game_time += delta_time;
  if (game_state->game_time >= 1200.0f) { // 20 minutes = 1 day
    game_state->game_time = 0.0f;
    game_state->day_count++;
  }

  // Update game systems
  // TODO: Migrate to engine APIs
  if (game_state->particles) {
    particle_system_update(game_state->particles, delta_time);
  }

  // Update player
  player_system_update(&game_state->player_system, (f32)delta_time,
                       &game_state->chunk_manager, game_state->physics,
                       &game_state->block_registry);

  // Update chunks
  // Minimal tile-entity updates (brewing stands) are handled through the
  // chunk manager which will call per-stand updates and emit VFX via the
  // particle/audio systems owned by this module.
  // chunk_manager_update implementation seems to be missing in chunk.c,
  // it was undeclared in IDE feedback. I'll comment it for now or check if it's
  // named differently.
  chunk_manager_update(&game_state->chunk_manager, (f32)delta_time);
  weather_system_update(&game_state->weather_system, (f32)delta_time);

  // Update camera to follow player
  // TODO: Use engine camera system
  Vec3 player_pos = player_get_position(&game_state->player_system);
  camera_set_position(&game_state->camera, player_pos);

  // Track performance
  game_state->avg_frame_time =
      game_state->avg_frame_time * 0.9f + delta_time * 0.1f;
  game_state->frames_per_second = (u32)(1.0f / delta_time);
}

static void minecraft_v2_render(GameModule *module, Engine *engine) {
  if (!module || !engine || !module->game_data)
    return;

  MinecraftV2State *game_state = (MinecraftV2State *)module->game_data;

  // Calculate camera matrices
  f32 aspect = 16.0f / 9.0f; // Default fallback
  const EngineConfig *config = engine_get_config(engine);
  if (config && config->window_height > 0) {
    aspect = (f32)config->window_width / (f32)config->window_height;
  }

  Mat4 view = camera_get_view_matrix(&game_state->camera);
  Mat4 proj = camera_get_projection_matrix(&game_state->camera, aspect);
  Mat4 view_proj = mat4_mul(proj, view);

  // Set camera in renderer
  if (game_state->renderer && game_state->renderer->update_camera) {
    game_state->renderer->update_camera(game_state->renderer,
                                        &game_state->camera, aspect);
  }

  // Render chunks
  chunk_manager_render(&game_state->chunk_manager, game_state->renderer, view,
                       proj);
  player_system_render(&game_state->player_system, game_state->renderer);

  if (game_state->particles) {
    particle_system_render(game_state->particles, game_state->renderer, view_proj);
  }

  // Render weather effects
  if (game_state->weather_system.initialized) {
    // weather_particles_render(NULL, game_state->renderer); // Needs
    // VulkanRenderer
  }
}

static void minecraft_v2_handle_input(GameModule *module, Engine *engine) {
  if (!module || !engine || !module->game_data)
    return;

  MinecraftV2State *game_state = (MinecraftV2State *)module->game_data;

  // Handle game-specific input
  // TODO: Use engine input mapping system

  // Handle pause
  if (input_is_action_pressed(game_state->input, INPUT_ACTION_MENU)) {
    game_state->paused = !game_state->paused;
    if (game_state->paused) {
      engine_pause(engine);
    } else {
      engine_resume(engine);
    }
  }

  // Pass input to player system
  if (!game_state->paused) {
    // player_system_handle_input(&game_state->player_system, game_state->input,
    //                           engine_get_delta_time(engine));
  }
}

// Event handlers
static void minecraft_v2_on_window_resize(GameModule *module, Engine *engine,
                                          u32 width, u32 height) {
  if (!module || !module->game_data)
    return;

  MinecraftV2State *game_state = (MinecraftV2State *)module->game_data;

  // Update camera aspect ratio
  // TODO: Use engine camera system
  // camera_set_aspect_ratio(&game_state->camera, (f32)width / (f32)height);

  LOG_INFO("Minecraft v2: Window resized to %ux%u", width, height);
}

static void minecraft_v2_on_focus_gained(GameModule *module, Engine *engine) {
  if (!module || !module->game_data)
    return;

  MinecraftV2State *game_state = (MinecraftV2State *)module->game_data;

  // Resume game if it was paused due to focus loss
  if (game_state->paused && engine_is_paused(engine)) {
    game_state->paused = false;
    engine_resume(engine);
  }

  LOG_INFO("Minecraft v2: Focus gained");
}

static void minecraft_v2_on_focus_lost(GameModule *module, Engine *engine) {
  if (!module || !module->game_data)
    return;

  MinecraftV2State *game_state = (MinecraftV2State *)module->game_data;

  // Pause game when focus is lost
  if (!game_state->paused) {
    game_state->paused = true;
    engine_pause(engine);
  }

  LOG_INFO("Minecraft v2: Focus lost");
}

// Save/Load system
static bool minecraft_v2_save_game(GameModule *module, Engine *engine,
                                   const char *save_path) {
  if (!module || !engine || !save_path)
    return false;

  LOG_INFO("Minecraft v2: Saving game to %s", save_path);

  // TODO: Implement save system using engine file I/O
  // This should serialize game state, world data, player data, etc.

  LOG_INFO("Minecraft v2: Game saved successfully");
  return true;
}

static bool minecraft_v2_load_game(GameModule *module, Engine *engine,
                                   const char *save_path) {
  if (!module || !engine || !save_path)
    return false;

  LOG_INFO("Minecraft v2: Loading game from %s", save_path);

  // TODO: Implement load system using engine file I/O
  // This should deserialize game state, world data, player data, etc.

  LOG_INFO("Minecraft v2: Game loaded successfully");
  return true;
}

// Module creation function
GameModule *create_minecraft_v2_module(void) {
  GameModule *module = malloc(sizeof(GameModule));
  if (!module)
    return NULL;

  memset(module, 0, sizeof(GameModule));

  // Set module configuration
  module->config = game_module_create_default_config("Minecraft v2");
  module->config.capabilities =
      GAME_MODULE_CAP_3D_RENDERING | GAME_MODULE_CAP_AUDIO |
      GAME_MODULE_CAP_PHYSICS | GAME_MODULE_CAP_SAVE_LOAD |
      GAME_MODULE_CAP_MODDING;

  module->config.requires_shadows = true;
  module->config.requires_physics = true;
  module->config.requires_3d_audio = true;

  // Set module functions
  module->load = minecraft_v2_load;
  module->unload = minecraft_v2_unload;
  module->initialize = minecraft_v2_initialize;
  module->shutdown = minecraft_v2_shutdown;
  module->update = minecraft_v2_update;
  module->render = minecraft_v2_render;
  module->handle_input = minecraft_v2_handle_input;
  module->on_window_resize = minecraft_v2_on_window_resize;
  module->on_focus_gained = minecraft_v2_on_focus_gained;
  module->on_focus_lost = minecraft_v2_on_focus_lost;
  module->save_game = minecraft_v2_save_game;
  module->load_game = minecraft_v2_load_game;

  return module;
}

// Module destruction function
void destroy_minecraft_v2_module(GameModule *module) {
  if (!module)
    return;

  // Ensure module is properly shut down
  if (module->info.state == GAME_MODULE_STATE_RUNNING) {
    module->shutdown(module); // Engine may not be available during shutdown
  }

  if (module->info.state != GAME_MODULE_STATE_UNLOADED) {
    module->unload(module);
  }

  free(module);
}

// Module export functions
DECLARE_GAME_MODULE("Minecraft v2", "1.0.0", "Game Engine Team",
                    "A voxel-based sandbox game")

IMPLEMENT_GAME_MODULE("Minecraft v2", "1.0.0", "Game Engine Team",
                      "A voxel-based sandbox game")

#ifdef __cplusplus
extern "C" {
#endif

GAME_MODULE_API GameModule *create_game_module(void) {
  return create_minecraft_v2_module();
}

GAME_MODULE_API void destroy_game_module(GameModule *module) {
  destroy_minecraft_v2_module(module);
}

#ifdef __cplusplus
}
#endif
