#ifndef VULKAN_H
#define VULKAN_H

#include <stdint.h>
#ifdef __APPLE__
#include <vulkan/vulkan.h>
#else
#include <vulkan/vulkan.h>
#endif

// Forward declarations for Vulkan types
struct VkCommandBuffer_T;
typedef struct VkCommandBuffer_T *VkCommandBuffer;

struct VkImageView_T;
typedef struct VkImageView_T *VkImageView;

typedef uint64_t VkAccelerationStructureKHR;

// Forward declaration for VulkanRenderer
typedef struct VulkanRenderer {
  void *device;
  void *instance;
  void *physical_device;
} VulkanRenderer;

#endif // VULKAN_H