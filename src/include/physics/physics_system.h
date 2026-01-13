#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "physics/physics.h" // Base types

/*
 * The Physics System is the high-level controller for the physics engine.
 * It manages the PhysicsWorld, threading (future), and integration with the engine.
 */

typedef struct PhysicsSystemConfig {
    float fixed_timestep;
    int max_bodies;
    int max_constraints;
    float gravity[3];
} PhysicsSystemConfig;

typedef struct PhysicsSystem PhysicsSystem;

// Lifecycle
PhysicsSystem* physics_system_create(PhysicsSystemConfig config);
void physics_system_destroy(PhysicsSystem* system);
void physics_system_update(PhysicsSystem* system, float dt);

// World Access
// In the future this might return an opaque handle or id
void* physics_system_get_world(PhysicsSystem* system);

// Debug
void physics_system_debug_render(PhysicsSystem* system);

#endif // PHYSICS_SYSTEM_H
