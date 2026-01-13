// include/vfx/particle_system.h
//
// Purpose: General-purpose particle system for visual effects throughout the
// game. Extends the weather particle system for gameplay effects (explosions,
// impacts, magic, etc.).
//
// Key Features:
// - Pooled particle management (up to 16,384 particles)
// - Multiple particle types with unique behaviors
// - Physics-based movement (gravity, velocity, friction)
// - Texture atlas support for varied particle visuals
// - Color and alpha interpolation over lifetime
// - Emitter system for continuous particle generation
// - Burst mode for instant particle clouds
//
// Public APIs:
// - particle_system_init: Initialize particle pool
// - particle_emit: Spawn single particle
// - particle_emit_burst: Spawn multiple particles at once
// - particle_system_update: Update all active particles
// - emitter_create: Create continuous particle emitter
//
// Invariants:
// - Particle system must be initialized before use
// - Maximum particle count is enforced
// - Particles are recycled when expired
// - Emitters reference particle pool
//
#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "engine/include/common.h"
#include <include/rendering/vulkan.h>
#include <math/vec3.h>
#include <math/vec4.h>

#define MAX_PARTICLES 16384
#define MAX_EMITTERS 128

typedef enum {
  PARTICLE_TYPE_SPARK,
  PARTICLE_TYPE_SMOKE,
  PARTICLE_TYPE_DUST,
  PARTICLE_TYPE_FLAME,
  PARTICLE_TYPE_WATER_SPLASH,
  PARTICLE_TYPE_EXPLOSION,
  PARTICLE_TYPE_MAGIC_GLOW,
  PARTICLE_TYPE_BLOOD,
  PARTICLE_TYPE_DEBRIS,
  PARTICLE_TYPE_BUBBLE,
  PARTICLE_TYPE_LIGHTNING,
  PARTICLE_TYPE_SOUL,
  PARTICLE_TYPE_PORTAL,
  PARTICLE_TYPE_ENCHANT,
  PARTICLE_TYPE_CRITICAL_HIT,
  PARTICLE_TYPE_HEART,
  PARTICLE_TYPE_ANGRY,
  PARTICLE_TYPE_HAPPY,
  PARTICLE_TYPE_CUSTOM
} ParticleType;

typedef struct {
  Vec3 position;
  Vec3 velocity;
  Vec3 acceleration;

  Vec4 color;       // RGBA (0.0-1.0)
  Vec4 start_color; // Initial color for interpolation
  Vec4 end_color;   // Final color for interpolation

  f32 size;
  f32 start_size;
  f32 end_size;

  f32 rotation; // Radians
  f32 angular_velocity;

  f32 life;     // Current lifetime
  f32 max_life; // Maximum lifetime

  ParticleType type;
  u16 texture_id; // Texture atlas index

  bool active;
  bool affected_by_gravity;
  bool collides_with_world;

  f32 friction; // Velocity damping (0.0-1.0)
  f32 alpha;    // Transparency (0.0-1.0)
} Particle;

typedef enum {
  EMITTER_SHAPE_POINT,
  EMITTER_SHAPE_SPHERE,
  EMITTER_SHAPE_BOX,
  EMITTER_SHAPE_CONE,
  EMITTER_SHAPE_CIRCLE
} EmitterShape;

typedef struct {
  bool active;
  Vec3 position;
  Vec3 direction; // For cone/directional emitters

  EmitterShape shape;
  f32 shape_param1; // Radius for sphere/circle, width for box
  f32 shape_param2; // Height for box, angle for cone

  ParticleType particle_type;
  u32 particles_per_second;
  f32 particle_lifetime;

  Vec3 velocity_min;
  Vec3 velocity_max;
  Vec4 color_start;
  Vec4 color_end;
  f32 size_start;
  f32 size_end;

  f32 spawn_timer;
  f32 duration; // -1.0 for infinite
  f32 time_alive;

  bool burst_mode; // Emit all at once instead of over time
  u32 burst_count;
} ParticleEmitter;

typedef struct ParticleSystem {
  Particle particles[MAX_PARTICLES];
  ParticleEmitter emitters[MAX_EMITTERS];

  u32 active_particle_count;
  u32 active_emitter_count;

  // GPU resources for particle rendering
  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_memory;
  u32 vertex_buffer_size;

  bool initialized;
} ParticleSystem;

// Lifecycle
void particle_system_init(ParticleSystem *system, VulkanRenderer *renderer);
void particle_system_shutdown(ParticleSystem *system, VulkanRenderer *renderer);

// Particle emission
u32 particle_emit(ParticleSystem *system, ParticleType type, Vec3 position,
                  Vec3 velocity, f32 lifetime);
void particle_emit_burst(ParticleSystem *system, ParticleType type,
                         Vec3 position, Vec3 velocity_base, f32 velocity_spread,
                         u32 count, f32 lifetime);

// Particle configuration
void particle_set_color(ParticleSystem *system, u32 particle_id,
                        Vec4 start_color, Vec4 end_color);
void particle_set_size(ParticleSystem *system, u32 particle_id, f32 start_size,
                       f32 end_size);
void particle_set_physics(ParticleSystem *system, u32 particle_id, bool gravity,
                          bool collision, f32 friction);

// Emitter management
u32 emitter_create(ParticleSystem *system, Vec3 position, ParticleType type,
                   u32 particles_per_sec, f32 duration);
void emitter_set_shape(ParticleSystem *system, u32 emitter_id,
                       EmitterShape shape, f32 param1, f32 param2);
void emitter_set_velocity_range(ParticleSystem *system, u32 emitter_id,
                                Vec3 min_vel, Vec3 max_vel);
void emitter_set_color_range(ParticleSystem *system, u32 emitter_id,
                             Vec4 start_color, Vec4 end_color);
void emitter_destroy(ParticleSystem *system, u32 emitter_id);

// Update and rendering
void particle_system_update(ParticleSystem *system, f32 delta_time);
void particle_system_render(ParticleSystem *system, VulkanRenderer *renderer,
                            Mat4 view_projection);

// Helper functions
u32 particle_get_active_count(ParticleSystem *system);
void particle_clear_all(ParticleSystem *system);

#endif // PARTICLE_SYSTEM_H
