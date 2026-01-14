#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Vulkan swapchain management system
#define VK_MAX_SWAPCHAIN_IMAGES 8
#define VK_MAX_SWAPCHAIN_FORMATS 32

typedef struct vk_swapchain {
    VkSwapchainKHR handle;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR color_space;
    VkPresentModeKHR present_mode;
    u32 image_count;
    VkImage images[VK_MAX_SWAPCHAIN_IMAGES];
    VkImageView image_views[VK_MAX_SWAPCHAIN_IMAGES];
    VkSemaphore image_available_semaphores[VK_MAX_SWAPCHAIN_IMAGES];
    VkSemaphore render_finished_semaphores[VK_MAX_SWAPCHAIN_IMAGES];
    VkFence in_flight_fences[VK_MAX_SWAPCHAIN_IMAGES];
    
    char name[256];
    u32 id;
    bool is_created;
    u32 current_frame;
    bool vsync_enabled;
    bool triple_buffering;
} vk_swapchain_t;

typedef struct vk_swapchain_manager {
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkInstance instance;
    
    vk_swapchain_t swapchains[VK_MAX_SWAPCHAIN_IMAGES];
    u32 swapchain_count;
    u32 next_swapchain_id;
    
    // Supported formats and present modes
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR formats[VK_MAX_SWAPCHAIN_FORMATS];
    VkPresentModeKHR present_modes[VK_MAX_SWAPCHAIN_FORMATS];
    u32 format_count;
    u32 present_mode_count;
    
    // Statistics
    u32 total_swapchains_created;
    u32 total_swapchains_destroyed;
} vk_swapchain_manager_t;

static vk_swapchain_manager_t g_swapchain_manager = {0};

// Find supported surface format
static VkSurfaceFormatKHR find_supported_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkFormat preferred_formats[], u32 format_count) {
    for (u32 i = 0; i < format_count; i++) {
        VkSurfaceFormatKHR format = preferred_formats[i];
        
        VkBool supported = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, surface, format);
        if (supported) {
            return format;
        }
    }
    
    return VK_FORMAT_UNDEFINED;
}

// Find supported present mode
static VkPresentModeKHR find_supported_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkPresentModeKHR preferred_modes[], u32 mode_count) {
    for (u32 i = 0; i < mode_count; i++) {
        VkPresentModeKHR mode = preferred_modes[i];
        
        VkBool supported = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, surface, mode);
        if (supported) {
            return mode;
        }
    }
    
    return VK_PRESENT_MODE_MAX_ENUM_KHR;
}

// Initialize swapchain manager
bool vk_swapchain_manager_init(VkDevice device, VkPhysicalDevice physical_device, VkInstance instance) {
    if (!device || !physical_device || !instance) {
        printf("Error: Invalid parameters for swapchain manager initialization\n");
        return false;
    }
    
    g_swapchain_manager.device = device;
    g_swapchain_manager.physical_device = physical_device;
    g_swapchain_manager.instance = instance;
    
    printf("Vulkan swapchain manager initialized\n");
    return true;
}

// Cleanup swapchain manager
void vk_swapchain_manager_cleanup(void) {
    if (!g_swapchain_manager.device) {
        return;
    }
    
    // Destroy all swapchains
    for (u32 i = 0; i < g_swapchain_manager.swapchain_count; i++) {
        vk_swapchain_t* swapchain = &g_swapchain_manager.swapchains[i];
        
        if (swapchain->handle != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(g_swapchain_manager.device, swapchain->handle, NULL);
        }
        
        // Destroy image views
        for (u32 j = 0; j < swapchain->image_count; j++) {
            if (swapchain->image_views[j] != VK_NULL_HANDLE) {
                vkDestroyImageView(g_swapchain_manager.device, swapchain->image_views[j], NULL);
            }
        }
        
        // Destroy semaphores and fences
        for (u32 j = 0; j < swapchain->image_count; j++) {
            if (swapchain->image_available_semaphores[j] != VK_NULL_HANDLE) {
                vkDestroySemaphore(g_swapchain_manager.device, swapchain->image_available_semaphores[j], NULL);
            }
            
            if (swapchain->render_finished_semaphores[j] != VK_NULL_HANDLE) {
                vkDestroySemaphore(g_swapchain_manager.device, swapchain->render_finished_semaphores[j], NULL);
            }
            
            if (swapchain->in_flight_fences[j] != VK_NULL_HANDLE) {
                vkDestroyFence(g_swapchain_manager.device, swapchain->in_flight_fences[j], NULL);
            }
        }
    }
    
    memset(&g_swapchain_manager, 0, sizeof(g_swapchain_manager));
    
    printf("Vulkan swapchain manager cleaned up\n");
}

// Query surface capabilities
bool vk_swapchain_query_capabilities(VkSurfaceKHR surface) {
    if (!g_swapchain_manager.physical_device || !surface || !g_swapchain_manager.instance) {
        return false;
    }
    
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_swapchain_manager.physical_device, surface, &g_swapchain_manager.capabilities);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to get surface capabilities\n");
        return false;
    }
    
    // Get supported formats
    VkFormat preferred_formats[] = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_A2R10G10B10_UNORM,
        VK_FORMAT_A2R10G10B10_SRGB,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R16G16B16A16_SFLOAT
    };
    
    g_swapchain_manager.format_count = 0;
    for (u32 i = 0; i < sizeof(preferred_formats) / sizeof(preferred_formats[0]); i++) {
        VkBool supported = vkGetPhysicalDeviceSurfaceSupportKHR(g_swapchain_manager.physical_device, surface, preferred_formats[i]);
        if (supported) {
            g_swapchain_manager.formats[g_swapchain_manager.format_count++] = preferred_formats[i];
        }
    }
    
    // Get supported present modes
    VkPresentModeKHR preferred_modes[] = {
        VK_PRESENT_MODE_MAILBOX_KHR,
        VK_PRESENT_MODE_FIFO_KHR,
        VK_PRESENT_MODE_IMMEDIATE_KHR
    };
    
    g_swapchain_manager.present_mode_count = 0;
    for (u32 i = 0; i < sizeof(preferred_modes) / sizeof(preferred_modes[0]); i++) {
        VkBool supported = vkGetPhysicalDeviceSurfaceSupportKHR(g_swapchain_manager.physical_device, surface, preferred_modes[i]);
        if (supported) {
            g_swapchain_present_modes[g_swapchain_manager.present_mode_count++] = preferred_modes[i];
        }
    }
    
    printf("Surface capabilities: min_extent: %ux%u, max_extent: %ux%u\n",
           g_swapchain_manager.capabilities.minImageExtent.width, g_swapchain_manager.capabilities.minImageExtent.height,
           g_swapchain_manager.capabilities.maxImageExtent.width, g_swapchain_manager.capabilities.maxImageExtent.height);
    
    printf("Supported formats: %u\n", g_swapchain_manager.format_count);
    printf("Supported present modes: %u\n", g_swapchain_manager.present_mode_count);
    
    return true;
}

// Create swapchain
u32 vk_swapchain_create(const char* name, VkSurfaceKHR surface, u32 width, u32 height, bool vsync, bool triple_buffering) {
    if (!name || !g_swapchain_manager.device || !surface || width == 0 || height == 0) {
        return 0;
    }
    
    if (g_swapchain_manager.swapchain_count >= VK_MAX_SWAPCHAIN_IMAGES) {
        printf("Error: Maximum swapchains reached\n");
        return 0;
    }
    
    // Query surface capabilities if not already done
    if (g_swapchain_manager.format_count == 0) {
        if (!vk_swapchain_query_capabilities(surface)) {
            return 0;
        }
    }
    
    // Choose format and present mode
    VkFormat format = find_supported_format(g_swapchain_manager.physical_device, surface, g_swapchain.formats, g_swapchain.format_count);
    if (format == VK_FORMAT_UNDEFINED) {
        printf("Error: No supported surface format found\n");
        return 0;
    }
    
    VkPresentModeKHR present_mode = find_supported_present_mode(g_swapchain_manager.physical_device, surface, g_swapchain_present_mode_count, g_swapchain.present_mode_count);
    if (present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR) {
        printf("Error: No supported present mode found\n");
        return 0;
    }
    
    // Determine image count
    u32 image_count = triple_buffering ? 3 : (vsync ? 2 : 1);
    image_count = MIN(image_count, g_swapchain_manager.capabilities.maxImageCount);
    
    // Create swapchain
    VkSwapchainCreateInfo swapchain_info = {0};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = format;
    swapchain.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain.imageExtent.width = width;
    swapchain_info.imageExtent.height = height;
    swapchain_info.presentMode = present_mode;
    swapchain.clipped = VK_FALSE;
    swapchain.preTransform = VK_FALSE;
    swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE;
    swapchain.imageArrayLayers = 1;
    swapchain.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkSwapchainKHR swapchain;
    VkResult result = vkCreateSwapchainKHR(g_swapchain_manager.device, &swapchain_info, NULL, &swapchain);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create swapchain\n");
        return 0;
    }
    
    // Get swapchain images
    u32 actual_image_count;
    result = vkGetSwapchainImagesKHR(g_swapchain_manager.device, swapchain, &actual_image_count);
    if (result != VK_SUCCESS || actual_image_count == 0) {
        printf("Error: Failed to get swapchain images\n");
        vkDestroySwapchainKHR(g_swapchain_manager.device, swapchain, NULL);
        return 0;
    }
    
    // Create image views
    VkImageViewCreateInfo view_info = {0};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
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
    
    VkImageView image_views[VK_MAX_SWAPCHAIN_IMAGES];
    for (u32 i = 0; i < actual_image_count; i++) {
        view_info.image = swapchain_images[i];
        result = vkCreateImageView(g_swapchain_manager.device, &view_info, NULL, &image_views[i]);
        if (result != VK_SUCCESS) {
            printf("Error: Failed to create image view %u\n", i);
            continue;
        }
    }
    
    // Create semaphores
    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkSemaphore image_available_semaphores[VK_MAX_SWAPCHAIN_IMAGES];
    VkSemaphore render_finished_semaphores[VK_MAX_SWAPCHAIN_IMAGES];
    VkFence in_flight_fences[VK_MAX_SWAPCHAIN_IMAGES];
    
    for (u32 i = 0; i < actual_image_count; i++) {
        result = vkCreateSemaphore(g_swapchain_manager.device, &semaphore_info, NULL, &image_available_semaphores[i]);
        if (result != VK_SUCCESS) {
            printf("Error: Failed to create image available semaphore %u\n", i);
            continue;
        }
        
        result = vkCreateSemaphore(g_swapchain_manager.device, &semaphore_info, NULL, &render_finished_semaphores[i]);
        if (result != VK_SUCCESS) {
            printf("Error: Failed to create render finished semaphore %u\n", i);
            continue;
        }
        
        VkFenceCreateInfo fence_info = {0};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = 0;
        
        result = vkCreateFence(g_swapchain_manager.device, &fence_info, NULL, &in_flight_fences[i]);
        if (result != VK_SUCCESS) {
            printf("Error: Failed to create fence %u\n", i);
            continue;
        }
    }
    
    // Store swapchain
    u32 swapchain_id = g_swapchain_manager.next_swapchain_id++;
    vk_swapchain_t* swapchain = &g_swapchain_manager.swapchains[swapchain_id - 1];
    
    swapchain->handle = swapchain;
    swapchain->surface = surface;
    swapchain->extent.width = width;
    swapchain->extent.height = height;
    swapchain->format = format;
    swapchain->color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain->present_mode = present_mode;
    swapchain->image_count = actual_image_count;
    swapchain->current_frame = 0;
    swapchain->vsync_enabled = vsync;
    swapchain->triple_buffering = triple_buffering;
    
    // Copy images and image views
    for (u32 i = 0; i < actual_image_count; i++) {
        swapchain->images[i] = swapchain_images[i];
        swapchain->image_views[i] = image_views[i];
    }
    
    // Copy semaphores and fences
    for (u32 i = 0; i < actual_image_count; i++) {
        swapchain->image_available_semaphores[i] = image_available_semaphores[i];
        swapchain->render_finished_semaphores[i] = render_finished_semaphores[i];
        swapchain->in_flight_fences[i] = in_flight_fences[i];
    }
    
    strncpy(swapchain->name, name, 255);
    swapchain->name[255] = '\0';
    swapchain->id = swapchain_id;
    swapchain->is_created = true;
    
    g_swapchain_manager.swapchain_count++;
    g_swapchain_manager.total_swapchains_created++;
    
    printf("Created swapchain '%s' (%ux%u, format: %d, images: %u, vsync: %s, triple: %s)\n",
           name, width, height, format, actual_image_count, vsync ? "yes" : "no", triple_buffering ? "yes" : "no");
    
    return swapchain_id;
}

// Destroy swapchain
bool vk_swapchain_destroy(u32 swapchain_id) {
    if (!g_swapchain_manager.device || swapchain_id == 0) {
        return false;
    }
    
    if (swapchain_id > g_swapchain_manager.next_swapchain_id) {
        printf("Error: Invalid swapchain ID %u\n", swapchain_id);
        return false;
    }
    
    vk_swapchain_t* swapchain = &g_swapchain_manager.swapchains[swapchain_id - 1];
    
    if (!swapchain->is_created) {
        return false;
    }
    
    // Wait for device idle
    vkDeviceWaitIdle(g_swapchain_manager.device);
    
    // Destroy semaphores and fences
    for (u32 i = 0; i < swapchain->image_count; i++) {
        if (swapchain->image_available_semaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(g_swapchain_manager.device, swapchain->image_available_semaphores[i], NULL);
        }
        
        if (swapchain->render_finished_semaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(g_swapchain_manager.device, swapchain->render_finished_semaphores[i], NULL);
        }
        
        if (swapchain->in_flight_fences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(g_swapchain_manager.device, swapchain->in_flight_fences[i], NULL);
        }
    }
    
    // Destroy image views
    for (u32 i = 0; i < swapchain->image_count; i++) {
        if (swapchain->image_views[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(g_swapchain_manager.device, swapchain->image_views[i], NULL);
        }
    }
    
    // Destroy swapchain
    vkDestroySwapchainKHR(g_swapchain_manager.device, swapchain->handle, NULL);
    
    memset(swapchain, 0, sizeof(vk_swapchain_t));
    swapchain->is_created = false;
    
    g_swapchain_manager.total_swapchains_destroyed++;
    
    printf("Destroyed swapchain '%s'\n", swapchain->name);
    return true;
}

// Acquire next image index
u32 vk_swapchain_acquire_next_image(u32 swapchain_id, u32* image_index, VkSemaphore* image_available_semaphore, VkSemaphore* render_finished_semaphore, VkFence* fence) {
    if (!g_swapchain_manager.device || swapchain_id == 0 || !image_index) {
        return 0;
    }
    
    if (swapchain_id > g_swapchain_manager.next_swapchain_id) {
        printf("Error: Invalid swapchain ID %u\n", swapchain_id);
        return 0;
    }
    
    vk_swapchain_t* swapchain = &g_swapchain_swapchains[swapchain_id - 1];
    
    if (!swapchain->is_created || swapchain->image_count == 0) {
        return 0;
    }
    
    // Advance frame
    swapchain->current_frame = (swapchain->current_frame + 1) % swapchain->image_count;
    
    *image_index = swapchain->current_frame;
    *image_available_semaphore = swapchain->image_available_semaphores[*image_index];
    *render_finished_semaphore = swapchain->render_finished_semaphores[*image_index];
    *fence = swapchain->in_flight_fences[*image_index];
    
    return swapchain_id;
}

// Present image
bool vk_swapchain_present(u32 swapchain_id, u32 image_index) {
    if (!g_swapchain_manager.device || swapchain_id == 0) {
        return false;
    }
    
    if (swapchain_id > g_swapchain_manager.next_swapchain_id) {
        printf("Error: Invalid swapchain ID %u\n", swapchain_id);
        return false;
    }
    
    vk_swapchain_t* swapchain = &g_swapchain_swapchains[swapchain_id - 1];
    
    if (!swapchain->is_created) {
        return false;
    }
    
    // Present image
    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &swapchain->render_finished_semaphores[image_index];
    present_info.swapchainCount = 1;
    pPresentInfo.pSwapchains = &swapchain->handle;
    pPresentInfo.pImageIndices = &image_index;
    
    VkResult result = vkQueuePresentKHR(g_swapchain_manager.graphics_queue, &present_info);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        printf("Warning: Swapchain out of date, recreating may be needed\n");
        return false;
    } else if (result != VK_SUCCESS) {
        printf("Error: Failed to present image\n");
        return false;
    }
    
    return true;
}

// Get swapchain info
bool vk_swapchain_get_info(u32 swapchain_id, char* name, size_t name_size, u32* width, u32* height, u32* image_count) {
    if (!g_swapchain_manager.device || swapchain_id == 0) {
        return false;
    }
    
    if (swapchain_id > g_swapchain_manager.next_swapchain_id) {
        return false;
    }
    
    vk_swapchain_t* swapchain = &g_swapchain_swapchains[swapchain_id - 1];
    
    if (!swapchain->is_created) {
        return false;
    }
    
    if (name && name_size > 0) {
        strncpy(name, swapchain->name, name_size - 1);
        name[name_size - 1] = '\0';
    }
    
    if (width) *width = swapchain->extent.width;
    if (height) *height = swapchain->extent.height;
    if (image_count) *image_count = swapchain->image_count;
    
    return true;
}

// Get swapchain statistics
void vk_swapchain_get_stats(u32* total_swapchains_created, u32* total_swapchains_destroyed) {
    if (total_swapchains_created) *total_swapchains_created = g_swapchain_manager.total_swapchains_created;
    if (total_swapchains_destroyed) *total_swapchains_destroyed = g_swapchain_manager.total_swapchains_destroyed;
}
 * TODO: Implement vk swapchain validation
 * TODO: Add vk swapchain error handling
 * TODO: Implement vk swapchain serialization
 * TODO: Add vk swapchain debug output
 * TODO: Implement vk swapchain unit tests
 * TODO: Add vk swapchain performance counters
 * TODO: Implement vk swapchain hot-reload
 * TODO: Add vk swapchain thread safety
 * TODO: Implement vk swapchain memory pooling
 * TODO: Add vk swapchain caching layer
 * TODO: Implement vk swapchain async operations
 * TODO: Add vk swapchain GPU integration
 * TODO: Implement vk swapchain SIMD optimization
 * TODO: Add vk swapchain batch processing
 * TODO: Implement vk swapchain streaming support
 * TODO: Add vk swapchain LOD support
 * TODO: Implement vk swapchain culling integration
 * TODO: Add vk swapchain render graph node
 */

#include "backend/vulkan/vk_swapchain.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define PLATFORM_VK_SWAPCHAIN_MAX_COUNT 4096
#define PLATFORM_VK_SWAPCHAIN_DEFAULT_CAPACITY 256
#define PLATFORM_VK_SWAPCHAIN_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct platform_vk_swapchain_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} platform_vk_swapchain_internal_t;

typedef struct platform_vk_swapchain_context {
    platform_vk_swapchain_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} platform_vk_swapchain_context_t;

static platform_vk_swapchain_context_t g_vk_swapchain_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool platform_vk_swapchain_validate(const platform_vk_swapchain_internal_t* item) {
    // TODO: Implement Vulkan backend
    // TODO: Implement Metal backend
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void platform_vk_swapchain_cleanup_internal(platform_vk_swapchain_internal_t* item) {
    // TODO: Implement D3D12 backend
    // TODO: Add thread-safe access patterns
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int platform_vk_swapchain_init(void) {
    // TODO: Implement proper error handling with error codes
    // TODO: Add memory tracking and leak detection
    // TODO: Implement hot-reload support
    // TODO: Add validation layer integration

    if (g_vk_swapchain_ctx.initialized) {
        return 0; // Already initialized
    }

    g_vk_swapchain_ctx.capacity = PLATFORM_VK_SWAPCHAIN_DEFAULT_CAPACITY;
    g_vk_swapchain_ctx.items = calloc(g_vk_swapchain_ctx.capacity, sizeof(platform_vk_swapchain_internal_t));
    if (!g_vk_swapchain_ctx.items) {
        return -1;
    }

    g_vk_swapchain_ctx.count = 0;
    g_vk_swapchain_ctx.initialized = true;

    return 0;
}

void platform_vk_swapchain_shutdown(void) {
    // TODO: Implement resource state tracking
    // TODO: Add GPU debugging markers
    // TODO: Implement vk swapchain initialization
    // TODO: Add vk swapchain cleanup/shutdown

    if (!g_vk_swapchain_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_vk_swapchain_ctx.count; i++) {
        platform_vk_swapchain_cleanup_internal(&g_vk_swapchain_ctx.items[i]);
    }

    free(g_vk_swapchain_ctx.items);
    g_vk_swapchain_ctx.items = NULL;
    g_vk_swapchain_ctx.count = 0;
    g_vk_swapchain_ctx.capacity = 0;
    g_vk_swapchain_ctx.initialized = false;
}

int platform_vk_swapchain_create(platform_vk_swapchain_handle_t* out_handle, const platform_vk_swapchain_desc_t* desc) {
    // TODO: Implement vk swapchain validation
    // TODO: Add vk swapchain error handling
    // TODO: Implement vk swapchain serialization
    // TODO: Add vk swapchain debug output

    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_vk_swapchain_ctx.initialized) {
        return -2;
    }

    if (g_vk_swapchain_ctx.count >= g_vk_swapchain_ctx.capacity) {
        // TODO: Implement vk swapchain unit tests
        return -3;
    }

    uint32_t index = g_vk_swapchain_ctx.count++;
    platform_vk_swapchain_internal_t* item = &g_vk_swapchain_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void platform_vk_swapchain_destroy(platform_vk_swapchain_handle_t handle) {
    // TODO: Add vk swapchain performance counters
    // TODO: Implement vk swapchain hot-reload

    if (handle.id >= g_vk_swapchain_ctx.count) {
        return;
    }

    platform_vk_swapchain_cleanup_internal(&g_vk_swapchain_ctx.items[handle.id]);
}

int platform_vk_swapchain_update(platform_vk_swapchain_handle_t handle, const void* data, size_t size) {
    // TODO: Add vk swapchain thread safety
    // TODO: Implement vk swapchain memory pooling
    // TODO: Add vk swapchain caching layer
    // TODO: Implement vk swapchain async operations

    if (handle.id >= g_vk_swapchain_ctx.count) {
        return -1;
    }

    platform_vk_swapchain_internal_t* item = &g_vk_swapchain_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // TODO: Add vk swapchain GPU integration
    // TODO: Implement vk swapchain SIMD optimization

    item->dirty = true;
    return 0;
}

bool platform_vk_swapchain_is_valid(platform_vk_swapchain_handle_t handle) {
    // TODO: Add vk swapchain batch processing
    if (handle.id >= g_vk_swapchain_ctx.count) {
        return false;
    }
    return g_vk_swapchain_ctx.items[handle.id].initialized;
}

int platform_vk_swapchain_get_info(platform_vk_swapchain_handle_t handle, platform_vk_swapchain_info_t* out_info) {
    // TODO: Implement vk swapchain streaming support
    // TODO: Add vk swapchain LOD support

    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_vk_swapchain_ctx.count) {
        return -2;
    }

    const platform_vk_swapchain_internal_t* item = &g_vk_swapchain_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void platform_vk_swapchain_mark_dirty(platform_vk_swapchain_handle_t handle) {
    // TODO: Implement vk swapchain culling integration
    if (handle.id < g_vk_swapchain_ctx.count) {
        g_vk_swapchain_ctx.items[handle.id].dirty = true;
    }
}

int platform_vk_swapchain_process_pending(void) {
    // TODO: Add vk swapchain render graph node
    // TODO: Implement batch processing

    int processed = 0;
    for (uint32_t i = 0; i < g_vk_swapchain_ctx.count; i++) {
        platform_vk_swapchain_internal_t* item = &g_vk_swapchain_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t platform_vk_swapchain_get_count(void) {
    return g_vk_swapchain_ctx.count;
}

size_t platform_vk_swapchain_get_memory_usage(void) {
    // TODO: Implement memory tracking
    size_t total = sizeof(g_vk_swapchain_ctx);
    total += g_vk_swapchain_ctx.capacity * sizeof(platform_vk_swapchain_internal_t);

    for (uint32_t i = 0; i < g_vk_swapchain_ctx.count; i++) {
        total += g_vk_swapchain_ctx.items[i].data_size;
    }

    return total;
}

void platform_vk_swapchain_debug_print(void) {
    // TODO: Implement debug output
    // Debug printing implementation
}

/* End of vk_swapchain.c */
