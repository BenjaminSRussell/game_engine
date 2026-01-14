#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Vulkan render pass creation system
#define VK_MAX_RENDER_PASSES 64
#define VK_MAX_ATTACHMENTS 8
#define VK_MAX_SUBPASSES 8

typedef enum {
    VK_ATTACHMENT_COLOR = 0,
    VK_ATTACHMENT_DEPTH,
    VK_ATTACHMENT_STENCIL,
    VK_ATTACHMENT_COUNT
} vk_attachment_type_t;

typedef struct vk_attachment_desc {
    vk_attachment_type_t type;
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
    VkAttachmentLoadOp stencil_load_op;
    VkAttachmentStoreOp stencil_store_op;
    VkImageLayout initial_layout;
    VkImageLayout final_layout;
    bool clear_on_load;
    f32 clear_color[4];
    f32 clear_depth;
    u32 clear_stencil;
} vk_attachment_desc_t;

typedef struct vk_subpass_desc {
    VkPipelineBindPoint pipeline_bind_point;
    VkPipelineStageFlags input_stage_mask;
    u32 color_attachment_count;
    u32 color_attachments[VK_MAX_ATTACHMENTS];
    u32 depth_stencil_attachment;
    u32 resolve_attachments[VK_MAX_ATTACHMENTS];
    bool preserve_attachments;
} vk_subpass_desc_t;

typedef struct vk_render_pass {
    VkRenderPass handle;
    VkRenderPassCreateInfo create_info;
    
    vk_attachment_desc_t attachments[VK_MAX_ATTACHMENTS];
    vk_subpass_desc_t subpasses[VK_MAX_SUBPASSES];
    
    u32 attachment_count;
    u32 subpass_count;
    
    char name[256];
    u32 id;
    bool is_created;
} vk_render_pass_t;

typedef struct vk_render_pass_manager {
    VkDevice device;
    
    vk_render_pass_t render_passes[VK_MAX_RENDER_PASSES];
    u32 render_pass_count;
    u32 next_render_pass_id;
    
    // Statistics
    u32 total_render_passes_created;
    u32 total_render_passes_destroyed;
} vk_render_pass_manager_t;

static vk_render_pass_manager_t g_render_pass_manager = {0};

// Convert attachment type to Vulkan aspect mask
static VkImageAspectFlags get_aspect_mask(vk_attachment_type_t type) {
    switch (type) {
        case VK_ATTACHMENT_COLOR:
            return VK_IMAGE_ASPECT_COLOR_BIT;
        case VK_ATTACHMENT_DEPTH:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case VK_ATTACHMENT_STENCIL:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            return 0;
    }
}

// Convert load operation to Vulkan
static VkAttachmentLoadOp convert_load_op(vk_attachment_load_op_t op) {
    switch (op) {
        case VK_ATTACHMENT_LOAD_OP_LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
        case VK_ATTACHMENT_LOAD_OP_CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case VK_ATTACHMENT_LOAD_OP_DONT_CARE: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        default: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

// Convert store operation to Vulkan
static VkAttachmentStoreOp convert_store_op(vk_attachment_store_op_t op) {
    switch (op) {
        case VK_ATTACHMENT_STORE_OP_STORE: return VK_ATTACHMENT_STORE_OP_STORE;
        case VK_ATTACHMENT_STORE_OP_DONT_CARE: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        default: return VK_ATTACHMENT_STORE_OP_STORE;
    }
}

// Initialize render pass manager
bool vk_render_pass_manager_init(VkDevice device) {
    if (!device) {
        printf("Error: Invalid device for render pass manager initialization\n");
        return false;
    }
    
    g_render_pass_manager.device = device;
    
    printf("Vulkan render pass manager initialized\n");
    return true;
}

// Cleanup render pass manager
void vk_render_pass_manager_cleanup(void) {
    if (!g_render_pass_manager.device) {
        return;
    }
    
    // Destroy all render passes
    for (u32 i = 0; i < g_render_pass_manager.render_pass_count; i++) {
        if (g_render_pass_manager.render_passes[i].handle != VK_NULL_HANDLE) {
            vkDestroyRenderPass(g_render_pass_manager.device, g_render_pass_manager.render_passes[i].handle, NULL);
        }
    }
    
    memset(&g_render_pass_manager, 0, sizeof(g_render_pass_manager));
    
    printf("Vulkan render pass manager cleaned up\n");
}

// Create render pass
u32 vk_render_pass_create(const char* name, const vk_attachment_desc_t* attachments, u32 attachment_count,
                           const vk_subpass_desc_t* subpasses, u32 subpass_count) {
    if (!name || !g_render_pass_manager.device || !attachments || attachment_count == 0) {
        return 0;
    }
    
    if (g_render_pass_manager.render_pass_count >= VK_MAX_RENDER_PASSES) {
        printf("Error: Maximum render passes reached\n");
        return 0;
    }
    
    if (attachment_count > VK_MAX_ATTACHMENTS || subpass_count > VK_MAX_SUBPASSES) {
        printf("Error: Too many attachments or subpasses\n");
        return 0;
    }
    
    // Create attachment descriptions
    VkAttachmentDescription vk_attachments[VK_MAX_ATTACHMENTS];
    for (u32 i = 0; i < attachment_count; i++) {
        const vk_attachment_desc_t* desc = &attachments[i];
        
        vk_attachments[i].flags = 0;
        vk_attachments[i].format = desc->format;
        vk_attachments[i].samples = desc->samples;
        vk_attachments[i].loadOp = convert_load_op(desc->load_op);
        vk_attachments[i].storeOp = convert_store_op(desc->store_op);
        vk_attachments[i].stencilLoadOp = convert_load_op(desc->stencil_load_op);
        vk_attachments[i].stencilStoreOp = convert_store_op(desc->stencil_store_op);
        vk_attachments[i].initialLayout = desc->initial_layout;
        vk_attachments[i].finalLayout = desc->final_layout;
    }
    
    // Create subpass descriptions
    VkSubpassDescription vk_subpasses[VK_MAX_SUBPASSES];
    for (u32 i = 0; i < subpass_count; i++) {
        const vk_subpass_desc_t* desc = &subpasses[i];
        
        vk_subpasses[i].pipelineBindPoint = desc->pipeline_bind_point;
        vk_subpasses[i].inputAttachmentIndex = 0; // Simplified
        vk_subpasses[i].colorAttachmentCount = desc->color_attachment_count;
        vk_subpasses[i].preserveAttachmentCount = desc->preserve_attachments ? 1 : 0;
        
        // Copy color attachments
        for (u32 j = 0; j < desc->color_attachment_count && j < VK_MAX_ATTACHMENTS; j++) {
            vk_subpasses[i].pColorAttachments[j].attachment = desc->color_attachments[j];
            vk_subpasses[i].pColorAttachments[j].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        
        // Set depth/stencil attachment
        if (desc->depth_stencil_attachment != 0) {
            vk_subpasses[i].pDepthStencilAttachment.attachment = desc->depth_stencil_attachment;
            vk_subpasses[i].pDepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        }
        
        // Set resolve attachments
        for (u32 j = 0; j < VK_MAX_ATTACHMENTS && j < desc->resolve_attachments[0]; j++) {
            vk_subpasses[i].pResolveAttachments[j].attachment = desc->resolve_attachments[j];
            vk_subpasses[i].pResolveAttachments[j].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        
        // Set input attachments
        if (desc->preserve_attachments) {
            vk_subpasses[i].pInputAttachments[0].attachment = VK_ATTACHMENT_UNUSED;
        }
        
        // Set preserve attachments
        if (desc->preserve_attachments) {
            vk_subpasses[i].pPreserveAttachments[0] = VK_TRUE;
        }
    }
    
    // Create subpass dependencies (simplified)
    VkSubpassDependency dependencies[VK_MAX_SUBPASSES * VK_MAX_SUBPASSES];
    u32 dependency_count = 0;
    
    for (u32 i = 0; i < subpass_count - 1; i++) {
        dependencies[dependency_count].srcSubpass = i;
        dependencies[dependency_count].dstSubpass = i + 1;
        dependencies[dependency_count].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[dependency_count].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[dependency_count].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependency_count++;
    }
    
    // Create render pass
    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = attachment_count;
    render_pass_info.pAttachments = vk_attachments;
    render_pass_info.subpassCount = subpass_count;
    render_pass_info.pSubpasses = vk_subpasses;
    render_pass_info.dependencyCount = dependency_count;
    render_pass_info.pDependencies = dependencies;
    
    VkRenderPass render_pass;
    VkResult result = vkCreateRenderPass(g_render_pass_manager.device, &render_pass_info, NULL, &render_pass);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create render pass\n");
        return 0;
    }
    
    // Store render pass
    u32 render_pass_id = g_render_pass_manager.next_render_pass_id++;
    vk_render_pass_t* rp = &g_render_pass_manager.render_passes[render_pass_id - 1];
    
    rp->handle = render_pass;
    rp->create_info = render_pass_info;
    rp->attachment_count = attachment_count;
    rp->subpass_count = subpass_count;
    
    // Copy attachment descriptions
    for (u32 i = 0; i < attachment_count; i++) {
        rp->attachments[i] = attachments[i];
    }
    
    // Copy subpass descriptions
    for (u32 i = 0; i < subpass_count; i++) {
        rp->subpasses[i] = subpasses[i];
    }
    
    strncpy(rp->name, name, 255);
    rp->name[255] = '\0';
    rp->id = render_pass_id;
    rp->is_created = true;
    
    g_render_pass_manager.render_pass_count++;
    g_render_pass_manager.total_render_passes_created++;
    
    printf("Created render pass '%s' (%u attachments, %u subpasses)\n", 
           name, attachment_count, subpass_count);
    
    return render_pass_id;
}

// Create simple render pass for forward rendering
u32 vk_render_pass_create_forward(const char* name, VkFormat color_format, VkFormat depth_format, VkSampleCountFlagBits samples) {
    if (!name || !g_render_pass_manager.device) {
        return 0;
    }
    
    vk_attachment_desc_t attachments[2];
    u32 attachment_count = 0;
    
    // Color attachment
    if (color_format != VK_FORMAT_UNDEFINED) {
        attachments[attachment_count].type = VK_ATTACHMENT_COLOR;
        attachments[attachment_count].format = color_format;
        attachments[attachment_count].samples = samples;
        attachments[attachment_count].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[attachment_count].store_op = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[attachment_count].stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[attachment_count].stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[attachment_count].initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[attachment_count].final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments[attachment_count].clear_on_load = true;
        attachments[attachment_count].clear_color[0] = 0.0f;
        attachments[attachment_count].clear_color[1] = 0.0f;
        attachments[attachment_attachment_count].clear_color[2] = 0.0f;
        attachments[attachment_count].clear_color[3] = 1.0f;
        attachment_count++;
    }
    
    // Depth attachment
    if (depth_format != VK_FORMAT_UNDEFINED) {
        attachments[attachment_count].type = VK_ATTACHMENT_DEPTH;
        attachments[attachment_count].format = depth_format;
        attachments[attachment_count].samples = samples;
        attachments[attachment_count].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[attachment_count].store_op = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[attachment_count].stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[attachment_count].stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[attachment_count].initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[attachment_count].final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[attachment_count].clear_on_load = true;
        attachments[attachment_count].clear_depth = 1.0f;
        attachments[attachment_count].clear_stencil = 0;
        attachment_count++;
    }
    
    // Create subpass
    vk_subpass_desc_t subpass = {0};
    subpass.pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.input_stage_mask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpass.color_attachment_count = (color_format != VK_FORMAT_UNDEFINED) ? 1 : 0;
    subpass.color_attachments[0] = (color_format != VK_FORMAT_UNDEFINED) ? 0 : VK_ATTACHMENT_UNUSED;
    subpass.depth_stencil_attachment = (depth_format != VK_FORMAT_UNDEFINED) ? 0 : VK_ATTACHMENT_UNUSED;
    subpass.resolve_attachments[0] = VK_ATTACHMENT_UNUSED;
    subpass.preserve_attachments = false;
    
    return vk_render_pass_create(name, attachments, attachment_count, &subpass, 1);
}

// Create simple render pass for deferred rendering
u32 vk_render_pass_create_deferred(const char* name, VkFormat color_format, VkFormat depth_format, VkSampleCountFlagBits samples) {
    if (!name || !g_render_pass_device) {
        return 0;
    }
    
    // G-buffer attachments
    vk_attachment_desc_t attachments[5];
    u32 attachment_count = 0;
    
    // Position buffer
    attachments[attachment_count].type = VK_ATTACHMENT_COLOR;
    attachments[attachment_count].format = VK_FORMAT_R16G16B16A16_UNORM;
    attachments[attachment_count].samples = samples;
    attachments[attachment_count].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[attachment_count].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[attachment_count].stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[attachment_count].stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[attachment_count].initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[attachment_count].final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[attachment_count].clear_on_load = true;
    attachments[attachment_count].clear_color[0] = 0.0f;
    attachments[attachment_count].clear_color[1] = 0.0f;
    attachments[attachment_count].clear_color[2] = 0.0f;
    attachments[attachment_count].clear_color[3] = 0.0f;
    attachment_count++;
    
    // Normal buffer
    attachments[attachment_count].type = VK_ATTACHMENT_COLOR;
    attachments[attachment_count].format = VK_FORMAT_R16G16B16A16_UNORM;
    attachments[attachment_count].samples = samples;
    attachments[attachment_count].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[attachment_count].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[attachment_count].stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[attachment_count].stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[attachment_count].initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[attachment_count].final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[attachment_count].clear_on_load = true;
    attachments[attachment_count].clear_color[0] = 0.0f;
    attachments[attachment_count].clear_color[1] = 0.0f;
    attachments[attachment_count].clear_color[2] = 0.0f;
    attachments[attachment_count].clear_color[3] = 0.0f;
    attachment_count++;
    
    // Albedo buffer
    attachments[attachment_count].type = VK_ATTACHMENT_COLOR;
    attachments[attachment_count].format = color_format;
    attachments[attachment_count].samples = samples;
    attachments[attachment_count].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[attachment_count].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[attachment_count].stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[attachment_count].stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[attachment_count].initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[attachment_count].final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[attachment_count].clear_on_load = true;
    attachments[attachment_count].clear_color[0] = 0.0f;
    attachments[attachment_count].clear_color[1] = 0.0f;
    attachments[attachment_count].clear_color[2] = 0.0f;
    attachments[attachment_count].clear_color[3] = 1.0f;
    attachment_count++;
    
    // Roughness buffer
    attachments[attachment_count].type = VK_ATTACHMENT_COLOR;
    attachments[attachment_count].format = VK_FORMAT_R8_UNORM;
    attachments[attachment_count].samples = samples;
    attachments[attachment_count].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[attachment_count].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[attachment_count].stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CONT_CARE;
    attachments[attachment_count].stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[attachment_count].initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[attachment_count].final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[attachment_count].clear_on_load = true;
    attachments[attachment_count].clear_color[attachment_count].clear_color[0] = 0.0f;
    attachments[attachment_count].clear_color[attachment_count].clear_color[1] = 0.0f;
    attachments[attachment_count].clear_color[attachment_count].clear_color[2] = 0.0f;
    attachments[attachment_count].clear_color[attachment_count].clear_color[3] = 0.0f;
    attachment_count++;
    
    // Depth buffer
    attachments[attachment_count].type = VK_ATTACHMENT_DEPTH;
    attachments[attachment_count].format = depth_format;
    attachments[attachment_count].samples = samples;
    attachments[attachment_count].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[attachment_count].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[attachment_count].stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[attachment_count].stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[attachment_count].initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[attachment_count].final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[attachment_count].clear_on_load = true;
    attachments[attachment_count].clear_depth = 1.0f;
    attachments[attachment_count].clear_stencil = 0;
    attachment_count++;
    
    // Create geometry subpass
    vk_subpass_desc_t geometry_subpass = {0};
    geometry_subpass.pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    geometry_subpass.input_stage_mask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    geometry_subpass.color_attachment_count = 3; // Position, Normal, Albedo
    geometry_subpass.color_attachments[0] = 0;
    geometry_subpass.color_attachments[1] = 1;
    geometry_subpass.color_attachments[2] = 2;
    geometry_subpass.depth_stencil_attachment = 4;
    geometry_subpass.resolve_attachments[0] = VK_ATTACHMENT_UNUSED;
    geometry_subpass.preserve_attachments = true;
    
    // Create lighting subpass
    vk_subpass_desc_t lighting_subpass = {0};
    lighting_subpass.pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    lighting_subpass.input_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    lighting_subpass.color_attachment_count = 2; // Albedo, Roughness
    lighting_subpass.color_attachments[0] = 2;
    lighting_subpass.color_attachments[1] = 3;
    lighting_subpass.depth_stencil_attachment = 4;
    lighting_subpass.resolve_attachments[0] = VK_ATTACHMENT_UNUSED;
    lighting_subpass.preserve_attachments = true;
    
    // Create composition subpass
    vk_subpass_desc_t composition_subpass = {0};
    composition_subpass.pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    composition_subpass.input_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    composition_subpass.color_attachment_count = 1; // Final color
    composition_subpass.color_attachments[0] = 2;
    composition_subpass.depth_stencil_attachment = 4;
    composition_subpass.resolve_attachments[0] = VK_ATTACHMENT_UNUSED;
    composition_subpass.preserve_attachments = false;
    
    // Create render pass with multiple subpasses
    vk_subpass_desc_t subpasses[3] = {geometry_subpass, lighting_subpass, composition_subpass};
    
    return vk_render_pass_create(name, attachments, attachment_count, subpasses, 3);
}

// Destroy render pass
bool vk_render_pass_destroy(u32 render_pass_id) {
    if (!g_render_pass_manager.device || render_pass_id == 0) {
        return false;
    }
    
    if (render_pass_id > g_render_pass_manager.next_render_pass_id) {
        printf("Error: Invalid render pass ID %u\n", render_pass_id);
        return false;
    }
    
    vk_render_pass_t* rp = &g_render_pass_manager.render_passes[render_pass_id - 1];
    
    if (!rp->is_created) {
        return false;
    }
    
    if (rp->handle != VK_NULL_HANDLE) {
        vkDestroyRenderPass(g_render_pass_manager.device, rp->handle, NULL);
    }
    
    memset(rp, 0, sizeof(vk_render_pass_t));
    rp->is_created = false;
    
    g_render_pass_manager.total_render_passes_destroyed++;
    
    printf("Destroyed render pass '%s'\n", rp->name);
    return true;
}

// Get render pass handle
VkRenderPass vk_render_pass_get_handle(u32 render_pass_id) {
    if (!g_render_pass_manager.device || render_pass_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (render_pass_id > g_render_pass_manager.next_render_pass_id) {
        return VK_NULL_HANDLE;
    }
    
    return g_render_pass_render_passes[render_pass_id - 1].handle;
}

// Get render pass info
bool vk_render_pass_get_info(u32 render_pass_id, char* name, size_t name_size, u32* attachment_count, u32* subpass_count) {
    if (!g_render_pass_manager.device || render_pass_id == 0) {
        return false;
    }
    
    if (render_pass_id > g_render_pass_manager.next_render_pass_id) {
        return false;
    }
    
    vk_render_pass_t* rp = &g_render_pass_manager.render_pass[render_pass_id - 1];
    
    if (!rp->is_created) {
        return false;
    }
    
    if (name && name_size > 0) {
        strncpy(name, rp->name, name_size - 1);
        name[name_size - 1] = '\0';
    }
    
    if (attachment_count) *attachment_count = rp->attachment_count;
    if (subpass_count) *subpass_count = rp->subpass_count;
    
    return true;
}

// Get statistics
void vk_render_pass_get_stats(u32* total_render_passes_created, u32* total_render_passes_destroyed) {
    if (total_render_passes_created) *total_render_passes_created = g_render_pass_manager.total_render_passes_created;
    if (total_render_passes_destroyed) *total_render_passes_destroyed = g_render_pass_manager.total_render_passes_destroyed;
}
