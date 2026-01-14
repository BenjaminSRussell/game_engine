/**
 * @file physics_api.h
 * @brief Main Physics API
 */

#ifndef VOXELFORGE_PHYSICS_API_H
#define VOXELFORGE_PHYSICS_API_H

#include "Physics/Public/physics_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// World Management
// ============================================================================

VF_API PhysicsWorld *physics_world_create(const PhysicsWorldDesc *desc);
VF_API void physics_world_destroy(PhysicsWorld *world);
VF_API void physics_world_step(PhysicsWorld *world, f32 delta_time);
VF_API void physics_world_set_gravity(PhysicsWorld *world, Vec3 gravity);
VF_API Vec3 physics_world_get_gravity(PhysicsWorld *world);

// ============================================================================
// Rigid Body
// ============================================================================

VF_API RigidBody *rigidbody_create(PhysicsWorld *world,
                                   const RigidBodyDesc *desc);
VF_API void rigidbody_destroy(RigidBody *body);

// Transform
VF_API Vec3 rigidbody_get_position(RigidBody *body);
VF_API void rigidbody_set_position(RigidBody *body, Vec3 position);
VF_API Quat rigidbody_get_rotation(RigidBody *body);
VF_API void rigidbody_set_rotation(RigidBody *body, Quat rotation);
VF_API Mat4 rigidbody_get_transform(RigidBody *body);

// Velocity
VF_API Vec3 rigidbody_get_linear_velocity(RigidBody *body);
VF_API void rigidbody_set_linear_velocity(RigidBody *body, Vec3 velocity);
VF_API Vec3 rigidbody_get_angular_velocity(RigidBody *body);
VF_API void rigidbody_set_angular_velocity(RigidBody *body, Vec3 velocity);

// Forces
VF_API void rigidbody_apply_force(RigidBody *body, Vec3 force);
VF_API void rigidbody_apply_force_at_point(RigidBody *body, Vec3 force,
                                           Vec3 point);
VF_API void rigidbody_apply_impulse(RigidBody *body, Vec3 impulse);
VF_API void rigidbody_apply_impulse_at_point(RigidBody *body, Vec3 impulse,
                                             Vec3 point);
VF_API void rigidbody_apply_torque(RigidBody *body, Vec3 torque);

// Properties
VF_API f32 rigidbody_get_mass(RigidBody *body);
VF_API void rigidbody_set_mass(RigidBody *body, f32 mass);
VF_API void rigidbody_set_kinematic(RigidBody *body, b8 kinematic);
VF_API b8 rigidbody_is_kinematic(RigidBody *body);
VF_API void rigidbody_set_sleeping(RigidBody *body, b8 sleeping);
VF_API b8 rigidbody_is_sleeping(RigidBody *body);

// User data
VF_API void *rigidbody_get_user_data(RigidBody *body);
VF_API void rigidbody_set_user_data(RigidBody *body, void *data);

// ============================================================================
// Collision Shapes
// ============================================================================

VF_API CollisionShape *shape_create_sphere(f32 radius);
VF_API CollisionShape *shape_create_box(Vec3 half_extents);
VF_API CollisionShape *shape_create_capsule(f32 radius, f32 height);
VF_API CollisionShape *shape_create_cylinder(f32 radius, f32 height);
VF_API CollisionShape *shape_create_convex_hull(const Vec3 *points,
                                                u32 point_count);
VF_API CollisionShape *shape_create_triangle_mesh(const Vec3 *vertices,
                                                  const u32 *indices,
                                                  u32 vertex_count,
                                                  u32 index_count);
VF_API CollisionShape *shape_create_compound(void);
VF_API void shape_compound_add(CollisionShape *compound, CollisionShape *child,
                               Vec3 offset, Quat rotation);
VF_API void shape_destroy(CollisionShape *shape);

// ============================================================================
// Collider (Shape + Transform attached to body)
// ============================================================================

VF_API Collider *collider_create(RigidBody *body, CollisionShape *shape,
                                 Vec3 offset, Quat rotation);
VF_API void collider_destroy(Collider *collider);
VF_API void collider_set_material(Collider *collider,
                                  const PhysicsMaterial *material);
VF_API void collider_set_trigger(Collider *collider, b8 is_trigger);
VF_API b8 collider_is_trigger(Collider *collider);

// ============================================================================
// Queries
// ============================================================================

VF_API b8 physics_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                          f32 max_distance, RaycastHit *out_hit, u32 mask);
VF_API u32 physics_raycast_all(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                               f32 max_distance, RaycastHit *hits, u32 max_hits,
                               u32 mask);
VF_API u32 physics_sphere_overlap(PhysicsWorld *world, Vec3 center, f32 radius,
                                  RigidBody **bodies, u32 max_bodies, u32 mask);
VF_API u32 physics_box_overlap(PhysicsWorld *world, Vec3 center,
                               Vec3 half_extents, Quat rotation,
                               RigidBody **bodies, u32 max_bodies, u32 mask);

// ============================================================================
// Joints
// ============================================================================

VF_API PhysicsJoint *joint_create_distance(RigidBody *a, RigidBody *b,
                                           Vec3 anchor_a, Vec3 anchor_b,
                                           f32 min_distance, f32 max_distance);
VF_API PhysicsJoint *joint_create_hinge(RigidBody *a, RigidBody *b, Vec3 anchor,
                                        Vec3 axis, f32 min_angle,
                                        f32 max_angle);
VF_API PhysicsJoint *joint_create_ball(RigidBody *a, RigidBody *b, Vec3 anchor);
VF_API PhysicsJoint *joint_create_slider(RigidBody *a, RigidBody *b, Vec3 axis,
                                         f32 min_distance, f32 max_distance);
VF_API PhysicsJoint *joint_create_fixed(RigidBody *a, RigidBody *b);
VF_API void joint_destroy(PhysicsJoint *joint);
VF_API void joint_set_motor(PhysicsJoint *joint, f32 target_velocity,
                            f32 max_force);
VF_API void joint_enable_collision(PhysicsJoint *joint, b8 enable);

// ============================================================================
// Character Controller
// ============================================================================

typedef struct CharacterControllerDesc {
  f32 radius;
  f32 height;
  f32 step_height;
  f32 slope_limit; // Max slope angle in degrees
  f32 skin_width;
  Vec3 up_direction;
  CollisionFilter filter;
} CharacterControllerDesc;

VF_API CharacterController *
character_controller_create(PhysicsWorld *world,
                            const CharacterControllerDesc *desc);
VF_API void character_controller_destroy(CharacterController *controller);
VF_API void character_controller_move(CharacterController *controller,
                                      Vec3 displacement, f32 delta_time);
VF_API Vec3 character_controller_get_position(CharacterController *controller);
VF_API void character_controller_set_position(CharacterController *controller,
                                              Vec3 position);
VF_API b8 character_controller_is_grounded(CharacterController *controller);

// ============================================================================
// Debug
// ============================================================================

VF_API void physics_debug_draw_world(PhysicsWorld *world);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_PHYSICS_API_H
