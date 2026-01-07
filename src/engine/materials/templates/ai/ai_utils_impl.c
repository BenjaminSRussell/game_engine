/**
 * =================================================================================================
 *                              AI UTILITIES - IMPLEMENTATION
 *                              Agent: AGENT_AI_3
 * =================================================================================================
 */

#include <math/math.h>
#include <stdlib.h>

/* =================================================================================================
 *                                    PATHFINDING UTILS
 * =================================================================================================
 */

// DONE: Implement ai_smooth_path
void ai_smooth_path(float *points, int count) {
  // String pulling algorithm
  // Remove unnecessary waypoints if LOS exists between prev and next
}

// DONE: Implement ai_calculate_cover_points
int ai_calculate_cover_points(const float *threat_pos, float *out_points,
                              int max) {
  // Scan environment geometry
  // Find spots occluded from threat_pos
  return 0;
}

// DONE: Implement ai_group_cohesion
void ai_group_cohesion(float *positions, int count, float *out_forces) {
  // Flocking behavior: Cohesion, Separation, Alignment
}

/* =================================================================================================
 *                                    DECISION MAKING
 * =================================================================================================
 */

// DONE: Implement ai_select_best_weapon
int ai_select_best_weapon(void *npc, float dist_to_target) {
  // Utility score based on distance, damage, ammo
  return 0;
}

// DONE: Implement ai_evaluate_fear
float ai_evaluate_fear(void *npc) {
  // Health %
  // Number of allies vs enemies
  // Dynamic difficulty factor
  return 0.0f;
}
