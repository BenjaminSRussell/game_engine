// include/ai/npc_combat.h
//
// Purpose: Defines the NPC combat system that handles combat interactions
// between NPCs and other entities. This system integrates with the main
// combat system but provides NPC-specific behaviors like AI-driven combat,
// weapon usage, and tactical decision making.
//
// Public APIs:
// - `NPCCombatSystem`: Main combat system for NPCs
// - `npc_combat_init`: Initialize NPC combat system
// - `npc_combat_update`: Update all active combat
// - `npc_combat_attack`: Perform NPC attack
// - `npc_combat_set_target`: Set combat target
// - `npc_combat_get_threat_level`: Get current threat assessment
//
// Ownership: The NPCCombatSystem manages combat state for all NPCs
// but does not own the entities themselves.
//
// Invariants:
// - Combat system must be initialized before combat operations
// - All combat participants must have required components
// - Combat state is updated each frame for active combatants
//
#ifndef AI_NPC_COMBAT_H
#define AI_NPC_COMBAT_H

#include <ai/npc.h>
#include <ai/npc_types.h>
#include <ecs/ecs.h>
#include <math/vec3.h>
#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct World;
struct PhysicsWorld;

// Combat behavior types for NPCs
typedef enum {
  NPC_COMBAT_MELEE,          // Close-quarters combat
  NPC_COMBAT_RANGED,         // Distance attacks
  NPC_COMBAT_EXPLOSIVE,      // Self-destruct attacks (creeper)
  NPC_COMBAT_MAGIC,          // Spell-based attacks
  NPC_COMBAT_DEFENSIVE,      // Focus on blocking/dodging
  NPC_COMBAT_TACTICAL,       // Strategic positioning
  NPC_COMBAT_COUNT
} NPCCombatBehavior;

// Combat tactics for AI decision making
typedef enum {
  NPC_TACTIC_AGGRESSIVE,     // Rush and overwhelm
  NPC_TACTIC_DEFENSIVE,     // Wait for openings
  NPC_TACTIC_FLANK,          // Attack from sides/back
  NPC_TACTIC_KITE,          // Hit and run
  NPC_TACTIC_GROUP,          // Coordinate with allies
  NPC_TACTIC_AMBUSH,        // Hide and surprise
  NPC_TACTIC_RETREAT,        // Fall back and regroup
  NPC_TACTIC_COUNT
} NPCTactic;

// Combat state for individual NPCs
typedef struct {
  Entity current_target;
  Entity last_attacker;
  Vec3 last_known_target_position;
  f32 time_since_last_attack;
  f32 time_since_seen_target;
  
  // Combat behavior
  NPCCombatBehavior combat_behavior;
  NPCTactic current_tactic;
  NPCTactic preferred_tactic;
  
  // Combat stats
  f32 attack_range;
  f32 attack_damage;
  f32 attack_speed;
  f32 critical_chance;
  f32 accuracy;
  
  // State tracking
  bool is_in_combat;
  bool is_attacking;
  bool is_blocking;
  bool is_reloading;
  bool is_fleeing;
  
  // Tactical data
  f32 threat_level;
  u32 combo_count;
  f32 combo_timer;
  Vec3 preferred_attack_direction;
  f32 circling_angle;
  
  // Equipment
  bool has_weapon;
  bool has_shield;
  bool has_ranged_weapon;
  u32 current_weapon_type;
  u32 ammunition_count;
} NPCCombatState;

// Threat assessment for target selection
typedef struct {
  Entity entity;
  f32 threat_score;
  f32 distance;
  f32 health_percentage;
  bool is_player;
  bool is_hostile;
  u32 last_attack_time;
} ThreatAssessment;

// Main NPC combat system
typedef struct {
  struct World *ecs;
  struct PhysicsWorld *physics;
  NPCSystem *npc_system;
  
  // Active combat states
  NPCCombatState *combat_states;
  u32 max_states;
  u32 active_count;
  
  // Global combat settings
  f32 combat_update_interval;
  f32 threat_decay_rate;
  f32 max_combat_range;
  
  // Combat statistics
  u32 total_attacks;
  u32 total_hits;
  u32 total_criticals;
  u32 total_combat_events;
  
  bool is_initialized;
} NPCCombatSystem;

// Combat system management
void npc_combat_init(NPCCombatSystem *system, struct World *ecs, 
                     struct PhysicsWorld *physics, NPCSystem *npc_system);
void npc_combat_free(NPCCombatSystem *system);
void npc_combat_update(NPCCombatSystem *system, f32 delta_time);

// Combat state management
NPCCombatState *npc_combat_get_state(NPCCombatSystem *system, Entity entity);
void npc_combat_create_state(NPCCombatSystem *system, Entity entity, NPCType type);
void npc_combat_remove_state(NPCCombatSystem *system, Entity entity);

// Combat actions
bool npc_combat_attack(NPCCombatSystem *system, Entity attacker, Entity target);
bool npc_combat_ranged_attack(NPCCombatSystem *system, Entity attacker, 
                              Vec3 direction, f32 range);
bool npc_combat_melee_attack(NPCCombatSystem *system, Entity attacker, Entity target);
bool npc_combat_explosive_attack(NPCCombatSystem *system, Entity attacker, Vec3 position);

// Target selection and tactics
Entity npc_combat_select_target(NPCCombatSystem *system, Entity entity);
NPCTactic npc_combat_select_tactic(NPCCombatSystem *system, Entity entity);
void npc_combat_update_tactic(NPCCombatSystem *system, Entity entity, f32 delta_time);

// Threat assessment
ThreatAssessment npc_combat_assess_threat(NPCCombatSystem *system, 
                                         Entity entity, Entity potential_target);
f32 npc_combat_calculate_threat_score(NPCCombatSystem *system, Entity entity, 
                                     Entity target);
void npc_combat_update_threats(NPCCombatSystem *system, Entity entity, f32 delta_time);

// Combat utilities
bool npc_combat_can_attack(NPCCombatSystem *system, Entity entity);
bool npc_combat_is_in_range(NPCCombatSystem *system, Entity attacker, Entity target);
f32 npc_combat_get_distance_to_target(NPCCombatSystem *system, Entity entity);
void npc_combat_set_combat_behavior(NPCCombatSystem *system, Entity entity, 
                                   NPCCombatBehavior behavior);

// Equipment and weapons
void npc_combat_equip_weapon(NPCCombatSystem *system, Entity entity, u32 weapon_type);
void npc_combat_unequip_weapon(NPCCombatSystem *system, Entity entity);
bool npc_combat_has_ammunition(NPCCombatSystem *system, Entity entity);
void npc_combat_reload(NPCCombatSystem *system, Entity entity);

// Combat events and callbacks
void npc_combat_on_attack_hit(NPCCombatSystem *system, Entity attacker, 
                             Entity target, f32 damage);
void npc_combat_on_attack_miss(NPCCombatSystem *system, Entity attacker, Entity target);
void npc_combat_on_combat_start(NPCCombatSystem *system, Entity entity, Entity target);
void npc_combat_on_combat_end(NPCCombatSystem *system, Entity entity);

// Debug and utilities
void npc_combat_debug_print_state(NPCCombatSystem *system, Entity entity);
u32 npc_combat_get_active_combat_count(NPCCombatSystem *system);

#ifdef __cplusplus
}
#endif

#endif // AI_NPC_COMBAT_H
