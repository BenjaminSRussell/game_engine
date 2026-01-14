// Real-Time Raytracing Pipeline Header
// RT-001: Raytracing pipeline core architecture
// RT-002: Acceleration structure management
// RT-003: Ray generation shaders
// RT-004: Closest hit shaders
// RT-005: Miss shaders
// RT-006: Shadow ray handling
// RT-007: Global illumination
// RT-008: Reflections and refractions
// RT-009: Denoising and post-processing
// RT-010: Performance optimization

#ifndef RAYTRACING_PIPELINE_H
#define RAYTRACING_PIPELINE_H

#include <common.h>
#include "include/rendering/vulkan.h"

#ifdef __cplusplus
extern "C" {
#endif

// Raytracing pipeline configuration
typedef struct {
    u32 maxRecursionDepth;
    u32 maxRayGenShaders;
    u32 maxMissShaders;
    u32 maxHitGroups;
    u32 maxCallableShaders;
    bool enableShadows;
    bool enableReflections;
    bool enableGI;
    bool enableDenoising;
    float rayTMax;
    float rayEpsilon;
} RaytracingConfig;

// Ray types
typedef enum {
    RAY_TYPE_PRIMARY = 0,
    RAY_TYPE_SHADOW,
    RAY_TYPE_REFLECTION,
    RAY_TYPE_GI,
    RAY_TYPE_COUNT
} RayType;

// Ray payload structure
typedef struct {
    vec3 origin;
    vec3 direction;
    float tMax;
    u32 rayType;
    u32 recursionDepth;
    vec3 accumulatedColor;
    vec3 accumulatedNormal;
    float accumulatedDistance;
    bool hit;
} RayPayload;

// Hit attributes structure
typedef struct {
    vec3 barycentrics;
    vec3 worldNormal;
    vec3 worldPosition;
    u32 instanceId;
    u32 primitiveId;
    u32 materialId;
} HitAttributes;

// Raytracing statistics
typedef struct {
    u64 totalRaysTraced;
    u64 primaryRays;
    u64 shadowRays;
    u64 reflectionRays;
    u64 giRays;
    u64 trianglesTested;
    u64 aabbTests;
    double averageRecursionDepth;
    double frameTime;
    u64 memoryUsage;
} RaytracingStats;

// Raytracing pipeline state
typedef struct RaytracingPipeline {
#ifdef VULKAN_BUILD
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    
    // Shader binding table
    VkBuffer raygenSBT;
    VkBuffer missSBT;
    VkBuffer hitSBT;
    VkBuffer callableSBT;
    VkDeviceMemory sbtMemory;
    
    // Acceleration structures
    VkAccelerationStructureKHR tlas;
    VkBuffer blasBuffer;
    VkDeviceMemory blasMemory;
    
    // Output targets
    VkImage outputImage;
    VkImageView outputImageView;
    VkDeviceMemory outputMemory;
    
    // Denoising resources
    VkImage denoiseImage;
    VkImageView denoiseImageView;
    VkDeviceMemory denoiseMemory;
#endif
    bool initialized;
    RaytracingConfig config;
    RaytracingStats stats;
} RaytracingPipeline;

// Core raytracing pipeline functions
bool rt_init(RaytracingPipeline* pipeline, VulkanRenderer* renderer, const RaytracingConfig* config);
void rt_shutdown(RaytracingPipeline* pipeline, VulkanRenderer* renderer);
bool rt_is_initialized(const RaytracingPipeline* pipeline);

// Acceleration structure management
bool rt_build_acceleration_structures(RaytracingPipeline* pipeline, VulkanRenderer* renderer,
                                      const void* vertices, u32 vertexCount,
                                      const u32* indices, u32 indexCount);
bool rt_update_acceleration_structures(RaytracingPipeline* pipeline, VulkanRenderer* renderer);

// Raytracing execution
bool rt_trace_rays(RaytracingPipeline* pipeline, VulkanRenderer* renderer,
                   VkCommandBuffer commandBuffer, u32 width, u32 height);
bool rt_trace_single_ray(RaytracingPipeline* pipeline, const RayPayload* ray, vec3* outColor);

// Shader management
bool rt_load_raygen_shader(RaytracingPipeline* pipeline, const char* shaderPath);
bool rt_load_miss_shader(RaytracingPipeline* pipeline, const char* shaderPath, u32 slot);
bool rt_load_closest_hit_shader(RaytracingPipeline* pipeline, const char* shaderPath, u32 slot);
bool rt_load_any_hit_shader(RaytracingPipeline* pipeline, const char* shaderPath, u32 slot);

// Configuration and optimization
void rt_set_config(RaytracingPipeline* pipeline, const RaytracingConfig* config);
void rt_get_config(const RaytracingPipeline* pipeline, RaytracingConfig* outConfig);
void rt_optimize_for_hardware(RaytracingPipeline* pipeline);

// Statistics and debugging
void rt_get_stats(const RaytracingPipeline* pipeline, RaytracingStats* outStats);
void rt_reset_stats(RaytracingPipeline* pipeline);
void rt_debug_print_stats(const RaytracingPipeline* pipeline);
bool rt_validate_pipeline(const RaytracingPipeline* pipeline);

// Denoising and post-processing
bool rt_apply_denoising(RaytracingPipeline* pipeline, VkCommandBuffer commandBuffer);
bool rt_apply_bilateral_filter(RaytracingPipeline* pipeline, VkCommandBuffer commandBuffer);
bool rt_apply_temporal_accumulation(RaytracingPipeline* pipeline, VkCommandBuffer commandBuffer);

// Advanced features
bool rt_enable_variable_rate_shading(RaytracingPipeline* pipeline);
bool rt_enable_ray_culling(RaytracingPipeline* pipeline);
bool rt_enable_early_termination(RaytracingPipeline* pipeline);

// Utility functions
u32 rt_calculate_sbt_size(const RaytracingPipeline* pipeline);
u64 rt_estimate_memory_usage(const RaytracingConfig* config);
bool rt_check_hardware_support(VulkanRenderer* renderer);

#ifdef __cplusplus
}
#endif

#endif // RAYTRACING_PIPELINE_H
