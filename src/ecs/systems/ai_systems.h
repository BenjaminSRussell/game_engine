#ifndef AI_SYSTEMS_H
#define AI_SYSTEMS_H

#include "ecs/ecs.h"

/**
 * ECS AI Systems
 *
 * - Perception System: Updates sensory data (vision/hearing)
 * - Decision System: Updates Agent state and logic (FSM)
 * - Navigation System: Updates movement along path
 */

/**
 * Register AI systems with ECS world
 */
void register_ai_systems(World *world);

/**
 * Perception System (Priority 45)
 * Detects visible entities and updates PerceptionComponent
 */
void ai_perception_system(SystemContext *ctx);

/**
 * Decision System (Priority 46)
 * Updates FSM state (Idle -> Chase via Aggression/Vision)
 */
void ai_decision_system(SystemContext *ctx);

/**
 * Navigation System (Priority 47)
 * Moves entity towards current waypoint
 */
void ai_navigation_system(SystemContext *ctx);

#endif // AI_SYSTEMS_H
