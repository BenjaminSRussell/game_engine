// effects/particles/niagara_emitter.h
// Niagara-equivalent GPU particle system
#ifndef NIAGARA_EMITTER_H
#define NIAGARA_EMITTER_H

#include "include/common.h"
#include "math/vec3.h"
#include <Metal/Metal.h>

#define NIAGARA_MAX_PARTICLES 1000000
#define NIAGARA_MAX_EMITTERS 256

// Particle data (GPU format)
typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec4 color;
    f32 size;
    f32 rotation;
    f32 lifetime;
    f32 age;
    u32 flags;
} NiagaraParticle;

// Emitter spawn parameters
typedef struct {
    u32 spawn_rate;        // Particles per second
    f32 spawn_burst_count;
    f32 spawn_burst_time;
    
    Vec3 spawn_location;
    Vec3 spawn_velocity_min;
    Vec3 spawn_velocity_max;
    
    f32 lifetime_min;
    f32 lifetime_max;
    f32 size_min;
    f32 size_max;
    
    Vec4 color_start;
    Vec4 color_end;
} NiagaraSpawnParams;

// Force field / attraction point
typedef struct {
    Vec3 position;
    f32 strength;
    f32 radius;
    u32 type; // 0=attract, 1=repel, 2=vortex
} NiagaraForceField;

// Emitter (collection of particles with shared behavior)
typedef struct {
    char name[64];
    NiagaraSpawnParams spawn_params;
    
    NiagaraParticle* particles;
    u32 particle_count;
    u32 max_particles;
    u32 alive_count;
    
    // Forces
    Vec3 gravity;
    f32 drag;
    NiagaraForceField* force_fields;
    u32 force_field_count;
    
    // Rendering
    id<MTLTexture> sprite_texture;
    u32 blend_mode; // 0=additive, 1=alpha, 2=multiply
    
    // GPU resources
    id<MTLBuffer> particle_buffer;
    id<MTLBuffer> dead_list;
    id<MTLBuffer> alive_list;
    id<MTLComputePipelineState> update_pipeline;
    id<MTLComputePipelineState> spawn_pipeline;
    
    f32 time_accumulator;
    bool looping;
    
} NiagaraEmitter;

typedef struct {
    NiagaraEmitter emitters[NIAGARA_MAX_EMITTERS];
    u32 emitter_count;
    
    id<MTLDevice> device;
    id<MTLRenderPipelineState> render_pipeline;
} NiagaraSystem;

#ifdef __cplusplus
extern "C" {
#endif

// System lifecycle
NiagaraSystem* niagara_create(id<MTLDevice> device);
void niagara_destroy(NiagaraSystem* system);

// Emitter management
u32 niagara_add_emitter(NiagaraSystem* system, const char* name, u32 max_particles);
void niagara_remove_emitter(NiagaraSystem* system, u32 emitter_id);
NiagaraEmitter* niagara_get_emitter(NiagaraSystem* system, u32 emitter_id);

// Emitter configuration
void niagara_set_spawn_params(NiagaraEmitter* emitter, const NiagaraSpawnParams* params);
void niagara_add_force_field(NiagaraEmitter* emitter, const NiagaraForceField* force);
void niagara_set_sprite_texture(NiagaraEmitter* emitter, id<MTLTexture> texture);

// Simulation
void niagara_update(NiagaraSystem* system, id<MTLCommandBuffer> cmd, f32 delta_time);
void niagara_spawn_burst(NiagaraEmitter* emitter, u32 count);

// Rendering
void niagara_render(NiagaraSystem* system,
                   id<MTLRenderCommandEncoder> encoder,
                   const Mat4* view_proj,
                   const Vec3* camera_pos);

// Event system
void niagara_on_particle_death(NiagaraEmitter* emitter, void (*callback)(u32 particle_id));
void niagara_on_collision(NiagaraEmitter* emitter, void (*callback)(u32 particle_id, Vec3 hit_pos));

#ifdef __cplusplus
}
#endif

#endif // NIAGARA_EMITTER_H
