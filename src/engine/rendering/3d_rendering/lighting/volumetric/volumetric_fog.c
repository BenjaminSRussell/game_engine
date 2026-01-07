/*
 * volumetric_fog.c
 * Volumetric fog rendering
 */

#include "volumetric_fog.h"
#include <stdlib.h>
#include <string.h>

#ifdef __OBJC__
#import <Metal/Metal.h>

volumetric_fog_t* volumetric_fog_create(metal_device_t* dev, uint32_t w, uint32_t h, uint32_t d) {
    volumetric_fog_t* fog = calloc(1, sizeof(volumetric_fog_t));
    if (!fog) return NULL;
    
    fog->resolution = simd_make_uint3(w, h, d);

    MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
    desc.textureType = MTLTextureType3D;
    desc.width = w;
    desc.height = h;
    desc.depth = d;
    desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    desc.storageMode = MTLStorageModePrivate;

    desc.pixelFormat = MTLPixelFormatRGBA16Float;
    fog->froxel_scattering = [dev->device newTextureWithDescriptor:desc];
    fog->integrated_scattering = [dev->device newTextureWithDescriptor:desc];

    desc.pixelFormat = MTLPixelFormatR16Float;
    fog->froxel_density = [dev->device newTextureWithDescriptor:desc];

    fog->near_plane = 0.1f;
    fog->far_plane = 500.0f;
    fog->density_scale = 0.01f;
    fog->fog_color = simd_make_float3(0.5f, 0.6f, 0.7f);

    // Pipelines would be loaded here from a shader library
    // fog->inject_pipeline = ...
    // fog->integrate_pipeline = ...

    return fog;
}

void volumetric_fog_destroy(volumetric_fog_t* fog) {
    if (!fog) return;
    
    // ARC handles these if they are id types in a struct in an Obj-C file
    // But since they are id types, we should set them to nil to be safe or if not using ARC
    fog->froxel_scattering = nil;
    fog->froxel_density = nil;
    fog->integrated_scattering = nil;
    fog->inject_pipeline = nil;
    fog->integrate_pipeline = nil;
    
    free(fog);
}

void volumetric_fog_render(volumetric_fog_t* fog, id<MTLCommandBuffer> cmd,
                           light_system_t* lights, camera_t* camera,
                           shadow_map_system_t* shadows) {
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    if (!encoder) return;

    // Pass 1: Inject density and lighting into froxels
    if (fog->inject_pipeline) {
        [encoder setComputePipelineState:fog->inject_pipeline];
        [encoder setTexture:fog->froxel_scattering atIndex:0];
        [encoder setTexture:fog->froxel_density atIndex:1];
        
        // This assumes light_system_t has a member light_buffer
        // [encoder setBuffer:lights->light_buffer offset:0 atIndex:0];

        struct {
            simd_float4x4 inv_view_proj;
            simd_float3 camera_pos;
            float near, far;
            simd_uint3 resolution;
            float density_scale;
            simd_float3 fog_color;
            uint32_t light_count;
        } uniforms = {
            // .inv_view_proj = mat4_to_simd(mat4_inverse(camera_get_view_proj(camera))),
            .near = fog->near_plane,
            .far = fog->far_plane,
            .resolution = fog->resolution,
            .density_scale = fog->density_scale,
            .fog_color = fog->fog_color,
            // .light_count = light_system_get_count(lights)
        };
        [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:1];

        // Bind shadow maps for light visibility
        // This part depends on the exact shadow_map_system_t layout
        /*
        for (uint32_t i = 0; i < shadows->cascade_count; i++) {
            [encoder setTexture:shadows->cascades[i].depth_texture atIndex:10 + i];
        }
        */

        MTLSize groups = MTLSizeMake((fog->resolution.x + 7) / 8,
                                      (fog->resolution.y + 7) / 8,
                                      (fog->resolution.z + 3) / 4);
        MTLSize threads = MTLSizeMake(8, 8, 4);
        [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threads];
    }

    // Pass 2: Ray march integration
    if (fog->integrate_pipeline) {
        [encoder setComputePipelineState:fog->integrate_pipeline];
        [encoder setTexture:fog->froxel_scattering atIndex:0];
        [encoder setTexture:fog->integrated_scattering atIndex:1];

        MTLSize groups = MTLSizeMake((fog->resolution.x + 7) / 8, (fog->resolution.y + 7) / 8, 1);
        MTLSize threads = MTLSizeMake(8, 8, 1);
        [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threads];
    }

    [encoder endEncoding];
}

#else
// C Stubs
volumetric_fog_t* volumetric_fog_create(metal_device_t* dev, uint32_t w, uint32_t h, uint32_t d) { return NULL; }
void volumetric_fog_destroy(volumetric_fog_t* fog) {}
void volumetric_fog_render(volumetric_fog_t* fog, void* cmd,
                           light_system_t* lights, camera_t* camera,
                           shadow_map_system_t* shadows) {}
#endif
