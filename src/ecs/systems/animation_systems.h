#ifndef ANIMATION_SYSTEMS_H
#define ANIMATION_SYSTEMS_H

#include "ecs/ecs.h"

/**
 * ECS Animation Systems
 *
 * - AnimationUpdateSystem: Advances time, handles state transitions + blending
 * - SkinningSystem: Generates final bone matrices for rendering
 */

/**
 * Register animation systems with ECS world
 */
void register_animation_systems(World *world);

/**
 * Animation Update System (Priority 70)
 * Advances playback time, updates blend weights
 */
void animation_update_system(SystemContext *ctx);

/**
 * Skinning System (Priority 80)
 * Evaluates poses and computes world-space bone matrices
 */
void animation_skinning_system(SystemContext *ctx);

#endif // ANIMATION_SYSTEMS_H
