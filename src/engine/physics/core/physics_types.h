#ifndef PHYSICS_CORE_TYPES_H
#define PHYSICS_CORE_TYPES_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
// Note: Do NOT include physics/physics.h here to avoid circular dependency
// physics.h includes this file for internal type definitions

// Constants
#define MAX_PHYSICS_BODIES 4096
#define MAX_CONSTRAINTS 1024
#define MAX_CONTACTS 2048
#define MAX_CONTACT_PAIRS 8192
#define MAX_SOFT_BODIES 64
#define MAX_CLOTHS 32

// Forward declarations
struct PhysicsWorld;
struct RigidBody;
// Note: PhysicsConfig is defined in physics/physics.h

/* =================================================================================================
 *                                    COLLISION SHAPES
 * =================================================================================================
 */

typedef enum CollisionShapeType {
  COLLISION_SHAPE_SPHERE,
  COLLISION_SHAPE_BOX,
  COLLISION_SHAPE_CAPSULE,
  COLLISION_SHAPE_CYLINDER,
  COLLISION_SHAPE_CONE,
  COLLISION_SHAPE_CONVEX_HULL,
  COLLISION_SHAPE_MESH,
  COLLISION_SHAPE_HEIGHTFIELD,
  COLLISION_SHAPE_COMPOUND,
} CollisionShapeType;

typedef struct CollisionShape {
  uint32_t id;
  CollisionShapeType type;
  float local_transform[16];
  float bounds_min[3];
  float bounds_max[3];
  union {
    struct { float radius; } sphere;
    struct { float half_extents[3]; } box;
    struct { float radius; float height; } capsule;
    struct { float radius; float height; } cylinder;
    struct { float radius; float height; } cone;
    struct { float *vertices; uint32_t vertex_count; } convex_hull;
    struct { float *vertices; uint32_t *indices; uint32_t tri_count; } mesh;
    struct { float *heights; uint32_t width; uint32_t depth; float scale[3]; } heightfield;
  } data;
} CollisionShape;

// Collider is forward-declared in physics.h as "struct Collider"
// We define the actual struct here, aliasing CollisionShape
struct Collider {
  uint32_t id;
  CollisionShapeType type;
  float local_transform[16];
  float bounds_min[3];
  float bounds_max[3];
  union {
    struct { float radius; } sphere;
    struct { float half_extents[3]; } box;
    struct { float radius; float height; } capsule;
    struct { float radius; float height; } cylinder;
    struct { float radius; float height; } cone;
    struct { float *vertices; uint32_t vertex_count; } convex_hull;
    struct { float *vertices; uint32_t *indices; uint32_t tri_count; } mesh;
    struct { float *heights; uint32_t width; uint32_t depth; float scale[3]; } heightfield;
  } data;
};
// Typedef for convenience (compatible with forward declaration in physics.h)
typedef struct Collider Collider;

// Note: ColliderType is defined in physics.h as a separate enum
// typedef enum CollisionShapeType ColliderType; // REMOVED - conflicts with physics.h

// Map collision shape type enum values (for internal use)
#define COLLISION_SHAPE_TO_COLLIDER_SPHERE COLLIDER_TYPE_SPHERE
#define COLLISION_SHAPE_TO_COLLIDER_BOX COLLIDER_TYPE_BOX
#define COLLISION_SHAPE_TO_COLLIDER_CAPSULE COLLIDER_TYPE_CAPSULE

/* =================================================================================================
 *                                    RIGID BODY
 * =================================================================================================
 */

typedef enum RigidBodyType {
  RIGID_BODY_STATIC,
  RIGID_BODY_KINEMATIC,
  RIGID_BODY_DYNAMIC,
} RigidBodyType;

typedef struct RigidBody {
  uint32_t id;
  RigidBodyType type;

  // Transform
  float position[3];
  float rotation[4];
  float velocity[3];
  float angular_velocity[3];

  // Properties
  float mass;
  float inv_mass;
  float inertia_tensor[9];
  float inv_inertia_tensor[9];
  float linear_damping;
  float angular_damping;
  float friction;
  float restitution;

  // Collision
  CollisionShape *shape;
  uint32_t collision_layer;
  uint32_t collision_mask;

  // Forces
  float accumulated_force[3];
  float accumulated_torque[3];

  // State
  bool is_active;
  bool is_sleeping;
  float sleep_timer;
  float energy_threshold;

  // Constraints
  uint32_t *constraint_ids;
  uint32_t constraint_count;

  // User data
  void *user_data;
  uint32_t entity_id;
} RigidBody;

/* =================================================================================================
 *                                    COLLISION DETECTION
 * =================================================================================================
 */

typedef struct ContactPoint {
  float position_world_a[3];
  float position_world_b[3];
  float position_local_a[3];
  float position_local_b[3];
  float normal[3];
  float penetration_depth;
  float impulse_normal;
  float impulse_tangent[2];
  float friction;
  float restitution;
} ContactPoint;

typedef struct ContactManifold {
  uint32_t body_a;
  uint32_t body_b;
  ContactPoint points[4];
  uint32_t point_count;
  float normal[3];
  float tangent[2][3];
} ContactManifold;

typedef struct RayHit {
  RigidBody *body;
  float point[3];
  float normal[3];
  float distance;
  bool hit;
} RayHit;

/* =================================================================================================
 *                                    BROADPHASE
 * =================================================================================================
 */

typedef struct BroadPhaseAABB {
  float min[3];
  float max[3];
  uint32_t body_id;
} BroadPhaseAABB;

typedef struct BroadPhasePair {
  uint32_t body_a;
  uint32_t body_b;
} BroadPhasePair;

typedef struct BVHNode {
  float bounds_min[3];
  float bounds_max[3];
  uint32_t left_child;
  uint32_t right_child;
  uint32_t body_id;
  bool is_leaf;
} BVHNode;

typedef struct BroadPhase {
  BVHNode *nodes;
  uint32_t node_count;
  uint32_t node_capacity;
  uint32_t root;

  BroadPhasePair *pairs;
  uint32_t pair_count;
  uint32_t pair_capacity;

  bool needs_rebuild;
} BroadPhase;

/* =================================================================================================
 *                                    CONSTRAINTS
 * =================================================================================================
 */

typedef enum ConstraintType {
  CONSTRAINT_FIXED,
  CONSTRAINT_POINT_TO_POINT,
  CONSTRAINT_HINGE,
  CONSTRAINT_SLIDER,
  CONSTRAINT_CONE_TWIST,
  CONSTRAINT_GENERIC_6DOF,
  CONSTRAINT_SPRING,
  CONSTRAINT_MOTOR,
  CONSTRAINT_GEAR,
} ConstraintType;

typedef struct Constraint {
  uint32_t id;
  ConstraintType type;
  uint32_t body_a;
  uint32_t body_b;

  float local_anchor_a[3];
  float local_anchor_b[3];
  float local_axis_a[3];
  float local_axis_b[3];

  // Constraint-specific data
  union {
    struct { float relative_position[3]; float relative_rotation[4]; } fixed;
    struct { float pivot_a[3]; float pivot_b[3]; } point_to_point;
    struct { float axis[3]; float angle_min; float angle_max; bool has_limits; } hinge;
    struct { float axis[3]; float min_limit; float max_limit; bool has_limits; } slider;
  } data;

  // Limits
  float linear_limits[3][2];
  float angular_limits[3][2];
  bool limit_enabled[6];

  // Motor
  float motor_target_velocity[6];
  float motor_max_force[6];
  bool motor_enabled[6];

  // Spring
  float spring_stiffness[6];
  float spring_damping[6];
  bool spring_enabled[6];

  // Solver data
  float accumulated_impulse[6];
  float bias[6];
  float mass_matrix[6];

  bool is_broken;
  float break_force;
  float break_torque;
} Constraint;

/* =================================================================================================
 *                                    SOFT BODY / CLOTH (Forward Decls)
 * =================================================================================================
 */

// Placeholder structs for softbody/cloth - detailed definition in dynamics/
typedef struct SoftBody SoftBody;
typedef struct Cloth Cloth;

/* =================================================================================================
 *                                    PHYSICS WORLD
 * =================================================================================================
 */

typedef struct PhysicsWorld {
  RigidBody **bodies;
  uint32_t body_count;
  uint32_t body_capacity;

  Constraint *constraints;
  uint32_t constraint_count;
  
  SoftBody *soft_bodies; // Placeholder for now
  uint32_t soft_body_count;

  Cloth *cloths; // Placeholder for now
  uint32_t cloth_count;

  BroadPhase broadphase;
  ContactManifold *contacts;
  uint32_t contact_count;
  uint32_t contact_capacity;

  float gravity[3];
  float timestep;
  uint32_t velocity_iterations;
  uint32_t position_iterations;

  bool paused;
  float time_accumulator;

  // Callbacks
  void (*on_collision_enter)(uint32_t body_a, uint32_t body_b, ContactPoint *contact);
  void (*on_collision_exit)(uint32_t body_a, uint32_t body_b);
  void (*on_constraint_break)(uint32_t constraint_id);
} PhysicsWorld;

#endif // PHYSICS_CORE_TYPES_H
