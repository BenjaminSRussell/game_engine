// Vulkan image and texture management
#include <common.h>
#include <core/logger.h>
#include <rendering/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
#include <include/rendering/vulkan.h>

// Forward declaration for find_memory_type
static u32 find_memory_type(VulkanRenderer *renderer, u32 type_filter,
                            VkMemoryPropertyFlags properties);

// VULKAN-IMG-001: Image Pooling System Implementation
// ==================================================

// Image pool entry structure
typedef struct ImagePoolEntry {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  u32 width;
  u32 height;
  VkFormat format;
  VkImageUsageFlags usage;
  VkImageTiling tiling;
  VkMemoryPropertyFlags properties;
  bool in_use;
  bool has_view;
  struct ImagePoolEntry *next;
} ImagePoolEntry;

// Image pool structure
typedef struct ImagePool {
  ImagePoolEntry *free_list;
  ImagePoolEntry *used_list;
  u32 total_images;
  u32 free_images;
  u32 used_images;
  VkDeviceSize total_memory;
  VkDeviceSize used_memory;
  VkDeviceSize peak_memory_usage;
} ImagePool;

// Global image pools for different usage types
static ImagePool g_texture_pool = {0};
static ImagePool g_render_target_pool = {0};
static ImagePool g_depth_stencil_pool = {0};

// Image pool statistics
typedef struct ImagePoolStats {
  u32 total_allocations;
  u32 total_deallocations;
  u32 pool_hits;
  u32 pool_misses;
  u32 fragmentation_events;
  VkDeviceSize total_allocated;
  VkDeviceSize total_freed;
  f32 average_allocation_size;
  f32 pool_hit_ratio;
} ImagePoolStats;

static ImagePoolStats g_image_pool_stats = {0};

// Initialize image pool
static void image_pool_init(ImagePool *pool) {
  if (!pool)
    return;

  pool->free_list = NULL;
  pool->used_list = NULL;
  pool->total_images = 0;
  pool->free_images = 0;
  pool->used_images = 0;
  pool->total_memory = 0;
  pool->used_memory = 0;
  pool->peak_memory_usage = 0;
}

// Find suitable image in pool
static ImagePoolEntry *
image_pool_find_suitable(ImagePool *pool, u32 width, u32 height,
                         VkFormat format, VkImageUsageFlags usage,
                         VkImageTiling tiling,
                         VkMemoryPropertyFlags properties) {
  ImagePoolEntry *entry = pool->free_list;
  ImagePoolEntry *best_fit = NULL;
  VkDeviceSize best_size_diff = UINT64_MAX;

  while (entry) {
    // Check if image meets requirements
    if (entry->width >= width && entry->height >= height &&
        entry->format == format && (entry->usage & usage) == usage &&
        entry->tiling == tiling &&
        (entry->properties & properties) == properties) {

      VkDeviceSize size_diff =
          (entry->width * entry->height) - (width * height);
      if (size_diff < best_size_diff) {
        best_fit = entry;
        best_size_diff = size_diff;

        // Perfect match found
        if (size_diff == 0) {
          break;
        }
      }
    }
    entry = entry->next;
  }

  return best_fit;
}

// Remove entry from list
static void image_pool_remove_from_list(ImagePoolEntry **list,
                                        ImagePoolEntry *entry) {
  if (!list || !entry)
    return;

  if (*list == entry) {
    *list = entry->next;
  } else {
    ImagePoolEntry *current = *list;
    while (current && current->next != entry) {
      current = current->next;
    }
    if (current) {
      current->next = entry->next;
    }
  }
  entry->next = NULL;
}

// Add entry to front of list
static void image_pool_add_to_list(ImagePoolEntry **list,
                                   ImagePoolEntry *entry) {
  if (!list || !entry)
    return;

  entry->next = *list;
  *list = entry;
}

// Get appropriate pool for image usage
static ImagePool *image_pool_get_for_usage(VkImageUsageFlags usage) {
  if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
    return &g_render_target_pool;
  } else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
    return &g_depth_stencil_pool;
  } else if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
    return &g_texture_pool;
  }
  return NULL;
}

// Create image view for pool entry
static bool image_pool_create_view(VulkanRenderer *renderer,
                                   ImagePoolEntry *entry) {
  if (!renderer || !entry || entry->has_view)
    return false;

  VkImageViewCreateInfo view_info = {0};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = entry->image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = entry->format;
  view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  // Adjust for depth/stencil images
  if (entry->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  }

  if (vkCreateImageView(renderer->device, &view_info, NULL, &entry->view) !=
      VK_SUCCESS) {
    return false;
  }

  entry->has_view = true;
  return true;
}

// Allocate image from pool
static bool image_pool_allocate(VulkanRenderer *renderer, u32 width, u32 height,
                                VkFormat format, VkImageTiling tiling,
                                VkImageUsageFlags usage,
                                VkMemoryPropertyFlags properties,
                                VkImage *image, VkDeviceMemory *memory,
                                VkImageView *view, bool create_view) {
  ImagePool *pool = image_pool_get_for_usage(usage);

  if (pool) {
    // Try to find suitable image in pool
    ImagePoolEntry *entry = image_pool_find_suitable(
        pool, width, height, format, usage, tiling, properties);

    if (entry) {
      // Pool hit - reuse existing image
      image_pool_remove_from_list(&pool->free_list, entry);
      image_pool_add_to_list(&pool->used_list, entry);

      entry->in_use = true;
      pool->free_images--;
      pool->used_images++;

      *image = entry->image;
      *memory = entry->memory;

      // Create view if needed and not already created
      if (create_view && !entry->has_view) {
        if (!image_pool_create_view(renderer, entry)) {
          // View creation failed, return to pool
          image_pool_remove_from_list(&pool->used_list, entry);
          image_pool_add_to_list(&pool->free_list, entry);
          entry->in_use = false;
          pool->free_images++;
          pool->used_images--;
          return false;
        }
      }

      *view = entry->has_view ? entry->view : VK_NULL_HANDLE;

      g_image_pool_stats.pool_hits++;
      return true;
    }
  }

  // Pool miss - create new image
  g_image_pool_stats.pool_misses++;

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
    return false;
  }

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(renderer->device, *image, &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      find_memory_type(renderer, mem_requirements.memoryTypeBits, properties);

  if (vkAllocateMemory(renderer->device, &alloc_info, NULL, memory) !=
      VK_SUCCESS) {
    vkDestroyImage(renderer->device, *image, NULL);
    return false;
  }

  vkBindImageMemory(renderer->device, *image, *memory, 0);

  // Create image view if requested
  VkImageView created_view = VK_NULL_HANDLE;
  if (create_view) {
    VkImageViewCreateInfo view_info = {0};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = *image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    // Adjust for depth/stencil images
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    if (vkCreateImageView(renderer->device, &view_info, NULL, &created_view) !=
        VK_SUCCESS) {
      vkDestroyImage(renderer->device, *image, NULL);
      vkFreeMemory(renderer->device, *memory, NULL);
      return false;
    }
  }

  *view = created_view;

  // Add to pool statistics
  if (pool) {
    ImagePoolEntry *new_entry = malloc(sizeof(ImagePoolEntry));
    new_entry->image = *image;
    new_entry->memory = *memory;
    new_entry->view = created_view;
    new_entry->width = width;
    new_entry->height = height;
    new_entry->format = format;
    new_entry->usage = usage;
    new_entry->tiling = tiling;
    new_entry->properties = properties;
    new_entry->in_use = true;
    new_entry->has_view = (created_view != VK_NULL_HANDLE);
    new_entry->next = NULL;

    image_pool_add_to_list(&pool->used_list, new_entry);

    pool->total_images++;
    pool->used_images++;
    pool->total_memory += mem_requirements.size;
    pool->used_memory += mem_requirements.size;

    if (pool->used_memory > pool->peak_memory_usage) {
      pool->peak_memory_usage = pool->used_memory;
    }
  }

  // Update global statistics
  g_image_pool_stats.total_allocations++;
  g_image_pool_stats.total_allocated += mem_requirements.size;
  g_image_pool_stats.average_allocation_size =
      (f32)g_image_pool_stats.total_allocated /
      g_image_pool_stats.total_allocations;

  return true;
}

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

// VULKAN-IMG-001: Image Pooling System Public API
// ===============================================

// Return image to pool
static void image_pool_deallocate(VkDevice device, VkImage image,
                                  VkDeviceMemory memory, VkImageView view) {
  // Find the image in used lists
  ImagePool *pools[] = {&g_texture_pool, &g_render_target_pool,
                        &g_depth_stencil_pool};

  for (int i = 0; i < 3; i++) {
    ImagePool *pool = pools[i];
    ImagePoolEntry *entry = pool->used_list;

    while (entry) {
      if (entry->image == image) {
        // Return to pool
        image_pool_remove_from_list(&pool->used_list, entry);
        image_pool_add_to_list(&pool->free_list, entry);

        entry->in_use = false;
        pool->free_images++;
        pool->used_images--;
        pool->used_memory -= 0; // Memory remains allocated for reuse

        g_image_pool_stats.total_deallocations++;
        g_image_pool_stats.total_freed += 0; // Memory not freed, just reused
        return;
      }
      entry = entry->next;
    }
  }

  // Image not found in pools - destroy it
  if (view != VK_NULL_HANDLE) {
    vkDestroyImageView(device, view, NULL);
  }
  vkDestroyImage(device, image, NULL);
  vkFreeMemory(device, memory, NULL);
}

// Cleanup image pool
static void image_pool_cleanup(VkDevice device, ImagePool *pool) {
  if (!pool)
    return;

  // Destroy all images in free list
  ImagePoolEntry *entry = pool->free_list;
  while (entry) {
    ImagePoolEntry *next = entry->next;
    if (entry->has_view) {
      vkDestroyImageView(device, entry->view, NULL);
    }
    vkDestroyImage(device, entry->image, NULL);
    vkFreeMemory(device, entry->memory, NULL);
    free(entry);
    entry = next;
  }

  // Destroy all images in used list
  entry = pool->used_list;
  while (entry) {
    ImagePoolEntry *next = entry->next;
    if (entry->has_view) {
      vkDestroyImageView(device, entry->view, NULL);
    }
    vkDestroyImage(device, entry->image, NULL);
    vkFreeMemory(device, entry->memory, NULL);
    free(entry);
    entry = next;
  }

  image_pool_init(pool);
}

// Initialize all image pools
void vulkan_image_pool_init(VkDevice device) {
  (void)device;

  image_pool_init(&g_texture_pool);
  image_pool_init(&g_render_target_pool);
  image_pool_init(&g_depth_stencil_pool);

  memset(&g_image_pool_stats, 0, sizeof(g_image_pool_stats));
}

// Cleanup all image pools
void vulkan_image_pool_cleanup(VkDevice device) {
  image_pool_cleanup(device, &g_texture_pool);
  image_pool_cleanup(device, &g_render_target_pool);
  image_pool_cleanup(device, &g_depth_stencil_pool);
}

// Get image pool statistics
void vulkan_image_pool_get_stats(ImagePoolStats *stats) {
  if (!stats)
    return;

  *stats = g_image_pool_stats;

  // Calculate hit ratio
  u32 total_requests =
      g_image_pool_stats.pool_hits + g_image_pool_stats.pool_misses;
  if (total_requests > 0) {
    stats->pool_hit_ratio = (f32)g_image_pool_stats.pool_hits / total_requests;
  }

  // Aggregate pool statistics
  stats->fragmentation_events = 0; // Could be calculated based on unused memory
}

// Print image pool statistics
void vulkan_image_pool_print_stats() {
  printf("=== Vulkan Image Pool Statistics ===\n");
  printf("Total Allocations: %u\n", g_image_pool_stats.total_allocations);
  printf("Total Deallocations: %u\n", g_image_pool_stats.total_deallocations);
  printf("Pool Hits: %u\n", g_image_pool_stats.pool_hits);
  printf("Pool Misses: %u\n", g_image_pool_stats.pool_misses);

  u32 total_requests =
      g_image_pool_stats.pool_hits + g_image_pool_stats.pool_misses;
  if (total_requests > 0) {
    f32 hit_ratio = (f32)g_image_pool_stats.pool_hits / total_requests * 100.0f;
    printf("Pool Hit Ratio: %.2f%%\n", hit_ratio);
  }

  printf("Total Allocated: %.2f MB\n",
         g_image_pool_stats.total_allocated / (1024.0f * 1024.0f));
  printf("Total Freed: %.2f MB\n",
         g_image_pool_stats.total_freed / (1024.0f * 1024.0f));
  printf("Average Allocation Size: %.2f KB\n",
         g_image_pool_stats.average_allocation_size / 1024.0f);

  printf("\nPool Details:\n");
  printf("Texture Pool: %u total, %u free, %u used\n",
         g_texture_pool.total_images, g_texture_pool.free_images,
         g_texture_pool.used_images);
  printf("Render Target Pool: %u total, %u free, %u used\n",
         g_render_target_pool.total_images, g_render_target_pool.free_images,
         g_render_target_pool.used_images);
  printf("Depth Stencil Pool: %u total, %u free, %u used\n",
         g_depth_stencil_pool.total_images, g_depth_stencil_pool.free_images,
         g_depth_stencil_pool.used_images);

  printf("=====================================\n");
}

// Create image with pooling (enhanced version)
bool vulkan_create_image_pooled(VulkanRenderer *renderer, u32 width, u32 height,
                                VkFormat format, VkImageTiling tiling,
                                VkImageUsageFlags usage,
                                VkMemoryPropertyFlags properties,
                                VkImage *image, VkDeviceMemory *image_memory,
                                VkImageView *view, bool create_view) {
#ifdef VULKAN_BUILD
  // Try to allocate from pool first
  if (image_pool_allocate(renderer, width, height, format, tiling, usage,
                          properties, image, image_memory, view, create_view)) {
    return true;
  }

  // Pool allocation failed, fall back to direct allocation
  return vulkan_create_image(renderer, width, height, format, tiling, usage,
                             properties, image, image_memory);
#else
  (void)renderer;
  (void)width;
  (void)height;
  (void)format;
  (void)tiling;
  (void)usage;
  (void)properties;
  (void)image;
  (void)image_memory;
  (void)view;
  (void)create_view;
  return true;
#endif
}

// Destroy image with pooling
void vulkan_destroy_image_pooled(VulkanRenderer *renderer, VkImage image,
                                 VkDeviceMemory memory, VkImageView view) {
#ifdef VULKAN_BUILD
  image_pool_deallocate(renderer->device, image, memory, view);
#else
  (void)renderer;
  (void)image;
  (void)memory;
  (void)view;
#endif
}
