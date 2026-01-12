#ifndef HAIR_COMMON_H
#define HAIR_COMMON_H

#ifdef __OBJC__
#include <Metal/Metal.h>
#else
typedef void* id;
typedef struct {} MTLBuffer;
#endif
#include <simd/simd.h>

#define HAIR_SEGMENTS_PER_STRAND 16

// Matches shader layout
typedef struct __attribute__((aligned(16))) {
    simd_float3 position;
    float inv_mass;
    simd_float3 velocity;
    float thickness;
    simd_float4 tangent; // xyz = tangent, w = unused or density
} HairControlPoint;

// CPU side representation for initialization
typedef struct {
    simd_float3 positions[HAIR_SEGMENTS_PER_STRAND];
    simd_float3 velocities[HAIR_SEGMENTS_PER_STRAND];
    float length;
    float stiffness;
} HairStrand;

typedef struct {
    id<MTLBuffer> strands_buffer;      // Structured buffer of HairControlPoint
    id<MTLBuffer> render_index_buffer; // Indices for line strip or tessellation
    id<MTLTexture> density_map;
    
    uint32_t strand_count;
    uint32_t points_per_strand; // = HAIR_SEGMENTS_PER_STRAND
    
    id<MTLComputePipelineState> simulation_pipeline;
    id<MTLRenderPipelineState> render_pipeline;
} HairSystem;

typedef struct {
    simd_float3 gravity;
    float dt;
    float damping;
    float wind_strength;
    simd_float3 wind_dir;
    float _pad;
} HairSimulationParams;

// Function declarations
HairSystem* hair_system_create(id<MTLDevice> device, uint32_t count, id<MTLTexture> density_map);
void hair_system_destroy(HairSystem* system);
void hair_simulate_step(id<MTLComputeCommandEncoder> encoder, HairSystem* system, float dt, simd_float3 gravity, simd_float3 wind);
void hair_render(id<MTLRenderCommandEncoder> encoder, HairSystem* system, const simd_float4x4* view_proj);

#endif // HAIR_COMMON_H
