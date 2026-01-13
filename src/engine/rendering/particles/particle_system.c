// src/engine/rendering/particles/particle_system.c
// Particle System - GPU-accelerated particle rendering with physics simulation

#include "engine/include/core/logger.h"
#include <core/time_system.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../render_pipeline.h"

// ============================================================================
// Particle System Types
// ============================================================================

typedef enum {
  PARTICLE_SHAPE_POINT,
  PARTICLE_SHAPE_QUAD,
  PARTICLE_SHAPE_BILLBOARD,
  PARTICLE_SHAPE_MESH,
  PARTICLE_SHAPE_RIBBON
} ParticleShape;

typedef enum {
  PARTICLE_EMISSION_TYPE_POINT,
  PARTICLE_EMISSION_TYPE_SPHERE,
  PARTICLE_EMISSION_TYPE_CONE,
  PARTICLE_EMISSION_TYPE_BOX,
  PARTICLE_EMISSION_TYPE_CYLINDER
} ParticleEmissionType;

typedef enum {
  PARTICLE_SIMULATION_CPU,
  PARTICLE_SIMULATION_GPU_COMPUTE,
  PARTICLE_SIMULATION_GPU_VERTEX
} ParticleSimulationType;

typedef struct {
  // Position and motion
  float position[3];
  float velocity[3];
  float acceleration[3];

  // Appearance
  float color[4];
  float size;
  float rotation;
  float rotation_speed;

  // Lifetime
  float age;
  float lifetime;

  // Physics
  float mass;
  float drag;

  // Texture animation
  float uv_offset[2];
  float uv_scale[2];
  uint32_t frame;

  // Flags
  uint32_t active : 1;
  uint32_t alive : 1;
} Particle;

typedef struct {
  // Emitter properties
  char name[64];
  ParticleEmissionType emission_type;
  ParticleShape shape;
  ParticleSimulationType simulation_type;

  // Position and orientation
  float position[3];
  float direction[3];
  float up[3];

  // Emission parameters
  float emission_rate;
  float emission_burst;
  float emission_timer;
  uint32_t max_particles;
  uint32_t active_count;

  // Particle properties
  float initial_velocity_min[3];
  float initial_velocity_max[3];
  float initial_color[4];
  float initial_color_variation[4];
  float initial_size_min;
  float initial_size_max;
  float initial_lifetime_min;
  float initial_lifetime_max;

  // Physics
  float gravity[3];
  float wind[3];
  float turbulence;
  float drag_coefficient;

  // Rendering
  void *texture;
  void *shader;
  bool depth_write;
  bool alpha_blending;
  bool soft_particles;

  // GPU resources
  void *particle_buffer;
  void *indirect_buffer;
  void *compute_buffer;
  void *vertex_buffer;
  void *index_buffer;

  // Simulation data
  Particle *particles;
  uint32_t *free_list;
  uint32_t free_list_size;

  // Statistics
  uint32_t total_emitted;
  uint32_t total_dead;
  float simulation_time_ms;
  float render_time_ms;

  // Threading
  pthread_mutex_t mutex;
  bool needs_update;

  bool enabled;
} ParticleEmitter;

typedef struct {
  ParticleEmitter *emitters[256];
  uint32_t emitter_count;
  uint32_t emitter_capacity;

  // Global settings
  float global_time_scale;
  float max_simulation_time;
  bool enable_gpu_simulation;

  // Sorting and culling
  bool sort_by_depth;
  bool enable_culling;
  float culling_distance;

  // Global resources
  void *default_texture;
  void *default_shader;
  void *quad_vbo;
  void *quad_ibo;

  // Memory management
  uint64_t total_particles;
  uint64_t max_total_particles;

  // Performance
  float total_simulation_time;
  float total_render_time;

  bool initialized;
} ParticleSystem;

static ParticleSystem g_particle_system = {0};

// ============================================================================
// Particle Physics and Simulation
// ============================================================================

static void update_particle_physics(Particle *particle, float dt,
                                    const ParticleEmitter *emitter) {
  if (!particle->alive)
    return;

  // Update age
  particle->age += dt;
  if (particle->age >= particle->lifetime) {
    particle->alive = false;
    return;
  }

  // Apply forces
  float force[3] = {0.0f, 0.0f, 0.0f};

  // Gravity
  force[0] += emitter->gravity[0] * particle->mass;
  force[1] += emitter->gravity[1] * particle->mass;
  force[2] += emitter->gravity[2] * particle->mass;

  // Wind
  force[0] += emitter->wind[0];
  force[1] += emitter->wind[1];
  force[2] += emitter->wind[2];

  // Turbulence (simplified Perlin noise)
  if (emitter->turbulence > 0.0f) {
    float noise_x = sinf(particle->position[0] * 0.1f + particle->age * 2.0f) *
                    emitter->turbulence;
    float noise_y = cosf(particle->position[1] * 0.1f + particle->age * 1.5f) *
                    emitter->turbulence;
    float noise_z = sinf(particle->position[2] * 0.1f + particle->age * 1.8f) *
                    emitter->turbulence;
    force[0] += noise_x;
    force[1] += noise_y;
    force[2] += noise_z;
  }

  // Update acceleration
  particle->acceleration[0] = force[0] / particle->mass;
  particle->acceleration[1] = force[1] / particle->mass;
  particle->acceleration[2] = force[2] / particle->mass;

  // Apply drag
  float drag_force = emitter->drag_coefficient * particle->drag;
  particle->velocity[0] *= (1.0f - drag_force * dt);
  particle->velocity[1] *= (1.0f - drag_force * dt);
  particle->velocity[2] *= (1.0f - drag_force * dt);

  // Update velocity and position
  particle->velocity[0] += particle->acceleration[0] * dt;
  particle->velocity[1] += particle->acceleration[1] * dt;
  particle->velocity[2] += particle->acceleration[2] * dt;

  particle->position[0] += particle->velocity[0] * dt;
  particle->position[1] += particle->velocity[1] * dt;
  particle->position[2] += particle->velocity[2] * dt;

  // Update rotation
  particle->rotation += particle->rotation_speed * dt;

  // Update texture animation
  // TODO: Implement UV animation based on age
}

static void emit_particle(ParticleEmitter *emitter) {
  if (emitter->free_list_size == 0)
    return;

  // Get particle from free list
  uint32_t index = emitter->free_list[--emitter->free_list_size];
  Particle *particle = &emitter->particles[index];

  // Initialize particle
  memset(particle, 0, sizeof(Particle));

  // Position based on emission type
  switch (emitter->emission_type) {
  case PARTICLE_EMISSION_TYPE_POINT:
    particle->position[0] = emitter->position[0];
    particle->position[1] = emitter->position[1];
    particle->position[2] = emitter->position[2];
    break;

  case PARTICLE_EMISSION_TYPE_SPHERE: {
    float theta = (float)rand() / RAND_MAX * 2.0f * M_PI;
    float phi = acosf(2.0f * (float)rand() / RAND_MAX - 1.0f);
    float radius = (float)rand() / RAND_MAX;

    particle->position[0] =
        emitter->position[0] + radius * sinf(phi) * cosf(theta);
    particle->position[1] =
        emitter->position[1] + radius * sinf(phi) * sinf(theta);
    particle->position[2] = emitter->position[2] + radius * cosf(phi);
    break;
  }

  case PARTICLE_EMISSION_TYPE_CONE: {
    float angle = (float)rand() / RAND_MAX * M_PI * 0.5f; // 30 degree cone
    float rotation = (float)rand() / RAND_MAX * 2.0f * M_PI;

    // Simplified cone emission
    particle->position[0] = emitter->position[0];
    particle->position[1] = emitter->position[1];
    particle->position[2] = emitter->position[2];
    break;
  }

  default:
    particle->position[0] = emitter->position[0];
    particle->position[1] = emitter->position[1];
    particle->position[2] = emitter->position[2];
    break;
  }

  // Random initial velocity
  particle->velocity[0] =
      emitter->initial_velocity_min[0] +
      (float)rand() / RAND_MAX *
          (emitter->initial_velocity_max[0] - emitter->initial_velocity_min[0]);
  particle->velocity[1] =
      emitter->initial_velocity_min[1] +
      (float)rand() / RAND_MAX *
          (emitter->initial_velocity_max[1] - emitter->initial_velocity_min[1]);
  particle->velocity[2] =
      emitter->initial_velocity_min[2] +
      (float)rand() / RAND_MAX *
          (emitter->initial_velocity_max[2] - emitter->initial_velocity_min[2]);

  // Random initial color
  particle->color[0] =
      emitter->initial_color[0] +
      (float)rand() / RAND_MAX * emitter->initial_color_variation[0];
  particle->color[1] =
      emitter->initial_color[1] +
      (float)rand() / RAND_MAX * emitter->initial_color_variation[1];
  particle->color[2] =
      emitter->initial_color[2] +
      (float)rand() / RAND_MAX * emitter->initial_color_variation[2];
  particle->color[3] =
      emitter->initial_color[3] +
      (float)rand() / RAND_MAX * emitter->initial_color_variation[3];

  // Clamp color values
  for (int i = 0; i < 4; i++) {
    particle->color[i] = fmaxf(0.0f, fminf(1.0f, particle->color[i]));
  }

  // Random initial size
  particle->size = emitter->initial_size_min +
                   (float)rand() / RAND_MAX *
                       (emitter->initial_size_max - emitter->initial_size_min);

  // Random lifetime
  particle->lifetime =
      emitter->initial_lifetime_min +
      (float)rand() / RAND_MAX *
          (emitter->initial_lifetime_max - emitter->initial_lifetime_min);

  particle->age = 0.0f;
  particle->mass = 1.0f;
  particle->drag = 1.0f;
  particle->rotation = 0.0f;
  particle->rotation_speed = 0.0f;
  particle->active = true;
  particle->alive = true;

  emitter->active_count++;
  emitter->total_emitted++;
}

// ============================================================================
// Particle System API
// ============================================================================

bool particle_system_init(uint32_t max_emitters, uint64_t max_total_particles) {
  if (g_particle_system.initialized) {
    LOG_WARN("Particle system already initialized");
    return true;
  }

  memset(&g_particle_system, 0, sizeof(ParticleSystem));

  g_particle_system.emitter_capacity = max_emitters;
  g_particle_system.emitters = calloc(max_emitters, sizeof(ParticleEmitter *));

  if (!g_particle_system.emitters) {
    LOG_ERROR("Failed to allocate particle emitters array");
    return false;
  }

  g_particle_system.max_total_particles = max_total_particles;
  g_particle_system.global_time_scale = 1.0f;
  g_particle_system.max_simulation_time = 0.016f; // 60 FPS cap
  g_particle_system.enable_gpu_simulation = true;
  g_particle_system.sort_by_depth = true;
  g_particle_system.enable_culling = true;
  g_particle_system.culling_distance = 100.0f;

  // Create default resources
  // TODO: Create default texture and shader
  // TODO: Create quad geometry for billboarding

  g_particle_system.initialized = true;
  LOG_INFO(
      "Particle system initialized (max emitters: %u, max particles: %llu)",
      max_emitters, max_total_particles);
  return true;
}

void particle_system_shutdown(void) {
  if (!g_particle_system.initialized)
    return;

  // Destroy all emitters
  for (uint32_t i = 0; i < g_particle_system.emitter_count; i++) {
    if (g_particle_system.emitters[i]) {
      particle_system_destroy_emitter(g_particle_system.emitters[i]);
    }
  }

  // TODO: Destroy default resources

  free(g_particle_system.emitters);
  memset(&g_particle_system, 0, sizeof(ParticleSystem));

  LOG_INFO("Particle system shutdown");
}

ParticleEmitter *particle_system_create_emitter(const char *name,
                                                uint32_t max_particles) {
  if (!g_particle_system.initialized || !name) {
    LOG_ERROR("Particle system not initialized or invalid name");
    return NULL;
  }

  if (g_particle_system.emitter_count >= g_particle_system.emitter_capacity) {
    LOG_ERROR("Too many particle emitters");
    return NULL;
  }

  // Check total particle limit
  if (g_particle_system.total_particles + max_particles >
      g_particle_system.max_total_particles) {
    LOG_ERROR("Particle limit exceeded");
    return NULL;
  }

  ParticleEmitter *emitter = calloc(1, sizeof(ParticleEmitter));
  if (!emitter) {
    LOG_ERROR("Failed to allocate particle emitter");
    return NULL;
  }

  strncpy(emitter->name, name, sizeof(emitter->name) - 1);
  emitter->emission_type = PARTICLE_EMISSION_TYPE_POINT;
  emitter->shape = PARTICLE_SHAPE_BILLBOARD;
  emitter->simulation_type = g_particle_system.enable_gpu_simulation
                                 ? PARTICLE_SIMULATION_GPU_COMPUTE
                                 : PARTICLE_SIMULATION_CPU;

  emitter->max_particles = max_particles;
  emitter->emission_rate = 10.0f;
  emitter->enabled = true;

  // Allocate particle array
  emitter->particles = calloc(max_particles, sizeof(Particle));
  emitter->free_list = calloc(max_particles, sizeof(uint32_t));

  if (!emitter->particles || !emitter->free_list) {
    LOG_ERROR("Failed to allocate particle data");
    free(emitter->particles);
    free(emitter->free_list);
    free(emitter);
    return NULL;
  }

  // Initialize free list
  for (uint32_t i = 0; i < max_particles; i++) {
    emitter->free_list[i] = i;
  }
  emitter->free_list_size = max_particles;

  // Set default values
  emitter->initial_color[0] = 1.0f;
  emitter->initial_color[1] = 1.0f;
  emitter->initial_color[2] = 1.0f;
  emitter->initial_color[3] = 1.0f;

  emitter->initial_size_min = 1.0f;
  emitter->initial_size_max = 1.0f;
  emitter->initial_lifetime_min = 2.0f;
  emitter->initial_lifetime_max = 2.0f;

  emitter->gravity[1] = -9.81f;
  emitter->drag_coefficient = 0.1f;
  emitter->turbulence = 0.0f;

  emitter->depth_write = false;
  emitter->alpha_blending = true;
  emitter->soft_particles = true;

  // Initialize mutex
  if (pthread_mutex_init(&emitter->mutex, NULL) != 0) {
    LOG_ERROR("Failed to initialize particle emitter mutex");
    free(emitter->particles);
    free(emitter->free_list);
    free(emitter);
    return NULL;
  }

  // TODO: Create GPU resources

  g_particle_system.emitters[g_particle_system.emitter_count++] = emitter;
  g_particle_system.total_particles += max_particles;

  LOG_INFO("Created particle emitter: %s (max particles: %u)", name,
           max_particles);
  return emitter;
}

void particle_system_destroy_emitter(ParticleEmitter *emitter) {
  if (!emitter)
    return;

  // Remove from global list
  for (uint32_t i = 0; i < g_particle_system.emitter_count; i++) {
    if (g_particle_system.emitters[i] == emitter) {
      g_particle_system.emitters[i] =
          g_particle_system.emitters[g_particle_system.emitter_count - 1];
      g_particle_system.emitter_count--;
      break;
    }
  }

  // Cleanup
  pthread_mutex_destroy(&emitter->mutex);
  free(emitter->particles);
  free(emitter->free_list);

  // TODO: Destroy GPU resources

  g_particle_system.total_particles -= emitter->max_particles;

  free(emitter);
  LOG_INFO("Destroyed particle emitter: %s", emitter->name);
}

void particle_system_update(float dt) {
  if (!g_particle_system.initialized)
    return;

  uint64_t start_time = get_time_nanos();

  // Clamp delta time
  dt = fminf(dt, g_particle_system.max_simulation_time) *
       g_particle_system.global_time_scale;

  for (uint32_t i = 0; i < g_particle_system.emitter_count; i++) {
    ParticleEmitter *emitter = g_particle_system.emitters[i];
    if (!emitter->enabled)
      continue;

    pthread_mutex_lock(&emitter->mutex);

    // Update emission
    emitter->emission_timer += dt;
    float emission_interval = 1.0f / emitter->emission_rate;

    while (emitter->emission_timer >= emission_interval &&
           emitter->active_count < emitter->max_particles) {
      emit_particle(emitter);
      emitter->emission_timer -= emission_interval;
    }

    // Handle burst emission
    if (emitter->emission_burst > 0.0f) {
      uint32_t burst_count = (uint32_t)emitter->emission_burst;
      for (uint32_t j = 0;
           j < burst_count && emitter->active_count < emitter->max_particles;
           j++) {
        emit_particle(emitter);
      }
      emitter->emission_burst = 0.0f;
    }

    // Update particles
    for (uint32_t j = 0; j < emitter->max_particles; j++) {
      Particle *particle = &emitter->particles[j];
      if (particle->alive) {
        update_particle_physics(particle, dt, emitter);

        if (!particle->alive) {
          // Add to free list
          emitter->free_list[emitter->free_list_size++] = j;
          emitter->active_count--;
          emitter->total_dead++;
        }
      }
    }

    emitter->needs_update = true;

    pthread_mutex_unlock(&emitter->mutex);
  }

  uint64_t end_time = get_time_nanos();
  g_particle_system.total_simulation_time = nanos_to_ms(end_time - start_time);
}

void particle_system_render(const float *view_matrix,
                            const float *proj_matrix) {
  if (!g_particle_system.initialized)
    return;

  uint64_t start_time = get_time_nanos();

  // TODO: Implement particle rendering
  // This would involve:
  // 1. Sort particles by depth if enabled
  // 2. Update GPU buffers with particle data
  // 3. Render particles with appropriate shaders

  for (uint32_t i = 0; i < g_particle_system.emitter_count; i++) {
    ParticleEmitter *emitter = g_particle_system.emitters[i];
    if (!emitter->enabled || emitter->active_count == 0)
      continue;

    // TODO: Render this emitter's particles
    LOG_DEBUG("Rendering emitter: %s (%u active particles)", emitter->name,
              emitter->active_count);
  }

  uint64_t end_time = get_time_nanos();
  g_particle_system.total_render_time = nanos_to_ms(end_time - start_time);
}

void particle_emitter_set_position(ParticleEmitter *emitter, float x, float y,
                                   float z) {
  if (!emitter)
    return;

  pthread_mutex_lock(&emitter->mutex);
  emitter->position[0] = x;
  emitter->position[1] = y;
  emitter->position[2] = z;
  pthread_mutex_unlock(&emitter->mutex);
}

void particle_emitter_set_emission_rate(ParticleEmitter *emitter, float rate) {
  if (!emitter)
    return;

  pthread_mutex_lock(&emitter->mutex);
  emitter->emission_rate = rate;
  pthread_mutex_unlock(&emitter->mutex);
}

void particle_emitter_burst(ParticleEmitter *emitter, uint32_t count) {
  if (!emitter)
    return;

  pthread_mutex_lock(&emitter->mutex);
  emitter->emission_burst = (float)count;
  pthread_mutex_unlock(&emitter->mutex);
}

void particle_emitter_set_color(ParticleEmitter *emitter, float r, float g,
                                float b, float a) {
  if (!emitter)
    return;

  pthread_mutex_lock(&emitter->mutex);
  emitter->initial_color[0] = r;
  emitter->initial_color[1] = g;
  emitter->initial_color[2] = b;
  emitter->initial_color[3] = a;
  pthread_mutex_unlock(&emitter->mutex);
}

void particle_system_get_stats(uint64_t *total_particles,
                               uint32_t *active_emitters,
                               float *simulation_time, float *render_time) {
  if (!g_particle_system.initialized)
    return;

  uint64_t active_particles = 0;
  for (uint32_t i = 0; i < g_particle_system.emitter_count; i++) {
    active_particles += g_particle_system.emitters[i]->active_count;
  }

  if (total_particles)
    *total_particles = active_particles;
  if (active_emitters)
    *active_emitters = g_particle_system.emitter_count;
  if (simulation_time)
    *simulation_time = g_particle_system.total_simulation_time;
  if (render_time)
    *render_time = g_particle_system.total_render_time;
}
