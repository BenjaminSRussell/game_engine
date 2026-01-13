#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Vulkan framebuffer management system
#define VK_MAX_FRAMEBUFFERS 16
#define VK_MAX_ATTACHMENTS 8

typedef enum {
    VK_ATTACHMENT_COLOR = 0,
    VK_ATTACHMENT_DEPTH,
    VK_ATTACHMENT_STENCIL,
    VK_ATTACHMENT_COUNT
} vk_attachment_type_t;

typedef struct vk_attachment {
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;
    VkFormat format;
    u32 width;
    u32 height;
    vk_attachment_type_t type;
    VkImageLayout current_layout;
    char name[256];
    bool is_valid;
} vk_attachment_t;

typedef struct vk_framebuffer {
    VkFramebuffer handle;
    VkRenderPass render_pass;
    
    vk_attachment_t attachments[VK_MAX_ATTACHMENTS];
    u32 attachment_count;
    u32 width;
    u32 height;
    
    char name[256];
    bool is_valid;
} vk_framebuffer_t;

typedef struct vk_framebuffer_manager {
    VkDevice device;
    
    vk_framebuffer_t framebuffers[VK_MAX_FRAMEBUFFERS];
    u32 framebuffer_count;
    u32 next_framebuffer_id;
    
    // Statistics
    u32 total_framebuffers_created;
    u32 total_framebuffers_destroyed;
    u64 total_memory_used;
} vk_framebuffer_manager_t;

static vk_framebuffer_manager_t g_framebuffer_manager = {0};

// Find memory type for image
static u32 find_memory_type(VkPhysicalDevice physical_device, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
    
    for (u32 i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    return UINT32_MAX;
}

// Create image for attachment
static VkImage create_attachment_image(VkDevice device, u32 width, u32 height, VkFormat format, VkImageUsageFlags usage) {
    VkImageCreateInfo image_info = {0};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkImage image;
    VkResult result = vkCreateImage(device, &image_info, NULL, &image);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create image\n");
        return VK_NULL_HANDLE;
    }
    
    return image;
}

// Allocate memory for image
static VkDeviceMemory allocate_image_memory(VkDevice device, VkPhysicalDevice physical_device, VkImage image, VkMemoryPropertyFlags properties) {
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device, image, &mem_requirements);
    
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(physical_device, properties);
    
    VkDeviceMemory memory;
    VkResult result = vkAllocateMemory(device, &alloc_info, NULL, &memory);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to allocate image memory\n");
        return VK_NULL_HANDLE;
    }
    
    return memory;
}

// Create image view for attachment
static VkImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageViewType view_type, u32 layer_count) {
    VkImageViewCreateInfo view_info = {0};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = view_type;
    view_info.format = format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = layer_count;
    
    VkImageView image_view;
    VkResult result = vkCreateImageView(device, &view_info, NULL, &image_view);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create image view\n");
        return VK_NULL_HANDLE;
    }
    
    return image_view;
}

// Initialize framebuffer manager
bool vk_framebuffer_manager_init(VkDevice device, VkPhysicalDevice physical_device) {
    if (!device || !physical_device) {
        printf("Error: Invalid device for framebuffer manager initialization\n");
        return false;
    }
    
    g_framebuffer_manager.device = device;
    
    printf("Vulkan framebuffer manager initialized\n");
    return true;
}

// Cleanup framebuffer manager
void vk_framebuffer_manager_cleanup(void) {
    if (!g_framebuffer_manager.device) {
        return;
    }
    
    // Destroy all framebuffers
    for (u32 i = 0; i < g_framebuffer_manager.framebuffer_count; i++) {
        vk_framebuffer_destroy(g_framebuffer_manager.framebuffers[i].id);
    }
    
    memset(&g_framebuffer_manager, 0, sizeof(g_framebuffer_manager));
    
    printf("Vulkan framebuffer manager cleaned up\n");
}

// Create framebuffer
u32 vk_framebuffer_create(const char* name, u32 width, u32 height) {
    if (!name || !g_framebuffer_manager.device || width == 0 || height == 0) {
        return 0;
    }
    
    if (g_framebuffer_manager.framebuffer_count >= VK_MAX_FRAMEBUFFERS) {
        printf("Error: Maximum framebuffers reached\n");
        return 0;
    }
    
    // Find free framebuffer slot
    u32 framebuffer_id = g_framebuffer_manager.next_framebuffer_id++;
    if (framebuffer_id >= VK_MAX_FRAMEBUFFERS) {
        printf("Error: No free framebuffer slots available\n");
        return 0;
    }
    
    vk_framebuffer_t* framebuffer = &g_framebuffer_manager.framebuffers[framebuffer_id - 1];
    
    // Create render pass
    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2; // Color + Depth
    
    VkAttachmentDescription attachments[2];
    
    // Color attachment
    attachments[0].format = VK_FORMAT_B8G8R8A8_SRGB;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    // Depth attachment
    attachments[1].format = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    render_pass_info.pAttachments = attachments;
    
    // Create subpass
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments[0].attachment = 0;
    subpass.pColorAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    subpass.pDepthStencilAttachment.attachment = 1;
    subpass.pDepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    
    VkRenderPass render_pass;
    VkResult result = vkCreateRenderPass(g_framebuffer_manager.device, &render_pass_info, NULL, &render_pass);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create render pass\n");
        return 0;
    }
    
    // Create color attachment
    VkImage color_image = create_attachment_image(g_framebuffer_manager.device, width, height, 
                                                    VK_FORMAT_B8G8R8A8_SRGB, 
                                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    if (color_image == VK_NULL_HANDLE) {
        printf("Error: Failed to create color image\n");
        vkDestroyRenderPass(g_framebuffer_manager.device, render_pass, NULL);
        return 0;
    }
    
    VkDeviceMemory color_memory = allocate_image_memory(g_framebuffer_device, 
                                                     g_framebuffer_manager.physical_device, 
                                                     color_image, 
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (color_memory == VK_NULL_HANDLE) {
        printf("Error: Failed to allocate color memory\n");
        vkDestroyImage(g_framebuffer_manager.device, color_image, NULL);
        vkDestroyRenderPass(g_framebuffer_manager.device, render_pass, NULL);
        return 0;
    }
    
    vkBindImageMemory(g_framebuffer_device, color_image, color_memory, 0, NULL);
    
    VkImageView color_view = create_image_view(g_framebuffer_device, color_image, 
                                                   VK_FORMAT_B8G8R8A8_SRGB, 
                                                   VK_IMAGE_VIEW_TYPE_2D, 1);
    if (color_view == VK_NULL_HANDLE) {
        printf("Error: Failed to create color image view\n");
        vkDestroyImage(g_framebuffer_device, color_image, NULL);
        vkFreeMemory(g_framebuffer_device, color_memory, NULL);
        vkDestroyRenderPass(g_framebuffer_device, render_pass, NULL);
        return 0;
    }
    
    // Create depth attachment
    VkImage depth_image = create_attachment_image(g_framebuffer_manager.device, width, height, 
                                                    VK_FORMAT_D32_SFLOAT, 
                                                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    if (depth_image == VK_NULL_HANDLE) {
        printf("Error: Failed to create depth image\n");
        vkDestroyImage(g_framebuffer_device, color_image, NULL);
        vkFreeMemory(g_framebuffer_device, color_memory, NULL);
        vkDestroyImageView(g_framebuffer_device, color_view, NULL);
        vkDestroyRenderPass(g_framebuffer_device, render_pass, NULL);
        return 0;
    }
    
    VkDeviceMemory depth_memory = allocate_image_memory(g_framebuffer_device, 
                                                     g_framebuffer_manager.physical_device, 
                                                     depth_image, 
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (depth_memory == VK_NULL_HANDLE) {
        printf("Error: Failed to allocate depth memory\n");
        vkDestroyImage(g_framebuffer_device, color_image, NULL);
        vkFreeMemory(g_framebuffer_device, color_memory, NULL);
        vkDestroyImageView(g_framebuffer_device, color_view, NULL);
        vkDestroyRenderPass(g_framebuffer_device, render_pass, NULL);
        return 0;
    }
    
    vkBindImageMemory(g_framebuffer_device, depth_image, depth_memory, 0, NULL);
    
    VkImageView depth_view = create_image_view(g_framebuffer_device, depth_image, 
                                                   VK_FORMAT_D32_SFLOAT, 
                                                   VK_IMAGE_VIEW_TYPE_2D, 1);
    if (depth_view == VK_NULL_HANDLE) {
        printf("Error: Failed to create depth image view\n");
        vkDestroyImage(g_framebuffer_device, color_image, NULL);
        vkFreeMemory(g_framebuffer_device, color_memory, NULL);
        vkDestroyImageView(g_framebuffer_device, color_view, NULL);
        vkDestroyRenderPass(g_framebuffer_device, render_pass, NULL);
        return 0;
    }
    
    // Create framebuffer
    VkImageView attachments_views[2] = {color_view, depth_view};
    
    VkFramebufferCreateInfo framebuffer_info = {0};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass;
    framebuffer_info.attachmentCount = 2;
    framebuffer_info.pAttachments = attachments_views;
    framebuffer_info.width = width;
    framebuffer_info.height = height;
    
    VkFramebuffer framebuffer;
    result = vkCreateFramebuffer(g_framebuffer_manager.device, &framebuffer_info, NULL, &framebuffer);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create framebuffer\n");
        
        // Cleanup resources
        vkDestroyImageView(g_framebuffer_device, color_view, NULL);
        vkDestroyImageView(g_framebuffer_device, depth_view, NULL);
        vkDestroyImage(g_framebuffer_device, color_image, NULL);
        vkDestroyImage(g_framebuffer_device, depth_image, NULL);
        vkFreeMemory(g_framebuffer_device, color_memory, NULL);
        vkFreeMemory(g_framebuffer_device, depth_memory, NULL);
        vkDestroyRenderPass(g_framebuffer_device, render_pass, NULL);
        return 0;
    }
    
    // Store framebuffer
    framebuffer->handle = framebuffer;
    framebuffer->render_pass = render_pass;
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->attachment_count = 2;
    
    // Store attachments
    framebuffer->attachments[0].image = color_image;
    framebuffer->attachments[0].image_view = color_view;
    framebuffer->attachments[0].memory = color_memory;
    framebuffer->attachments[0].format = VK_FORMAT_B8G8R8A8_SRGB;
    framebuffer->attachments[0].width = width;
    framebuffer->attachments[0].height = height;
    framebuffer->attachments[0].type = VK_ATTACHMENT_COLOR;
    framebuffer->attachments[0].current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    framebuffer->attachments[0].is_valid = true;
    
    framebuffer->attachments[1].image = depth_image;
    framebuffer->attachments[1].image_view = depth_view;
    framebuffer->attachments[1].memory = depth_memory;
    framebuffer->attachments[1].format = VK_FORMAT_D32_SFLOAT;
    framebuffer->attachments[1].width = width;
    framebuffer->attachments[1].height = height;
    framebuffer->attachments[1].type = VK_ATTACHMENT_DEPTH;
    framebuffer->attachments[1].current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    framebuffer->attachments[1].is_valid = true;
    
    strncpy(framebuffer->name, name, 255);
    framebuffer->name[255] = '\0';
    framebuffer->id = framebuffer_id;
    framebuffer->is_valid = true;
    
    g_framebuffer_manager.framebuffer_count++;
    g_framebuffer_manager.total_framebuffers_created++;
    g_framebuffer_manager.total_memory_used += (width * height * 4 + width * height * 4); // Approximate memory usage
    
    printf("Created framebuffer '%s' (%ux%u, %u attachments)\n", name, width, height, framebuffer->attachment_count);
    
    return framebuffer_id;
}

// Destroy framebuffer
bool vk_framebuffer_destroy(u32 framebuffer_id) {
    if (!g_framebuffer_manager.device || framebuffer_id == 0) {
        return false;
    }
    
    if (framebuffer_id > g_framebuffer_manager.next_framebuffer_id) {
        printf("Error: Invalid framebuffer ID %u\n", framebuffer_id);
        return false;
    }
    
    vk_framebuffer_t* framebuffer = &g_framebuffer_manager.framebuffers[framebuffer_id - 1];
    
    if (!framebuffer->is_valid) {
        return false;
    }
    
    // Destroy attachments
    for (u32 i = 0; i < framebuffer->attachment_count; i++) {
        if (framebuffer->attachments[i].is_valid) {
            if (framebuffer->attachments[i].image_view != VK_NULL_HANDLE) {
                vkDestroyImageView(g_framebuffer_manager.device, framebuffer->attachments[i].image_view, NULL);
            }
            
            if (framebuffer->attachments[i].image != VK_NULL_HANDLE) {
                vkDestroyImage(g_framebuffer_manager.device, framebuffer->attachments[i].image, NULL);
            }
            
            if (framebuffer->attachments[i].memory != VK_NULL_HANDLE) {
                vkFreeMemory(g_framebuffer_manager.device, framebuffer->attachments[i].memory, NULL);
            }
            
            framebuffer->attachments[i].is_valid = false;
        }
    }
    
    // Destroy framebuffer
    if (framebuffer->handle != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(g_framebuffer_manager.device, framebuffer->handle, NULL);
    }
    
    // Destroy render pass
    if (framebuffer->render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(g_framebuffer_manager.device, framebuffer->render_pass, NULL);
    }
    
    memset(framebuffer, 0, sizeof(vk_framebuffer_t));
    framebuffer->is_valid = false;
    
    g_framebuffer_manager.total_framebuffers_destroyed++;
    g_framebuffer_manager.framebuffer_count--;
    
    printf("Destroyed framebuffer '%s'\n", framebuffer->name);
    return true;
}

// Get framebuffer handle
VkFramebuffer vk_framebuffer_get_handle(u32 framebuffer_id) {
    if (!g_framebuffer_manager.device || framebuffer_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (framebuffer_id > g_framebuffer_manager.next_framebuffer_id) {
        return VK_NULL_HANDLE;
    }
    
    vk_framebuffer_t* framebuffer = &g_framebuffer_manager.framebuffers[framebuffer_id - 1];
    
    return framebuffer->is_valid ? framebuffer->handle : VK_NULL_HANDLE;
}

// Get render pass handle
VkRenderPass vk_framebuffer_get_render_pass(u32 framebuffer_id) {
    if (!g_framebuffer_manager.device || framebuffer_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (framebuffer_id > g_framebuffer_manager.next_framebuffer_id) {
        return VK_NULL_HANDLE;
    }
    
    vk_framebuffer_t* framebuffer = &g_framebuffer_manager.framebuffers[framebuffer_id - 1];
    
    return framebuffer->is_valid ? framebuffer->render_pass : VK_NULL_HANDLE;
}

// Get framebuffer dimensions
bool vk_framebuffer_get_dimensions(u32 framebuffer_id, u32* width, u32* height) {
    if (!g_framebuffer_manager.device || framebuffer_id == 0) {
        return false;
    }
    
    if (framebuffer_id > g_framebuffer_manager.next_framebuffer_id) {
        return false;
    }
    
    vk_framebuffer_t* framebuffer = &g_framebuffer.framebuffers[framebuffer_id - 1];
    
    if (!framebuffer->is_valid) {
        return false;
    }
    
    if (width) *width = framebuffer->width;
    if (height) *height = framebuffer->height;
    
    return true;
}

// Get attachment by index
VkImage vk_framebuffer_get_attachment(u32 framebuffer_id, u32 attachment_index) {
    if (!g_framebuffer_manager.device || framebuffer_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (framebuffer_id > g_framebuffer_manager.next_framebuffer_id) {
        return VK_NULL_HANDLE;
    }
    
    vk_framebuffer_t* framebuffer = &g_framebuffer.framebuffers[framebuffer_id - 1];
    
    if (!framebuffer->is_valid || attachment_index >= framebuffer->attachment_count) {
        return VK_NULL_HANDLE;
    }
    
    return framebuffer->attachments[attachment_index].image;
}

// Get attachment image view
VkImageView vk_framebuffer_get_attachment_view(u32 framebuffer_id, u32 attachment_index) {
    if (!g_framebuffer_manager.device || framebuffer_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (framebuffer_id > g_framebuffer_manager.next_framebuffer_id) {
        return VK_NULL_HANDLE;
    }
    
    vk_framebuffer_t* framebuffer = &g_framebuffer.framebuffers[framebuffer_id - 1];
    
    if (!framebuffer->is_valid || attachment_index >= framebuffer->attachment_count) {
        return VK_NULL_HANDLE;
    }
    
    return framebuffer->attachments[attachment_index].image_view;
}

// Begin rendering to framebuffer
bool vk_framebuffer_begin(u32 framebuffer_id) {
    if (!g_framebuffer_manager.device || framebuffer_id == 0) {
        return false;
    }
    
    if (framebuffer_id > g_framebuffer_manager.next_framebuffer_id) {
        return false;
    }
    
    vk_framebuffer_t* framebuffer = &g_framebuffer.framebuffers[framebuffer_id - 1];
    
    if (!framebuffer->is_valid) {
        return false;
    }
    
    // Begin render pass
    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = framebuffer->render_pass;
    render_pass_info.framebuffer = framebuffer->handle;
    render_pass_info.renderArea.offset.x = 0;
    render_pass_info.renderArea.extent.width = framebuffer->width;
    render_pass_info.renderArea.extent.height = framebuffer->height;
    render_pass_info.clearValue.color.float32[0] = 0.0f;
    render_pass_info.clearValue.color.float32[1] = 0.0f;
    render_pass_info.clearValue.color.float32[2] = 0.0f;
    render_pass_info.clearValue.color.float32[3] = 1.0f;
    render_passInfo.clearValue.depthStencil.depth = 1.0f;
    render_passInfo.clearValue.stencil = 0;
    
    VkCommandBufferBeginInfo command_buffer_info = {0};
    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_info.flags = 0;
    
    VkCommandBuffer command_buffer;
    vkBeginCommandBuffer(g_framebuffer_manager.device, &command_buffer_info, NULL, &command_buffer);
    
    // Begin render pass
    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE_BIT, 0, NULL, 0, NULL);
    
    return true;
}

// End rendering to framebuffer
bool vk_framebuffer_end(u32 framebuffer_id) {
    if (!g_framebuffer_manager.device || framebuffer_id == 0) {
        return false;
    }
    
    if (framebuffer_id > g_framebuffer_manager.next_framebuffer_id) {
        return false;
    }
    
    vk_framebuffer_t* framebuffer = &g_framebuffer.framebuffers[framebuffer_id - 1];
    
    if (!framebuffer->is_valid) {
        return false;
    }
    
    // End render pass
    vkCmdEndRenderPass(command_buffer);
    
    // End command buffer
    vkEndCommandBuffer(command_buffer);
    
    return true;
}

// Get statistics
void vk_framebuffer_get_stats(u32* total_framebuffers_created, u32* total_framebuffers_destroyed, u64* total_memory_used) {
    if (total_framebuffers_created) *total_framebuffers_created = g_framebuffer_manager.total_framebuffers_created;
    if (total_framebuffers_destroyed) *total_framebuffers_destroyed = g_framebuffer_manager.total_framebuffers_destroyed;
    if (total_memory_used) *total_memory_used = g_framebuffer_manager.total_memory_used;
}

// Validate framebuffer
bool vk_framebuffer_validate(u32 framebuffer_id) {
    if (!g_framebuffer_manager.device || framebuffer_id == 0) {
        return false;
    }
    
    if (framebuffer_id > g_framebuffer_manager.next_framebuffer_id) {
        return false;
    }
    
    vk_framebuffer_t* framebuffer = &g_framebuffer.framebuffers[framebuffer_id - 1];
    
    return framebuffer->is_valid;
}
