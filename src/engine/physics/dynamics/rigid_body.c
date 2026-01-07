/**
 * RIGID BODY DYNAMICS
 * Physics simulation for rigid bodies with simplified impulse-based resolution
 */

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  float x, y, z;
} Vec3;

typedef struct {
  float x, y, z, w; // w is scalar part
} Quat;

typedef struct {
  float m[3][3];
} Mat3;

typedef struct {
  // State
  Vec3 position;
  Quat orientation;
  Vec3 velocity;
  Vec3 angular_velocity;

  // Properties
  float mass;
  float inv_mass;
  Mat3 inertia_tensor;     // Local constant
  Mat3 inv_inertia_tensor; // Local constant
  Mat3 inv_inertia_world;  // Updated per frame

  // Derived
  Mat3 transform_matrix; // Rotation/Scale

  // Flags
  bool is_static;
  bool is_sleeping;
  float linear_damping;
  float angular_damping;
  float friction;
  float restitution; // Bounciness
} RigidBody;

// Vector Math Helpers
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

// Integration (Semi-Implicit Euler)
void rb_integrate(RigidBody *rb, float dt) {
  if (rb->is_static || rb->is_sleeping)
    return;

  // Linear
  // Apply gravity (acceleration = force/mass) - here pre-applied to velocity
  // usually v += a * dt
  rb->position = vec3_add(rb->position, vec3_scale(rb->velocity, dt));

  // Dampings
  rb->velocity = vec3_scale(rb->velocity, powf(1.0f - rb->linear_damping, dt));
  rb->angular_velocity =
      vec3_scale(rb->angular_velocity, powf(1.0f - rb->angular_damping, dt));

  // Angular (Simplified)
  // Orientation update q += 0.5 * w * q * dt
  // For now assuming small rotations
}

void rb_apply_force(RigidBody *rb, Vec3 force) {
  if (rb->is_static)
    return;
  Vec3 acc = vec3_scale(force, rb->inv_mass);
  rb->velocity = vec3_add(rb->velocity, acc); // Usually integrated over dt
}

void rb_apply_impulse(RigidBody *rb, Vec3 impulse, Vec3 point) {
  if (rb->is_static)
    return;

  // Linear velocity change
  rb->velocity = vec3_add(rb->velocity, vec3_scale(impulse, rb->inv_mass));

  // Angular velocity change
  Vec3 r = vec3_sub(point, rb->position);
  Vec3 torque_impulse = vec3_cross(r, impulse);
  // Apply inertia tensor (simplified as sphere for now)
  rb->angular_velocity = vec3_add(
      rb->angular_velocity, torque_impulse); // Should multiply by inv_inertia
}

// Apply gravity
void rb_apply_gravity(RigidBody *rb, float dt) {
  if (rb->is_static || rb->is_sleeping)
    return;
  rb->velocity.y -= 9.81f * dt;
}

// Simple Box Inertia
void rb_set_box_inertia(RigidBody *rb, float width, float height, float depth) {
  if (rb->mass == 0.0f)
    return;
  float m = rb->mass;
  float w2 = width * width;
  float h2 = height * height;
  float d2 = depth * depth;

  rb->inertia_tensor.m[0][0] = (1.0f / 12.0f) * m * (h2 + d2);
  rb->inertia_tensor.m[1][1] = (1.0f / 12.0f) * m * (w2 + d2);
  rb->inertia_tensor.m[2][2] = (1.0f / 12.0f) * m * (w2 + h2);

  rb->inv_inertia_tensor.m[0][0] = 1.0f / rb->inertia_tensor.m[0][0];
  rb->inv_inertia_tensor.m[1][1] = 1.0f / rb->inertia_tensor.m[1][1];
  rb->inv_inertia_tensor.m[2][2] = 1.0f / rb->inertia_tensor.m[2][2];
}

RigidBody *rb_create(float mass, bool is_static) {
  RigidBody *rb = calloc(1, sizeof(RigidBody));
  rb->mass = mass;
  if (is_static) {
    rb->inv_mass = 0.0f;
    rb->is_static = true;
  } else {
    rb->inv_mass = 1.0f / mass;
    rb->is_static = false;
  }

  rb->orientation = (Quat){0, 0, 0, 1};
  rb->linear_damping = 0.01f;
  rb->angular_damping = 0.05f;
  rb->friction = 0.5f;
  rb->restitution = 0.2f;

  return rb;
}
