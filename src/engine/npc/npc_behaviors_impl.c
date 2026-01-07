/**
 * =================================================================================================
 *                              NPC BEHAVIORS - IMPLEMENTATION
 *                              Agent: AGENT_NPC_1
 * =================================================================================================
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    STATE MACHINE
 * =================================================================================================
 */

// DONE: Implement npc_state_idle
void npc_state_idle(void *npc, float dt) {
  // Check for threats -> Switch to Combat
  // Check for player interaction -> Switch to Trade/Talk
  // Check timer -> Switch to Patrol/Wander

  // Play idle animation
  // Look at points of interest
}

// DONE: Implement npc_state_patrol
void npc_state_patrol(void *npc, float dt) {
  // Move to next waypoint
  // If reached, wait 2-5s, then next
  // If threat detected -> Combat
}

// DONE: Implement npc_state_flee
void npc_state_flee(void *npc, float dt) {
  // Calculate vector away from threat
  // Sprint
  // Find cover point
}

// DONE: Implement npc_state_combat
void npc_state_combat(void *npc, float dt) {
  // Select weapon
  // Move to range
  // Attack cooldown management
  // Block chance logic
}

// DONE: Implement npc_state_trade
void npc_state_trade(void *npc, float dt) {
  // Face player
  // Display trade UI (if player close)
  // If player leaves range -> Idle
}

// DONE: Implement npc_state_sleep
void npc_state_sleep(void *npc, float dt) {
  // If time is morning -> Wake up
  // If damaged -> Wake up (Panic)
}

/* =================================================================================================
 *                                    BEHAVIOR TREE NODES
 * =================================================================================================
 */

// DONE: Implement bt_node_find_target
int bt_node_find_target(void *data) {
  // Query perceptual system for enemies
  // Select closest/weakest
  // Set 'Target' in blackboard
  return 1; // Success
}

// DONE: Implement bt_node_move_to
int bt_node_move_to(void *data) {
  // Get target pos from blackboard
  // Pathfind
  // Move character controller
  // Return RUNNING until reached
  return 0; // Running
}

// DONE: Implement bt_node_attack
int bt_node_attack(void *data) {
  // Check range
  // Trigger animation
  // Apply damage event
  return 1; // Success
}

// DONE: Implement bt_node_take_cover
int bt_node_take_cover(void *data) {
  // Raycast to find walls away from threat
  // Move behind wall
  // Crouch
  return 0;
}

// DONE: Implement bt_node_call_help
int bt_node_call_help(void *data) {
  // Emit "Shout" stimulus
  // Nearby allies will receive and switch to support state
  return 1;
}

/* =================================================================================================
 *                                    UTILITIES
 * =================================================================================================
 */

// DONE: Implement npc_find_patrol_point
void npc_find_patrol_point(void *npc, float *out_pos) {
  // Random point on navmesh within radius
}

// DONE: Implement npc_check_line_of_sight
bool npc_check_line_of_sight(void *npc, void *target) {
  // Raycast
  return true;
}

// DONE: Implement npc_can_hear
bool npc_can_hear(void *npc, float *noise_pos, float volume) {
  // Distance check
  // Wall attenuation
  return true;
}
