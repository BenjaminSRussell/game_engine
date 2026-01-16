// Vulkan Backend Implementation - Cross-Platform 3D Rendering
// VULKAN-001: Device setup and management
// VULKAN-002: Command queue management
// VULKAN-003: Shader module and pipeline creation
// VULKAN-004: Buffer management
// VULKAN-005: Texture management
// VULKAN-006: Sampler management
// VULKAN-007: Render pass management
// VULKAN-008: Command encoding
// VULKAN-009: Swapchain management
// VULKAN-010: Synchronization and memory barriers

#include "../include/rendering/vulkan_backend.h"
#include "../include/rendering/frame_graph/frame_graph.h"
#include "../include/rendering/gpu_memory.h"
#include "../include/rendering/vulkan.h"
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

// Vulkan backend state structure
struct VulkanBackend {
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue computeQueue;
  VkQueue transferQueue;
  uint32_t graphicsQueueFamily;
  uint32_t computeQueueFamily;
  uint32_t transferQueueFamily;

  // Memory allocator
  GPUMemoryAllocator *memoryAllocator;

  // Command pools
  VkCommandPool graphicsCommandPool;
  VkCommandPool computeCommandPool;
  VkCommandPool transferCommandPool;

  // Swapchain
  VkSwapchainKHR swapchain;
  VkExtent2D swapchainExtent;
  VkFormat swapchainFormat;
  uint32_t currentFrameIndex;
  uint32_t currentImageIndex; // Added for swapchain image tracking

  // Render pass
  VkRenderPass renderPass;
  std::vector<VkFramebuffer> framebuffers;
  std::vector<VkImageView> swapchainImageViews; // Added member for image views

  // Synchronization
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  // Pipeline cache
  std::unordered_map<uint64_t, VkPipeline> pipelineCache;

  // Debug markers
  bool enableDebugMarkers;

  // Validation layers
  std::vector<const char *> enabledValidationLayers;

  // Extensions
  std::vector<const char *> enabledExtensions;

  // Statistics
  uint64_t frameCount;
  double averageFrameTime;
  std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
};

// Global Vulkan backend instance
static VulkanBackend g_vulkanBackend = {};

// Validation layer names
static const char *g_validationLayers[] = {
    "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_standard_validation",
    "VK_LAYER_GOOGLE_threading", "VK_LAYER_LUNARG_parameter_validation"};

// Required device extensions
static const char *g_deviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME,
    VK_KHR_MAINTENANCE2_EXTENSION_NAME, VK_KHR_MAINTENANCE3_EXTENSION_NAME,
    // VK_KHR_MAINTENANCE4_EXTENSION_NAME, // Disabled due to missing header
    // support
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME};

// Internal helper functions
static VKAPI_ATTR bool
check_device_supports_extension(VkPhysicalDevice device,
                                const char *extensionName);

static VKAPI_ATTR VkResult create_vulkan_instance(const char *app_name,
                                                  VkInstance *instance) {
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = app_name;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Jules Engine";
  appInfo.apiVersion = VK_API_VERSION_1_0;

  // Enable validation layers in debug builds
  uint32_t layerCount = 0;
  const char *const *enabledLayers = nullptr;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

#ifdef DEBUG
  enabledLayers = g_validationLayers;
  layerCount = sizeof(g_validationLayers) / sizeof(g_validationLayers[0]);
#endif

  createInfo.enabledLayerCount = layerCount;
  createInfo.ppEnabledLayerNames = enabledLayers;
  createInfo.enabledExtensionCount = 0; // TODO: Add glfw extensions if needed
  createInfo.ppEnabledExtensionNames = nullptr; // TODO: Add glfw extensions

  // Start with MacOS Portability extensions if on Apple
#ifdef __APPLE__
  const char *appleExtensions[] = {
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
      "VK_KHR_get_physical_device_properties2" // Required for portability
                                               // subset
  };
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  createInfo.enabledExtensionCount = 2; // update if adding more
  createInfo.ppEnabledExtensionNames = appleExtensions;
#endif

  return vkCreateInstance(&createInfo, nullptr, instance);
}

static VKAPI_ATTR VkResult
select_physical_device(VkInstance instance, VkPhysicalDevice *selectedDevice) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  // Select first suitable device
  *selectedDevice = VK_NULL_HANDLE;

  for (uint32_t i = 0; i < deviceCount; i++) {
    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    // Check if device supports required extensions
    bool supportsAllExtensions = true;
    for (const char *extension : g_deviceExtensions) {
      if (!check_device_supports_extension(devices[i], extension)) {
        supportsAllExtensions = false;
        break;
      }
    }

    // Check if device has graphics and compute queues
    bool hasGraphicsQueue = false;
    bool hasComputeQueue = false;
    bool hasTransferQueue = false;

    uint32_t graphicsQueueFamily = UINT32_MAX;
    uint32_t computeQueueFamily = UINT32_MAX;
    uint32_t transferQueueFamily = UINT32_MAX;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount,
                                             queueFamilies.data());

    for (uint32_t j = 0; j < queueFamilies.size(); j++) {
      const auto &properties = queueFamilies[j];

      if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
          graphicsQueueFamily == UINT32_MAX) {
        graphicsQueueFamily = j;
      }

      if (properties.queueFlags & VK_QUEUE_COMPUTE_BIT &&
          computeQueueFamily == UINT32_MAX) {
        computeQueueFamily = j;
      }

      if (properties.queueFlags & VK_QUEUE_TRANSFER_BIT &&
          transferQueueFamily == UINT32_MAX) {
        transferQueueFamily = j;
      }
    }

    if (supportsAllExtensions && hasGraphicsQueue && hasComputeQueue &&
        hasTransferQueue) {
      *selectedDevice = devices[i];
      break;
    }
  }

  return (*selectedDevice != VK_NULL_HANDLE) ? VK_SUCCESS
                                             : VK_ERROR_INITIALIZATION_FAILED;
}

static VKAPI_ATTR bool
check_device_supports_extension(VkPhysicalDevice device,
                                const char *extensionName) {
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       extensions.data());

  for (const auto &ext : extensions) {
    if (strcmp(ext.extensionName, extensionName) == 0) {
      return true;
    }
  }

  return false;
}

static VKAPI_ATTR VkResult
create_logical_device(VkPhysicalDevice physicalDevice, VkDevice *device) {
  // Find queue families
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  int32_t graphicsFamily = -1;
  int32_t computeFamily = -1;
  int32_t transferFamily = -1;

  for (uint32_t i = 0; i < queueFamilies.size(); i++) {
    const auto &properties = queueFamilies[i];

    if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT && graphicsFamily == -1) {
      graphicsFamily = i;
    }

    if (properties.queueFlags & VK_QUEUE_COMPUTE_BIT && computeFamily == -1) {
      computeFamily = i;
    }

    if (properties.queueFlags & VK_QUEUE_TRANSFER_BIT && transferFamily == -1) {
      transferFamily = i;
    }
  }

  if (graphicsFamily == -1 || computeFamily == -1 || transferFamily == -1) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  g_vulkanBackend.graphicsQueueFamily = graphicsFamily;
  g_vulkanBackend.computeQueueFamily = computeFamily;
  g_vulkanBackend.transferQueueFamily = transferFamily;

  // Create logical device
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::vector<uint32_t> queueFamilyIndices;

  if (graphicsFamily != -1) {
    VkDeviceQueueCreateInfo graphicsQueueInfo = {};
    graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueInfo.queueFamilyIndex = graphicsFamily;
    graphicsQueueInfo.queueCount = 1;
    queueCreateInfos.push_back(graphicsQueueInfo);
    queueFamilyIndices.push_back(graphicsFamily);
  }

  if (computeFamily != -1) {
    VkDeviceQueueCreateInfo computeQueueInfo = {};
    computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    computeQueueInfo.queueFamilyIndex = computeFamily;
    computeQueueInfo.queueCount = 1;
    queueCreateInfos.push_back(computeQueueInfo);
    queueFamilyIndices.push_back(computeFamily);
  }

  if (transferFamily != -1) {
    VkDeviceQueueCreateInfo transferQueueInfo = {};
    transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    transferQueueInfo.queueFamilyIndex = transferFamily;
    transferQueueInfo.queueCount = 1;
    queueCreateInfos.push_back(transferQueueInfo);
    queueFamilyIndices.push_back(transferFamily);
  }

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = queueCreateInfos.size();
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  // createInfo.pEnabledQueueIndices = queueFamilyIndices.data(); // Removed
  // invalid member
  createInfo.pEnabledFeatures = nullptr;

  VkPhysicalDeviceFeatures features = {};
  vkGetPhysicalDeviceFeatures(physicalDevice, &features);

  // Enable required features
  features.samplerAnisotropy = VK_TRUE;
  features.fillModeNonSolid = VK_TRUE;
  features.pipelineStatisticsQuery = VK_TRUE;
  features.independentBlend = VK_TRUE;

  createInfo.pEnabledFeatures = &features;

  VkResult result =
      vkCreateDevice(physicalDevice, &createInfo, nullptr, device);
  if (result != VK_SUCCESS) {
    return result;
  }

  // Get device queues
  // Get device queues
  vkGetDeviceQueue(g_vulkanBackend.device, (uint32_t)graphicsFamily, 0,
                   &g_vulkanBackend.graphicsQueue);
  vkGetDeviceQueue(g_vulkanBackend.device, (uint32_t)computeFamily, 0,
                   &g_vulkanBackend.computeQueue);
  vkGetDeviceQueue(g_vulkanBackend.device, (uint32_t)transferFamily, 0,
                   &g_vulkanBackend.transferQueue);

  return VK_SUCCESS;
}

static VKAPI_ATTR VkResult create_command_pools(
    VkDevice device, uint32_t graphicsFamily, uint32_t computeFamily,
    uint32_t transferFamily, VkCommandPool *graphicsPool,
    VkCommandPool *computePool, VkCommandPool *transferPool) {
  VkCommandPoolCreateInfo graphicsPoolInfo = {};
  graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  graphicsPoolInfo.queueFamilyIndex = graphicsFamily;
  // graphicsPoolInfo.commandBufferCount = 2; // Removed invalid member

  VkResult result =
      vkCreateCommandPool(device, &graphicsPoolInfo, nullptr, graphicsPool);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkCommandPoolCreateInfo computePoolInfo = {};
  computePoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  computePoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  computePoolInfo.queueFamilyIndex = computeFamily;
  // computePoolInfo.commandBufferCount = 1;

  result = vkCreateCommandPool(device, &computePoolInfo, nullptr, computePool);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkCommandPoolCreateInfo transferPoolInfo = {};
  transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  transferPoolInfo.queueFamilyIndex = transferFamily;
  // transferPoolInfo.commandBufferCount = 1;

  result =
      vkCreateCommandPool(device, &transferPoolInfo, nullptr, transferPool);
  if (result != VK_SUCCESS) {
    return result;
  }

  return VK_SUCCESS;
}

static VKAPI_ATTR VkResult create_swapchain(
    VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface,
    VkSwapchainKHR *swapchain, VkExtent2D *extent, VkFormat *format) {
  // Get surface capabilities
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &capabilities);

  // Get surface formats
  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       nullptr);
  std::vector<VkSurfaceFormatKHR> formats(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       formats.data());

  // Choose surface format
  VkSurfaceFormatKHR surfaceFormat = formats[0];
  VkColorSpaceKHR colorSpace = surfaceFormat.colorSpace;

  // Try to find a suitable format
  for (const auto &availableFormat : formats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      surfaceFormat = availableFormat;
      colorSpace = availableFormat.colorSpace;
      break;
    }
  }

  // Choose present mode
  // Choose present mode
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                            &presentModeCount, nullptr);
  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice, surface, &presentModeCount, presentModes.data());

  for (const auto &availablePresentMode : presentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = availablePresentMode;
      break;
    }
  }

  // Choose extent
  if (capabilities.currentExtent.width != UINT32_MAX) {
    *extent = capabilities.minImageExtent;
  } else {
    *extent = capabilities.currentExtent;
  }

  // Create swapchain
  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = 2;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = colorSpace;
  createInfo.imageExtent = *extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_FALSE;
  createInfo.oldSwapchain = *swapchain;

  VkResult result =
      vkCreateSwapchainKHR(device, &createInfo, nullptr, swapchain);
  if (result != VK_SUCCESS) {
    return result;
  }

  // Get swapchain images
  uint32_t imageCount = 0;
  vkGetSwapchainImagesKHR(device, *swapchain, &imageCount, nullptr);
  std::vector<VkImage> swapchainImages(imageCount);
  vkGetSwapchainImagesKHR(device, *swapchain, &imageCount,
                          swapchainImages.data());

  // Create image views for swapchain
  g_vulkanBackend.swapchainImageViews.resize(imageCount);
  for (uint32_t i = 0; i < imageCount; i++) {
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = swapchainImages[i];
    imageViewCreateInfo.format =
        surfaceFormat.format; // Use .format from struct
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    result =
        vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS) {
      return result;
    }

    g_vulkanBackend.swapchainImageViews[i] = imageView;
  }

  *extent = g_vulkanBackend.swapchainExtent;
  *format = g_vulkanBackend.swapchainFormat;

  return VK_SUCCESS;
}

static VKAPI_ATTR VkResult create_render_pass(VkDevice device, VkFormat format,
                                              VkRenderPass *renderPass) {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 0;

  return vkCreateRenderPass(device, &renderPassInfo, nullptr, renderPass);
}

static VKAPI_ATTR VkResult
create_framebuffers(VkDevice device, VkRenderPass renderPass, VkExtent2D extent,
                    VkFormat format, std::vector<VkFramebuffer> *framebuffers) {
  framebuffers->resize(g_vulkanBackend.swapchainImageViews.size());

  for (uint32_t i = 0; i < framebuffers->size(); i++) {
    VkImageView imageView = g_vulkanBackend.swapchainImageViews[i];

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &imageView;
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer;
    VkResult result =
        vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer);
    if (result != VK_SUCCESS) {
      return result;
    }

    (*framebuffers)[i] = framebuffer;
  }

  return VK_SUCCESS;
}

static VKAPI_ATTR VkResult
create_synchronization_objects(VkDevice device, uint32_t maxFramesInFlight) {
  g_vulkanBackend.imageAvailableSemaphores.resize(maxFramesInFlight);
  g_vulkanBackend.renderFinishedSemaphores.resize(maxFramesInFlight);
  g_vulkanBackend.inFlightFences.resize(maxFramesInFlight);

  for (uint32_t i = 0; i < maxFramesInFlight; i++) {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result =
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &g_vulkanBackend.imageAvailableSemaphores[i]);
    if (result != VK_SUCCESS) {
      return result;
    }

    result = vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                               &g_vulkanBackend.renderFinishedSemaphores[i]);
    if (result != VK_SUCCESS) {
      return result;
    }

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    result = vkCreateFence(device, &fenceInfo, nullptr,
                           &g_vulkanBackend.inFlightFences[i]);
    if (result != VK_SUCCESS) {
      return result;
    }
  }

  return VK_SUCCESS;
}

// Public API implementation
extern "C" {

bool vulkan_init(void *window_handle, const VulkanInitParams *params) {
  if (g_vulkanBackend.instance != VK_NULL_HANDLE) {
    printf("Vulkan backend already initialized\n");
    return true;
  }

  printf("Initializing Vulkan backend...\n");

  // Create Vulkan instance
  VkResult result =
      create_vulkan_instance(params->app_name, &g_vulkanBackend.instance);
  if (result != VK_SUCCESS) {
    printf("Failed to create Vulkan instance: %d\n", result);
    return false;
  }

  // Select physical device
  result = select_physical_device(g_vulkanBackend.instance,
                                  &g_vulkanBackend.physicalDevice);
  if (result != VK_SUCCESS) {
    printf("Failed to select physical device: %d\n", result);
    return false;
  }

  // Create logical device
  result = create_logical_device(g_vulkanBackend.physicalDevice,
                                 &g_vulkanBackend.device);
  if (result != VK_SUCCESS) {
    printf("Failed to create logical device: %d\n", result);
    return false;
  }

  // Initialize memory allocator
  if (!gpu_memory_init(g_vulkanBackend.memoryAllocator, g_vulkanBackend.device,
                       g_vulkanBackend.physicalDevice,
                       params->device_local_budget)) {
    printf("Failed to initialize GPU memory allocator\n");
    return false;
  }

  // Create swapchain
  VkSurfaceKHR surface = (VkSurfaceKHR)window_handle;
  result = create_swapchain(
      g_vulkanBackend.physicalDevice, g_vulkanBackend.device, surface,
      &g_vulkanBackend.swapchain, &g_vulkanBackend.swapchainExtent,
      &g_vulkanBackend.swapchainFormat);
  if (result != VK_SUCCESS) {
    printf("Failed to create swapchain: %d\n", result);
    return false;
  }

  // Create render pass
  result = create_render_pass(g_vulkanBackend.device,
                              g_vulkanBackend.swapchainFormat,
                              &g_vulkanBackend.renderPass);
  if (result != VK_SUCCESS) {
    printf("Failed to create render pass: %d\n", result);
    return false;
  }

  // Create framebuffers
  result = create_framebuffers(
      g_vulkanBackend.device, g_vulkanBackend.renderPass,
      g_vulkanBackend.swapchainExtent, g_vulkanBackend.swapchainFormat,
      &g_vulkanBackend.framebuffers);
  if (result != VK_SUCCESS) {
    printf("Failed to create framebuffers: %d\n", result);
    return false;
  }

  // Create synchronization objects
  result = create_synchronization_objects(g_vulkanBackend.device,
                                          params->max_frames_in_flight);
  if (result != VK_SUCCESS) {
    printf("Failed to create synchronization objects: %d\n", result);
    return false;
  }

  g_vulkanBackend.currentFrameIndex = 0;
  g_vulkanBackend.frameCount = 0;
  g_vulkanBackend.lastFrameTime = std::chrono::high_resolution_clock::now();

  printf("Vulkan backend initialized successfully\n");
  return true;
}

void vulkan_shutdown(void) {
  if (g_vulkanBackend.instance == VK_NULL_HANDLE) {
    return;
  }

  printf("Shutting down Vulkan backend...\n");

  // Wait for device to be idle
  // Wait for device to be idle
  vkDeviceWaitIdle(g_vulkanBackend.device);

  // Destroy synchronization objects
  for (VkFence fence : g_vulkanBackend.inFlightFences) {
    vkDestroyFence(g_vulkanBackend.device, fence, nullptr);
  }

  for (VkSemaphore semaphore : g_vulkanBackend.imageAvailableSemaphores) {
    vkDestroySemaphore(g_vulkanBackend.device, semaphore, nullptr);
  }

  for (VkSemaphore semaphore : g_vulkanBackend.renderFinishedSemaphores) {
    vkDestroySemaphore(g_vulkanBackend.device, semaphore, nullptr);
  }

  // Destroy framebuffers
  for (VkFramebuffer framebuffer : g_vulkanBackend.framebuffers) {
    vkDestroyFramebuffer(g_vulkanBackend.device, framebuffer, nullptr);
  }

  // Destroy render pass
  vkDestroyRenderPass(g_vulkanBackend.device, g_vulkanBackend.renderPass,
                      nullptr);

  // Destroy swapchain image views
  for (VkImageView imageView : g_vulkanBackend.swapchainImageViews) {
    vkDestroyImageView(g_vulkanBackend.device, imageView, nullptr);
  }

  // Destroy swapchain
  vkDestroySwapchainKHR(g_vulkanBackend.device, g_vulkanBackend.swapchain,
                        nullptr);

  // Destroy command pools
  vkDestroyCommandPool(g_vulkanBackend.device,
                       g_vulkanBackend.graphicsCommandPool, nullptr);
  vkDestroyCommandPool(g_vulkanBackend.device,
                       g_vulkanBackend.computeCommandPool, nullptr);
  vkDestroyCommandPool(g_vulkanBackend.device,
                       g_vulkanBackend.transferCommandPool, nullptr);

  // Destroy logical device
  vkDestroyDevice(g_vulkanBackend.device, nullptr);

  // Destroy instance
  vkDestroyInstance(g_vulkanBackend.instance, nullptr);

  // Reset state
  memset(&g_vulkanBackend, 0, sizeof(g_vulkanBackend));

  printf("Vulkan backend shutdown complete\n");
}

bool vulkan_is_initialized(void) {
  return g_vulkanBackend.instance != VK_NULL_HANDLE;
}

// Frame rendering
bool vulkan_begin_frame(VulkanBackend *backend, void *commandBuffer,
                        uint32_t imageIndex) {
  if (!vulkan_is_initialized()) {
    return false;
  }

  VkResult result = vkAcquireNextImageKHR(
      g_vulkanBackend.device, g_vulkanBackend.swapchain, UINT64_MAX,
      g_vulkanBackend.imageAvailableSemaphores[imageIndex], VK_NULL_HANDLE,
      &g_vulkanBackend.currentImageIndex);
  if (result != VK_SUCCESS) {
    printf("Failed to acquire next image: %d\n", result);
    return false;
  }

  // Record command buffer
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = VK_NULL_HANDLE;
  beginInfo.pNext = nullptr;

  vkBeginCommandBuffer((VkCommandBuffer)commandBuffer, &beginInfo);

  // Set render pass
  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.pNext = nullptr;
  renderPassBeginInfo.renderPass = backend->renderPass;
  renderPassBeginInfo.framebuffer =
      backend->framebuffers[g_vulkanBackend.currentImageIndex];
  renderPassBeginInfo.renderArea.offset = {0, 0};
  renderPassBeginInfo.renderArea.extent = backend->swapchainExtent;

  VkClearValue clearValues[2] = {};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPassBeginInfo.clearValueCount = 2;
  renderPassBeginInfo.pClearValues = clearValues;

  vkCmdBeginRenderPass((VkCommandBuffer)commandBuffer, &renderPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Set viewport
  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)backend->swapchainExtent.width;
  viewport.height = (float)backend->swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport((VkCommandBuffer)commandBuffer, 0, 1, &viewport);

  // Clear color attachment
  // vkCmdClearAttachments removal: Logic handled by RenderPass LoadOp

  return true;
}

void vulkan_end_frame(VulkanBackend *backend, void *commandBuffer,
                      uint32_t imageIndex) {
  if (!vulkan_is_initialized()) {
    return;
  }

  // End render pass
  vkCmdEndRenderPass((VkCommandBuffer)commandBuffer);

  // End command buffer
  vkEndCommandBuffer((VkCommandBuffer)commandBuffer);

  // Submit command buffer
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores =
      &g_vulkanBackend.imageAvailableSemaphores[imageIndex];
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = (VkCommandBuffer *)commandBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores =
      &g_vulkanBackend.renderFinishedSemaphores[imageIndex];

  VkResult result = vkQueueSubmit(g_vulkanBackend.graphicsQueue, 1, &submitInfo,
                                  g_vulkanBackend.inFlightFences[imageIndex]);
  if (result != VK_SUCCESS) {
    printf("Failed to submit command buffer: %d\n", result);
    return;
  }

  // Present
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores =
      &g_vulkanBackend.renderFinishedSemaphores[imageIndex];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &g_vulkanBackend.swapchain;
  presentInfo.pImageIndices = &g_vulkanBackend.currentImageIndex;

  result = vkQueuePresentKHR(g_vulkanBackend.graphicsQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    // Handle window resize by recreating swapchain
    printf("Window resize detected, recreating swapchain\n");
    // TODO: Implement swapchain recreation
  }

  // backend->currentFrameIndex = (imageIndex + 1) %
  // g_vulkanBackend.imageAvailableSemaphores.size();

  // Update frame statistics
  auto currentTime = std::chrono::high_resolution_clock::now();
  auto frameTime = std::chrono::duration<double, std::milli>(
                       currentTime - backend->lastFrameTime)
                       .count();
  backend->lastFrameTime = currentTime;
  backend->frameCount++;
  backend->averageFrameTime =
      (backend->averageFrameTime * (backend->frameCount - 1) + frameTime) /
      backend->frameCount;
}

// Get Vulkan backend statistics
void vulkan_get_stats(VulkanStats *outStats) {
  if (!vulkan_is_initialized()) {
    memset(outStats, 0, sizeof(*outStats));
    return;
  }

  outStats->frame_count = g_vulkanBackend.frameCount;
  outStats->average_frame_time = g_vulkanBackend.averageFrameTime;
  outStats->memory_usage =
      gpu_memory_get_allocated_size(g_vulkanBackend.memoryAllocator);
}

// Vulkan backend stability validation
bool vulkan_validate_backend(VulkanBackend *backend) {
  if (!vulkan_is_initialized()) {
    return false;
  }

  // Validate device state
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(backend->physicalDevice, &properties);

  // Check for critical issues
  if (properties.limits.maxImageDimension2D < 256) {
    printf("ERROR: Device minimum texture size too small\n");
    return false;
  }

  if (properties.limits.maxFramebufferWidth < 256 ||
      properties.limits.maxFramebufferHeight < 256) {
    printf("ERROR: Device minimum framebuffer size too small\n");
    return false;
  }

  // Validate swapchain
  if (backend->swapchain == VK_NULL_HANDLE) {
    printf("ERROR: No swapchain created\n");
    return false;
  }

  // Validate render pass
  if (backend->renderPass == VK_NULL_HANDLE) {
    printf("ERROR: No render pass created\n");
    return false;
  }

  // Validate framebuffers
  if (backend->framebuffers.empty()) {
    printf("ERROR: No framebuffers created\n");
    return false;
  }

  // Validate synchronization objects
  if (backend->inFlightFences.empty() ||
      backend->imageAvailableSemaphores.empty() ||
      backend->renderFinishedSemaphores.empty()) {
    printf("ERROR: Missing synchronization objects\n");
    return false;
  }

  printf("Vulkan backend validation passed\n");
  return true;
}

// Get Vulkan backend name
const char *vulkan_get_backend_name(void) { return "Vulkan"; }

} // extern "C"
