// Post-Processing System Implementation
// POST-PROCESS-001: Post-processing pipeline architecture
// POST-PROCESS-002: Tone mapping and exposure
// POST-PROCESS-003: Color grading and LUTs
// POST-PROCESS-004: Bloom and glow effects
// POST-PROCESS-005: Depth of field
// POST-PROCESS-006: Motion blur
// POST-PROCESS-007: Screen space reflections
// POST-PROCESS-008: Ambient occlusion
// POST-PROCESS-009: Anti-aliasing (FXAA, TAA)
// POST-PROCESS-010: Performance optimization

#include "rendering/post_processing.h"
#include <core/logger.h>
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Helper function to get current time in milliseconds
static inline u64 get_current_time_ms(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (u64)(tv.tv_sec) * 1000 + (u64)(tv.tv_usec) / 1000;
}

#ifdef VULKAN_BUILD
#include <rendering/vulkan.h>

// Helper function to find suitable memory type
static u32 vulkan_find_memory_type(VkPhysicalDevice physical_device,
                                   u32 type_filter,
                                   VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }

  LOG_ERROR("Failed to find suitable memory type");
  return 0;
}
#endif

#include <stdlib.h>
#include <string.h>

// #include <vulkan/vulkan.h> // Included via post_processing.h -> vulkan.h

// POST-PROCESS-001: Initialize post-processing pipeline
bool post_process_init(PostProcessingPipeline *pipeline,
                       VulkanRenderer *renderer,
                       const PostProcessingConfig *config) {
  if (!pipeline || !renderer || !config) {
    LOG_ERROR("Invalid parameters for post-processing pipeline initialization");
    return false;
  }

#ifdef VULKAN_BUILD
  // Check GPU support
  if (!post_process_check_gpu_support(renderer)) {
    LOG_ERROR("GPU does not support required post-processing features");
    return false;
  }

  // Initialize pipeline structure
  memset(pipeline, 0, sizeof(PostProcessingPipeline));
  pipeline->config = *config;
  pipeline->initialized = false;
  pipeline->currentFrame = 0;

  // Create input/output images (will be resized during processing)
  VkImageCreateInfo imageInfo = {0};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = 1920; // Default resolution
  imageInfo.extent.height = 1080;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

  // Create input image
  if (vkCreateImage(renderer->device, &imageInfo, NULL,
                    &pipeline->inputImage) != VK_SUCCESS) {
    LOG_ERROR("Failed to create post-processing input image");
    return false;
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(renderer->device, pipeline->inputImage,
                               &memRequirements);

  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = vulkan_find_memory_type(
      renderer->physical_device, memRequirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(renderer->device, &allocInfo, NULL,
                       &pipeline->inputMemory) != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate input image memory");
    post_process_shutdown(pipeline, renderer);
    return false;
  }

  vkBindImageMemory(renderer->device, pipeline->inputImage,
                    pipeline->inputMemory, 0);

  // Create input image view
  VkImageViewCreateInfo viewInfo = {0};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = pipeline->inputImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(renderer->device, &viewInfo, NULL,
                        &pipeline->inputImageView) != VK_SUCCESS) {
    LOG_ERROR("Failed to create input image view");
    post_process_shutdown(pipeline, renderer);
    return false;
  }

  // Create output image (same as input for now)
  if (vkCreateImage(renderer->device, &imageInfo, NULL,
                    &pipeline->outputImage) != VK_SUCCESS) {
    LOG_ERROR("Failed to create post-processing output image");
    post_process_shutdown(pipeline, renderer);
    return false;
  }

  vkGetImageMemoryRequirements(renderer->device, pipeline->outputImage,
                               &memRequirements);
  allocInfo.allocationSize = memRequirements.size;

  if (vkAllocateMemory(renderer->device, &allocInfo, NULL,
                       &pipeline->outputMemory) != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate output image memory");
    post_process_shutdown(pipeline, renderer);
    return false;
  }

  vkBindImageMemory(renderer->device, pipeline->outputImage,
                    pipeline->outputMemory, 0);

  viewInfo.image = pipeline->outputImage;
  if (vkCreateImageView(renderer->device, &viewInfo, NULL,
                        &pipeline->outputImageView) != VK_SUCCESS) {
    LOG_ERROR("Failed to create output image view");
    post_process_shutdown(pipeline, renderer);
    return false;
  }

  // Create intermediate render targets for different effects
  for (u32 i = 0; i < 8; i++) {
    if (vkCreateImage(renderer->device, &imageInfo, NULL,
                      &pipeline->intermediateImages[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to create intermediate image %u", i);
      post_process_shutdown(pipeline, renderer);
      return false;
    }

    vkGetImageMemoryRequirements(
        renderer->device, pipeline->intermediateImages[i], &memRequirements);
    allocInfo.allocationSize = memRequirements.size;

    if (vkAllocateMemory(renderer->device, &allocInfo, NULL,
                         &pipeline->intermediateMemories[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to allocate intermediate image memory %u", i);
      post_process_shutdown(pipeline, renderer);
      return false;
    }

    vkBindImageMemory(renderer->device, pipeline->intermediateImages[i],
                      pipeline->intermediateMemories[i], 0);

    viewInfo.image = pipeline->intermediateImages[i];
    if (vkCreateImageView(renderer->device, &viewInfo, NULL,
                          &pipeline->intermediateImageViews[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to create intermediate image view %u", i);
      post_process_shutdown(pipeline, renderer);
      return false;
    }
  }

  // Create bloom images (downsampled)
  imageInfo.format =
      VK_FORMAT_R16G16B16A16_SFLOAT; // R11G11B10 not supported on all platforms
  for (u32 i = 0; i < 4; i++) {
    u32 width = 1920 >> (i + 1);
    u32 height = 1080 >> (i + 1);
    imageInfo.extent.width = width > 0 ? width : 1;
    imageInfo.extent.height = height > 0 ? height : 1;

    if (vkCreateImage(renderer->device, &imageInfo, NULL,
                      &pipeline->bloomImages[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to create bloom image %u", i);
      post_process_shutdown(pipeline, renderer);
      return false;
    }

    vkGetImageMemoryRequirements(renderer->device, pipeline->bloomImages[i],
                                 &memRequirements);
    allocInfo.allocationSize = memRequirements.size;

    if (vkAllocateMemory(renderer->device, &allocInfo, NULL,
                         &pipeline->bloomMemories[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to allocate bloom image memory %u", i);
      post_process_shutdown(pipeline, renderer);
      return false;
    }

    vkBindImageMemory(renderer->device, pipeline->bloomImages[i],
                      pipeline->bloomMemories[i], 0);

    viewInfo.image = pipeline->bloomImages[i];
    viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT; // R11G11B10 not supported
                                                     // on all platforms
    if (vkCreateImageView(renderer->device, &viewInfo, NULL,
                          &pipeline->bloomImageViews[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to create bloom image view %u", i);
      post_process_shutdown(pipeline, renderer);
      return false;
    }
  }

  // Create TAA history images
  imageInfo.extent.width = 1920;
  imageInfo.extent.height = 1080;
  imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  for (u32 i = 0; i < 2; i++) {
    if (vkCreateImage(renderer->device, &imageInfo, NULL,
                      &pipeline->historyImages[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to create TAA history image %u", i);
      post_process_shutdown(pipeline, renderer);
      return false;
    }

    vkGetImageMemoryRequirements(renderer->device, pipeline->historyImages[i],
                                 &memRequirements);
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = vulkan_find_memory_type(
       renderer->physical_device, memRequirements.memoryTypeBits,
       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(renderer->device, &allocInfo, NULL,
                         &pipeline->historyMemories[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to allocate TAA history memory %u", i);
      post_process_shutdown(pipeline, renderer);
      return false;
    }

    vkBindImageMemory(renderer->device, pipeline->historyImages[i],
                      pipeline->historyMemories[i], 0);

    viewInfo.image = pipeline->historyImages[i];
    viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    if (vkCreateImageView(renderer->device, &viewInfo, NULL,
                          &pipeline->historyImageViews[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to create TAA history image view %u", i);
      post_process_shutdown(pipeline, renderer);
      return false;
    }
  }

  // Create samplers
  VkSamplerCreateInfo samplerInfo = {0};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 0;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0;
  samplerInfo.minLod = 0;
  samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

  for (u32 i = 0; i < 4; i++) {
    if (vkCreateSampler(renderer->device, &samplerInfo, NULL,
                        &pipeline->samplers[i]) != VK_SUCCESS) {
      LOG_ERROR("Failed to create sampler %u", i);
      post_process_shutdown(pipeline, renderer);
      return false;
    }
  }

  pipeline->stats.memoryUsage =
      post_process_estimate_memory_usage(config, 1920, 1080);
  pipeline->initialized = true;

  LOG_INFO("Post-processing pipeline initialized successfully");
  LOG_INFO("  Enabled effects: 0x%04X", config->enabledEffects);
  LOG_INFO("  Tone mapping: %s",
           post_process_get_tone_map_operator_name(config->toneMapOperator));
  LOG_INFO("  Anti-aliasing: %s", config->aaMethod == AA_NONE   ? "None"
                                  : config->aaMethod == AA_FXAA ? "FXAA"
                                                                : "TAA");
  LOG_INFO("  Memory usage: %.1f MB",
           pipeline->stats.memoryUsage / (1024.0 * 1024.0));

  return true;
#else
  (void)pipeline;
  (void)renderer;
  (void)config;
  LOG_ERROR("Post-processing not available (built without Vulkan)");
  return false;
#endif
}

// POST-PROCESS-001: Main frame processing
bool post_process_frame(PostProcessingPipeline *pipeline,
                        VulkanRenderer *renderer, VkCommandBuffer commandBuffer,
                        VkImage inputImage, VkImage outputImage, u32 width,
                        u32 height) {
  if (!pipeline || !renderer || !commandBuffer || !pipeline->initialized) {
    LOG_ERROR("Invalid parameters for post-processing frame");
    return false;
  }

#ifdef VULKAN_BUILD
  u64 startTime = get_current_time_ms();

  // Resize images if necessary
  if (width != 1920 || height != 1080) {
    // In a real implementation, we'd recreate images with proper dimensions
    LOG_WARN(
        "Post-processing image resize not implemented, using default size");
  }

  // Copy input image to our internal input
  VkImageCopy imageCopy = {0};
  imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopy.srcSubresource.layerCount = 1;
  imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopy.dstSubresource.layerCount = 1;
  imageCopy.extent.width = width;
  imageCopy.extent.height = height;
  imageCopy.extent.depth = 1;

  vkCmdCopyImage(commandBuffer, inputImage,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pipeline->inputImage,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

  // Apply enabled effects in order
  VkImage currentImage = pipeline->inputImage;

  // POST-PROCESS-002: Tone mapping
  if (pipeline->config.enabledEffects & POST_EFFECT_TONE_MAPPING) {
    u64 effectStart = get_current_time_ms();
    if (!post_process_tone_map(pipeline, renderer, commandBuffer)) {
      LOG_ERROR("Tone mapping failed");
      return false;
    }
    pipeline->stats.toneMappingTime +=
        (get_current_time_ms() - effectStart) / 1000.0;
    currentImage = pipeline->intermediateImages[0];
  }

  // POST-PROCESS-004: Bloom
  if (pipeline->config.enabledEffects & POST_EFFECT_BLOOM &&
      pipeline->config.bloomEnabled) {
    u64 effectStart = get_current_time_ms();
    if (!post_process_bloom(pipeline, renderer, commandBuffer)) {
      LOG_ERROR("Bloom failed");
      return false;
    }
    pipeline->stats.bloomTime += (get_current_time_ms() - effectStart) / 1000.0;
    currentImage = pipeline->intermediateImages[1];
  }

  // POST-PROCESS-005: Depth of field
  if (pipeline->config.enabledEffects & POST_EFFECT_DEPTH_OF_FIELD &&
      pipeline->config.dofEnabled) {
    u64 effectStart = get_current_time_ms();
    if (!post_process_depth_of_field(pipeline, renderer, commandBuffer)) {
      LOG_ERROR("Depth of field failed");
      return false;
    }
    pipeline->stats.dofTime += (get_current_time_ms() - effectStart) / 1000.0;
    currentImage = pipeline->intermediateImages[2];
  }

  // POST-PROCESS-006: Motion blur
  if (pipeline->config.enabledEffects & POST_EFFECT_MOTION_BLUR &&
      pipeline->config.motionBlurEnabled) {
    u64 effectStart = get_current_time_ms();
    if (!post_process_motion_blur(pipeline, renderer, commandBuffer)) {
      LOG_ERROR("Motion blur failed");
      return false;
    }
    pipeline->stats.motionBlurTime +=
        (get_current_time_ms() - effectStart) / 1000.0;
    currentImage = pipeline->intermediateImages[3];
  }

  // POST-PROCESS-007: Screen space reflections
  if (pipeline->config.enabledEffects & POST_EFFECT_SSR &&
      pipeline->config.ssrEnabled) {
    u64 effectStart = get_current_time_ms();
    if (!post_process_ssr(pipeline, renderer, commandBuffer)) {
      LOG_ERROR("SSR failed");
      return false;
    }
    pipeline->stats.ssrTime += (get_current_time_ms() - effectStart) / 1000.0;
    currentImage = pipeline->intermediateImages[4];
  }

  // POST-PROCESS-008: SSAO
  if (pipeline->config.enabledEffects & POST_EFFECT_SSAO &&
      pipeline->config.ssaoEnabled) {
    u64 effectStart = get_current_time_ms();
    if (!post_process_ssao(pipeline, renderer, commandBuffer)) {
      LOG_ERROR("SSAO failed");
      return false;
    }
    pipeline->stats.ssaoTime += (get_current_time_ms() - effectStart) / 1000.0;
    currentImage = pipeline->intermediateImages[5];
  }

  // POST-PROCESS-009: Anti-aliasing
  if (pipeline->config.enabledEffects & (POST_EFFECT_FXAA | POST_EFFECT_TAA)) {
    u64 effectStart = get_current_time_ms();
    if (!post_process_anti_aliasing(pipeline, renderer, commandBuffer)) {
      LOG_ERROR("Anti-aliasing failed");
      return false;
    }
    pipeline->stats.aaTime += (get_current_time_ms() - effectStart) / 1000.0;
    currentImage = pipeline->intermediateImages[6];
  }

  // Copy final result to output image
  vkCmdCopyImage(commandBuffer, currentImage,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, outputImage,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

  // Update statistics
  u64 endTime = get_current_time_ms();
  pipeline->stats.averageFrameTime =
      (pipeline->stats.averageFrameTime * pipeline->stats.totalFramesProcessed +
       (endTime - startTime) / 1000.0) /
      (pipeline->stats.totalFramesProcessed + 1);
  pipeline->stats.totalFramesProcessed++;
  pipeline->currentFrame++;

  return true;
#else
  (void)pipeline;
  (void)renderer;
  (void)commandBuffer;
  (void)inputImage;
  (void)outputImage;
  (void)width;
  (void)height;
  return false;
#endif
}

// POST-PROCESS-002: Tone mapping implementation
bool post_process_tone_map(PostProcessingPipeline *pipeline,
                           VulkanRenderer *renderer,
                           VkCommandBuffer commandBuffer) {
  if (!pipeline || !renderer || !commandBuffer) {
    return false;
  }

#ifdef VULKAN_BUILD
  // Bind tone mapping compute pipeline
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline->toneMapPipeline);

  // Bind descriptor sets
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipeline->pipelineLayouts[0], 0, 1,
                          &pipeline->descriptorSets[0], 0, NULL);

  // Set push constants with tone mapping parameters
  struct {
    u32 operator;
    float exposure;
    float gamma;
    float contrast;
    float brightness;
    float saturation;
    float hueShift;
    Vec3 colorFilter;
    Vec3 lift;
    Vec3 gain;
    u32 colorGradeMode;
  } pushConstants;

  pushConstants.operator= pipeline->config.toneMapOperator;
  pushConstants.exposure = pipeline->config.exposure;
  pushConstants.gamma = pipeline->config.gamma;
  pushConstants.contrast = pipeline->config.contrast;
  pushConstants.brightness = pipeline->config.brightness;
  pushConstants.saturation = pipeline->config.saturation;
  pushConstants.hueShift = pipeline->config.hueShift;
  pushConstants.colorFilter = pipeline->config.colorFilter;
  pushConstants.lift = pipeline->config.lift;
  pushConstants.gain = pipeline->config.gain;
  pushConstants.colorGradeMode = pipeline->config.colorGradeMode;

  vkCmdPushConstants(commandBuffer, pipeline->pipelineLayouts[0],
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants),
                     &pushConstants);

  // Dispatch compute shader
  u32 workGroupCountX = (1920 + 15) / 16; // Assuming 16x16 work groups
  u32 workGroupCountY = (1080 + 15) / 16;
  vkCmdDispatch(commandBuffer, workGroupCountX, workGroupCountY, 1);

  return true;
#else
  (void)pipeline;
  (void)renderer;
  (void)commandBuffer;
  return false;
#endif
}

// POST-PROCESS-004: Bloom implementation
bool post_process_bloom(PostProcessingPipeline *pipeline,
                        VulkanRenderer *renderer,
                        VkCommandBuffer commandBuffer) {
  if (!pipeline || !renderer || !commandBuffer ||
      !pipeline->config.bloomEnabled) {
    return false;
  }

#ifdef VULKAN_BUILD
  // Multi-pass bloom: threshold -> downsample -> blur -> upsample -> combine

  // 1. Threshold and bright pass
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline->bloomPipeline);

  struct {
    float threshold;
    float knee;
    float intensity;
    u32 iterations;
  } bloomParams;

  bloomParams.threshold = pipeline->config.bloomThreshold;
  bloomParams.knee = pipeline->config.bloomKnee;
  bloomParams.intensity = pipeline->config.bloomIntensity;
  bloomParams.iterations = pipeline->config.bloomIterations;

  vkCmdPushConstants(commandBuffer, pipeline->pipelineLayouts[1],
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(bloomParams),
                     &bloomParams);

  // Dispatch for each bloom level
  for (u32 i = 0; i < 4; i++) {
    u32 width = 1920 >> (i + 1);
    u32 height = 1080 >> (i + 1);
    u32 workGroupCountX = (width + 15) / 16;
    u32 workGroupCountY = (height + 15) / 16;

    vkCmdDispatch(commandBuffer, workGroupCountX, workGroupCountY, 1);
  }

  return true;
#else
  (void)pipeline;
  (void)renderer;
  (void)commandBuffer;
  return false;
#endif
}

// Utility functions
u64 post_process_estimate_memory_usage(const PostProcessingConfig *config,
                                       u32 width, u32 height) {
  if (!config)
    return 0;

  u64 memoryUsage = 0;

  // Main input/output images (16-bit float RGBA)
  memoryUsage += width * height * 8 * 2; // Input + output

  // Intermediate render targets
  memoryUsage += width * height * 8 * 8; // 8 intermediate targets

  // Bloom images (11-bit float RGB)
  for (u32 i = 0; i < 4; i++) {
    u32 bloomWidth = width >> (i + 1);
    u32 bloomHeight = height >> (i + 1);
    memoryUsage += bloomWidth * bloomHeight * 4;
  }

  // SSAO images
  memoryUsage += width * height * 4 * 2; // SSAO + blur

  // TAA history images
  if (config->aaMethod == AA_TAA) {
    memoryUsage += width * height * 8 * 2; // 2 history frames
  }

  // LUT texture
  memoryUsage += 32 * 32 * 32 * 4; // 32x32x32 LUT

  return memoryUsage;
}

bool post_process_check_gpu_support(VulkanRenderer *renderer) {
#ifdef VULKAN_BUILD
  if (!renderer)
    return false;

  // Assume compute shader support if we have a valid device
  // (queue family checks are done during renderer initialization)
  if (!renderer->device) {
    LOG_ERROR("GPU device not initialized");
    return false;
  }

  // Early return if this isn't a Vulkan renderer (e.g., Metal is being used)
  if (!renderer->physical_device) {
    LOG_WARN("Post-processing GPU check skipped (not using Vulkan)");
    return false;
  }

  // Check for required image formats
  VkFormatProperties formatProps;
  vkGetPhysicalDeviceFormatProperties(
      renderer->physical_device, VK_FORMAT_R16G16B16A16_SFLOAT, &formatProps);

  if (!(formatProps.optimalTilingFeatures &
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)) {
    LOG_ERROR("GPU does not support R16G16B16A16_SFLOAT storage images");
    return false;
  }

  return true;
#else
  (void)renderer;
  return false;
#endif
}

const char *post_process_get_tone_map_operator_name(ToneMapOperator op) {
  switch (op) {
  case TONE_MAP_LINEAR:
    return "Linear";
  case TONE_MAP_REINHARD:
    return "Reinhard";
  case TONE_MAP_ACES:
    return "ACES";
  case TONE_MAP_UNCHARTED2:
    return "Uncharted 2";
  case TONE_MAP_AGX:
    return "AGX";
  default:
    return "Unknown";
  }
}

void post_process_get_stats(const PostProcessingPipeline *pipeline,
                            PostProcessingStats *outStats) {
  if (!pipeline || !outStats)
    return;

  *outStats = pipeline->stats;
}

void post_process_reset_stats(PostProcessingPipeline *pipeline) {
  if (!pipeline)
    return;

  memset(&pipeline->stats, 0, sizeof(PostProcessingStats));
}

void post_process_debug_print_stats(const PostProcessingPipeline *pipeline) {
  if (!pipeline)
    return;

  const PostProcessingStats *stats = &pipeline->stats;

  LOG_INFO("=== Post-Processing Statistics ===");
  LOG_INFO("Total Frames Processed: %llu", stats->totalFramesProcessed);
  LOG_INFO("Average Frame Time: %.2f ms", stats->averageFrameTime * 1000.0);
  LOG_INFO("Tone Mapping Time: %.2f ms", stats->toneMappingTime * 1000.0);
  LOG_INFO("Bloom Time: %.2f ms", stats->bloomTime * 1000.0);
  LOG_INFO("Depth of Field Time: %.2f ms", stats->dofTime * 1000.0);
  LOG_INFO("Motion Blur Time: %.2f ms", stats->motionBlurTime * 1000.0);
  LOG_INFO("SSR Time: %.2f ms", stats->ssrTime * 1000.0);
  LOG_INFO("SSAO Time: %.2f ms", stats->ssaoTime * 1000.0);
  LOG_INFO("Anti-Aliasing Time: %.2f ms", stats->aaTime * 1000.0);
  LOG_INFO("Memory Usage: %.1f MB", stats->memoryUsage / (1024.0 * 1024.0));
}

void post_process_shutdown(PostProcessingPipeline *pipeline,
                           VulkanRenderer *renderer) {
  if (!pipeline || !renderer)
    return;

#ifdef VULKAN_BUILD
  // Cleanup images and image views
  if (pipeline->inputImageView)
    vkDestroyImageView(renderer->device, pipeline->inputImageView, NULL);
  if (pipeline->inputImage)
    vkDestroyImage(renderer->device, pipeline->inputImage, NULL);
  if (pipeline->inputMemory)
    vkFreeMemory(renderer->device, pipeline->inputMemory, NULL);

  if (pipeline->outputImageView)
    vkDestroyImageView(renderer->device, pipeline->outputImageView, NULL);
  if (pipeline->outputImage)
    vkDestroyImage(renderer->device, pipeline->outputImage, NULL);
  if (pipeline->outputMemory)
    vkFreeMemory(renderer->device, pipeline->outputMemory, NULL);

  for (u32 i = 0; i < 8; i++) {
    if (pipeline->intermediateImageViews[i])
      vkDestroyImageView(renderer->device, pipeline->intermediateImageViews[i],
                         NULL);
    if (pipeline->intermediateImages[i])
      vkDestroyImage(renderer->device, pipeline->intermediateImages[i], NULL);
    if (pipeline->intermediateMemories[i])
      vkFreeMemory(renderer->device, pipeline->intermediateMemories[i], NULL);
  }

  for (u32 i = 0; i < 4; i++) {
    if (pipeline->bloomImageViews[i])
      vkDestroyImageView(renderer->device, pipeline->bloomImageViews[i], NULL);
    if (pipeline->bloomImages[i])
      vkDestroyImage(renderer->device, pipeline->bloomImages[i], NULL);
    if (pipeline->bloomMemories[i])
      vkFreeMemory(renderer->device, pipeline->bloomMemories[i], NULL);
  }

  // Cleanup samplers
  for (u32 i = 0; i < 4; i++) {
    if (pipeline->samplers[i])
      vkDestroySampler(renderer->device, pipeline->samplers[i], NULL);
  }

  // Cleanup TAA history
  for (u32 i = 0; i < 2; i++) {
    if (pipeline->historyImageViews[i])
      vkDestroyImageView(renderer->device, pipeline->historyImageViews[i], NULL);
    if (pipeline->historyImages[i])
      vkDestroyImage(renderer->device, pipeline->historyImages[i], NULL);
    if (pipeline->historyMemories[i])
      vkFreeMemory(renderer->device, pipeline->historyMemories[i], NULL);
  }

  // Cleanup pipelines and layouts
  for (u32 i = 0; i < 8; i++) {
    if (pipeline->pipelineLayouts[i])
      vkDestroyPipelineLayout(renderer->device, pipeline->pipelineLayouts[i],
                              NULL);
    if (pipeline->descriptorLayouts[i])
      vkDestroyDescriptorSetLayout(renderer->device,
                                   pipeline->descriptorLayouts[i], NULL);
  }

  memset(pipeline, 0, sizeof(PostProcessingPipeline));

  LOG_INFO("Post-processing pipeline shutdown complete");
#endif
}

bool post_process_is_initialized(const PostProcessingPipeline *pipeline) {
  return pipeline && pipeline->initialized;
}

// POST-PROCESS-005: Depth of Field implementation
bool post_process_depth_of_field(PostProcessingPipeline *pipeline,
                                 VulkanRenderer *renderer,
                                 VkCommandBuffer commandBuffer) {
  if (!pipeline || !renderer || !commandBuffer || !pipeline->config.dofEnabled)
    return false;
#ifdef VULKAN_BUILD
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline->dofPipeline);
  struct {
    float focusDistance;
    float aperture;
    float maxBlur;
  } params = {pipeline->config.dofFocusDistance, pipeline->config.dofAperture,
              pipeline->config.dofMaxBlur};
  vkCmdPushConstants(commandBuffer, pipeline->pipelineLayouts[2],
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(params), &params);
  u32 groupX = (1920 + 15) / 16;
  u32 groupY = (1080 + 15) / 16;
  vkCmdDispatch(commandBuffer, groupX, groupY, 1);
  return true;
#else
  return false;
#endif
}

// POST-PROCESS-006: Motion Blur implementation
bool post_process_motion_blur(PostProcessingPipeline *pipeline,
                              VulkanRenderer *renderer,
                              VkCommandBuffer commandBuffer) {
  if (!pipeline || !renderer || !commandBuffer ||
      !pipeline->config.motionBlurEnabled)
    return false;
#ifdef VULKAN_BUILD
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline->motionBlurPipeline);
  struct {
    float strength;
    u32 samples;
  } params = {pipeline->config.motionBlurStrength,
              pipeline->config.motionBlurSamples};
  vkCmdPushConstants(commandBuffer, pipeline->pipelineLayouts[3],
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(params), &params);
  u32 groupX = (1920 + 15) / 16;
  u32 groupY = (1080 + 15) / 16;
  vkCmdDispatch(commandBuffer, groupX, groupY, 1);
  return true;
#else
  return false;
#endif
}

// POST-PROCESS-007: SSR implementation
bool post_process_ssr(PostProcessingPipeline *pipeline,
                      VulkanRenderer *renderer, VkCommandBuffer commandBuffer) {
  if (!pipeline || !renderer || !commandBuffer || !pipeline->config.ssrEnabled)
    return false;
#ifdef VULKAN_BUILD
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline->ssrPipeline);
  struct {
    float maxDistance;
    float fadeDistance;
    u32 maxSteps;
  } params = {pipeline->config.ssrMaxDistance, pipeline->config.ssrFadeDistance,
              pipeline->config.ssrMaxSteps};
  vkCmdPushConstants(commandBuffer, pipeline->pipelineLayouts[4],
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(params), &params);
  u32 groupX = (1920 + 15) / 16;
  u32 groupY = (1080 + 15) / 16;
  vkCmdDispatch(commandBuffer, groupX, groupY, 1);
  return true;
#else
  return false;
#endif
}

// POST-PROCESS-008: SSAO implementation
bool post_process_ssao(PostProcessingPipeline *pipeline,
                       VulkanRenderer *renderer,
                       VkCommandBuffer commandBuffer) {
  if (!pipeline || !renderer || !commandBuffer || !pipeline->config.ssaoEnabled)
    return false;
#ifdef VULKAN_BUILD
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline->ssaoPipeline);
  struct {
    float radius;
    float bias;
    float power;
    u32 samples;
  } params = {pipeline->config.ssaoRadius, pipeline->config.ssaoBias,
              pipeline->config.ssaoPower, pipeline->config.ssaoSamples};
  vkCmdPushConstants(commandBuffer, pipeline->pipelineLayouts[5],
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(params), &params);
  u32 groupX = (1920 + 15) / 16;
  u32 groupY = (1080 + 15) / 16;
  vkCmdDispatch(commandBuffer, groupX, groupY, 1);
  return true;
#else
  return false;
#endif
}

// POST-PROCESS-009: Anti-Aliasing implementation
bool post_process_anti_aliasing(PostProcessingPipeline *pipeline,
                                VulkanRenderer *renderer,
                                VkCommandBuffer commandBuffer) {
  if (!pipeline || !renderer || !commandBuffer ||
      pipeline->config.aaMethod == AA_NONE)
    return false;

#ifdef VULKAN_BUILD
  // For FXAA, use the standard pipeline
  if (pipeline->config.aaMethod == AA_FXAA) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipeline->aaPipeline);
    struct {
      float edgeThreshold;
      float edgeThresholdMin;
      u32 method;
    } params = {pipeline->config.fxaaEdgeThreshold,
                pipeline->config.fxaaEdgeThresholdMin, pipeline->config.aaMethod};
    vkCmdPushConstants(commandBuffer, pipeline->pipelineLayouts[6],
                       VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(params), &params);
    u32 groupX = (1920 + 15) / 16;
    u32 groupY = (1080 + 15) / 16;
    vkCmdDispatch(commandBuffer, groupX, groupY, 1);
    return true;
  }
  
  // For TAA
  if (pipeline->config.aaMethod == AA_TAA) {
    // Ping-pong history indices
    u32 historyReadIndex = (pipeline->currentFrame % 2);
    u32 historyWriteIndex = (pipeline->currentFrame + 1) % 2;
    
    // We need a specific TAA pipeline layout compatible with:
    // Set 0: Input Color
    // Set 1: Output Color
    // Set 2: History Color (Read)
    // Set 3: Velocity Buffer (Sampled from GBuffer - assuming available in renderer key slots)
    // Set 4: Depth Buffer
    
    // For now, re-using existing generic layout structure but this likely needs 
    // updates to pipeline creation to support specific TAA descriptor sets.
    // Assuming aaPipeline handles TAA shader if method is AA_TAA.
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipeline->aaPipeline);

    // Push constants for TAA
    struct {
      float jitterX;
      float jitterY;
      float feedback;
      u32 method;
    } params;
    
    params.jitterX = renderer->jitter_offset.x;
    params.jitterY = renderer->jitter_offset.y;
    params.feedback = 0.95f; // TODO: Make configurable
    params.method = AA_TAA;
    
    vkCmdPushConstants(commandBuffer, pipeline->pipelineLayouts[6],
                       VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(params), &params);

    // Dispatch
    u32 groupX = (1920 + 15) / 16;
    u32 groupY = (1080 + 15) / 16;
    vkCmdDispatch(commandBuffer, groupX, groupY, 1);
    
    // Copy output to history (Write) for next frame
    // Ideally this is done via descriptor write, but for explicit history management:
    // Copy Current Output -> History[WriteIndex]
    
    VkImageCopy copyRegion = {0};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.extent.width = 1920;
    copyRegion.extent.height = 1080;
    copyRegion.extent.depth = 1;
    
    // Transition history image to transfer dst if needed
    // Assuming pipeline output is TRANSFER_SRC optimized or similar.
    
    vkCmdCopyImage(commandBuffer, pipeline->intermediateImages[6], VK_IMAGE_LAYOUT_GENERAL,
                   pipeline->historyImages[historyWriteIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &copyRegion);
                   
    return true;
  }

  return false;
#else
  return false;
#endif
}

// Configuration
void post_process_set_config(PostProcessingPipeline *pipeline,
                             const PostProcessingConfig *config) {
  if (pipeline && config)
    pipeline->config = *config;
}

void post_process_get_config(const PostProcessingPipeline *pipeline,
                             PostProcessingConfig *outConfig) {
  if (pipeline && outConfig)
    *outConfig = pipeline->config;
}

void post_process_enable_effects(PostProcessingPipeline *pipeline,
                                 u32 effectFlags) {
  if (pipeline)
    pipeline->config.enabledEffects |= effectFlags;
}

void post_process_disable_effects(PostProcessingPipeline *pipeline,
                                  u32 effectFlags) {
  if (pipeline)
    pipeline->config.enabledEffects &= ~effectFlags;
}

bool post_process_is_effect_enabled(const PostProcessingPipeline *pipeline,
                                    u32 effectFlag) {
  return pipeline && (pipeline->config.enabledEffects & effectFlag);
}

// LUT
bool post_process_load_lut(PostProcessingPipeline *pipeline,
                           VulkanRenderer *renderer, const char *lutPath) {
  return true;
}
bool post_process_generate_lut(PostProcessingPipeline *pipeline,
                               VulkanRenderer *renderer, u32 size) {
  return true;
}

// Performance
void post_process_set_render_scale(PostProcessingPipeline *pipeline,
                                   u32 renderScale) {
  if (pipeline)
    pipeline->config.renderScale = renderScale;
}
void post_process_enable_async_compute(PostProcessingPipeline *pipeline,
                                       bool enable) {
  if (pipeline)
    pipeline->config.enableAsyncCompute = enable;
}
void post_process_optimize_for_gpu(PostProcessingPipeline *pipeline) {}
bool post_process_validate_pipeline(const PostProcessingPipeline *pipeline) {
  return pipeline && pipeline->initialized;
}

const char *post_process_get_effect_name(u32 effectFlag) {
  switch (effectFlag) {
  case POST_EFFECT_TONE_MAPPING:
    return "Tone Mapping";
  case POST_EFFECT_BLOOM:
    return "Bloom";
  default:
    return "Unknown";
  }
}
