// effects/particles/niagara_emitter.c
// Complete Niagara particle system
#include "include/effects/particles/niagara_emitter.h"
#include "include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

NiagaraSystem *niagara_create(id<MTLDevice> device) {
  NiagaraSystem *system = (NiagaraSystem *)calloc(1, sizeof(NiagaraSystem));
  system->device = device;

  LOG_INFO("Niagara particle system created");
  return system;
}

void niagara_destroy(NiagaraSystem *system) {
  if (!system)
    return;

  for (u32 i = 0; i < system->emitter_count; i++) {
    NiagaraEmitter *emitter = &system->emitters[i];
    free(emitter->particles);
    free(emitter->force_fields);
    emitter->particle_buffer = nil;
    emitter->dead_list = nil;
    emitter->alive_list = nil;
  }

  free(system);
}

u32 niagara_add_emitter(NiagaraSystem *system, const char *name,
                        u32 max_particles) {
  if (!system || system->emitter_count >= NIAGARA_MAX_EMITTERS) {
    LOG_ERROR("Cannot add emitter: limit reached");
    return 0;
  }

  u32 id = system->emitter_count++;
  NiagaraEmitter *emitter = &system->emitters[id];

  strncpy(emitter->name, name, sizeof(emitter->name) - 1);
  emitter->max_particles = max_particles;
  emitter->particles =
      (NiagaraParticle *)calloc(max_particles, sizeof(NiagaraParticle));
  emitter->particle_count = 0;
  emitter->alive_count = 0;
  emitter->looping = true;

  // Default physics
  emitter->gravity = vec3(0, -9.81f, 0);
  emitter->drag = 0.1f;

  // Create GPU buffers
  emitter->particle_buffer = [system->device
      newBufferWithLength:max_particles * sizeof(NiagaraParticle)
                  options:MTLResourceStorageModeShared];
  emitter->dead_list =
      [system->device newBufferWithLength:max_particles * sizeof(u32)
                                  options:MTLResourceStorageModeShared];
  emitter->alive_list =
      [system->device newBufferWithLength:max_particles * sizeof(u32)
                                  options:MTLResourceStorageModeShared];

  LOG_INFO("Created Niagara emitter '%s' with %u max particles", name,
           max_particles);
  return id;
}

NiagaraEmitter *niagara_get_emitter(NiagaraSystem *system, u32 emitter_id) {
  if (!system || emitter_id >= system->emitter_count)
    return NULL;
  return &system->emitters[emitter_id];
}

void niagara_set_spawn_params(NiagaraEmitter *emitter,
                              const NiagaraSpawnParams *params) {
  if (!emitter || !params)
    return;
  emitter->spawn_params = *params;
}

void niagara_add_force_field(NiagaraEmitter *emitter,
                             const NiagaraForceField *force) {
  if (!emitter || !force)
    return;

  // Reallocate force field array
  emitter->force_field_count++;
  emitter->force_fields = (NiagaraForceField *)realloc(
      emitter->force_fields,
      emitter->force_field_count * sizeof(NiagaraForceField));
  emitter->force_fields[emitter->force_field_count - 1] = *force;
}

void niagara_update(NiagaraSystem *system, id<MTLCommandBuffer> cmd,
                    f32 delta_time) {
  if (!system || !cmd)
    return;

  for (u32 i = 0; i < system->emitter_count; i++) {
    NiagaraEmitter *emitter = &system->emitters[i];

    // Spawn new particles
    emitter->time_accumulator += delta_time;
    f32 spawn_interval = 1.0f / emitter->spawn_params.spawn_rate;

    while (emitter->time_accumulator >= spawn_interval &&
           emitter->alive_count < emitter->max_particles) {
      emitter->time_accumulator -= spawn_interval;

      // Find dead particle slot
      u32 particle_id = emitter->alive_count++;
      if (particle_id >= emitter->particle_count) {
        emitter->particle_count = particle_id + 1;
      }

      NiagaraParticle *p = &emitter->particles[particle_id];

      // Spawn settings
      p->position = emitter->spawn_params.spawn_location;
      p->velocity = vec3_lerp(emitter->spawn_params.spawn_velocity_min,
                              emitter->spawn_params.spawn_velocity_max,
                              (f32)rand() / RAND_MAX);
      p->color = emitter->spawn_params.color_start;
      p->size = emitter->spawn_params.size_min +
                ((f32)rand() / RAND_MAX) * (emitter->spawn_params.size_max -
                                            emitter->spawn_params.size_min);
      p->lifetime =
          emitter->spawn_params.lifetime_min +
          ((f32)rand() / RAND_MAX) * (emitter->spawn_params.lifetime_max -
                                      emitter->spawn_params.lifetime_min);
      p->age = 0.0f;
      p->rotation = 0.0f;
    }

    // Update particle physics (CPU for now, should be GPU compute)
    for (u32 j = 0; j < emitter->alive_count; j++) {
      NiagaraParticle *p = &emitter->particles[j];
      p->age += delta_time;

      if (p->age >= p->lifetime) {
        // Kill particle (swap with last alive)
        if (j < emitter->alive_count - 1) {
          emitter->particles[j] = emitter->particles[emitter->alive_count - 1];
          j--; // Reprocess this slot
        }
        emitter->alive_count--;
        continue;
      }

      // Apply forces
      Vec3 acceleration = emitter->gravity;

      // Force fields
      for (u32 k = 0; k < emitter->force_field_count; k++) {
        NiagaraForceField *ff = &emitter->force_fields[k];
        Vec3 to_force = vec3_sub(ff->position, p->position);
        f32 dist = vec3_length(to_force);

        if (dist < ff->radius) {
          f32 strength = ff->strength * (1.0f - dist / ff->radius);
          Vec3 force_dir = vec3_normalize(to_force);

          if (ff->type == 0) { // Attract
            acceleration =
                vec3_add(acceleration, vec3_scale(force_dir, strength));
          } else if (ff->type == 1) { // Repel
            acceleration =
                vec3_sub(acceleration, vec3_scale(force_dir, strength));
          }
        }
      }

      // Integrate
      p->velocity = vec3_add(p->velocity, vec3_scale(acceleration, delta_time));
      p->velocity = vec3_scale(p->velocity, 1.0f - emitter->drag * delta_time);
      p->position = vec3_add(p->position, vec3_scale(p->velocity, delta_time));

      // Color interpolation
      f32 life_t = p->age / p->lifetime;
      p->color = vec4_lerp(emitter->spawn_params.color_start,
                           emitter->spawn_params.color_end, life_t);
    }

    // Upload to GPU
    if (emitter->alive_count > 0) {
      memcpy([emitter->particle_buffer contents], emitter -> particles,
             emitter -> alive_count * sizeof(NiagaraParticle));
    }
  }
}

void niagara_spawn_burst(NiagaraEmitter *emitter, u32 count) {
  if (!emitter)
    return;

  for (u32 i = 0; i < count && emitter->alive_count < emitter->max_particles;
       i++) {
    // Same as spawn logic in update
    u32 particle_id = emitter->alive_count++;
    if (particle_id >= emitter->particle_count) {
      emitter->particle_count = particle_id + 1;
    }

    NiagaraParticle *p = &emitter->particles[particle_id];
    p->position = emitter->spawn_params.spawn_location;
    p->velocity = vec3_lerp(emitter->spawn_params.spawn_velocity_min,
                            emitter->spawn_params.spawn_velocity_max,
                            (f32)rand() / RAND_MAX);
    p->age = 0.0f;
    p->lifetime = emitter->spawn_params.lifetime_min;
  }

  LOG_INFO("Spawned burst of %u particles in emitter '%s'", count,
           emitter->name);
}

void niagara_render(NiagaraSystem *system, id<MTLRenderCommandEncoder> encoder,
                    const Mat4 *view_proj, const Vec3 *camera_pos) {
  if (!system || !encoder)
    return;

  // Render all emitters
  for (u32 i = 0; i < system->emitter_count; i++) {
    NiagaraEmitter *emitter = &system->emitters[i];

    if (emitter->alive_count == 0)
      continue;

    // Set particle vertex buffer
    [encoder setVertexBuffer:emitter->particle_buffer offset:0 atIndex:0];

    // Draw particles as points/quads
    // TODO: Actual rendering with sprite texture
    // [encoder drawPrimitives:MTLPrimitiveTypePoint vertexStart:0
    // vertexCount:emitter->alive_count];
  }
}
