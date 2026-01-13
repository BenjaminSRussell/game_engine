#include "character/animation/gpu_skinning.h"

#ifdef __OBJC__
#import <Metal/Metal.h>

void skinned_mesh_update(id<MTLComputeCommandEncoder> encoder,
                         skinned_mesh_t* mesh,
                         const simd_float4x4* bone_matrices,
                         uint32_t bone_count) {
    if (!encoder || !mesh || !bone_matrices || bone_count == 0) return;
    
    // Update bone matrices buffer
    // Assuming mesh->bone_matrices is large enough and has shared storage mode for simplicity
    // in a real engine, we'd use a triple buffer strategy or ring buffer
    id<MTLBuffer> bone_buffer = (id<MTLBuffer>)mesh->bone_matrices;
    if (bone_buffer) {
        memcpy(bone_buffer.contents, bone_matrices, sizeof(simd_float4x4) * bone_count);
    }
    
    // Bind buffers
    [encoder setBuffer:(id<MTLBuffer>)mesh->vertex_buffer offset:0 atIndex:0];
    [encoder setBuffer:(id<MTLBuffer>)mesh->skinned_buffer offset:0 atIndex:1];
    [encoder setBuffer:(id<MTLBuffer>)mesh->bone_matrices offset:0 atIndex:2];
    
    // Dispatch
    NSUInteger threadGroupSize = 64; // Workgroup size
    MTLSize threadGroupCount = MTLSizeMake(threadGroupSize, 1, 1);
    MTLSize threadGroups = MTLSizeMake((mesh->vertex_count + threadGroupSize - 1) / threadGroupSize, 1, 1);
    
    [encoder dispatchThreadgroups:threadGroups threadsPerThreadgroup:threadGroupCount];
}

#else

// Fallback for non-ObjC compilers (should not happen in this context but good for safety)
void skinned_mesh_update(void* encoder,
                         skinned_mesh_t* mesh,
                         const simd_float4x4* bone_matrices,
                         uint32_t bone_count) {
    // No-op or error
}

#endif
