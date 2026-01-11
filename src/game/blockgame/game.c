#include <core/logger.h>
#include <ecs/ecs.h>
#include <game/game.h>
#include <game/game_hooks.h>
#include <game/mode.h>
#include <mobs/mob_spawning.h>
#include <modding/mod_api.h>
#include <stdio.h>
#include <stdlib.h>

static struct GameState s_state = {0, 0};
static World *s_ecs_world = NULL;
static GameModeState *s_game_mode = NULL;
static struct ModRegistry *s_mod_registry = NULL;
static MobManager *s_mob_manager = NULL;
static MobSpawner *s_mob_spawner = NULL;

// Global references (declared in other systems)
extern struct ChunkManager *g_chunk_manager;
extern struct WorldGenerator *g_world_generator;
extern struct PlayerSystem *g_player_system;
extern World *g_ecs_world;

int game_init(void) {
  LOG_INFO("Initializing game systems...");

  s_state.running = 1;
  s_state.tick_count = 0;

  // Initialize ECS world
  s_ecs_world = (World *)malloc(sizeof(World));
  if (!s_ecs_world) {
    LOG_ERROR("Failed to allocate ECS world");
    return -1;
  }
  ecs_world_init(s_ecs_world, 1024, 32, 16384);
  LOG_INFO("ECS world initialized");

  // Initialize game mode (default: Survival, Normal difficulty)
  s_game_mode = (GameModeState *)malloc(sizeof(GameModeState));
  if (!s_game_mode) {
    LOG_ERROR("Failed to allocate game mode");
    ecs_world_free(s_ecs_world);
    free(s_ecs_world);
    s_ecs_world = NULL;
    return -1;
  }

  game_mode_init(s_game_mode, GAME_MODE_SURVIVAL, DIFFICULTY_NORMAL);
  LOG_INFO("Game mode initialized (Survival, Normal)");

  // Initialize mod registry and hooks
  s_mod_registry = mod_registry_create();
  if (!s_mod_registry) {
    LOG_ERROR("Failed to create mod registry");
    free(s_game_mode);
    s_game_mode = NULL;
    ecs_world_free(s_ecs_world);
    free(s_ecs_world);
    s_ecs_world = NULL;
    return -1;
  }
  game_hook_init(s_mod_registry);
  LOG_INFO("Mod system initialized");

  // Initialize mob system (Phase 2a/2b)
  s_mob_manager = (MobManager *)malloc(sizeof(MobManager));
  if (s_mob_manager) {
    mob_manager_init(s_mob_manager, 500); // Max 500 mobs
    LOG_INFO("Mob manager initialized (500 mob capacity)");

    // Initialize mob spawner (Phase 2b)
    s_mob_spawner = (MobSpawner *)malloc(sizeof(MobSpawner));
    if (s_mob_spawner) {
      mob_spawner_init(s_mob_spawner, s_mob_manager, g_chunk_manager,
                       g_world_generator, g_player_system);
      LOG_INFO("Mob spawner initialized");
    } else {
      LOG_ERROR("Failed to allocate mob spawner");
    }
  } else {
    LOG_ERROR("Failed to allocate mob manager");
  }

  // Set global ECS world reference for mob AI
  g_ecs_world = s_ecs_world;

  // Trigger world init hooks
  game_hook_trigger_world_init(&s_state, s_game_mode);

  LOG_INFO("Game systems initialized successfully");
  return 0;
}

void game_tick(float dt) {
  if (!s_state.running)
    return;

  s_state.tick_count++;

  // Update mod registry (runs mod update hooks)
  if (s_mod_registry) {
    mod_registry_update(s_mod_registry, dt);
  }

  // Trigger world tick hooks
  game_hook_trigger_world_tick(dt, s_state.tick_count);

  // Update ECS world (entities, systems)
  if (s_ecs_world) {
    ecs_update_systems(s_ecs_world, dt);
  }

  // Update mob AI (Phase 2a)
  if (s_mob_manager) {
    mob_update(s_mob_manager, dt);
  }

  // Spawn/despawn mobs every 1 second (20 ticks at 20 ticks/sec)
  // Throttle to avoid constant spawning
  static u32 spawn_tick_counter = 0;
  spawn_tick_counter++;
  if (spawn_tick_counter >= 20) {
    spawn_tick_counter = 0;

    if (s_mob_spawner) {
      mob_spawner_update(s_mob_spawner, 1.0f);
      mob_spawner_despawn_distant(s_mob_spawner);
      mob_spawner_despawn_old(s_mob_spawner);
    }
  }

  // Update game mode-specific logic
  if (s_game_mode && s_game_mode->mode == GAME_MODE_SURVIVAL && s_ecs_world) {
    // Note: survival_update requires specific parameters - this is a
    // placeholder In production, the player entity would need to be tracked and
    // passed here survival_update(s_game_mode, s_ecs_world, player_entity, dt);
  }
}

void game_shutdown(void) {
  LOG_INFO("Shutting down game systems...");

  s_state.running = 0;

  // Trigger shutdown hooks before cleanup
  if (s_mod_registry) {
    mod_registry_trigger_hook(s_mod_registry, MOD_HOOK_SHUTDOWN, NULL);
  }

  // Cleanup mod registry
  if (s_mod_registry) {
    mod_registry_destroy(s_mod_registry);
    s_mod_registry = NULL;
  }

  // Cleanup mob systems (Phase 2a/2b)
  if (s_mob_spawner) {
    mob_spawner_free(s_mob_spawner);
    free(s_mob_spawner);
    s_mob_spawner = NULL;
  }

  if (s_mob_manager) {
    mob_manager_free(s_mob_manager);
    free(s_mob_manager);
    s_mob_manager = NULL;
  }

  // Cleanup game mode
  if (s_game_mode) {
    free(s_game_mode);
    s_game_mode = NULL;
  }

  // Cleanup ECS world
  if (s_ecs_world) {
    ecs_world_free(s_ecs_world);
    free(s_ecs_world);
    s_ecs_world = NULL;
  }

  LOG_INFO("Game systems shut down");
}

struct GameState *game_state(void) { return &s_state; }

World *game_get_ecs_world(void) { return s_ecs_world; }

GameModeState *game_get_mode_state(void) { return s_game_mode; }

struct ModRegistry *game_get_mod_registry(void) { return s_mod_registry; }
