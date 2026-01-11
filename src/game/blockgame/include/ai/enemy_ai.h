// include/ai/enemy_ai.h
//
// Purpose: Defines the public API and data structures for enemy artificial intelligence
// with different behavior patterns. This header provides structures for AI states,
// behavior types, decision-making logic, and functions for managing AI entities
// with various combat and movement patterns.
//
// Public APIs:
// - `AIBehaviorType`: Enumeration for different AI behavior patterns.
// - `AIState`: Enumeration for AI mental states and conditions.
// - `AITargetPriority`: Enumeration for target selection priorities.
// - `AISensorData`: Structure containing sensory information about the environment.
// - `AIBehavior`: Structure defining behavior parameters and decision weights.
// - `EnemyAI`: Main AI component containing current state, behavior, and decision data.
// - `enemy_ai_init`: Initializes an enemy AI with specific behavior type.
// - `enemy_ai_free`: Frees AI component resources.
// - `enemy_ai_update`: Updates AI decision-making and behavior each frame.
// - `enemy_ai_set_state`: Changes the AI's current state.
// - `enemy_ai_perceive`: Updates AI's sensory data about the environment.
// - `enemy_ai_select_target`: Selects the best target based on priorities.
// - `enemy_ai_execute_behavior`: Executes the current behavior pattern.
// - `enemy_ai_should_attack`: Determines if AI should attack based on conditions.
// - `enemy_ai_should_flee`: Determines if AI should retreat based on health/threat.
// - `enemy_ai_should_patrol`: Determines if AI should patrol or hold position.
//
// Ownership: `EnemyAI` is owned by entities as an ECS component.
// The AI system manages decision-making but does not own the entities it controls.
//
// Invariants:
// - AI components must be initialized before use.
// - Behavior weights should sum to 1.0 for proper probability distribution.
// - State transitions must follow valid state machine rules.
// - Sensory range values must be positive and reasonable for game scale.

#ifndef ENEMY_AI_H
#define ENEMY_AI_H

#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct ECSWorld;
typedef struct Entity Entity;

// AI behavior types
typedef enum {
  AI_BEHAVIOR_AGGRESSIVE,    // Always attacks on sight, high aggression
  AI_BEHAVIOR_DEFENSIVE,     // Attacks only when provoked or defending territory
  AI_BEHAVIOR_PASSIVE,       // Never attacks unless attacked first
  AI_BEHAVIOR_COWARDLY,      // Flees from combat, avoids confrontation
  AI_BEHAVIOR_TERRITORIAL,   // Defends specific area, aggressive in territory
  AI_BEHAVIOR_HUNTER,        // Stalks prey, strategic attacks from behind
  AI_BEHAVIOR_GUARDIAN,      // Protects specific target or location
  AI_BEHAVIOR_BERSERKER,     // Attacks relentlessly, ignores danger
  AI_BEHAVIOR_COUNT
} AIBehaviorType;

// AI states
typedef enum {
  AI_STATE_IDLE,             // Waiting, no specific action
  AI_STATE_PATROLLING,       // Moving along patrol route
  AI_STATE_CHASING,          // Pursuing a target
  AI_STATE_ATTACKING,         // Engaging in combat
  AI_STATE_FLEEING,          // Retreating from danger
  AI_STATE_SEARCHING,        // Looking for lost target or resources
  AI_STATE_GUARDING,         // Protecting position or target
  AI_STATE_STUNNED,          // Temporarily incapacitated
  AI_STATE_SLEEPING,         // Inactive until disturbed
  AI_STATE_COUNT
} AIState;

// Target priority types
typedef enum {
  AI_PRIORITY_CLOSEST,        // Target nearest entity
  AI_PRIORITY_WEAKEST,        // Target entity with lowest health
  AI_PRIORITY_STRONGEST,      // Target entity with highest threat
  AI_PRIORITY_PLAYER,          // Prioritize player entities
  AI_PRIORITY_RANDOM,          // Random target selection
  AI_PRIORITY_COUNT
} AITargetPriority;

// AI sensory data
typedef struct {
  bool can_see_player;        // Player detected visually
  bool can_hear_player;       // Player detected by sound
  bool player_in_range;       // Player within attack range
  bool player_in_territory;   // Player within AI's territory
  bool under_attack;          // AI is currently being attacked
  bool low_health;            // Health is below threshold
  bool allies_nearby;         // Friendly entities nearby
  bool obstacles_ahead;        // Path blocked by obstacles
  Vec3 last_known_position;   // Last known player position
  f32 distance_to_player;     // Current distance to player
  f32 time_since_seen;        // Time since player was last seen
  f32 threat_level;          // Overall threat assessment
} AISensorData;

// AI behavior parameters
typedef struct {
  AIBehaviorType type;
  f32 aggression_level;       // How likely to engage in combat (0.0-1.0)
  f32 courage_level;         // How likely to stand ground (0.0-1.0)
  f32 intelligence_level;     // Affects decision quality (0.0-1.0)
  f32 patrol_radius;         // Range for patrolling behavior
  f32 territory_radius;      // Size of defended territory
  f32 chase_range;          // Maximum distance to pursue targets
  f32 attack_range;          // Preferred attack distance
  f32 flee_threshold;        // Health percentage to trigger fleeing
  f32 decision_frequency;    // How often AI makes decisions (seconds)
  f32 reaction_time;        // Delay before responding to stimuli
  AITargetPriority target_priority; // How targets are selected
} AIBehavior;

// Enemy AI component
typedef struct {
  AIBehavior behavior;
  AIState current_state;
  AIState previous_state;
  AISensorData sensors;
  
  // Decision making
  f32 decision_timer;
  f32 state_timer;
  Vec3 patrol_center;
  Vec3 patrol_target;
  u32 patrol_waypoint_index;
  bool has_patrol_route;
  
  // Combat behavior
  EntityID current_target;
  f32 attack_cooldown;
  f32 last_attack_time;
  f32 combat_engagement_time;
  u32 attacks_landed;
  u32 times_attacked;
  
  // Movement and navigation
  Vec3 movement_target;
  Vec3 look_direction;
  f32 movement_speed;
  bool is_moving;
  bool is_running;
  
  // Memory and learning
  Vec3 danger_locations[8];   // Remember dangerous areas
  u32 danger_location_count;
  f32 memory_duration;        // How long memories last
  
  // Behavioral modifiers
  f32 morale;                // Current morale level
  f32 fatigue;               // Current fatigue level
  bool is_enraged;           // Berserk mode activation
  f32 enrage_timer;          // Time remaining in enrage state
} EnemyAI;

// AI lifecycle functions
void enemy_ai_init(EnemyAI* ai, AIBehaviorType behavior_type, Vec3 patrol_center);
void enemy_ai_free(EnemyAI* ai);

// AI update functions
void enemy_ai_update(EnemyAI* ai, struct ECSWorld* ecs, EntityID entity_id, f32 delta_time);
void enemy_ai_perceive(EnemyAI* ai, struct ECSWorld* ecs, EntityID entity_id);

// State management
void enemy_ai_set_state(EnemyAI* ai, AIState new_state);
bool enemy_ai_can_transition_to(EnemyAI* ai, AIState target_state);

// Hierarchical state machine
void enemy_ai_hierarchical_update(EnemyAI *ai, struct ECSWorld *ecs, EntityID entity_id, f32 delta_time);
void enemy_ai_force_state_transition(EnemyAI *ai, AIState new_state, struct ECSWorld *ecs, EntityID entity_id);

// Decision making
EntityID enemy_ai_select_target(EnemyAI* ai, struct ECSWorld* ecs, EntityID entity_id);
void enemy_ai_execute_behavior(EnemyAI* ai, struct ECSWorld* ecs, EntityID entity_id, f32 delta_time);

// Behavior checks
bool enemy_ai_should_attack(EnemyAI* ai, EntityID target);
bool enemy_ai_should_flee(EnemyAI* ai);
bool enemy_ai_should_patrol(EnemyAI* ai);
bool enemy_ai_should_chase(EnemyAI* ai, EntityID target);

// Combat AI
void enemy_ai_update_combat(EnemyAI* ai, struct ECSWorld* ecs, EntityID entity_id, f32 delta_time);
void enemy_ai_select_attack_pattern(EnemyAI* ai, EntityID target);
Vec3 enemy_ai_get_attack_position(EnemyAI* ai, EntityID target);

// Movement AI
void enemy_ai_update_movement(EnemyAI* ai, struct ECSWorld* ecs, EntityID entity_id, f32 delta_time);
void enemy_ai_set_patrol_route(EnemyAI* ai, Vec3* waypoints, u32 waypoint_count);
Vec3 enemy_ai_get_next_patrol_point(EnemyAI* ai);

// Utility functions
const char* ai_get_behavior_name(AIBehaviorType type);
const char* ai_get_state_name(AIState state);
f32 ai_calculate_threat_level(EnemyAI* ai, struct ECSWorld* ecs, EntityID entity_id, EntityID target);
void ai_add_danger_memory(EnemyAI* ai, Vec3 location);
bool ai_is_location_dangerous(EnemyAI* ai, Vec3 location);

// AI creation helpers
AIBehavior ai_create_behavior(AIBehaviorType type);
EnemyAI enemy_ai_create(AIBehaviorType behavior_type, Vec3 patrol_center);

#ifdef __cplusplus
}
#endif

#endif // ENEMY_AI_H
