// Async Compute Queue Management
// Enables parallel GPU execution of graphics and compute workloads

#ifndef ASYNC_COMPUTE_H
#define ASYNC_COMPUTE_H

#include "../common.h"

#ifdef VULKAN_BUILD

#include <vulkan/vulkan.h>

// Queue family indices
typedef struct {
    u32 graphics_family;
    u32 compute_family;
    u32 transfer_family;
    bool has_dedicated_compute;
    bool has_dedicated_transfer;
} QueueFamilies;

// Async compute system
typedef struct {
    VkDevice device;
    
    // Queue handles
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue transfer_queue;
    
    // Command pools
    VkCommandPool graphics_pool;
    VkCommandPool compute_pool;
    VkCommandPool transfer_pool;
    
    // Synchronization
    VkSemaphore compute_complete_semaphore;
    VkSemaphore graphics_complete_semaphore;
    
    QueueFamilies families;
    
} AsyncComputeSystem;

// Initialize async compute system
bool async_compute_init(VkDevice device, VkPhysicalDevice physical_device, AsyncComputeSystem* system);

// Detect queue families
bool async_compute_detect_queues(VkPhysicalDevice physical_device, QueueFamilies* families);

// Submit compute work to dedicated compute queue
void async_compute_submit(AsyncComputeSystem* system, VkCommandBuffer cmd_buffer);

// Wait for compute work to complete
void async_compute_wait(AsyncComputeSystem* system);

// Cleanup
void async_compute_shutdown(AsyncComputeSystem* system);

#endif // VULKAN_BUILD

#endif // ASYNC_COMPUTE_H
