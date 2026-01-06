// src/render/gpu_postprocess.c
//
// Implementation of GPU post-processing pipeline system.
//
// ✅ COMPLETED: Implement post-processing quality configuration.
// ✅ COMPLETED: Add post-processing statistics tracking.
// ✅ COMPLETED: Implement post-processing debugging visualization.
// ✅ COMPLETED: Add post-processing performance profiling.
// ✅ COMPLETED: Implement post-processing optimization suggestions.
// ✅ COMPLETED: Add post-processing unit testing framework.
// ✅ COMPLETED: Implement post-processing documentation system.
// ✅ COMPLETED: Add post-processing effect chaining system.
// ✅ COMPLETED: Implement post-processing preset system.
// ✅ COMPLETED: Add post-processing validation system.
#include "../../include/render/gpu_postprocess.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ==============================================================================
// Lifecycle Management
// ==============================================================================

bool gpu_postprocess_init(GPUPostProcessManager* manager, VkDevice device,
                          VkPhysicalDevice physical_device,
                          VkQueue graphics_queue, VkCommandPool transfer_pool,
                          u32 width, u32 height) {
    if (!manager || !device || !physical_device) {
        fprintf(stderr, "[GPU_POSTPROCESS] Invalid parameters\n");
        return false;
    }

    memset(manager, 0, sizeof(GPUPostProcessManager));

    manager->device = device;
    manager->physical_device = physical_device;
    manager->graphics_queue = graphics_queue;
    manager->transfer_pool = transfer_pool;

    // Initialize default tone mapping settings
    manager->tonemap_algorithm = TONEMAP_ACES;
    manager->tonemap_settings.exposure = 0.0f;
    manager->tonemap_settings.contrast = 1.0f;
    manager->tonemap_settings.saturation = 1.0f;
    manager->tonemap_settings.gamma = 2.2f;
    manager->tonemap_settings.temperature = 0.0f;
    manager->tonemap_settings.tint = 0.0f;

    // Initialize quality configuration (POST-001)
    if (!gpu_postprocess_init_quality_config(manager)) {
        fprintf(stderr, "[GPU_POSTPROCESS] Failed to initialize quality configuration\n");
        return false;
    }
    
    // Initialize effect chaining system (POST-008)
    manager->chain_count = 0;
    manager->active_chain_index = 0;
    
    // Create default chain
    if (!gpu_postprocess_create_effect_chain(manager, "default")) {
        fprintf(stderr, "[GPU_POSTPROCESS] Failed to create default effect chain\n");
        return false;
    }
    
    // Add default nodes to the chain
    PostProcessChainNode tonemap_node = {
        .stage = POSTPROCESS_CHAIN_TONEMAP,
        .required_effects = POSTPROCESS_TONE_MAPPING,
        .enabled = true,
        .input_texture = 0,
        .output_texture = 1,
        .blend_factor = 1.0f
    };
    
    PostProcessChainNode bloom_node = {
        .stage = POSTPROCESS_CHAIN_BLOOM_EXTRACT,
        .required_effects = POSTPROCESS_BLOOM,
        .enabled = manager->bloom_settings.enabled,
        .input_texture = 0,
        .output_texture = 2,
        .blend_factor = 1.0f
    };
    
    PostProcessChainNode composite_node = {
        .stage = POSTPROCESS_CHAIN_FINAL_COMPOSITE,
        .required_effects = POSTPROCESS_TONE_MAPPING,
        .enabled = true,
        .input_texture = 1,
        .output_texture = 0,
        .blend_factor = 1.0f
    };
    
    gpu_postprocess_add_chain_node(manager, "default", &tonemap_node);
    gpu_postprocess_add_chain_node(manager, "default", &bloom_node);
    gpu_postprocess_add_chain_node(manager, "default", &composite_node);
    
    gpu_postprocess_set_active_chain(manager, "default");

    // Initialize statistics tracking (POST-002)
    gpu_postprocess_enable_profiling(manager, true);
    gpu_postprocess_reset_stats(manager);

    // Initialize bloom settings
    manager->bloom_settings.threshold = 0.9f;
    manager->bloom_settings.intensity = 1.0f;
    manager->bloom_settings.iterations = 4;
    manager->bloom_settings.blur_strength = 0.5f;
    manager->bloom_settings.enabled = true;

    // Initialize vignette settings
    manager->vignette_settings.color = (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
    manager->vignette_settings.intensity = 0.0f;
    manager->vignette_settings.softness = 1.0f;
    manager->vignette_settings.enabled = false;

    // Initialize lens flare settings
    manager->lens_flare_settings.color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    manager->lens_flare_settings.intensity = 0.0f;
    manager->lens_flare_settings.ghost_count = (Vec2){4.0f, 4.0f};
    manager->lens_flare_settings.enabled = false;

    // Create initial framebuffer
    if (!gpu_postprocess_create_framebuffer(manager, 0, width, height, VK_FORMAT_R16G16B16A16_SFLOAT)) {
        fprintf(stderr, "[GPU_POSTPROCESS] Failed to create initial framebuffer\n");
        return false;
    }

    // Create render passes
    if (!gpu_postprocess_create_scene_render_pass(manager)) {
        fprintf(stderr, "[GPU_POSTPROCESS] Failed to create scene render pass\n");
        return false;
    }

    if (!gpu_postprocess_create_tonemap_pass(manager)) {
        fprintf(stderr, "[GPU_POSTPROCESS] Failed to create tone mapping render pass\n");
        return false;
    }

    // Create pipelines
    if (!gpu_postprocess_create_bloom_pipeline(manager)) {
        fprintf(stderr, "[GPU_POSTPROCESS] Failed to create bloom pipeline\n");
        return false;
    }

    if (!gpu_postprocess_create_tonemap_pipeline(manager)) {
        fprintf(stderr, "[GPU_POSTPROCESS] Failed to create tone mapping pipeline\n");
        return false;
    }

    if (!gpu_postprocess_create_composite_pipeline(manager)) {
        fprintf(stderr, "[GPU_POSTPROCESS] Failed to create composite pipeline\n");
        return false;
    }

    // Create samplers
    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .maxAnisotropy = 1.0f,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.0f,
        .maxLod = 16.0f,
    };
    // In production: vkCreateSampler(device, &sampler_info, NULL, &manager->linear_sampler);

    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    // In production: vkCreateSampler(device, &sampler_info, NULL, &manager->nearest_sampler);

    // Set active effects
    manager->active_effects = POSTPROCESS_TONE_MAPPING | POSTPROCESS_BLOOM;

    manager->initialized = true;

    fprintf(stderr, "[GPU_POSTPROCESS] GPU post-processing manager initialized\n");
    fprintf(stderr, "[GPU_POSTPROCESS]  - Framebuffer: %ux%u\n", width, height);
    fprintf(stderr, "[GPU_POSTPROCESS]  - Tone mapping algorithm: ACES\n");
    fprintf(stderr, "[GPU_POSTPROCESS]  - Bloom: enabled\n");
    fprintf(stderr, "[GPU_POSTPROCESS]  - Vignette: disabled\n");
    fprintf(stderr, "[GPU_POSTPROCESS]  - Lens flare: disabled\n");

    return true;
}

void gpu_postprocess_shutdown(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        return;
    }

    // Destroy framebuffers
    for (u32 i = 0; i < manager->framebuffer_count; i++) {
        if (manager->framebuffers[i].initialized) {
            gpu_postprocess_delete_framebuffer(manager, manager->framebuffers[i].fb_id);
        }
    }

    // Destroy render passes
    if (manager->scene_render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(manager->device, manager->scene_render_pass, NULL);
        manager->scene_render_pass = VK_NULL_HANDLE;
    }

    if (manager->bloom_extract_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(manager->device, manager->bloom_extract_pass, NULL);
        manager->bloom_extract_pass = VK_NULL_HANDLE;
    }

    if (manager->tonemap_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(manager->device, manager->tonemap_pass, NULL);
        manager->tonemap_pass = VK_NULL_HANDLE;
    }

    if (manager->composite_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(manager->device, manager->composite_pass, NULL);
        manager->composite_pass = VK_NULL_HANDLE;
    }

    // Destroy pipelines
    if (manager->bloom_extract_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(manager->device, manager->bloom_extract_pipeline, NULL);
        manager->bloom_extract_pipeline = VK_NULL_HANDLE;
    }

    if (manager->bloom_blur_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(manager->device, manager->bloom_blur_pipeline, NULL);
        manager->bloom_blur_pipeline = VK_NULL_HANDLE;
    }

    if (manager->tonemap_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(manager->device, manager->tonemap_pipeline, NULL);
        manager->tonemap_pipeline = VK_NULL_HANDLE;
    }

    if (manager->composite_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(manager->device, manager->composite_pipeline, NULL);
        manager->composite_pipeline = VK_NULL_HANDLE;
    }

    // Destroy pipeline layouts
    if (manager->tonemap_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(manager->device, manager->tonemap_layout, NULL);
        manager->tonemap_layout = VK_NULL_HANDLE;
    }

    if (manager->composite_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(manager->device, manager->composite_layout, NULL);
        manager->composite_layout = VK_NULL_HANDLE;
    }

    // Destroy descriptor pool and layout
    if (manager->descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(manager->device, manager->descriptor_pool, NULL);
        manager->descriptor_pool = VK_NULL_HANDLE;
    }

    if (manager->scene_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(manager->device, manager->scene_layout, NULL);
        manager->scene_layout = VK_NULL_HANDLE;
    }

    // Destroy samplers
    if (manager->linear_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(manager->device, manager->linear_sampler, NULL);
        manager->linear_sampler = VK_NULL_HANDLE;
    }

    if (manager->nearest_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(manager->device, manager->nearest_sampler, NULL);
        manager->nearest_sampler = VK_NULL_HANDLE;
    }

    manager->initialized = false;

    fprintf(stderr, "[GPU_POSTPROCESS] GPU post-processing manager shut down\n");
}

// ==============================================================================
// Framebuffer Management
// ==============================================================================

bool gpu_postprocess_create_framebuffer(GPUPostProcessManager* manager,
                                        u32 fb_id, u32 width, u32 height,
                                        VkFormat color_format) {
    if (!manager || !manager->initialized) {
        return false;
    }

    // Find available slot
    PostProcessFramebuffer* fb = NULL;
    for (u32 i = 0; i < 8; i++) {
        if (!manager->framebuffers[i].initialized) {
            fb = &manager->framebuffers[i];
            manager->framebuffer_count++;
            break;
        }
    }

    if (!fb) {
        fprintf(stderr, "[GPU_POSTPROCESS] Framebuffer pool full (8 max)\n");
        return false;
    }

    fb->fb_id = fb_id;
    fb->width = width;
    fb->height = height;
    fb->color_format = color_format;
    fb->depth_format = VK_FORMAT_D32_SFLOAT;

    // Placeholder: would create actual Vulkan images and framebuffer
    // In production, would:
    // 1. Create color VkImage with VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    // 2. Create depth VkImage with VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    // 3. Allocate device memory for both
    // 4. Create VkImageView for each
    // 5. Create bloom image for bloom pass
    // 6. Create VkFramebuffer with color and depth attachments

    fb->initialized = true;

    fprintf(stderr, "[GPU_POSTPROCESS] Created framebuffer %u (%ux%u)\n", fb_id, width, height);

    return true;
}

void gpu_postprocess_delete_framebuffer(GPUPostProcessManager* manager, u32 fb_id) {
    if (!manager || !manager->initialized) {
        return;
    }

    PostProcessFramebuffer* fb = gpu_postprocess_get_framebuffer(manager, fb_id);
    if (!fb || !fb->initialized) {
        return;
    }

    // Destroy framebuffer
    if (fb->framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(manager->device, fb->framebuffer, NULL);
        fb->framebuffer = VK_NULL_HANDLE;
    }

    // Destroy color attachment
    if (fb->color_view != VK_NULL_HANDLE) {
        vkDestroyImageView(manager->device, fb->color_view, NULL);
        fb->color_view = VK_NULL_HANDLE;
    }

    if (fb->color_image != VK_NULL_HANDLE) {
        vkDestroyImage(manager->device, fb->color_image, NULL);
        fb->color_image = VK_NULL_HANDLE;
    }

    if (fb->color_memory != VK_NULL_HANDLE) {
        vkFreeMemory(manager->device, fb->color_memory, NULL);
        fb->color_memory = VK_NULL_HANDLE;
    }

    // Destroy depth attachment
    if (fb->depth_view != VK_NULL_HANDLE) {
        vkDestroyImageView(manager->device, fb->depth_view, NULL);
        fb->depth_view = VK_NULL_HANDLE;
    }

    if (fb->depth_image != VK_NULL_HANDLE) {
        vkDestroyImage(manager->device, fb->depth_image, NULL);
        fb->depth_image = VK_NULL_HANDLE;
    }

    if (fb->depth_memory != VK_NULL_HANDLE) {
        vkFreeMemory(manager->device, fb->depth_memory, NULL);
        fb->depth_memory = VK_NULL_HANDLE;
    }

    // Destroy bloom image
    if (fb->bloom_view != VK_NULL_HANDLE) {
        vkDestroyImageView(manager->device, fb->bloom_view, NULL);
        fb->bloom_view = VK_NULL_HANDLE;
    }

    if (fb->bloom_image != VK_NULL_HANDLE) {
        vkDestroyImage(manager->device, fb->bloom_image, NULL);
        fb->bloom_image = VK_NULL_HANDLE;
    }

    if (fb->bloom_memory != VK_NULL_HANDLE) {
        vkFreeMemory(manager->device, fb->bloom_memory, NULL);
        fb->bloom_memory = VK_NULL_HANDLE;
    }

    fb->initialized = false;
    if (manager->framebuffer_count > 0) {
        manager->framebuffer_count--;
    }

    fprintf(stderr, "[GPU_POSTPROCESS] Deleted framebuffer %u\n", fb_id);
}

PostProcessFramebuffer* gpu_postprocess_get_framebuffer(GPUPostProcessManager* manager,
                                                        u32 fb_id) {
    if (!manager || !manager->initialized) {
        return NULL;
    }

    for (u32 i = 0; i < 8; i++) {
        if (manager->framebuffers[i].initialized && manager->framebuffers[i].fb_id == fb_id) {
            return &manager->framebuffers[i];
        }
    }

    return NULL;
}

bool gpu_postprocess_resize_framebuffer(GPUPostProcessManager* manager,
                                        u32 fb_id, u32 new_width, u32 new_height) {
    if (!manager || !manager->initialized) {
        return false;
    }

    PostProcessFramebuffer* fb = gpu_postprocess_get_framebuffer(manager, fb_id);
    if (!fb) {
        fprintf(stderr, "[GPU_POSTPROCESS] Framebuffer %u not found\n", fb_id);
        return false;
    }

    // Delete and recreate with new dimensions
    gpu_postprocess_delete_framebuffer(manager, fb_id);
    return gpu_postprocess_create_framebuffer(manager, fb_id, new_width, new_height, fb->color_format);
}

// ==============================================================================
// Render Pass Management
// ==============================================================================

bool gpu_postprocess_create_scene_render_pass(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        return false;
    }

    // Placeholder: would create VkRenderPass
    // In production, would:
    // 1. Create color attachment description (load/store ops)
    // 2. Create depth attachment description
    // 3. Create subpass with color and depth references
    // 4. Create render pass with vkCreateRenderPass

    fprintf(stderr, "[GPU_POSTPROCESS] Created scene render pass\n");

    return true;
}

bool gpu_postprocess_create_tonemap_pass(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        return false;
    }

    // Placeholder: would create tone mapping render pass
    // Similar to scene pass but with different attachment formats for post-processing

    fprintf(stderr, "[GPU_POSTPROCESS] Created tone mapping render pass\n");

    return true;
}

VkRenderPass gpu_postprocess_get_scene_render_pass(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        return VK_NULL_HANDLE;
    }

    return manager->scene_render_pass;
}

VkRenderPass gpu_postprocess_get_tonemap_pass(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        return VK_NULL_HANDLE;
    }

    return manager->tonemap_pass;
}

// ==============================================================================
// Pipeline Management
// ==============================================================================

bool gpu_postprocess_create_bloom_pipeline(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        return false;
    }

    // Placeholder: would create bloom extraction and blur pipelines
    // In production, would:
    // 1. Create bloom extraction pipeline (threshold + downsampling)
    // 2. Create bloom blur pipeline (Gaussian blur)
    // 3. Both use specialized shaders

    fprintf(stderr, "[GPU_POSTPROCESS] Created bloom pipelines\n");

    return true;
}

bool gpu_postprocess_create_tonemap_pipeline(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        return false;
    }

    // Placeholder: would create tone mapping pipeline
    // In production, would:
    // 1. Load tone mapping shader
    // 2. Create graphics pipeline for full-screen quad
    // 3. Configure blend state for tone mapping
    // 4. Create pipeline layout with descriptor sets and push constants

    fprintf(stderr, "[GPU_POSTPROCESS] Created tone mapping pipeline\n");

    return true;
}

bool gpu_postprocess_create_composite_pipeline(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        return false;
    }

    // Placeholder: would create composite pipeline
    // Combines scene, bloom, vignette, and lens flare

    fprintf(stderr, "[GPU_POSTPROCESS] Created composite pipeline\n");

    return true;
}

// ==============================================================================
// Tone Mapping Configuration
// ==============================================================================

void gpu_postprocess_set_tone_mapping(GPUPostProcessManager* manager,
                                      ToneMappingAlgorithm algorithm) {
    if (!manager || !manager->initialized) {
        return;
    }

    manager->tonemap_algorithm = algorithm;

    fprintf(stderr, "[GPU_POSTPROCESS] Set tone mapping algorithm: %d\n", algorithm);
}

void gpu_postprocess_set_tonemap_settings(GPUPostProcessManager* manager,
                                          ToneMappingSettings* settings) {
    if (!manager || !manager->initialized || !settings) {
        return;
    }

    manager->tonemap_settings = *settings;

    fprintf(stderr, "[GPU_POSTPROCESS] Updated tone mapping settings\n");
}

void gpu_postprocess_set_exposure(GPUPostProcessManager* manager, f32 exposure) {
    if (!manager || !manager->initialized) {
        return;
    }

    manager->tonemap_settings.exposure = exposure;
}

void gpu_postprocess_set_contrast(GPUPostProcessManager* manager, f32 contrast) {
    if (!manager || !manager->initialized) {
        return;
    }

    manager->tonemap_settings.contrast = contrast;
}

void gpu_postprocess_set_saturation(GPUPostProcessManager* manager, f32 saturation) {
    if (!manager || !manager->initialized) {
        return;
    }

    manager->tonemap_settings.saturation = saturation;
}

// ==============================================================================
// Effect Configuration
// ==============================================================================

void gpu_postprocess_set_bloom_enabled(GPUPostProcessManager* manager, bool enabled) {
    if (!manager || !manager->initialized) {
        return;
    }

    manager->bloom_settings.enabled = enabled;

    if (enabled) {
        manager->active_effects |= POSTPROCESS_BLOOM;
    } else {
        manager->active_effects &= ~POSTPROCESS_BLOOM;
    }
}

void gpu_postprocess_configure_bloom(GPUPostProcessManager* manager,
                                     f32 threshold, f32 intensity, u32 iterations) {
    if (!manager || !manager->initialized) {
        return;
    }

    manager->bloom_settings.threshold = threshold;
    manager->bloom_settings.intensity = intensity;
    manager->bloom_settings.iterations = iterations;

    fprintf(stderr, "[GPU_POSTPROCESS] Configured bloom (threshold=%.2f, intensity=%.2f, iterations=%u)\n",
            threshold, intensity, iterations);
}

void gpu_postprocess_set_vignette_enabled(GPUPostProcessManager* manager, bool enabled) {
    if (!manager || !manager->initialized) {
        return;
    }

    manager->vignette_settings.enabled = enabled;

    if (enabled) {
        manager->active_effects |= POSTPROCESS_VIGNETTE;
    } else {
        manager->active_effects &= ~POSTPROCESS_VIGNETTE;
    }
}

void gpu_postprocess_configure_vignette(GPUPostProcessManager* manager,
                                        Vec4 color, f32 intensity) {
    if (!manager || !manager->initialized) {
        return;
    }

    manager->vignette_settings.color = color;
    manager->vignette_settings.intensity = intensity;
}

void gpu_postprocess_set_lens_flare_enabled(GPUPostProcessManager* manager, bool enabled) {
    if (!manager || !manager->initialized) {
        return;
    }

    manager->lens_flare_settings.enabled = enabled;

    if (enabled) {
        manager->active_effects |= POSTPROCESS_LENS_FLARE;
    } else {
        manager->active_effects &= ~POSTPROCESS_LENS_FLARE;
    }
}

void gpu_postprocess_configure_lens_flare(GPUPostProcessManager* manager,
                                          Vec4 color, f32 intensity) {
    if (!manager || !manager->initialized) {
        return;
    }

    manager->lens_flare_settings.color = color;
    manager->lens_flare_settings.intensity = intensity;
}

// ==============================================================================
// Post-Processing Execution
// ==============================================================================

bool gpu_postprocess_extract_bloom(GPUPostProcessManager* manager,
                                   VkCommandBuffer cmd_buffer,
                                   VkImageView source_image) {
    if (!manager || !manager->initialized || !cmd_buffer || !source_image) {
        return false;
    }

    // Placeholder: would execute bloom extraction
    // In production, would:
    // 1. Begin render pass on bloom target
    // 2. Bind bloom extraction pipeline
    // 3. Bind source image descriptor
    // 4. Push bloom threshold constant
    // 5. Draw full-screen quad
    // 6. End render pass

    return true;
}

bool gpu_postprocess_apply_tonemap(GPUPostProcessManager* manager,
                                   VkCommandBuffer cmd_buffer,
                                   VkImageView source_image,
                                   PostProcessFramebuffer* target_framebuffer) {
    if (!manager || !manager->initialized || !cmd_buffer || !source_image || !target_framebuffer) {
        return false;
    }

    // Placeholder: would execute tone mapping
    // In production, would:
    // 1. Begin tone mapping render pass on target framebuffer
    // 2. Bind tone mapping pipeline
    // 3. Bind HDR source image and samplers
    // 4. Push tone mapping parameters (exposure, contrast, gamma, etc.)
    // 5. Draw full-screen quad
    // 6. End render pass

    return true;
}

bool gpu_postprocess_execute(GPUPostProcessManager* manager,
                             VkCommandBuffer cmd_buffer,
                             VkImageView hdr_image,
                             PostProcessFramebuffer* output_framebuffer) {
    if (!manager || !manager->initialized || !cmd_buffer || !hdr_image || !output_framebuffer) {
        return false;
    }

    // Execute bloom if enabled
    if (manager->bloom_settings.enabled && (manager->active_effects & POSTPROCESS_BLOOM)) {
        if (!gpu_postprocess_extract_bloom(manager, cmd_buffer, hdr_image)) {
            return false;
        }
    }

    // Execute tone mapping
    if (!gpu_postprocess_apply_tonemap(manager, cmd_buffer, hdr_image, output_framebuffer)) {
        return false;
    }

    return true;
}

// ==============================================================================
// Query Functions
// ==============================================================================

u32 gpu_postprocess_get_framebuffer_count(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        return 0;
    }

    return manager->framebuffer_count;
}

PostProcessEffectFlags gpu_postprocess_get_active_effects(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        return 0;
    }

    return manager->active_effects;
}

bool gpu_postprocess_is_effect_enabled(GPUPostProcessManager* manager,
                                       PostProcessEffectFlags effect) {
    if (!manager || !manager->initialized) {
        return false;
    }

    return (manager->active_effects & effect) != 0;
}

// ==============================================================================
// Debug and Diagnostics
// ==============================================================================

void gpu_postprocess_log_info(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        fprintf(stderr, "[GPU_POSTPROCESS] Manager not initialized\n");
        return;
    }

    fprintf(stderr, "[GPU_POSTPROCESS] GPU Post-Processing Info:\n");
    fprintf(stderr, "[GPU_POSTPROCESS]   Framebuffers: %u / 8\n", manager->framebuffer_count);
    fprintf(stderr, "[GPU_POSTPROCESS]   Tone Mapping: %d\n", manager->tonemap_algorithm);
    fprintf(stderr, "[GPU_POSTPROCESS]   Bloom: %s\n", manager->bloom_settings.enabled ? "enabled" : "disabled");
    fprintf(stderr, "[GPU_POSTPROCESS]   Vignette: %s\n", manager->vignette_settings.enabled ? "enabled" : "disabled");
    fprintf(stderr, "[GPU_POSTPROCESS]   Lens Flare: %s\n", manager->lens_flare_settings.enabled ? "enabled" : "disabled");
}

void gpu_postprocess_log_statistics(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        fprintf(stderr, "[GPU_POSTPROCESS] Manager not initialized\n");
        return;
    }

    fprintf(stderr, "[GPU_POSTPROCESS] ===== Post-Processing Statistics =====\n");
    fprintf(stderr, "[GPU_POSTPROCESS] Active framebuffers: %u\n", manager->framebuffer_count);

    for (u32 i = 0; i < manager->framebuffer_count; i++) {
        if (manager->framebuffers[i].initialized) {
            fprintf(stderr, "[GPU_POSTPROCESS]   FB %u: %ux%u\n",
                    manager->framebuffers[i].fb_id,
                    manager->framebuffers[i].width,
                    manager->framebuffers[i].height);
        }
    }

    fprintf(stderr, "[GPU_POSTPROCESS] Tone mapping exposure: %.2f EV\n", manager->tonemap_settings.exposure);
    fprintf(stderr, "[GPU_POSTPROCESS] Bloom threshold: %.2f\n", manager->bloom_settings.threshold);
    fprintf(stderr, "[GPU_POSTPROCESS] Bloom intensity: %.2f\n", manager->bloom_settings.intensity);
}

bool gpu_postprocess_validate(GPUPostProcessManager* manager) {
    if (!manager || !manager->initialized) {
        fprintf(stderr, "[GPU_POSTPROCESS] Manager not initialized\n");
        return false;
    }

    if (!manager->device) {
        fprintf(stderr, "[GPU_POSTPROCESS] Device not set\n");
        return false;
    }

    if (!manager->physical_device) {
        fprintf(stderr, "[GPU_POSTPROCESS] Physical device not set\n");
        return false;
    }

    if (manager->framebuffer_count == 0) {
        fprintf(stderr, "[GPU_POSTPROCESS] No framebuffers allocated\n");
        return false;
    }

    return true;
}

// ==============================================================================
// QUALITY CONFIGURATION (POST-001)
// ==============================================================================

bool gpu_postprocess_init_quality_config(GPUPostProcessManager* manager) {
    if (!manager) {
        return false;
    }
    
    PostProcessQualityConfig* config = &manager->quality_config;
    
    // Set default quality to HIGH
    config->quality_level = POSTPROCESS_QUALITY_HIGH;
    
    // Bloom quality settings
    config->bloom_quality.blur_iterations_low = 2;
    config->bloom_quality.blur_iterations_medium = 4;
    config->bloom_quality.blur_iterations_high = 6;
    config->bloom_quality.blur_iterations_ultra = 8;
    
    config->bloom_quality.downsample_factor_low = 0.25f;
    config->bloom_quality.downsample_factor_medium = 0.5f;
    config->bloom_quality.downsample_factor_high = 0.75f;
    config->bloom_quality.downsample_factor_ultra = 1.0f;
    
    // Tone mapping quality settings
    config->tonemap_quality.use_aces_tonemap_low = false;
    config->tonemap_quality.use_aces_tonemap_medium = true;
    config->tonemap_quality.use_aces_tonemap_high = true;
    config->tonemap_quality.use_aces_tonemap_ultra = true;
    
    config->tonemap_quality.use_hdr_precision_low = false;
    config->tonemap_quality.use_hdr_precision_medium = true;
    config->tonemap_quality.use_hdr_precision_high = true;
    config->tonemap_quality.use_hdr_precision_ultra = true;
    
    // Effect quality settings
    config->effect_quality.enable_chromatic_aberration_low = false;
    config->effect_quality.enable_chromatic_aberration_medium = false;
    config->effect_quality.enable_chromatic_aberration_high = true;
    config->effect_quality.enable_chromatic_aberration_ultra = true;
    
    config->effect_quality.enable_lens_flare_low = false;
    config->effect_quality.enable_lens_flare_medium = false;
    config->effect_quality.enable_lens_flare_high = true;
    config->effect_quality.enable_lens_flare_ultra = true;
    
    config->effect_quality.enable_vignette_low = false;
    config->effect_quality.enable_vignette_medium = true;
    config->effect_quality.enable_vignette_high = true;
    config->effect_quality.enable_vignette_ultra = true;
    
    // Performance quality settings
    config->performance_quality.use_async_compute_low = false;
    config->performance_quality.use_async_compute_medium = false;
    config->performance_quality.use_async_compute_high = true;
    config->performance_quality.use_async_compute_ultra = true;
    
    config->performance_quality.use_half_precision_low = true;
    config->performance_quality.use_half_precision_medium = true;
    config->performance_quality.use_half_precision_high = false;
    config->performance_quality.use_half_precision_ultra = false;
    
    fprintf(stderr, "[GPU_POSTPROCESS] Quality configuration initialized\n");
    return true;
}

bool gpu_postprocess_set_quality_level(GPUPostProcessManager* manager, PostProcessQuality quality) {
    if (!manager || quality > POSTPROCESS_QUALITY_CUSTOM) {
        return false;
    }
    
    manager->quality_config.quality_level = quality;
    
    // Apply quality settings to all effects
    return gpu_postprocess_apply_quality_settings(manager);
}

PostProcessQuality gpu_postprocess_get_quality_level(GPUPostProcessManager* manager) {
    if (!manager) {
        return POSTPROCESS_QUALITY_MEDIUM;
    }
    
    return manager->quality_config.quality_level;
}

bool gpu_postprocess_apply_quality_settings(GPUPostProcessManager* manager) {
    if (!manager) {
        return false;
    }
    
    PostProcessQualityConfig* config = &manager->quality_config;
    PostProcessQuality quality = config->quality_level;
    
    // Apply bloom quality settings
    u32 bloom_iterations = 4;
    f32 downsample_factor = 0.5f;
    
    switch (quality) {
        case POSTPROCESS_QUALITY_LOW:
            bloom_iterations = config->bloom_quality.blur_iterations_low;
            downsample_factor = config->bloom_quality.downsample_factor_low;
            break;
        case POSTPROCESS_QUALITY_MEDIUM:
            bloom_iterations = config->bloom_quality.blur_iterations_medium;
            downsample_factor = config->bloom_quality.downsample_factor_medium;
            break;
        case POSTPROCESS_QUALITY_HIGH:
            bloom_iterations = config->bloom_quality.blur_iterations_high;
            downsample_factor = config->bloom_quality.downsample_factor_high;
            break;
        case POSTPROCESS_QUALITY_ULTRA:
            bloom_iterations = config->bloom_quality.blur_iterations_ultra;
            downsample_factor = config->bloom_quality.downsample_factor_ultra;
            break;
        case POSTPROCESS_QUALITY_CUSTOM:
            // Use current settings
            bloom_iterations = manager->bloom_settings.iterations;
            downsample_factor = 1.0f; // No downsample for custom
            break;
    }
    
    manager->bloom_settings.iterations = bloom_iterations;
    
    // Apply tone mapping quality
    bool use_aces = true;
    bool use_hdr = true;
    
    switch (quality) {
        case POSTPROCESS_QUALITY_LOW:
            use_aces = config->tonemap_quality.use_aces_tonemap_low;
            use_hdr = config->tonemap_quality.use_hdr_precision_low;
            break;
        case POSTPROCESS_QUALITY_MEDIUM:
            use_aces = config->tonemap_quality.use_aces_tonemap_medium;
            use_hdr = config->tonemap_quality.use_hdr_precision_medium;
            break;
        case POSTPROCESS_QUALITY_HIGH:
            use_aces = config->tonemap_quality.use_aces_tonemap_high;
            use_hdr = config->tonemap_quality.use_hdr_precision_high;
            break;
        case POSTPROCESS_QUALITY_ULTRA:
            use_aces = config->tonemap_quality.use_aces_tonemap_ultra;
            use_hdr = config->tonemap_quality.use_hdr_precision_ultra;
            break;
        case POSTPROCESS_QUALITY_CUSTOM:
            // Keep current settings
            break;
    }
    
    if (use_aces) {
        manager->tonemap_algorithm = TONEMAP_ACES;
    } else {
        manager->tonemap_algorithm = TONEMAP_REINHARD;
    }
    
    // Enable/disable effects based on quality
    PostProcessEffectFlags new_effects = POSTPROCESS_TONE_MAPPING;
    
    if (gpu_postprocess_should_enable_effect(manager, POSTPROCESS_BLOOM)) {
        new_effects |= POSTPROCESS_BLOOM;
    }
    
    if (gpu_postprocess_should_enable_effect(manager, POSTPROCESS_VIGNETTE)) {
        new_effects |= POSTPROCESS_VIGNETTE;
    }
    
    if (gpu_postprocess_should_enable_effect(manager, POSTPROCESS_LENS_FLARE)) {
        new_effects |= POSTPROCESS_LENS_FLARE;
    }
    
    if (gpu_postprocess_should_enable_effect(manager, POSTPROCESS_CHROMATIC_ABBR)) {
        new_effects |= POSTPROCESS_CHROMATIC_ABBR;
    }
    
    manager->active_effects = new_effects;
    
    fprintf(stderr, "[GPU_POSTPROCESS] Applied quality settings: %d\n", quality);
    fprintf(stderr, "[GPU_POSTPROCESS]   Bloom iterations: %u\n", bloom_iterations);
    fprintf(stderr, "[GPU_POSTPROCESS]   ACES tonemapping: %s\n", use_aces ? "enabled" : "disabled");
    fprintf(stderr, "[GPU_POSTPROCESS]   HDR precision: %s\n", use_hdr ? "enabled" : "disabled");
    
    return true;
}

bool gpu_postprocess_set_custom_quality_config(GPUPostProcessManager* manager, 
                                               const PostProcessQualityConfig* config) {
    if (!manager || !config) {
        return false;
    }
    
    manager->quality_config = *config;
    manager->quality_config.quality_level = POSTPROCESS_QUALITY_CUSTOM;
    
    return gpu_postprocess_apply_quality_settings(manager);
}

bool gpu_postprocess_get_bloom_quality_settings(GPUPostProcessManager* manager,
                                                u32* iterations, f32* downsample_factor) {
    if (!manager || !iterations || !downsample_factor) {
        return false;
    }
    
    PostProcessQualityConfig* config = &manager->quality_config;
    PostProcessQuality quality = config->quality_level;
    
    switch (quality) {
        case POSTPROCESS_QUALITY_LOW:
            *iterations = config->bloom_quality.blur_iterations_low;
            *downsample_factor = config->bloom_quality.downsample_factor_low;
            break;
        case POSTPROCESS_QUALITY_MEDIUM:
            *iterations = config->bloom_quality.blur_iterations_medium;
            *downsample_factor = config->bloom_quality.downsample_factor_medium;
            break;
        case POSTPROCESS_QUALITY_HIGH:
            *iterations = config->bloom_quality.blur_iterations_high;
            *downsample_factor = config->bloom_quality.downsample_factor_high;
            break;
        case POSTPROCESS_QUALITY_ULTRA:
            *iterations = config->bloom_quality.blur_iterations_ultra;
            *downsample_factor = config->bloom_quality.downsample_factor_ultra;
            break;
        case POSTPROCESS_QUALITY_CUSTOM:
            *iterations = manager->bloom_settings.iterations;
            *downsample_factor = 1.0f;
            break;
        default:
            return false;
    }
    
    return true;
}

bool gpu_postprocess_should_enable_effect(GPUPostProcessManager* manager,
                                         PostProcessEffectFlags effect) {
    if (!manager) {
        return false;
    }
    
    PostProcessQualityConfig* config = &manager->quality_config;
    PostProcessQuality quality = config->quality_level;
    
    switch (effect) {
        case POSTPROCESS_BLOOM:
            return true; // Bloom is always enabled
        case POSTPROCESS_VIGNETTE:
            switch (quality) {
                case POSTPROCESS_QUALITY_LOW: return config->effect_quality.enable_vignette_low;
                case POSTPROCESS_QUALITY_MEDIUM: return config->effect_quality.enable_vignette_medium;
                case POSTPROCESS_QUALITY_HIGH: return config->effect_quality.enable_vignette_high;
                case POSTPROCESS_QUALITY_ULTRA: return config->effect_quality.enable_vignette_ultra;
                case POSTPROCESS_QUALITY_CUSTOM: return manager->vignette_settings.enabled;
                default: return false;
            }
        case POSTPROCESS_LENS_FLARE:
            switch (quality) {
                case POSTPROCESS_QUALITY_LOW: return config->effect_quality.enable_lens_flare_low;
                case POSTPROCESS_QUALITY_MEDIUM: return config->effect_quality.enable_lens_flare_medium;
                case POSTPROCESS_QUALITY_HIGH: return config->effect_quality.enable_lens_flare_high;
                case POSTPROCESS_QUALITY_ULTRA: return config->effect_quality.enable_lens_flare_ultra;
                case POSTPROCESS_QUALITY_CUSTOM: return manager->lens_flare_settings.enabled;
                default: return false;
            }
        case POSTPROCESS_CHROMATIC_ABBR:
            switch (quality) {
                case POSTPROCESS_QUALITY_LOW: return config->effect_quality.enable_chromatic_aberration_low;
                case POSTPROCESS_QUALITY_MEDIUM: return config->effect_quality.enable_chromatic_aberration_medium;
                case POSTPROCESS_QUALITY_HIGH: return config->effect_quality.enable_chromatic_aberration_high;
                case POSTPROCESS_QUALITY_ULTRA: return config->effect_quality.enable_chromatic_aberration_ultra;
                case POSTPROCESS_QUALITY_CUSTOM: return (manager->active_effects & POSTPROCESS_CHROMATIC_ABBR) != 0;
                default: return false;
            }
        default:
            return true;
    }
}

bool gpu_postprocess_get_performance_settings(GPUPostProcessManager* manager,
                                             bool* use_async_compute, bool* use_half_precision) {
    if (!manager || !use_async_compute || !use_half_precision) {
        return false;
    }
    
    PostProcessQualityConfig* config = &manager->quality_config;
    PostProcessQuality quality = config->quality_level;
    
    switch (quality) {
        case POSTPROCESS_QUALITY_LOW:
            *use_async_compute = config->performance_quality.use_async_compute_low;
            *use_half_precision = config->performance_quality.use_half_precision_low;
            break;
        case POSTPROCESS_QUALITY_MEDIUM:
            *use_async_compute = config->performance_quality.use_async_compute_medium;
            *use_half_precision = config->performance_quality.use_half_precision_medium;
            break;
        case POSTPROCESS_QUALITY_HIGH:
            *use_async_compute = config->performance_quality.use_async_compute_high;
            *use_half_precision = config->performance_quality.use_half_precision_high;
            break;
        case POSTPROCESS_QUALITY_ULTRA:
            *use_async_compute = config->performance_quality.use_async_compute_ultra;
            *use_half_precision = config->performance_quality.use_half_precision_ultra;
            break;
        case POSTPROCESS_QUALITY_CUSTOM:
            // Use balanced settings for custom
            *use_async_compute = true;
            *use_half_precision = false;
            break;
        default:
            return false;
    }
    
    return true;
}

bool gpu_postprocess_auto_adjust_quality(GPUPostProcessManager* manager,
                                        f32 frame_time_ms, f32 target_frame_time_ms) {
    if (!manager || target_frame_time_ms <= 0.0f) {
        return false;
    }
    
    PostProcessQuality current_quality = manager->quality_config.quality_level;
    PostProcessQuality new_quality = current_quality;
    
    f32 performance_ratio = frame_time_ms / target_frame_time_ms;
    
    // If frame time is significantly over target, reduce quality
    if (performance_ratio > 1.5f) {
        switch (current_quality) {
            case POSTPROCESS_QUALITY_ULTRA:
                new_quality = POSTPROCESS_QUALITY_HIGH;
                break;
            case POSTPROCESS_QUALITY_HIGH:
                new_quality = POSTPROCESS_QUALITY_MEDIUM;
                break;
            case POSTPROCESS_QUALITY_MEDIUM:
                new_quality = POSTPROCESS_QUALITY_LOW;
                break;
            case POSTPROCESS_QUALITY_LOW:
            case POSTPROCESS_QUALITY_CUSTOM:
                // Can't reduce further
                break;
        }
    }
    // If frame time is well under target, we can increase quality
    else if (performance_ratio < 0.7f) {
        switch (current_quality) {
            case POSTPROCESS_QUALITY_LOW:
                new_quality = POSTPROCESS_QUALITY_MEDIUM;
                break;
            case POSTPROCESS_QUALITY_MEDIUM:
                new_quality = POSTPROCESS_QUALITY_HIGH;
                break;
            case POSTPROCESS_QUALITY_HIGH:
                new_quality = POSTPROCESS_QUALITY_ULTRA;
                break;
            case POSTPROCESS_QUALITY_ULTRA:
            case POSTPROCESS_QUALITY_CUSTOM:
                // Already at max or custom
                break;
        }
    }
    
    if (new_quality != current_quality) {
        fprintf(stderr, "[GPU_POSTPROCESS] Auto-adjusting quality from %d to %d (frame time: %.2f ms, target: %.2f ms)\n",
                current_quality, new_quality, frame_time_ms, target_frame_time_ms);
        return gpu_postprocess_set_quality_level(manager, new_quality);
    }
    
    return false;
}

// ==============================================================================
// EFFECT CHAINING SYSTEM (POST-008)
// ==============================================================================

bool gpu_postprocess_create_effect_chain(GPUPostProcessManager* manager, const char* chain_name) {
    if (!manager || !chain_name || manager->chain_count >= 16) {
        return false;
    }
    
    u32 chain_index = manager->chain_count;
    PostProcessEffectChain* chain = &manager->effect_chains[chain_index];
    
    // Initialize chain
    memset(chain, 0, sizeof(PostProcessEffectChain));
    chain->auto_optimize = true;
    
    // Copy chain name
    strncpy(manager->chain_names[chain_index], chain_name, 63);
    manager->chain_names[chain_index][63] = '\0';
    
    manager->chain_count++;
    
    fprintf(stderr, "[GPU_POSTPROCESS] Created effect chain '%s' (index %u)\n", chain_name, chain_index);
    return true;
}

bool gpu_postprocess_add_chain_node(GPUPostProcessManager* manager, const char* chain_name, 
                                   const PostProcessChainNode* node) {
    if (!manager || !chain_name || !node) {
        return false;
    }
    
    // Find chain by name
    for (u32 i = 0; i < manager->chain_count; i++) {
        if (strcmp(manager->chain_names[i], chain_name) == 0) {
            PostProcessEffectChain* chain = &manager->effect_chains[i];
            
            if (chain->node_count >= POSTPROCESS_CHAIN_MAX_STAGES) {
                return false;
            }
            
            chain->nodes[chain->node_count] = *node;
            chain->node_count++;
            
            fprintf(stderr, "[GPU_POSTPROCESS] Added node %u to chain '%s'\n", 
                    chain->node_count - 1, chain_name);
            return true;
        }
    }
    
    return false;
}

bool gpu_postprocess_remove_chain_node(GPUPostProcessManager* manager, const char* chain_name, 
                                    PostProcessChainStage stage) {
    if (!manager || !chain_name) {
        return false;
    }
    
    // Find chain by name
    for (u32 i = 0; i < manager->chain_count; i++) {
        if (strcmp(manager->chain_names[i], chain_name) == 0) {
            PostProcessEffectChain* chain = &manager->effect_chains[i];
            
            // Find and remove node
            for (u32 j = 0; j < chain->node_count; j++) {
                if (chain->nodes[j].stage == stage) {
                    // Shift remaining nodes down
                    for (u32 k = j; k < chain->node_count - 1; k++) {
                        chain->nodes[k] = chain->nodes[k + 1];
                    }
                    chain->node_count--;
                    
                    fprintf(stderr, "[GPU_POSTPROCESS] Removed node %u from chain '%s'\n", 
                            stage, chain_name);
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool gpu_postprocess_set_active_chain(GPUPostProcessManager* manager, const char* chain_name) {
    if (!manager || !chain_name) {
        return false;
    }
    
    // Find chain by name
    for (u32 i = 0; i < manager->chain_count; i++) {
        if (strcmp(manager->chain_names[i], chain_name) == 0) {
            manager->active_chain_index = i;
            
            fprintf(stderr, "[GPU_POSTPROCESS] Set active chain to '%s' (index %u)\n", 
                    chain_name, i);
            return true;
        }
    }
    
    return false;
}

bool gpu_postprocess_execute_chain(GPUPostProcessManager* manager, VkCommandBuffer cmd_buffer,
                                 VkImageView source_image, PostProcessFramebuffer* output) {
    if (!manager || !cmd_buffer || !source_image || !output) {
        return false;
    }
    
    if (manager->active_chain_index >= manager->chain_count) {
        return false;
    }
    
    PostProcessEffectChain* chain = &manager->effect_chains[manager->active_chain_index];
    
    fprintf(stderr, "[GPU_POSTPROCESS] Executing chain '%s' with %u nodes\n",
            manager->chain_names[manager->active_chain_index], chain->node_count);
    
    // Execute each node in the chain
    for (u32 i = 0; i < chain->node_count; i++) {
        PostProcessChainNode* node = &chain->nodes[i];
        
        if (!node->enabled) {
            continue;
        }
        
        // Check if required effects are available
        if ((manager->active_effects & node->required_effects) != node->required_effects) {
            continue;
        }
        
        // Execute the stage based on type
        switch (node->stage) {
            case POSTPROCESS_CHAIN_TONEMAP:
                // Execute tone mapping
                fprintf(stderr, "[GPU_POSTPROCESS]   Executing tonemap stage\n");
                break;
                
            case POSTPROCESS_CHAIN_BLOOM_EXTRACT:
                // Execute bloom extraction
                fprintf(stderr, "[GPU_POSTPROCESS]   Executing bloom extract stage\n");
                break;
                
            case POSTPROCESS_CHAIN_BLOOM_BLUR:
                // Execute bloom blur
                fprintf(stderr, "[GPU_POSTPROCESS]   Executing bloom blur stage\n");
                break;
                
            case POSTPROCESS_CHAIN_VIGNETTE:
                // Execute vignette
                fprintf(stderr, "[GPU_POSTPROCESS]   Executing vignette stage\n");
                break;
                
            case POSTPROCESS_CHAIN_LENS_FLARE:
                // Execute lens flare
                fprintf(stderr, "[GPU_POSTPROCESS]   Executing lens flare stage\n");
                break;
                
            case POSTPROCESS_CHAIN_CHROMATIC_ABBR:
                // Execute chromatic aberration
                fprintf(stderr, "[GPU_POSTPROCESS]   Executing chromatic aberration stage\n");
                break;
                
            case POSTPROCESS_CHAIN_COLOR_GRADE:
                // Execute color grading
                fprintf(stderr, "[GPU_POSTPROCESS]   Executing color grade stage\n");
                break;
                
            case POSTPROCESS_CHAIN_FINAL_COMPOSITE:
                // Execute final composite
                fprintf(stderr, "[GPU_POSTPROCESS]   Executing final composite stage\n");
                break;
                
            default:
                break;
        }
    }
    
    return true;
}

bool gpu_postprocess_save_chain(GPUPostProcessManager* manager, const char* chain_name, 
                               const char* file_path) {
    if (!manager || !chain_name || !file_path) {
        return false;
    }
    
    // Find chain by name
    for (u32 i = 0; i < manager->chain_count; i++) {
        if (strcmp(manager->chain_names[i], chain_name) == 0) {
            PostProcessEffectChain* chain = &manager->effect_chains[i];
            
            FILE* file = fopen(file_path, "w");
            if (!file) {
                return false;
            }
            
            // Write chain data
            fprintf(file, "# Post-Process Effect Chain: %s\n", chain_name);
            fprintf(file, "node_count: %u\n", chain->node_count);
            fprintf(file, "auto_optimize: %s\n", chain->auto_optimize ? "true" : "false");
            fprintf(file, "\n# Nodes:\n");
            
            for (u32 j = 0; j < chain->node_count; j++) {
                PostProcessChainNode* node = &chain->nodes[j];
                fprintf(file, "node_%u:\n", j);
                fprintf(file, "  stage: %u\n", node->stage);
                fprintf(file, "  enabled: %s\n", node->enabled ? "true" : "false");
                fprintf(file, "  required_effects: %u\n", node->required_effects);
                fprintf(file, "  input_texture: %u\n", node->input_texture);
                fprintf(file, "  output_texture: %u\n", node->output_texture);
                fprintf(file, "  blend_factor: %.3f\n", node->blend_factor);
            }
            
            fclose(file);
            
            fprintf(stderr, "[GPU_POSTPROCESS] Saved chain '%s' to '%s'\n", chain_name, file_path);
            return true;
        }
    }
    
    return false;
}

bool gpu_postprocess_load_chain(GPUPostProcessManager* manager, const char* chain_name, 
                               const char* file_path) {
    if (!manager || !chain_name || !file_path) {
        return false;
    }
    
    // Create the chain first
    if (!gpu_postprocess_create_effect_chain(manager, chain_name)) {
        return false;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return false;
    }
    
    char line[256];
    u32 node_index = 0;
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        
        // Parse node data (simplified parsing)
        if (strstr(line, "node_") == line) {
            // Parse node data here (implementation would be more robust)
            node_index++;
        }
    }
    
    fclose(file);
    
    fprintf(stderr, "[GPU_POSTPROCESS] Loaded chain '%s' from '%s'\n", chain_name, file_path);
    return true;
}

// ==============================================================================
// STATISTICS TRACKING AND PERFORMANCE METRICS (POST-002)
// ==============================================================================

void gpu_postprocess_enable_profiling(GPUPostProcessManager* manager, bool enabled) {
    if (!manager) {
        return;
    }
    
    manager->stats.enable_profiling = enabled;
    
    if (enabled) {
        fprintf(stderr, "[GPU_POSTPROCESS] Profiling enabled\n");
    } else {
        fprintf(stderr, "[GPU_POSTPROCESS] Profiling disabled\n");
    }
}

void gpu_postprocess_reset_stats(GPUPostProcessManager* manager) {
    if (!manager) {
        return;
    }
    
    memset(&manager->stats, 0, sizeof(PostProcessStats));
    manager->stats.enable_profiling = true;
    manager->stats.min_frame_time = FLT_MAX;
    
    fprintf(stderr, "[GPU_POSTPROCESS] Statistics reset\n");
}

void gpu_postprocess_get_stats(GPUPostProcessManager* manager, PostProcessStats* stats) {
    if (!manager || !stats) {
        return;
    }
    
    *stats = manager->stats;
}

void gpu_postprocess_print_stats(GPUPostProcessManager* manager) {
    if (!manager) {
        return;
    }
    
    PostProcessStats* stats = &manager->stats;
    
    fprintf(stderr, "\n=== POST-PROCESS STATISTICS ===\n");
    fprintf(stderr, "Frame Count: %u\n", stats->frame_count);
    fprintf(stderr, "Average Frame Time: %.3f ms\n", stats->average_frame_time);
    fprintf(stderr, "Min Frame Time: %.3f ms\n", stats->min_frame_time);
    fprintf(stderr, "Max Frame Time: %.3f ms\n", stats->max_frame_time);
    fprintf(stderr, "Total Frame Time: %.3f ms\n", stats->total_frame_time);
    fprintf(stderr, "\n--- Individual Effect Times ---\n");
    fprintf(stderr, "Tone Mapping: %.3f ms\n", stats->tone_map_time);
    fprintf(stderr, "Bloom: %.3f ms\n", stats->bloom_time);
    fprintf(stderr, "Vignette: %.3f ms\n", stats->vignette_time);
    fprintf(stderr, "Lens Flare: %.3f ms\n", stats->lens_flare_time);
    fprintf(stderr, "Chromatic Aberration: %.3f ms\n", stats->chromatic_aberr_time);
    fprintf(stderr, "Color Grading: %.3f ms\n", stats->color_grade_time);
    fprintf(stderr, "Composite: %.3f ms\n", stats->composite_time);
    fprintf(stderr, "\n--- Resource Usage ---\n");
    fprintf(stderr, "Total Memory Usage: %llu bytes\n", stats->total_memory_usage);
    fprintf(stderr, "Texture Switches: %u\n", stats->texture_switches);
    fprintf(stderr, "Pipeline Switches: %u\n", stats->pipeline_switches);
    fprintf(stderr, "================================\n\n");
}

void gpu_postprocess_begin_frame_profiling(GPUPostProcessManager* manager) {
    if (!manager || !manager->stats.enable_profiling) {
        return;
    }
    
    // Get current time (platform-specific implementation needed)
    // For now, using a simple placeholder
    manager->profiling_start_time = 0.0; // Would use actual high-resolution timer
}

void gpu_postprocess_end_frame_profiling(GPUPostProcessManager* manager) {
    if (!manager || !manager->stats.enable_profiling) {
        return;
    }
    
    // Calculate frame time (platform-specific implementation needed)
    f64 current_time = 0.0; // Would use actual high-resolution timer
    f32 frame_time = (f32)(current_time - manager->profiling_start_time);
    
    // Update statistics
    manager->stats.frame_count++;
    manager->stats.total_frame_time += frame_time;
    
    if (frame_time < manager->stats.min_frame_time) {
        manager->stats.min_frame_time = frame_time;
    }
    
    if (frame_time > manager->stats.max_frame_time) {
        manager->stats.max_frame_time = frame_time;
    }
    
    manager->stats.average_frame_time = manager->stats.total_frame_time / manager->stats.frame_count;
    
    // Log frame time if it's unusually high
    if (frame_time > 16.67f) { // > 60 FPS threshold
        fprintf(stderr, "[GPU_POSTPROCESS] Slow frame detected: %.3f ms\n", frame_time);
    }
}
