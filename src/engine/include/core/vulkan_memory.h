#ifndef VULKAN_MEMORY_H
#define VULKAN_MEMORY_H

#include <core/memory.h>

#ifdef VULKAN_BUILD
#include <vulkan/vulkan.h>
#else
// Forward declarations/dummies if Vulkan not enabled
// But this header implies Vulkan usage.
#ifndef VKAPI_PTR
#define VKAPI_PTR
#endif
typedef int VkSystemAllocationScope;
#endif

// Vulkan allocation callbacks - exposed for testing and advanced configuration
// In normal usage, use memory_get_vulkan_callbacks()

void* VKAPI_PTR vulkan_alloc_callback(void* user_data, size_t size, size_t alignment, VkSystemAllocationScope allocation_scope);
void* VKAPI_PTR vulkan_realloc_callback(void* user_data, void* original, size_t size, size_t alignment, VkSystemAllocationScope allocation_scope);
void VKAPI_PTR vulkan_free_callback(void* user_data, void* memory);

#endif // VULKAN_MEMORY_H
