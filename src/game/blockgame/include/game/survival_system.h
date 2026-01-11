// include/game/survival_system.h
//
// Purpose: Defines the public API and data structures for managing survival mechanics
// within the game. This header provides structures for `SurvivalSettings` (difficulty,
// damage scaling, hunger drain, mob spawn rates) and `SurvivalManager` (player system,
// current settings, day-night cycle, weather). It declares functions for initializing,
// updating, and interacting with these core survival elements.
//
// Public APIs:
// - `DifficultyLevel`: Enumeration defining various game difficulty levels.
// - `SurvivalSettings`: Structure encapsulating parameters that vary by difficulty,
//   such as damage scaling, hunger drain rate, mob spawn rate, PvP allowance, and mob griefing.
// - `SurvivalManager`: The main structure overseeing all survival mechanics, holding
//   a reference to the `PlayerSystem`, the current `SurvivalSettings`, and managing
//   the day-night cycle and weather conditions.
// - `survival_manager_init`: Initializes the survival manager, linking it to the player system.
// - `survival_manager_free`: Frees resources associated with the survival manager.
// - `survival_manager_update`: Updates all survival-related mechanics each frame,
//   including hunger depletion, day-night cycle progression, and environmental effects.
// - `survival_apply_difficulty`: Applies a new difficulty level, updating relevant settings.
// - `survival_damage_player`: Applies damage to the player, respecting game rules.
// - `survival_hunger_deplete`, `survival_restore_hunger`: Functions to manage player hunger levels.
// - `survival_get_day_time`, `survival_set_day_time`: Functions to query and set the current in-game time.
// - `survival_is_day`, `survival_is_night`: Convenience functions to check if it's currently day or night.
//
// Ownership: A `SurvivalManager` instance owns its `SurvivalSettings`. It interacts
// with a `PlayerSystem` but does not own it.
//
// Invariants:
// - A `SurvivalManager` must be initialized with `survival_manager_init` before use.
// - `player_system` must be a valid, initialized `PlayerSystem` instance.
// - `delta_time` should be consistently passed to `survival_manager_update` for accurate time progression.
// - Difficulty settings should be applied appropriately to influence gameplay balance.
//
#ifndef SURVIVAL_SYSTEM_H
#define SURVIVAL_SYSTEM_H


#include "../game_common.h"
#include "../player/player.h"
#include "mode.h"  // For Difficulty enum

// Use Difficulty from mode.h instead of defining DifficultyLevel
// This avoids duplicate enum values

typedef struct {
    Difficulty difficulty;
    f32 damage_scale;
    f32 hunger_drain_rate;
    f32 mob_spawn_rate;
    bool allow_pvp;
    bool mob_griefing_enabled;
    u32 play_time_seconds;
    u32 day_time;
    bool challenge_mode;
    f32 scarcity_factor;
} SurvivalSettings;

typedef struct {
    u32 deaths;
    u32 respawns;
    u32 milestones_completed;
    f32 time_survived_seconds;
    f32 longest_survival_seconds;
    u32 hunger_warnings;
    u32 low_health_warnings;
    u32 disease_cases;
} SurvivalStats;

typedef struct {
    bool hunger_tip_shown;
    bool weather_tip_shown;
    bool temperature_tip_shown;
    bool disease_tip_shown;
    bool respawn_tip_shown;
    bool challenge_tip_shown;
} SurvivalTutorialFlags;

typedef struct {
    PlayerSystem *player_system;
    SurvivalSettings settings;
    f32 day_night_cycle_time;
    bool is_raining;
    f32 hunger_timer;
    u32 day_count;
    f32 difficulty_scale;
    f32 base_damage_scale;
    f32 base_hunger_drain_rate;
    f32 base_mob_spawn_rate;
    f32 last_health;
    bool respawn_pending;
    f32 respawn_timer;
    f32 temperature;
    f32 infection_timer;
    f32 infection_severity;
    bool infected;
    SurvivalStats stats;
    SurvivalTutorialFlags tutorial;
} SurvivalManager;

void survival_manager_init(SurvivalManager *mgr, PlayerSystem *player_system);
void survival_manager_free(SurvivalManager *mgr);

void survival_manager_update(SurvivalManager *mgr, f32 delta_time);
void survival_apply_difficulty(SurvivalManager *mgr, Difficulty difficulty);

void survival_damage_player(SurvivalManager *mgr, f32 damage);
void survival_hunger_deplete(SurvivalManager *mgr, f32 amount);
void survival_restore_hunger(SurvivalManager *mgr, f32 amount);

u32 survival_get_day_time(SurvivalManager *mgr);
void survival_set_day_time(SurvivalManager *mgr, u32 time);

bool survival_is_day(SurvivalManager *mgr);
bool survival_is_night(SurvivalManager *mgr);

#endif
