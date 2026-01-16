#ifndef AI_COMPONENTS_H
#define AI_COMPONENTS_H

#include "core/types.h"
#include "engine/include/math/math_all.h"

/**
 * ECS AI Components
 *
 * - AgentComponent: Core AI logic data (aggression, target)
 * - PathfindingComponent: Navigation path and waypoints
 * - PerceptionComponent: Sensory data (vision, hearing)
 */

// ============================================================================
// AGENT COMPONENT
// ============================================================================

typedef enum {
  AGENT_STATE_IDLE,
  AGENT_STATE_PATROL,
  AGENT_STATE_CHASE,
  AGENT_STATE_ATTACK,
  AGENT_STATE_FLEE
} AgentState;

typedef struct {
  u32 type_id;      // Archetype (Grunt, Boss, etc.)
  AgentState state; // Current high-level state

  // Logic Parameters
  f32 aggression; // 0.0 - 1.0
  f32 speed;      // Movement speed
  f32 attack_range;

  // Target
  u32 target_entity_id; // Current target
  Vec3 last_known_pos;  // Last seen position of target

  // Timers
  f32 reaction_timer; // Time until next decision
} AgentComponent;

// ============================================================================
// PATHFINDING COMPONENT
// ============================================================================

#define MAX_WAYPOINTS 64

typedef struct {
  Vec3 waypoints[MAX_WAYPOINTS];
  u32 waypoint_count;
  u32 current_waypoint;

  Vec3 destination;
  bool has_path;
  bool path_pending; // Async path request active?
  bool reached_destination;

  void *path_handle; // Internal allocator handle
} PathfindingComponent;

// ============================================================================
// PERCEPTION COMPONENT
// ============================================================================

typedef struct {
  f32 view_distance; // Vision range
  f32 fov_angle;     // Field of view (degrees)
  f32 hearing_range; // Hearing range

  // Sensory Buffer (updated each frame by PerceptionSystem)
  u32 visible_count;
  u32 visible_entities[16]; // Entities currently seen

  bool can_see_target;
} PerceptionComponent;

// ============================================================================
// REGISTRATION & HELPERS
// ============================================================================

extern u32 g_agent_component_id;
extern u32 g_pathfinding_component_id;
extern u32 g_perception_component_id;

void register_ai_components(void *world);

AgentComponent agent_create(u32 type, f32 speed, f32 aggression);
PathfindingComponent pathfinding_create(void);
PerceptionComponent perception_create(f32 view_dist, f32 fov);

#endif // AI_COMPONENTS_H
