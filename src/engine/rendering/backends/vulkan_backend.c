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

#include "rendering/vulkan_backend.h"
#include "core/memory/vulkan_memory.h"
#include "rendering/frame_graph/frame_graph.h"
#include "core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Vulkan backend state structure
typedef struct {
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
    GPUMemoryAllocator* memoryAllocator;
    
    // Command pools
    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;
    VkCommandPool transferCommandPool;
    
    // Swapchain
    VkSwapchainKHR swapchain;
    VkExtent2D swapchainExtent;
    VkFormat swapchainFormat;
    uint32_t currentFrameIndex;
    
    // Render pass
    VkRenderPass renderPass;
    VkFramebuffer* framebuffers;
    uint32_t framebufferCount;
    
    // Synchronization
    VkSemaphore* imageAvailableSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    VkFence* inFlightFences;
    uint32_t maxFramesInFlight;
    
    // Pipeline cache
    // Simple hash map for pipeline cache
    struct {
        uint64_t key;
        VkPipeline pipeline;
    }* pipelineCache;
    uint32_t pipelineCacheSize;
    uint32_t pipelineCacheCapacity;
    
    // Debug markers
    bool enableDebugMarkers;
    
    // Validation layers
    const char** enabledValidationLayers;
    uint32_t validationLayerCount;
    
    // Extensions
    const char** enabledExtensions;
    uint32_t extensionCount;
    
    // Statistics
    uint64_t frameCount;
    double averageFrameTime;
    double lastFrameTime;
} VulkanBackend;

// Global Vulkan backend instance
static VulkanBackend g_vulkanBackend = {0};

// Validation layer names
static const char* g_validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};

// Required device extensions
static const char* g_deviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_MAINTENANCE1_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
};

// Internal helper functions
static VkResult create_vulkan_instance(const char* app_name, VkInstance* instance) {
    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = app_name;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Jules Engine";
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    // Enable validation layers in debug builds
    uint32_t layerCount = 0;
    const char* const* enabledLayers = nullptr;
    
    #ifdef DEBUG
    enabledLayers = g_validationLayers;
    layerCount = sizeof(g_validationLayers) / sizeof(g_validationLayers[0]);
    #endif
    
    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = enabledLayers;
    
    return vkCreateInstance(&createInfo, nullptr, instance);
}

static bool check_device_supports_extension(VkPhysicalDevice device, const char* extensionName) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, &extensionCount, nullptr);
    
    VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
    vkEnumerateDeviceExtensionProperties(device, &extensionCount, extensions);
    
    bool supported = false;
    for (uint32_t i = 0; i < extensionCount; i++) {
        if (strcmp(extensions[i].extensionName, extensionName) == 0) {
            supported = true;
            break;
        }
    }
    
    free(extensions);
    return supported;
}

static VkResult select_physical_device(VkInstance instance, VkPhysicalDevice* selectedDevice) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    
    // Select first suitable device
    *selectedDevice = VK_NULL_HANDLE;
    
    for (uint32_t i = 0; i < deviceCount; i++) {
        // Check if device supports required extensions
        bool supportsAllExtensions = true;
        for (uint32_t j = 0; j < sizeof(g_deviceExtensions) / sizeof(g_deviceExtensions[0]); j++) {
            if (!check_device_supports_extension(devices[i], g_deviceExtensions[j])) {
                supportsAllExtensions = false;
                break;
            }
        }
        
        if (supportsAllExtensions) {
            *selectedDevice = devices[i];
            break;
        }
    }
    
    free(devices);
    return (*selectedDevice != VK_NULL_HANDLE) ? VK_SUCCESS : VK_ERROR_INITIALIZATION_FAILED;
}

static VkResult create_logical_device(VkPhysicalDevice physicalDevice, VkDevice* device) {
    // Find queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    
    VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);
    
    uint32_t graphicsFamily = UINT32_MAX;
    uint32_t computeFamily = UINT32_MAX;
    uint32_t transferFamily = UINT32_MAX;
    
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && graphicsFamily == UINT32_MAX) {
            graphicsFamily = i;
        }
        
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT && computeFamily == UINT32_MAX) {
            computeFamily = i;
        }
        
        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT && transferFamily == UINT32_MAX) {
            transferFamily = i;
        }
    }
    
    free(queueFamilies);
    
    if (graphicsFamily == UINT32_MAX) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    // Create logical device
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamily;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.enabledExtensionCount = sizeof(g_deviceExtensions) / sizeof(g_deviceExtensions[0]);
    createInfo.ppEnabledExtensionNames = g_deviceExtensions;
    
    VkPhysicalDeviceFeatures features = {0};
    features.samplerAnisotropy = VK_TRUE;
    createInfo.pEnabledFeatures = &features;
    
    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, device);
    if (result == VK_SUCCESS) {
        // Get device queue
        vkGetDeviceQueue(*device, graphicsFamily, 0, &g_vulkanBackend.graphicsQueue);
        g_vulkanBackend.graphicsQueueFamily = graphicsFamily;
    }
    
    return result;
}

// Public API implementation
bool vulkan_backend_init(const char* app_name, void* window_handle) {
    if (g_vulkanBackend.instance != VK_NULL_HANDLE) {
        LOG_INFO("Vulkan backend already initialized");
        return true;
    }
    
    LOG_INFO("Initializing Vulkan backend...");
    
    // Create Vulkan instance
    VkResult result = create_vulkan_instance(app_name, &g_vulkanBackend.instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create Vulkan instance: %d", result);
        return false;
    }
    
    // Select physical device
    result = select_physical_device(g_vulkanBackend.instance, &g_vulkanBackend.physicalDevice);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to select physical device: %d", result);
        return false;
    }
    
    // Create logical device
    result = create_logical_device(g_vulkanBackend.physicalDevice, &g_vulkanBackend.device);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create logical device: %d", result);
        return false;
    }
    
    g_vulkanBackend.currentFrameIndex = 0;
    g_vulkanBackend.frameCount = 0;
    g_vulkanBackend.lastFrameTime = 0.0;
    
    LOG_INFO("Vulkan backend initialized successfully");
    return true;
}

void vulkan_backend_shutdown(void) {
    if (g_vulkanBackend.instance == VK_NULL_HANDLE) {
        return;
    }
    
    LOG_INFO("Shutting down Vulkan backend...");
    
    // Wait for device to be idle
    vkDeviceWaitIdle(g_vulkanBackend.device);
    
    // Destroy logical device
    vkDestroyDevice(g_vulkanBackend.device, nullptr);
    
    // Destroy instance
    vkDestroyInstance(g_vulkanBackend.instance, nullptr);
    
    // Reset state
    memset(&g_vulkanBackend, 0, sizeof(g_vulkanBackend));
    
    LOG_INFO("Vulkan backend shutdown complete");
}

bool vulkan_backend_is_initialized(void) {
    return g_vulkanBackend.instance != VK_NULL_HANDLE;
}

const char* vulkan_backend_get_name(void) {
    return "Vulkan";
}
