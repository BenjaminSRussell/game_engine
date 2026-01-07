#ifndef PHYSICS_INTERNAL_H
#define PHYSICS_INTERNAL_H

#include <ecs/ecs.h>
#include <physics/physics.h>

struct Collider {
  ColliderType type;
  union {
    struct {
      f32 radius;
    } sphere;
    struct {
      Vec3 half_extents;
    } box;
  } data;
};

struct RigidBody {
  BodyType type;
  EntityID entity_id;

  // State
  Vec3 position;
  Quat rotation;
  Vec3 velocity;
  Vec3 angular_velocity;

  // Forces
  Vec3 force_accumulator;
  Vec3 torque_accumulator;

  // Properties
  f32 mass;
  f32 inv_mass;
  f32 friction;
  f32 restitution;
  f32 linear_damping;
  f32 angular_damping;

  // Shape
  Collider *collider;

  // Sleep state
  bool is_awake;
  f32 sleep_timer;
};

struct PhysicsWorld {
  PhysicsConfig config;
  // Map from entity ID to RigidBody
  RigidBody *body_map[MAX_ENTITIES];
  // List of all bodies for iteration
  RigidBody *bodies[MAX_ENTITIES];
  u32 body_count;
  // Fixed timestep accumulator
  f32 accumulator;
};

#endif // PHYSICS_INTERNAL_H
