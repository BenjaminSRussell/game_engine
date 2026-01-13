#ifndef VULKAN_H
#define VULKAN_H

#include <stdint.h>

// Forward declarations for Vulkan types
struct VkCommandBuffer_T;
typedef struct VkCommandBuffer_T* VkCommandBuffer;

struct VkImageView_T;
typedef struct VkImageView_T* VkImageView;

// Basic Vulkan types required by other headers
struct VkBuffer_T;
typedef struct VkBuffer_T* VkBuffer;

struct VkDeviceMemory_T;
typedef struct VkDeviceMemory_T* VkDeviceMemory;

typedef uint64_t VkAccelerationStructureKHR;

// Forward declaration for VulkanRenderer
typedef struct VulkanRenderer {
    void* device;
    void* instance;
    void* physical_device;
} VulkanRenderer;

#endif // VULKAN_H