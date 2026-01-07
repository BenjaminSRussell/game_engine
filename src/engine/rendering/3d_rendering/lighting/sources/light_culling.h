#ifndef LIGHTING_LIGHT_CULLING_H
#define LIGHTING_LIGHT_CULLING_H

#include <simd/simd.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
// Forward declarations for C
typedef struct objc_object* id;
typedef struct MTLBuffer* MTLBuffer;
typedef struct MTLComputeCommandEncoder* MTLComputeCommandEncoder;
typedef struct MTLComputePipelineState* MTLComputePipelineState;
#endif

// Forward declaration of metal_device_t (assuming it exists based on user context)
typedef struct metal_device metal_device_t;

// Camera structure (assuming it exists based on user context)
typedef struct camera {
    simd_float4x4 view;
    simd_float4x4 proj;
    float near;
    float far;
} camera_t;

typedef enum light_type {
    LIGHT_TYPE_DIRECTIONAL = 0,
    LIGHT_TYPE_POINT = 1,
    LIGHT_TYPE_SPOT = 2,
} light_type_t;

typedef struct light {
    simd_float3 position;
    float radius;
    simd_float3 direction;
    float inner_cone;
    simd_float3 color;
    float outer_cone;
    float intensity;
    light_type_t type;
    bool cast_shadows;
    uint32_t shadow_map_index;
} light_t;

typedef struct light_system {
    light_t* lights;
    uint32_t light_count;
    uint32_t max_lights;

#ifdef __OBJC__
    id<MTLBuffer> light_buffer;
    id<MTLBuffer> light_grid;  // For clustered culling
    id<MTLBuffer> light_indices;
#else
    void* light_buffer;
    void* light_grid;
    void* light_indices;
#endif

    // Cluster grid dimensions
    uint32_t cluster_x, cluster_y, cluster_z;
} light_system_t;

// API
light_system_t* light_system_create(metal_device_t* dev, uint32_t max_lights);
uint32_t light_system_add(light_system_t* sys, light_t* light);
void light_system_update_gpu(light_system_t* sys);

#ifdef __OBJC__
void light_system_cull_clustered(light_system_t* sys, id<MTLComputeCommandEncoder> encoder,
                                  camera_t* camera, id<MTLComputePipelineState> cull_pipeline);
#else
void light_system_cull_clustered(light_system_t* sys, void* encoder,
                                  camera_t* camera, void* cull_pipeline);
#endif

#endif // LIGHTING_LIGHT_CULLING_H
