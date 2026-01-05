// Ray Query API - Inline Ray Tracing for MoltenVK Compatibility
// Replaces VK_KHR_ray_tracing_pipeline with VK_KHR_ray_query for better Metal support

#ifndef RAY_QUERY_H
#define RAY_QUERY_H

#include "../common.h"

#ifdef VULKAN_BUILD

#include <vulkan/vulkan.h>

// Ray query configuration
typedef struct {
    VkDevice device;
    VkPhysicalDevice physical_device;
    
    // Acceleration structures (same as pipeline-based RT)
    VkAccelerationStructureKHR top_level_as;
    VkBuffer tlas_buffer;
    VkDeviceMemory tlas_memory;
    
    // Ray query support flags
    bool ray_query_supported;
    bool compute_shader_ray_query;
    
} RayQuerySystem;

// Initialize ray query system
bool ray_query_init(VkDevice device, VkPhysicalDevice physical_device, RayQuerySystem* system);

// Create acceleration structure (compatible with both pipeline and query)
bool ray_query_create_acceleration_structure(
    RayQuerySystem* system,
    VkAccelerationStructureGeometryKHR* geometries,
    u32 geometry_count,
    VkAccelerationStructureKHR* out_as
);

// Perform ray tracing using compute shader + ray query
// (This replaces vkCmdTraceRaysKHR from pipeline-based RT)
void ray_query_trace_rays(
    RayQuerySystem* system,
    VkCommandBuffer cmd_buffer,
    VkPipeline compute_pipeline,
    u32 width, u32 height, u32 depth
);

// Check if ray query is supported
bool ray_query_is_supported(RayQuerySystem* system);

// Cleanup
void ray_query_shutdown(RayQuerySystem* system);

#endif // VULKAN_BUILD

#endif // RAY_QUERY_H
