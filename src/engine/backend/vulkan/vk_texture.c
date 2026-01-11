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

typedef enum {
    VK_TEXTURE_FORMAT_UNDEFINED = 0,
    VK_TEXTURE_FORMAT_R8_UNORM,
    VK_TEXTURE_FORMAT_R8_SNORM,
    VK_TEXTURE_FORMAT_R8_UINT,
    VK_TEXTURE_FORMAT_R8_SINT,
    VK_TEXTURE_FORMAT_R16_UNORM,
    VK_TEXTURE_FORMAT_R16_SNORM,
    VK_TEXTURE_FORMAT_R16_UINT,
    VK_TEXTURE_FORMAT_R16_SINT,
    VK_TEXTURE_FORMAT_R16_SFLOAT,
    VK_TEXTURE_FORMAT_R32_UINT,
    VK_TEXTURE_FORMAT_R32_SINT,
    VK_TEXTURE_FORMAT_R32_SFLOAT,
    VK_TEXTURE_FORMAT_RG8_UNORM,
    VK_TEXTURE_FORMAT_RG8_SNORM,
    VK_TEXTURE_FORMAT_RG8_UINT,
    VK_TEXTURE_FORMAT_RG8_SINT,
    VK_TEXTURE_FORMAT_RG16_UNORM,
    VK_TEXTURE_FORMAT_RG16_SNORM,
    VK_TEXTURE_FORMAT_RG16_UINT,
    VK_TEXTURE_FORMAT_RG16_SINT,
    VK_TEXTURE_FORMAT_RG16_SFLOAT,
    VK_TEXTURE_FORMAT_RG32_UINT,
    VK_TEXTURE_FORMAT_RG32_SINT,
    VK_TEXTURE_FORMAT_RG32_SFLOAT,
    VK_TEXTURE_FORMAT_RGB8_UNORM,
    VK_TEXTURE_FORMAT_RGB8_SNORM,
    VK_TEXTURE_FORMAT_RGB8_UINT,
    VK_TEXTURE_FORMAT_RGB8_SINT,
    VK_TEXTURE_FORMAT_RGB16_UNORM,
    VK_TEXTURE_FORMAT_RGB16_SNORM,
    VK_TEXTURE_FORMAT_RGB16_UINT,
    VK_TEXTURE_FORMAT_RGB16_SINT,
    VK_TEXTURE_FORMAT_RGB16_SFLOAT,
    VK_TEXTURE_FORMAT_RGB32_UINT,
    VK_TEXTURE_FORMAT_RGB32_SINT,
    VK_TEXTURE_FORMAT_RGB32_SFLOAT,
    VK_TEXTURE_FORMAT_RGBA8_UNORM,
    VK_TEXTURE_FORMAT_RGBA8_SNORM,
    VK_TEXTURE_FORMAT_RGBA8_UINT,
    VK_TEXTURE_FORMAT_RGBA8_SINT,
    VK_TEXTURE_FORMAT_RGBA16_UNORM,
    VK_TEXTURE_FORMAT_RGBA16_SNORM,
    VK_TEXTURE_FORMAT_RGBA16_UINT,
    VK_TEXTURE_FORMAT_RGBA16_SINT,
    VK_TEXTURE_FORMAT_RGBA16_SFLOAT,
    VK_TEXTURE_FORMAT_RGBA32_UINT,
    VK_TEXTURE_FORMAT_RGBA32_SINT,
    VK_TEXTURE_FORMAT_RGBA32_SFLOAT,
    VK_TEXTURE_FORMAT_COUNT
} vk_texture_format_t;

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

// Convert our texture format to Vulkan format
static VkFormat convert_texture_format(vk_texture_format_t format) {
    switch (format) {
        case VK_TEXTURE_FORMAT_R8_UNORM: return VK_FORMAT_R8_UNORM;
        case VK_TEXTURE_FORMAT_R8_SNORM: return VK_FORMAT_R8_SNORM;
        case VK_TEXTURE_FORMAT_R8_UINT: return VK_FORMAT_R8_UINT;
        case VK_TEXTURE_FORMAT_R8_SINT: return VK_FORMAT_R8_SINT;
        case VK_TEXTURE_FORMAT_R16_UNORM: return VK_FORMAT_R16_UNORM;
        case VK_TEXTURE_FORMAT_R16_SNORM: return VK_FORMAT_R16_SNORM;
        case VK_TEXTURE_FORMAT_R16_UINT: return VK_FORMAT_R16_UINT;
        case VK_TEXTURE_FORMAT_R16_SINT: return VK_FORMAT_R16_SINT;
        case VK_TEXTURE_FORMAT_R16_SFLOAT: return VK_FORMAT_R16_SFLOAT;
        case VK_TEXTURE_FORMAT_R32_UINT: return VK_FORMAT_R32_UINT;
        case VK_TEXTURE_FORMAT_R32_SINT: return VK_FORMAT_R32_SINT;
        case VK_TEXTURE_FORMAT_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
        case VK_TEXTURE_FORMAT_RG8_UNORM: return VK_FORMAT_R8G8_UNORM;
        case VK_TEXTURE_FORMAT_RG8_SNORM: return VK_FORMAT_R8G8_SNORM;
        case VK_TEXTURE_FORMAT_RG8_UINT: return VK_FORMAT_R8G8_UINT;
        case VK_TEXTURE_FORMAT_RG8_SINT: return VK_FORMAT_R8G8_SINT;
        case VK_TEXTURE_FORMAT_RG16_UNORM: return VK_FORMAT_R16G16_UNORM;
        case VK_TEXTURE_FORMAT_RG16_SNORM: return VK_FORMAT_R16G16_SNORM;
        case VK_TEXTURE_FORMAT_RG16_UINT: return VK_FORMAT_R16G16_UINT;
        case VK_TEXTURE_FORMAT_RG16_SINT: return VK_FORMAT_R16G16_SINT;
        case VK_TEXTURE_FORMAT_RG16_SFLOAT: return VK_FORMAT_R16G16_SFLOAT;
        case VK_TEXTURE_FORMAT_RG32_UINT: return VK_FORMAT_R32G32_UINT;
        case VK_TEXTURE_FORMAT_RG32_SINT: return VK_FORMAT_R32G32_SINT;
        case VK_TEXTURE_FORMAT_RG32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
        case VK_TEXTURE_FORMAT_RGB8_UNORM: return VK_FORMAT_R8G8B8_UNORM;
        case VK_TEXTURE_FORMAT_RGB8_SNORM: return VK_FORMAT_R8G8B8_SNORM;
        case VK_TEXTURE_FORMAT_RGB8_UINT: return VK_FORMAT_R8G8B8_UINT;
        case VK_TEXTURE_FORMAT_RGB8_SINT: return VK_FORMAT_R8G8B8_SINT;
        case VK_TEXTURE_FORMAT_RGB16_UNORM: return VK_FORMAT_R16G16B16_UNORM;
        case VK_TEXTURE_FORMAT_RGB16_SNORM: return VK_FORMAT_R16G16B16_SNORM;
        case VK_TEXTURE_FORMAT_RGB16_UINT: return VK_FORMAT_R16G16B16_UINT;
        case VK_TEXTURE_FORMAT_RGB16_SINT: return VK_FORMAT_R16G16B16_SINT;
        case VK_TEXTURE_FORMAT_RGB16_SFLOAT: return VK_FORMAT_R16G16B16_SFLOAT;
        case VK_TEXTURE_FORMAT_RGB32_UINT: return VK_FORMAT_R32G32B32_UINT;
        case VK_TEXTURE_FORMAT_RGB32_SINT: return VK_FORMAT_R32G32B32_SINT;
        case VK_TEXTURE_FORMAT_RGB32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
        case VK_TEXTURE_FORMAT_RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case VK_TEXTURE_FORMAT_RGBA8_SNORM: return VK_FORMAT_R8G8B8A8_SNORM;
        case VK_TEXTURE_FORMAT_RGBA8_UINT: return VK_FORMAT_R8G8B8A8_UINT;
        case VK_TEXTURE_FORMAT_RGBA8_SINT: return VK_FORMAT_R8G8B8A8_SINT;
        case VK_TEXTURE_FORMAT_RGBA16_UNORM: return VK_FORMAT_R16G16B16A16_UNORM;
        case VK_TEXTURE_FORMAT_RGBA16_SNORM: return VK_FORMAT_R16G16B16A16_SNORM;
        case VK_TEXTURE_FORMAT_RGBA16_UINT: return VK_FORMAT_R16G16B16A16_UINT;
        case VK_TEXTURE_FORMAT_RGBA16_SINT: return VK_FORMAT_R16G16B16A16_SINT;
        case VK_TEXTURE_FORMAT_RGBA16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case VK_TEXTURE_FORMAT_RGBA32_UINT: return VK_FORMAT_R32G32B32A32_UINT;
        case VK_TEXTURE_FORMAT_RGBA32_SINT: return VK_FORMAT_R32G32B32A32_SINT;
        case VK_TEXTURE_FORMAT_RGBA32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        default: return VK_FORMAT_UNDEFINED;
    }
}

// Convert our texture type to Vulkan image type
static VkImageType convert_texture_type(vk_texture_type_t type) {
    switch (type) {
        case VK_TEXTURE_TYPE_1D:
        case VK_TEXTURE_TYPE_1D_ARRAY:
            return VK_IMAGE_TYPE_1D;
        case VK_TEXTURE_TYPE_2D:
        case VK_TEXTURE_TYPE_2D_ARRAY:
            return VK_IMAGE_TYPE_2D;
        case VK_TEXTURE_TYPE_3D:
            return VK_IMAGE_TYPE_3D;
        case VK_TEXTURE_TYPE_CUBE:
        case VK_TEXTURE_TYPE_CUBE_ARRAY:
            return VK_IMAGE_TYPE_2D;
        default:
            return VK_IMAGE_TYPE_2D;
    }
}

// Get image usage flags for texture type
static VkImageUsageFlags get_image_usage_flags(vk_texture_type_t type, bool is_sampled, bool is_mipmapped) {
    VkImageUsageFlags flags = 0;
    
    if (is_sampled) {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    
    if (is_mipmapped) {
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    
    flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // For texture uploads
    
    return flags;
}

// Create image view
static VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format, vk_texture_type_t type, u32 mip_levels, u32 array_layers) {
    VkImageViewCreateInfo view_info = {0};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.format = format;
    
    switch (type) {
        case VK_TEXTURE_TYPE_1D:
            view_info.viewType = VK_IMAGE_VIEW_TYPE_1D;
            break;
        case VK_TEXTURE_TYPE_2D:
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case VK_TEXTURE_TYPE_3D:
            view_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        case VK_TEXTURE_TYPE_CUBE:
            view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            array_layers = 6;
            break;
        case VK_TEXTURE_TYPE_1D_ARRAY:
            view_info.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            break;
        case VK_TEXTURE_TYPE_2D_ARRAY:
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        case VK_TEXTURE_TYPE_CUBE_ARRAY:
            view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            array_layers *= 6;
            break;
        default:
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            break;
    }
    
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = array_layers;
    
    VkImageView image_view;
    VkResult result = vkCreateImageView(device, &view_info, NULL, &image_view);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create image view\n");
        return VK_NULL_HANDLE;
    }
    
    return image_view;
}

// Create sampler
static VkSampler create_sampler(VkDevice device, bool is_mipmapped) {
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
    VkResult result = vkCreateSampler(device, &sampler_info, NULL, &sampler);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create sampler\n");
        return VK_NULL_HANDLE;
    }
    
    return sampler;
}

// Find memory type for image
static u32 find_memory_type_for_image(VkPhysicalDevice physical_device, u32 type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
    
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    return UINT32_MAX;
}

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

// Create texture
u32 vk_texture_create_2d(const char* name, u32 width, u32 height, vk_texture_format_t format, bool is_mipmapped, bool is_sampled) {
    if (!name || !g_texture_manager.device || width == 0 || height == 0) {
        return 0;
    }
    
    if (g_texture_manager.texture_count >= VK_MAX_TEXTURES) {
        printf("Error: Maximum textures reached\n");
        return 0;
    }
    
    VkFormat vk_format = convert_texture_format(format);
    if (vk_format == VK_FORMAT_UNDEFINED) {
        printf("Error: Unsupported texture format\n");
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
    image_info.format = vk_format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = get_image_usage_flags(VK_TEXTURE_TYPE_2D, is_sampled, is_mipmapped);
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
    
    u32 memory_type = find_memory_type_for_image(g_texture_manager.physical_device, 
                                                 mem_requirements.memoryTypeBits, 
                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (memory_type == UINT32_MAX) {
        printf("Error: No suitable memory type found for image\n");
        vkDestroyImage(g_texture_manager.device, image, NULL);
        return 0;
    }
    
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memory_type;
    
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
    VkImageView image_view = create_image_view(g_texture_manager.device, image, vk_format, VK_TEXTURE_TYPE_2D, mip_levels, 1);
    if (image_view == VK_NULL_HANDLE) {
        printf("Error: Failed to create image view\n");
        vkDestroyImage(g_texture_manager.device, image, NULL);
        vkFreeMemory(g_texture_manager.device, memory, NULL);
        return 0;
    }
    
    // Create sampler
    VkSampler sampler = create_sampler(g_texture_manager.device, is_mipmapped);
    if (sampler == VK_NULL_HANDLE) {
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
    texture->format = vk_format;
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
    
    printf("Created 2D texture '%s' (%ux%u, format: %d, mips: %u)\n", 
           name, width, height, format, mip_levels);
    
    return texture_id;
}

// Create cube texture
u32 vk_texture_create_cube(const char* name, u32 size, vk_texture_format_t format, bool is_mipmapped, bool is_sampled) {
    if (!name || !g_texture_manager.device || size == 0) {
        return 0;
    }
    
    if (g_texture_manager.texture_count >= VK_MAX_TEXTURES) {
        printf("Error: Maximum textures reached\n");
        return 0;
    }
    
    VkFormat vk_format = convert_texture_format(format);
    if (vk_format == VK_FORMAT_UNDEFINED) {
        printf("Error: Unsupported texture format\n");
        return 0;
    }
    
    // Calculate mip levels
    u32 mip_levels = 1;
    if (is_mipmapped) {
        mip_levels = (u32)floor(log2(size)) + 1;
        mip_levels = MIN(mip_levels, VK_MAX_MIP_LEVELS);
    }
    
    // Create image
    VkImageCreateInfo image_info = {0};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = size;
    image_info.extent.height = size;
    image_info.extent.depth = 1;
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = 6;
    image_info.format = vk_format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = get_image_usage_flags(VK_TEXTURE_TYPE_CUBE, is_sampled, is_mipmapped);
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    VkImage image;
    VkResult result = vkCreateImage(g_texture_manager.device, &image_info, NULL, &image);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create cube image\n");
        return 0;
    }
    
    // Allocate memory (similar to 2D texture)
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(g_texture_manager.device, image, &mem_requirements);
    
    u32 memory_type = find_memory_type_for_image(g_texture_manager.physical_device, 
                                                 mem_requirements.memoryTypeBits, 
                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (memory_type == UINT32_MAX) {
        printf("Error: No suitable memory type found for cube image\n");
        vkDestroyImage(g_texture_manager.device, image, NULL);
        return 0;
    }
    
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memory_type;
    
    VkDeviceMemory memory;
    result = vkAllocateMemory(g_texture_manager.device, &alloc_info, NULL, &memory);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to allocate cube image memory\n");
        vkDestroyImage(g_texture_manager.device, image, NULL);
        return 0;
    }
    
    // Bind memory
    result = vkBindImageMemory(g_texture_manager.device, image, memory, 0);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to bind cube image memory\n");
        vkDestroyImage(g_texture_manager.device, image, NULL);
        vkFreeMemory(g_texture_manager.device, memory, NULL);
        return 0;
    }
    
    // Create image view
    VkImageView image_view = create_image_view(g_texture_manager.device, image, vk_format, VK_TEXTURE_TYPE_CUBE, mip_levels, 6);
    if (image_view == VK_NULL_HANDLE) {
        printf("Error: Failed to create cube image view\n");
        vkDestroyImage(g_texture_manager.device, image, NULL);
        vkFreeMemory(g_texture_manager.device, memory, NULL);
        return 0;
    }
    
    // Create sampler
    VkSampler sampler = create_sampler(g_texture_manager.device, is_mipmapped);
    if (sampler == VK_NULL_HANDLE) {
        printf("Error: Failed to create cube sampler\n");
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
    texture->type = VK_TEXTURE_TYPE_CUBE;
    texture->format = vk_format;
    texture->width = size;
    texture->height = size;
    texture->depth = 1;
    texture->mip_levels = mip_levels;
    texture->array_layers = 6;
    strncpy(texture->name, name, 255);
    texture->name[255] = '\0';
    texture->id = texture_id;
    texture->is_allocated = true;
    texture->is_mipmapped = is_mipmapped;
    texture->is_sampled = is_sampled;
    
    g_texture_manager.texture_count++;
    g_texture_manager.total_textures_created++;
    g_texture_manager.total_memory_used += mem_requirements.size;
    
    printf("Created cube texture '%s' (%ux%u, format: %d, mips: %u)\n", 
           name, size, size, format, mip_levels);
    
    return texture_id;
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

// Get texture info
bool vk_texture_get_info(u32 texture_id, char* name, size_t name_size, u32* width, u32* height, u32* mip_levels) {
    if (!g_texture_manager.device || texture_id == 0) {
        return false;
    }
    
    if (texture_id > g_texture_manager.next_texture_id) {
        return false;
    }
    
    vk_texture_t* texture = &g_texture_manager.textures[texture_id - 1];
    
    if (!texture->is_allocated) {
        return false;
    }
    
    if (name && name_size > 0) {
        strncpy(name, texture->name, name_size - 1);
        name[name_size - 1] = '\0';
    }
    
    if (width) *width = texture->width;
    if (height) *height = texture->height;
    if (mip_levels) *mip_levels = texture->mip_levels;
    
    return true;
}

// Get statistics
void vk_texture_get_stats(u64* total_memory_used, u32* total_textures_created, u32* total_textures_destroyed) {
    if (total_memory_used) *total_memory_used = g_texture_manager.total_memory_used;
    if (total_textures_created) *total_textures_created = g_texture_manager.total_textures_created;
    if (total_textures_destroyed) *total_textures_destroyed = g_texture_manager.total_textures_destroyed;
}
