// gpu_particles.c - GPU-based Particle System Implementation
#include <include/effects/particles/niagara_emitter.h>
#include <include/core/logger.h>
#include <stdlib.h>
#include <string.h>

static NiagaraSystem *g_niagara_system = NULL;

NiagaraSystem* niagara_create(id<MTLDevice> device) {
  if (!device) {
    LOG_ERROR("Cannot create Niagara system without Metal device");
    return NULL;
  }
  
  NiagaraSystem *system = (NiagaraSystem*)calloc(1, sizeof(NiagaraSystem));
  if (!system) {
    LOG_ERROR("Failed to allocate Niagara system");
    return NULL;
  }
  
  system->device = device;
  system->emitter_count = 0;
  
  // Create render pipeline for particles
  NSError *error = nil;
  id<MTLLibrary> library = [device newDefaultLibrary];
  
  // Vertex function
  id<MTLFunction> vertex_fn = [library newFunctionWithName:@"particle_vertex"];
  // Fragment function  
  id<MTLFunction> fragment_fn = [library newFunctionWithName:@"particle_fragment"];
  
  if (vertex_fn && fragment_fn) {
    MTLRenderPipelineDescriptor *desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = vertex_fn;
    desc.fragmentFunction = fragment_fn;
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    desc.colorAttachments[0].blendingEnabled = YES;
    desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    desc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
    system->render_pipeline = [device newRenderPipelineStateWithDescriptor:desc error:&error];
    
    if (error) {
      LOG_ERROR("Failed to create particle render pipeline: %s", error.localizedDescription.UTF8String);
    }
  }
  
  [library release];
  [vertex_fn release];
  [fragment_fn release];
  
  LOG_INFO("Niagara GPU particle system created");
  return system;
}

void niagara_destroy(NiagaraSystem *system) {
  if (!system) return;
  
  // Destroy all emitters
  for (u32 i = 0; i < system->emitter_count; i++) {
    niagara_destroy_emitter(system, i);
  }
  
  if (system->render_pipeline) {
    [system->render_pipeline release];
  }
  
  free(system);
  LOG_INFO("Niagara GPU particle system destroyed");
}

NiagaraEmitter* niagara_create_emitter(NiagaraSystem *system, const char *name, u32 max_particles) {
  if (!system || !name || system->emitter_count >= NIAGARA_MAX_EMITTERS) {
    return NULL;
  }
  
  NiagaraEmitter *emitter = &system->emitters[system->emitter_count++];
  memset(emitter, 0, sizeof(NiagaraEmitter));
  
  strncpy(emitter->name, name, sizeof(emitter->name) - 1);
  emitter->max_particles = max_particles;
  emitter->particle_count = 0;
  emitter->alive_count = 0;
  emitter->looping = true;
  
  // Allocate particle buffer
  size_t particle_buffer_size = max_particles * sizeof(NiagaraParticle);
  emitter->particle_buffer = [system->device newBufferWithLength:particle_buffer_size 
                                                            options:MTLResourceStorageModeShared];
  
  // Allocate dead and alive list buffers
  size_t list_buffer_size = max_particles * sizeof(u32);
  emitter->dead_list = [system->device newBufferWithLength:list_buffer_size 
                                                     options:MTLResourceStorageModeShared];
  emitter->alive_list = [system->device newBufferWithLength:list_buffer_size 
                                                     options:MTLResourceStorageModeShared];
  
  // Initialize dead list with all particle indices
  u32 *dead_indices = (u32*)[emitter->dead_list contents];
  for (u32 i = 0; i < max_particles; i++) {
    dead_indices[i] = i;
  }
  
  // Set default spawn parameters
  emitter->spawn_params.spawn_rate = 100; // 100 particles per second
  emitter->spawn_params.spawn_location = (Vec3){0, 0, 0};
  emitter->spawn_params.spawn_velocity_min = (Vec3){-1, -1, -1};
  emitter->spawn_params.spawn_velocity_max = (Vec3){1, 1, 1};
  emitter->spawn_params.lifetime_min = 1.0f;
  emitter->spawn_params.lifetime_max = 3.0f;
  emitter->spawn_params.size_min = 0.1f;
  emitter->spawn_params.size_max = 0.5f;
  emitter->spawn_params.color_start = (Vec4){1, 1, 1, 1};
  emitter->spawn_params.color_end = (Vec4){1, 1, 1, 0};
  
  // Set default physics
  emitter->gravity = (Vec3){0, -9.81f, 0};
  emitter->drag = 0.1f;
  
  // Create compute pipelines for GPU simulation
  niagara_create_compute_pipelines(emitter);
  
  LOG_INFO("Created Niagara emitter '%s' with %u max particles", name, max_particles);
  return emitter;
}

void niagara_destroy_emitter(NiagaraSystem *system, u32 emitter_index) {
  if (!system || emitter_index >= system->emitter_count) return;
  
  NiagaraEmitter *emitter = &system->emitters[emitter_index];
  
  if (emitter->particle_buffer) {
    [emitter->particle_buffer release];
  }
  if (emitter->dead_list) {
    [emitter->dead_list release];
  }
  if (emitter->alive_list) {
    [emitter->alive_list release];
  }
  if (emitter->update_pipeline) {
    [emitter->update_pipeline release];
  }
  if (emitter->spawn_pipeline) {
    [emitter->spawn_pipeline release];
  }
  if (emitter->sprite_texture) {
    [emitter->sprite_texture release];
  }
  
  // Remove emitter from array
  for (u32 i = emitter_index; i < system->emitter_count - 1; i++) {
    system->emitters[i] = system->emitters[i + 1];
  }
  system->emitter_count--;
  
  LOG_INFO("Destroyed Niagara emitter at index %u", emitter_index);
}

void niagara_update_emitter(NiagaraEmitter *emitter, f32 delta_time) {
  if (!emitter || emitter->alive_count == 0) return;
  
  // Update time accumulator for spawning
  emitter->time_accumulator += delta_time;
  
  // Spawn new particles
  f32 spawn_interval = 1.0f / emitter->spawn_params.spawn_rate;
  u32 particles_to_spawn = (u32)(emitter->time_accumulator / spawn_interval);
  
  if (particles_to_spawn > 0) {
    niagara_spawn_particles(emitter, particles_to_spawn);
    emitter->time_accumulator -= particles_to_spawn * spawn_interval;
  }
  
  // Update existing particles on GPU
  niagara_update_particles_gpu(emitter, delta_time);
}

void niagara_spawn_particles(NiagaraEmitter *emitter, u32 count) {
  if (!emitter || count == 0) return;
  
  u32 available_slots = emitter->max_particles - emitter->alive_count;
  u32 actual_spawn = (count < available_slots) ? count : available_slots;
  
  if (actual_spawn == 0) return;
  
  // Get dead particle indices
  u32 *dead_indices = (u32*)[emitter->dead_list contents];
  
  // Spawn particles on GPU
  niagara_spawn_particles_gpu(emitter, dead_indices, actual_spawn);
  
  emitter->alive_count += actual_spawn;
  LOG_DEBUG("Spawned %u particles in emitter '%s'", actual_spawn, emitter->name);
}

void niagara_update_particles_gpu(NiagaraEmitter *emitter, f32 delta_time) {
  if (!emitter || !emitter->update_pipeline) return;
  
  // This would execute a compute shader to update particle positions, velocities, colors, etc.
  // For now, we'll simulate the update on CPU
  
  NiagaraParticle *particles = (NiagaraParticle*)[emitter->particle_buffer contents];
  u32 *alive_indices = (u32*)[emitter->alive_list contents];
  
  u32 alive_count = 0;
  
  for (u32 i = 0; i < emitter->alive_count; i++) {
    u32 particle_index = alive_indices[i];
    NiagaraParticle *particle = &particles[particle_index];
    
    // Update age
    particle->age += delta_time;
    
    // Check if particle should die
    if (particle->age >= particle->lifetime) {
      // Move to dead list
      u32 *dead_indices = (u32*)[emitter->dead_list contents];
      dead_indices[emitter->max_particles - emitter->alive_count + alive_count] = particle_index;
      continue;
    }
    
    // Update physics
    particle->velocity = vec3_add(particle->velocity, vec3_scale(emitter->gravity, delta_time));
    particle->velocity = vec3_scale(particle->velocity, 1.0f - emitter->drag * delta_time);
    particle->position = vec3_add(particle->position, vec3_scale(particle->velocity, delta_time));
    
    // Update color based on age
    f32 t = particle->age / particle->lifetime;
    particle->color = vec4_lerp(emitter->spawn_params.color_start, emitter->spawn_params.color_end, t);
    
    // Update size based on age
    particle->size = fmaxf(0.01f, particle->size * (1.0f - delta_time * 0.1f));
    
    // Keep particle alive
    alive_indices[alive_count++] = particle_index;
  }
  
  emitter->alive_count = alive_count;
}

void niagara_spawn_particles_gpu(NiagaraEmitter *emitter, u32 *indices, u32 count) {
  NiagaraParticle *particles = (NiagaraParticle*)[emitter->particle_buffer contents];
  
  for (u32 i = 0; i < count; i++) {
    u32 particle_index = indices[i];
    NiagaraParticle *particle = &particles[particle_index];
    
    // Random spawn parameters
    f32 t = (f32)rand() / RAND_MAX;
    
    // Position
    particle->position = emitter->spawn_params.spawn_location;
    
    // Velocity
    Vec3 velocity_range = vec3_sub(emitter->spawn_params.spawn_velocity_max, 
                                  emitter->spawn_params.spawn_velocity_min);
    Vec3 random_velocity = vec3_scale(velocity_range, t);
    particle->velocity = vec3_add(emitter->spawn_params.spawn_velocity_min, random_velocity);
    
    // Lifetime
    f32 lifetime_range = emitter->spawn_params.lifetime_max - emitter->spawn_params.lifetime_min;
    particle->lifetime = emitter->spawn_params.lifetime_min + lifetime_range * t;
    particle->age = 0.0f;
    
    // Size
    f32 size_range = emitter->spawn_params.size_max - emitter->spawn_params.size_min;
    particle->size = emitter->spawn_params.size_min + size_range * t;
    
    // Color
    particle->color = emitter->spawn_params.color_start;
    particle->rotation = 0.0f;
    particle->flags = 0;
  }
}

void niagara_render_emitter(NiagaraEmitter *emitter, id<MTLRenderCommandEncoder> encoder, 
                           const Mat4 *view_proj) {
  if (!emitter || !encoder || emitter->alive_count == 0) return;
  
  // Set render pipeline
  [encoder setRenderPipelineState:emitter->render_pipeline];
  
  // Set particle buffer
  [encoder setVertexBuffer:emitter->particle_buffer offset:0 atIndex:0];
  
  // Set view-projection matrix
  [encoder setVertexBytes:view_proj length:sizeof(Mat4) atIndex:1];
  
  // Draw particles
  [encoder drawPrimitives:MTLPrimitiveTypePoint vertexStart:0 vertexCount:emitter->alive_count];
}

void niagara_set_spawn_rate(NiagaraEmitter *emitter, u32 particles_per_second) {
  if (!emitter) return;
  emitter->spawn_params.spawn_rate = particles_per_second;
}

void niagara_set_spawn_location(NiagaraEmitter *emitter, const Vec3 *location) {
  if (!emitter || !location) return;
  emitter->spawn_params.spawn_location = *location;
}

void niagara_set_velocity_range(NiagaraEmitter *emitter, const Vec3 *min_vel, const Vec3 *max_vel) {
  if (!emitter || !min_vel || !max_vel) return;
  emitter->spawn_params.spawn_velocity_min = *min_vel;
  emitter->spawn_params.spawn_velocity_max = *max_vel;
}

void niagara_set_lifetime_range(NiagaraEmitter *emitter, f32 min_lifetime, f32 max_lifetime) {
  if (!emitter) return;
  emitter->spawn_params.lifetime_min = min_lifetime;
  emitter->spawn_params.lifetime_max = max_lifetime;
}

void niagara_set_color_gradient(NiagaraEmitter *emitter, const Vec4 *start_color, const Vec4 *end_color) {
  if (!emitter || !start_color || !end_color) return;
  emitter->spawn_params.color_start = *start_color;
  emitter->spawn_params.color_end = *end_color;
}

void niagara_set_size_range(NiagaraEmitter *emitter, f32 min_size, f32 max_size) {
  if (!emitter) return;
  emitter->spawn_params.size_min = min_size;
  emitter->spawn_params.size_max = max_size;
}

void niagara_set_gravity(NiagaraEmitter *emitter, const Vec3 *gravity) {
  if (!emitter || !gravity) return;
  emitter->gravity = *gravity;
}

void niagara_set_drag(NiagaraEmitter *emitter, f32 drag) {
  if (!emitter) return;
  emitter->drag = drag;
}

void niagara_add_force_field(NiagaraEmitter *emitter, const NiagaraForceField *force_field) {
  if (!emitter || !force_field || emitter->force_field_count >= 16) return;
  
  emitter->force_fields[emitter->force_field_count++] = *force_field;
}

void niagara_set_sprite_texture(NiagaraEmitter *emitter, id<MTLTexture> texture) {
  if (!emitter) return;
  
  if (emitter->sprite_texture) {
    [emitter->sprite_texture release];
  }
  
  emitter->sprite_texture = texture;
  [emitter->sprite_texture retain];
}

void niagara_set_blend_mode(NiagaraEmitter *emitter, u32 blend_mode) {
  if (!emitter) return;
  emitter->blend_mode = blend_mode;
}

u32 niagara_get_particle_count(const NiagaraEmitter *emitter) {
  return emitter ? emitter->alive_count : 0;
}

bool niagara_is_emitter_active(const NiagaraEmitter *emitter) {
  return emitter ? emitter->alive_count > 0 : false;
}

void niagara_reset_emitter(NiagaraEmitter *emitter) {
  if (!emitter) return;
  
  emitter->alive_count = 0;
  emitter->time_accumulator = 0.0f;
  
  // Reset dead list to contain all indices
  u32 *dead_indices = (u32*)[emitter->dead_list contents];
  for (u32 i = 0; i < emitter->max_particles; i++) {
    dead_indices[i] = i;
  }
}

// Internal helper functions
static void niagara_create_compute_pipelines(NiagaraEmitter *emitter) {
  if (!emitter || !emitter->particle_buffer) return;
  
  NSError *error = nil;
  id<MTLLibrary> library = [emitter->particle_buffer.device newDefaultLibrary];
  
  // Update pipeline
  id<MTLFunction> update_fn = [library newFunctionWithName:@"particle_update"];
  if (update_fn) {
    MTLComputePipelineDescriptor *desc = [[MTLComputePipelineDescriptor alloc] init];
    desc.computeFunction = update_fn;
    
    emitter->update_pipeline = [emitter->particle_buffer.device newComputePipelineStateWithDescriptor:desc error:&error];
    
    if (error) {
      LOG_ERROR("Failed to create particle update pipeline: %s", error.localizedDescription.UTF8String);
    }
  }
  
  // Spawn pipeline
  id<MTLFunction> spawn_fn = [library newFunctionWithName:@"particle_spawn"];
  if (spawn_fn) {
    MTLComputePipelineDescriptor *desc = [[MTLComputePipelineDescriptor alloc] init];
    desc.computeFunction = spawn_fn;
    
    emitter->spawn_pipeline = [emitter->particle_buffer.device newComputePipelineStateWithDescriptor:desc error:&error];
    
    if (error) {
      LOG_ERROR("Failed to create particle spawn pipeline: %s", error.localizedDescription.UTF8String);
    }
  }
  
  [library release];
  [update_fn release];
  [spawn_fn release];
}

static Vec4 vec4_lerp(Vec4 a, Vec4 b, f32 t) {
  return (Vec4){
    a.x + (b.x - a.x) * t,
    a.y + (b.y - a.y) * t,
    a.z + (b.z - a.z) * t,
    a.w + (b.w - a.w) * t
  };
}
