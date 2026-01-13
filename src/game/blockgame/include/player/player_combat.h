// include/player/player_combat.h
//
// Purpose: Defines the public API and data structures for managing the player's
// combat interactions. This header encapsulates the `PlayerCombatState`,
// tracking various combat-related timers, cooldowns, and active states such as
// attacking, blocking, and dodging. It provides functions to initiate combat actions,
// update the player's combat status, and query their current combat capabilities.
//
// Public APIs:
// - `PlayerCombatState`: Structure to hold the current combat state of the player,
//   including cooldowns for attacks, blocks, and dodges, flags for active actions,
//   combo tracking, block strength, dodge parameters, and hit stun.
// - `CombatActionType`: Enumeration defining various combat actions a player can perform
//   (e.g., `COMBAT_ACTION_MELEE`, `COMBAT_ACTION_RANGED`, `COMBAT_ACTION_BLOCK`, `COMBAT_ACTION_DODGE`).
// - `player_combat_init`: Initializes the player's combat state with default values.
// - `player_attack`: Initiates an attack action for the player, potentially consuming resources
//   and triggering animations based on the `CombatActionType`.
// - `player_block`: Toggles the player's blocking state.
// - `player_dodge`: Triggers a dodge maneuver in a specified direction.
// - `player_combat_update`: Updates all combat-related timers and states each frame,
//   handling cooldowns, combo decay, and other time-dependent mechanics.
// - `player_can_attack`, `player_can_block`: Queries to determine if the player is currently
//   able to perform attack or block actions.
// - `player_is_invulnerable`: Checks if the player is currently in an invulnerable state (e.g., during a dodge).
//
// Ownership: The `PlayerCombatState` is typically a component within the player's overall
// system or ECS entity. It interacts with `PlayerSystem` and `PlayerComponent` (forward
// declared) but does not own them.
//
// Invariants:
// - `player_combat_init` must be called once at player creation or game start.
// - `player_combat_update` should be called once per frame with `delta_time` for proper time progression.
// - `attack_cooldown`, `block_cooldown`, `dodge_cooldown` manage the timing of player actions.
// - Combo mechanics rely on timely successive attacks.
//
#ifndef PLAYER_COMBAT_H
#define PLAYER_COMBAT_H


#include "../combat/combat.h" // This already defines WeaponType
#include "../game_common.h"
#include <math/vec3.h>

// Player combat state
typedef struct {
  f32 attack_cooldown;
  f32 block_cooldown;
  f32 damage_timer;
  f32 dodge_cooldown;
  f32 dodge_timer;
  f32 cooldown_reduction;
  bool is_blocking;
  bool is_attacking;
  bool is_dodging;
  u32 combo_count;
  u32 combo_score;
  f32 combo_timer;
  f32 block_strength;       // Shield blocking strength multiplier
  f32 perfect_block_window; // Perfect block timing window
  f32 dodge_distance;       // Distance traveled during dodge
  bool dodge_invulnerable;  // Invulnerability during dodge
  f32 hit_stun_timer;
  f32 last_damage_amount;
  WeaponType weapon_override;
  bool has_weapon_override;
  
  // Advanced combat features
  CombatStance stance;
  f32 damage_multiplier;
  f32 defense_multiplier;
  f32 speed_multiplier;
} PlayerCombatState;

// Combat buff system
#define MAX_COMBAT_BUFFS 8

typedef enum {
  COMBAT_BUFF_NONE = 0,
  COMBAT_BUFF_DAMAGE_BOOST,
  COMBAT_BUFF_DEFENSE_BOOST,
  COMBAT_BUFF_SPEED_BOOST,
  COMBAT_BUFF_STAMINA_REGEN,
  COMBAT_BUFF_HEALTH_REGEN,
  COMBAT_BUFF_CRITICAL_CHANCE,
  COMBAT_BUFF_COOLDOWN_REDUCTION
} CombatBuffType;

typedef struct {
  CombatBuffType type;
  f32 duration;
  f32 magnitude;
  f32 time_remaining;
} CombatBuff;

typedef struct {
  u32 attacks;
  u32 hits;
  u32 crits;
  u32 blocks;
  u32 dodges;
  u32 counter_attacks;
  u32 finishers;
  f32 damage_dealt;
  f32 damage_taken;
} PlayerCombatStats;

typedef struct {
  void (*on_attack)(WeaponType weapon, bool critical, void *user_data);
  void (*on_block)(bool starting, bool perfect, void *user_data);
  void (*on_dodge)(Vec3 direction, void *user_data);
  void (*on_hit)(Vec3 position, f32 damage, void *user_data);
  void (*on_sound)(const char *sound_id, f32 volume, void *user_data);
  void *user_data;
} PlayerCombatCallbacks;

// Forward declarations
struct PlayerSystem;
struct PlayerComponent;

// Combat action types
typedef enum {
  COMBAT_ACTION_MELEE = 0,
  COMBAT_ACTION_RANGED,
  COMBAT_ACTION_MAGIC,
  COMBAT_ACTION_BLOCK,
  COMBAT_ACTION_DODGE,
  COMBAT_ACTION_COUNT
} CombatActionType;

// Combat stances
typedef enum {
  COMBAT_STANCE_BALANCED = 0,
  COMBAT_STANCE_AGGRESSIVE,
  COMBAT_STANCE_DEFENSIVE,
  COMBAT_STANCE_COUNT
} CombatStance;

// Initialize player combat system
void player_combat_init(PlayerCombatState *combat);

// Handle combat actions
void player_attack(struct PlayerSystem *system, CombatActionType type);
void player_block(struct PlayerSystem *system, bool enable);
void player_dodge(struct PlayerSystem *system, Vec3 direction);

// Update combat state
void player_combat_update(PlayerCombatState *combat, f32 delta_time);

// Combat queries
bool player_can_attack(PlayerCombatState *combat, const struct PlayerComponent *player);
bool player_can_block(PlayerCombatState *combat, const struct PlayerComponent *player);
bool player_is_invulnerable(PlayerCombatState *combat);

void player_combat_set_weapon_override(PlayerCombatState *combat, WeaponType weapon);
void player_combat_clear_weapon_override(PlayerCombatState *combat);
const PlayerCombatStats *player_combat_get_stats(void);
void player_combat_reset_stats(void);
void player_combat_set_callbacks(const PlayerCombatCallbacks *callbacks);

// Advanced combat mechanics
void player_counter_attack(struct PlayerSystem *system);
void player_perform_finisher(struct PlayerSystem *system);
bool player_can_perform_finisher(PlayerCombatState *combat);

// Combat stance system
void player_set_combat_stance(PlayerCombatState *combat, CombatStance stance);
CombatStance player_get_combat_stance(PlayerCombatState *combat);

// Combat buff system
void player_apply_combat_buff(PlayerCombatState *combat, CombatBuffType type, f32 duration, f32 magnitude);
void player_update_combat_buffs(PlayerCombatState *combat, f32 delta_time);
f32 player_get_combat_buff_multiplier(PlayerCombatState *combat, CombatBuffType type);

#endif // PLAYER_COMBAT_H
