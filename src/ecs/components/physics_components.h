#ifndef PHYSICS_COMPONENTS_H
#define PHYSICS_COMPONENTS_H

#include "core/types.h"
#include "engine/include/math/math_all.h"

/**
 * ECS Physics Components
 *
 * - RigidbodyComponent: Physical properties (mass, velocity)
 * - ColliderComponent: Collision shape (box, sphere, mesh)
 */

// ============================================================================
// RIGIDBODY COMPONENT
// ============================================================================

typedef struct {
  f32 mass;         // Mass in kg (0 = infinite/static)
  f32 drag;         // Linear drag (0-1)
  f32 angular_drag; // Angular drag (0-1)

  Vec3 velocity;         // Linear velocity
  Vec3 angular_velocity; // Angular velocity

  bool use_gravity;  // Apply gravity?
  bool is_kinematic; // Controlled by script, not physics?

  // Internal state (allocator handle)
  void *internal_body; // Pointer to physics engine body
} RigidbodyComponent;

// ============================================================================
// COLLIDER COMPONENT
// ============================================================================

typedef enum {
  COLLIDER_SHAPE_BOX,
  COLLIDER_SHAPE_SPHERE,
  COLLIDER_SHAPE_CAPSULE,
  COLLIDER_SHAPE_MESH
} ColliderShape;

typedef struct {
  ColliderShape shape_type;

  union {
    struct {
      Vec3 size;
    } box;
    struct {
      f32 radius;
    } sphere;
    struct {
      f32 radius;
      f32 height;
    } capsule;
    struct {
      u32 mesh_id;
      bool convex;
    } mesh;
  };

  Vec3 center; // Offset from entity center

  bool is_trigger; // Trigger only (no collision response)?
  f32 friction;    // Material friction
  f32 restitution; // Bounciness (0-1)

  // Internal state
  void *internal_collider;
} ColliderComponent;

// ============================================================================
// COMPONENT IDs (Global)
// ============================================================================

extern u32 g_rigidbody_component_id;
extern u32 g_collider_component_id;

// ============================================================================
// REGISTRATION
// ============================================================================

/**
 * Register physics components with ECS world
 */
void register_physics_components(void *world);

/**
 * Helper: Create rigidbody
 */
RigidbodyComponent rigidbody_create(f32 mass);

/**
 * Helper: Create box collider
 */
ColliderComponent collider_create_box(Vec3 size);

/**
 * Helper: Create sphere collider
 */
ColliderComponent collider_create_sphere(f32 radius);

/**
 * Helper: Create capsule collider
 */
ColliderComponent collider_create_capsule(f32 radius, f32 height);

#endif // PHYSICS_COMPONENTS_H
