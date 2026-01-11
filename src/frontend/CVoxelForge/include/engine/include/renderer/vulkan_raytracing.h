// Vulkan Raytracing Header
// VULKAN-RT-620: Build BLAS (Bottom Level Acceleration Structure)
// VULKAN-RT-621: Build TLAS (Top Level Acceleration Structure)  
// VULKAN-RT-622: Bind Descriptor Sets for raytracing
// VULKAN-RT-623: Implement vkCmdTraceRaysKHR

#ifndef VULKAN_RAYTRACING_H
#define VULKAN_RAYTRACING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct VulkanRenderer VulkanRenderer;

#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
#include <vulkan/vulkan.h>
#else
// Stub types when Vulkan is not available
typedef void* VkAccelerationStructureKHR;
typedef void* VkBuffer;
typedef void* VkDeviceMemory;
typedef void* VkCommandBuffer;
typedef void* VkPipeline;
typedef void* VkDescriptorSet;
typedef uint32_t VkFormat;
#endif

// BLAS build data structure
typedef struct BLASBuildData {
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_memory;
    VkBuffer transform_buffer;
    VkDeviceMemory transform_memory;
    VkAccelerationStructureKHR blas;
    VkBuffer blas_buffer;
    VkDeviceMemory blas_memory;
    uint32_t vertex_count;
    uint32_t index_count;
    bool has_transform;
} BLASBuildData;

// Raytracing API Functions
bool vulkan_raytracing_check_support(VulkanRenderer* renderer);

// VULKAN-RT-620: Build Bottom Level Acceleration Structure (BLAS)
bool vulkan_build_blas(VulkanRenderer* renderer, const void* vertices, uint32_t vertex_count,
                      const uint32_t* indices, uint32_t index_count, VkFormat vertex_format,
                      BLASBuildData* out_blas_data);

// VULKAN-RT-621: Build Top Level Acceleration Structure (TLAS)
bool vulkan_build_tlas(VulkanRenderer* renderer, const BLASBuildData* blas_instances, 
                      uint32_t instance_count, VkAccelerationStructureKHR* out_tlas);

// VULKAN-RT-622: Bind Descriptor Sets for raytracing
bool vulkan_bind_raytracing_descriptor_sets(VulkanRenderer* renderer, VkAccelerationStructureKHR tlas,
                                           VkDescriptorSet* descriptor_set);

// VULKAN-RT-623: Implement vkCmdTraceRaysKHR
void vulkan_trace_rays(VulkanRenderer* renderer, VkCommandBuffer command_buffer,
                       VkPipeline raytracing_pipeline,
                       VkDescriptorSet descriptor_set,
                       uint32_t width, uint32_t height);

// Cleanup functions
void vulkan_destroy_blas(VulkanRenderer* renderer, BLASBuildData* blas_data);
void vulkan_destroy_tlas(VulkanRenderer* renderer, VkAccelerationStructureKHR tlas);

#ifdef __cplusplus
}
#endif

#endif // VULKAN_RAYTRACING_H
