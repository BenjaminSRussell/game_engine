#include "../Public/unified_physics.h"
#include "physics_types.h"
#include <math.h>

void physics_resolve_collision(RigidBody *a, RigidBody *b,
                               CollisionManifold *m) {
  if (!a || !b || !m)
    return;

  // Calculate relative velocity
  Vec3 relative_velocity = vec3_sub(b->velocity, a->velocity);
  f32 velocity_along_normal = vec3_dot(relative_velocity, m->normal);

  // Don't resolve if velocities are separating
  if (velocity_along_normal > 0.0f)
    return;

  // Calculate restitution
  f32 restitution = fmaxf(a->restitution, b->restitution);

  // Calculate impulse scalar
  f32 impulse_scalar = -(1.0f + restitution) * velocity_along_normal;
  impulse_scalar /= a->inv_mass + b->inv_mass;

  // Apply impulse
  Vec3 impulse = vec3_mul(m->normal, impulse_scalar);

  if (a->type == BODY_TYPE_DYNAMIC) {
    a->velocity = vec3_sub(a->velocity, vec3_mul(impulse, a->inv_mass));
  }
  if (b->type == BODY_TYPE_DYNAMIC) {
    b->velocity = vec3_add(b->velocity, vec3_mul(impulse, b->inv_mass));
  }

  // Apply friction
  Vec3 tangent =
      vec3_sub(relative_velocity, vec3_mul(m->normal, velocity_along_normal));
  f32 tangent_length = vec3_length(tangent);

  if (tangent_length > 0.001f) {
    tangent = vec3_mul(tangent, 1.0f / tangent_length);
    f32 friction_impulse =
        -velocity_along_normal * fmaxf(a->friction, b->friction);

    Vec3 friction = vec3_mul(tangent, friction_impulse);

    if (a->type == BODY_TYPE_DYNAMIC) {
      a->velocity = vec3_sub(a->velocity, vec3_mul(friction, a->inv_mass));
    }
    if (b->type == BODY_TYPE_DYNAMIC) {
      b->velocity = vec3_add(b->velocity, vec3_mul(friction, b->inv_mass));
    }
  }
}
