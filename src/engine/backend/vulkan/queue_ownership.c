// Queue Ownership Transfer Implementation
// Proper cross-queue resource sharing with barriers

#include <rendering/queue_ownership.h>
#include <core/logger.h>

#ifdef VULKAN_BUILD

void queue_ownership_release_buffer(
    VkCommandBuffer cmd_buffer,
    VkBuffer buffer,
    u32 src_queue_family,
    u32 dst_queue_family,
    VkAccessFlags src_access,
    VkPipelineStageFlags src_stage
) {
    if (!cmd_buffer || !buffer) {
        LOG_ERROR("Invalid parameters for queue ownership release");
        return;
    }
    
    // Create release barrier
    VkBufferMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = src_access,
        .dstAccessMask = 0,  // Ignored for release
        .srcQueueFamilyIndex = src_queue_family,
        .dstQueueFamilyIndex = dst_queue_family,
        .buffer = buffer,
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };
    
    vkCmdPipelineBarrier(
        cmd_buffer,
        src_stage,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,  // Release happens at end of pipeline
        0,  // No dependency flags
        0, NULL,  // No memory barriers
        1, &barrier,  // Buffer barrier
        0, NULL  // No image barriers
    );
    
    LOG_DEBUG("Released buffer ownership: queue %u -> %u", src_queue_family, dst_queue_family);
}

void queue_ownership_acquire_buffer(
    VkCommandBuffer cmd_buffer,
    VkBuffer buffer,
    u32 src_queue_family,
    u32 dst_queue_family,
    VkAccessFlags dst_access,
    VkPipelineStageFlags dst_stage
) {
    if (!cmd_buffer || !buffer) {
        LOG_ERROR("Invalid parameters for queue ownership acquire");
        return;
    }
    
    // Create acquire barrier
    VkBufferMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = 0,  // Ignored for acquire
        .dstAccessMask = dst_access,
        .srcQueueFamilyIndex = src_queue_family,
        .dstQueueFamilyIndex = dst_queue_family,
        .buffer = buffer,
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };
    
    vkCmdPipelineBarrier(
        cmd_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  // Acquire happens at start of pipeline
        dst_stage,
        0,  // No dependency flags
        0, NULL,  // No memory barriers
        1, &barrier,  // Buffer barrier
        0, NULL  // No image barriers
    );
    
    LOG_DEBUG("Acquired buffer ownership: queue %u -> %u", src_queue_family, dst_queue_family);
}

void queue_ownership_transfer_vertex_buffer(
    VkCommandBuffer compute_cmd,
    VkCommandBuffer graphics_cmd,
    VkBuffer vertex_buffer,
    u32 compute_family,
    u32 graphics_family
) {
    if (!compute_cmd || !graphics_cmd || !vertex_buffer) {
        LOG_ERROR("Invalid parameters for vertex buffer transfer");
        return;
    }
    
    // Release from compute queue
    queue_ownership_release_buffer(
        compute_cmd,
        vertex_buffer,
        compute_family,
        graphics_family,
        VK_ACCESS_SHADER_WRITE_BIT,  // Compute shader wrote to buffer
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
    );
    
    // Acquire on graphics queue
    queue_ownership_acquire_buffer(
        graphics_cmd,
        vertex_buffer,
        compute_family,
        graphics_family,
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,  // Graphics will read vertices
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
    );
    
    LOG_INFO("Vertex buffer ownership transferred: compute -> graphics");
}

#endif // VULKAN_BUILD
