// Vulkan Dispatch Table - Dynamic function loading for Vulkan 1.2/1.3 compatibility
// Implements dispatch table pattern to load Vulkan functions at runtime

#ifndef VULKAN_DISPATCH_H
#define VULKAN_DISPATCH_H

#include "include/common.h"

#ifdef VULKAN_BUILD

#include <include/rendering/vulkan.h>

// Vulkan dispatch table structure
typedef struct {
    // Instance functions
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceProperties2 GetPhysicalDeviceProperties2;
    PFN_vkGetPhysicalDeviceFeatures GetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceFeatures2 GetPhysicalDeviceFeatures2;
    PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;
    PFN_vkCreateDevice CreateDevice;
    PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties;
    
    // Device functions
    PFN_vkDestroyDevice DestroyDevice;
    PFN_vkGetDeviceQueue GetDeviceQueue;
    PFN_vkCreateBuffer CreateBuffer;
    PFN_vkDestroyBuffer DestroyBuffer;
    PFN_vkCreateImage CreateImage;
    PFN_vkDestroyImage DestroyImage;
    PFN_vkAllocateMemory AllocateMemory;
    PFN_vkFreeMemory FreeMemory;
    PFN_vkBindBufferMemory BindBufferMemory;
    PFN_vkBindImageMemory BindImageMemory;
    PFN_vkGetBufferMemoryRequirements GetBufferMemoryRequirements;
    PFN_vkGetImageMemoryRequirements GetImageMemoryRequirements;
    
    // Command buffer functions
    PFN_vkCreateCommandPool CreateCommandPool;
    PFN_vkDestroyCommandPool DestroyCommandPool;
    PFN_vkAllocateCommandBuffers AllocateCommandBuffers;
    PFN_vkFreeCommandBuffers FreeCommandBuffers;
    PFN_vkBeginCommandBuffer BeginCommandBuffer;
    PFN_vkEndCommandBuffer EndCommandBuffer;
    PFN_vkQueueSubmit QueueSubmit;
    PFN_vkQueueWaitIdle QueueWaitIdle;
    
    // Dynamic Rendering (VK 1.3 core or VK 1.2 extension)
    PFN_vkCmdBeginRendering CmdBeginRendering;           // VK 1.3 core
    PFN_vkCmdEndRendering CmdEndRendering;               // VK 1.3 core
    PFN_vkCmdBeginRenderingKHR CmdBeginRenderingKHR;     // VK 1.2 extension fallback
    PFN_vkCmdEndRenderingKHR CmdEndRenderingKHR;         // VK 1.2 extension fallback
    
    // Graphics commands
    PFN_vkCmdBindPipeline CmdBindPipeline;
    PFN_vkCmdDraw CmdDraw;
    PFN_vkCmdDrawIndexed CmdDrawIndexed;
    PFN_vkCmdBindVertexBuffers CmdBindVertexBuffers;
    PFN_vkCmdBindIndexBuffer CmdBindIndexBuffer;
    PFN_vkCmdSetViewport CmdSetViewport;
    PFN_vkCmdSetScissor CmdSetScissor;
    
    // Synchronization
    PFN_vkCreateSemaphore CreateSemaphore;
    PFN_vkDestroySemaphore DestroySemaphore;
    PFN_vkCreateFence CreateFence;
    PFN_vkDestroyFence DestroyFence;
    PFN_vkWaitForFences WaitForFences;
    PFN_vkResetFences ResetFences;
    
    // Additional features
    bool has_dynamic_rendering_core;  // VK 1.3
    bool has_dynamic_rendering_ext;   // VK_KHR_dynamic_rendering
    bool has_buffer_device_address;   // VK_KHR_buffer_device_address
    bool has_ray_query;               // VK_KHR_ray_query
    
} VulkanDispatchTable;

// Global dispatch table
extern VulkanDispatchTable g_vk_dispatch;

// Load Vulkan library and initialize dispatch table
bool vulkan_dispatch_load(void);
bool vulkan_dispatch_load_instance(VkInstance instance);
bool vulkan_dispatch_load_device(VkDevice device);
void vulkan_dispatch_unload(void);

// Feature detection
bool vulkan_has_dynamic_rendering(void);
bool vulkan_has_buffer_device_address(void);
bool vulkan_has_ray_query(void);

#endif // VULKAN_BUILD

#endif // VULKAN_DISPATCH_H
