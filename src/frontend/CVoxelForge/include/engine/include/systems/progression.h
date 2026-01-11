// include/systems/progression.h
//
// Purpose: Defines the public API and data structures for the game's player
// progression system. This header is responsible for tracking a player's level,
// experience, and various gameplay statistics (e.g., blocks broken, kills, deaths).
// It also includes a comprehensive achievement system for managing, unlocking,
// and querying player achievements.
//
// Public APIs:
// - `PlayerProgression`: Structure encapsulating all player progression data,
//   including `level`, `experience`, `experience_for_next_level`, and various
//   gameplay statistics like `total_blocks_broken`, `total_kills`, and `total_miles_walked`.
// - `Achievement`: Structure defining a single achievement with its `id`, `name`,
//   `description`, `unlocked` status, and progress tracking fields.
// - `AchievementSystem`: Structure managing a collection of `Achievement` objects.
// - `progression_create`, `progression_free`: Lifecycle functions for `PlayerProgression`.
// - `progression_add_experience`: Adds experience points to the player and handles leveling up.
// - `progression_level_up`: Manually advances the player to the next level.
// - `progression_get_level`, `progression_get_experience_percent`: Query functions for player progression.
// - `achievement_system_create`, `achievement_system_free`: Lifecycle functions for `AchievementSystem`.
// - `achievement_system_add_achievement`: Adds a new achievement definition to the system.
// - `achievement_system_unlock`: Marks an achievement as unlocked.
// - `achievement_system_is_unlocked`: Checks if a specific achievement has been unlocked.
// - `achievement_system_get_count`: Returns the total number of achievements registered.
//
// Ownership: `PlayerProgression` and `AchievementSystem` instances own their internal data.
//
// Invariants:
// - `PlayerProgression` and `AchievementSystem` must be initialized before use.
// - `experience_for_next_level` should be dynamically calculated based on the current level.
// - Achievement IDs (`id`) must be unique strings.
// - The system assumes a mechanism for saving and loading progression and achievement states
//   (handled by the save system, likely).
//
#ifndef PROGRESSION_H
#define PROGRESSION_H


#include <common.h>

// Phase 18: Gameplay/Progression System
typedef struct {
    u32 level;
    u32 experience;
    u32 experience_for_next_level;
    u32 total_blocks_broken;
    u32 total_blocks_placed;
    u32 total_kills;
    u32 total_deaths;
    u32 total_miles_walked;
    u32 total_blocks_mined;
} PlayerProgression;

typedef struct {
    const char *id;
    const char *name;
    const char *description;
    bool unlocked;
    u32 progress;
    u32 max_progress;
} Achievement;

typedef struct {
    Achievement *achievements;
    u32 achievement_count;
    u32 achievement_capacity;
} AchievementSystem;

PlayerProgression *progression_create(void);
void progression_free(PlayerProgression *prog);
void progression_add_experience(PlayerProgression *prog, u32 xp);
void progression_level_up(PlayerProgression *prog);
u32 progression_get_level(PlayerProgression *prog);
f32 progression_get_experience_percent(PlayerProgression *prog);

AchievementSystem *achievement_system_create(u32 initial_capacity);
void achievement_system_free(AchievementSystem *sys);
void achievement_system_add_achievement(AchievementSystem *sys, const char *id, const char *name, const char *description);
void achievement_system_unlock(AchievementSystem *sys, const char *id);
bool achievement_system_is_unlocked(AchievementSystem *sys, const char *id);
u32 achievement_system_get_count(AchievementSystem *sys);

#endif
