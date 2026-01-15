// Metal Voxel Renderer Implementation
// Metal-specific optimizations and features for voxel rendering

#ifndef VOXEL_RENDERER_METAL_H
#define VOXEL_RENDERER_METAL_H

#include "voxel_renderer.h"
#include "core/types.h"

#ifdef __APPLE__
#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Metal-specific voxel renderer with GPU mesh generation
typedef struct VoxelRendererMetal {
    // Base voxel renderer
    VoxelRenderer base;
    
    // Metal resources
#ifdef __APPLE__
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
    id<MTLRenderPipelineState> pipeline_state;
    id<MTLComputePipelineState> compute_pipeline;
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> index_buffer;
    id<MTLBuffer> uniform_buffer;
    id<MTLBuffer> lighting_buffer;
    id<MTLTexture> block_texture_array;
    id<MTLBuffer> chunk_data_buffer;
    
    // GPU mesh generation
    id<MTLBuffer> gpu_vertex_buffer;
    id<MTLBuffer> gpu_index_buffer;
    id<MTLBuffer> gpu_counter_buffer;
    id<MTLBuffer> gpu_chunk_buffer;
#endif
    
    // Metal-specific state
    bool supports_gpu_mesh_generation;
    bool supports_texture_arrays;
    bool supports_indirect_draw;
    
    // Performance metrics
    u32 gpu_mesh_generation_time;
    u32 cpu_mesh_generation_time;
    u32 metal_draw_calls;
    
} VoxelRendererMetal;

// Create Metal voxel renderer
VoxelRendererMetal *voxel_renderer_metal_create(void *metal_device);

// Destroy Metal voxel renderer
void voxel_renderer_metal_destroy(VoxelRendererMetal *renderer);

// Update with Metal-specific optimizations
void voxel_renderer_metal_update(VoxelRendererMetal *renderer, float delta_time);

// Render with Metal pipeline
void voxel_renderer_metal_render(VoxelRendererMetal *renderer, const Mat4 *view, const Mat4 *projection);

// GPU mesh generation (Metal compute shader)
void voxel_renderer_metal_generate_chunk_mesh_gpu(VoxelRendererMetal *renderer, VoxelChunk *chunk);

// CPU mesh generation fallback
void voxel_renderer_metal_generate_chunk_mesh_cpu(VoxelRendererMetal *renderer, VoxelChunk *chunk);

// Create Metal resources
bool voxel_renderer_metal_create_resources(VoxelRendererMetal *renderer);

// Update Metal uniforms
void voxel_renderer_metal_update_uniforms(VoxelRendererMetal *renderer, const Mat4 *view, const Mat4 *projection);

// Get Metal-specific statistics
void voxel_renderer_metal_get_stats(VoxelRendererMetal *renderer, u32 *gpu_time, u32 *cpu_time, u32 *draw_calls);

#ifdef __cplusplus
}
#endif

#endif // VOXEL_RENDERER_METAL_H
