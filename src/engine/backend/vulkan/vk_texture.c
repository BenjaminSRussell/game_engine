#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Vulkan texture management system
#define VK_MAX_TEXTURES 1024
#define VK_MAX_MIP_LEVELS 16
#define VK_MAX_ARRAY_LAYERS 256

typedef enum {
    VK_TEXTURE_TYPE_1D = 0,
    VK_TEXTURE_TYPE_2D,
    VK_TEXTURE_TYPE_3D,
    VK_TEXTURE_TYPE_CUBE,
    VK_TEXTURE_TYPE_1D_ARRAY,
    VK_TEXTURE_TYPE_2D_ARRAY,
    VK_TEXTURE_TYPE_CUBE_ARRAY,
    VK_TEXTURE_TYPE_COUNT
} vk_texture_type_t;

typedef struct vk_texture {
    VkImage image;
    VkImageView image_view;
    VkSampler sampler;
    VkDeviceMemory memory;
    
    vk_texture_type_t type;
    VkFormat format;
    u32 width;
    u32 height;
    u32 depth;
    u32 mip_levels;
    u32 array_layers;
    
    char name[256];
    u32 id;
    bool is_allocated;
    bool is_mipmapped;
    bool is_sampled;
} vk_texture_t;

typedef struct vk_texture_manager {
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkCommandPool command_pool;
    VkQueue graphics_queue;
    
    vk_texture_t textures[VK_MAX_TEXTURES];
    u32 texture_count;
    u32 next_texture_id;
    
    // Statistics
    u64 total_memory_used;
    u32 total_textures_created;
    u32 total_textures_destroyed;
} vk_texture_manager_t;

static vk_texture_manager_t g_texture_manager = {0};

// Initialize texture manager
bool vk_texture_manager_init(VkDevice device, VkPhysicalDevice physical_device, VkCommandPool command_pool, VkQueue graphics_queue) {
    if (!device || !physical_device || !command_pool || !graphics_queue) {
        printf("Error: Invalid parameters for texture manager initialization\n");
        return false;
    }
    
    g_texture_manager.device = device;
    g_texture_manager.physical_device = physical_device;
    g_texture_manager.command_pool = command_pool;
    g_texture_manager.graphics_queue = graphics_queue;
    
    printf("Vulkan texture manager initialized\n");
    return true;
}

// Cleanup texture manager
void vk_texture_manager_cleanup(void) {
    if (!g_texture_manager.device) {
        return;
    }
    
    // Destroy all textures
    for (u32 i = 0; i < g_texture_manager.texture_count; i++) {
        vk_texture_t* texture = &g_texture_manager.textures[i];
        
        if (texture->image_view != VK_NULL_HANDLE) {
            vkDestroyImageView(g_texture_manager.device, texture->image_view, NULL);
        }
        
        if (texture->sampler != VK_NULL_HANDLE) {
            vkDestroySampler(g_texture_manager.device, texture->sampler, NULL);
        }
        
        if (texture->image != VK_NULL_HANDLE) {
            vkDestroyImage(g_texture_manager.device, texture->image, NULL);
        }
        
        if (texture->memory != VK_NULL_HANDLE) {
            vkFreeMemory(g_texture_manager.device, texture->memory, NULL);
        }
    }
    
    memset(&g_texture_manager, 0, sizeof(g_texture_manager));
    
    printf("Vulkan texture manager cleaned up\n");
}

// Create 2D texture
u32 vk_texture_create_2d(const char* name, u32 width, u32 height, VkFormat format, bool is_mipmapped, bool is_sampled) {
    if (!name || !g_texture_manager.device || width == 0 || height == 0) {
        return 0;
    }
    
    if (g_texture_manager.texture_count >= VK_MAX_TEXTURES) {
        printf("Error: Maximum textures reached\n");
        return 0;
    }
    
    // Calculate mip levels
    u32 mip_levels = 1;
    if (is_mipmapped) {
        mip_levels = (u32)floor(log2(MAX(width, height))) + 1;
        mip_levels = MIN(mip_levels, VK_MAX_MIP_LEVELS);
    }
    
    // Create image
    VkImageCreateInfo image_info = {0};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkImage image;
    VkResult result = vkCreateImage(g_texture_manager.device, &image_info, NULL, &image);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create image\n");
        return 0;
    }
    
    // Allocate memory
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(g_texture_manager.device, image, &mem_requirements);
    
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = 0; // Simplified - should find proper memory type
    
    VkDeviceMemory memory;
    result = vkAllocateMemory(g_texture_manager.device, &alloc_info, NULL, &memory);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to allocate image memory\n");
        vkDestroyImage(g_texture_manager.device, image, NULL);
        return 0;
    }
    
    // Bind memory
    result = vkBindImageMemory(g_texture_manager.device, image, memory, 0);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to bind image memory\n");
        vkDestroyImage(g_texture_manager.device, image, NULL);
        vkFreeMemory(g_texture_manager.device, memory, NULL);
        return 0;
    }
    
    // Create image view
    VkImageViewCreateInfo view_info = {0};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    
    VkImageView image_view;
    result = vkCreateImageView(g_texture_manager.device, &view_info, NULL, &image_view);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create image view\n");
        vkDestroyImage(g_texture_manager.device, image, NULL);
        vkFreeMemory(g_texture_manager.device, memory, NULL);
        return 0;
    }
    
    // Create sampler
    VkSamplerCreateInfo sampler_info = {0};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    
    if (is_mipmapped) {
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.minLod = VK_LOD_CLAMP_NONE;
        sampler_info.maxLod = VK_LOD_CLAMP_NONE;
    } else {
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 0.0f;
    }
    
    VkSampler sampler;
    result = vkCreateSampler(g_texture_manager.device, &sampler_info, NULL, &sampler);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create sampler\n");
        vkDestroyImageView(g_texture_manager.device, image_view, NULL);
        vkDestroyImage(g_texture_manager.device, image, NULL);
        vkFreeMemory(g_texture_manager.device, memory, NULL);
        return 0;
    }
    
    // Store texture
    u32 texture_id = g_texture_manager.next_texture_id++;
    vk_texture_t* texture = &g_texture_manager.textures[texture_id - 1];
    
    texture->image = image;
    texture->image_view = image_view;
    texture->sampler = sampler;
    texture->memory = memory;
    texture->type = VK_TEXTURE_TYPE_2D;
    texture->format = format;
    texture->width = width;
    texture->height = height;
    texture->depth = 1;
    texture->mip_levels = mip_levels;
    texture->array_layers = 1;
    strncpy(texture->name, name, 255);
    texture->name[255] = '\0';
    texture->id = texture_id;
    texture->is_allocated = true;
    texture->is_mipmapped = is_mipmapped;
    texture->is_sampled = is_sampled;
    
    g_texture_manager.texture_count++;
    g_texture_manager.total_textures_created++;
    g_texture_manager.total_memory_used += mem_requirements.size;
    
    printf("Created 2D texture '%s' (%ux%u, mips: %u)\n", name, width, height, mip_levels);
    
    return texture_id;
}

// Get texture image
VkImage vk_texture_get_image(u32 texture_id) {
    if (!g_texture_manager.device || texture_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (texture_id > g_texture_manager.next_texture_id) {
        return VK_NULL_HANDLE;
    }
    
    return g_texture_manager.textures[texture_id - 1].image;
}

// Get texture image view
VkImageView vk_texture_get_image_view(u32 texture_id) {
    if (!g_texture_manager.device || texture_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (texture_id > g_texture_manager.next_texture_id) {
        return VK_NULL_HANDLE;
    }
    
    return g_texture_manager.textures[texture_id - 1].image_view;
}

// Get texture sampler
VkSampler vk_texture_get_sampler(u32 texture_id) {
    if (!g_texture_manager.device || texture_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (texture_id > g_texture_manager.next_texture_id) {
        return VK_NULL_HANDLE;
    }
    
    return g_texture_manager.textures[texture_id - 1].sampler;
}

// Destroy texture
bool vk_texture_destroy(u32 texture_id) {
    if (!g_texture_manager.device || texture_id == 0) {
        return false;
    }
    
    if (texture_id > g_texture_manager.next_texture_id) {
        printf("Error: Invalid texture ID %u\n", texture_id);
        return false;
    }
    
    vk_texture_t* texture = &g_texture_manager.textures[texture_id - 1];
    
    if (!texture->is_allocated) {
        return false;
    }
    
    if (texture->image_view != VK_NULL_HANDLE) {
        vkDestroyImageView(g_texture_manager.device, texture->image_view, NULL);
    }
    
    if (texture->sampler != VK_NULL_HANDLE) {
        vkDestroySampler(g_texture_manager.device, texture->sampler, NULL);
    }
    
    if (texture->image != VK_NULL_HANDLE) {
        vkDestroyImage(g_texture_manager.device, texture->image, NULL);
    }
    
    if (texture->memory != VK_NULL_HANDLE) {
        vkFreeMemory(g_texture_manager.device, texture->memory, NULL);
    }
    
    memset(texture, 0, sizeof(vk_texture_t));
    texture->is_allocated = false;
    
    g_texture_manager.total_textures_destroyed++;
    
    printf("Destroyed texture '%s'\n", texture->name);
    return true;
}

// Get statistics
void vk_texture_get_stats(u64* total_memory_used, u32* total_textures_created, u32* total_textures_destroyed) {
    if (total_memory_used) *total_memory_used = g_texture_manager.total_memory_used;
    if (total_textures_created) *total_textures_created = g_texture_manager.total_textures_created;
    if (total_textures_destroyed) *total_textures_destroyed = g_texture_manager.total_textures_destroyed;
}