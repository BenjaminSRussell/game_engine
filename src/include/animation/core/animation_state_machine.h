/**
 * @file animation_state_machine.h
 * @brief Graph-based Animation State Machine (ASM).
 */
#ifndef ANIMATION_CORE_ANIMATION_STATE_MACHINE_H
#define ANIMATION_CORE_ANIMATION_STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <animation/animation_system.h>
#include <ai/blackboard.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for internal types
typedef struct AnimGraph AnimGraph;
typedef struct AnimState AnimState;
typedef struct AnimTransition AnimTransition;

/**
 * Update the animation state machine.
 */
void asm_update(AnimGraph *graph, Blackboard *blackboard, float dt);

/**
 * Evaluate the graph to get the final output pose.
 */
void asm_evaluate_pose(AnimGraph *graph, float dt, Pose *out_pose);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_CORE_ANIMATION_STATE_MACHINE_H
