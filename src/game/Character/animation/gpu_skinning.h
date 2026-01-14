#ifndef GPU_SKINNING_H
#define GPU_SKINNING_H

#include <simd/simd.h>
#include <stdint.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

typedef struct skinned_mesh {
    id vertex_buffer;      // id<MTLBuffer> Source vertices with bone weights
    id skinned_buffer;     // id<MTLBuffer> Output skinned vertices
    id bone_matrices;      // id<MTLBuffer> Current pose matrices
    uint32_t vertex_count;
} skinned_mesh_t;

// Updates the bone matrices buffer and dispatches the compute shader
// encoder: id<MTLComputeCommandEncoder>
void skinned_mesh_update(id encoder,
                         skinned_mesh_t* mesh,
                         const simd_float4x4* bone_matrices,
                         uint32_t bone_count);

#endif // GPU_SKINNING_H
