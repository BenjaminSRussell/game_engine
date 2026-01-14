#include "../Public/unified_physics.h"
#include "physics_types.h"
#include <math.h>

bool physics_check_collision(RigidBody *a, RigidBody *b,
                             CollisionManifold *out) {
  if (!a || !b || !out)
    return false;

  if (!a->collider || !b->collider)
    return false;

  // Sphere-Sphere collision
  if (a->collider->type == COLLIDER_TYPE_SPHERE &&
      b->collider->type == COLLIDER_TYPE_SPHERE) {

    Vec3 diff = vec3_sub(b->position, a->position);
    f32 distance = vec3_length(diff);
    f32 radius_sum =
        a->collider->shape.sphere.radius + b->collider->shape.sphere.radius;

    if (distance < radius_sum) {
      out->body_a = a;
      out->body_b = b;
      out->depth = radius_sum - distance;
      out->normal =
          (distance > 0.0f) ? vec3_mul(diff, 1.0f / distance) : (Vec3){1, 0, 0};

      // Point of contact on surface of A
      out->point = vec3_add(
          a->position, vec3_mul(out->normal, a->collider->shape.sphere.radius -
                                                 out->depth * 0.5f));
      return true;
    }
  }

  // Future: Box-Box, Sphere-Box, etc.
  // Logic for other types would go here.

  return false;
}
