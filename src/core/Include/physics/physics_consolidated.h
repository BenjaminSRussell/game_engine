// include/physics/physics_consolidated.h
//
// Purpose: Unified physics API consolidating all physics subsystems
// This replaces multiple disparate physics APIs with a single interface

#ifndef PHYSICS_CONSOLIDATED_H
#define PHYSICS_CONSOLIDATED_H

#include <common.h>
#include "math/vec3.h"
#include "math/quat.h"
#include "physics/physics.h"  // Include base physics definitions
#include "ecs/ecs.h"

// Forward declarations
typedef struct PhysicsWorld PhysicsWorld;
typedef struct RigidBody RigidBody;
typedef struct SoftBody SoftBody;
typedef struct FluidVolume FluidVolume;
typedef struct Cloth Cloth;
typedef struct VehicleChassis VehicleChassis;

// Unified physics configuration
typedef struct {
    Vec3 gravity;
    f32 time_step;
    u32 max_substeps;
    u32 solver_iterations;
    bool enable_gpu_acceleration;
    bool enable_sleeping;
    u32 worker_threads;
// PhysicsConfig is defined in physics.h to avoid duplication

// Unified material system
typedef struct {
    f32 density;
    f32 friction;
    f32 restitution;
    f32 youngs_modulus;
    f32 poisson_ratio;
    f32 viscosity;
    u32 material_id;
} PhysicsMaterial;

// Collision shapes
typedef enum {
    COLLISION_SHAPE_SPHERE,
    COLLISION_SHAPE_BOX,
    COLLISION_SHAPE_CAPSULE,
    COLLISION_SHAPE_MESH,
    COLLISION_SHAPE_HEIGHTFIELD
} CollisionShapeType;

// Body types are defined in physics.h to avoid duplication

// ============================================================================
// MAIN PHYSICS WORLD API
// ============================================================================

// World management
PhysicsWorld* physics_world_create(const PhysicsConfig* config);
void physics_world_destroy(PhysicsWorld* world);
void physics_world_update(PhysicsWorld* world, f32 delta_time);
void physics_world_step(PhysicsWorld* world, f32 time_step);

// Configuration
PhysicsConfig physics_get_default_config(void);
void physics_set_gravity(PhysicsWorld* world, Vec3 gravity);
void physics_set_time_step(PhysicsWorld* world, f32 time_step);

// ============================================================================
// RIGID BODY API
// ============================================================================

RigidBody* physics_create_rigid_body(PhysicsWorld* world, Entity entity, BodyType type);
void physics_destroy_rigid_body(PhysicsWorld* world, RigidBody* body);
void physics_set_body_shape(RigidBody* body, CollisionShapeType type, void* shape_data);
void physics_set_body_position(RigidBody* body, Vec3 position);
void physics_set_body_velocity(RigidBody* body, Vec3 velocity);
void physics_apply_force(RigidBody* body, Vec3 force);
void physics_apply_impulse(RigidBody* body, Vec3 impulse);

// ============================================================================
// RAYCASTING API
// ============================================================================

typedef struct {
    bool hit;
    Vec3 point;
    Vec3 normal;
    f32 distance;
    Entity entity;
} RaycastHit;

bool physics_raycast(PhysicsWorld* world, Vec3 origin, Vec3 direction, 
                   f32 max_distance, RaycastHit* hit);

// ============================================================================
// BLOCK PHYSICS API
// ============================================================================

typedef struct {
    Vec3 position;
    Vec3 normal;
    f32 distance;
    u32 block_type;
    bool hit;
} BlockRaycastResult;

bool physics_block_raycast(PhysicsWorld* world, Vec3 origin, Vec3 direction,
                         f32 max_distance, BlockRaycastResult* result);

// ============================================================================
// PERFORMANCE STATS
// ============================================================================

typedef struct {
    u32 rigid_body_count;
    u32 collision_pairs;
    f64 simulation_time;
    f64 collision_time;
} PhysicsStats;

PhysicsStats physics_get_stats(PhysicsWorld* world);

#endif // PHYSICS_CONSOLIDATED_H
