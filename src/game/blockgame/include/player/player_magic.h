// include/player/player_magic.h
//
// Purpose: Defines the public API and data structures for the player's magic
// system. This header provides the framework for spell casting, mana
// management, and the overall magical capabilities of the player. It includes
// spell definitions, tracking of spell states (cooldowns, cast times,
// channeling), and functions to initiate, update, and query magical actions.
//
// Public APIs:
// - `SpellType`: Enumeration defining various magical spells (e.g.,
// `SPELL_FIREBALL`, `SPELL_HEAL`, `SPELL_TELEPORT`).
// - `SpellState`: Structure to hold the current state of an individual spell,
//   including its type, cooldown, cast time, mana cost, channeling status,
//   and potential target information.
// - `PlayerMagicComponent`: The main component for player magic, tracking
//   `max_mana`, `current_mana`, `mana_regen_rate`, the state of all learned
//   `spells`, and the count of active spells.
// - `player_magic_init`: Initializes the player's magic component with default
// values.
// - `player_cast_spell`: Initiates the casting of a specific spell towards a
// target.
// - `player_channel_spell`: Continues channeling an ongoing spell over time.
// - `player_cancel_spell`: Aborts the current spell casting or channeling.
// - `player_magic_update`: Updates all magic-related timers, mana regeneration,
//   and spell effects each frame.
// - `player_can_cast_spell`: Checks if the player meets the requirements (mana,
// cooldown)
//   to cast a given spell.
// - `player_has_mana`: Queries if the player has sufficient mana for an action.
//
// Ownership: The `PlayerMagicComponent` instance manages its internal spell
// states. It interacts with the `PlayerSystem` (forward declared) for
// contextual information.
//
// Invariants:
// - A `PlayerMagicComponent` must be initialized with `player_magic_init`
// before use.
// - `player_magic_update` should be called once per frame with `delta_time` for
// proper
//   mana regeneration and spell progression.
// - Spell cooldowns and mana costs are managed internally by the system.
// - `target_position` and `target_entity` are used for targeted spells.
//
#ifndef PLAYER_MAGIC_H
#define PLAYER_MAGIC_H

#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>

// Forward declare PlayerSystem to avoid including full player header here
typedef struct PlayerSystem PlayerSystem;

// Magic spell types
typedef enum {
  SPELL_FIREBALL = 0,
  SPELL_HEAL,
  SPELL_TELEPORT,
  SPELL_SHIELD,
  SPELL_LIGHTNING,
  SPELL_FREEZE,
  SPELL_INVISIBILITY,
  SPELL_FLIGHT,
  SPELL_COUNT
} SpellType;

// Spell state
typedef struct {
  SpellType type;
  u32 level;
  f32 cooldown;
  f32 cast_time;
  f32 mana_cost;
  bool is_channeling;
  f32 channel_time;
  Vec3 target_position;
  EntityID target_entity;
} SpellState;

typedef struct {
    u32 total_spells_cast;
    f32 total_mana_consumed;
    u32 spell_counts[SPELL_COUNT];
} SpellStats;

// Player magic component
typedef struct {
  f32 max_mana;
  f32 current_mana;
  f32 mana_regen_rate;
  SpellState spells[SPELL_COUNT];
  u32 active_spell_count;
  bool casting_disabled;
  u32 spell_points;
  SpellStats stats;
} PlayerMagicComponent;

// Magic initialization
void player_magic_init(PlayerMagicComponent *magic);

// Spell casting
bool player_cast_spell(PlayerSystem *system, SpellType spell, Vec3 target);
bool player_channel_spell(PlayerSystem *system, SpellType spell,
                          f32 delta_time);
void player_cancel_spell(PlayerSystem *system);

// Magic updates
void player_magic_update(PlayerSystem *system, f32 delta_time);

// Magic queries
bool player_can_cast_spell(PlayerMagicComponent *magic, SpellType spell);
bool player_has_mana(PlayerMagicComponent *magic, f32 amount);

#endif // PLAYER_MAGIC_H
