// include/npc/npc_types.h
//
// Purpose: Defines enumerations for various NPC types, behavioral states, and
// high-level behavior categories. It also provides the `NPCStats` structure to
// encapsulate base attributes for each NPC type, such as health, damage, speed,
// and detection ranges. This header serves as a central definition point for
// fundamental NPC characteristics and is used throughout the NPC system.
//
// Public APIs:
// - `NPCType`: Enumeration listing all distinct types of Non-Player Characters
//   (e.g., `NPC_TYPE_VILLAGER`, `NPC_TYPE_ZOMBIE`, `NPC_TYPE_COW`).
// - `NPCState`: Enumeration defining various states an NPC can be in during its
//   lifecycle or behavior (e.g., `NPC_STATE_IDLE`, `NPC_STATE_ATTACKING`, `NPC_STATE_FLEEING`).
// - `NPCBehavior`: Enumeration categorizing NPCs by their general behavior pattern
//   (e.g., `NPC_BEHAVIOR_PASSIVE`, `NPC_BEHAVIOR_HOSTILE`, `NPC_BEHAVIOR_NEUTRAL`).
// - `NPCStats`: Structure holding core statistics for an NPC type, including
//   `max_health`, `damage`, `move_speed`, `attack_range`, `detection_range`,
//   `flee_range`, and its `behavior` category.
// - `npc_get_stats`: An inline function that returns the predefined `NPCStats`
//   for a given `NPCType`.
//
// Ownership: This file defines constant enumerations and a lookup function for
// static data. It does not "own" any runtime-modifiable data or allocated memory.
//
// Invariants:
// - Each `NPCType` and `NPCState` value must be unique.
// - `NPC_TYPE_COUNT`, `NPC_STATE_COUNT`, `NPC_BEHAVIOR_COUNT` must accurately
//   reflect the number of entries in their respective enumerations.
// - The `npc_get_stats` function must provide a complete and consistent `NPCStats`
//   for every defined `NPCType`.
//
#ifndef NPC_TYPES_H
#define NPC_TYPES_H


#include "../game_common.h"

// NPC types
typedef enum {
  NPC_TYPE_VILLAGER,
  NPC_TYPE_ZOMBIE,
  NPC_TYPE_SKELETON,
  NPC_TYPE_CREEPER,
  NPC_TYPE_COW,
  NPC_TYPE_PIG,
  NPC_TYPE_CHICKEN,
  NPC_TYPE_COUNT
} NPCType;

// NPC states
typedef enum {
  NPC_STATE_IDLE,
  NPC_STATE_WANDERING,
  NPC_STATE_CHASING,
  NPC_STATE_ATTACKING,
  NPC_STATE_FLEEING,
  NPC_STATE_BREEDING,
  NPC_STATE_COUNT
} NPCState;

// NPC behavior categories
typedef enum {
  NPC_BEHAVIOR_PASSIVE, // Animals like cows, pigs, chickens
  NPC_BEHAVIOR_HOSTILE, // Zombies, skeletons, creepers
  NPC_BEHAVIOR_NEUTRAL, // Villagers, endermen (don't attack unless provoked)
  NPC_BEHAVIOR_COUNT
} NPCBehavior;

typedef enum {
  NPC_MOOD_NEUTRAL,
  NPC_MOOD_HAPPY,
  NPC_MOOD_ANGRY,
  NPC_MOOD_SCARED,
  NPC_MOOD_TIRED,
  NPC_MOOD_HUNGRY
} NPCMood;

// NPC stats for different types
typedef struct {
  f32 max_health;
  f32 damage;
  f32 move_speed;
  f32 attack_range;
  f32 detection_range; // How far can see player/targets
  f32 flee_range;      // For passive mobs - when to flee
  NPCBehavior behavior;
} NPCStats;

// Get stats for NPC type
static inline NPCStats npc_get_stats(NPCType type) {
  switch (type) {
  case NPC_TYPE_VILLAGER:
    return (NPCStats){.max_health = 20.0f,
                      .damage = 0.0f,
                      .move_speed = 1.5f,
                      .attack_range = 0.0f,
                      .detection_range = 10.0f,
                      .flee_range = 8.0f,
                      .behavior = NPC_BEHAVIOR_NEUTRAL};
  case NPC_TYPE_ZOMBIE:
    return (NPCStats){.max_health = 20.0f,
                      .damage = 3.0f,
                      .move_speed = 2.3f,
                      .attack_range = 1.5f,
                      .detection_range = 16.0f,
                      .flee_range = 0.0f,
                      .behavior = NPC_BEHAVIOR_HOSTILE};
  case NPC_TYPE_SKELETON:
    return (NPCStats){.max_health = 20.0f,
                      .damage = 2.5f,
                      .move_speed = 2.5f,
                      .attack_range = 8.0f,
                      .detection_range = 16.0f,
                      .flee_range = 0.0f,
                      .behavior = NPC_BEHAVIOR_HOSTILE};
  case NPC_TYPE_CREEPER:
    return (NPCStats){.max_health = 20.0f,
                      .damage = 25.0f,
                      .move_speed = 2.0f,
                      .attack_range = 3.0f,
                      .detection_range = 16.0f,
                      .flee_range = 0.0f,
                      .behavior = NPC_BEHAVIOR_HOSTILE};
  case NPC_TYPE_COW:
    return (NPCStats){.max_health = 10.0f,
                      .damage = 0.0f,
                      .move_speed = 1.0f,
                      .attack_range = 0.0f,
                      .detection_range = 8.0f,
                      .flee_range = 6.0f,
                      .behavior = NPC_BEHAVIOR_PASSIVE};
  case NPC_TYPE_PIG:
    return (NPCStats){.max_health = 10.0f,
                      .damage = 0.0f,
                      .move_speed = 1.0f,
                      .attack_range = 0.0f,
                      .detection_range = 8.0f,
                      .flee_range = 6.0f,
                      .behavior = NPC_BEHAVIOR_PASSIVE};
  case NPC_TYPE_CHICKEN:
    return (NPCStats){.max_health = 4.0f,
                      .damage = 0.0f,
                      .move_speed = 1.0f,
                      .attack_range = 0.0f,
                      .detection_range = 8.0f,
                      .flee_range = 6.0f,
                      .behavior = NPC_BEHAVIOR_PASSIVE};
  default:
    return (NPCStats){.max_health = 10.0f,
                      .damage = 0.0f,
                      .move_speed = 1.0f,
                      .attack_range = 0.0f,
                      .detection_range = 10.0f,
                      .flee_range = 0.0f,
                      .behavior = NPC_BEHAVIOR_NEUTRAL};
  }
}

typedef enum {
  NPC_JOB_NONE,
  NPC_JOB_FARMER,
  NPC_JOB_BLACKSMITH,
  NPC_JOB_LIBRARIAN,
  NPC_JOB_PRIEST,
  NPC_JOB_BUTCHER,
  NPC_JOB_FISHERMAN,
  NPC_JOB_GUARD,
  NPC_JOB_MERCHANT,
  NPC_JOB_MINER,
  NPC_JOB_LUMBERJACK,
  NPC_JOB_BAKER,
  NPC_JOB_ALCHEMIST,
  NPC_JOB_CARTOGRAPHER,
  NPC_JOB_SHEPHERD,
  NPC_JOB_LEATHERWORKER,
  NPC_JOB_MASON,
  NPC_JOB_ARCHITECT,
  NPC_JOB_TAILOR,
  NPC_JOB_COUNT
} NPCJob;

typedef enum {
  NPC_TASK_NONE,
  NPC_TASK_WORK,
  NPC_TASK_EAT,
  NPC_TASK_SLEEP,
  NPC_TASK_LEISURE,
  NPC_TASK_TRAVEL,
  NPC_TASK_SOCIALIZE,
  NPC_TASK_SHOP,
  NPC_TASK_WORSHIP,
  NPC_TASK_COUNT
} NPCTask;

typedef enum {
  NPC_SCHEDULE_WORK,
  NPC_SCHEDULE_EVENING,
  NPC_SCHEDULE_NIGHT,
  NPC_SCHEDULE_MORNING,
  NPC_SCHEDULE_COUNT
} NPCSchedulePhase;

typedef enum {
  NPC_FACTION_NEUTRAL,
  NPC_FACTION_VILLAGER,
  NPC_FACTION_MONSTER,
  NPC_FACTION_ANIMAL
} NPCFaction;

static inline NPCFaction npc_get_faction(NPCType type) {
  switch (type) {
  case NPC_TYPE_VILLAGER:
    return NPC_FACTION_VILLAGER;
  case NPC_TYPE_ZOMBIE:
  case NPC_TYPE_SKELETON:
  case NPC_TYPE_CREEPER:
    return NPC_FACTION_MONSTER;
  case NPC_TYPE_COW:
  case NPC_TYPE_PIG:
  case NPC_TYPE_CHICKEN:
    return NPC_FACTION_ANIMAL;
  default:
    return NPC_FACTION_NEUTRAL;
  }
}

#endif // NPC_TYPES_H
