/*
 * ssr_trace.h
 * SSR System - Advanced V2
 */

#ifndef SSR_TRACE_H
#define SSR_TRACE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <simd/simd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ssr_uniforms {
    simd_float4x4 view_proj;
    simd_float4x4 inv_view_proj;
    simd_float4x4 prev_view_proj;
    simd_float3 camera_pos;
    float pad0;
    simd_uint2 screen_size;
    uint32_t max_steps;
    float thickness;
    float initial_step;
    float max_distance;
    float roughness_threshold;
    float edge_fade_distance;
    uint32_t frame_index;
    float temporal_blend_weight;
    float confidence_threshold;
    
    // New parameters for V2
    simd_float3 probe_position;
    float pad1;
    simd_float3 probe_box_min;
    float pad2;
    simd_float3 probe_box_max;
    float pad3;
    float anisotropy;
    float anisotropy_rotation;
} ssr_uniforms_t;

typedef struct ssr_quality_settings {
    uint32_t max_steps;
    uint32_t stochastic_samples;
    float bilateral_radius;
    bool enable_temporal;
    bool enable_stochastic;
    bool enable_inpainting;
    bool enable_box_projection;
} ssr_quality_settings_t;

typedef struct gbuffer {
    void* depth;
    void* normal;
    void* material;
    void* velocity;
    uint32_t width;
    uint32_t height;
} gbuffer_t;

typedef struct camera {
    simd_float4x4 view_proj;
    simd_float4x4 prev_view_proj;
    simd_float3 position;
} camera_t;

typedef struct ssr_system {
#ifdef __OBJC__
    id<MTLComputePipelineState> trace_pipeline;
    id<MTLComputePipelineState> trace_stochastic_pipeline;
    id<MTLComputePipelineState> reproject_pipeline;
    id<MTLComputePipelineState> inpaint_pipeline; // New
    id<MTLComputePipelineState> bilateral_filter_pipeline;
    id<MTLComputePipelineState> resolve_pipeline;
    id<MTLComputePipelineState> composite_pipeline;
    
    id<MTLTexture> ray_hit_texture;
    id<MTLTexture> ray_hit_accumulation;
    id<MTLTexture> ray_hit_inpainted; // New
    id<MTLTexture> reflection_texture;
    id<MTLTexture> filtered_reflection;
    id<MTLTexture> history_hit;
    id<MTLTexture> history_reflection;
    
    id<MTLTexture> environment_cubemap;
    void* hzb_builder;
#else
    void* trace_pipeline;
    void* trace_stochastic_pipeline;
    void* reproject_pipeline;
    void* inpaint_pipeline;
    void* bilateral_filter_pipeline;
    void* resolve_pipeline;
    void* composite_pipeline;
    
    void* ray_hit_texture;
    void* ray_hit_accumulation;
    void* ray_hit_inpainted;
    void* reflection_texture;
    void* filtered_reflection;
    void* history_hit;
    void* history_reflection;
    void* environment_cubemap;
    void* hzb_builder;
#endif
    
    ssr_quality_settings_t quality;
    uint32_t frame_index;
    float max_steps;
    float max_distance;
    float thickness;
    bool initialized;
} ssr_system_t;

#ifdef __OBJC__
int ssr_init(ssr_system_t* ssr, id<MTLDevice> device, uint32_t width, uint32_t height);
void ssr_shutdown(ssr_system_t* ssr);
void ssr_resize(ssr_system_t* ssr, id<MTLDevice> device, uint32_t width, uint32_t height);
void ssr_render(
    ssr_system_t* ssr,
    id<MTLCommandBuffer> cmd,
    gbuffer_t* gbuffer,
    camera_t* camera,
    id<MTLTexture> scene_color,
    id<MTLTexture> final_output
);
void ssr_set_quality(ssr_system_t* ssr, ssr_quality_settings_t quality);
void ssr_set_environment(ssr_system_t* ssr, id<MTLTexture> cubemap);
#else
int ssr_init(void* ssr, void* device, uint32_t width, uint32_t height);
void ssr_shutdown(void* ssr);
void ssr_resize(void* ssr, void* device, uint32_t width, uint32_t height);
void ssr_render(void* ssr, void* cmd, void* gbuffer, void* camera, void* scene_color, void* final_output);
void ssr_set_quality(void* ssr, ssr_quality_settings_t quality);
void ssr_set_environment(void* ssr, void* cubemap);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SSR_TRACE_H */
