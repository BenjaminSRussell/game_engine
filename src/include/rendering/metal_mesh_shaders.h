// include/rendering/metal_mesh_shaders.h
// Metal Object/Mesh Shader Support (Apple7+ GPU family)
#ifndef METAL_MESH_SHADERS_H
#define METAL_MESH_SHADERS_H

#include "engine/include/common.h"
#include <Metal/Metal.h>

typedef struct MetalMeshPipeline MetalMeshPipeline;

// Mesh shader configuration
typedef struct {
    u32 max_meshlets_per_threadgroup;
    u32 max_vertices_per_meshlet;
    u32 max_primitives_per_meshlet;
    bool enable_amplification_shader;  // For LOD/culling in object shader
} MetalMeshShaderConfig;

#ifdef __cplusplus
extern "C" {
#endif

// Capability check
bool metal_mesh_shaders_supported(id<MTLDevice> device);

// Pipeline creation
MetalMeshPipeline* metal_create_mesh_pipeline(id<MTLDevice> device,
                                              id<MTLFunction> object_shader,
                                              id<MTLFunction> mesh_shader,
                                              id<MTLFunction> fragment_shader,
                                              const MetalMeshShaderConfig* config);

// Draw with mesh shaders
void metal_draw_mesh_shaders(id<MTLRenderCommandEncoder> encoder,
                             MetalMeshPipeline* pipeline,
                             u32 meshlet_count,
                             u32 threadgroups_per_grid);

void metal_destroy_mesh_pipeline(MetalMeshPipeline* pipeline);

#ifdef __cplusplus
}
#endif

#endif // METAL_MESH_SHADERS_H
