// include/ecs/components/npc.h
//
// Purpose: Defines the `NPCComponent` structure, a fundamental component within
// the Entity-Component-System (ECS) architecture, specifically for Non-Player
// Characters (NPCs). This component encapsulates an NPC's type, current state,
// target entity, pathfinding data, various behavior-related timers, and a
// bitfield for managing dynamic behavior flags.
//
// Public APIs:
// - `NPC_FLAG_*` macros: Bitfield flags defining various behavioral states or
// attributes for NPCs
//   (e.g., `CAN_BREED`, `IS_BABY`, `IN_LOVE`, `ANGRY`).
// - `NPCComponent`: A structure containing:
//   - `type`: The specific type of NPC (from `npc_types.h`).
//   - `state`: The current behavioral state of the NPC.
//   - `target`: The EntityID of the NPC's current target.
//   - `path`: An array of `Vec3` points defining the NPC's current path.
//   - `path_length`, `current_path_index`: Control path traversal.
//   - `behavior_timer`, `breed_cooldown`, `panic_timer`, `growth_timer`:
//   Various timers
//     to manage complex NPC behaviors.
//   - `flee_target`: The EntityID of an entity the NPC is fleeing from.
//   - `behavior_flags`: A bitfield for efficient storage and checking of
//   multiple boolean behaviors.
//
// Ownership: `NPCComponent` instances are typically owned by the ECS framework
// within its component arrays. The component itself is a data structure, with
// complex behaviors being driven by ECS systems that process entities with this
// component.
//
// Invariants:
// - `type` should be a valid `NPCType` defined in `npc_types.h`.
// - `target` and `flee_target` should refer to valid `EntityID`s or
// `NULL_ENTITY` if not set.
// - Pathfinding data (`path`, `path_length`, `current_path_index`) should be
// consistent.
// - Timers (`behavior_timer`, etc.) should be updated by relevant ECS systems.
// - `behavior_flags` are managed via bitwise operations.
//
#ifndef NPC_COMPONENT_H
#define NPC_COMPONENT_H

#include "include/common.h"
#include "include/math/vec3.h"
#include "include/ecs/ecs.h"
#include <npc/npc_types.h>

// Behavior flags for NPCs
#define NPC_FLAG_CAN_BREED (1 << 0)
#define NPC_FLAG_IS_BABY (1 << 1)
#define NPC_FLAG_IN_LOVE (1 << 2)
#define NPC_FLAG_ANGRY (1 << 3)
#define NPC_FLAG_TRADING (1 << 4)
#define NPC_FLAG_DIALOGUE (1 << 5)
#define NPC_BEHAVIOR_SEEK_SHELTER                                              \
  (1 << 6)                           // Weather: seeking shelter from storms
#define NPC_BEHAVIOR_SLOWED (1 << 7) // Weather: movement slowed by weather
#define NPC_BEHAVIOR_ACTIVE (1 << 8) // Weather: more active in clear weather
#define NPC_FLAG_AT_WORK (1 << 9)    // Job: currently working
#define NPC_FLAG_HAS_HOME (1 << 10)  // Housing: has assigned house

typedef struct {
  NPCType type;
  NPCState state;
  EntityID target;

  Vec3 path[32];
  u32 path_length;
  u32 current_path_index;

  // Behavior timers and state
  f32 behavior_timer;   // General purpose timer for AI behaviors
  f32 breed_cooldown;   // Time until can breed again
  f32 panic_timer;      // Time remaining in panic/flee state
  EntityID flee_target; // Entity to flee from
  u32 behavior_flags;   // Bitfield for AI states (breed, baby, etc.)
  f32 growth_timer;     // For baby animals growing up
  NPCMood mood;
  f32 schedule_timer;
  u8 schedule_index;
  EntityID last_attacker;
  f32 time_since_last_attacked;
  i16 reputation;
  EntityID relations_entities[8];
  i16 relations_values[8];
  u8 relations_count;

  // Jobs and daily life (Milestone 3)
  NPCJob job;
  NPCTask current_task;
  NPCSchedulePhase schedule_phase;
  f32 task_timer;
  EntityID home;
  EntityID workplace;
  f32 hunger;
  f32 energy;
  f32 social_need;
} NPCComponent;

#endif // NPC_COMPONENT_H
