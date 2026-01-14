#include "../Public/unified_physics.h"
#include "physics_types.h"
#include <stdlib.h>
#include <string.h>
#include <unified_logger.h>
#include <unified_memory.h>

RigidBody *rigid_body_create(BodyType type, Vec3 position) {
  RigidBody *body = UNIFIED_ALLOC(sizeof(RigidBody));
  if (!body) {
    LOG_ERROR(LOG_CAT_PHYSICS, "Failed to allocate rigid body");
    return NULL;
  }

  memset(body, 0, sizeof(RigidBody));

  body->type = type;
  body->position = position;
  body->rotation = quat_identity();
  body->mass = 1.0f;
  body->inv_mass = (type == BODY_TYPE_DYNAMIC) ? 1.0f : 0.0f;
  body->inertia = 1.0f;
  body->inv_inertia = (type == BODY_TYPE_DYNAMIC) ? 1.0f : 0.0f;
  body->restitution = 0.5f;
  body->friction = 0.5f;
  body->linear_damping = 0.01f;
  body->angular_damping = 0.01f;

  LOG_INFO(LOG_CAT_PHYSICS,
           "Rigid body created at position: (%.2f, %.2f, %.2f)", position.x,
           position.y, position.z);
  return body;
}

void rigid_body_destroy(RigidBody *body) {
  if (!body)
    return;

  if (body->collider) {
    collider_destroy(body->collider);
  }

  UNIFIED_FREE(body);
  LOG_INFO(LOG_CAT_PHYSICS, "Rigid body destroyed");
}

void rigid_body_set_mass(RigidBody *body, f32 mass) {
  if (!body || body->type != BODY_TYPE_DYNAMIC)
    return;

  body->mass = mass;
  body->inv_mass = (mass > 0.0f) ? 1.0f / mass : 0.0f;

  // Update inertia (simplified as sphere)
  f32 radius = 1.0f; // Default radius
  body->inertia = 0.4f * mass * radius * radius;
  body->inv_inertia = (body->inertia > 0.0f) ? 1.0f / body->inertia : 0.0f;
}

void rigid_body_set_friction(RigidBody *body, f32 friction) {
  if (!body)
    return;
  body->friction = clamp_f32(friction, 0.0f, 1.0f);
}

void rigid_body_set_restitution(RigidBody *body, f32 restitution) {
  if (!body)
    return;
  body->restitution = clamp_f32(restitution, 0.0f, 1.0f);
}

void rigid_body_set_linear_damping(RigidBody *body, f32 damping) {
  if (!body)
    return;
  body->linear_damping = clamp_f32(damping, 0.0f, 1.0f);
}

void rigid_body_set_angular_damping(RigidBody *body, f32 damping) {
  if (!body)
    return;
  body->angular_damping = clamp_f32(damping, 0.0f, 1.0f);
}

void rigid_body_set_position(RigidBody *body, Vec3 position) {
  if (!body)
    return;
  body->position = position;
}

Vec3 rigid_body_get_position(const RigidBody *body) {
  if (!body)
    return (Vec3){0, 0, 0};
  return body->position;
}

void rigid_body_set_rotation(RigidBody *body, Quat rotation) {
  if (!body)
    return;
  body->rotation = quat_normalize(rotation);
}

Quat rigid_body_get_rotation(const RigidBody *body) {
  if (!body)
    return quat_identity();
  return body->rotation;
}

Vec3 rigid_body_get_velocity(const RigidBody *body) {
  if (!body)
    return (Vec3){0, 0, 0};
  return body->velocity;
}

void rigid_body_set_velocity(RigidBody *body, Vec3 velocity) {
  if (!body)
    return;
  body->velocity = velocity;
}

void rigid_body_add_force(RigidBody *body, Vec3 force) {
  if (!body || body->type != BODY_TYPE_DYNAMIC)
    return;
  body->force = vec3_add(body->force, force);
}

void rigid_body_add_impulse(RigidBody *body, Vec3 impulse) {
  if (!body || body->type != BODY_TYPE_DYNAMIC)
    return;
  body->velocity = vec3_add(body->velocity, vec3_mul(impulse, body->inv_mass));
}

void rigid_body_clear_forces(RigidBody *body) {
  if (!body)
    return;
  body->force = (Vec3){0, 0, 0};
  body->torque = (Vec3){0, 0, 0};
}

f32 rigid_body_get_inv_mass(const RigidBody *body) {
  if (!body)
    return 0.0f;
  return body->inv_mass;
}

f32 rigid_body_get_restitution(const RigidBody *body) {
  if (!body)
    return 0.0f;
  return body->restitution;
}

Collider *rigid_body_get_collider(const RigidBody *body) {
  if (!body)
    return NULL;
  return body->collider;
}

bool rigid_body_is_sleeping(const RigidBody *body) {
  if (!body)
    return false;
  return body->is_sleeping;
}

void rigid_body_attach_collider(RigidBody *body, Collider *collider) {
  if (!body || !collider)
    return;

  if (body->collider) {
    collider_destroy(body->collider);
  }

  body->collider = collider;
  LOG_INFO(LOG_CAT_PHYSICS, "Collider attached to rigid body");
}
