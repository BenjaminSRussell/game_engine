#ifndef VULKAN_H
#define VULKAN_H

#include <vulkan/vulkan.h>

// Forward declaration for VulkanRenderer
typedef struct VulkanRenderer {
  void *device;
  void *instance;
  void *physical_device;
} VulkanRenderer;

#endif // VULKAN_H