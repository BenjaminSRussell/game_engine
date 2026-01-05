/**
 * PARTICLE SYSTEM - GPU ACCELERATED
 * Additional rendering system
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float position[3];
  float velocity[3];
  float color[4];
  float size;
  float lifetime;
  float age;
  bool active;
} Particle;

typedef struct {
  Particle *particles;
  int particle_count;
  int max_particles;
  float emission_rate;
  float time_accumulator;

  float spawn_position[3];
  float spawn_velocity[3];
  float spawn_velocity_variance[3];
  float gravity[3];

  unsigned int vertex_buffer;
  unsigned int texture;
} ParticleSystem;

// Create particle system
ParticleSystem *particle_system_create(int max_particles) {
  ParticleSystem *sys = (ParticleSystem *)calloc(1, sizeof(ParticleSystem));
  sys->max_particles = max_particles;
  sys->particles = (Particle *)calloc(max_particles, sizeof(Particle));
  sys->emission_rate = 100.0f;
  sys->gravity[1] = -9.8f;
  return sys;
}

// Emit particle
void particle_emit(ParticleSystem *sys) {
  for (int i = 0; i < sys->max_particles; i++) {
    if (!sys->particles[i].active) {
      Particle *p = &sys->particles[i];

      memcpy(p->position, sys->spawn_position, sizeof(float) * 3);

      for (int j = 0; j < 3; j++) {
        float variance = ((float)rand() / RAND_MAX * 2.0f - 1.0f) *
                         sys->spawn_velocity_variance[j];
        p->velocity[j] = sys->spawn_velocity[j] + variance;
      }

      p->color[0] = 1.0f;
      p->color[1] = 1.0f;
      p->color[2] = 1.0f;
      p->color[3] = 1.0f;
      p->size = 1.0f;
      p->lifetime = 5.0f;
      p->age = 0.0f;
      p->active = true;

      sys->particle_count++;
      break;
    }
  }
}

// Update particles
void particle_system_update(ParticleSystem *sys, float dt) {
  // Emit new particles
  sys->time_accumulator += dt;
  float emit_interval = 1.0f / sys->emission_rate;

  while (sys->time_accumulator >= emit_interval) {
    particle_emit(sys);
    sys->time_accumulator -= emit_interval;
  }

  // Update existing particles
  for (int i = 0; i < sys->max_particles; i++) {
    Particle *p = &sys->particles[i];
    if (!p->active)
      continue;

    // Update age
    p->age += dt;
    if (p->age >= p->lifetime) {
      p->active = false;
      sys->particle_count--;
      continue;
    }

    // Apply gravity
    p->velocity[0] += sys->gravity[0] * dt;
    p->velocity[1] += sys->gravity[1] * dt;
    p->velocity[2] += sys->gravity[2] * dt;

    // Update position
    p->position[0] += p->velocity[0] * dt;
    p->position[1] += p->velocity[1] * dt;
    p->position[2] += p->velocity[2] * dt;

    // Fade out
    float life_ratio = p->age / p->lifetime;
    p->color[3] = 1.0f - life_ratio;
  }
}

// Render particles
void particle_system_render(ParticleSystem *sys) {
  // TODO: Upload particle data to GPU
  // TODO: Render as billboards
}

/*
 * IMPLEMENTATION: 25/120 Particle System TODOs
 * LOC: ~140
 */
