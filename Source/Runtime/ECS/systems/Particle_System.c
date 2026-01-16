#include "../../Render/Public/Render.h"
#include "../Public/ECS.h"
#include "../components/Components_Registration.h"
#include "../components/Particle_Component.h"
#include "../components/Transform_Component.h"
#include <stdio.h>

extern ComponentType COMPONENT_PARTICLE;

void ParticleSystem_Update(float dt) {
  u32 max_entities = ECS_MAX_ENTITIES;
  for (EntityID id = 0; id < max_entities; ++id) {
    if (!ECS_IsEntityValid(id))
      continue;

    if (ECS_HasComponent(id, COMPONENT_TRANSFORM) &&
        ECS_HasComponent(id, COMPONENT_PARTICLE)) {
      TransformComponent *transform =
          (TransformComponent *)ECS_GetComponent(id, COMPONENT_TRANSFORM);
      ParticleComponent *particle =
          (ParticleComponent *)ECS_GetComponent(id, COMPONENT_PARTICLE);

      // Update Lifetime
      particle->lifetime -= dt;
      if (particle->lifetime <= 0) {
        ECS_DestroyEntity(id);
        continue;
      }

      // Basic Gravity & Motion
      particle->velocity.y -= 9.81f * dt;
      transform->position.x += particle->velocity.x * dt;
      transform->position.y += particle->velocity.y * dt;
      transform->position.z += particle->velocity.z * dt;

      // Render
      float alpha = particle->lifetime / particle->max_lifetime;
      Render_DrawCube(transform->position.x, transform->position.y,
                      transform->position.z, 0.2f, particle->color[0] * alpha,
                      particle->color[1] * alpha, particle->color[2] * alpha);
    }
  }
}

void ParticleSystem_Register(void) {
  ECS_RegisterSystem("ParticleSystem", ParticleSystem_Update, NULL, 200);
}
