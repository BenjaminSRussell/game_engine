// include/mobs/mob_system.h
//
// Purpose: Defines the public API and data structures for a comprehensive mob
// management system. This header provides enumerations for various `MobType`s
// and `MobState`s, a detailed `Mob` structure to encapsulate individual mob
// data (health, position, velocity, AI parameters), and the `MobManager` to
// centrally control the lifecycle and behavior of all active mobs within the
// game world. It offers functions for spawning, despawning, updating, and
// interacting with mobs.
//
// Public APIs:
// - `MobType`: Enumeration categorizing different types of mobs (e.g., Zombie,
// Skeleton, Cow).
// - `MobState`: Enumeration defining the current behavioral state of a mob
// (e.g., Idle, Attacking, Fleeing).
// - `Mob`: Structure representing an individual mob, containing its
// `entity_id`, `type`, `state`,
//   physical properties (`position`, `velocity`), combat stats (`health`,
//   `attack_cooldown`, `attack_range`), AI parameters (`speed`,
//   `detection_range`, `wander_timer`, `target_position`, `target_entity`), and
//   lifecycle information (`spawn_time`, `alive`).
// - `MobManager`: Structure managing a dynamic collection of `Mob` objects,
// including their count and capacity.
// - `mob_manager_init`: Initializes the `MobManager` with a specified capacity.
// - `mob_manager_free`: Frees all resources held by the `MobManager`.
// - `mob_spawn`: Creates and initializes a new mob of a given type at a
// specified position.
// - `mob_despawn`: Removes a mob from the game world based on its entity ID.
// - `mob_update`: Updates the state and behavior of all active mobs each frame.
// - `mob_get`: Retrieves a `Mob` instance by its `entity_id`.
// - `mob_damage`: Applies damage to a specific mob.
// - `mob_set_target`, `mob_set_target_entity`: Functions for setting a mob's
// movement or attack target.
// - `mob_can_see_target`, `mob_in_range`: Utility functions for AI
// decision-making related to targets.
//
// Ownership: The `MobManager` owns the collection of `Mob` objects it manages.
// Each `Mob` itself is designed to be part of an ECS system, with its
// `entity_id` linking it to broader entity components.
//
// Invariants:
// - A `MobManager` must be initialized before spawning or managing mobs.
// - `entity_id` within a `Mob` struct should be a valid ECS entity ID.
// - `delta_time` should be consistently passed to `mob_update` for accurate
// time-based behaviors.
// - `target_entity` (if set) should refer to a valid `EntityID`.
//
#ifndef MOB_SYSTEM_H
#define MOB_SYSTEM_H

#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>

typedef enum {
  MOB_TYPE_ZOMBIE,
  MOB_TYPE_SKELETON,
  MOB_TYPE_SPIDER,
  MOB_TYPE_CREEPER,
  MOB_TYPE_COW,
  MOB_TYPE_PIG,
  MOB_TYPE_SHEEP,
  MOB_TYPE_CHICKEN,
  MOB_TYPE_ENDERMAN,
  MOB_TYPE_BLAZE
} MobType;

typedef enum {
  MOB_STATE_IDLE,
  MOB_STATE_WANDERING,
  MOB_STATE_ATTACKING,
  MOB_STATE_FLEEING,
  MOB_STATE_DEAD
} MobState;

typedef struct {
  EntityID entity_id;
  MobType type;
  MobState state;
  Vec3 position;
  Vec3 velocity;
  f32 health;
  f32 max_health;
  f32 attack_cooldown;
  f32 attack_range;
  f32 speed;
  f32 detection_range;
  f32 wander_timer;
  Vec3 target_position;
  EntityID target_entity;
  u32 spawn_time;
  bool alive;
} Mob;

typedef struct {
  u32 spawned_total;
  u32 despawned_total;
  u32 killed_total;
  u32 attacks_total;
  f32 damage_dealt_total;
} MobStats;

typedef struct {
  Mob *mobs;
  u32 count;
  u32 capacity;
  MobStats stats;
} MobManager;

void mob_manager_init(MobManager *manager, u32 capacity);
void mob_manager_free(MobManager *manager);

EntityID mob_spawn(MobManager *manager, MobType type, Vec3 position);
void mob_despawn(MobManager *manager, EntityID entity_id);
void mob_update(MobManager *manager, f32 delta_time);

Mob *mob_get(MobManager *manager, EntityID entity_id);
void mob_damage(MobManager *manager, EntityID entity_id, f32 damage);

void mob_set_target(Mob *mob, Vec3 target);
void mob_set_target_entity(Mob *mob, EntityID target);

bool mob_can_see_target(Mob *mob, Vec3 target);
bool mob_in_range(Mob *mob, Vec3 target);

const MobStats *mob_get_stats(const MobManager *manager);
void mob_reset_stats(MobManager *manager);

#endif
