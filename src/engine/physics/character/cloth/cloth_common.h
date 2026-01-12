#ifndef CLOTH_COMMON_H
#define CLOTH_COMMON_H

#ifdef __OBJC__
#include <Metal/Metal.h>
#else
typedef void* id;
typedef struct {} MTLBuffer;
#endif
#include <simd/simd.h>

// Shared Setup
#define CLOTH_SOLVER_ITERATIONS 4

// Particle Structure (Aligned to 16 bytes/32 bytes for GPU)
typedef struct {
    simd_float3 position;
    float _pad0;
    simd_float3 prev_position;
    float inv_mass;
    simd_float3 velocity;
    float _pad1;
    simd_float3 normal;
    float _pad2;
    simd_float2 uv;
    simd_float2 _pad3;
} ClothParticle;

// Constraint Structure
typedef struct {
    uint32_t particle_a;
    uint32_t particle_b;
    float rest_length;
    float stiffness;
} ClothConstraint;

// Collision Shapes
typedef enum {
    COLLIDER_SPHERE = 0,
    COLLIDER_CAPSULE = 1
} ColliderType;

typedef struct {
    simd_float3 position;
    float radius;
    simd_float3 axis; // For capsule (end point relative to position or direction)
    float height;
    int type;
    int _pad[3]; 
} ClothCollider;

// Cloth Mesh Object
typedef struct {
    id<MTLBuffer> particles_buffer;
    id<MTLBuffer> constraints_buffer;
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> colliders_buffer;
    
    uint32_t particle_count;
    uint32_t constraint_count;
    uint32_t collider_count;
    
    uint32_t grid_width;
    uint32_t grid_height;
    
    id<MTLComputePipelineState> compute_pipeline;
    id<MTLComputePipelineState> constraint_pipeline;
    id<MTLComputePipelineState> collision_pipeline;
    id<MTLComputePipelineState> normal_pipeline;
} ClothMesh;

// Functions potentially shared or exposed
void cloth_generate_constraints(ClothMesh* cloth, float structural_stiff, float shear_stiff, float bend_stiff);
void cloth_generate_colliders(ClothMesh* cloth); // Placeholder or actual implementation

#endif // CLOTH_COMMON_H
