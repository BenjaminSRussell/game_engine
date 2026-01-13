// include/ai/npc_ai.h
//
// Purpose: Defines the NPC AI system that integrates behavior trees with the
// modern ECS architecture. This system provides high-level AI behaviors for NPCs
// including pathfinding, combat, social interactions, and goal-oriented planning.
//
// Public APIs:
// - `NPCAISystem`: Main AI system managing behavior trees for all NPCs
// - `npc_ai_system_init`: Initialize the AI system with behavior tree templates
// - `npc_ai_system_free`: Clean up AI system resources
// - `npc_ai_update`: Update AI for all NPCs (called each frame)
// - `npc_ai_set_behavior_tree`: Assign a behavior tree to an NPC
// - `npc_ai_get_blackboard`: Access an NPC's AI blackboard
// - `npc_ai_interrupt`: Interrupt current AI behavior
//
// Ownership: The NPCAISystem owns behavior tree instances and manages their
// lifecycle. Individual NPCs reference their assigned behavior trees.
//
// Invariants:
// - AI system must be initialized before assigning behavior trees
// - Behavior trees must be valid and properly constructed
// - Blackboard access should be thread-safe if using multi-threading
//
#ifndef AI_NPC_AI_H
#define AI_NPC_AI_H

#include <ai/npc.h>
#include <ai/npc_types.h>
#include <ai/behavior_tree.h>
#include <ecs/ecs.h>
#include <math/vec3.h>
#include "engine/include/common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct World;
struct PhysicsWorld;

// AI behavior templates for different NPC types
typedef enum {
  NPC_AI_TEMPLATE_NONE,
  NPC_AI_TEMPLATE_PASSIVE_ANIMAL,    // Cows, pigs, chickens
  NPC_AI_TEMPLATE_HOSTILE_MOB,       // Zombies, skeletons
  NPC_AI_TEMPLATE_NEUTRAL_VILLAGER,  // Villagers, traders
  NPC_AI_TEMPLATE_GUARD,             // Guards with patrol routes
  NPC_AI_TEMPLATE_MERCHANT,           // Shopkeepers with trading AI
  NPC_AI_TEMPLATE_CREEPER,            // Special creeper behavior
  NPC_AI_TEMPLATE_COUNT
} NPCAITemplate;

// AI goals for utility-based decision making
typedef enum {
  NPC_GOAL_NONE,
  NPC_GOAL_SURVIVE,           // Find safety, heal, avoid danger
  NPC_GOAL_EAT,               // Find and consume food
  NPC_GOAL_SLEEP,              // Find safe place to rest
  NPC_GOAL_SOCIALIZE,         // Interact with other NPCs
  NPC_GOAL_WORK,               // Perform job-related tasks
  NPC_GOAL_EXPLORE,           // Discover new areas
  NPC_GOAL_HUNT,              // Track and attack prey
  NPC_GOAL_FLEE,              // Escape from threats
  NPC_GOAL_TRADE,             // Engage in trading
  NPC_GOAL_BREED,             // Find mate and reproduce
  NPC_GOAL_PATROL,            // Guard patrol routes
  NPC_GOAL_COUNT
} NPCAIGoal;

// AI memory structure for storing recent events
typedef struct NPCAIMemory {
  Entity last_attacker;
  f32 time_since_attack;
  Vec3 last_known_player_position;
  f32 time_since_seen_player;
  Vec3 threat_location;
  f32 threat_level;
  Entity current_target;
  f32 target_acquisition_time;
  
  // Social memory
  Entity recent_interactions[8];
  f32 interaction_times[8];
  u32 interaction_count;
  
  // Environmental memory
  Vec3 safe_location;
  Vec3 food_location;
  Vec3 water_location;
  bool has_safe_location;
  bool has_food_location;
  bool has_water_location;
} NPCAIMemory;

// AI context for each NPC
typedef struct NPCAIContext {
  Entity entity;
  NPCType npc_type;
  BehaviorTree *behavior_tree;
  BTBlackboard *blackboard;
  
  NPCAIMemory memory;
  NPCAIGoal current_goal;
  NPCAIGoal previous_goal;
  
  f32 last_update_time;
  f32 decision_interval;
  f32 next_decision_time;
  
  // State tracking
  bool is_alert;
  bool is_combat_mode;
  bool is_fleeing;
  bool is_resting;
  bool is_working;
  
  // Performance optimization
  u32 update_frequency; // Update every N frames
  u32 frame_counter;
} NPCAIContext;

// Main AI system
typedef struct {
  struct World *ecs;
  struct PhysicsWorld *physics;
  
  // Behavior tree templates
  BehaviorTree *templates[NPC_AI_TEMPLATE_COUNT];
  
  // Active AI contexts
  NPCAIContext *contexts;
  u32 max_contexts;
  u32 context_count;
  
  // Global AI settings
  f32 global_update_interval;
  f32 perception_range;
  f32 combat_range;
  
  bool is_initialized;
} NPCAISystem;

// AI system management
void npc_ai_system_init(NPCAISystem *system, struct World *ecs, 
                       struct PhysicsWorld *physics);
void npc_ai_system_free(NPCAISystem *system);

// AI context management
NPCAIContext *npc_ai_create_context(NPCAISystem *system, Entity entity, NPCType type);
void npc_ai_destroy_context(NPCAISystem *system, Entity entity);
NPCAIContext *npc_ai_get_context(NPCAISystem *system, Entity entity);

// Behavior tree assignment
bool npc_ai_assign_behavior_tree(NPCAISystem *system, Entity entity, 
                               NPCAITemplate template);
bool npc_ai_assign_custom_tree(NPCAISystem *system, Entity entity, 
                              BehaviorTree *tree);

// AI update and control
void npc_ai_system_update(NPCAISystem *system, f32 delta_time);
void npc_ai_update_single(NPCAISystem *system, NPCAIContext *context, f32 delta_time);
void npc_ai_interrupt(NPCAISystem *system, Entity entity, const char *reason);

// Goal and utility system
NPCAIGoal npc_ai_select_goal(NPCAISystem *system, NPCAIContext *context);
f32 npc_ai_evaluate_goal(NPCAISystem *system, NPCAIContext *context, NPCAIGoal goal);
void npc_ai_set_goal(NPCAISystem *system, NPCAIContext *context, NPCAIGoal goal);

// Blackboard utilities
BTBlackboard *npc_ai_get_blackboard(NPCAISystem *system, Entity entity);
void npc_ai_set_blackboard_value(NPCAISystem *system, Entity entity, 
                                const char *key, void *value, BTValueType type);
void *npc_ai_get_blackboard_value(NPCAISystem *system, Entity entity, 
                                  const char *key, BTValueType *type);

// Memory and perception
void npc_ai_update_memory(NPCAISystem *system, NPCAIContext *context, f32 delta_time);
void npc_ai_perceive_environment(NPCAISystem *system, NPCAIContext *context);
bool npc_ai_can_see_entity(NPCAISystem *system, NPCAIContext *context, Entity target);
bool npc_ai_can_hear_entity(NPCAISystem *system, NPCAIContext *context, Entity target);

// Behavior tree creation helpers
BehaviorTree *npc_ai_create_passive_animal_tree(void);
BehaviorTree *npc_ai_create_hostile_mob_tree(void);
BehaviorTree *npc_ai_create_villager_tree(void);
BehaviorTree *npc_ai_create_guard_tree(void);
BehaviorTree *npc_ai_create_merchant_tree(void);
BehaviorTree *npc_ai_create_creeper_tree(void);

// Utility and debugging
u32 npc_ai_get_active_context_count(NPCAISystem *system);
void npc_ai_debug_print_context(NPCAISystem *system, Entity entity);
void npc_ai_debug_print_tree(NPCAISystem *system, Entity entity);

#ifdef __cplusplus
}
#endif

#endif // AI_NPC_AI_H
