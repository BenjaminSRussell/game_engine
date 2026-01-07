// NPC combat system API.
#ifndef NPC_COMBAT_H
#define NPC_COMBAT_H

#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>
#include "npc_types.h"

// Forward declarations
struct NPCSystem;
struct PhysicsWorld;

// Damage types
#include "../combat/combat.h"

// Damage information
typedef struct {
  f32 amount;
  DamageType type;
  EntityID source;
  Vec3 knockback_direction;
  f32 knockback_force;
} DamageInfo;

// Loot drop item
typedef struct {
  u32 item_id;
  u32 count;
  f32 drop_chance;
} LootDrop;

// Loot table for NPC type
typedef struct {
  LootDrop drops[8];
  u32 drop_count;
  u32 min_exp;
  u32 max_exp;
} LootTable;

// Combat functions
bool npc_take_damage(struct NPCSystem *system, EntityID entity,
                     DamageInfo *damage);
void npc_apply_knockback(struct NPCSystem *system, EntityID entity,
                         Vec3 direction, f32 force);
void npc_on_death(struct NPCSystem *system, EntityID entity, Vec3 position,
                  NPCType type);
void npc_spawn_loot(struct NPCSystem *system, Vec3 position,
                    const LootTable *loot_table);
void npc_spawn_experience(struct NPCSystem *system, Vec3 position, u32 amount);

// Attack functions
bool npc_melee_attack(struct NPCSystem *system, EntityID attacker,
                      EntityID target);
f32 npc_calculate_damage(NPCType attacker_type, DamageType damage_type);

// Get loot table for NPC type
const LootTable *npc_get_loot_table(NPCType type);

#endif // NPC_COMBAT_H
