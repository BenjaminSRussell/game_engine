// Queue Ownership Transfer for Cross-Queue Resource Sharing
// Implements proper synchronization for async compute workloads

#ifndef QUEUE_OWNERSHIP_H
#define QUEUE_OWNERSHIP_H

#include "../common.h"

#ifdef VULKAN_BUILD

#include <vulkan/vulkan.h>

// Queue ownership transfer operation
typedef struct {
    VkBuffer buffer;
    VkImage image;
    u32 src_queue_family;
    u32 dst_queue_family;
    VkAccessFlags src_access_mask;
    VkAccessFlags dst_access_mask;
    VkPipelineStageFlags src_stage_mask;
    VkPipelineStageFlags dst_stage_mask;
} QueueOwnershipTransfer;

// Release buffer ownership from source queue (e.g., compute)
void queue_ownership_release_buffer(
    VkCommandBuffer cmd_buffer,
    VkBuffer buffer,
    u32 src_queue_family,
    u32 dst_queue_family,
    VkAccessFlags src_access,
    VkPipelineStageFlags src_stage
);

// Acquire buffer ownership on destination queue (e.g., graphics)
void queue_ownership_acquire_buffer(
    VkCommandBuffer cmd_buffer,
    VkBuffer buffer,
    u32 src_queue_family,
    u32 dst_queue_family,
    VkAccessFlags dst_access,
    VkPipelineStageFlags dst_stage
);

// Example: Transfer vertex buffer from compute to graphics
void queue_ownership_transfer_vertex_buffer(
    VkCommandBuffer compute_cmd,
    VkCommandBuffer graphics_cmd,
    VkBuffer vertex_buffer,
    u32 compute_family,
    u32 graphics_family
);

#endif // VULKAN_BUILD

#endif // QUEUE_OWNERSHIP_H
