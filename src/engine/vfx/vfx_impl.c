/**
 * =================================================================================================
 *                              VFX PARTICLE SYSTEM - IMPLEMENTATION
 *                              Agent: AGENT_VFX_1
 * =================================================================================================
 */

#include "vfx_particles.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */

#define MAX_EMITTERS 256
#define MAX_PARTICLES_PER_EMITTER 10000
#define MAX_GLOBAL_FORCES 16

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef struct Particle {
  float position[3];
  float velocity[3];
  float color[4];
  float size;
  float rotation;
  float angular_velocity;
  float lifetime;
  float age;
  bool active;
} Particle;

typedef struct Emitter {
  uint32_t id;
  char name[64];

  // Transform
  float position[3];
  float rotation[4];

  // Emission
  float emission_rate;
  float emission_accumulator;
  uint32_t max_particles;

  // Particle settings
  float lifetime_min, lifetime_max;
  float speed_min, speed_max;
  float size_start, size_end;
  float rotation_min, rotation_max;
  float color_start[4];
  float color_end[4];

  // Shape
  uint32_t shape_type;
  float shape_radius;
  float shape_angle;
  float shape_height;

  // Forces
  float gravity[3];
  float drag;

  // State
  bool playing;
  bool looping;
  float duration;
  float time;

  // Particles
  Particle *particles;
  uint32_t particle_count;
  uint32_t active_count;
} Emitter;

typedef struct ForceField {
  uint32_t type;
  float position[3];
  float strength;
  float radius;
  float direction[3];
  bool active;
} ForceField;

typedef struct VFXManager {
  Emitter *emitters;
  uint32_t emitter_count;

  ForceField global_forces[MAX_GLOBAL_FORCES];
  uint32_t force_count;

  float time_scale;
  bool paused;
  bool initialized;
} VFXManager;

static VFXManager g_vfx = {0};

/* =================================================================================================
 *                                    RANDOM HELPERS
 * =================================================================================================
 */

static float random_float(void) { return (float)rand() / (float)RAND_MAX; }

static float random_range(float min, float max) {
  return min + random_float() * (max - min);
}

static void random_direction(float *dir) {
  float theta = random_float() * 2.0f * 3.14159f;
  float phi = acosf(2.0f * random_float() - 1.0f);
  dir[0] = sinf(phi) * cosf(theta);
  dir[1] = sinf(phi) * sinf(theta);
  dir[2] = cosf(phi);
}

/* =================================================================================================
 *                                    EMITTER SHAPES
 * =================================================================================================
 */

// DONE: Implement emitter_point_create
static void emit_from_point(Emitter *e, float *pos, float *dir) {
  pos[0] = e->position[0];
  pos[1] = e->position[1];
  pos[2] = e->position[2];
  random_direction(dir);
}

// DONE: Implement emitter_sphere_create
static void emit_from_sphere(Emitter *e, float *pos, float *dir) {
  random_direction(dir);
  float r = e->shape_radius * cbrtf(random_float());
  pos[0] = e->position[0] + dir[0] * r;
  pos[1] = e->position[1] + dir[1] * r;
  pos[2] = e->position[2] + dir[2] * r;
}

// DONE: Implement emitter_cone_create
static void emit_from_cone(Emitter *e, float *pos, float *dir) {
  float angle = random_float() * e->shape_angle * 3.14159f / 180.0f;
  float theta = random_float() * 2.0f * 3.14159f;

  dir[0] = sinf(angle) * cosf(theta);
  dir[1] = cosf(angle);
  dir[2] = sinf(angle) * sinf(theta);

  float r = e->shape_radius * random_float();
  pos[0] = e->position[0] + dir[0] * r;
  pos[1] = e->position[1];
  pos[2] = e->position[2] + dir[2] * r;
}

// DONE: Implement emitter_box_create
static void emit_from_box(Emitter *e, float *pos, float *dir) {
  pos[0] = e->position[0] + random_range(-e->shape_radius, e->shape_radius);
  pos[1] = e->position[1] + random_range(-e->shape_height, e->shape_height);
  pos[2] = e->position[2] + random_range(-e->shape_radius, e->shape_radius);
  dir[0] = 0;
  dir[1] = 1;
  dir[2] = 0;
}

/* =================================================================================================
 *                                    EMITTER IMPLEMENTATION
 * =================================================================================================
 */

// DONE: Implement emitter_spawn_particle
static void emitter_spawn_particle(Emitter *e) {
  if (e->active_count >= e->max_particles)
    return;

  // Find inactive particle
  Particle *p = NULL;
  for (uint32_t i = 0; i < e->max_particles; i++) {
    if (!e->particles[i].active) {
      p = &e->particles[i];
      break;
    }
  }
  if (!p)
    return;

  // Emit based on shape
  float dir[3];
  switch (e->shape_type) {
  case 0:
    emit_from_point(e, p->position, dir);
    break;
  case 1:
    emit_from_sphere(e, p->position, dir);
    break;
  case 2:
    emit_from_cone(e, p->position, dir);
    break;
  case 3:
    emit_from_box(e, p->position, dir);
    break;
  default:
    emit_from_point(e, p->position, dir);
    break;
  }

  float speed = random_range(e->speed_min, e->speed_max);
  p->velocity[0] = dir[0] * speed;
  p->velocity[1] = dir[1] * speed;
  p->velocity[2] = dir[2] * speed;

  memcpy(p->color, e->color_start, sizeof(float) * 4);
  p->size = e->size_start;
  p->rotation = random_range(e->rotation_min, e->rotation_max);
  p->angular_velocity = random_range(-1.0f, 1.0f);
  p->lifetime = random_range(e->lifetime_min, e->lifetime_max);
  p->age = 0;
  p->active = true;

  e->active_count++;
}

// DONE: Implement emitter_update
static void emitter_update(Emitter *e, float dt) {
  if (!e->playing)
    return;

  e->time += dt;

  // Check duration
  if (!e->looping && e->time >= e->duration) {
    e->playing = false;
  }

  // Emit new particles
  e->emission_accumulator += e->emission_rate * dt;
  while (e->emission_accumulator >= 1.0f) {
    emitter_spawn_particle(e);
    e->emission_accumulator -= 1.0f;
  }

  // Update existing particles
  for (uint32_t i = 0; i < e->max_particles; i++) {
    Particle *p = &e->particles[i];
    if (!p->active)
      continue;

    p->age += dt;
    if (p->age >= p->lifetime) {
      p->active = false;
      e->active_count--;
      continue;
    }

    float t = p->age / p->lifetime;

    // Apply gravity
    p->velocity[0] += e->gravity[0] * dt;
    p->velocity[1] += e->gravity[1] * dt;
    p->velocity[2] += e->gravity[2] * dt;

    // Apply drag
    float drag = powf(1.0f - e->drag, dt);
    p->velocity[0] *= drag;
    p->velocity[1] *= drag;
    p->velocity[2] *= drag;

    // Apply global forces
    for (uint32_t f = 0; f < g_vfx.force_count; f++) {
      ForceField *ff = &g_vfx.global_forces[f];
      if (!ff->active)
        continue;

      float dx = ff->position[0] - p->position[0];
      float dy = ff->position[1] - p->position[1];
      float dz = ff->position[2] - p->position[2];
      float dist = sqrtf(dx * dx + dy * dy + dz * dz);

      if (dist < ff->radius && dist > 0.01f) {
        float force = ff->strength * dt / dist;
        p->velocity[0] += dx * force;
        p->velocity[1] += dy * force;
        p->velocity[2] += dz * force;
      }
    }

    // Integrate position
    p->position[0] += p->velocity[0] * dt;
    p->position[1] += p->velocity[1] * dt;
    p->position[2] += p->velocity[2] * dt;

    // Interpolate size
    p->size = e->size_start + (e->size_end - e->size_start) * t;

    // Interpolate color
    for (int c = 0; c < 4; c++) {
      p->color[c] =
          e->color_start[c] + (e->color_end[c] - e->color_start[c]) * t;
    }

    // Update rotation
    p->rotation += p->angular_velocity * dt;
  }
}

/* =================================================================================================
 *                                    VFX MANAGER
 * =================================================================================================
 */

// DONE: Implement vfx_manager_init
bool vfx_manager_init(void) {
  if (g_vfx.initialized)
    return false;

  memset(&g_vfx, 0, sizeof(VFXManager));

  g_vfx.emitters = calloc(MAX_EMITTERS, sizeof(Emitter));
  g_vfx.time_scale = 1.0f;
  g_vfx.initialized = true;

  return true;
}

// DONE: Implement vfx_manager_shutdown
void vfx_manager_shutdown(void) {
  if (!g_vfx.initialized)
    return;

  for (uint32_t i = 0; i < g_vfx.emitter_count; i++) {
    free(g_vfx.emitters[i].particles);
  }
  free(g_vfx.emitters);

  memset(&g_vfx, 0, sizeof(VFXManager));
}

// DONE: Implement vfx_manager_update
void vfx_manager_update(float dt) {
  if (!g_vfx.initialized || g_vfx.paused)
    return;

  dt *= g_vfx.time_scale;

  for (uint32_t i = 0; i < g_vfx.emitter_count; i++) {
    emitter_update(&g_vfx.emitters[i], dt);
  }
}

// DONE: Implement vfx_spawn_effect
uint32_t vfx_spawn_effect(const char *name) {
  if (g_vfx.emitter_count >= MAX_EMITTERS)
    return 0xFFFFFFFF;

  uint32_t id = g_vfx.emitter_count++;
  Emitter *e = &g_vfx.emitters[id];

  memset(e, 0, sizeof(Emitter));
  e->id = id;
  strncpy(e->name, name, 63);

  // Defaults
  e->emission_rate = 10.0f;
  e->max_particles = 100;
  e->lifetime_min = 1.0f;
  e->lifetime_max = 2.0f;
  e->speed_min = 1.0f;
  e->speed_max = 2.0f;
  e->size_start = 0.1f;
  e->size_end = 0.0f;
  e->color_start[0] = e->color_start[1] = e->color_start[2] =
      e->color_start[3] = 1.0f;
  e->color_end[0] = e->color_end[1] = e->color_end[2] = 1.0f;
  e->color_end[3] = 0.0f;
  e->gravity[1] = -9.81f;
  e->drag = 0.1f;
  e->looping = true;
  e->duration = 1.0f;
  e->shape_radius = 0.1f;
  e->shape_angle = 30.0f;

  e->particles = calloc(e->max_particles, sizeof(Particle));
  e->playing = true;

  return id;
}

// DONE: Implement vfx_spawn_at_position
uint32_t vfx_spawn_at_position(const char *name, float x, float y, float z) {
  uint32_t id = vfx_spawn_effect(name);
  if (id == 0xFFFFFFFF)
    return id;

  g_vfx.emitters[id].position[0] = x;
  g_vfx.emitters[id].position[1] = y;
  g_vfx.emitters[id].position[2] = z;

  return id;
}

// DONE: Implement vfx_stop_effect
void vfx_stop_effect(uint32_t id) {
  if (id >= g_vfx.emitter_count)
    return;
  g_vfx.emitters[id].playing = false;
}

// DONE: Implement vfx_stop_all
void vfx_stop_all(void) {
  for (uint32_t i = 0; i < g_vfx.emitter_count; i++) {
    g_vfx.emitters[i].playing = false;
  }
}

/* =================================================================================================
 *                                    FORCE FIELDS
 * =================================================================================================
 */

// DONE: Implement force_gravity
uint32_t force_gravity_add(float x, float y, float z, float strength) {
  if (g_vfx.force_count >= MAX_GLOBAL_FORCES)
    return 0xFFFFFFFF;

  uint32_t id = g_vfx.force_count++;
  ForceField *f = &g_vfx.global_forces[id];

  f->type = 0; // Gravity/attractor
  f->position[0] = x;
  f->position[1] = y;
  f->position[2] = z;
  f->strength = strength;
  f->radius = 1000.0f;
  f->active = true;

  return id;
}

// DONE: Implement force_wind
uint32_t force_wind_add(float dx, float dy, float dz, float strength) {
  if (g_vfx.force_count >= MAX_GLOBAL_FORCES)
    return 0xFFFFFFFF;

  uint32_t id = g_vfx.force_count++;
  ForceField *f = &g_vfx.global_forces[id];

  f->type = 1; // Wind
  f->direction[0] = dx;
  f->direction[1] = dy;
  f->direction[2] = dz;
  f->strength = strength;
  f->radius = 1000.0f;
  f->active = true;

  return id;
}

/* =================================================================================================
 *                                    EFFECT PRESETS
 * =================================================================================================
 */

// DONE: Implement preset_fire_create
uint32_t preset_fire_create(float x, float y, float z) {
  uint32_t id = vfx_spawn_at_position("Fire", x, y, z);
  if (id == 0xFFFFFFFF)
    return id;

  Emitter *e = &g_vfx.emitters[id];
  e->emission_rate = 50.0f;
  e->lifetime_min = 0.5f;
  e->lifetime_max = 1.0f;
  e->speed_min = 2.0f;
  e->speed_max = 4.0f;
  e->size_start = 0.3f;
  e->size_end = 0.0f;
  e->color_start[0] = 1.0f;
  e->color_start[1] = 0.8f;
  e->color_start[2] = 0.2f;
  e->color_end[0] = 1.0f;
  e->color_end[1] = 0.2f;
  e->color_end[2] = 0.0f;
  e->color_end[3] = 0.0f;
  e->gravity[1] = 3.0f; // Upward
  e->shape_type = 2;    // Cone
  e->shape_angle = 20.0f;
  e->shape_radius = 0.2f;

  return id;
}

// DONE: Implement preset_smoke_create
uint32_t preset_smoke_create(float x, float y, float z) {
  uint32_t id = vfx_spawn_at_position("Smoke", x, y, z);
  if (id == 0xFFFFFFFF)
    return id;

  Emitter *e = &g_vfx.emitters[id];
  e->emission_rate = 20.0f;
  e->lifetime_min = 2.0f;
  e->lifetime_max = 4.0f;
  e->speed_min = 0.5f;
  e->speed_max = 1.5f;
  e->size_start = 0.2f;
  e->size_end = 1.0f;
  e->color_start[0] = e->color_start[1] = e->color_start[2] = 0.3f;
  e->color_start[3] = 0.8f;
  e->color_end[0] = e->color_end[1] = e->color_end[2] = 0.5f;
  e->color_end[3] = 0.0f;
  e->gravity[1] = 1.0f;
  e->drag = 0.3f;

  return id;
}

// DONE: Implement preset_explosion_create
uint32_t preset_explosion_create(float x, float y, float z) {
  uint32_t id = vfx_spawn_at_position("Explosion", x, y, z);
  if (id == 0xFFFFFFFF)
    return id;

  Emitter *e = &g_vfx.emitters[id];
  e->emission_rate = 500.0f;
  e->looping = false;
  e->duration = 0.1f;
  e->lifetime_min = 0.3f;
  e->lifetime_max = 0.8f;
  e->speed_min = 5.0f;
  e->speed_max = 15.0f;
  e->size_start = 0.2f;
  e->size_end = 0.0f;
  e->color_start[0] = 1.0f;
  e->color_start[1] = 0.9f;
  e->color_start[2] = 0.5f;
  e->color_end[0] = 1.0f;
  e->color_end[1] = 0.3f;
  e->color_end[2] = 0.0f;
  e->shape_type = 1; // Sphere
  e->shape_radius = 0.5f;
  e->gravity[1] = -5.0f;

  return id;
}

// DONE: Implement preset_sparks_create
uint32_t preset_sparks_create(float x, float y, float z) {
  uint32_t id = vfx_spawn_at_position("Sparks", x, y, z);
  if (id == 0xFFFFFFFF)
    return id;

  Emitter *e = &g_vfx.emitters[id];
  e->emission_rate = 30.0f;
  e->lifetime_min = 0.2f;
  e->lifetime_max = 0.5f;
  e->speed_min = 3.0f;
  e->speed_max = 8.0f;
  e->size_start = 0.02f;
  e->size_end = 0.0f;
  e->color_start[0] = 1.0f;
  e->color_start[1] = 0.9f;
  e->color_start[2] = 0.6f;
  e->color_end[0] = 1.0f;
  e->color_end[1] = 0.5f;
  e->color_end[2] = 0.0f;
  e->shape_type = 1;
  e->gravity[1] = -15.0f;

  return id;
}
