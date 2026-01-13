// physics_api.c - Unified Physics API Implementation
#include "physics_api.h"
#include "../include/core/logger.h"
#include "../include/math/mat4.h"
#include "../include/math/quat.h"
#include "../include/math/vec3.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RIGID_BODIES 2048
#define MAX_SHAPES 1024
#define MAX_CONSTRAINTS 512
#define MAX_DEBUG_LINES 4096
#define GRAVITY_DEFAULT {0.0f, -9.81f, 0.0f}
#define TIMESTEP_DEFAULT (1.0f / 60.0f)
#define MAX_SUBSTEPS_DEFAULT 4

// Internal structures
typedef struct {
  uint32_t id;
  RigidBodyType type;
  CollisionShape *shape;

  // Transform
  float position[3];
  float rotation[4]; // quaternion
  float scale[3];

  // Dynamics
  float velocity[3];
  float angular_velocity[3];
  float force_accumulator[3];
  float torque_accumulator[3];

  // Properties
  float mass;
  float inverse_mass;
  float inertia[9]; // 3x3 matrix
  float inverse_inertia[9];
  float friction;
  float restitution;

  // State
  bool is_awake;
  bool is_static;
  bool is_active;

  // Collision
  uint32_t collision_layer;
  uint32_t collision_mask;

  void *user_data;
} RigidBodyInternal;

typedef struct {
  ShapeType type;
  union {
    struct {
      float radius;
    } sphere;
    struct {
      float half_extents[3];
    } box;
    struct {
      float radius;
      float height;
    } capsule;
    struct {
      float radius;
      float height;
    } cylinder;
    struct {
      float radius;
      float height;
    } cone;
    struct {
      float *vertices;
      uint32_t vertex_count;
      float *normals;
      uint32_t normal_count;
    } convex_hull;
    struct {
      float *vertices;
      uint32_t vertex_count;
      uint32_t *indices;
      uint32_t index_count;
    } mesh;
    struct {
      float *heights;
      uint32_t width;
      uint32_t depth;
      float scale[3];
    } heightfield;
    struct {
      CollisionShape **shapes;
      uint32_t shape_count;
      uint32_t shape_capacity;
      float *transforms; // 4x4 matrix for each shape
    } compound;
  } data;

  // Cached properties
  float local_inertia[9];
  float volume;
  float bounding_radius;
} CollisionShapeInternal;

typedef struct {
  RigidBodyInternal *body_a;
  RigidBodyInternal *body_b;

  // Constraint type specific data
  union {
    struct {
      float pivot_a[3];
      float pivot_b[3];
    } point_to_point;
    struct {
      float pivot_a[3];
      float pivot_b[3];
      float axis_a[3];
      float axis_b[3];
      float min_angle;
      float max_angle;
    } hinge;
    struct {
      float axis_a[3];
      float axis_b[3];
      float min_offset;
      float max_offset;
    } slider;
    struct {
      float transform_a[16]; // 4x4 matrix
      float transform_b[16]; // 4x4 matrix
    } fixed;
  } data;

  float breaking_impulse;
  bool enabled;
  bool is_active;
} ConstraintInternal;

typedef struct {
  float radius;
  float height;
  float step_height;
  float slope_angle_rad;
  float gravity;
  float fall_speed;
  float jump_speed;

  RigidBodyInternal *body;
  float velocity[3];
  bool on_ground;
  bool jumping;
} CharacterControllerInternal;

struct PhysicsWorld {
  // Configuration
  PhysicsConfig config;

  // Bodies and shapes
  RigidBodyInternal bodies[MAX_RIGID_BODIES];
  uint32_t body_count;
  CollisionShapeInternal shapes[MAX_SHAPES];
  uint32_t shape_count;

  // Constraints
  ConstraintInternal constraints[MAX_CONSTRAINTS];
  uint32_t constraint_count;

  // Character controllers
  CharacterControllerInternal controllers[64];
  uint32_t controller_count;

  // Simulation state
  float accumulated_time;
  bool is_stepping;

  // Debug drawing
  DebugLine debug_lines[MAX_DEBUG_LINES];
  uint32_t debug_line_count;
  void (*debug_drawer)(const float *from, const float *to, const float *color);

  // Performance stats
  uint32_t step_count;
  float total_step_time;
  uint32_t body_active_count;
};

// Utility functions
static void vector3_set(float *vec, float x, float y, float z) {
  vec[0] = x;
  vec[1] = y;
  vec[2] = z;
}

static void vector3_copy(float *dest, const float *src) {
  dest[0] = src[0];
  dest[1] = src[1];
  dest[2] = src[2];
}

static void vector3_add(float *result, const float *a, const float *b) {
  result[0] = a[0] + b[0];
  result[1] = a[1] + b[1];
  result[2] = a[2] + b[2];
}

static void vector3_subtract(float *result, const float *a, const float *b) {
  result[0] = a[0] - b[0];
  result[1] = a[1] - b[1];
  result[2] = a[2] - b[2];
}

static void vector3_multiply(float *result, const float *vec, float scalar) {
  result[0] = vec[0] * scalar;
  result[1] = vec[1] * scalar;
  result[2] = vec[2] * scalar;
}

static float vector3_dot(const float *a, const float *b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static float vector3_length(const float *vec) {
  return sqrtf(vector3_dot(vec, vec));
}

static void vector3_normalize(float *vec) {
  float length = vector3_length(vec);
  if (length > 1e-6f) {
    vector3_multiply(vec, vec, 1.0f / length);
  }
}

static void quaternion_identity(float *quat) {
  quat[0] = 0.0f;
  quat[1] = 0.0f;
  quat[2] = 0.0f;
  quat[3] = 1.0f;
}

static void quaternion_from_euler(float *quat, float yaw, float pitch,
                                  float roll) {
  float half_yaw = yaw * 0.5f;
  float half_pitch = pitch * 0.5f;
  float half_roll = roll * 0.5f;

  float cos_yaw = cosf(half_yaw);
  float sin_yaw = sinf(half_yaw);
  float cos_pitch = cosf(half_pitch);
  float sin_pitch = sinf(half_pitch);
  float cos_roll = cosf(half_roll);
  float sin_roll = sinf(half_roll);

  quat[0] = sin_yaw * cos_pitch * cos_roll - cos_yaw * sin_pitch * sin_roll;
  quat[1] = cos_yaw * sin_pitch * cos_roll + sin_yaw * cos_pitch * sin_roll;
  quat[2] = cos_yaw * cos_pitch * sin_roll - sin_yaw * sin_pitch * cos_roll;
  quat[3] = cos_yaw * cos_pitch * cos_roll + sin_yaw * sin_pitch * sin_roll;
}

static void quaternion_multiply(float *result, const float *a, const float *b) {
  result[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
  result[1] = a[3] * b[1] + a[1] * b[3] + a[2] * b[0] - a[0] * b[2];
  result[2] = a[3] * b[2] + a[2] * b[3] + a[0] * b[1] - a[1] * b[0];
  result[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
}

static void quaternion_rotate_vector(float *result, const float *quat,
                                     const float *vec) {
  // Using quaternion rotation formula: v' = q * v * q^-1
  float qvec[3] = {quat[0], quat[1], quat[2]};
  float cross1[3];
  float cross2[3];

  // cross1 = qvec x vec
  cross1[0] = qvec[1] * vec[2] - qvec[2] * vec[1];
  cross1[1] = qvec[2] * vec[0] - qvec[0] * vec[2];
  cross1[2] = qvec[0] * vec[1] - qvec[1] * vec[0];

  // cross2 = qvec x cross1
  cross2[0] = qvec[1] * cross1[2] - qvec[2] * cross1[1];
  cross2[1] = qvec[2] * cross1[0] - qvec[0] * cross1[2];
  cross2[2] = qvec[0] * cross1[1] - qvec[1] * cross1[0];

  // result = vec + 2.0 * (quat[3] * cross1 + cross2)
  result[0] = vec[0] + 2.0f * (quat[3] * cross1[0] + cross2[0]);
  result[1] = vec[1] + 2.0f * (quat[3] * cross1[1] + cross2[1]);
  result[2] = vec[2] + 2.0f * (quat[3] * cross1[2] + cross2[2]);
}

// World Management
PhysicsWorld *physics_world_create(PhysicsConfig config) {
  PhysicsWorld *world = malloc(sizeof(PhysicsWorld));
  if (!world)
    return NULL;

  memset(world, 0, sizeof(PhysicsWorld));

  // Set default config if not provided
  if (config.gravity[0] == 0.0f && config.gravity[1] == 0.0f &&
      config.gravity[2] == 0.0f) {
    vector3_set(world->config.gravity, 0.0f, -9.81f, 0.0f);
  } else {
    vector3_copy(world->config.gravity, config.gravity);
  }

  world->config.timestep =
      (config.timestep > 0.0f) ? config.timestep : TIMESTEP_DEFAULT;
  world->config.max_substeps =
      (config.max_substeps > 0) ? config.max_substeps : MAX_SUBSTEPS_DEFAULT;
  world->config.enable_debug_draw = config.enable_debug_draw;

  log_info("Physics world created with gravity: (%.2f, %.2f, %.2f)",
           world->config.gravity[0], world->config.gravity[1],
           world->config.gravity[2]);

  return world;
}

void physics_world_destroy(PhysicsWorld *world) {
  if (!world)
    return;

  // Destroy all bodies
  for (uint32_t i = 0; i < world->body_count; i++) {
    if (world->bodies[i].shape) {
      physics_shape_destroy(world->bodies[i].shape);
    }
  }

  // Destroy all shapes
  for (uint32_t i = 0; i < world->shape_count; i++) {
    CollisionShapeInternal *shape = &world->shapes[i];
    switch (shape->type) {
    case SHAPE_CONVEX_HULL:
      free(shape->data.convex_hull.vertices);
      free(shape->data.convex_hull.normals);
      break;
    case SHAPE_MESH:
      free(shape->data.mesh.vertices);
      free(shape->data.mesh.indices);
      break;
    case SHAPE_HEIGHTFIELD:
      free(shape->data.heightfield.heights);
      break;
    case SHAPE_COMPOUND:
      free(shape->data.compound.shapes);
      free(shape->data.compound.transforms);
      break;
    default:
      break;
    }
  }

  free(world);
  log_info("Physics world destroyed");
}

void physics_world_step(PhysicsWorld *world, float dt) {
  if (!world || world->is_stepping)
    return;

  world->is_stepping = true;
  float start_time = 0.0f; // TODO: Add timing

  // Fixed timestep with accumulation
  world->accumulated_time += dt;

  int substeps = 0;
  while (world->accumulated_time >= world->config.timestep &&
         substeps < world->config.max_substeps) {
    // Integrate forces
    for (uint32_t i = 0; i < world->body_count; i++) {
      RigidBodyInternal *body = &world->bodies[i];
      if (!body->is_active || body->is_static)
        continue;

      // Apply gravity
      if (body->mass > 0.0f) {
        float gravity_force[3];
        vector3_multiply(gravity_force, world->config.gravity, body->mass);
        vector3_add(body->force_accumulator, body->force_accumulator,
                    gravity_force);
      }

      // Update velocity (F = ma, a = F/m, v = v0 + a*dt)
      float acceleration[3];
      vector3_multiply(acceleration, body->force_accumulator,
                       body->inverse_mass);
      vector3_multiply(acceleration, acceleration, world->config.timestep);
      vector3_add(body->velocity, body->velocity, acceleration);

      // Update angular velocity
      // TODO: Apply torque with inertia tensor

      // Clear force accumulators
      vector3_set(body->force_accumulator, 0.0f, 0.0f, 0.0f);
      vector3_set(body->torque_accumulator, 0.0f, 0.0f, 0.0f);
    }

    // Integrate velocities
    for (uint32_t i = 0; i < world->body_count; i++) {
      RigidBodyInternal *body = &world->bodies[i];
      if (!body->is_active || body->is_static)
        continue;

      // Update position
      float displacement[3];
      vector3_multiply(displacement, body->velocity, world->config.timestep);
      vector3_add(body->position, body->position, displacement);

      // TODO: Update rotation with angular velocity
    }

    // Solve constraints
    for (uint32_t i = 0; i < world->constraint_count; i++) {
      ConstraintInternal *constraint = &world->constraints[i];
      if (!constraint->enabled || !constraint->is_active)
        continue;

      // TODO: Implement constraint solving
    }

    // Update character controllers
    for (uint32_t i = 0; i < world->controller_count; i++) {
      CharacterControllerInternal *controller = &world->controllers[i];
      if (!controller->body)
        continue;

      // Apply gravity to controller
      if (!controller->on_ground && controller->gravity != 0.0f) {
        controller->velocity[1] -= controller->gravity * world->config.timestep;
        if (controller->velocity[1] < -controller->fall_speed) {
          controller->velocity[1] = -controller->fall_speed;
        }
      }

      // Update controller body velocity
      physics_body_set_velocity((RigidBody *)controller->body,
                                controller->velocity);
    }

    world->accumulated_time -= world->config.timestep;
    substeps++;
  }

  // Update performance stats
  world->step_count++;
  world->total_step_time += 0.0f; // TODO: Add timing

  // Count active bodies
  world->body_active_count = 0;
  for (uint32_t i = 0; i < world->body_count; i++) {
    if (world->bodies[i].is_active) {
      world->body_active_count++;
    }
  }

  world->is_stepping = false;
}

void physics_world_set_gravity(PhysicsWorld *world, const float *gravity) {
  if (!world || !gravity)
    return;
  vector3_copy(world->config.gravity, gravity);
}

void physics_world_get_gravity(PhysicsWorld *world, float *out_gravity) {
  if (!world || !out_gravity)
    return;
  vector3_copy(out_gravity, world->config.gravity);
}

// Rigid Body Management
RigidBody *physics_body_create(uint32_t id, RigidBodyType type,
                               CollisionShape *shape) {
  // Note: This function has a design issue - we don't have a world parameter.
  // For now, we'll return NULL and require users to use physics_world_add_body
  // instead. A properly created body needs a world context.
  log_warn("physics_body_create called without world context - use "
           "physics_world_add_body instead");
  return NULL;
}

void physics_body_destroy(RigidBody *body) {
  if (!body)
    return;

  RigidBodyInternal *internal = (RigidBodyInternal *)body;

  // Clean up shape reference (don't free - shape might be shared)
  internal->shape = NULL;

  // Mark as inactive
  internal->is_active = false;
  internal->is_awake = false;

  log_debug("Destroyed rigid body ID: %u", internal->id);
}

// Shape Management
CollisionShape *physics_shape_sphere_create(float radius) {
  // TODO: Implement sphere shape creation
  return NULL;
}

CollisionShape *physics_shape_box_create(float half_extents[3]) {
  // TODO: Implement box shape creation
  return NULL;
}

// Public API utility functions
void physics_vector3_set(float *vec, float x, float y, float z) {
  vector3_set(vec, x, y, z);
}

void physics_vector3_copy(float *dest, const float *src) {
  vector3_copy(dest, src);
}

void physics_vector3_add(float *result, const float *a, const float *b) {
  vector3_add(result, a, b);
}

void physics_vector3_subtract(float *result, const float *a, const float *b) {
  vector3_subtract(result, a, b);
}

void physics_vector3_multiply(float *result, const float *vec, float scalar) {
  vector3_multiply(result, vec, scalar);
}

void physics_vector3_cross(float *result, const float *a, const float *b) {
  result[0] = a[1] * b[2] - a[2] * b[1];
  result[1] = a[2] * b[0] - a[0] * b[2];
  result[2] = a[0] * b[1] - a[1] * b[0];
}

float physics_vector3_dot(const float *a, const float *b) {
  return vector3_dot(a, b);
}

float physics_vector3_length(const float *vec) { return vector3_length(vec); }

void physics_vector3_normalize(float *vec) { vector3_normalize(vec); }

void physics_quaternion_identity(float *quat) { quaternion_identity(quat); }

void physics_quaternion_from_euler(float *quat, float yaw, float pitch,
                                   float roll) {
  quaternion_from_euler(quat, yaw, pitch, roll);
}

void physics_quaternion_multiply(float *result, const float *a,
                                 const float *b) {
  quaternion_multiply(result, a, b);
}

void physics_quaternion_rotate_vector(float *result, const float *quat,
                                      const float *vec) {
  quaternion_rotate_vector(result, quat, vec);
}

// Body property implementations
void physics_body_set_position(RigidBody *body, const float *position) {
  if (!body || !position)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  vector3_copy(internal->position, position);
}

void physics_body_set_rotation(RigidBody *body, const float *rotation) {
  if (!body || !rotation)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  // Rotation is quaternion [x, y, z, w]
  internal->rotation[0] = rotation[0];
  internal->rotation[1] = rotation[1];
  internal->rotation[2] = rotation[2];
  internal->rotation[3] = rotation[3];
}

void physics_body_set_velocity(RigidBody *body, const float *velocity) {
  if (!body || !velocity)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  vector3_copy(internal->velocity, velocity);
}

void physics_body_set_angular_velocity(RigidBody *body,
                                       const float *angular_velocity) {
  if (!body || !angular_velocity)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  vector3_copy(internal->angular_velocity, angular_velocity);
}

void physics_body_get_position(RigidBody *body, float *out_position) {
  if (!body || !out_position)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  vector3_copy(out_position, internal->position);
}

void physics_body_get_rotation(RigidBody *body, float *out_rotation) {
  if (!body || !out_rotation)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  out_rotation[0] = internal->rotation[0];
  out_rotation[1] = internal->rotation[1];
  out_rotation[2] = internal->rotation[2];
  out_rotation[3] = internal->rotation[3];
}

void physics_body_get_velocity(RigidBody *body, float *out_velocity) {
  if (!body || !out_velocity)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  vector3_copy(out_velocity, internal->velocity);
}

void physics_body_get_angular_velocity(RigidBody *body,
                                       float *out_angular_velocity) {
  if (!body || !out_angular_velocity)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  vector3_copy(out_angular_velocity, internal->angular_velocity);
}

void physics_body_apply_force(RigidBody *body, const float *force) {
  if (!body || !force)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  vector3_add(internal->force_accumulator, internal->force_accumulator, force);
}

void physics_body_apply_impulse(RigidBody *body, const float *impulse) {
  if (!body || !impulse)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  // J = m * Δv => Δv = J / m
  if (internal->inverse_mass > 0.0f) {
    float delta_v[3];
    vector3_multiply(delta_v, impulse, internal->inverse_mass);
    vector3_add(internal->velocity, internal->velocity, delta_v);
  }
}

void physics_body_apply_torque(RigidBody *body, const float *torque) {
  if (!body || !torque)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  vector3_add(internal->torque_accumulator, internal->torque_accumulator,
              torque);
}

void physics_body_set_mass(RigidBody *body, float mass) {
  if (!body || mass <= 0.0f)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  internal->mass = mass;
  internal->inverse_mass = 1.0f / mass;
}

float physics_body_get_mass(RigidBody *body) {
  if (!body)
    return 0.0f;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  return internal->mass;
}

void physics_body_set_friction(RigidBody *body, float friction) {
  if (!body)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  internal->friction = friction;
}

void physics_body_set_restitution(RigidBody *body, float restitution) {
  if (!body)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  internal->restitution = restitution;
}

void physics_body_set_user_data(RigidBody *body, void *user_data) {
  if (!body)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  internal->user_data = user_data;
}

void *physics_body_get_user_data(RigidBody *body) {
  if (!body)
    return NULL;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;
  return internal->user_data;
}

void physics_body_get_aabb(RigidBody *body, float *min_out, float *max_out) {
  if (!body || !min_out || !max_out)
    return;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;

  // Simple AABB based on shape type
  if (!internal->shape) {
    // No shape - point AABB
    vector3_copy(min_out, internal->position);
    vector3_copy(max_out, internal->position);
    return;
  }

  CollisionShapeInternal *shape = (CollisionShapeInternal *)internal->shape;
  float radius = shape->bounding_radius;

  // Conservative AABB using bounding radius
  min_out[0] = internal->position[0] - radius;
  min_out[1] = internal->position[1] - radius;
  min_out[2] = internal->position[2] - radius;
  max_out[0] = internal->position[0] + radius;
  max_out[1] = internal->position[1] + radius;
  max_out[2] = internal->position[2] + radius;
}

bool physics_body_get_velocity_at_point(RigidBody *body, const float *point,
                                        float *out_vel) {
  if (!body || !point || !out_vel)
    return false;
  RigidBodyInternal *internal = (RigidBodyInternal *)body;

  // v = v_linear + ω × r
  // where r = point - center_of_mass
  float r[3];
  vector3_subtract(r, point, internal->position);

  float angular_contrib[3];
  angular_contrib[0] = internal->angular_velocity[1] * r[2] -
                       internal->angular_velocity[2] * r[1];
  angular_contrib[1] = internal->angular_velocity[2] * r[0] -
                       internal->angular_velocity[0] * r[2];
  angular_contrib[2] = internal->angular_velocity[0] * r[1] -
                       internal->angular_velocity[1] * r[0];

  vector3_add(out_vel, internal->velocity, angular_contrib);
  return true;
}

CollisionShape *physics_shape_capsule_create(float radius, float height) {
  return NULL; /* TODO */
}
CollisionShape *physics_shape_cylinder_create(float radius, float height) {
  return NULL; /* TODO */
}
CollisionShape *physics_shape_cone_create(float radius, float height) {
  return NULL; /* TODO */
}
CollisionShape *physics_shape_convex_hull_create(float *vertices,
                                                 uint32_t vertex_count) {
  return NULL; /* TODO */
}
CollisionShape *physics_shape_mesh_create(float *vertices,
                                          uint32_t vertex_count,
                                          uint32_t *indices,
                                          uint32_t index_count) {
  return NULL; /* TODO */
}
CollisionShape *physics_shape_heightfield_create(float *heights, uint32_t width,
                                                 uint32_t depth,
                                                 float scale[3]) {
  return NULL; /* TODO */
}
CollisionShape *physics_shape_compound_create(void) { return NULL; /* TODO */ }
void physics_shape_destroy(CollisionShape *shape) { /* TODO */ }
void physics_shape_compound_add_shape(CollisionShape *compound,
                                      CollisionShape *shape,
                                      float *transform) { /* TODO */ }
void physics_shape_calculate_local_inertia(CollisionShape *shape, float mass,
                                           float *inertia) { /* TODO */ }
ShapeType physics_shape_get_type(CollisionShape *shape) {
  return SHAPE_SPHERE; /* TODO */
}

RigidBody *physics_world_add_body(PhysicsWorld *world, RigidBody *body) {
  return NULL; /* TODO */
}
void physics_world_remove_body(PhysicsWorld *world,
                               RigidBody *body) { /* TODO */ }
bool physics_world_raycast(PhysicsWorld *world, const float *origin,
                           const float *direction, float max_distance,
                           RayHit *out_hit) {
  return false; /* TODO */
}
void physics_world_query_aabb(PhysicsWorld *world, const float *min_aabb,
                              const float *max_aabb, RigidBody **out_bodies,
                              uint32_t *out_count,
                              uint32_t max_count) { /* TODO */ }

Constraint *physics_constraint_point_to_point_create(RigidBody *body_a,
                                                     RigidBody *body_b,
                                                     const float *pivot_a,
                                                     const float *pivot_b) {
  return NULL; /* TODO */
}
Constraint *
physics_constraint_hinge_create(RigidBody *body_a, RigidBody *body_b,
                                const float *pivot_a, const float *pivot_b,
                                const float *axis_a, const float *axis_b) {
  return NULL; /* TODO */
}
Constraint *physics_constraint_slider_create(RigidBody *body_a,
                                             RigidBody *body_b,
                                             const float *axis_a,
                                             const float *axis_b) {
  return NULL; /* TODO */
}
Constraint *physics_constraint_fixed_create(RigidBody *body_a,
                                            RigidBody *body_b,
                                            const float *transform_a,
                                            const float *transform_b) {
  return NULL; /* TODO */
}
void physics_constraint_destroy(Constraint *constraint) { /* TODO */ }
void physics_constraint_set_enabled(Constraint *constraint,
                                    bool enabled) { /* TODO */ }
void physics_constraint_set_breaking_impulse(Constraint *constraint,
                                             float threshold) { /* TODO */ }
bool physics_constraint_is_enabled(Constraint *constraint) {
  return false; /* TODO */
}

CharacterController *physics_character_controller_create(float radius,
                                                         float height,
                                                         float step_height) {
  return NULL; /* TODO */
}
void physics_character_controller_destroy(
    CharacterController *controller) { /* TODO */ }
void physics_character_controller_set_world(CharacterController *controller,
                                            PhysicsWorld *world) { /* TODO */ }
void physics_character_controller_set_position(
    CharacterController *controller, const float *position) { /* TODO */ }
void physics_character_controller_get_position(CharacterController *controller,
                                               float *out_position) { /* TODO */
}
void physics_character_controller_set_velocity(
    CharacterController *controller, const float *velocity) { /* TODO */ }
void physics_character_controller_get_velocity(CharacterController *controller,
                                               float *out_velocity) { /* TODO */
}
void physics_character_controller_set_gravity(CharacterController *controller,
                                              float gravity) { /* TODO */ }
void physics_character_controller_set_fall_speed(
    CharacterController *controller, float fall_speed) { /* TODO */ }
void physics_character_controller_set_jump_speed(
    CharacterController *controller, float jump_speed) { /* TODO */ }
void physics_character_controller_set_slope_angle(
    CharacterController *controller, float slope_angle) { /* TODO */ }
bool physics_character_controller_on_ground(CharacterController *controller) {
  return false; /* TODO */
}
void physics_character_controller_jump(
    CharacterController *controller) { /* TODO */ }
void physics_character_controller_move(CharacterController *controller,
                                       const float *direction,
                                       float dt) { /* TODO */ }

void physics_world_debug_draw(PhysicsWorld *world, DebugLine **out_lines,
                              uint32_t *out_count) { /* TODO */ }
void physics_world_set_debug_drawer(
    PhysicsWorld *world, void (*draw_line)(const float *from, const float *to,
                                           const float *color)) { /* TODO */ }
