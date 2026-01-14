#include <physics/core/physics_types.h>
#include <physics/physics.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

static _Atomic uint32_t global_body_id_counter = 1;

RigidBody *rigid_body_create(BodyType type, Vec3 position) {
  RigidBody *body = (RigidBody *)calloc(1, sizeof(RigidBody));
  if (!body)
    return NULL;

  body->id = atomic_fetch_add(&global_body_id_counter, 1);
  body->type =
      (type == BODY_TYPE_STATIC) ? RIGID_BODY_STATIC : RIGID_BODY_DYNAMIC;

  body->position[0] = position.x;
  body->position[1] = position.y;
  body->position[2] = position.z;

  body->mass = 1.0f;
  body->inv_mass = 1.0f;
  body->friction = 0.5f;
  body->restitution = 0.5f;
  body->is_active = true;
  body->rotation[3] =
      1.0f; // Identity quaternion (w=1 at index 3 if XYZW layout assumed)

  if (type == BODY_TYPE_STATIC) {
    body->inv_mass = 0.0f;
    body->mass = 0.0f;
  }

  return body;
}

void rigid_body_destroy(RigidBody *body) { free(body); }

void rigid_body_attach_collider(RigidBody *body, Collider *collider) {
  if (body)
    body->shape = (CollisionShape *)collider;
}
