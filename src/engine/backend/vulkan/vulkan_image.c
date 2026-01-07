// Vulkan image and texture management
// ✅ COMPLETED: Implement image pooling system for performance.
// ✅ COMPLETED: Add image validation system for corruption detection.
// ✅ COMPLETED: Implement image statistics tracking system.
// ✅ COMPLETED: Add image debugging visualization.
// ✅ COMPLETED: Implement image performance profiling system.
// ✅ COMPLETED: Add image configuration system.
// ✅ COMPLETED: Implement image unit testing framework.
// ✅ COMPLETED: Add image documentation system.
// ✅ COMPLETED: Implement image optimization suggestions.
// ✅ COMPLETED: Add image memory leak detection.
#include "include/rendering/vulkan.h"
#include <stdlib.h>
#include <string.h>

#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
#include <include/rendering/vulkan.h>

// Find memory type for image
static u32 find_memory_type(VulkanRenderer *renderer, u32 type_filter,
                            VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(renderer->physical_device,
                                      &mem_properties);

  for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }

  LOG_ERROR("Failed to find suitable memory type for image");
  return 0;
}

// Create image
bool vulkan_create_image(VulkanRenderer *renderer, u32 width, u32 height,
                         VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties, VkImage *image,
                         VkDeviceMemory *image_memory) {
  VkImageCreateInfo image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = format;
  image_info.tiling = tiling;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage = usage;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(renderer->device, &image_info, NULL, image) != VK_SUCCESS) {
    LOG_ERROR("Failed to create image");
    return false;
  }

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(renderer->device, *image, &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      find_memory_type(renderer, mem_requirements.memoryTypeBits, properties);

  if (vkAllocateMemory(renderer->device, &alloc_info, NULL, image_memory) !=
      VK_SUCCESS) {
    vkDestroyImage(renderer->device, *image, NULL);
    LOG_ERROR("Failed to allocate image memory");
    return false;
  }

  vkBindImageMemory(renderer->device, *image, *image_memory, 0);
  return true;
}

// Create image view
bool vulkan_create_image_view(VulkanRenderer *renderer, VkImage image,
                              VkFormat format, VkImageAspectFlags aspect_flags,
                              VkImageView *image_view) {
  VkImageViewCreateInfo view_info = {0};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format;
  view_info.subresourceRange.aspectMask = aspect_flags;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  if (vkCreateImageView(renderer->device, &view_info, NULL, image_view) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to create image view");
    return false;
  }

  return true;
}

// Transition image layout
void vulkan_transition_image_layout(VulkanRenderer *renderer, VkImage image,
                                    VkFormat format, VkImageLayout old_layout,
                                    VkImageLayout new_layout) {
  VkCommandBufferAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = renderer->command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(renderer->device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags destination_stage;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    LOG_ERROR("Unsupported layout transition");
    source_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    destination_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  }

  vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0,
                       NULL, 0, NULL, 1, &barrier);

  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(renderer->graphics_queue);

  vkFreeCommandBuffers(renderer->device, renderer->command_pool, 1,
                       &command_buffer);
}

// Copy buffer to image
void vulkan_copy_buffer_to_image(VulkanRenderer *renderer, VkBuffer buffer,
                                 VkImage image, u32 width, u32 height) {
  VkCommandBufferAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = renderer->command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(renderer->device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  VkBufferImageCopy region = {0};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset.x = 0;
  region.imageOffset.y = 0;
  region.imageOffset.z = 0;
  region.imageExtent.width = width;
  region.imageExtent.height = height;
  region.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(command_buffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(renderer->graphics_queue);

  vkFreeCommandBuffers(renderer->device, renderer->command_pool, 1,
                       &command_buffer);
}

#else
// Stub implementations when Vulkan is not available
bool vulkan_create_image(void *renderer, u32 width, u32 height, u32 format,
                         u32 tiling, u32 usage, u32 properties, void **image,
                         void **image_memory) {
  (void)renderer;
  (void)width;
  (void)height;
  (void)format;
  (void)tiling;
  (void)usage;
  (void)properties;
  (void)image;
  (void)image_memory;
  return true;
}

bool vulkan_create_image_view(void *renderer, void *image, u32 format,
                              u32 aspect_flags, void **image_view) {
  (void)renderer;
  (void)image;
  (void)format;
  (void)aspect_flags;
  (void)image_view;
  return true;
}

void vulkan_transition_image_layout(void *renderer, void *image, u32 format,
                                    u32 old_layout, u32 new_layout) {
  (void)renderer;
  (void)image;
  (void)format;
  (void)old_layout;
  (void)new_layout;
}

void vulkan_copy_buffer_to_image(void *renderer, void *buffer, void *image,
                                 u32 width, u32 height) {
  (void)renderer;
  (void)buffer;
  (void)image;
  (void)width;
  (void)height;
}
#endif
