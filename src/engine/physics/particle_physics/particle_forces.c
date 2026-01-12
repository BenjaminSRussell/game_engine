#include "particle_forces.h"
#include "particle_collision.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct ParticleForceSystem {
  ParticleForce *forces;
  int force_count;
  int force_capacity;
  float time;
  uint32_t next_id;
};

static void vec3_add_inplace(float *a, const float *b) {
  a[0] += b[0];
  a[1] += b[1];
  a[2] += b[2];
}

static void vec3_scale(float *out, const float *v, float s) {
  out[0] = v[0] * s;
  out[1] = v[1] * s;
  out[2] = v[2] * s;
}

static float vec3_dot(const float *a, const float *b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static float vec3_length(const float *v) {
  return sqrtf(vec3_dot(v, v));
}

static void vec3_normalize(float *out, const float *v) {
  float len = vec3_length(v);
  if (len > 1e-6f) {
    vec3_scale(out, v, 1.0f / len);
  } else {
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
  }
}

static void vec3_cross(float *out, const float *a, const float *b) {
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}

static float apply_falloff(float distance, float radius, float falloff) {
  if (radius <= 0.0f)
    return 1.0f;
  if (distance > radius)
    return 0.0f;
  float t = 1.0f - (distance / radius);
  return t * t * (1.0f + falloff);
}

ParticleForceSystem *particle_forces_create(int max_forces) {
  if (max_forces <= 0)
    max_forces = 32;

  ParticleForceSystem *system =
      (ParticleForceSystem *)calloc(1, sizeof(ParticleForceSystem));
  if (!system)
    return NULL;

  system->forces =
      (ParticleForce *)calloc((size_t)max_forces, sizeof(ParticleForce));
  if (!system->forces) {
    free(system);
    return NULL;
  }

  system->force_capacity = max_forces;
  system->force_count = 0;
  system->time = 0.0f;
  system->next_id = 1;
  return system;
}

void particle_forces_destroy(ParticleForceSystem *system) {
  if (!system)
    return;
  free(system->forces);
  free(system);
}

int particle_forces_add(ParticleForceSystem *system, const ParticleForce *force) {
  if (!system || !force)
    return -1;

  if (system->force_count >= system->force_capacity) {
    int new_capacity = system->force_capacity * 2;
    ParticleForce *new_forces =
        (ParticleForce *)realloc(system->forces,
                                 sizeof(ParticleForce) * (size_t)new_capacity);
    if (!new_forces)
      return -1;
    system->forces = new_forces;
    system->force_capacity = new_capacity;
  }

  ParticleForce copy = *force;
  copy.id = system->next_id++;
  if (!copy.enabled)
    copy.enabled = true;
  system->forces[system->force_count++] = copy;
  return (int)copy.id;
}

bool particle_forces_remove(ParticleForceSystem *system, int force_id) {
  if (!system || force_id <= 0)
    return false;

  for (int i = 0; i < system->force_count; ++i) {
    if ((int)system->forces[i].id == force_id) {
      int last = system->force_count - 1;
      if (i != last)
        system->forces[i] = system->forces[last];
      system->force_count--;
      return true;
    }
  }

  return false;
}

void particle_forces_update(ParticleForceSystem *system, int force_id,
                            const ParticleForce *force) {
  if (!system || !force || force_id <= 0)
    return;

  for (int i = 0; i < system->force_count; ++i) {
    if ((int)system->forces[i].id == force_id) {
      ParticleForce updated = *force;
      updated.id = system->forces[i].id;
      system->forces[i] = updated;
      return;
    }
  }
}

ParticleForce *particle_forces_get(ParticleForceSystem *system, int force_id) {
  if (!system || force_id <= 0)
    return NULL;

  for (int i = 0; i < system->force_count; ++i) {
    if ((int)system->forces[i].id == force_id)
      return &system->forces[i];
  }

  return NULL;
}

void particle_forces_clear(ParticleForceSystem *system) {
  if (!system)
    return;
  system->force_count = 0;
}

void particle_forces_apply(ParticleForceSystem *system, Particle *particles,
                           int particle_count, float dt) {
  if (!system || !particles || particle_count <= 0)
    return;

  system->time += dt;

  for (int i = 0; i < particle_count; ++i) {
    Particle *p = &particles[i];
    if (p->inv_mass <= 0.0f)
      continue;

    float total_accel[3] = {0.0f, 0.0f, 0.0f};

    for (int j = 0; j < system->force_count; ++j) {
      ParticleForce *force = &system->forces[j];
      if (!force->enabled)
        continue;

      float accel[3] = {0.0f, 0.0f, 0.0f};

      switch (force->type) {
      case PARTICLE_FORCE_GRAVITY: {
        float dir[3] = {force->direction[0], force->direction[1],
                        force->direction[2]};
        if (vec3_length(dir) < 1e-6f) {
          dir[0] = 0.0f;
          dir[1] = -1.0f;
          dir[2] = 0.0f;
        }
        vec3_normalize(dir, dir);
        vec3_scale(accel, dir, force->strength);
        break;
      }
      case PARTICLE_FORCE_WIND: {
        vec3_scale(accel, force->direction, force->strength);
        break;
      }
      case PARTICLE_FORCE_DRAG: {
        accel[0] = -p->velocity[0] * force->drag;
        accel[1] = -p->velocity[1] * force->drag;
        accel[2] = -p->velocity[2] * force->drag;
        break;
      }
      case PARTICLE_FORCE_POINT: {
        float to_center[3] = {force->position[0] - p->position[0],
                              force->position[1] - p->position[1],
                              force->position[2] - p->position[2]};
        float distance = vec3_length(to_center);
        if (distance > 1e-6f) {
          float falloff =
              apply_falloff(distance, force->radius, force->falloff);
          vec3_normalize(to_center, to_center);
          vec3_scale(accel, to_center, force->strength * falloff);
        }
        break;
      }
      case PARTICLE_FORCE_VORTEX: {
        float axis[3] = {force->direction[0], force->direction[1],
                         force->direction[2]};
        vec3_normalize(axis, axis);
        float to_center[3] = {p->position[0] - force->position[0],
                              p->position[1] - force->position[1],
                              p->position[2] - force->position[2]};
        float radial[3];
        vec3_cross(radial, axis, to_center);
        float distance = vec3_length(to_center);
        float falloff =
            apply_falloff(distance, force->radius, force->falloff);
        vec3_normalize(radial, radial);
        vec3_scale(accel, radial, force->strength * falloff);
        break;
      }
      case PARTICLE_FORCE_TURBULENCE: {
        float n = sinf((p->position[0] + system->time) * force->noise_scale) +
                  cosf((p->position[1] - system->time) * force->noise_scale) +
                  sinf((p->position[2] + system->time) * force->noise_scale);
        accel[0] = force->strength * n * 0.33f;
        accel[1] = force->strength * sinf(n);
        accel[2] = force->strength * cosf(n);
        break;
      }
      default:
        break;
      }

      vec3_add_inplace(total_accel, accel);
    }

    p->acceleration[0] += total_accel[0];
    p->acceleration[1] += total_accel[1];
    p->acceleration[2] += total_accel[2];
  }
}
