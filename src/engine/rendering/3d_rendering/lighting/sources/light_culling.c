#include "light_culling.h"
#include <stdlib.h>
#include <string.h>

// Assuming definition of metal_device_t is needed or included from somewhere
// For now, we'll assume it has a 'device' member which is an id<MTLDevice>
// If this file is compiled as C, it won't work with Obj-C syntax. 
// Assuming the user treats .c files as Obj-C or this is mixed code source.
// We will explicitly define the struct helper locally if needed or assume user environment handles it.

#ifdef __OBJC__

typedef struct metal_device {
    id<MTLDevice> device;
    // ... potentially other fields
} metal_device_t;

light_system_t* light_system_create(metal_device_t* dev, uint32_t max_lights) {
    light_system_t* sys = calloc(1, sizeof(light_system_t));
    sys->max_lights = max_lights;
    sys->lights = calloc(max_lights, sizeof(light_t));

    sys->light_buffer = [dev->device newBufferWithLength:max_lights * sizeof(light_t)
                                                 options:MTLResourceStorageModeShared];

    // Clustered light grid (16x9x24 typical)
    sys->cluster_x = 16;
    sys->cluster_y = 9;
    sys->cluster_z = 24;

    uint32_t cluster_count = sys->cluster_x * sys->cluster_y * sys->cluster_z;
    sys->light_grid = [dev->device newBufferWithLength:cluster_count * sizeof(uint32_t) * 2
                                               options:MTLResourceStorageModeShared];
    sys->light_indices = [dev->device newBufferWithLength:cluster_count * 256 * sizeof(uint32_t)
                                                  options:MTLResourceStorageModeShared];

    return sys;
}

uint32_t light_system_add(light_system_t* sys, light_t* light) {
    if (sys->light_count >= sys->max_lights) return UINT32_MAX;
    sys->lights[sys->light_count] = *light;
    return sys->light_count++;
}

void light_system_update_gpu(light_system_t* sys) {
    memcpy([sys->light_buffer contents], sys->lights, sys->light_count * sizeof(light_t));
}

// Clustered light culling (compute shader)
void light_system_cull_clustered(light_system_t* sys, id<MTLComputeCommandEncoder> encoder,
                                  camera_t* camera, id<MTLComputePipelineState> cull_pipeline) {
    // Compute shader assigns lights to clusters based on frustum intersection
    [encoder setComputePipelineState:cull_pipeline];
    [encoder setBuffer:sys->light_buffer offset:0 atIndex:0];
    [encoder setBuffer:sys->light_grid offset:0 atIndex:1];
    [encoder setBuffer:sys->light_indices offset:0 atIndex:2];

    // Camera uniforms
    struct {
        simd_float4x4 view;
        simd_float4x4 proj;
        float near, far;
        uint32_t light_count;
    } uniforms = {
        .view = camera->view,
        .proj = camera->proj,
        .near = camera->near,
        .far = camera->far,
        .light_count = sys->light_count
    };
    [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:3];

    MTLSize threads = MTLSizeMake(sys->cluster_x, sys->cluster_y, sys->cluster_z);
    MTLSize groups = MTLSizeMake(1, 1, 1);
    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threads];
}

#endif
