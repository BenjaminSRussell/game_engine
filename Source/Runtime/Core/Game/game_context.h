#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include "platform_bootstrap.h"
#include <stdbool.h>
#include <stdint.h>

// Includes from monolithic_main.c to support GameState struct
#include "engine/include/core/logger.h"
#include <audio/audio_system.h>
#include <block/block.h>
#include <block/block_states.h>
#include <block/interaction.h>
#include <block/mining.h>
#include <chunk/chunk.h>
#include <chunk/chunk_buffers.h>
#include <combat/combat.h>
#include <combat/combat_animations.h>
#include <common.h>
#include <core/config.h>
#include <core/memory/pool.h>
#include <core/resource/vfs/vfs.h>
#include <core/threading/job.h>
#include <crafting/advanced_crafting.h>
#include <crafting/furnace.h>
#include <crafting/resource_processing.h>
#include <ecs/components/health.h>
#include <ecs/components/rigidbody.h>
#include <ecs/components/transform.h>
#include <ecs/ecs.h>
#include <game/mode.h>
#include <include/core/memory.h>
#include <include/ecs/components/npc.h>
#include <include/math/math_all.h>
#include <include/platform/input/controls.h>
#include <include/rendering/lighting.h>
#include <include/rendering/mesh.h>
#include <include/rendering/renderer.h>
#include <inventory/inventory.h>
#include <inventory/item_registry.h>
#include <npc/dialogue_manager.h>
#include <npc/npc_combat_behavior.h>
#include <npc/npc_housing.h>
#include <npc/npc_jobs.h>
#include <npc/npc_perf.h>
#include <npc/npc_schedule.h>
#include <npc/npc_visuals.h>
#include <physics/physics.h>
#include <physics/physics_system.h>
#include <player/food_spoilage.h>
#include <player/player.h>
#include <rendering/mesh_optimizer.h>
#include <rendering/particle_renderer.h>
#include <rendering/vulkan.h>
#include <save/save.h>
#include <stdlib.h>
#include <string.h>
#include <tech/solar_energy.h>
#include <ui/hud.h>
#include <ui/menu.h>
#include <ui/menu_renderer.h>
#include <weather/weather.h>
#include <world/dungeon_generation.h>
#include <world/generator.h>
#include <world/plant_vfx.h>
#include <world/settlement_generation.h>
#include <world/structures.h>
#include <world/tree_varieties.h>
#include <world/water_integration.h>
#include <world/water_physics.h>
#include <world/water_system.h>

// Forward declarations
typedef enum {
  RENDERER_UNKNOWN = 0,
  // Use RendererType from renderer.h instead
} GameRendererType;

typedef struct {
  // Core systems
  BlockRegistry block_registry;
  ItemRegistry item_registry;
  ChunkManager chunk_manager;
  WorldGenerator world_generator;
  GenerationContext gen_context; // Context for world generation
  ThreadPool thread_pool;
  GameConfig config;
  VFS vfs;

  // Rendering
  IRenderer *renderer;
  RendererType renderer_type;
  Camera camera;
  RenderState render_state;

  // Physics
  PhysicsWorld *physics_world;

  // Fixed-timestep accumulator for physics (PHY-002)
  f32 physics_accumulator;
  f32 physics_fixed_dt;
  f32 physics_max_frame_time;
  f32 physics_interpolation_alpha; // Alpha for smooth rendering between physics
                                   // states

  World ecs_world;

  // Block States
  BlockStateManager block_state_manager;

  // Player
  PlayerSystem player_system;
  InputState input_state;
  InputProfiles input_profiles;
  u32 input_profile_index;
  f32 profile_message_timer;
  char profile_message[64];
  GameModeState game_mode;

  // Save system
  SaveSystem save_system;

  // Menu
  MenuSystem menu_system;

  // Mining
  MiningState mining_state;

  // Crafting systems
  FurnaceState furnace_state;
  ProcessingMachine processing_machine;

  // Technology
  SolarEnergySystem solar_system;

  // Combat
  CombatSystem combat_system;
  CombatAnimationSystem combat_animations;

  // NPC
  NPCSystem npc_system;
  Mesh npc_batch_mesh;

  // Audio
  AudioSystem audio_system;

  // Weather
  WeatherSystem weather_system;
  WeatherParticleSystem weather_particles;

  // Water
  WaterSystem water_system;
  WaterPhysicsSystem water_physics;
  WaterIntegration water_integration;

  // Plant VFX
  PlantVFXSystem plant_vfx;
  ParticleRenderer particle_renderer;

  // Game state
  bool running;
  bool in_game;
  InGameState in_game_state;
  f32 delta_time;
  u64 last_frame_time;

  // Window
  void *window;
  u32 window_width;
  u32 window_height;
  HUDSystem hud;
} GameState;

// Global game state access
extern GameState g_game;
extern InGameState g_in_game_state;
extern HUDSystem g_hud;
extern ItemRegistry g_item_registry;

#endif
