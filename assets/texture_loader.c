// src/assets/texture_loader.c
//
// Module Overview:
// This module provides a robust texture loading and atlas management system
// specifically designed for the Vulkan rendering backend. Its primary responsibility
// is to load pre-packed texture atlases (PNG images) and their associated JSON mapping
// files (detailing UV coordinates for individual textures within the atlas). It handles
// file I/O via the Virtual File System (VFS), parses JSON data to extract texture metadata,
// and orchestrates the creation of Vulkan image, image view, and sampler objects.
//
// Key Flows:
// 1. **Initialization:** `texture_load_atlas_map` reads the `block_atlas_map.json` file,
//    parsing version, tile size, tiles per row, and individual tile names/indices.
//    `texture_validate_atlas_map` ensures the consistency of the loaded map.
// 2. **Texture Loading:** `texture_load_from_file` reads a texture image (e.g., `block_atlas.png`)
//    from the VFS, uses `stb_image` (or a placeholder) to decode it, and then stages
//    the pixel data to a Vulkan staging buffer.
// 3. **Vulkan Resource Creation:** The module then creates a Vulkan image and allocates
//    device memory for it, transferring the pixel data from the staging buffer. It handles
//    image layout transitions (undefined -> transfer destination -> shader read only).
// 4. **Sampler and Descriptor Setup:** `texture_create_sampler` creates a Vulkan sampler
//    object for filtering and addressing modes. `texture_setup_descriptors` allocates
//    and updates descriptor sets to bind the loaded texture and sampler for shader access.
// 5. **UV Lookup:** `texture_get_uv` provides UV coordinates for a given texture ID,
//    allowing shaders to sample the correct portion of the atlas.
//
// Invariants:
// - Assumes a `VulkanRenderer` instance is properly initialized.
// - Relies on the `VFS` for all file access.
// - `block_atlas.png` and `block_atlas_map.json` are expected to exist at predefined paths
//   (`assets/textures/atlas/`).
// - The `block_atlas_map.json` format must be adhered to for correct parsing.
// - `stb_image.h` (or a compatible image loading library) is required for image decoding.
// - Texture IDs map directly to indices within the atlas map.
// - All Vulkan resource creation/destruction must be handled carefully to avoid leaks.
//
// (Additional comments from the file indicating external dependencies and generation processes are preserved.)
// Texture loading helpers for Vulkan using VFS and in-memory images.
//
// ASSET LOCATION: Block Texture Atlas
// ===========================================
// This loader expects a pre-packed texture atlas and mapping JSON.
//
// Required Files:
//   1. assets/textures/atlas/block_atlas.png
//      - 512x512 texture atlas containing all block textures
//      - Generated from individual block textures
//
//   2. assets/textures/atlas/block_atlas_map.json
//      - JSON file mapping block names to UV coordinates
//      - Format: {"version": 1, "tile_size": 32, "tiles_per_row": 16, "tiles": {...}}
//
// How to Generate:
//   1. Download block textures from:
//      - Faithful 32x32: https://faithful.team (CC0)
//      - Kenney Voxel Pack: https://kenney.nl/assets/voxel-pack (CC0)
//      - OpenGameArt: https://opengameart.org/content/faithful-32x32-texture-pack
//
//   2. Place individual textures in: assets/textures/blocks/
//      Example: stone.png, grass_top.png, dirt.png, etc.
//
//   3. Run packing script: scripts/pack_texture_atlas.py
//      (See: docs/QUICK_START_ASSETS.md Part 2 for instructions)
//
//   4. Output will be placed in: assets/textures/atlas/
//
// See: docs/ASSET_ACQUISITION_ROADMAP.md for complete guide
//
#include "../../include/render/vulkan.h"
#include "../../include/vfs/vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef VULKAN_BUILD
#include <vulkan/vulkan.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
// DEPENDENCY: stb_image.h (Image Loader Library)
// Download from: https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
// Place at: include/vendor/stb_image.h
// Or run: scripts/download_dependencies.sh
// #include "../vendor/stb_image.h"  // Uncomment when stb_image.h is downloaded

// Placeholder stb_image functions
typedef unsigned char stbi_uc;
static stbi_uc *stbi_load_from_memory(void *buffer, int len, int *x, int *y,
                                      int *channels_in_file,
                                      int desired_channels) {
  (void)buffer;
  (void)len;
  (void)channels_in_file;
  (void)desired_channels;
  *x = 64; // Use 64x64 for better visibility
  *y = 64;
  stbi_uc *pixels = (stbi_uc *)malloc(*x * *y * 4);
  for (int i = 0; i < *y; ++i) {
    for (int j = 0; j < *x; ++j) {
      int idx = (i * *x + j) * 4;
      // Checkerboard pattern (8x8 squares)
      bool light = ((i / 8) + (j / 8)) % 2 == 0;
      if (light) {
        pixels[idx] = 255;
        pixels[idx + 1] = 0;
        pixels[idx + 2] = 255;
        pixels[idx + 3] = 255; // Magenta
      } else {
        pixels[idx] = 0;
        pixels[idx + 1] = 255;
        pixels[idx + 2] = 0;
        pixels[idx + 3] = 255; // Green
      }
    }
  }
  return pixels;
}
static void stbi_image_free(void *retval_from_stbi_load) {
  free(retval_from_stbi_load);
}
#endif

// Simple JSON parser for block_atlas_map.json
typedef struct {
  int version;
  int tile_size;
  int tiles_per_row;
  // Maximum 1024 entries for now
  char *tile_names[1024];
  int max_index;
} AtlasMap;

static AtlasMap g_atlas_map = {0};

// Minimal JSON parsing: find integer after "key"
static int json_extract_int(const char *json, const char *key) {
  char search[64];
  snprintf(search, sizeof(search), "\"%s\":", key);
  const char *p = strstr(json, search);
  if (!p)
    return -1;
  p += strlen(search);
  while (*p && (*p == ' ' || *p == '\t' || *p == '\n'))
    p++;
  return atoi(p);
}

// Extract tile names dictionary: "tiles": {"0":"stone","1":"grass_top",...}
static void json_extract_tiles(const char *json, AtlasMap *map) {
  const char *tiles_start = strstr(json, "\"tiles\":");
  if (!tiles_start)
    return;
  const char *obj_start = strchr(tiles_start, '{');
  if (!obj_start)
    return;
  obj_start++;
  const char *p = obj_start;
  while (*p && *p != '}') {
    // Skip whitespace
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n'))
      p++;
    if (*p == '"') {
      p++; // skip opening quote
      // Parse index
      int idx = atoi(p);
      // Skip to value string
      while (*p && *p != ':')
        p++;
      if (*p == ':')
        p++;
      while (*p && (*p == ' ' || *p == '\t'))
        p++;
      if (*p == '"') {
        p++; // skip opening quote of value
        const char *val_start = p;
        while (*p && *p != '"')
          p++;
        int val_len = p - val_start;
        if (idx >= 0 && idx < 1024) {
          if (!map->tile_names[idx]) {
            map->tile_names[idx] = malloc(val_len + 1);
            memcpy(map->tile_names[idx], val_start, val_len);
            map->tile_names[idx][val_len] = '\0';
            if (idx > map->max_index)
              map->max_index = idx;
          }
        }
        p++; // skip closing quote
      }
    }
    // Skip to next entry
    while (*p && *p != ',' && *p != '}')
      p++;
    if (*p == ',')
      p++;
  }
}

// Load atlas map JSON from VFS
bool texture_load_atlas_map(VFS *vfs, const char *path) {
  VFSFile *file = vfs_open(vfs, path, VFS_MODE_READ);
  if (!file) {
    LOG_ERROR("Failed to open atlas map file: %s", path);
    return false;
  }
  u64 size = vfs_size(file);
  char *json = malloc(size + 1);
  vfs_read(file, json, size);
  json[size] = '\0';
  vfs_close(file);

  // Free previous entries
  for (int i = 0; i <= g_atlas_map.max_index; ++i) {
    if (g_atlas_map.tile_names[i]) {
      free(g_atlas_map.tile_names[i]);
      g_atlas_map.tile_names[i] = NULL;
    }
  }
  memset(&g_atlas_map, 0, sizeof(g_atlas_map));

  g_atlas_map.version = json_extract_int(json, "version");
  g_atlas_map.tile_size = json_extract_int(json, "tile_size");
  g_atlas_map.tiles_per_row = json_extract_int(json, "tiles_per_row");
  json_extract_tiles(json, &g_atlas_map);

  free(json);
  LOG_INFO(
      "Atlas map loaded: version=%d tile_size=%d tiles_per_row=%d max_index=%d",
      g_atlas_map.version, g_atlas_map.tile_size, g_atlas_map.tiles_per_row,
      g_atlas_map.max_index);
  return true;
}

// Get UV rect for a texture_id (index) in the atlas
bool texture_get_uv(int texture_id, float *u_min, float *v_min, float *u_max,
                    float *v_max) {
  if (texture_id < 0 || texture_id > g_atlas_map.max_index ||
      !g_atlas_map.tile_names[texture_id]) {
    LOG_ERROR("Invalid texture_id: %d", texture_id);
    return false;
  }
  int tiles_per_row = g_atlas_map.tiles_per_row;
  int tile_size = g_atlas_map.tile_size;
  int col = texture_id % tiles_per_row;
  int row = texture_id / tiles_per_row;
  float inv_tiles_per_row = 1.0f / tiles_per_row;
  *u_min = col * inv_tiles_per_row;
  *v_min = row * inv_tiles_per_row;
  *u_max = (col + 1) * inv_tiles_per_row;
  *v_max = (row + 1) * inv_tiles_per_row;
  return true;
}

// Get tile name for texture_id (for debugging)
const char *texture_get_name(int texture_id) {
  if (texture_id < 0 || texture_id > g_atlas_map.max_index)
    return NULL;
  return g_atlas_map.tile_names[texture_id];
}

// Validate atlas map consistency (called after load)
bool texture_validate_atlas_map(void) {
  if (g_atlas_map.tile_size != 32) {
    LOG_ERROR("Atlas map tile_size must be 32, got %d", g_atlas_map.tile_size);
    return false;
  }
  if (g_atlas_map.tiles_per_row <= 0 ||
      (g_atlas_map.tiles_per_row & (g_atlas_map.tiles_per_row - 1)) != 0) {
    LOG_ERROR("Atlas map tiles_per_row must be power of two, got %d",
              g_atlas_map.tiles_per_row);
    return false;
  }
  // Ensure no gaps in indices up to max_index
  for (int i = 0; i <= g_atlas_map.max_index; ++i) {
    if (!g_atlas_map.tile_names[i]) {
      LOG_ERROR("Atlas map missing entry for index %d", i);
      return false;
    }
  }
  LOG_INFO("Atlas map validation passed");
  return true;
}

// Load texture from file
bool texture_load_from_file(VulkanRenderer *renderer, VFS *vfs,
                            const char *path, VkImage *texture_image,
                            VkDeviceMemory *texture_image_memory) {
#ifdef VULKAN_BUILD
  VFSFile *file = vfs_open(vfs, path, VFS_MODE_READ_BINARY);
  if (!file) {
    LOG_ERROR("Failed to open texture file: %s", path);
    return false;
  }

  u64 file_size = vfs_size(file);
  if (file_size == 0 || file_size > 100 * 1024 * 1024) { // Cap at 100MB
    LOG_ERROR("Invalid texture file size: %llu bytes", file_size);
    vfs_close(file);
    return false;
  }

  void *file_data = malloc(file_size);
  if (!file_data) {
    LOG_ERROR("OOM: Failed to allocate %llu bytes for texture", file_size);
    vfs_close(file);
    return false;
  }

  u64 bytes_read = vfs_read(file, file_data, file_size);
  if (bytes_read != file_size) {
    LOG_ERROR("Short read: expected %llu bytes, got %llu", file_size,
              bytes_read);
    free(file_data);
    vfs_close(file);
    return false;
  }
  vfs_close(file);

  int width, height, channels;
  stbi_uc *pixels = stbi_load_from_memory(file_data, (int)file_size, &width,
                                          &height, &channels, STBI_rgb_alpha);
  free(file_data);

  if (!pixels) {
    LOG_ERROR("Failed to load texture image: %s", path);
    return false;
  }

  VkDeviceSize image_size = width * height * 4;

  // Create staging buffer
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  if (!vulkan_create_buffer(renderer, image_size,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &staging_buffer, &staging_buffer_memory)) {
    stbi_image_free(pixels);
    return false;
  }

  // Copy pixel data to staging buffer
  void *data;
  VkResult map_result = vkMapMemory(renderer->device, staging_buffer_memory, 0,
                                    image_size, 0, &data);
  if (map_result != VK_SUCCESS) {
    LOG_ERROR("vkMapMemory failed with error code: %d", map_result);
    stbi_image_free(pixels);
    vkDestroyBuffer(renderer->device, staging_buffer, NULL);
    vkFreeMemory(renderer->device, staging_buffer_memory, NULL);
    return false;
  }
  memcpy(data, pixels, image_size);
  vkUnmapMemory(renderer->device, staging_buffer_memory);

  stbi_image_free(pixels);

  // Create image
  VkImageCreateInfo image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(renderer->device, &image_info, NULL, texture_image) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to create texture image");
    vkDestroyBuffer(renderer->device, staging_buffer, NULL);
    vkFreeMemory(renderer->device, staging_buffer_memory, NULL);
    return false;
  }

  // Allocate image memory
  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(renderer->device, *texture_image,
                               &mem_requirements);

  // Find suitable memory type
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(renderer->physical_device,
                                      &mem_properties);

  u32 memory_type_index = UINT32_MAX;
  VkMemoryPropertyFlags required_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((mem_requirements.memoryTypeBits & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & required_props) ==
            required_props) {
      memory_type_index = i;
      break;
    }
  }

  if (memory_type_index == UINT32_MAX) {
    LOG_ERROR("Failed to find suitable memory type for texture image");
    vkDestroyImage(renderer->device, *texture_image, NULL);
    vkDestroyBuffer(renderer->device, staging_buffer, NULL);
    vkFreeMemory(renderer->device, staging_buffer_memory, NULL);
    return false;
  }

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = memory_type_index;
  // Vulkan texture pipeline roadmap: docs/TEXTURE_LOADER_ROADMAP.md.

  if (vkAllocateMemory(renderer->device, &alloc_info, NULL,
                       texture_image_memory) != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate texture image memory");
    vkDestroyImage(renderer->device, *texture_image, NULL);
    vkDestroyBuffer(renderer->device, staging_buffer, NULL);
    vkFreeMemory(renderer->device, staging_buffer_memory, NULL);
    return false;
  }

  // Create staging buffer (existing code)
  // ... (staging buffer creation)

  // NOTE: Helper functions for one-time commands should be in vulkan_utils.c,
  // but inline here for now

  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = renderer->command_pool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(renderer->device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  // 1. Transition to TRANSFER_DST
  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = *texture_image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                       &barrier);

  // 2. Copy buffer to image
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

  vkCmdCopyBufferToImage(commandBuffer, staging_buffer, *texture_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  // 3. Transition to SHADER_READ_ONLY
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0,
                       NULL, 1, &barrier);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(renderer->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(renderer->graphics_queue);

  vkFreeCommandBuffers(renderer->device, renderer->command_pool, 1,
                       &commandBuffer);

  vkDestroyBuffer(renderer->device, staging_buffer, NULL);
  vkFreeMemory(renderer->device, staging_buffer_memory, NULL);

  LOG_INFO("Texture loaded and uploaded: %s (%dx%d)", path, width, height);
  return true;
#else
  (void)renderer;
  (void)vfs;
  (void)path;
  (void)texture_image;
  (void)texture_image_memory;
  return true;
#endif
}

// Setup descriptor sets for texture sampling
bool texture_setup_descriptors(VulkanRenderer *renderer) {
#ifdef VULKAN_BUILD
  if (!renderer->descriptor_set_layout || !renderer->descriptor_pool ||
      !renderer->texture_image_view || !renderer->texture_sampler) {
    LOG_ERROR("Missing dependencies for descriptor setup");
    return false;
  }

  // Allocate descriptor sets (one for each frame in flight)
  if (renderer->descriptor_sets) {
    free(renderer->descriptor_sets);
  }
  renderer->descriptor_sets =
      malloc(sizeof(VkDescriptorSet) * renderer->max_frames_in_flight);

  VkDescriptorSetLayout *layouts =
      malloc(sizeof(VkDescriptorSetLayout) * renderer->max_frames_in_flight);
  for (uint32_t i = 0; i < renderer->max_frames_in_flight; i++) {
    layouts[i] = renderer->descriptor_set_layout;
  }

  VkDescriptorSetAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = renderer->descriptor_pool;
  alloc_info.descriptorSetCount = renderer->max_frames_in_flight;
  alloc_info.pSetLayouts = layouts;

  if (vkAllocateDescriptorSets(renderer->device, &alloc_info,
                               renderer->descriptor_sets) != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate descriptor sets");
    free(layouts);
    return false;
  }
  free(layouts);

  // Update descriptor sets
  for (uint32_t i = 0; i < renderer->max_frames_in_flight; i++) {
    VkDescriptorImageInfo image_info = {0};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = renderer->texture_image_view;
    image_info.sampler = renderer->texture_sampler;

    VkWriteDescriptorSet descriptor_write = {0};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = renderer->descriptor_sets[i];
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(renderer->device, 1, &descriptor_write, 0, NULL);
  }

  LOG_INFO("Descriptor sets setup successfully");
  return true;
#else
  (void)renderer;
  return true;
#endif
}

// Load texture atlas
bool texture_load_atlas(VulkanRenderer *renderer, VFS *vfs,
                        const char *atlas_path) {
  if (!texture_load_from_file(renderer, vfs, atlas_path,
                              &renderer->texture_image,
                              &renderer->texture_image_memory)) {
    return false;
  }

// Create image view
#ifdef VULKAN_BUILD
  VkImageViewCreateInfo view_info = {0};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = renderer->texture_image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
  view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  if (vkCreateImageView(renderer->device, &view_info, NULL,
                        &renderer->texture_image_view) != VK_SUCCESS) {
    LOG_ERROR("Failed to create texture image view");
    return false;
  }
#endif

  return true;
}

// Create texture sampler
bool texture_create_sampler(VulkanRenderer *renderer) {
#ifdef VULKAN_BUILD
  VkSamplerCreateInfo sampler_info = {0};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = VK_FILTER_NEAREST;
  sampler_info.minFilter = VK_FILTER_NEAREST;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.anisotropyEnable = VK_FALSE;
  sampler_info.maxAnisotropy = 1.0f;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0f;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = 0.0f;

  if (vkCreateSampler(renderer->device, &sampler_info, NULL,
                      &renderer->texture_sampler) != VK_SUCCESS) {
    LOG_ERROR("Failed to create texture sampler");
    return false;
  }

  return true;
#else
  (void)renderer;
  return true;
#endif
}
