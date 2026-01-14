#ifndef PHYSICS_TYPES_H
#define PHYSICS_TYPES_H

#include "../Public/unified_physics.h"
#include <core/logger.h>
#include <pthread.h>

// -----------------------------------------------------------------------------
// Internal Types
// -----------------------------------------------------------------------------

// Performance statistics structure
typedef struct {
  double integration_time_ms;
  double broadphase_time_ms;
  double narrowphase_time_ms;
  double solver_time_ms;
  double total_time_ms;
  int bodies_processed;
  int collision_pairs;
  int constraint_islands;
  u64 cache_misses;
  u64 branch_mispredictions;
} PhysicsPerformance;

// Internal Physics World Structure
struct PhysicsWorld {
  Vec3 gravity;
  f32 fixed_timestep;
  u32 velocity_iterations;
  u32 position_iterations;

  // Body management
  RigidBody *bodies;
  u32 body_count;
  u32 body_capacity;

  // Collision detection
  CollisionManifold *manifolds;
  u32 manifold_count;
  u32 manifold_capacity;

  // Performance tracking
  PhysicsPerformance performance;

  // Threading
  void *solver_mutex; // pthread_mutex_t*
  bool deterministic_mode;
};

// Internal Rigid Body Structure
struct RigidBody {
  BodyType type;
  Vec3 position;
  Vec3 velocity;
  Vec3 force;
  Vec3 acceleration;
  Quat rotation;
  Vec3 angular_velocity;
  Vec3 torque;
  Vec3 angular_acceleration;

  f32 mass;
  f32 inv_mass;
  f32 inertia;
  f32 inv_inertia;
  f32 restitution;
  f32 friction;
  f32 linear_damping;
  f32 angular_damping;

  Collider *collider;
  bool is_sleeping;
  f32 sleep_timer;

  // User data
  void *user_data;
};

// Internal Collider Structure
struct Collider {
  ColliderType type;
  union {
    struct {
      f32 radius;
    } sphere;
    struct {
      Vec3 half_extents;
    } box;
    struct {
      f32 radius;
      f32 height;
    } capsule;
    struct {
      void *mesh_data;
    } mesh;
  } shape;

  PhysicsMaterial material;

  // Transform relative to body
  Vec3 local_position;
  Quat local_rotation;
};

// -----------------------------------------------------------------------------
// Internal Function Declarations
// -----------------------------------------------------------------------------

// Collision Detection
bool physics_check_collision(RigidBody *a, RigidBody *b,
                             CollisionManifold *out);

// Solver
void physics_resolve_collision(RigidBody *a, RigidBody *b,
                               CollisionManifold *m);

// Utils
static inline f32 clamp_f32(f32 value, f32 min, f32 max) {
  return fmaxf(min, fminf(max, value));
}

#endif // PHYSICS_TYPES_H
