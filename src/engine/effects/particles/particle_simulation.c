#include "particle_simulation.h"
// effects/particles/particle_simulation.c
// GPU-Driven Particle Simulation and Niagara-like system implementation.
//
// TODO: Implement Compute-Shader based particle simulation (Niagara
// integration).
// TODO: Add support for particle-to-particle collision and fluid-like behavior
// (SPH).
// TODO: Implement Curl-Noise and Vector-Field driven motion.
// TODO: Add support for particle-lights (dynamic light injection per particle).
// TODO: Implement Ribbon-rendering (trails) using geometry shaders or compute
// path.
// TODO: Add support for GPU-driven sorting (Bitonic sort) for alpha blending.
// TODO: Implement Mesh-Particles (instanced geometry) with individual
// orientation.
// TODO: Add support for programmable event-triggers (Spawn on death, Spawn on
// collision).
// TODO: Implement distance-based LOD (culling/coarsening) for massive emitter
// counts.
// TODO: Add support for texture-driven particle attributes (Color-over-life,
// Size-over-life).
// TODO: Implement a robust GPU-buffer pooling system for dynamic emitters.

#include <common.h>

void particle_simulate_cpu(particle_t *particles, uint32_t count, float dt,
                           vec3_t gravity, float drag) {
  if (!particles || count == 0)
    return;

  for (uint32_t i = 0; i < count; i++) {
    particle_t *p = &particles[i];

    if (p->age >= p->lifetime)
      continue;

    // Accumulate age
    p->age += dt;

    // Euler Integration
    // velocity += (gravity - drag * velocity) * dt
    p->velocity.x += (gravity.x - drag * p->velocity.x) * dt;
    p->velocity.y += (gravity.y - drag * p->velocity.y) * dt;
    p->velocity.z += (gravity.z - drag * p->velocity.z) * dt;

    // position += velocity * dt
    p->position.x += p->velocity.x * dt;
    p->position.y += p->velocity.y * dt;
    p->position.z += p->velocity.z * dt;

    // Update other properties (e.g. rotation)
    p->rotation += p->rotation_speed * dt;
  }
}
