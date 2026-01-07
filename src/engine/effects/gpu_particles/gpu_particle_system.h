/*
 * gpu_particle_system.h
 * GPU-driven particle system using Metal
 */

#ifndef GPU_PARTICLE_SYSTEM_H
#define GPU_PARTICLE_SYSTEM_H

#include <stdint.h>
#include <simd/simd.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
// Forward declarations for non-ObjC context
typedef struct  MTLBuffer MTLBuffer;
typedef struct  MTLComputePipelineState MTLComputePipelineState;
typedef struct  MTLRenderPipelineState MTLRenderPipelineState;
typedef struct  MTLComputeCommandEncoder MTLComputeCommandEncoder;
typedef struct  MTLRenderCommandEncoder MTLRenderCommandEncoder;
#define id __typeof__(id)
#endif

typedef struct particle_soa {
    id<MTLBuffer> positions;      // float4: xyz + padding
    id<MTLBuffer> velocities;     // float4: xyz + padding
    id<MTLBuffer> colors;         // float4: rgba
    id<MTLBuffer> sizes;          // float2: size + rotation
    id<MTLBuffer> lifetimes;      // float2: age, max_life
    id<MTLBuffer> alive_list;     // uint: indices of alive particles
    id<MTLBuffer> dead_list;      // uint: indices of available slots
    id<MTLBuffer> counters;       // uint4: alive_count, dead_count, emit_count, ...
    id<MTLBuffer> indirect_args;  // MTLDrawPrimitivesIndirectArguments
    uint32_t max_particles;
} particle_soa_t;

typedef struct particle_emitter {
    simd_float3 position;
    simd_float3 velocity_min;
    simd_float3 velocity_max;
    simd_float4 color_start;
    simd_float4 color_end;
    float size_start;
    float size_end;
    float lifetime_min;
    float lifetime_max;
    float emit_rate;
    float emit_accumulator;
    uint32_t emit_count; // Added to match emit function logic
    uint32_t frame;      // Added for randomness
} particle_emitter_t;

typedef struct update_uniforms {
    simd_float3 gravity;
    float delta_time;
    simd_float4 color_start;
    simd_float4 color_end;
    float size_start;
    float size_end;
} update_uniforms_t;

typedef struct gpu_particle_system {
    particle_soa_t particles;
    id<MTLComputePipelineState> spawn_pipeline;
    id<MTLComputePipelineState> update_pipeline;
    id<MTLComputePipelineState> compact_pipeline;
    id<MTLRenderPipelineState> render_pipeline;
} gpu_particle_system_t;

// C API
void gpu_particles_emit(gpu_particle_system_t* sys, particle_emitter_t* emitter,
                         id<MTLComputeCommandEncoder> encoder, float dt);

void gpu_particles_update(gpu_particle_system_t* sys, update_uniforms_t* uniforms,
                           id<MTLComputeCommandEncoder> encoder);

void gpu_particles_render(gpu_particle_system_t* sys, id<MTLRenderCommandEncoder> encoder);

#endif /* GPU_PARTICLE_SYSTEM_H */
