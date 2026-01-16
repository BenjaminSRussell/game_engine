#include "../../Physics/Public/Physics.h"
#include "../../Render/Public/Render.h"
#include "../Public/ECS.h"
#include "../components/Components_Registration.h"
#include "../components/Physics_Component.h"
#include "../components/Transform_Component.h"
#include <math.h>
#include <stdio.h>

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

void PhysicsSystem_Update(float dt) {
  // Semi-Implicit Euler Integration for all entities with Physics & Transform
  u32 max_entities = ECS_MAX_ENTITIES;

  // First pass: Apply forces and integrate
  for (EntityID id = 0; id < max_entities; ++id) {
    if (!ECS_IsEntityValid(id))
      continue;

    bool has_transform = ECS_HasComponent(id, COMPONENT_TRANSFORM);
    bool has_physics = ECS_HasComponent(id, COMPONENT_PHYSICS);

    if (has_transform && has_physics) {
      TransformComponent *transform =
          (TransformComponent *)ECS_GetComponent(id, COMPONENT_TRANSFORM);
      PhysicsComponent *physics =
          (PhysicsComponent *)ECS_GetComponent(id, COMPONENT_PHYSICS);

      if (!physics->active)
        continue;

      // v += g * dt (Gravity)
      float gravity[3] = {0.0f, -9.81f, 0.0f};
      physics->velocity.x += gravity[0] * dt;
      physics->velocity.y += gravity[1] * dt;
      physics->velocity.z += gravity[2] * dt;

      // p += v * dt (Position integration)
      transform->position.x += physics->velocity.x * dt;
      transform->position.y += physics->velocity.y * dt;
      transform->position.z += physics->velocity.z * dt;

      // Rotation integration
      transform->rotation.x += physics->angular_velocity.x * dt;
      transform->rotation.y += physics->angular_velocity.y * dt;
      transform->rotation.z += physics->angular_velocity.z * dt;

      // Floor collision (Simple bounce)
      if (transform->position.y - physics->radius < 0.0f) {
        transform->position.y = physics->radius;
        physics->velocity.y = -physics->velocity.y * physics->restitution;

        // Add spin on bounce
        physics->angular_velocity.x += physics->velocity.x * 0.5f;
        physics->angular_velocity.z += physics->velocity.z * 0.5f;

        // Damping
        if (fabsf(physics->velocity.y) < 0.5f) {
          physics->velocity.y = 0.0f;
          physics->angular_velocity.x *= 0.95f;
          physics->angular_velocity.z *= 0.95f;
        }
      }
    }
  }

  // Second pass: Collision detection between objects
  for (EntityID id1 = 0; id1 < max_entities; ++id1) {
    if (!ECS_IsEntityValid(id1))
      continue;
    if (!ECS_HasComponent(id1, COMPONENT_PHYSICS))
      continue;

    TransformComponent *t1 =
        (TransformComponent *)ECS_GetComponent(id1, COMPONENT_TRANSFORM);
    PhysicsComponent *p1 =
        (PhysicsComponent *)ECS_GetComponent(id1, COMPONENT_PHYSICS);

    if (!p1->active)
      continue;

    for (EntityID id2 = id1 + 1; id2 < max_entities; ++id2) {
      if (!ECS_IsEntityValid(id2))
        continue;
      if (!ECS_HasComponent(id2, COMPONENT_PHYSICS))
        continue;

      TransformComponent *t2 =
          (TransformComponent *)ECS_GetComponent(id2, COMPONENT_TRANSFORM);
      PhysicsComponent *p2 =
          (PhysicsComponent *)ECS_GetComponent(id2, COMPONENT_PHYSICS);

      if (!p2->active)
        continue;

      // Sphere-sphere collision detection
      float dx = t2->position.x - t1->position.x;
      float dy = t2->position.y - t1->position.y;
      float dz = t2->position.z - t1->position.z;
      float dist_sq = dx * dx + dy * dy + dz * dz;
      float min_dist = p1->radius + p2->radius;

      if (dist_sq < min_dist * min_dist && dist_sq > 0.001f) {
        // Collision detected!
        float dist = sqrtf(dist_sq);
        float nx = dx / dist;
        float ny = dy / dist;
        float nz = dz / dist;

        // Separate objects
        float overlap = min_dist - dist;
        float mass_sum = p1->mass + p2->mass;
        float ratio1 = p2->mass / mass_sum;
        float ratio2 = p1->mass / mass_sum;

        t1->position.x -= nx * overlap * ratio1;
        t1->position.y -= ny * overlap * ratio1;
        t1->position.z -= nz * overlap * ratio1;

        t2->position.x += nx * overlap * ratio2;
        t2->position.y += ny * overlap * ratio2;
        t2->position.z += nz * overlap * ratio2;

        // Relative velocity
        float dvx = p2->velocity.x - p1->velocity.x;
        float dvy = p2->velocity.y - p1->velocity.y;
        float dvz = p2->velocity.z - p1->velocity.z;

        float vel_along_normal = dvx * nx + dvy * ny + dvz * nz;

        // Don't resolve if velocities are separating
        if (vel_along_normal < 0) {
          float restitution = (p1->restitution + p2->restitution) * 0.5f;
          float impulse = -(1.0f + restitution) * vel_along_normal / mass_sum;

          p1->velocity.x -= impulse * p2->mass * nx;
          p1->velocity.y -= impulse * p2->mass * ny;
          p1->velocity.z -= impulse * p2->mass * nz;

          p2->velocity.x += impulse * p1->mass * nx;
          p2->velocity.y += impulse * p1->mass * ny;
          p2->velocity.z += impulse * p1->mass * nz;

          // Add angular velocity (simple torque approximation)
          p1->angular_velocity.x += ny * impulse * 0.1f;
          p1->angular_velocity.y -= nx * impulse * 0.1f;
          p2->angular_velocity.x -= ny * impulse * 0.1f;
          p2->angular_velocity.y += nx * impulse * 0.1f;
        }
      }
    }
  }
}

void PhysicsSystem_Register(void) {
  ECS_RegisterSystem("PhysicsSystem", PhysicsSystem_Update, NULL, 100);
}
