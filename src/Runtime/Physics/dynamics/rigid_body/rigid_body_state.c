#include <physics/core/physics_types.h>
#include <physics/physics.h>

Vec3 rigid_body_get_position(const RigidBody *body) {
  if (!body)
    return (Vec3){0};
  return (Vec3){body->position[0], body->position[1], body->position[2]};
}

Vec3 rigid_body_get_velocity(const RigidBody *body) {
  if (!body)
    return (Vec3){0};
  return (Vec3){body->velocity[0], body->velocity[1], body->velocity[2]};
}

void rigid_body_set_velocity(RigidBody *body, Vec3 velocity) {
  if (body) {
    body->velocity[0] = velocity.x;
    body->velocity[1] = velocity.y;
    body->velocity[2] = velocity.z;
  }
}

void rigid_body_set_mass(RigidBody *body, f32 mass) {
  if (body) {
    body->mass = mass;
    body->inv_mass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
  }
}

void rigid_body_set_restitution(RigidBody *body, f32 restitution) {
  if (body)
    body->restitution = restitution;
}

void rigid_body_set_friction(RigidBody *body, f32 friction) {
  if (body)
    body->friction = friction;
}

void rigid_body_set_position(RigidBody *body, Vec3 position) {
  if (body) {
    body->position[0] = position.x;
    body->position[1] = position.y;
    body->position[2] = position.z;
  }
}

void rigid_body_set_rotation(RigidBody *body, Quat rotation) {
  if (body) {
    body->rotation[0] = rotation.x;
    body->rotation[1] = rotation.y;
    body->rotation[2] = rotation.z;
    body->rotation[3] = rotation.w;
  }
}

Quat rigid_body_get_rotation(const RigidBody *body) {
  if (!body)
    return quat_identity();
  // Assuming XYZW layout matches body->rotation storage if Quat is XYZW in
  // types.h If types.h Quat is XYZW, then mapping is direct.
  return (Quat){body->rotation[0], body->rotation[1], body->rotation[2],
                body->rotation[3]};
}

void rigid_body_set_linear_damping(RigidBody *body, f32 damping) {
  if (body)
    body->linear_damping = damping;
}

void rigid_body_set_angular_damping(RigidBody *body, f32 damping) {
  if (body)
    body->angular_damping = damping;
}

f32 rigid_body_get_inv_mass(const RigidBody *body) {
  return body ? body->inv_mass : 0.0f;
}

Collider *rigid_body_get_collider(const RigidBody *body) {
  return (Collider *)(body ? body->shape : NULL);
}
