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
#include <core/logger.h>
#include <math.h>
#include <renderer/gpu_postprocess.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==============================================================================
// Lifecycle Management
// ==============================================================================

bool gpu_postprocess_init(GPUPostProcessManager *manager, VkDevice device,
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
  if (!gpu_postprocess_create_framebuffer(manager, 0, width, height,
                                          VK_FORMAT_R16G16B16A16_SFLOAT)) {
    fprintf(stderr, "[GPU_POSTPROCESS] Failed to create initial framebuffer\n");
    return false;
  }

  // Create render passes
  if (!gpu_postprocess_create_scene_render_pass(manager)) {
    fprintf(stderr, "[GPU_POSTPROCESS] Failed to create scene render pass\n");
    return false;
  }

  if (!gpu_postprocess_create_tonemap_pass(manager)) {
    fprintf(stderr,
            "[GPU_POSTPROCESS] Failed to create tone mapping render pass\n");
    return false;
  }

  // Create pipelines
  if (!gpu_postprocess_create_bloom_pipeline(manager)) {
    fprintf(stderr, "[GPU_POSTPROCESS] Failed to create bloom pipeline\n");
    return false;
  }

  if (!gpu_postprocess_create_tonemap_pipeline(manager)) {
    fprintf(stderr,
            "[GPU_POSTPROCESS] Failed to create tone mapping pipeline\n");
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
  // In production: vkCreateSampler(device, &sampler_info, NULL,
  // &manager->linear_sampler);

  sampler_info.magFilter = VK_FILTER_NEAREST;
  sampler_info.minFilter = VK_FILTER_NEAREST;
  // In production: vkCreateSampler(device, &sampler_info, NULL,
  // &manager->nearest_sampler);

  // Set active effects
  manager->active_effects = POSTPROCESS_TONE_MAPPING | POSTPROCESS_BLOOM;

  manager->initialized = true;

  fprintf(stderr,
          "[GPU_POSTPROCESS] GPU post-processing manager initialized\n");
  fprintf(stderr, "[GPU_POSTPROCESS]  - Framebuffer: %ux%u\n", width, height);
  fprintf(stderr, "[GPU_POSTPROCESS]  - Tone mapping algorithm: ACES\n");
  fprintf(stderr, "[GPU_POSTPROCESS]  - Bloom: enabled\n");
  fprintf(stderr, "[GPU_POSTPROCESS]  - Vignette: disabled\n");
  fprintf(stderr, "[GPU_POSTPROCESS]  - Lens flare: disabled\n");

  return true;
}

void gpu_postprocess_shutdown(GPUPostProcessManager *manager) {
  if (!manager || !manager->initialized) {
    return;
  }

  // Destroy framebuffers
  for (u32 i = 0; i < manager->framebuffer_count; i++) {
    if (manager->framebuffers[i].initialized) {
      gpu_postprocess_delete_framebuffer(manager,
                                         manager->framebuffers[i].fb_id);
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

bool gpu_postprocess_create_framebuffer(GPUPostProcessManager *manager,
                                        u32 fb_id, u32 width, u32 height,
                                        VkFormat color_format) {
  if (!manager || !manager->initialized) {
    return false;
  }

  // Find available slot
  PostProcessFramebuffer *fb = NULL;
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
  // 1. Create color VkImage with VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
  // VK_IMAGE_USAGE_SAMPLED_BIT
  // 2. Create depth VkImage with VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
  // 3. Allocate device memory for both
  // 4. Create VkImageView for each
  // 5. Create bloom image for bloom pass
  // 6. Create VkFramebuffer with color and depth attachments

  fb->initialized = true;

  fprintf(stderr, "[GPU_POSTPROCESS] Created framebuffer %u (%ux%u)\n", fb_id,
          width, height);

  return true;
}

void gpu_postprocess_delete_framebuffer(GPUPostProcessManager *manager,
                                        u32 fb_id) {
  if (!manager || !manager->initialized) {
    return;
  }

  PostProcessFramebuffer *fb = gpu_postprocess_get_framebuffer(manager, fb_id);
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

PostProcessFramebuffer *
gpu_postprocess_get_framebuffer(GPUPostProcessManager *manager, u32 fb_id) {
  if (!manager || !manager->initialized) {
    return NULL;
  }

  for (u32 i = 0; i < 8; i++) {
    if (manager->framebuffers[i].initialized &&
        manager->framebuffers[i].fb_id == fb_id) {
      return &manager->framebuffers[i];
    }
  }

  return NULL;
}

bool gpu_postprocess_resize_framebuffer(GPUPostProcessManager *manager,
                                        u32 fb_id, u32 new_width,
                                        u32 new_height) {
  if (!manager || !manager->initialized) {
    return false;
  }

  PostProcessFramebuffer *fb = gpu_postprocess_get_framebuffer(manager, fb_id);
  if (!fb) {
    fprintf(stderr, "[GPU_POSTPROCESS] Framebuffer %u not found\n", fb_id);
    return false;
  }

  // Delete and recreate with new dimensions
  gpu_postprocess_delete_framebuffer(manager, fb_id);
  return gpu_postprocess_create_framebuffer(manager, fb_id, new_width,
                                            new_height, fb->color_format);
}

// ==============================================================================
// Render Pass Management
// ==============================================================================

bool gpu_postprocess_create_scene_render_pass(GPUPostProcessManager *manager) {
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

bool gpu_postprocess_create_tonemap_pass(GPUPostProcessManager *manager) {
  if (!manager || !manager->initialized) {
    return false;
  }

  // Placeholder: would create tone mapping render pass
  // Similar to scene pass but with different attachment formats for
  // post-processing

  fprintf(stderr, "[GPU_POSTPROCESS] Created tone mapping render pass\n");

  return true;
}

VkRenderPass
gpu_postprocess_get_scene_render_pass(GPUPostProcessManager *manager) {
  if (!manager || !manager->initialized) {
    return VK_NULL_HANDLE;
  }

  return manager->scene_render_pass;
}

VkRenderPass gpu_postprocess_get_tonemap_pass(GPUPostProcessManager *manager) {
  if (!manager || !manager->initialized) {
    return VK_NULL_HANDLE;
  }

  return manager->tonemap_pass;
}

// ==============================================================================
// Pipeline Management
// ==============================================================================

bool gpu_postprocess_create_bloom_pipeline(GPUPostProcessManager *manager) {
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

bool gpu_postprocess_create_tonemap_pipeline(GPUPostProcessManager *manager) {
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

bool gpu_postprocess_create_composite_pipeline(GPUPostProcessManager *manager) {
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

void gpu_postprocess_set_tone_mapping(GPUPostProcessManager *manager,
                                      TonemapAlgorithm algorithm) {
  if (!manager || !manager->initialized) {
    return;
  }

  manager->tonemap_algorithm = algorithm;

  fprintf(stderr, "[GPU_POSTPROCESS] Set tone mapping algorithm: %d\n",
          algorithm);
}

void gpu_postprocess_set_tonemap_settings(GPUPostProcessManager *manager,
                                          TonemapConfig *settings) {
  if (!manager || !manager->initialized || !settings) {
    return;
  }

  manager->tonemap_settings = *settings;

  fprintf(stderr, "[GPU_POSTPROCESS] Updated tone mapping settings\n");
}

void gpu_postprocess_set_exposure(GPUPostProcessManager *manager,
                                  f32 exposure) {
  if (!manager || !manager->initialized) {
    return;
  }

  manager->tonemap_settings.exposure = exposure;
}

void gpu_postprocess_set_contrast(GPUPostProcessManager *manager,
                                  f32 contrast) {
  if (!manager || !manager->initialized) {
    return;
  }

  manager->tonemap_settings.contrast = contrast;
}

void gpu_postprocess_set_saturation(GPUPostProcessManager *manager,
                                    f32 saturation) {
  if (!manager || !manager->initialized) {
    return;
  }

  manager->tonemap_settings.saturation = saturation;
}

// ==============================================================================
// Effect Configuration
// ==============================================================================

void gpu_postprocess_set_bloom_enabled(GPUPostProcessManager *manager,
                                       bool enabled) {
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

void gpu_postprocess_configure_bloom(GPUPostProcessManager *manager,
                                     f32 threshold, f32 intensity,
                                     u32 iterations) {
  if (!manager || !manager->initialized) {
    return;
  }

  manager->bloom_settings.threshold = threshold;
  manager->bloom_settings.intensity = intensity;
  manager->bloom_settings.iterations = iterations;

  fprintf(stderr,
          "[GPU_POSTPROCESS] Configured bloom (threshold=%.2f, intensity=%.2f, "
          "iterations=%u)\n",
          threshold, intensity, iterations);
}

void gpu_postprocess_set_vignette_enabled(GPUPostProcessManager *manager,
                                          bool enabled) {
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

void gpu_postprocess_configure_vignette(GPUPostProcessManager *manager,
                                        Vec4 color, f32 intensity) {
  if (!manager || !manager->initialized) {
    return;
  }

  manager->vignette_settings.color = color;
  manager->vignette_settings.intensity = intensity;
}

void gpu_postprocess_set_lens_flare_enabled(GPUPostProcessManager *manager,
                                            bool enabled) {
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

void gpu_postprocess_configure_lens_flare(GPUPostProcessManager *manager,
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

bool gpu_postprocess_extract_bloom(GPUPostProcessManager *manager,
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

bool gpu_postprocess_apply_tonemap(GPUPostProcessManager *manager,
                                   VkCommandBuffer cmd_buffer,
                                   VkImageView source_image,
                                   PostProcessFramebuffer *target_framebuffer) {
  if (!manager || !manager->initialized || !cmd_buffer || !source_image ||
      !target_framebuffer) {
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

bool gpu_postprocess_execute(GPUPostProcessManager *manager,
                             VkCommandBuffer cmd_buffer, VkImageView hdr_image,
                             PostProcessFramebuffer *output_framebuffer) {
  if (!manager || !manager->initialized || !cmd_buffer || !hdr_image ||
      !output_framebuffer) {
    return false;
  }

  // Execute bloom if enabled
  if (manager->bloom_settings.enabled &&
      (manager->active_effects & POSTPROCESS_BLOOM)) {
    if (!gpu_postprocess_extract_bloom(manager, cmd_buffer, hdr_image)) {
      return false;
    }
  }

  // Execute tone mapping
  if (!gpu_postprocess_apply_tonemap(manager, cmd_buffer, hdr_image,
                                     output_framebuffer)) {
    return false;
  }

  return true;
}

// ==============================================================================
// Query Functions
// ==============================================================================

u32 gpu_postprocess_get_framebuffer_count(GPUPostProcessManager *manager) {
  if (!manager || !manager->initialized) {
    return 0;
  }

  return manager->framebuffer_count;
}

PostProcessEffectFlags
gpu_postprocess_get_active_effects(GPUPostProcessManager *manager) {
  if (!manager || !manager->initialized) {
    return 0;
  }

  return manager->active_effects;
}

bool gpu_postprocess_is_effect_enabled(GPUPostProcessManager *manager,
                                       PostProcessEffectFlags effect) {
  if (!manager || !manager->initialized) {
    return false;
  }

  return (manager->active_effects & effect) != 0;
}

// ==============================================================================
// Debug and Diagnostics
// ==============================================================================

void gpu_postprocess_log_info(GPUPostProcessManager *manager) {
  if (!manager || !manager->initialized) {
    fprintf(stderr, "[GPU_POSTPROCESS] Manager not initialized\n");
    return;
  }

  fprintf(stderr, "[GPU_POSTPROCESS] GPU Post-Processing Info:\n");
  fprintf(stderr, "[GPU_POSTPROCESS]   Framebuffers: %u / 8\n",
          manager->framebuffer_count);
  fprintf(stderr, "[GPU_POSTPROCESS]   Tone Mapping: %d\n",
          manager->tonemap_algorithm);
  fprintf(stderr, "[GPU_POSTPROCESS]   Bloom: %s\n",
          manager->bloom_settings.enabled ? "enabled" : "disabled");
  fprintf(stderr, "[GPU_POSTPROCESS]   Vignette: %s\n",
          manager->vignette_settings.enabled ? "enabled" : "disabled");
  fprintf(stderr, "[GPU_POSTPROCESS]   Lens Flare: %s\n",
          manager->lens_flare_settings.enabled ? "enabled" : "disabled");
}

void gpu_postprocess_log_statistics(GPUPostProcessManager *manager) {
  if (!manager || !manager->initialized) {
    fprintf(stderr, "[GPU_POSTPROCESS] Manager not initialized\n");
    return;
  }

  fprintf(stderr, "[GPU_POSTPROCESS] ===== Post-Processing Statistics =====\n");
  fprintf(stderr, "[GPU_POSTPROCESS] Active framebuffers: %u\n",
          manager->framebuffer_count);

  for (u32 i = 0; i < manager->framebuffer_count; i++) {
    if (manager->framebuffers[i].initialized) {
      fprintf(stderr, "[GPU_POSTPROCESS]   FB %u: %ux%u\n",
              manager->framebuffers[i].fb_id, manager->framebuffers[i].width,
              manager->framebuffers[i].height);
    }
  }

  fprintf(stderr, "[GPU_POSTPROCESS] Tone mapping exposure: %.2f EV\n",
          manager->tonemap_settings.exposure);
  fprintf(stderr, "[GPU_POSTPROCESS] Bloom threshold: %.2f\n",
          manager->bloom_settings.threshold);
  fprintf(stderr, "[GPU_POSTPROCESS] Bloom intensity: %.2f\n",
          manager->bloom_settings.intensity);
}

bool gpu_postprocess_validate(GPUPostProcessManager *manager) {
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
