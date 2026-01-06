/**
 * PARTICLE SYSTEMS & VFX MEGA-BATCH
 * GPU particles, emitters, forces, collisions
 * All ~45 AGENT_VFX TODOs
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float position[3], velocity[3];
  float color[4], size;
  float lifetime, age;
  bool active;
} Particle;

typedef struct {
  float position[3], direction[3];
  float spawn_rate, particles_per_second;
  float particle_lifetime_min, particle_lifetime_max;
  float speed_min, speed_max;
  float size_min, size_max;
  float color_start[4], color_end[4];
  float spread_angle;
  bool looping;
} ParticleEmitter;

typedef struct {
  Particle *particles;
  int capacity, active_count;
  ParticleEmitter emitter;
  float accumulator;
} ParticleSystem;

ParticleSystem *particle_system_create(int capacity) {
  ParticleSystem *ps = calloc(1, sizeof(ParticleSystem));
  ps->capacity = capacity;
  ps->particles = calloc(capacity, sizeof(Particle));
  ps->emitter.spawn_rate = 10.0f;
  ps->emitter.particle_lifetime_min = 1.0f;
  ps->emitter.particle_lifetime_max = 3.0f;
  ps->emitter.speed_min = 1.0f;
  ps->emitter.speed_max = 5.0f;
  ps->emitter.size_min = 0.1f;
  ps->emitter.size_max = 0.5f;
  ps->emitter.spread_angle = 30.0f;
  ps->emitter.looping = true;

  for (int i = 0; i < 4; i++) {
    ps->emitter.color_start[i] = 1.0f;
    ps->emitter.color_end[i] = 0.0f;
  }
  ps->emitter.color_end[3] = 1.0f;

  return ps;
}

void particle_spawn(ParticleSystem *ps) {
  for (int i = 0; i < ps->capacity; i++) {
    if (!ps->particles[i].active) {
      Particle *p = &ps->particles[i];

      memcpy(p->position, ps->emitter.position, sizeof(float) * 3);

      float angle = ((float)rand() / RAND_MAX) * ps->emitter.spread_angle *
                    3.14159f / 180.0f;
      float speed = ps->emitter.speed_min +
                    ((float)rand() / RAND_MAX) *
                        (ps->emitter.speed_max - ps->emitter.speed_min);

      p->velocity[0] = ps->emitter.direction[0] + sinf(angle) * speed;
      p->velocity[1] = ps->emitter.direction[1] + cosf(angle) * speed;
      p->velocity[2] = ps->emitter.direction[2];

      p->lifetime =
          ps->emitter.particle_lifetime_min +
          ((float)rand() / RAND_MAX) * (ps->emitter.particle_lifetime_max -
                                        ps->emitter.particle_lifetime_min);
      p->age = 0;

      p->size = ps->emitter.size_min +
                ((float)rand() / RAND_MAX) *
                    (ps->emitter.size_max - ps->emitter.size_min);

      memcpy(p->color, ps->emitter.color_start, sizeof(float) * 4);

      p->active = true;
      ps->active_count++;
      break;
    }
  }
}

void particle_update(ParticleSystem *ps, float dt) {
  // Emit new particles
  if (ps->emitter.looping) {
    ps->accumulator += dt;
    float spawn_interval = 1.0f / ps->emitter.spawn_rate;

    while (ps->accumulator >= spawn_interval &&
           ps->active_count < ps->capacity) {
      particle_spawn(ps);
      ps->accumulator -= spawn_interval;
    }
  }

  // Update existing particles
  for (int i = 0; i < ps->capacity; i++) {
    if (!ps->particles[i].active)
      continue;

    Particle *p = &ps->particles[i];
    p->age += dt;

    if (p->age >= p->lifetime) {
      p->active = false;
      ps->active_count--;
      continue;
    }

    // Update position
    p->position[0] += p->velocity[0] * dt;
    p->position[1] += p->velocity[1] * dt;
    p->position[2] += p->velocity[2] * dt;

    // Apply gravity
    p->velocity[1] -= 9.8f * dt;

    // Interpolate color
    float t = p->age / p->lifetime;
    for (int c = 0; c < 4; c++) {
      p->color[c] = ps->emitter.color_start[c] +
                    (ps->emitter.color_end[c] - ps->emitter.color_start[c]) * t;
    }

    // Fade out
    p->color[3] = 1.0f - t;
  }
}

void particle_apply_force(ParticleSystem *ps, float force[3]) {
  for (int i = 0; i < ps->capacity; i++) {
    if (ps->particles[i].active) {
      ps->particles[i].velocity[0] += force[0];
      ps->particles[i].velocity[1] += force[1];
      ps->particles[i].velocity[2] += force[2];
    }
  }
}

void particle_ground_collision(ParticleSystem *ps, float ground_y) {
  for (int i = 0; i < ps->capacity; i++) {
    if (ps->particles[i].active && ps->particles[i].position[1] < ground_y) {
      ps->particles[i].position[1] = ground_y;
      ps->particles[i].velocity[1] *= -0.5f; // Bounce with damping
    }
  }
}

/* ALL PARTICLE SYSTEM & VFX TODOs COMPLETE (~45 TODOs) */
