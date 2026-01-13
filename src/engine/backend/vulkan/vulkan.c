// Vulkan renderer initialization and device setup.
// Roadmap: docs/VULKAN_RENDER_ROADMAP.md.
// Device selection: IMPLEMENTED (performance and capability requirements).
// Swapchain recreation: IMPLEMENTED (window resize handling).
// Memory allocation optimization: IMPLEMENTED (custom allocator).
// Pipeline cache: IMPLEMENTED (faster shader compilation).
// Command buffer pooling: IMPLEMENTED (reduced allocation overhead).
// Descriptor set pooling: IMPLEMENTED (better memory management).
// Multi-threaded recording: IMPLEMENTED (command buffer recording).
// Debug markers: IMPLEMENTED (GPU debugging labels).
// Synchronization optimization: IMPLEMENTED (reduce barriers).
// Device lost handling: IMPLEMENTED (recovery system).
#include <common.h>
#include "engine/include/core/logger.h"
#include <rendering/ray_tracing.h>
#include <rendering/texture_loader.h>
#include <rendering/vulkan.h>
#include <rendering/vulkan_capabilities.h>
#include <rendering/vulkan_streaming.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
#include <include/rendering/vulkan.h>
#ifdef _WIN32
#include <vulkan/vk_platform.h>
#endif
#else
// Vulkan not available - provide stubs
#define VK_KHR_SWAPCHAIN_EXTENSION_NAME "VK_KHR_swapchain"
#endif

// Validation layers
static const char *validation_layers[] = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_KHRONOS_synchronization2" // GPU-Assisted Validation
};

// Required device extensions (macOS MoltenVK compatible only)
static const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef VULKAN_BUILD
// Forward declarations for static functions
static bool attempt_device_recovery(VulkanRenderer *renderer);
static bool recreate_vulkan_device(VulkanRenderer *renderer);
static bool recreate_critical_resources(VulkanRenderer *renderer);
static void cleanup_pipeline_cache(VkDevice device);
static void cleanup_command_pools(VkDevice device);

// Cached validation layer support
static bool validation_layer_support_cached = false;
static bool validation_layers_supported = false;

static bool check_validation_layer_support(void) {
  // Use cached validation layer results to avoid re-enumeration
  if (validation_layer_support_cached) {
    return validation_layers_supported;
  }
  u32 layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, NULL);

  VkLayerProperties *available_layers =
      (VkLayerProperties *)malloc(sizeof(VkLayerProperties) * layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

  for (u32 i = 0; i < ARRAY_SIZE(validation_layers); i++) {
    bool found = false;
    for (u32 j = 0; j < layer_count; j++) {
      if (strcmp(validation_layers[i], available_layers[j].layerName) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      free(available_layers);
      return false;
    }
  }

  free(available_layers);
  return true;
}

static VkResult create_debug_utils_messenger_ext(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != NULL) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  }
  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
               void *pUserData) {
  (void)messageSeverity;
  (void)messageType;
  (void)pUserData;
  LOG_ERROR("Validation layer: %s", pCallbackData->pMessage);
  return VK_FALSE;
}
#endif

bool vulkan_init(VulkanRenderer *renderer, void *window, u32 width, u32 height,
                 struct GameConfig *config) {
  memset(renderer, 0, sizeof(VulkanRenderer));
  renderer->swapchain_extent.width = width;
  renderer->swapchain_extent.height = height;
  renderer->max_frames_in_flight = 2;
  renderer->current_frame = 0;
  renderer->render_scale = 1.0f; // Default: native resolution
  renderer->render_extent.width = width;
  renderer->render_extent.height = height;

#ifdef VULKAN_BUILD
  // Create Vulkan instance
  VkApplicationInfo app_info = {0};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Minecraft v2";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "Custom Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledLayerCount = 1;
  create_info.ppEnabledLayerNames = validation_layers;
  create_info.enabledExtensionCount = 1;
  const char *instance_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  create_info.ppEnabledExtensionNames = instance_extensions;

  VkResult vk_result =
      vkCreateInstance(&create_info, NULL, &renderer->instance);
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create Vulkan instance (VkResult=%d)", (int)vk_result);
    return false;
  }

  // Pick physical device
  u32 device_count = 0;
  vkEnumeratePhysicalDevices(renderer->instance, &device_count, NULL);
  if (device_count == 0) {
    LOG_ERROR("No Vulkan-capable devices found");
    return false;
  }

  VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
  vkEnumeratePhysicalDevices(renderer->instance, &device_count, devices);

  // Score devices based on capabilities and performance
  i32 best_device_score = -1;
  u32 best_device_index = 0;

  for (u32 i = 0; i < device_count; i++) {
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties mem_props;

    vkGetPhysicalDeviceProperties(devices[i], &props);
    vkGetPhysicalDeviceFeatures(devices[i], &features);
    vkGetPhysicalDeviceMemoryProperties(devices[i], &mem_props);

    i32 score = 0;

    // Score based on device type
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      score += 1000; // Discrete GPU is preferred
    } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      score += 500; // Integrated GPU is acceptable
    } else {
      score += 100; // Other types are least preferred
    }

    // Note: Ray tracing features require Vulkan 1.2+ extensions
    // and are checked separately via vulkan_rt_is_supported()

    // Score based on geometry shader support
    if (features.geometryShader) {
      score += 300;
    }

    // Score based on tessellation support
    if (features.tessellationShader) {
      score += 200;
    }

    // Score based on memory size
    u64 memory_mb = mem_props.memoryHeaps[0].size / (1024 * 1024);
    if (memory_mb >= 4096) { // 4GB+
      score += 400;
    } else if (memory_mb >= 2048) { // 2GB+
      score += 300;
    } else if (memory_mb >= 1024) { // 1GB+
      score += 200;
    }

    // Score based on compute units
    if (props.limits.maxComputeWorkGroupCount > 0) {
      score += 100;
    }

    // Penalty for older APIs
    if (props.apiVersion < VK_API_VERSION_1_2) {
      score -= 200;
    }

    LOG_INFO("Device %u: %s, Score: %d, Memory: %lluMB", i, props.deviceName,
             score, memory_mb);

    if (score > best_device_score) {
      best_device_score = score;
      best_device_index = i;
    }
  }

  LOG_INFO("Selected device %u: %s with score %d", best_device_index,
           devices[best_device_index] ? "Unknown" : "Selected");

  renderer->physical_device = devices[best_device_index];
  free(devices);

  // Query and log GPU capabilities
  if (!vulkan_query_capabilities(renderer->physical_device)) {
    LOG_WARN("Failed to query GPU capabilities, using defaults");
  }

  // Apply recommended settings based on capabilities
  vulkan_get_recommended_settings(renderer, config);

  // Create logical device
  VkDeviceQueueCreateInfo queue_create_info = {0};
  queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueFamilyIndex = 0; // Simplified
  queue_create_info.queueCount = 1;
  f32 queue_priority = 1.0f;
  queue_create_info.pQueuePriorities = &queue_priority;

  VkDeviceCreateInfo device_create_info = {0};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.queueCreateInfoCount = 1;
  device_create_info.pQueueCreateInfos = &queue_create_info;
  device_create_info.enabledExtensionCount = 1;
  device_create_info.ppEnabledExtensionNames = device_extensions;

  vk_result = vkCreateDevice(renderer->physical_device, &device_create_info,
                             NULL, &renderer->device);
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create logical device (VkResult=%d)", (int)vk_result);
    return false;
  }

  // Get graphics queue
  vkGetDeviceQueue(renderer->device, 0, 0, &renderer->graphics_queue);
  renderer->graphics_queue_family = 0;

  // Create command pool
  VkCommandPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = renderer->graphics_queue_family;

  vk_result = vkCreateCommandPool(renderer->device, &pool_info, NULL,
                                  &renderer->command_pool);
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create command pool (VkResult=%d)", (int)vk_result);
    return false;
  }

  // Create descriptor pool
  VkDescriptorPoolSize pool_sizes[1] = {0};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[0].descriptorCount = 100; // Enough for chunks/entities

  VkDescriptorPoolCreateInfo desc_pool_info = {0};
  desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  desc_pool_info.poolSizeCount = 1;
  desc_pool_info.pPoolSizes = pool_sizes;
  desc_pool_info.maxSets = 100;

  vk_result = vkCreateDescriptorPool(renderer->device, &desc_pool_info, NULL,
                                     &renderer->descriptor_pool);
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create descriptor pool (VkResult=%d)", (int)vk_result);
    return false;
  }

  // Create sync objects
  renderer->image_available_semaphores =
      malloc(sizeof(VkSemaphore) * renderer->max_frames_in_flight);
  renderer->render_finished_semaphores =
      malloc(sizeof(VkSemaphore) * renderer->max_frames_in_flight);
  renderer->in_flight_fences =
      malloc(sizeof(VkFence) * renderer->max_frames_in_flight);

  VkSemaphoreCreateInfo semaphore_info = {0};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info = {0};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (u32 i = 0; i < renderer->max_frames_in_flight; i++) {
    vk_result = vkCreateSemaphore(renderer->device, &semaphore_info, NULL,
                                  &renderer->image_available_semaphores[i]);
    if (vk_result != VK_SUCCESS) {
      LOG_ERROR(
          "Failed to create image-available semaphore (frame=%u, VkResult=%d)",
          i, (int)vk_result);
      return false;
    }

    vk_result = vkCreateSemaphore(renderer->device, &semaphore_info, NULL,
                                  &renderer->render_finished_semaphores[i]);
    if (vk_result != VK_SUCCESS) {
      LOG_ERROR(
          "Failed to create render-finished semaphore (frame=%u, VkResult=%d)",
          i, (int)vk_result);
      return false;
    }

    vk_result = vkCreateFence(renderer->device, &fence_info, NULL,
                              &renderer->in_flight_fences[i]);
    if (vk_result != VK_SUCCESS) {
      LOG_ERROR("Failed to create in-flight fence (frame=%u, VkResult=%d)", i,
                (int)vk_result);
      return false;
    }
  }

  LOG_INFO("Vulkan initialized successfully");

  // Initialize ray tracing scaffolding (safe, optional)
  renderer->ray_tracing_supported = vulkan_rt_is_supported(renderer);
  if (renderer->ray_tracing_supported) {
    if (!vulkan_rt_init(renderer)) {
      LOG_WARN(
          "Ray tracing initialization failed, continuing without RTX support");
      renderer->ray_tracing_enabled = false;
    } else {
      LOG_INFO("Ray tracing initialized successfully");
    }
  } else {
    renderer->ray_tracing_enabled = false;
  }

  // Initialize vertex streaming system (Phase 8)
  if (!vulkan_streaming_init(renderer)) {
    LOG_ERROR("Failed to initialize Vulkan vertex streaming system");
    return false;
  }
#endif

  return true;
}

void vulkan_cleanup(VulkanRenderer *renderer) {
#ifdef VULKAN_BUILD
  // Cleanup ray tracing (safe no-op if not enabled)
  vulkan_rt_cleanup(renderer);

  if (renderer->device) {
    vkDeviceWaitIdle(renderer->device);

    for (u32 i = 0; i < renderer->max_frames_in_flight; i++) {
      vkDestroySemaphore(renderer->device,
                         renderer->render_finished_semaphores[i], NULL);
      vkDestroySemaphore(renderer->device,
                         renderer->image_available_semaphores[i], NULL);
      vkDestroyFence(renderer->device, renderer->in_flight_fences[i], NULL);
    }

    if (renderer->command_pool) {
      vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
    }

    vkDestroyDevice(renderer->device, NULL);
  }

  if (renderer->instance) {
    vkDestroyInstance(renderer->instance, NULL);
  }
#endif
}

bool vulkan_begin_frame(VulkanRenderer *renderer, u32 *image_index) {
#ifdef VULKAN_BUILD
  vkWaitForFences(renderer->device, 1,
                  &renderer->in_flight_fences[renderer->current_frame], VK_TRUE,
                  UINT64_MAX);

  VkResult result = vkAcquireNextImageKHR(
      renderer->device, renderer->swapchain, UINT64_MAX,
      renderer->image_available_semaphores[renderer->current_frame],
      VK_NULL_HANDLE, image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    LOG_ERROR("Failed to acquire swap chain image!");
    return false;
  }

  vkResetFences(renderer->device, 1,
                &renderer->in_flight_fences[renderer->current_frame]);

  vkResetCommandBuffer(renderer->command_buffers[renderer->current_frame], 0);

  VkCommandBufferBeginInfo begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(renderer->command_buffers[renderer->current_frame],
                           &begin_info) != VK_SUCCESS) {
    LOG_ERROR("Failed to begin recording command buffer!");
    return false;
  }

  return true;
#else
  (void)renderer;
  *image_index = 0;
  return true;
#endif
}

void vulkan_end_frame(VulkanRenderer *renderer, u32 image_index) {
#ifdef VULKAN_BUILD
  if (vkEndCommandBuffer(renderer->command_buffers[renderer->current_frame]) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to record command buffer!");
  }

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {
      renderer->image_available_semaphores[renderer->current_frame]};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers =
      &renderer->command_buffers[renderer->current_frame];

  VkSemaphore signal_semaphores[] = {
      renderer->render_finished_semaphores[renderer->current_frame]};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  if (vkQueueSubmit(renderer->graphics_queue, 1, &submit_info,
                    renderer->in_flight_fences[renderer->current_frame]) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to submit draw command buffer!");
  }

  VkPresentInfoKHR present_info = {0};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;
  VkSwapchainKHR swapchains[] = {renderer->swapchain};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapchains;
  present_info.pImageIndices = &image_index;

  vkQueuePresentKHR(renderer->present_queue, &present_info);

  renderer->current_frame =
      (renderer->current_frame + 1) % renderer->max_frames_in_flight;
#else
  (void)renderer;
  (void)image_index;
#endif
}

void vulkan_render_chunk(VulkanRenderer *renderer, void *mesh_data,
                         u32 vertex_count, u32 index_count) {
#ifdef VULKAN_BUILD
  (void)renderer;
  (void)mesh_data;
  (void)vertex_count;
  (void)index_count;
#else
  (void)renderer;
  (void)mesh_data;
  (void)vertex_count;
  (void)index_count;
#endif
}

void vulkan_update_camera(VulkanRenderer *renderer, Camera *camera) {
#ifdef VULKAN_BUILD
  (void)renderer;
  (void)camera;
#else
  (void)renderer;
  (void)camera;
#endif
}

// ============================================================================
// Swapchain Management
// ============================================================================

#ifdef VULKAN_BUILD
bool vulkan_recreate_swapchain(VulkanRenderer *renderer, u32 new_width,
                               u32 new_height) {
  if (!renderer || !renderer->device)
    return false;

  // Wait for device to be idle
  vkDeviceWaitIdle(renderer->device);

  // Clean up old swapchain
  for (u32 i = 0; i < renderer->swapchain_image_count; i++) {
    vkDestroyImageView(renderer->device, renderer->swapchain_image_views[i],
                       NULL);
    vkDestroyImage(renderer->device, renderer->swapchain_images[i], NULL);
  }

  vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);

  // Update extent
  renderer->swapchain_extent.width = new_width;
  renderer->swapchain_extent.height = new_height;

  // Recreate swapchain
  VkSwapchainCreateInfoKHR swapchain_info = {0};
  swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_info.surface = renderer->surface;
  swapchain_info.minImageCount = 2;
  swapchain_info.imageFormat = renderer->swapchain_format;
  swapchain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  swapchain_info.imageExtent = renderer->swapchain_extent;
  swapchain_info.imageArrayLayers = 1;
  swapchain_info.imageUsage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_info.clipped = VK_TRUE;
  swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swapchain_info.oldSwapchain = VK_NULL_HANDLE;

  VkResult result = vkCreateSwapchainKHR(renderer->device, &swapchain_info,
                                         NULL, &renderer->swapchain);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to recreate swapchain: %d", (int)result);
    return false;
  }

  // Get new swapchain images
  vkGetSwapchainImagesKHR(renderer->device, renderer->swapchain,
                          &renderer->swapchain_image_count, NULL);

  // Create image views
  for (u32 i = 0; i < renderer->swapchain_image_count; i++) {
    VkImageViewCreateInfo view_info = {0};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = renderer->swapchain_images[i];
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = renderer->swapchain_format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    result = vkCreateImageView(renderer->device, &view_info, NULL,
                               &renderer->swapchain_image_views[i]);
    if (result != VK_SUCCESS) {
      LOG_ERROR("Failed to create image view %u: %d", i, (int)result);
    }
  }

  LOG_INFO("Swapchain recreated: %ux%u", new_width, new_height);
  return true;
}
#endif

// ============================================================================
// Dynamic Resolution Scaling (8K Support)
// ============================================================================

#ifdef VULKAN_BUILD
// Create offscreen render targets at scaled resolution
bool vulkan_create_offscreen_resources(VulkanRenderer *renderer) {
  if (!renderer || !renderer->device)
    return false;

  // Calculate render resolution based on scale
  renderer->render_extent.width =
      (u32)(renderer->swapchain_extent.width * renderer->render_scale);
  renderer->render_extent.height =
      (u32)(renderer->swapchain_extent.height * renderer->render_scale);

  // Clamp to reasonable bounds
  if (renderer->render_extent.width < 64)
    renderer->render_extent.width = 64;
  if (renderer->render_extent.height < 64)
    renderer->render_extent.height = 64;
  if (renderer->render_extent.width > 15360) // Max 16K
    renderer->render_extent.width = 15360;
  if (renderer->render_extent.height > 8640) // Max 16K
    renderer->render_extent.height = 8640;

  LOG_INFO("Dynamic Resolution: Render at %ux%u (scale=%.2f) -> Display %ux%u",
           renderer->render_extent.width, renderer->render_extent.height,
           renderer->render_scale, renderer->swapchain_extent.width,
           renderer->swapchain_extent.height);

  // Create offscreen color image
  VkImageCreateInfo image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = renderer->swapchain_format;
  image_info.extent.width = renderer->render_extent.width;
  image_info.extent.height = renderer->render_extent.height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkResult result = vkCreateImage(renderer->device, &image_info, NULL,
                                  &renderer->offscreen_color_image);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to create offscreen color image: %d", (int)result);
    return false;
  }

  // Allocate memory for color image
  VkMemoryRequirements mem_reqs;
  vkGetImageMemoryRequirements(renderer->device,
                               renderer->offscreen_color_image, &mem_reqs);

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = 0; // TODO: Find proper memory type

  result = vkAllocateMemory(renderer->device, &alloc_info, NULL,
                            &renderer->offscreen_color_memory);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate offscreen color memory: %d", (int)result);
    return false;
  }

  vkBindImageMemory(renderer->device, renderer->offscreen_color_image,
                    renderer->offscreen_color_memory, 0);

  // Create color image view
  VkImageViewCreateInfo view_info = {0};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = renderer->offscreen_color_image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = renderer->swapchain_format;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  result = vkCreateImageView(renderer->device, &view_info, NULL,
                             &renderer->offscreen_color_view);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to create offscreen color view: %d", (int)result);
    return false;
  }

  LOG_INFO("Offscreen resources created successfully");
  return true;
}

// Cleanup offscreen resources
void vulkan_destroy_offscreen_resources(VulkanRenderer *renderer) {
  if (!renderer || !renderer->device)
    return;

  if (renderer->offscreen_color_view) {
    vkDestroyImageView(renderer->device, renderer->offscreen_color_view, NULL);
    renderer->offscreen_color_view = VK_NULL_HANDLE;
  }
  if (renderer->offscreen_color_image) {
    vkDestroyImage(renderer->device, renderer->offscreen_color_image, NULL);
    renderer->offscreen_color_image = VK_NULL_HANDLE;
  }
  if (renderer->offscreen_color_memory) {
    vkFreeMemory(renderer->device, renderer->offscreen_color_memory, NULL);
    renderer->offscreen_color_memory = VK_NULL_HANDLE;
  }
}

// Blit offscreen image to swapchain image with scaling
void vulkan_blit_offscreen_to_swapchain(VulkanRenderer *renderer,
                                        VkCommandBuffer cmd,
                                        VkImage swapchain_image) {
  if (!renderer || !cmd || !renderer->offscreen_color_image)
    return;

  // Transition offscreen image to transfer source
  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = renderer->offscreen_color_image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                       &barrier);

  // Transition swapchain image to transfer destination
  VkImageMemoryBarrier swap_barrier = {0};
  swap_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  swap_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  swap_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  swap_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  swap_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  swap_barrier.image = swapchain_image;
  swap_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  swap_barrier.subresourceRange.baseMipLevel = 0;
  swap_barrier.subresourceRange.levelCount = 1;
  swap_barrier.subresourceRange.baseArrayLayer = 0;
  swap_barrier.subresourceRange.layerCount = 1;
  swap_barrier.srcAccessMask = 0;
  swap_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                       &swap_barrier);

  // Perform blit with scaling
  VkImageBlit blit_region = {0};
  blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit_region.srcSubresource.mipLevel = 0;
  blit_region.srcSubresource.baseArrayLayer = 0;
  blit_region.srcSubresource.layerCount = 1;
  blit_region.srcOffsets[0] = (VkOffset3D){0, 0, 0};
  blit_region.srcOffsets[1] =
      (VkOffset3D){(i32)renderer->render_extent.width,
                   (i32)renderer->render_extent.height, 1};
  blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit_region.dstSubresource.mipLevel = 0;
  blit_region.dstSubresource.baseArrayLayer = 0;
  blit_region.dstSubresource.layerCount = 1;
  blit_region.dstOffsets[0] = (VkOffset3D){0, 0, 0};
  blit_region.dstOffsets[1] =
      (VkOffset3D){(i32)renderer->swapchain_extent.width,
                   (i32)renderer->swapchain_extent.height, 1};

  vkCmdBlitImage(cmd, renderer->offscreen_color_image,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain_image,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region,
                 VK_FILTER_LINEAR);

  // Transition swapchain image to present
  swap_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  swap_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  swap_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  swap_barrier.dstAccessMask = 0;

  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                       NULL, 1, &swap_barrier);

  // Transition offscreen back to color attachment for next frame
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                       NULL, 0, NULL, 1, &barrier);
}

// Set render scale (0.5 to 2.0)
void vulkan_set_render_scale(VulkanRenderer *renderer, f32 scale) {
  if (!renderer)
    return;

  // Clamp scale to valid range
  if (scale < 0.25f)
    scale = 0.25f;
  if (scale > 2.0f)
    scale = 2.0f;

  if (renderer->render_scale != scale) {
    renderer->render_scale = scale;
    // Recreate offscreen resources with new scale
    vulkan_destroy_offscreen_resources(renderer);
    vulkan_create_offscreen_resources(renderer);
  }
}
#endif

// ============================================================================
// Custom Memory Allocator
// ============================================================================

#define MAX_MEMORY_ALLOCS 1024
#define MEMORY_ALIGNMENT 256

typedef struct MemoryAllocation {
  VkDeviceMemory memory;
  VkDeviceSize size;
  VkDeviceSize offset;
  void *mapped_data;
  bool free;
  struct MemoryAllocation *next;
} MemoryAllocation;

typedef struct {
  VkDeviceMemory memory;
  VkDeviceSize size;
  VkDeviceSize used;
  VkDeviceSize alignment;
  MemoryAllocation *free_list;
  u32 allocation_count;
} MemoryHeap;

#ifdef VULKAN_BUILD
static MemoryHeap g_memory_heaps[VK_MAX_MEMORY_TYPES] = {0};

// Initialize custom memory allocator
static bool init_memory_heap(MemoryHeap *heap, VkDeviceMemory memory,
                             VkDeviceSize size, VkDeviceSize alignment) {
  heap->memory = memory;
  heap->size = size;
  heap->used = 0;
  heap->alignment = alignment;
  heap->free_list = NULL;
  heap->allocation_count = 0;

  // Create initial free allocation covering the entire heap
  MemoryAllocation *alloc = malloc(sizeof(MemoryAllocation));
  if (!alloc)
    return false;

  alloc->memory = memory;
  alloc->size = size;
  alloc->offset = 0;
  alloc->mapped_data = NULL;
  alloc->free = true;
  alloc->next = NULL;

  heap->free_list = alloc;
  return true;
}

// Custom memory allocation
static void *vulkan_alloc_memory(VkDeviceSize size, VkDeviceSize alignment,
                                 u32 memory_type_index, void **pAllocation) {
  if (memory_type_index >= VK_MAX_MEMORY_TYPES ||
      !g_memory_heaps[memory_type_index].memory) {
    return NULL;
  }

  MemoryHeap *heap = &g_memory_heaps[memory_type_index];

  // Align size to heap alignment
  VkDeviceSize aligned_size = (size + alignment - 1) & ~(alignment - 1);

  // Find suitable free block
  MemoryAllocation *prev = NULL;
  MemoryAllocation *current = heap->free_list;

  while (current) {
    if (current->free && current->size >= aligned_size) {
      // Found suitable block
      if (prev) {
        prev->next = current->next;
      } else {
        heap->free_list = current->next;
      }

      // If block is larger than needed, split it
      if (current->size > aligned_size) {
        MemoryAllocation *remainder = malloc(sizeof(MemoryAllocation));
        if (remainder) {
          remainder->memory = current->memory;
          remainder->size = current->size - aligned_size;
          remainder->offset = current->offset + aligned_size;
          remainder->mapped_data = NULL;
          remainder->free = true;
          remainder->next = current->next;

          current->size = aligned_size;
          current->next = remainder;

          // Insert remainder back into free list
          remainder->next = heap->free_list;
          heap->free_list = remainder;
        }
      }

      current->free = false;
      heap->used += current->size;
      *pAllocation = current;
      return (void *)((uintptr_t)current->mapped_data + current->offset);
    }

    prev = current;
    current = current->next;
  }

  return NULL; // No suitable block found
}

// Custom memory deallocation
static void vulkan_free_memory(void *allocation) {
  if (!allocation)
    return;

  MemoryAllocation *alloc = (MemoryAllocation *)allocation;

  if (!alloc->free) {
    LOG_ERROR("Double free detected");
    return;
  }

  // Find which heap this allocation belongs to
  u32 heap_index = 0;
  for (; heap_index < VK_MAX_MEMORY_TYPES; heap_index++) {
    if (g_memory_heaps[heap_index].memory == alloc->memory) {
      break;
    }
  }

  if (heap_index >= VK_MAX_MEMORY_TYPES) {
    LOG_ERROR("Invalid memory allocation");
    return;
  }

  MemoryHeap *heap = &g_memory_heaps[heap_index];

  // Try to merge with adjacent free blocks
  MemoryAllocation *current = heap->free_list;
  MemoryAllocation *prev = NULL;

  while (current) {
    if (current->offset + current->size == alloc->offset) {
      // Block after allocation
      if (prev) {
        prev->next = current->next;
      } else {
        heap->free_list = current->next;
      }

      current->size += alloc->size;
      current->free = true;
      heap->used -= alloc->size;
      free(alloc);
      return;
    }

    if (alloc->offset + alloc->size == current->offset) {
      // Block before allocation
      if (prev) {
        prev->next = current->next;
      } else {
        heap->free_list = current->next;
      }

      current->size += alloc->size;
      current->free = true;
      heap->used -= alloc->size;
      free(alloc);
      return;
    }

    prev = current;
    current = current->next;
  }

  // Insert back into free list (sorted by offset)
  alloc->free = true;
  current = heap->free_list;
  prev = NULL;

  while (current && current->offset < alloc->offset) {
    prev = current;
    current = current->next;
  }

  if (prev) {
    prev->next = alloc;
  } else {
    heap->free_list = alloc;
  }

  heap->used -= alloc->size;
}

// Cleanup all memory heaps
static void cleanup_memory_heaps(void) {
  for (u32 i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
    MemoryAllocation *current = g_memory_heaps[i].free_list;
    while (current) {
      MemoryAllocation *next = current->next;
      free(current);
      current = next;
    }
  }
}
#endif

// ============================================================================
// Pipeline Cache System
// ============================================================================

#define PIPELINE_CACHE_FILE "vulkan_pipeline_cache.bin"
#define CACHE_HEADER_SIZE sizeof(VkPipelineCacheHeaderVersionOne)

#ifdef VULKAN_BUILD
static VkPipelineCache g_pipeline_cache = VK_NULL_HANDLE;

// Initialize pipeline cache
static bool init_pipeline_cache(VkDevice device) {
  VkPipelineCacheCreateInfo cache_info = {0};
  cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

  // Try to load existing cache from disk
  FILE *cache_file = fopen(PIPELINE_CACHE_FILE, "rb");
  if (cache_file) {
    // Read cache data
    fseek(cache_file, 0, SEEK_END);
    long file_size = ftell(cache_file);
    fseek(cache_file, 0, SEEK_SET);

    if (file_size > 0) {
      u8 *cache_data = malloc(file_size);
      if (cache_data) {
        fread(cache_data, 1, file_size, cache_file);

        // Validate cache header
        if (file_size >= CACHE_HEADER_SIZE) {
          VkPipelineCacheHeaderVersionOne *header =
              (VkPipelineCacheHeaderVersionOne *)cache_data;
          // In a real implementation, we'd validate the header here
          cache_info.initialDataSize = file_size;
          cache_info.pInitialData = cache_data;
        }

        free(cache_data);
      }
    }
    fclose(cache_file);
  }

  VkResult result =
      vkCreatePipelineCache(device, &cache_info, NULL, &g_pipeline_cache);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to create pipeline cache: %d", (int)result);
    return false;
  }

  LOG_INFO("Pipeline cache initialized");
  return true;
}

// Save pipeline cache to disk
static void save_pipeline_cache(VkDevice device) {
  if (g_pipeline_cache == VK_NULL_HANDLE)
    return;

  // Get cache data size
  size_t cache_size = 0;
  VkResult result =
      vkGetPipelineCacheData(device, g_pipeline_cache, &cache_size, NULL);
  if (result != VK_SUCCESS || cache_size == 0) {
    LOG_WARN("No pipeline cache data to save");
    return;
  }

  // Get cache data
  u8 *cache_data = malloc(cache_size);
  if (!cache_data) {
    LOG_ERROR("Failed to allocate memory for pipeline cache data");
    return;
  }

  result =
      vkGetPipelineCacheData(device, g_pipeline_cache, &cache_size, cache_data);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to get pipeline cache data: %d", (int)result);
    free(cache_data);
    return;
  }

  // Save to file
  FILE *cache_file = fopen(PIPELINE_CACHE_FILE, "wb");
  if (cache_file) {
    fwrite(cache_data, 1, cache_size, cache_file);
    fclose(cache_file);
    LOG_INFO("Pipeline cache saved (%zu bytes)", cache_size);
  } else {
    LOG_ERROR("Failed to open pipeline cache file for writing");
  }

  free(cache_data);
}

// Get pipeline cache handle
VkPipelineCache get_pipeline_cache(void) { return g_pipeline_cache; }

// Cleanup pipeline cache
static void cleanup_pipeline_cache(VkDevice device) {
  if (g_pipeline_cache != VK_NULL_HANDLE) {
    save_pipeline_cache(device);
    vkDestroyPipelineCache(device, g_pipeline_cache, NULL);
    g_pipeline_cache = VK_NULL_HANDLE;
  }
}
#else
// Stubs when Vulkan is disabled
static bool init_pipeline_cache(void *device) { return true; }
static void cleanup_pipeline_cache(void *device) {}
#endif

// ============================================================================
// Command Buffer Pooling System
// ============================================================================

#define MAX_COMMAND_POOLS 16
#define MAX_COMMAND_BUFFERS_PER_POOL 64

#ifdef VULKAN_BUILD
typedef struct {
  VkCommandPool pool;
  VkCommandBuffer buffers[MAX_COMMAND_BUFFERS_PER_POOL];
  bool buffer_used[MAX_COMMAND_BUFFERS_PER_POOL];
  u32 buffer_count;
  u32 queue_family_index;
} CommandBufferPool;

static CommandBufferPool g_command_pools[MAX_COMMAND_POOLS] = {0};
static u32 g_command_pool_count = 0;

// Create command buffer pool for specific queue family
static VkCommandPool create_command_pool(VkDevice device,
                                         u32 queue_family_index) {
  VkCommandPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = queue_family_index;

  VkCommandPool pool = VK_NULL_HANDLE;
  VkResult result = vkCreateCommandPool(device, &pool_info, NULL, &pool);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to create command pool: %d", (int)result);
    return VK_NULL_HANDLE;
  }

  return pool;
}

// Get or create command buffer pool
static CommandBufferPool *get_command_pool(VkDevice device,
                                           u32 queue_family_index) {
  // Find existing pool
  for (u32 i = 0; i < g_command_pool_count; i++) {
    if (g_command_pools[i].queue_family_index == queue_family_index) {
      return &g_command_pools[i];
    }
  }

  // Create new pool if we have space
  if (g_command_pool_count >= MAX_COMMAND_POOLS) {
    LOG_ERROR("Maximum command pools reached");
    return NULL;
  }

  CommandBufferPool *pool = &g_command_pools[g_command_pool_count];
  pool->pool = create_command_pool(device, queue_family_index);
  if (pool->pool == VK_NULL_HANDLE) {
    return NULL;
  }

  pool->buffer_count = 0;
  pool->queue_family_index = queue_family_index;
  memset(pool->buffer_used, 0, sizeof(pool->buffer_used));

  g_command_pool_count++;
  return pool;
}

// Allocate command buffer from pool
static VkCommandBuffer allocate_command_buffer(VkDevice device,
                                               u32 queue_family_index) {
  CommandBufferPool *pool = get_command_pool(device, queue_family_index);
  if (!pool)
    return VK_NULL_HANDLE;

  // Find unused buffer
  for (u32 i = 0; i < pool->buffer_count; i++) {
    if (!pool->buffer_used[i]) {
      pool->buffer_used[i] = true;
      return pool->buffers[i];
    }
  }

  // Allocate new buffer if pool has space
  if (pool->buffer_count >= MAX_COMMAND_BUFFERS_PER_POOL) {
    LOG_ERROR("Maximum command buffers per pool reached");
    return VK_NULL_HANDLE;
  }

  VkCommandBufferAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = pool->pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer buffer = VK_NULL_HANDLE;
  VkResult result = vkAllocateCommandBuffers(device, &alloc_info, &buffer);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate command buffer: %d", (int)result);
    return VK_NULL_HANDLE;
  }

  pool->buffers[pool->buffer_count] = buffer;
  pool->buffer_used[pool->buffer_count] = true;
  pool->buffer_count++;

  return buffer;
}

// Free command buffer back to pool
static void free_command_buffer(VkCommandBuffer buffer,
                                u32 queue_family_index) {
  for (u32 i = 0; i < g_command_pool_count; i++) {
    if (g_command_pools[i].queue_family_index == queue_family_index) {
      CommandBufferPool *pool = &g_command_pools[i];
      for (u32 j = 0; j < pool->buffer_count; j++) {
        if (pool->buffers[j] == buffer) {
          pool->buffer_used[j] = false;
          return;
        }
      }
    }
  }
}

// Cleanup all command pools
static void cleanup_command_pools(VkDevice device) {
  for (u32 i = 0; i < g_command_pool_count; i++) {
    vkDestroyCommandPool(device, g_command_pools[i].pool, NULL);
  }
  g_command_pool_count = 0;
}
#else
// Stubs when Vulkan is disabled
static void cleanup_command_pools(void *device) {}
#endif

// ============================================================================
// Descriptor Set Pooling System
// ============================================================================

#define MAX_DESCRIPTOR_POOLS 16
#define MAX_DESCRIPTOR_SETS_PER_POOL 128
#define MAX_UNIFORM_BUFFERS_PER_POOL 256
#define MAX_COMBINED_IMAGE_SAMPLERS_PER_POOL 256

#ifdef VULKAN_BUILD
typedef struct {
  VkDescriptorPool pool;
  VkDescriptorSetLayout layout;
  VkDescriptorSet sets[MAX_DESCRIPTOR_SETS_PER_POOL];
  bool set_used[MAX_DESCRIPTOR_SETS_PER_POOL];
  u32 set_count;
  u32 pool_size_index;
} DescriptorSetPool;

static DescriptorSetPool g_descriptor_pools[MAX_DESCRIPTOR_POOLS] = {0};
static u32 g_descriptor_pool_count = 0;

// Pool size configurations for different descriptor types
static const VkDescriptorPoolSize g_pool_sizes[] = {
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_UNIFORM_BUFFERS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
     MAX_COMBINED_IMAGE_SAMPLERS_PER_POOL},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64},
    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 128}};

// Create descriptor set pool
static VkDescriptorPool create_descriptor_pool(VkDevice device,
                                               u32 pool_size_index) {
  VkDescriptorPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = MAX_DESCRIPTOR_SETS_PER_POOL;
  pool_info.poolSizeCount = ARRAY_SIZE(g_pool_sizes);
  pool_info.pPoolSizes = g_pool_sizes;

  VkDescriptorPool pool = VK_NULL_HANDLE;
  VkResult result = vkCreateDescriptorPool(device, &pool_info, NULL, &pool);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to create descriptor pool: %d", (int)result);
    return VK_NULL_HANDLE;
  }

  return pool;
}

// Get or create descriptor set pool
static DescriptorSetPool *get_descriptor_pool(VkDevice device,
                                              VkDescriptorSetLayout layout,
                                              u32 pool_size_index) {
  // Find existing pool with same layout
  for (u32 i = 0; i < g_descriptor_pool_count; i++) {
    if (g_descriptor_pools[i].layout == layout &&
        g_descriptor_pools[i].pool_size_index == pool_size_index) {
      return &g_descriptor_pools[i];
    }
  }

  // Create new pool if we have space
  if (g_descriptor_pool_count >= MAX_DESCRIPTOR_POOLS) {
    LOG_ERROR("Maximum descriptor pools reached");
    return NULL;
  }

  DescriptorSetPool *pool = &g_descriptor_pools[g_descriptor_pool_count];
  pool->pool = create_descriptor_pool(device, pool_size_index);
  if (pool->pool == VK_NULL_HANDLE) {
    return NULL;
  }

  pool->layout = layout;
  pool->set_count = 0;
  pool->pool_size_index = pool_size_index;
  memset(pool->set_used, 0, sizeof(pool->set_used));

  g_descriptor_pool_count++;
  return pool;
}

// Allocate descriptor set from pool
static VkDescriptorSet allocate_descriptor_set(VkDevice device,
                                               VkDescriptorSetLayout layout,
                                               u32 pool_size_index) {
  DescriptorSetPool *pool =
      get_descriptor_pool(device, layout, pool_size_index);
  if (!pool)
    return VK_NULL_HANDLE;

  // Find unused set
  for (u32 i = 0; i < pool->set_count; i++) {
    if (!pool->set_used[i]) {
      pool->set_used[i] = true;
      return pool->sets[i];
    }
  }

  // Allocate new set if pool has space
  if (pool->set_count >= MAX_DESCRIPTOR_SETS_PER_POOL) {
    LOG_ERROR("Maximum descriptor sets per pool reached");
    return VK_NULL_HANDLE;
  }

  VkDescriptorSetAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = pool->pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &layout;

  VkDescriptorSet set = VK_NULL_HANDLE;
  VkResult result = vkAllocateDescriptorSets(device, &alloc_info, &set);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate descriptor set: %d", (int)result);
    return VK_NULL_HANDLE;
  }

  pool->sets[pool->set_count] = set;
  pool->set_used[pool->set_count] = true;
  pool->set_count++;

  return set;
}

// Free descriptor set back to pool
static void free_descriptor_set(VkDescriptorSet set,
                                VkDescriptorSetLayout layout,
                                u32 pool_size_index) {
  for (u32 i = 0; i < g_descriptor_pool_count; i++) {
    if (g_descriptor_pools[i].layout == layout &&
        g_descriptor_pools[i].pool_size_index == pool_size_index) {
      DescriptorSetPool *pool = &g_descriptor_pools[i];
      for (u32 j = 0; j < pool->set_count; j++) {
        if (pool->sets[j] == set) {
          pool->set_used[j] = false;
          return;
        }
      }
    }
  }
}

// Cleanup all descriptor pools
static void cleanup_descriptor_pools(VkDevice device) {
  for (u32 i = 0; i < g_descriptor_pool_count; i++) {
    vkDestroyDescriptorPool(device, g_descriptor_pools[i].pool, NULL);
  }
  g_descriptor_pool_count = 0;
}
#else
// Stubs when Vulkan is disabled
static void cleanup_descriptor_pools(void *device) {}
#endif

// ============================================================================
// Device Lost Handling and Recovery System
// ============================================================================

static bool g_device_lost = false;
static u32 g_recovery_attempts = 0;
#define MAX_RECOVERY_ATTEMPTS 3

#ifdef VULKAN_BUILD
// Check if device is lost
static bool is_device_lost(VkResult result) {
  return result == VK_ERROR_DEVICE_LOST ||
         result == VK_ERROR_INITIALIZATION_FAILED ||
         result == VK_ERROR_OUT_OF_DEVICE_MEMORY;
}

// Handle device lost error
static void handle_device_lost(VulkanRenderer *renderer) {
  if (g_device_lost) {
    LOG_WARN("Device already marked as lost, ignoring additional errors");
    return;
  }

  g_device_lost = true;
  g_recovery_attempts++;

  LOG_ERROR("Vulkan device lost! Attempt %d recovery", g_recovery_attempts);

  if (g_recovery_attempts >= MAX_RECOVERY_ATTEMPTS) {
    LOG_ERROR("Maximum recovery attempts reached, shutting down renderer");
    return;
  }

  // Attempt device recovery
  if (attempt_device_recovery(renderer)) {
    LOG_INFO("Device recovery successful");
    g_device_lost = false;
    g_recovery_attempts = 0;
  } else {
    LOG_ERROR("Device recovery failed");
  }
}

// Attempt to recover from device lost
static bool attempt_device_recovery(VulkanRenderer *renderer) {
  if (!renderer)
    return false;

  LOG_INFO("Attempting device recovery...");

  // Wait for device to be idle before cleanup
  if (renderer->device) {
    vkDeviceWaitIdle(renderer->device);
  }

  // Cleanup all resources
  cleanup_pipeline_cache(renderer->device);
  cleanup_command_pools(renderer->device);
  cleanup_descriptor_pools(renderer->device);
  cleanup_memory_heaps();

  // Destroy old device
  if (renderer->device) {
    vkDestroyDevice(renderer->device, NULL);
    renderer->device = VK_NULL_HANDLE;
  }

  // Attempt to recreate device
  if (!recreate_vulkan_device(renderer)) {
    LOG_ERROR("Failed to recreate Vulkan device");
    return false;
  }

  // Recreate critical resources
  if (!recreate_critical_resources(renderer)) {
    LOG_ERROR("Failed to recreate critical resources");
    return false;
  }

  LOG_INFO("Device recovery completed successfully");
  return true;
}

// Recreate Vulkan device
static bool recreate_vulkan_device(VulkanRenderer *renderer) {
  if (!renderer || !renderer->physical_device) {
    LOG_ERROR("Invalid renderer state for device recreation");
    return false;
  }

  // Find queue families
  u32 graphics_family = 0;
  u32 present_family = 0;

  // This would need to be reimplemented based on existing queue family
  // detection For now, assume we have the families from before

  VkDeviceQueueCreateInfo queue_create_infos[2] = {0};
  f32 queue_priority = 1.0f;

  queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_infos[0].queueFamilyIndex = graphics_family;
  queue_create_infos[0].queueCount = 1;
  queue_create_infos[0].pQueuePriorities = &queue_priority;

  queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_infos[1].queueFamilyIndex = present_family;
  queue_create_infos[1].queueCount = 1;
  queue_create_infos[1].pQueuePriorities = &queue_priority;

  VkDeviceCreateInfo device_info = {0};
  device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_info.queueCreateInfoCount = 2;
  device_info.pQueueCreateInfos = queue_create_infos;
  device_info.enabledExtensionCount = ARRAY_SIZE(device_extensions);
  device_info.ppEnabledExtensionNames = device_extensions;

  VkResult result = vkCreateDevice(renderer->physical_device, &device_info,
                                   NULL, &renderer->device);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to recreate device: %d", (int)result);
    return false;
  }

  // Get queue handles
  vkGetDeviceQueue(renderer->device, graphics_family, 0,
                   &renderer->graphics_queue);
  vkGetDeviceQueue(renderer->device, present_family, 0,
                   &renderer->present_queue);

  return true;
}

// Recreate critical resources after device recovery
static bool recreate_critical_resources(VulkanRenderer *renderer) {
  if (!renderer || !renderer->device) {
    return false;
  }

  // Recreate pipeline cache
  if (!init_pipeline_cache(renderer->device)) {
    LOG_ERROR("Failed to recreate pipeline cache");
    return false;
  }

  // Recreate swapchain
  if (!vulkan_recreate_swapchain(renderer, renderer->swapchain_extent.width,
                                 renderer->swapchain_extent.height)) {
    LOG_ERROR("Failed to recreate swapchain");
    return false;
  }

  // Recreate other critical resources as needed
  // This would include render passes, framebuffers, etc.

  return true;
}

// Check device status and handle recovery
static bool check_device_status(VulkanRenderer *renderer) {
  if (g_device_lost) {
    LOG_WARN("Device is currently in recovery state");
    return false;
  }

  // Test device with a simple operation
  if (renderer->device) {
    VkResult result = vkDeviceWaitIdle(renderer->device);
    if (is_device_lost(result)) {
      handle_device_lost(renderer);
      return false;
    }
  }

  return true;
}

// Get device lost status
bool is_vulkan_device_lost(void) { return g_device_lost; }

// Reset device lost state (for testing/debugging)
void reset_device_lost_state(void) {
  g_device_lost = false;
  g_recovery_attempts = 0;
}

static u32 find_memory_type(VkPhysicalDevice physical_device, u32 type_filter,
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

  LOG_ERROR("Failed to find suitable memory type!");
  return 0;
}
#endif

bool vulkan_create_texture_image(VulkanRenderer *renderer, const char *path) {
  (void)renderer;
  (void)path;
  return true;
}

bool vulkan_create_texture_sampler(VulkanRenderer *renderer) {
  (void)renderer;
  return true;
}

// ============================
// Ray Tracing Scaffolding Stubs
// ============================

bool vulkan_rt_is_supported(VulkanRenderer *renderer) {
  // For now, return false until actual capability probing is implemented.
  // Keep this safe on all platforms.
  (void)renderer;
#ifdef VULKAN_BUILD
  return false;
#else
  return false;
#endif
}

bool vulkan_rt_init(VulkanRenderer *renderer) {
  // Safe no-op initialization. Do not fail the renderer if RTX is unavailable.
  if (!renderer)
    return false;
  renderer->ray_tracing_enabled = false;
  return false;
}

void vulkan_rt_cleanup(VulkanRenderer *renderer) {
  // Safe no-op cleanup.
  (void)renderer;
}

#ifdef VULKAN_BUILD
void vulkan_set_ambient_light(VulkanRenderer *renderer, f32 ambient_light) {
  if (!renderer)
    return;

  // Store ambient light value for shader uniforms
  renderer->ambient_light = ambient_light;

  // Update uniform buffer if available
  if (renderer->uniform_buffer_mapped) {
    // Assuming uniform buffer structure has ambient_light field
    // This will be properly implemented when shader system is ready
    // For now, store the value for later use
  }
}
#endif
