#ifndef PARTICLE_COMPONENT_H
#define PARTICLE_COMPONENT_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

typedef struct ParticleComponent {
  vec3 velocity;
  float lifetime;
  float max_lifetime;
  float color[3];
} ParticleComponent;

#endif // PARTICLE_COMPONENT_H
