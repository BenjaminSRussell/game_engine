#ifndef PHYSICS_ENGINE_CORE_H
#define PHYSICS_ENGINE_CORE_H

#include "core/physics_types.h"
#include "physics/physics.h"

// Re-export core creation functions from physics_rigid_body_helpers.c
// Core Rigid Body Helper Declarations
RigidBody *core_rigid_body_create(uint32_t id, RigidBodyType type);
void core_rigid_body_destroy(RigidBody *body);
void core_rigid_body_get_aabb(RigidBody *body, float *min_out, float *max_out);

void core_rigid_body_set_position(RigidBody *body, const float *pos);
void core_rigid_body_set_rotation(RigidBody *body, const float *rot);
void core_rigid_body_set_velocity(RigidBody *body, const float *vel);
void core_rigid_body_set_angular_velocity(RigidBody *body, const float *vel);
void core_rigid_body_apply_force(RigidBody *body, const float *force);
void core_rigid_body_apply_impulse(RigidBody *body, const float *impulse);
bool core_rigid_body_get_velocity_at_point(RigidBody *body, const float *point, float *out_vel);

// Core Physics World Functions
PhysicsWorld *core_physics_world_create(PhysicsConfig config);
void core_physics_world_destroy(PhysicsWorld *world);
void core_physics_world_step(PhysicsWorld *world, float dt);
RigidBody *physics_world_add_body(PhysicsWorld *world, RigidBody *body);
bool core_physics_world_raycast(PhysicsWorld *world, const float *origin, const float *dir, float max_dist, RayHit *out_hit);

// Shapes (implemented in core/physics_shapes.c)
CollisionShape *shape_sphere_create(float radius);
CollisionShape *shape_box_create(float hx, float hy, float hz);
CollisionShape *shape_capsule_create(float radius, float height);
CollisionShape *shape_cylinder_create(float radius, float height);
CollisionShape *shape_cone_create(float radius, float height);
CollisionShape *shape_convex_hull_create(float *vertices, uint32_t count);
CollisionShape *shape_mesh_create(float *vertices, uint32_t v_count, uint32_t *indices, uint32_t i_count);
CollisionShape *shape_heightfield_create(float *heights, uint32_t width, uint32_t depth, float scale_x, float scale_y, float scale_z);
CollisionShape *shape_compound_create(void);
void shape_destroy(CollisionShape *shape);
void shape_calculate_bounds(CollisionShape *shape);
void shape_calculate_inertia(CollisionShape *shape, float mass, float *inertia);
bool shape_raycast(CollisionShape *shape, const float *origin, const float *dir, float max_dist, float *out_hit);

#endif // PHYSICS_ENGINE_CORE_H
