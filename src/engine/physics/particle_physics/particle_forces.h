/**
 * @file particle_forces.h
 * @brief Particle force simulation
 *
 * Basic force system for particle simulation.
 */

#ifndef RENDER_PARTICLE_FORCES_H
#define RENDER_PARTICLE_FORCES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Particle Particle;
typedef struct ParticleForceSystem ParticleForceSystem;

typedef enum ParticleForceType {
  PARTICLE_FORCE_GRAVITY,
  PARTICLE_FORCE_WIND,
  PARTICLE_FORCE_DRAG,
  PARTICLE_FORCE_POINT,
  PARTICLE_FORCE_VORTEX,
  PARTICLE_FORCE_TURBULENCE
} ParticleForceType;

typedef struct ParticleForce {
  uint32_t id;
  ParticleForceType type;
  float strength;
  float direction[3];
  float position[3];
  float radius;
  float falloff;
  float drag;
  float noise_scale;
  bool enabled;
} ParticleForce;

ParticleForceSystem *particle_forces_create(int max_forces);
void particle_forces_destroy(ParticleForceSystem *system);

int particle_forces_add(ParticleForceSystem *system, const ParticleForce *force);
bool particle_forces_remove(ParticleForceSystem *system, int force_id);
void particle_forces_update(ParticleForceSystem *system, int force_id,
                            const ParticleForce *force);
ParticleForce *particle_forces_get(ParticleForceSystem *system, int force_id);
void particle_forces_clear(ParticleForceSystem *system);

void particle_forces_apply(ParticleForceSystem *system, Particle *particles,
                           int particle_count, float dt);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_PARTICLE_FORCES_H */
