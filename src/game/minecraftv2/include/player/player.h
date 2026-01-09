// include/player/player.h
//
// Purpose: Defines the comprehensive player system, including the
// `PlayerComponent` and `PlayerSystem`. This header centralizes all data and
// logic related to the player character, covering aspects such as physical
// state (position, velocity, physics body), camera control, inventory
// management, combat, magic abilities, vehicle interaction, block interaction
// (mining, placing), and various player-specific statistics (health, hunger,
// experience). It acts as a primary interface for interacting with the player
// character from other game systems.
//
// Public APIs:
// - `InGameState`: Enumeration defining various in-game states the player can
// be in (playing, crafting, inventory, menu).
// - `PlayerComponent`: A rich structure containing all player-specific data:
//   - `entity_id`: Link to the player's ECS entity.
//   - `velocity`, `acceleration`, `yaw`, `pitch`: Movement and orientation.
//   - `physics_body`, `on_ground`, `is_flying`: Physics-related state.
//   - `walk_speed`, `sprint_speed`, `jump_force`, `gravity`: Movement
//   parameters.
//   - `inventory`, `hotbar`: Item management.
//   - `equipped_weapon`, `equipped_armor`: Equipment slots.
//   - `spirit_model`, `combat_state`, `magic_component`, `vehicle_state`,
//   `status_effects`:
//     Components integrated from other systems.
//   - `reach_distance`, `target_entity`, `target_block`: Interaction data.
//   - `mining_active`, `mining_progress`: Mining state.
//   - `is_sprinting`, `is_crouching`, `stamina`: Action states and resources.
//   - `hunger`, `saturation`, `experience`, `oxygen`: Survival stats.
//   - `camera_mode`, `look_x_smooth`, `deadzone`: Camera and input settings.
// - `PlayerSystem`: The main system managing the player. Holds references to
// other
//   core systems (`InputState`, `GameConfig`, `PhysicsWorld`, `ECSWorld`,
//   `ChunkManager`, `BlockRegistry`, `Camera`, `CombatSystem`, `AudioSystem`).
// - `player_system_init`: Initializes the player system, linking it to various
// game modules.
// - `player_system_free`: Frees resources associated with the player system.
// - `player_create`: Spawns and initializes the player entity in the world.
// - `player_update`, `player_system_update`: Main update functions for the
// player's state and interactions.
// - `player_update_movement`, `player_update_physics`, `player_update_camera`,
// `player_update_interaction`:
//   Sub-systems for updating specific player aspects.
// - Player actions: `player_jump`, `player_sprint`, `player_crouch`,
// `player_fly`.
// - Block interaction: `player_place_block`, `player_break_block`,
// `player_interact_block`.
// - Inventory management: `player_add_item`, `player_remove_item`,
// `player_get_item_count`, etc.
// - Equipment: `player_equip_weapon`, `player_equip_armor`.
// - Vehicle interaction: `player_enter_vehicle`, `player_exit_vehicle`.
// - Queries: `player_get_position`, `player_get_forward`, `player_get_right`,
// `player_get_health`, etc.
// - Health & Status: `player_damage`, `player_heal`, `player_set_hunger`, etc.
// - Environment interactions: `player_enter_water`, `player_check_fall_damage`,
// etc.
// - Camera control: `player_set_camera_mode`.
// - Input settings: `player_set_deadzone`, `player_set_invert_y`.
// - `player_respawn`: Respawns the player at a given position.
//
// Ownership: The `PlayerSystem` manages the `PlayerComponent` instance. The
// `PlayerComponent` owns its `Inventory`, `Hotbar`, `SpiritModelComponent`,
// `PlayerCombatState`, `PlayerMagicComponent`, `VehicleState`, and
// `StatusEffectManager`. It holds references to other core system instances.
//
// Invariants:
// - A `PlayerSystem` must be initialized before player creation or updates.
// - The player's `entity_id` should always be valid within the `ECSWorld`.
// - `delta_time` should be consistently passed to update functions.
// - External systems referenced (e.g., `PhysicsWorld`, `InputState`) must be
// properly
//   initialized and managed externally.
//
#ifndef PLAYER_H
#define PLAYER_H

#include <block/block.h>
#include <combat/combat.h>
#include <ecs/ecs.h>
#include <game/mode.h>
#include <game_common.h>
#include <platform/input/controls.h>
#include <inventory/inventory.h>
#include <math/vec3.h>
#include <physics/physics.h>
#include <player/player_combat.h>
#include <player/player_magic.h>
#include <player/player_vehicle.h>
#include <player/spirit_model.h>
#include <player/status_effects.h>
#include <rendering/camera.h>
#include <ui/hotbar.h>

// Spirit model and player magic components are defined in their
// respective headers (spirit_model.h, player_magic.h) which are included
// above to avoid duplication and keep single source of truth.

// Game state
typedef enum {
  IN_GAME_STATE_PLAYING,
  IN_GAME_STATE_CRAFTING,
  IN_GAME_STATE_INVENTORY,
  IN_GAME_STATE_MENU,
} InGameState;

// Forward declarations
typedef struct AudioSystem AudioSystem;
typedef struct Camera Camera;

// Forward declarations from combat.h to avoid circular dependency
// Need full definitions, so include combat.h after PlayerSystem forward
// declaration

// Eating state defined in player_food.h
#include "player_food.h"

// Player component
typedef struct PlayerComponent {
  EntityID entity_id;

  // Transform (now in TransformComponent)
  Vec3 velocity;
  Vec3 acceleration;
  f32 yaw;
  f32 pitch;

  // Physics
  RigidBody *physics_body;
  bool on_ground;
  bool is_flying;
  f32 ground_check_distance;

  // Movement
  f32 walk_speed;
  f32 sprint_speed;
  f32 run_speed; // Running speed (between walk and sprint)
  f32 fly_speed;
  f32 jump_force;
  f32 gravity;
  f32 air_control;

  // Movement state
  bool in_water;
  bool in_lava;
  bool on_ladder;
  bool on_ice;
  bool on_soul_sand;
  f32 friction_modifier; // 1.0 = normal, <1.0 = slippery, >1.0 = sticky

  // Camera
  Vec3 camera_offset;
  f32 camera_height;
  f32 base_fov; // Base field of view
  f32 fov;      // Current field of view (dynamic)
  f32 near_plane;
  f32 far_plane;
  f32 camera_shake_timer;
  f32 camera_shake_strength;
  Vec3 camera_smoothed_position;
  Vec3 camera_prev_position;

  // Inventory
  Inventory inventory;
  HotbarSystem hotbar;

  // Equipment (use void* to avoid circular dependency with combat.h)
  void *equipped_weapon;
  void *equipped_armor; // Pointer to array of 4 Armor structs

  // 3D model 'spirit' component placeholder
  SpiritModelComponent spirit_model;
  SpiritAnimState action_anim_state;
  f32 action_anim_timer;
  f32 action_anim_duration;

  // Combat state
  PlayerCombatState combat_state;

  // Magic system
  PlayerMagicComponent magic_component;

  // Vehicle system
  VehicleState vehicle_state;

  // Interaction
  f32 reach_distance;
  EntityID target_entity;
  Vec3 target_block;
  bool has_target;

  // Mining
  bool mining_active;
  Vec3 mining_block;
  BlockID mining_block_id;
  f32 mining_progress;
  f32 mining_target_time;

  // State
  bool is_sprinting;
  bool is_crouching;
  bool is_jumping;
  f32 stamina;
  f32 max_stamina;
  f32 jump_cooldown_timer;
  f32 jump_cooldown_duration;

  // Health & Status (Health is now in HealthComponent)
  f32 hunger;
  f32 max_hunger;
  f32 saturation; // Food saturation for health regen
  f32 max_saturation;
  f32 experience;
  u32 level;
  f32 experience_to_next;
  f32 experience_progress; // 0-1 normalized
  f32 experience_display;  // smoothed value for UI
  u32 skill_points;
  f32 level_up_timer;
  f32 oxygen;
  f32 max_oxygen;
  bool is_swimming;
  f32 fall_start_y;
  bool was_on_ground;

  // Water physics state (for currents/pressure)
  Vec3 water_current_velocity;
  f32 water_pressure;
  f32 water_depth; // Depth below water surface (positive when submerged)
  bool in_river;   // True when in flowing water (river biome)
  bool in_ocean;   // True when in deep ocean biome

  // Status effects
  StatusEffectManager status_effects;

  // Food/Eating state
  EatingState eating_state;

  // Vehicle
  EntityID vehicle;
  bool in_vehicle;

  // Input state
  f32 look_x_smooth;
  f32 look_y_smooth;
  f32 deadzone;
  bool invert_y;

  // Camera modes
  u32 camera_mode; // 0=first-person, 1=third-person, 2=top-down
  f32 camera_distance;
  f32 head_bob_timer;
  Vec3 prev_physics_position;
  bool has_prev_physics_state;

  // Health tracking
  f32 last_health;
} PlayerComponent;

typedef PlayerComponent Player;

// Player system
struct PlayerSystem {
  PlayerComponent *player;
  InputState *input;
  ControlSettings *controls;
  GameModeState *game_mode;
  PhysicsWorld *physics_world;
  struct World *ecs_world;
  ChunkManager *chunk_manager;
  BlockRegistry *block_registry;
  Camera *camera;
  CombatSystem *combat_system;
  struct AudioSystem *audio_system; // For playing sound effects
  f32 interpolation_alpha;
};
typedef struct PlayerSystem PlayerSystem;

// Initialize player system
void player_system_init(PlayerSystem *system, InputState *input,
                        ControlSettings *controls, GameModeState *game_mode,
                        PhysicsWorld *physics, struct World *ecs,
                        ChunkManager *chunks, BlockRegistry *blocks,
                        Camera *camera, CombatSystem *combat_system,
                        struct AudioSystem *audio_system);
void player_system_free(PlayerSystem *system);

// Create player
EntityID player_create(PlayerSystem *system, Vec3 position);

// Update player
void player_update(PlayerSystem *system, f32 delta_time);
void player_system_update(PlayerSystem *system, f32 delta_time,
                          ChunkManager *chunk_manager,
                          PhysicsWorld *physics_world,
                          BlockRegistry *block_registry);
void player_system_render(PlayerSystem *system, void *renderer);

// Player movement
void player_update_movement(PlayerSystem *system, f32 delta_time);
void player_update_physics(PlayerSystem *system, f32 delta_time);
void player_update_camera(PlayerSystem *system, f32 delta_time);
void player_update_interaction(PlayerSystem *system, f32 delta_time);
bool player_check_block_collision(PlayerSystem *system, Vec3 position,
                                  Vec3 size, Vec3 *out_normal);
void player_update_physics_with_blocks(PlayerSystem *system, f32 delta_time);

// Player actions
void player_jump(PlayerSystem *system);
void player_sprint(PlayerSystem *system, bool enable);
void player_crouch(PlayerSystem *system, bool enable);
void player_fly(PlayerSystem *system, bool enable);
void player_trigger_action_animation(Player *player, SpiritAnimState state,
                                     f32 duration);

// Block interaction
bool player_place_block(PlayerSystem *system, BlockID block, Vec3 position);
bool player_break_block(PlayerSystem *system, Vec3 position);
bool player_interact_block(PlayerSystem *system, Vec3 position);

// Inventory management
bool player_add_item(PlayerSystem *system, u32 item_id, u32 quantity);
bool player_remove_item(PlayerSystem *system, u32 item_id, u32 quantity);
u32 player_get_item_count(PlayerSystem *system, u32 item_id);
void player_select_slot(PlayerSystem *system, u32 slot);
u32 player_get_selected_item(PlayerSystem *system);

// Equipment (use void* to avoid circular dependency)
void player_equip_weapon(PlayerSystem *system, void *weapon);
void player_equip_armor(PlayerSystem *system, u32 slot, void *armor);

// Vehicle interaction
void player_enter_vehicle(PlayerSystem *system, EntityID vehicle);
void player_exit_vehicle(PlayerSystem *system);

// Get player position
Vec3 player_get_position(PlayerSystem *system);
Vec3 player_get_forward(PlayerSystem *system);
Vec3 player_get_right(PlayerSystem *system);

// Health & Status
void player_set_health(PlayerSystem *system, f32 health);
f32 player_get_health(PlayerSystem *system);
void player_damage(PlayerSystem *system, f32 damage);
void player_damage_detailed(PlayerSystem *system, f32 damage, DamageType type);
void player_heal(PlayerSystem *system, f32 amount);
void player_set_hunger(PlayerSystem *system, f32 hunger);
f32 player_get_hunger(PlayerSystem *system);
void player_add_experience(PlayerSystem *system, f32 xp);
f32 player_get_experience(PlayerSystem *system);

// Swimming
void player_enter_water(PlayerSystem *system);
void player_exit_water(PlayerSystem *system);
bool player_is_in_water(PlayerSystem *system);

// Fall damage
void player_check_fall_damage(PlayerSystem *system);

// Player damage functions
void player_apply_fall_damage(Player *player, struct World *ecs_world,
                              AudioSystem *audio_system, Camera *camera);
void player_apply_lava_damage(Player *player, struct World *ecs_world,
                              AudioSystem *audio_system, Camera *camera,
                              f32 delta_time);
void player_apply_fire_damage(Player *player, struct World *ecs_world,
                              AudioSystem *audio_system, Camera *camera,
                              f32 delta_time);
void player_apply_drowning_damage(Player *player, struct World *ecs_world,
                                  AudioSystem *audio_system, Camera *camera,
                                  f32 delta_time);
void player_apply_suffocation_damage(Player *player, struct World *ecs_world,
                                     ChunkManager *chunk_manager,
                                     BlockRegistry *block_registry,
                                     AudioSystem *audio_system, Camera *camera,
                                     f32 delta_time);
void player_update_damage_systems(f32 delta_time);
void player_update_fall_tracking(Player *player);

// Environment state updates
void player_update_environment_state(PlayerSystem *system);

// Camera modes
void player_set_camera_mode(PlayerSystem *system, u32 mode);
u32 player_get_camera_mode(PlayerSystem *system);

// Input smoothing
void player_set_deadzone(PlayerSystem *system, f32 deadzone);
void player_set_invert_y(PlayerSystem *system, bool invert);

// Respawn
void player_respawn(PlayerSystem *system, Vec3 position);

#endif // PLAYER_H
