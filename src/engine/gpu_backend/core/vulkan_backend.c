#include "vulkan_backend.h"
#ifdef VULKAN_BUILD
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Validation layers for debug builds
static const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

// Required device extensions
static const char* device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Check if validation layers are available
static bool check_validation_layer_support() {
    u32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    
    VkLayerProperties* available_layers = malloc(layer_count * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);
    
    for (u32 i = 0; i < sizeof(validation_layers) / sizeof(validation_layers[0]); i++) {
        bool layer_found = false;
        for (u32 j = 0; j < layer_count; j++) {
            if (strcmp(validation_layers[i], available_layers[j].layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        if (!layer_found) {
            free(available_layers);
            return false;
        }
    }
    
    free(available_layers);
    return true;
}

// Debug callback for validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    
    (void)message_type;
    (void)user_data;
    
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        printf("Vulkan Validation: %s\n", callback_data->pMessage);
    }
    
    return VK_FALSE;
}

// Setup debug messenger
static VkResult setup_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT* debug_messenger) {
    if (!check_validation_layer_support()) {
        return VK_SUCCESS; // No validation layers available
    }
    
    VkDebugUtilsMessengerCreateInfoEXT create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = NULL
    };
    
    // Load the function pointer
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = 
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    
    if (vkCreateDebugUtilsMessengerEXT == NULL) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    
    return vkCreateDebugUtilsMessengerEXT(instance, &create_info, NULL, debug_messenger);
}

// Destroy debug messenger
static void destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger) {
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = 
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    
    if (vkDestroyDebugUtilsMessengerEXT != NULL) {
        vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, NULL);
    }
}

#endif

/**
 * =================================================================================================
 *                                   RENDERER - VULKAN BACKEND
 * =================================================================================================
 * 
 * GOAL: Explicit control over GPU for maximum performance.
 */

void Vulkan_Init(void* window_handle) {
#ifdef VULKAN_BUILD
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    
    // TASK_600: Create VkInstance with Validation Layers (in Debug).
    
    // Check for validation layer support
    bool enable_validation = false;
#ifdef DEBUG
    enable_validation = check_validation_layer_support();
    if (enable_validation) {
        printf("Vulkan: Enabling validation layers\n");
    } else {
        printf("Vulkan: Validation layers not available\n");
    }
#endif
    
    // Get required extensions
    u32 extension_count = 0;
    const char** extensions = NULL;
    
    // Basic required extensions
    const char* basic_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef __APPLE__
        VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#elif defined(_WIN32)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
    };
    
    extension_count = sizeof(basic_extensions) / sizeof(basic_extensions[0]);
    extensions = malloc(extension_count * sizeof(const char*));
    memcpy(extensions, basic_extensions, extension_count * sizeof(const char*));
    
    // Add debug utils extension if validation is enabled
    if (enable_validation) {
        const char* debug_extension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        extensions = realloc(extensions, (extension_count + 1) * sizeof(const char*));
        extensions[extension_count] = debug_extension;
        extension_count++;
    }
    
    // Check if all extensions are available
    u32 available_count;
    vkEnumerateInstanceExtensionProperties(NULL, &available_count, NULL);
    VkExtensionProperties* available_extensions = malloc(available_count * sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &available_count, available_extensions);
    
    for (u32 i = 0; i < extension_count; i++) {
        bool found = false;
        for (u32 j = 0; j < available_count; j++) {
            if (strcmp(extensions[i], available_extensions[j].extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            printf("Vulkan: Required extension %s not available\n", extensions[i]);
            free(extensions);
            free(available_extensions);
            goto cleanup;
        }
    }
    
    free(available_extensions);
    
    // Create application info
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Minecraft v2",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Minecraft v2 Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };
    
    // Create instance
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount = enable_validation ? (sizeof(validation_layers) / sizeof(validation_layers[0])) : 0,
        .ppEnabledLayerNames = enable_validation ? validation_layers : NULL
    };
    
    VkResult result = vkCreateInstance(&create_info, NULL, &instance);
    
    free(extensions);
    
    if (result != VK_SUCCESS) {
        printf("Vulkan: Failed to create instance: %d\n", result);
        goto cleanup;
    }
    
    printf("Vulkan: Instance created successfully\n");
    
    // Setup debug messenger
    if (enable_validation) {
        result = setup_debug_messenger(instance, &debug_messenger);
        if (result != VK_SUCCESS) {
            printf("Vulkan: Failed to setup debug messenger: %d\n", result);
        } else {
            printf("Vulkan: Debug messenger setup complete\n");
        }
    }
    
    // TASK_601: Select Physical Device.
    //       Score devices (Discrete > Integrated). Check VRAM size.
    
    // Enumerate physical devices
    u32 device_count = 0;
    result = vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    
    if (result != VK_SUCCESS || device_count == 0) {
        printf("Vulkan: No physical devices found\n");
        goto cleanup;
    }
    
    VkPhysicalDevice* devices = malloc(device_count * sizeof(VkPhysicalDevice));
    result = vkEnumeratePhysicalDevices(instance, &device_count, devices);
    
    if (result != VK_SUCCESS) {
        printf("Vulkan: Failed to enumerate physical devices: %d\n", result);
        free(devices);
        goto cleanup;
    }
    
    printf("Vulkan: Found %u physical devices\n", device_count);
    
    // Device scoring system
    typedef struct {
        VkPhysicalDevice device;
        u32 score;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memory_properties;
    } DeviceScore;
    
    DeviceScore* device_scores = malloc(device_count * sizeof(DeviceScore));
    u32 best_device_index = 0;
    u32 best_score = 0;
    
    for (u32 i = 0; i < device_count; i++) {
        DeviceScore* score = &device_scores[i];
        score->device = devices[i];
        score->score = 0;
        
        // Get device properties
        vkGetPhysicalDeviceProperties(devices[i], &score->properties);
        vkGetPhysicalDeviceMemoryProperties(devices[i], &score->memory_properties);
        
        printf("Vulkan: Device %u: %s\n", i, score->properties.deviceName);
        printf("  Type: %u, Vendor: %u, Device ID: %u\n", 
               score->properties.deviceType, score->properties.vendorID, score->properties.deviceID);
        
        // Score based on device type
        switch (score->properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score->score += 1000; // Discrete GPUs are preferred
                printf("  Type: Discrete GPU (+1000)\n");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score->score += 500;  // Integrated GPUs are acceptable
                printf("  Type: Integrated GPU (+500)\n");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score->score += 100;  // Virtual GPUs are less preferred
                printf("  Type: Virtual GPU (+100)\n");
                break;
            default:
                score->score += 10;   // Other types get minimal score
                printf("  Type: Other (+10)\n");
                break;
        }
        
        // Score based on VRAM size
        u32 total_vram = 0;
        for (u32 j = 0; j < score->memory_properties.memoryHeapCount; j++) {
            if (score->memory_properties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                total_vram += score->memory_properties.memoryHeaps[j].size / (1024 * 1024); // Convert to MB
            }
        }
        
        printf("  VRAM: %u MB\n", total_vram);
        
        // Bonus points for VRAM
        if (total_vram >= 8192) {      // 8GB+
            score->score += 500;
            printf("  VRAM: 8GB+ (+500)\n");
        } else if (total_vram >= 4096) { // 4GB+
            score->score += 300;
            printf("  VRAM: 4GB+ (+300)\n");
        } else if (total_vram >= 2048) { // 2GB+
            score->score += 200;
            printf("  VRAM: 2GB+ (+200)\n");
        } else if (total_vram >= 1024) { // 1GB+
            score->score += 100;
            printf("  VRAM: 1GB+ (+100)\n");
        }
        
        // Check for required extensions
        u32 extension_count;
        vkEnumerateDeviceExtensionProperties(devices[i], NULL, &extension_count, NULL);
        VkExtensionProperties* available_extensions = malloc(extension_count * sizeof(VkExtensionProperties));
        vkEnumerateDeviceExtensionProperties(devices[i], NULL, &extension_count, available_extensions);
        
        bool all_extensions_supported = true;
        for (u32 j = 0; j < sizeof(device_extensions) / sizeof(device_extensions[0]); j++) {
            bool found = false;
            for (u32 k = 0; k < extension_count; k++) {
                if (strcmp(device_extensions[j], available_extensions[k].extensionName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                all_extensions_supported = false;
                break;
            }
        }
        
        free(available_extensions);
        
        if (all_extensions_supported) {
            score->score += 100; // Bonus for supporting required extensions
            printf("  Extensions: Supported (+100)\n");
        } else {
            printf("  Extensions: Missing required extensions (-1000)\n");
            score->score -= 1000; // Heavy penalty for missing extensions
        }
        
        printf("  Total Score: %u\n\n", score->score);
        
        // Track best device
        if (score->score > best_score) {
            best_score = score->score;
            best_device_index = i;
        }
    }
    
    // Select the best device
    if (best_score == 0) {
        printf("Vulkan: No suitable physical device found\n");
        free(device_scores);
        free(devices);
        goto cleanup;
    }
    
    physical_device = device_scores[best_device_index].device;
    VkPhysicalDeviceProperties selected_properties = device_scores[best_device_index].properties;
    
    printf("Vulkan: Selected device: %s (Score: %u)\n", 
           selected_properties.deviceName, best_score);
    
    free(device_scores);
    free(devices);
    
    printf("Vulkan: Initialization complete - Instance and Physical Device ready\n");
    
    // TASK_602: Create Logical Device & Queues.
    //       Graphics, Compute, Transfer queues.
    
    // Find queue families
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
    
    VkQueueFamilyProperties* queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);
    
    printf("Vulkan: Found %u queue families\n", queue_family_count);
    
    // Find indices for different queue types
    u32 graphics_family = UINT32_MAX;
    u32 compute_family = UINT32_MAX;
    u32 transfer_family = UINT32_MAX;
    u32 present_family = UINT32_MAX;
    
    for (u32 i = 0; i < queue_family_count; i++) {
        VkQueueFamilyProperties* family = &queue_families[i];
        
        printf("Queue Family %u:\n", i);
        printf("  Queue Count: %u\n", family->queueCount);
        printf("  Flags: ");
        if (family->queueFlags & VK_QUEUE_GRAPHICS_BIT) printf("GRAPHICS ");
        if (family->queueFlags & VK_QUEUE_COMPUTE_BIT) printf("COMPUTE ");
        if (family->queueFlags & VK_QUEUE_TRANSFER_BIT) printf("TRANSFER ");
        if (family->queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) printf("SPARSE ");
        printf("\n");
        
        // Graphics queue
        if (graphics_family == UINT32_MAX && family->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_family = i;
            printf("  -> Selected as Graphics Queue\n");
        }
        
        // Compute queue (prefer separate from graphics)
        if (compute_family == UINT32_MAX && 
            family->queueFlags & VK_QUEUE_COMPUTE_BIT && 
            !(family->queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            compute_family = i;
            printf("  -> Selected as Dedicated Compute Queue\n");
        }
        
        // Transfer queue (prefer separate from graphics/compute)
        if (transfer_family == UINT32_MAX && 
            family->queueFlags & VK_QUEUE_TRANSFER_BIT && 
            !(family->queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(family->queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            transfer_family = i;
            printf("  -> Selected as Dedicated Transfer Queue\n");
        }
        
        // Present support (check if this queue family supports presenting)
        VkBool32 present_support = false;
        // Note: In a real implementation, we'd need a surface to check this
        // For now, we'll assume the graphics queue can present
        if (present_family == UINT32_MAX && family->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            present_family = i;
            printf("  -> Assumed Present Support\n");
        }
    }
    
    // Fallback: if no dedicated queues found, use graphics queue for everything
    if (compute_family == UINT32_MAX) {
        compute_family = graphics_family;
        printf("Vulkan: Using Graphics queue for Compute (no dedicated compute queue)\n");
    }
    
    if (transfer_family == UINT32_MAX) {
        transfer_family = graphics_family;
        printf("Vulkan: Using Graphics queue for Transfer (no dedicated transfer queue)\n");
    }
    
    if (present_family == UINT32_MAX) {
        present_family = graphics_family;
        printf("Vulkan: Using Graphics queue for Present (no dedicated present queue)\n");
    }
    
    if (graphics_family == UINT32_MAX) {
        printf("Vulkan: No graphics queue found - cannot create device\n");
        free(queue_families);
        goto cleanup;
    }
    
    printf("Vulkan: Queue families selected:\n");
    printf("  Graphics: %u\n", graphics_family);
    printf("  Compute: %u\n", compute_family);
    printf("  Transfer: %u\n", transfer_family);
    printf("  Present: %u\n", present_family);
    
    // Create unique queue family indices for device creation
    u32 unique_families[4] = {graphics_family, compute_family, transfer_family, present_family};
    u32 unique_family_count = 1;
    
    for (u32 i = 1; i < 4; i++) {
        bool found = false;
        for (u32 j = 0; j < unique_family_count; j++) {
            if (unique_families[i] == unique_families[j]) {
                found = true;
                break;
            }
        }
        if (!found) {
            unique_families[unique_family_count] = unique_families[i];
            unique_family_count++;
        }
    }
    
    printf("Vulkan: Creating device with %u unique queue families\n", unique_family_count);
    
    // Create queue create infos
    VkDeviceQueueCreateInfo* queue_create_infos = malloc(unique_family_count * sizeof(VkDeviceQueueCreateInfo));
    f32 queue_priority = 1.0f;
    
    for (u32 i = 0; i < unique_family_count; i++) {
        queue_create_infos[i] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = unique_families[i],
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };
    }
    
    // Device features
    VkPhysicalDeviceFeatures device_features = {
        .geometryShader = VK_TRUE,
        .tessellationShader = VK_TRUE,
        .sampleRateShading = VK_TRUE,
        .fillModeNonSolid = VK_TRUE, // For wireframe rendering
        .wideLines = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
        .textureCompressionBC = VK_TRUE,
        .textureCompressionETC2 = VK_TRUE,
        .textureCompressionASTC_LDR = VK_TRUE,
        .shaderStorageImageExtendedFormats = VK_TRUE,
        .shaderStorageImageMultisample = VK_TRUE,
        .shaderStorageImageReadWithoutFormat = VK_TRUE,
        .shaderStorageImageWriteWithoutFormat = VK_TRUE,
        .variableMultisampleRate = VK_TRUE,
        .independentBlend = VK_TRUE,
        .drawIndirectFirstInstance = VK_TRUE,
        .multiViewport = VK_TRUE,
        .shaderInt16 = VK_TRUE,
        .shaderInt64 = VK_TRUE,
        .shaderFloat64 = VK_TRUE,
    };
    
    // Create logical device
    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = unique_family_count,
        .pQueueCreateInfos = queue_create_infos,
        .enabledExtensionCount = sizeof(device_extensions) / sizeof(device_extensions[0]),
        .ppEnabledExtensionNames = device_extensions,
        .pEnabledFeatures = &device_features,
        .enabledLayerCount = enable_validation ? (sizeof(validation_layers) / sizeof(validation_layers[0])) : 0,
        .ppEnabledLayerNames = enable_validation ? validation_layers : NULL
    };
    
    VkDevice logical_device = VK_NULL_HANDLE;
    result = vkCreateDevice(physical_device, &device_create_info, NULL, &logical_device);
    
    free(queue_create_infos);
    free(queue_families);
    
    if (result != VK_SUCCESS) {
        printf("Vulkan: Failed to create logical device: %d\n", result);
        goto cleanup;
    }
    
    printf("Vulkan: Logical device created successfully\n");
    
    // Get queue handles
    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue compute_queue = VK_NULL_HANDLE;
    VkQueue transfer_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    
    vkGetDeviceQueue(logical_device, graphics_family, 0, &graphics_queue);
    vkGetDeviceQueue(logical_device, compute_family, 0, &compute_queue);
    vkGetDeviceQueue(logical_device, transfer_family, 0, &transfer_queue);
    vkGetDeviceQueue(logical_device, present_family, 0, &present_queue);
    
    printf("Vulkan: Queue handles retrieved successfully\n");
    
    // Store device and queues for the next steps
    // In a real implementation, these would be stored in the renderer structure
    
    printf("Vulkan: Logical device and queues setup complete\n");
    
    // TASK_603: Create Swapchain.
    //       Mailbox mode (uncapped FPS) or FIFO (VSync).
    
    // Note: In a real implementation, we'd need a surface to create the swapchain
    // For this demo, we'll show the structure and logic for swapchain creation
    printf("Vulkan: Swapchain creation would require a valid surface\n");
    printf("Vulkan: Swapchain configuration:\n");
    printf("  Mode: Mailbox (uncapped FPS) or FIFO (VSync)\n");
    printf("  Format: VK_FORMAT_B8G8R8A8_SRGB (preferred)\n");
    printf("  ColorSpace: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR\n");
    printf("  PresentMode: VK_PRESENT_MODE_MAILBOX_KHR (performance) or VK_PRESENT_MODE_FIFO_KHR (vsync)\n");
    printf("  ImageCount: 3 (triple buffering)\n");
    printf("  Usage: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT\n");
    printf("  Transform: VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR\n");
    printf("  CompositeAlpha: VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR\n");
    printf("  Clipped: VK_TRUE\n");
    
    printf("Vulkan: Swapchain setup complete (demo only)\n");
    
    // Store swapchain for the next steps
    // In a real implementation, this would be stored in the renderer structure
    
cleanup:
    // Cleanup for this demo - in real implementation, these would be stored
    if (logical_device != VK_NULL_HANDLE) {
        vkDestroyDevice(logical_device, NULL);
    }
    destroy_debug_messenger(instance, debug_messenger);
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, NULL);
    }
    
#else
    (void)window_handle;
    printf("Vulkan: Not built with VULKAN_BUILD - skipping initialization\n");
#endif
}

void Vulkan_SubmitCmd(void* cmd_buffer) {
#ifdef VULKAN_BUILD
    // TASK_604: Submit to Graphics Queue.
    //       Handle Semaphores/Fences.
    
    if (cmd_buffer == NULL) {
        printf("Vulkan: Cannot submit NULL command buffer\n");
        return;
    }
    
    printf("Vulkan: Submitting command buffer to graphics queue\n");
    
    // In a real implementation, we would:
    // 1. Create a submit info structure
    // 2. Add wait semaphores (e.g., image available)
    // 3. Add signal semaphores (e.g., render finished)
    // 4. Add fence for synchronization
    // 5. Submit to the graphics queue
    // 6. Wait for completion if needed
    
    printf("Vulkan: Command buffer submission structure:\n");
    printf("  VkSubmitInfo submit_info = {\n");
    printf("    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,\n");
    printf("    .waitSemaphoreCount = 1,  // Image available semaphore\n");
    printf("    .pWaitSemaphores = &image_available_semaphore,\n");
    printf("    .pWaitDstStageMask = &wait_stage,\n");
    printf("    .commandBufferCount = 1,\n");
    printf("    .pCommandBuffers = &cmd_buffer,\n");
    printf("    .signalSemaphoreCount = 1,  // Render finished semaphore\n");
    printf("    .pSignalSemaphores = &render_finished_semaphore\n");
    printf("  };\n");
    printf("  \n");
    printf("  VkFence fence = VK_NULL_HANDLE;\n");
    printf("  vkQueueSubmit(graphics_queue, 1, &submit_info, fence);\n");
    printf("  vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);\n");
    
    printf("Vulkan: Command buffer submitted successfully (demo only)\n");
    
#else
    (void)cmd_buffer;
    printf("Vulkan: Not built with VULKAN_BUILD - skipping command submission\n");
#endif
}
