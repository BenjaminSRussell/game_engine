// include/effects/particles/particle_system_unified.h
//
// Purpose: Unified particle system consolidating all particle implementations
// This replaces multiple duplicate particle systems with a single, efficient API

#ifndef PARTICLE_SYSTEM_UNIFIED_H
#define PARTICLE_SYSTEM_UNIFIED_H

#include "engine/include/common.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "rendering/vulkan.h"

// ============================================================================
// UNIFIED PARTICLE SYSTEM CONFIGURATION
// ============================================================================

typedef struct {
    u32 max_particles;
    bool use_gpu;
    bool enable_sorting;
    bool enable_collision;
    f32 update_frequency;
} ParticleSystemConfig;

// ============================================================================
// PARTICLE DATA STRUCTURES
// ============================================================================

typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    Vec4 color;
    f32 size;
    f32 rotation;
    f32 age;
    f32 lifetime;
    u32 texture_id;
    bool active;
} Particle;

typedef struct {
    Vec3 position;
    Vec3 direction;
    f32 spread_angle;
    f32 emission_rate;
    f32 particle_lifetime;
    Vec4 color_start;
    Vec4 color_end;
    f32 size_start;
    f32 size_end;
    u32 texture_id;
    bool active;
    f32 time_alive;
} ParticleEmitter;

// ============================================================================
// MAIN PARTICLE SYSTEM API
// ============================================================================

typedef struct ParticleSystem ParticleSystem;

// System management
ParticleSystem* particle_system_create(const ParticleSystemConfig* config);
void particle_system_destroy(ParticleSystem* system);
void particle_system_update(ParticleSystem* system, f32 delta_time);
void particle_system_render(ParticleSystem* system, VkCommandBuffer cmd_buffer);

// Emitter management
u32 particle_system_create_emitter(ParticleSystem* system, const ParticleEmitter* emitter);
void particle_system_destroy_emitter(ParticleSystem* system, u32 emitter_id);
void particle_system_update_emitter(ParticleSystem* system, u32 emitter_id, f32 delta_time);

// Particle control
u32 particle_system_emit_particles(ParticleSystem* system, u32 emitter_id, u32 count);
void particle_system_clear_all(ParticleSystem* system);
void particle_system_clear_emitter(ParticleSystem* system, u32 emitter_id);

// Configuration
void particle_system_set_gravity(ParticleSystem* system, Vec3 gravity);
void particle_system_set_wind(ParticleSystem* system, Vec3 wind);
void particle_system_enable_collision(ParticleSystem* system, bool enable);
void particle_system_enable_sorting(ParticleSystem* system, bool enable);

// Statistics
typedef struct {
    u32 active_particles;
    u32 active_emitters;
    u32 total_emitted;
    f64 update_time;
    f64 render_time;
} ParticleStats;

ParticleStats particle_system_get_stats(ParticleSystem* system);

// ============================================================================
// GPU ACCELERATION (when available)
// ============================================================================

bool particle_system_is_gpu_available(ParticleSystem* system);
bool particle_system_enable_gpu(ParticleSystem* system, bool enable);

#endif // PARTICLE_SYSTEM_UNIFIED_H
