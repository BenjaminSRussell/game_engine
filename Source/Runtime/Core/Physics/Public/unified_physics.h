#ifndef UNIFIED_PHYSICS_H
#define UNIFIED_PHYSICS_H

#include <common.h>
#include <math/quat.h>
#include <math/vec3.h>
// Include unified_logger and unified_memory if needed for macros in headers,
// strictly speaking headers shouldn't depend on them unless inline functions
// use them.

// -----------------------------------------------------------------------------
// Opaque Types
// -----------------------------------------------------------------------------
typedef struct PhysicsWorld PhysicsWorld;
typedef struct RigidBody RigidBody;
typedef struct Collider Collider;
typedef struct CharacterController CharacterController;

// -----------------------------------------------------------------------------
// Enums & Structs (Public)
// -----------------------------------------------------------------------------

typedef enum {
  BODY_TYPE_STATIC,    // Zero mass, zero velocity, moved manually
  BODY_TYPE_KINEMATIC, // Zero mass, non-zero velocity, moved by solver
  BODY_TYPE_DYNAMIC    // Positive mass, non-zero velocity, moved by solver
} BodyType;

typedef enum {
  COLLIDER_TYPE_SPHERE,
  COLLIDER_TYPE_BOX,
  COLLIDER_TYPE_CAPSULE,
  COLLIDER_TYPE_MESH
} ColliderType;

typedef struct {
  RigidBody *body_a;
  RigidBody *body_b;
  Vec3 point;
  Vec3 normal;
  f32 depth;
} CollisionManifold;

typedef struct {
  Vec3 gravity;
  f32 fixed_timestep;
  u32 velocity_iterations;
  u32 position_iterations;
} PhysicsConfig;

typedef struct {
  f32 friction;
  f32 restitution;
  f32 density;
} PhysicsMaterial;

typedef struct {
  Vec3 origin;
  Vec3 direction;
  f32 max_distance;
  bool hit;
  Vec3 hit_point;
  Vec3 hit_normal;
  f32 hit_distance;
  RigidBody *hit_body;
} RaycastResult;

// -----------------------------------------------------------------------------
// Physics World API
// -----------------------------------------------------------------------------

/**
 * @brief Creates a new physics world with the given configuration.
 */
PhysicsWorld *physics_world_create(PhysicsConfig config);

/**
 * @brief Destroys the physics world and frees all associated resources.
 */
void physics_world_destroy(PhysicsWorld *world);

/**
 * @brief Steps the physics simulation by delta_time.
 */
void physics_world_step(PhysicsWorld *world, f32 delta_time);

/**
 * @brief Adds a rigid body to the world.
 */
RigidBody *physics_world_add_body(PhysicsWorld *world, RigidBody *body);

/**
 * @brief Removes a rigid body from the world.
 */
void physics_world_remove_body(PhysicsWorld *world, RigidBody *body);

/**
 * @brief Performs a raycast in the physics world.
 */
RaycastResult physics_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                              f32 max_distance);

// -----------------------------------------------------------------------------
// Rigid Body API
// -----------------------------------------------------------------------------

RigidBody *rigid_body_create(BodyType type, Vec3 position);
void rigid_body_destroy(RigidBody *body);

// Properties
void rigid_body_set_mass(RigidBody *body, f32 mass);
void rigid_body_set_friction(RigidBody *body, f32 friction);
void rigid_body_set_restitution(RigidBody *body, f32 restitution);
void rigid_body_set_linear_damping(RigidBody *body, f32 damping);
void rigid_body_set_angular_damping(RigidBody *body, f32 damping);

// State
void rigid_body_set_position(RigidBody *body, Vec3 position);
Vec3 rigid_body_get_position(const RigidBody *body);
void rigid_body_set_rotation(RigidBody *body, Quat rotation);
Quat rigid_body_get_rotation(const RigidBody *body);
Vec3 rigid_body_get_velocity(const RigidBody *body);
void rigid_body_set_velocity(RigidBody *body, Vec3 velocity);

// Forces
void rigid_body_add_force(RigidBody *body, Vec3 force);
void rigid_body_add_impulse(RigidBody *body, Vec3 impulse);
void rigid_body_clear_forces(RigidBody *body);

// Accessors
f32 rigid_body_get_inv_mass(const RigidBody *body);
f32 rigid_body_get_restitution(const RigidBody *body);
Collider *rigid_body_get_collider(const RigidBody *body);
bool rigid_body_is_sleeping(const RigidBody *body);

// Setup
void rigid_body_attach_collider(RigidBody *body, Collider *collider);

// -----------------------------------------------------------------------------
// Collider API
// -----------------------------------------------------------------------------

Collider *collider_create_sphere(f32 radius);
Collider *collider_create_box(Vec3 half_extents);
void collider_destroy(Collider *collider);

ColliderType collider_get_type(const Collider *collider);
void collider_get_box_half_extents(const Collider *collider, f32 *out_x,
                                   f32 *out_y, f32 *out_z);
f32 collider_get_sphere_radius(const Collider *collider);

// Helper
RigidBody *physics_create_box(PhysicsWorld *world, Vec3 pos, Vec3 size,
                              f32 mass, PhysicsMaterial *mat);

#endif // UNIFIED_PHYSICS_H
