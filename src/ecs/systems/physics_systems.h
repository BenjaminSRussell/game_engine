#ifndef PHYSICS_SYSTEMS_H
#define PHYSICS_SYSTEMS_H

#include "ecs/ecs.h"

/**
 * ECS Physics Systems
 *
 * - Physics Simulation System: Integrates velocity and position
 * - Collision Detection System: Detects collisions (broad/narrow phase)
 * - Constraint Solver System: Solves constraints and contacts
 */

/**
 * Register all physics systems with ECS world
 */
void register_physics_systems(World *world);

/**
 * Physics Simulation System (Integration)
 * Updates position/velocity based on forces and gravity
 */
void physics_simulation_system(SystemContext *ctx);

/**
 * Collision Detection System
 * Detects collisions between colliders and generates contacts
 */
void collision_detection_system(SystemContext *ctx);

#endif // PHYSICS_SYSTEMS_H
