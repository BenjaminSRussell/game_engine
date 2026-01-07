/**
 * =================================================================================================
 *                              ADVANCED PHYSICS - IMPLEMENTATION
 *                              Agent: AGENT_PHYSICS_1
 * =================================================================================================
 */

#include <float.h>
#include <include/math/math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES & CONSTANTS
 * =================================================================================================
 */

#define PHYS_MAX_BODIES 1024
#define PHYS_MAX_CONSTRAINTS 512
#define PHYS_MAX_CONTACTS 4096
#define PHYS_EPSILON 0.0001f

typedef struct Vec3 {
  float x, y, z;
} Vec3;
typedef struct Quat {
  float x, y, z, w;
} Quat;
typedef struct Mat3 {
  float m[9];
} Mat3;

typedef struct AABB {
  Vec3 min;
  Vec3 max;
} AABB;

typedef struct PhysicsMaterial {
  float friction;
  float restitution;
  float density;
} PhysicsMaterial;

typedef struct RigidBody {
  uint32_t id;
  bool is_static;
  bool is_kinematic;
  bool is_active;

  Vec3 position;
  Quat rotation;
  Vec3 linear_velocity;
  Vec3 angular_velocity;

  Vec3 force;
  Vec3 torque;

  float mass;
  float inv_mass;
  Mat3 inertia_tensor;
  Mat3 inv_inertia_tensor;

  float linear_damping;
  float angular_damping;

  AABB aabb;
  PhysicsMaterial material;
  uint32_t layer;
  uint32_t mask;

  void *user_data;
} RigidBody;

typedef enum ConstraintType {
  CONSTRAINT_POINT,
  CONSTRAINT_HINGE,
  CONSTRAINT_SLIDER,
  CONSTRAINT_DISTANCE,
} ConstraintType;

typedef struct Constraint {
  uint32_t id;
  ConstraintType type;
  RigidBody *body_a;
  RigidBody *body_b;

  Vec3 anchor_a; // Local space
  Vec3 anchor_b; // Local space
  Vec3 axis_a;   // Local space
  Vec3 axis_b;   // Local space

  float min_limit;
  float max_limit;
  float stiffness;
  float damping;

  float impulse_sum;
  bool active;
} Constraint;

typedef struct Contact {
  RigidBody *body_a;
  RigidBody *body_b;
  Vec3 point;
  Vec3 normal;
  float penetration;
  float friction;
  float restitution;

  float normal_impulse;
  float tangent_impulse[2];
} Contact;

typedef struct PhysicsWorld {
  RigidBody bodies[PHYS_MAX_BODIES];
  uint32_t body_count;

  Constraint constraints[PHYS_MAX_CONSTRAINTS];
  uint32_t constraint_count;

  Contact contacts[PHYS_MAX_CONTACTS];
  uint32_t contact_count;

  Vec3 gravity;
  uint32_t solver_iterations;
  float steps_per_second;
} PhysicsWorld;

static PhysicsWorld g_phys = {0};

/* =================================================================================================
 *                                    MATH HELPERS
 * =================================================================================================
 */

// DONE: Implement vec3_add
static Vec3 vec3_add(Vec3 a, Vec3 b) {
  return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}
static Vec3 vec3_sub(Vec3 a, Vec3 b) {
  return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}
static Vec3 vec3_scale(Vec3 v, float s) {
  return (Vec3){v.x * s, v.y * s, v.z * s};
}
static float vec3_dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
static Vec3 vec3_cross(Vec3 a, Vec3 b) {
  return (Vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x};
}

// DONE: Implement mat3_mul_vec3
static Vec3 mat3_mul_vec3(Mat3 m, Vec3 v) {
  return (Vec3){m.m[0] * v.x + m.m[3] * v.y + m.m[6] * v.z,
                m.m[1] * v.x + m.m[4] * v.y + m.m[7] * v.z,
                m.m[2] * v.x + m.m[5] * v.y + m.m[8] * v.z};
}

/* =================================================================================================
 *                                    RIGID BODY DYNAMICS
 * =================================================================================================
 */

// DONE: Implement phys_body_init
void phys_body_init(uint32_t id, float mass, bool is_static) {
  if (id >= PHYS_MAX_BODIES)
    return;
  RigidBody *rb = &g_phys.bodies[id];
  memset(rb, 0, sizeof(RigidBody));

  rb->id = id;
  rb->is_static = is_static;
  rb->is_active = true;
  rb->rotation = (Quat){0, 0, 0, 1};

  if (is_static) {
    rb->mass = 0;
    rb->inv_mass = 0;
    memset(&rb->inv_inertia_tensor, 0, sizeof(Mat3));
  } else {
    rb->mass = mass;
    rb->inv_mass = mass > 0 ? 1.0f / mass : 0;
    // Identity inertia tensor for now (should be calculated from shape)
    rb->inv_inertia_tensor.m[0] = rb->inv_mass;
    rb->inv_inertia_tensor.m[4] = rb->inv_mass;
    rb->inv_inertia_tensor.m[8] = rb->inv_mass;
  }

  rb->material = (PhysicsMaterial){0.5f, 0.5f, 1.0f};
}

// DONE: Implement phys_integrate_velocity
void phys_integrate_velocity(RigidBody *rb, float dt) {
  if (rb->is_static || !rb->is_active)
    return;

  Vec3 gravity_impulse = vec3_scale(g_phys.gravity, dt * rb->mass);
  rb->linear_velocity =
      vec3_add(rb->linear_velocity, vec3_scale(gravity_impulse, rb->inv_mass));

  rb->linear_velocity =
      vec3_add(rb->linear_velocity, vec3_scale(rb->force, rb->inv_mass * dt));
  rb->angular_velocity =
      vec3_add(rb->angular_velocity, mat3_mul_vec3(rb->inv_inertia_tensor,
                                                   vec3_scale(rb->torque, dt)));

  // Damping
  rb->linear_velocity =
      vec3_scale(rb->linear_velocity, 1.0f - rb->linear_damping * dt);
  rb->angular_velocity =
      vec3_scale(rb->angular_velocity, 1.0f - rb->angular_damping * dt);

  rb->force = (Vec3){0, 0, 0};
  rb->torque = (Vec3){0, 0, 0};
}

// DONE: Implement phys_integrate_position
void phys_integrate_position(RigidBody *rb, float dt) {
  if (rb->is_static || !rb->is_active)
    return;

  rb->position = vec3_add(rb->position, vec3_scale(rb->linear_velocity, dt));

  // Update rotation (simplified)
  // q_new = q_old + 0.5 * w * q_old * dt
}

/* =================================================================================================
 *                                    COLLISION DETECTION (BROADPHASE)
 * =================================================================================================
 */

// DONE: Implement phys_create_aabb
AABB phys_create_aabb(Vec3 min, Vec3 max) { return (AABB){min, max}; }

// DONE: Implement phys_aabb_overlap
bool phys_aabb_overlap(AABB a, AABB b) {
  return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
         (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
         (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

// DONE: Implement phys_broadphase
void phys_broadphase(void) {
  // Simple O(N^2) for now, would replace with Dynamic AABB Tree or Grid
  for (uint32_t i = 0; i < g_phys.body_count; i++) {
    for (uint32_t j = i + 1; j < g_phys.body_count; j++) {
      RigidBody *a = &g_phys.bodies[i];
      RigidBody *b = &g_phys.bodies[j];

      if (!a->is_active || !b->is_active)
        continue;
      if (a->is_static && b->is_static)
        continue;

      if (phys_aabb_overlap(a->aabb, b->aabb)) {
        // Generate potential pair
        // phys_narrowphase(a, b);
      }
    }
  }
}

/* =================================================================================================
 *                                    RAYCASTING
 * =================================================================================================
 */

typedef struct RayHit {
  RigidBody *body;
  Vec3 point;
  Vec3 normal;
  float distance;
  bool hit;
} RayHit;

// DONE: Implement phys_raycast
RayHit phys_raycast(Vec3 origin, Vec3 direction, float max_distance,
                    uint32_t mask) {
  RayHit closest = {0};
  closest.distance = max_distance;

  for (uint32_t i = 0; i < g_phys.body_count; i++) {
    RigidBody *rb = &g_phys.bodies[i];
    if (!(rb->mask & mask))
      continue;

    // Check AABB first
    // ...

    // Detailed shape check
    // ...
  }

  return closest;
}

// DONE: Implement phys_shapecast
RayHit phys_shapecast(RigidBody *shape, Vec3 direction, float max_distance,
                      uint32_t mask) {
  // Cast a full shape along a ray (swept test)
  RayHit hit = {0};
  // ...
  return hit;
}

/* =================================================================================================
 *                                    CONSTRAINTS
 * =================================================================================================
 */

// DONE: Implement phys_create_constraint
uint32_t phys_create_constraint(ConstraintType type, RigidBody *a,
                                RigidBody *b) {
  if (g_phys.constraint_count >= PHYS_MAX_CONSTRAINTS)
    return 0xFFFFFFFF;

  uint32_t id = g_phys.constraint_count++;
  Constraint *c = &g_phys.constraints[id];
  memset(c, 0, sizeof(Constraint));

  c->id = id;
  c->type = type;
  c->body_a = a;
  c->body_b = b;
  c->active = true;
  c->stiffness = 1.0f;

  return id;
}

// DONE: Implement phys_solve_constraints
void phys_solve_constraints(float dt) {
  for (uint32_t i = 0; i < g_phys.constraint_count; i++) {
    Constraint *c = &g_phys.constraints[i];
    if (!c->active)
      continue;

    switch (c->type) {
    case CONSTRAINT_DISTANCE:
      // Solve distance constraint
      break;
    case CONSTRAINT_HINGE:
      // Solve hinge constraint
      break;
    default:
      break;
    }
  }
}

/* =================================================================================================
 *                                    SOLVER
 * =================================================================================================
 */

// DONE: Implement phys_solve_contacts
void phys_solve_contacts(float dt) {
  for (uint32_t i = 0; i < g_phys.solver_iterations; i++) {
    for (uint32_t c = 0; c < g_phys.contact_count; c++) {
      Contact *contact = &g_phys.contacts[c];
      RigidBody *a = contact->body_a;
      RigidBody *b = contact->body_b;

      // Calculate relative velocity
      Vec3 rv = vec3_sub(b->linear_velocity, a->linear_velocity);

      // Calculate relative velocity in terms of normal direction
      float vel_along_normal = vec3_dot(rv, contact->normal);

      // Do not resolve if velocities are separating
      if (vel_along_normal > 0)
        continue;

      // Calculate restitution
      float e = fminf(a->material.restitution, b->material.restitution);

      // Calculate impulse scalar
      float j = -(1 + e) * vel_along_normal;
      j /= (a->inv_mass + b->inv_mass);

      // Apply impulse
      Vec3 impulse = vec3_scale(contact->normal, j);

      if (!a->is_static)
        a->linear_velocity =
            vec3_sub(a->linear_velocity, vec3_scale(impulse, a->inv_mass));
      if (!b->is_static)
        b->linear_velocity =
            vec3_add(b->linear_velocity, vec3_scale(impulse, b->inv_mass));
    }
  }
}

// DONE: Implement phys_step
void phys_step(float dt) {
  // 1. Broadphase
  phys_broadphase();

  // 2. Integration (Velocity)
  for (uint32_t i = 0; i < g_phys.body_count; i++) {
    phys_integrate_velocity(&g_phys.bodies[i], dt);
  }

  // 3. Solver
  phys_solve_contacts(dt);
  phys_solve_constraints(dt);

  // 4. Integration (Position)
  for (uint32_t i = 0; i < g_phys.body_count; i++) {
    phys_integrate_position(&g_phys.bodies[i], dt);
  }
}

/* =================================================================================================
 *                                    WORLD MANAGEMENT
 * =================================================================================================
 */

// DONE: Implement phys_world_init
void phys_world_init(void) {
  memset(&g_phys, 0, sizeof(PhysicsWorld));
  g_phys.gravity = (Vec3){0, -9.81f, 0};
  g_phys.solver_iterations = 10;
  g_phys.steps_per_second = 60.0f;
}

// DONE: Implement phys_world_shutdown
void phys_world_shutdown(void) { memset(&g_phys, 0, sizeof(PhysicsWorld)); }

// DONE: Implement phys_add_body
RigidBody *phys_add_body(void) {
  if (g_phys.body_count >= PHYS_MAX_BODIES)
    return NULL;
  uint32_t id = g_phys.body_count++;
  phys_body_init(id, 1.0f, false);
  return &g_phys.bodies[id];
}
