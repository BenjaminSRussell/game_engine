/**
 * @file physics_types.h
 * @brief Physics subsystem type definitions
 */

#ifndef VOXELFORGE_PHYSICS_TYPES_H
#define VOXELFORGE_PHYSICS_TYPES_H

#include "Core/Public/Math/Geometry/aabb.h"
#include "Core/Public/Math/Quaternion/quat.h"
#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct PhysicsWorld PhysicsWorld;
typedef struct RigidBody RigidBody;
typedef struct Collider Collider;
typedef struct CollisionShape CollisionShape;
typedef struct PhysicsJoint PhysicsJoint;
typedef struct CharacterController CharacterController;
typedef struct SoftBody SoftBody;

// ============================================================================
// Body Types
// ============================================================================

typedef enum PhysicsBodyType {
  PHYSICS_BODY_STATIC = 0,    // Zero mass, never moves
  PHYSICS_BODY_KINEMATIC = 1, // Controlled externally, affects others
  PHYSICS_BODY_DYNAMIC = 2,   // Simulated by physics
} PhysicsBodyType;

typedef enum PhysicsShapeType {
  PHYSICS_SHAPE_SPHERE = 0,
  PHYSICS_SHAPE_BOX,
  PHYSICS_SHAPE_CAPSULE,
  PHYSICS_SHAPE_CYLINDER,
  PHYSICS_SHAPE_CONE,
  PHYSICS_SHAPE_CONVEX_HULL,
  PHYSICS_SHAPE_TRIANGLE_MESH,
  PHYSICS_SHAPE_HEIGHTFIELD,
  PHYSICS_SHAPE_COMPOUND,
} PhysicsShapeType;

// ============================================================================
// Collision Filtering
// ============================================================================

typedef struct CollisionFilter {
  u32 category; // What this object is
  u32 mask;     // What it can collide with
  i32 group;    // For grouping objects
} CollisionFilter;

#define COLLISION_CATEGORY_DEFAULT (1 << 0)
#define COLLISION_CATEGORY_STATIC (1 << 1)
#define COLLISION_CATEGORY_DYNAMIC (1 << 2)
#define COLLISION_CATEGORY_CHARACTER (1 << 3)
#define COLLISION_CATEGORY_TRIGGER (1 << 4)
#define COLLISION_CATEGORY_DEBRIS (1 << 5)

// ============================================================================
// Material Properties
// ============================================================================

typedef struct PhysicsMaterial {
  f32 friction;    // 0.0 - 1.0
  f32 restitution; // Bounciness 0.0 - 1.0
  f32 density;     // kg/m^3
} PhysicsMaterial;

// ============================================================================
// Rigid Body Descriptor
// ============================================================================

typedef struct RigidBodyDesc {
  PhysicsBodyType type;
  Vec3 position;
  Quat rotation;
  Vec3 linear_velocity;
  Vec3 angular_velocity;
  f32 mass;
  f32 linear_damping;
  f32 angular_damping;
  b8 allow_sleep;
  b8 continuous_collision;
  CollisionFilter filter;
  void *user_data;
} RigidBodyDesc;

// ============================================================================
// Shape Descriptors
// ============================================================================

typedef struct SphereShapeDesc {
  f32 radius;
} SphereShapeDesc;

typedef struct BoxShapeDesc {
  Vec3 half_extents;
} BoxShapeDesc;

typedef struct CapsuleShapeDesc {
  f32 radius;
  f32 height; // Total height including hemispheres
} CapsuleShapeDesc;

typedef struct ConvexHullShapeDesc {
  Vec3 *points;
  u32 point_count;
} ConvexHullShapeDesc;

typedef struct TriangleMeshShapeDesc {
  Vec3 *vertices;
  u32 *indices;
  u32 vertex_count;
  u32 index_count;
} TriangleMeshShapeDesc;

// ============================================================================
// Joint Types
// ============================================================================

typedef enum PhysicsJointType {
  PHYSICS_JOINT_DISTANCE = 0,
  PHYSICS_JOINT_HINGE,
  PHYSICS_JOINT_BALL,
  PHYSICS_JOINT_SLIDER,
  PHYSICS_JOINT_FIXED,
  PHYSICS_JOINT_GENERIC_6DOF,
} PhysicsJointType;

typedef struct JointDesc {
  PhysicsJointType type;
  RigidBody *body_a;
  RigidBody *body_b;
  Vec3 anchor_a;
  Vec3 anchor_b;
  Vec3 axis_a;
  Vec3 axis_b;
  b8 collision_enabled;
} JointDesc;

// ============================================================================
// Raycast/Query Results
// ============================================================================

typedef struct RaycastHit {
  RigidBody *body;
  Collider *collider;
  Vec3 point;
  Vec3 normal;
  f32 distance;
  u32 face_index;
} RaycastHit;

typedef struct OverlapResult {
  RigidBody **bodies;
  u32 count;
  u32 capacity;
} OverlapResult;

// ============================================================================
// Contact Information
// ============================================================================

typedef struct ContactPoint {
  Vec3 position;
  Vec3 normal;
  f32 penetration;
  f32 impulse;
} ContactPoint;

typedef struct ContactManifold {
  RigidBody *body_a;
  RigidBody *body_b;
  ContactPoint points[4];
  u32 point_count;
} ContactManifold;

// ============================================================================
// Callbacks
// ============================================================================

typedef void (*PhysicsContactCallback)(const ContactManifold *manifold,
                                       void *user_data);
typedef void (*PhysicsTriggerCallback)(RigidBody *body_a, RigidBody *body_b,
                                       b8 entered, void *user_data);

// ============================================================================
// World Configuration
// ============================================================================

typedef struct PhysicsWorldDesc {
  Vec3 gravity;
  f32 fixed_timestep;
  u32 max_substeps;
  u32 solver_iterations;
  b8 allow_ccd;
  PhysicsContactCallback contact_callback;
  PhysicsTriggerCallback trigger_callback;
  void *callback_user_data;
} PhysicsWorldDesc;

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_PHYSICS_TYPES_H
