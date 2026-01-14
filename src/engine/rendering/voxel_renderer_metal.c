// Metal Voxel Renderer Implementation
// Metal-specific optimizations and features for voxel rendering

#include "voxel_renderer_metal.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include "engine/include/math/math_all.h"
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

// Metal shader source (embedded)
static const char *voxel_vertex_shader_metal = R"(
#include <metal_stdlib>
using namespace metal;

struct VoxelVertex {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
    uchar  ao       [[attribute(3)]];
    uchar  light    [[attribute(4)]];
    ushort block_id [[attribute(5)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 normal;
    float2 uv;
    float  ao;
    float  light;
    uint   block_id;
    float3 world_pos;
    float3 view_pos;
};

struct Uniforms {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 model_view_projection;
    float3 camera_pos;
    float time;
    float fog_density;
    float fog_start;
    float fog_end;
};

struct LightingData {
    float3 sun_direction;
    float3 sun_color;
    float3 ambient_color;
    float sun_intensity;
    float ambient_intensity;
};

vertex VertexOut voxel_vertex_main(VoxelVertex v [[stage_in]],
                                  constant Uniforms &uniforms [[buffer(1)]],
                                  constant LightingData &lighting [[buffer(2)]]) {
    VertexOut out;
    
    float4 world_pos = uniforms.model * float4(v.position, 1.0);
    out.world_pos = world_pos.xyz;
    out.view_pos = (uniforms.view * world_pos).xyz;
    out.position = uniforms.projection * float4(out.view_pos, 1.0);
    
    out.normal = (uniforms.model * float4(v.normal, 0.0)).xyz;
    out.uv = v.uv;
    out.ao = float(v.ao) / 255.0;
    out.light = float(v.light) / 255.0;
    out.block_id = uint(v.block_id);
    
    return out;
}
)";

static const char *voxel_fragment_shader_metal = R"(
#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float3 normal;
    float2 uv;
    float  ao;
    float  light;
    uint   block_id;
    float3 world_pos;
    float3 view_pos;
};

struct Uniforms {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 model_view_projection;
    float3 camera_pos;
    float time;
    float fog_density;
    float fog_start;
    float fog_end;
};

struct LightingData {
    float3 sun_direction;
    float3 sun_color;
    float3 ambient_color;
    float sun_intensity;
    float ambient_intensity;
};

constexpr sampler texture_sampler(address::repeat, mag_filter::linear, min_filter::linear);

fragment float4 voxel_fragment_main(VertexOut in [[stage_in]],
                                   constant Uniforms &uniforms [[buffer(0)]],
                                   constant LightingData &lighting [[buffer(1)]],
                                   texture2d_array<float> block_textures [[texture(0)]]) {
    float3 albedo;
    if (in.block_id < block_textures.get_array_size()) {
        albedo = block_textures.sample(texture_sampler, in.uv, in.block_id).rgb;
    } else {
        switch (in.block_id) {
            case 1: albedo = float3(0.5, 0.5, 0.5); break;
            case 2: albedo = float3(0.1, 0.8, 0.2); break;
            case 3: albedo = float3(0.5, 0.3, 0.1); break;
            case 4: albedo = float3(0.6, 0.4, 0.2); break;
            case 5: albedo = float3(0.2, 0.6, 0.1); break;
            case 6: albedo = float3(0.2, 0.4, 0.8); break;
            case 7: albedo = float3(0.8, 0.8, 0.4); break;
            case 8: albedo = float3(0.2, 0.2, 0.2); break;
            case 9: albedo = float3(0.7, 0.7, 0.7); break;
            case 10: albedo = float3(1.0, 0.8, 0.2); break;
            case 11: albedo = float3(0.8, 0.8, 1.0); break;
            default: albedo = float3(0.5, 0.5, 0.5); break;
        }
    }
    
    float3 normal = normalize(in.normal);
    float NdotL = max(dot(normal, lighting.sun_direction), 0.0);
    float3 sunlight = lighting.sun_color * lighting.sun_intensity * NdotL;
    float3 ambient = lighting.ambient_color * lighting.ambient_intensity;
    float3 block_light = albedo * in.light;
    float ao_factor = mix(0.3, 1.0, in.ao);
    
    float3 final_color = albedo * (sunlight + ambient) * ao_factor + block_light * 0.5;
    
    // Fog
    float distance = length(in.view_pos);
    float fog_factor = saturate((distance - uniforms.fog_start) / (uniforms.fog_end - uniforms.fog_start));
    fog_factor = pow(fog_factor, uniforms.fog_density);
    float3 fog_color = float3(0.7, 0.8, 0.9);
    final_color = mix(final_color, fog_color, fog_factor);
    
    // Water animation
    if (in.block_id == 6) {
        float wave = sin(uniforms.time * 2.0 + in.world_pos.x * 0.5) * 0.05;
        final_color += wave;
    }
    
    return float4(final_color, 1.0);
}
)";

// Metal uniform structures
typedef struct {
    Mat4 model;
    Mat4 view;
    Mat4 projection;
    Mat4 model_view_projection;
    Vec3 camera_pos;
    float time;
    float fog_density;
    float fog_start;
    float fog_end;
} MetalUniforms;

typedef struct {
    Vec3 sun_direction;
    Vec3 sun_color;
    Vec3 ambient_color;
    float sun_intensity;
    float ambient_intensity;
} MetalLightingData;

// Helper functions
static id<MTLDevice> get_default_metal_device(void) {
#ifdef __APPLE__
    return MTLCreateSystemDefaultDevice();
#else
    return NULL;
#endif
}

static id<MTLLibrary> create_metal_library(id<MTLDevice> device) {
#ifdef __APPLE__
    NSError *error = nil;
    
    // Create library from embedded shader source
    NSString *shader_source = [NSString stringWithUTF8String:voxel_vertex_shader_metal];
    shader_source = [shader_source stringByAppendingString:[NSString stringWithUTF8String:voxel_fragment_shader_metal]];
    
    MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
    options.languageVersion = MTLLanguageVersion2_0;
    
    id<MTLLibrary> library = [device newLibraryWithSource:shader_source 
                                                options:options 
                                                  error:&error];
    
    [options release];
    
    if (error) {
        LOG_ERROR("Metal shader compilation failed: %s", [[error localizedDescription] UTF8String]);
        return NULL;
    }
    
    return library;
#else
    return NULL;
#endif
}

// Create Metal voxel renderer
VoxelRendererMetal *voxel_renderer_metal_create(void *metal_device) {
    VoxelRendererMetal *renderer = malloc(sizeof(VoxelRendererMetal));
    if (!renderer) {
        LOG_ERROR("Failed to allocate Metal voxel renderer");
        return NULL;
    }
    
    memset(renderer, 0, sizeof(VoxelRendererMetal));
    
#ifdef __APPLE__
    // Get Metal device
    if (metal_device) {
        renderer->device = (id<MTLDevice>)metal_device;
    } else {
        renderer->device = get_default_metal_device();
    }
    
    if (!renderer->device) {
        LOG_ERROR("Failed to get Metal device");
        free(renderer);
        return NULL;
    }
    
    // Create command queue
    renderer->command_queue = [renderer->device newCommandQueue];
    if (!renderer->command_queue) {
        LOG_ERROR("Failed to create Metal command queue");
        free(renderer);
        return NULL;
    }
    
    // Check device capabilities
    renderer->supports_gpu_mesh_generation = [renderer->device supportsFamily:MTLGPUFamilyApple2];
    renderer->supports_texture_arrays = [renderer->device supportsFamily:MTLGPUFamilyApple1];
    renderer->supports_indirect_draw = [renderer->device supportsFamily:MTLGPUFamilyApple2];
    
    LOG_INFO("Metal device capabilities: GPU mesh gen: %s, Texture arrays: %s, Indirect draw: %s",
             renderer->supports_gpu_mesh_generation ? "YES" : "NO",
             renderer->supports_texture_arrays ? "YES" : "NO", 
             renderer->supports_indirect_draw ? "YES" : "NO");
    
    // Create Metal resources
    if (!voxel_renderer_metal_create_resources(renderer)) {
        voxel_renderer_metal_destroy(renderer);
        return NULL;
    }
    
    // Initialize base voxel renderer
    if (!voxel_renderer_create()) {
        LOG_ERROR("Failed to initialize base voxel renderer");
        voxel_renderer_metal_destroy(renderer);
        return NULL;
    }
    
#else
    LOG_ERROR("Metal not supported on this platform");
    free(renderer);
    return NULL;
#endif
    
    LOG_INFO("Metal voxel renderer created successfully");
    return renderer;
}

// Create Metal resources
bool voxel_renderer_metal_create_resources(VoxelRendererMetal *renderer) {
#ifdef __APPLE__
    // Create shader library
    id<MTLLibrary> library = create_metal_library(renderer->device);
    if (!library) {
        LOG_ERROR("Failed to create Metal shader library");
        return false;
    }
    
    // Get shader functions
    id<MTLFunction> vertex_function = [library newFunctionWithName:@"voxel_vertex_main"];
    id<MTLFunction> fragment_function = [library newFunctionWithName:@"voxel_fragment_main"];
    
    if (!vertex_function || !fragment_function) {
        LOG_ERROR("Failed to get Metal shader functions");
        [library release];
        return false;
    }
    
    // Create render pipeline
    MTLRenderPipelineDescriptor *pipeline_desc = [[MTLRenderPipelineDescriptor alloc] init];
    pipeline_desc.vertexFunction = vertex_function;
    pipeline_desc.fragmentFunction = fragment_function;
    
    // Set up pixel format
    pipeline_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipeline_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    
    // Create pipeline state
    NSError *error = nil;
    renderer->pipeline_state = [renderer->device newRenderPipelineStateWithDescriptor:pipeline_desc 
                                                                                   error:&error];
    
    [pipeline_desc release];
    [vertex_function release];
    [fragment_function release];
    [library release];
    
    if (error) {
        LOG_ERROR("Failed to create Metal render pipeline: %s", [[error localizedDescription] UTF8String]);
        return false;
    }
    
    // Create uniform buffers
    renderer->uniform_buffer = [renderer->device newBufferWithLength:sizeof(MetalUniforms)
                                                            options:MTLResourceStorageModeShared];
    renderer->lighting_buffer = [renderer->device newBufferWithLength:sizeof(MetalLightingData)
                                                              options:MTLResourceStorageModeShared];
    
    if (!renderer->uniform_buffer || !renderer->lighting_buffer) {
        LOG_ERROR("Failed to create Metal uniform buffers");
        return false;
    }
    
    // Create GPU mesh generation buffers if supported
    if (renderer->supports_gpu_mesh_generation) {
        renderer->gpu_vertex_buffer = [renderer->device newBufferWithLength:sizeof(VoxelVertex) * 65536
                                                                      options:MTLResourceStorageModePrivate];
        renderer->gpu_index_buffer = [renderer->device newBufferWithLength:sizeof(u32) * 98304
                                                                     options:MTLResourceStorageModePrivate];
        renderer->gpu_counter_buffer = [renderer->device newBufferWithLength:sizeof(u32) * 4
                                                                      options:MTLResourceStorageModePrivate];
        renderer->gpu_chunk_buffer = [renderer->device newBufferWithLength:sizeof(u16) * (16 * 64 * 16)
                                                                    options:MTLResourceStorageModePrivate];
        
        if (!renderer->gpu_vertex_buffer || !renderer->gpu_index_buffer || 
            !renderer->gpu_counter_buffer || !renderer->gpu_chunk_buffer) {
            LOG_WARN("Failed to create GPU mesh generation buffers, falling back to CPU");
            renderer->supports_gpu_mesh_generation = false;
        }
    }
    
    // Create block texture array if supported
    if (renderer->supports_texture_arrays) {
        MTLTextureDescriptor *texture_desc = [[MTLTextureDescriptor alloc] init];
        texture_desc.textureType = MTLTextureType2DArray;
        texture_desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
        texture_desc.width = 16;
        texture_desc.height = 16;
        texture_desc.arrayLength = BLOCK_TYPE_COUNT;
        texture_desc.usage = MTLTextureUsageShaderRead;
        
        renderer->block_texture_array = [renderer->device newTextureWithDescriptor:texture_desc];
        [texture_desc release];
        
        if (!renderer->block_texture_array) {
            LOG_WARN("Failed to create block texture array, using fallback colors");
            renderer->supports_texture_arrays = false;
        }
    }
    
    return true;
#else
    return false;
#endif
}

// Destroy Metal voxel renderer
void voxel_renderer_metal_destroy(VoxelRendererMetal *renderer) {
    if (!renderer) return;
    
#ifdef __APPLE__
    // Release Metal resources
    if (renderer->pipeline_state) [renderer->pipeline_state release];
    if (renderer->compute_pipeline) [renderer->compute_pipeline release];
    if (renderer->vertex_buffer) [renderer->vertex_buffer release];
    if (renderer->index_buffer) [renderer->index_buffer release];
    if (renderer->uniform_buffer) [renderer->uniform_buffer release];
    if (renderer->lighting_buffer) [renderer->lighting_buffer release];
    if (renderer->block_texture_array) [renderer->block_texture_array release];
    if (renderer->chunk_data_buffer) [renderer->chunk_data_buffer release];
    if (renderer->gpu_vertex_buffer) [renderer->gpu_vertex_buffer release];
    if (renderer->gpu_index_buffer) [renderer->gpu_index_buffer release];
    if (renderer->gpu_counter_buffer) [renderer->gpu_counter_buffer release];
    if (renderer->gpu_chunk_buffer) [renderer->gpu_chunk_buffer release];
    if (renderer->command_queue) [renderer->command_queue release];
    if (renderer->device) [renderer->device release];
#endif
    
    free(renderer);
    LOG_INFO("Metal voxel renderer destroyed");
}

// Update Metal uniforms
void voxel_renderer_metal_update_uniforms(VoxelRendererMetal *renderer, const Mat4 *view, const Mat4 *projection) {
#ifdef __APPLE__
    if (!renderer->uniform_buffer || !renderer->lighting_buffer) return;
    
    // Update uniform buffer
    MetalUniforms *uniforms = (MetalUniforms *)[renderer->uniform_buffer contents];
    uniforms->model = mat4_identity();
    uniforms->view = *view;
    uniforms->projection = *projection;
    uniforms->model_view_projection = mat4_multiply(projection, view);
    uniforms->camera_pos = renderer->base.camera_position;
    uniforms->time = 0.0f; // Would get from engine time
    uniforms->fog_density = 1.5f;
    uniforms->fog_start = 32.0f;
    uniforms->fog_end = 128.0f;
    
    // Update lighting buffer
    MetalLightingData *lighting = (MetalLightingData *)[renderer->lighting_buffer contents];
    lighting->sun_direction = vec3_normalize(vec3_create(0.3f, -0.7f, 0.2f));
    lighting->sun_color = vec3_create(1.0f, 0.95f, 0.8f);
    lighting->ambient_color = vec3_create(0.4f, 0.5f, 0.6f);
    lighting->sun_intensity = 1.0f;
    lighting->ambient_intensity = 0.3f;
#endif
}

// Render with Metal pipeline
void voxel_renderer_metal_render(VoxelRendererMetal *renderer, const Mat4 *view, const Mat4 *projection) {
#ifdef __APPLE__
    if (!renderer || !renderer->pipeline_state) return;
    
    // Update uniforms
    voxel_renderer_metal_update_uniforms(renderer, view, projection);
    
    // Create command buffer
    id<MTLCommandBuffer> command_buffer = [renderer->command_queue commandBuffer];
    
    // This would normally get the render pass descriptor from the view
    // For now, we'll just log the render call
    LOG_DEBUG("Metal voxel render: %d chunks", renderer->base.chunk_count);
    
    // Release command buffer
    [command_buffer release];
#endif
}

// GPU mesh generation
void voxel_renderer_metal_generate_chunk_mesh_gpu(VoxelRendererMetal *renderer, VoxelChunk *chunk) {
#ifdef __APPLE__
    if (!renderer->supports_gpu_mesh_generation || !renderer->compute_pipeline) {
        voxel_renderer_metal_generate_chunk_mesh_cpu(renderer, chunk);
        return;
    }
    
    // Upload chunk data to GPU
    if (chunk->blocks && renderer->gpu_chunk_buffer) {
        void *chunk_data = [renderer->gpu_chunk_buffer contents];
        memcpy(chunk_data, chunk->blocks, sizeof(u16) * (16 * 64 * 16));
        
        // Create command buffer for compute
        id<MTLCommandBuffer> command_buffer = [renderer->command_queue commandBuffer];
        id<MTLComputeCommandEncoder> compute_encoder = [command_buffer computeCommandEncoder];
        
        // Set compute pipeline
        [compute_encoder setComputePipelineState:renderer->compute_pipeline];
        
        // Set buffers
        [compute_encoder setBuffer:renderer->gpu_vertex_buffer offset:0 atIndex:0];
        [compute_encoder setBuffer:renderer->gpu_index_buffer offset:0 atIndex:1];
        [compute_encoder setBuffer:renderer->gpu_counter_buffer offset:0 atIndex:2];
        [compute_encoder setBuffer:renderer->gpu_counter_buffer offset:sizeof(u32) atIndex:3];
        [compute_encoder setBuffer:renderer->gpu_chunk_buffer offset:0 atIndex:4];
        [compute_encoder setBuffer:renderer->uniform_buffer offset:0 atIndex:5];
        
        // Reset counters
        u32 zero = 0;
        [compute_encoder setBytes:&zero length:sizeof(u32) atIndex:2];
        [compute_encoder setBytes:&zero length:sizeof(u32) atIndex:3];
        
        // Dispatch compute shader
        MTLSize threadgroup_size = MTLSizeMake(8, 8, 8);
        MTLSize threadgroup_count = MTLSizeMake(2, 8, 2); // 16x64x16 grid
        
        [compute_encoder dispatchThreadgroups:threadgroup_count threadsPerThreadgroup:threadgroup_size];
        [compute_encoder endEncoding];
        
        // Commit and wait for completion
        [command_buffer commit];
        [command_buffer waitUntilCompleted];
        
        // Read back results
        void *vertex_data = [renderer->gpu_vertex_buffer contents];
        void *index_data = [renderer->gpu_index_buffer contents];
        u32 *vertex_count = (u32 *)[renderer->gpu_counter_buffer contents];
        u32 *index_count = (u32 *)((char *)[renderer->gpu_counter_buffer contents] + sizeof(u32));
        
        // Update chunk with generated mesh
        if (*vertex_count > 0 && *index_count > 0) {
            // Upload to chunk buffers
            buffer_upload(chunk->vertex_buffer, vertex_data, sizeof(VoxelVertex) * *vertex_count);
            buffer_upload(chunk->index_buffer, index_data, sizeof(u32) * *index_count);
            
            chunk->vertex_count = *vertex_count;
            chunk->index_count = *index_count;
        }
        
        [command_buffer release];
        
        LOG_DEBUG("GPU mesh generation: %d vertices, %d indices", *vertex_count, *index_count);
    }
#else
    voxel_renderer_metal_generate_chunk_mesh_cpu(renderer, chunk);
#endif
}

// CPU mesh generation fallback
void voxel_renderer_metal_generate_chunk_mesh_cpu(VoxelRendererMetal *renderer, VoxelChunk *chunk) {
    // Use the base voxel renderer's mesh generation
    voxel_renderer_rebuild_chunk_mesh(&renderer->base, chunk);
    
    LOG_DEBUG("CPU mesh generation: %d vertices, %d indices", chunk->vertex_count, chunk->index_count);
}

// Update with Metal-specific optimizations
void voxel_renderer_metal_update(VoxelRendererMetal *renderer, float delta_time) {
    if (!renderer) return;
    
    // Update base renderer
    voxel_renderer_update(&renderer->base, delta_time);
    
    // Use GPU mesh generation for dirty chunks if supported
    for (u32 i = 0; i < renderer->base.chunk_count; i++) {
        VoxelChunk *chunk = &renderer->base.chunks[i];
        
        if (chunk->dirty && chunk->visible) {
            if (renderer->supports_gpu_mesh_generation) {
                voxel_renderer_metal_generate_chunk_mesh_gpu(renderer, chunk);
            } else {
                voxel_renderer_metal_generate_chunk_mesh_cpu(renderer, chunk);
            }
        }
    }
}

// Get Metal-specific statistics
void voxel_renderer_metal_get_stats(VoxelRendererMetal *renderer, u32 *gpu_time, u32 *gpu_time, u32 *draw_calls) {
    if (!renderer) return;
    
    if (gpu_time) *gpu_time = renderer->gpu_mesh_generation_time;
    if (gpu_time) *gpu_time = renderer->cpu_mesh_generation_time;
    if (draw_calls) *draw_calls = renderer->metal_draw_calls;
}
