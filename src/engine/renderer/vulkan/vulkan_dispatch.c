// Vulkan Dispatch Table Implementation
// Dynamic loading of Vulkan functions for cross-version compatibility

#include <renderer/vulkan_dispatch.h>

#ifdef VULKAN_BUILD

#include <core/logger.h>
#include <string.h>

// Platform-specific dynamic library loading
#if defined(_WIN32)
    #include <windows.h>
    typedef HMODULE LibHandle;
    #define LOAD_LIB(name) LoadLibraryA(name)
    #define GET_PROC(lib, name) GetProcAddress((HMODULE)lib, name)
    #define FREE_LIB(lib) FreeLibrary((HMODULE)lib)
#else
    #include <dlfcn.h>
    typedef void* LibHandle;
    #define LOAD_LIB(name) dlopen(name, RTLD_NOW | RTLD_LOCAL)
    #define GET_PROC(lib, name) dlsym(lib, name)
    #define FREE_LIB(lib) dlclose(lib)
#endif

// Global dispatch table and library handle
VulkanDispatchTable g_vk_dispatch = {0};
static LibHandle g_vulkan_lib = NULL;
static PFN_vkGetInstanceProcAddr g_vkGetInstanceProcAddr = NULL;

bool vulkan_dispatch_load(void) {
    // Load Vulkan library
    #if defined(_WIN32)
        const char* lib_name = "vulkan-1.dll";
    #elif defined(__APPLE__)
        const char* lib_name = "libvulkan.1.dylib";  // Or MoltenVK
    #else
        const char* lib_name = "libvulkan.so.1";
    #endif
    
    g_vulkan_lib = LOAD_LIB(lib_name);
    if (!g_vulkan_lib) {
        LOG_ERROR("Failed to load Vulkan library: %s", lib_name);
        return false;
    }
    
    // Load vkGetInstanceProcAddr (the only function we need to load manually)
    g_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GET_PROC(g_vulkan_lib, "vkGetInstanceProcAddr");
    if (!g_vkGetInstanceProcAddr) {
        LOG_ERROR("Failed to load vkGetInstanceProcAddr");
        FREE_LIB(g_vulkan_lib);
        return false;
    }
    
    LOG_INFO("Vulkan library loaded: %s", lib_name);
    return true;
}

bool vulkan_dispatch_load_instance(VkInstance instance) {
    if (!g_vkGetInstanceProcAddr) {
        LOG_ERROR("Vulkan library not loaded");
        return false;
    }
    
    #define LOAD_INSTANCE_FUNC(name) \
        g_vk_dispatch.name = (PFN_vk##name)g_vkGetInstanceProcAddr(instance, "vk"#name); \
        if (!g_vk_dispatch.name) { \
            LOG_WARN("Failed to load vk"#name); \
        }
    
    // Load instance functions
    LOAD_INSTANCE_FUNC(DestroyInstance);
    LOAD_INSTANCE_FUNC(EnumeratePhysicalDevices);
    LOAD_INSTANCE_FUNC(GetPhysicalDeviceProperties);
    LOAD_INSTANCE_FUNC(GetPhysicalDeviceProperties2);
    LOAD_INSTANCE_FUNC(GetPhysicalDeviceFeatures);
    LOAD_INSTANCE_FUNC(GetPhysicalDeviceFeatures2);
    LOAD_INSTANCE_FUNC(GetPhysicalDeviceMemoryProperties);
    LOAD_INSTANCE_FUNC(CreateDevice);
    LOAD_INSTANCE_FUNC(EnumerateDeviceExtensionProperties);
    LOAD_INSTANCE_FUNC(GetPhysicalDeviceQueueFamilyProperties);
    
    #undef LOAD_INSTANCE_FUNC
    
    LOG_INFO("Vulkan instance functions loaded");
    return true;
}

bool vulkan_dispatch_load_device(VkDevice device) {
    if (!device) {
        LOG_ERROR("Invalid Vulkan device");
        return false;
    }
    
    // We need vkGetDeviceProcAddr from the instance
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = 
        (PFN_vkGetDeviceProcAddr)g_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkGetDeviceProcAddr");
    
    if (!vkGetDeviceProcAddr) {
        LOG_ERROR("Failed to load vkGetDeviceProcAddr");
        return false;
    }
    
    #define LOAD_DEVICE_FUNC(name) \
        g_vk_dispatch.name = (PFN_vk##name)vkGetDeviceProcAddr(device, "vk"#name); \
        if (!g_vk_dispatch.name) { \
            LOG_WARN("Failed to load vk"#name); \
        }
    
    // Load device functions
    LOAD_DEVICE_FUNC(DestroyDevice);
    LOAD_DEVICE_FUNC(GetDeviceQueue);
    LOAD_DEVICE_FUNC(CreateBuffer);
    LOAD_DEVICE_FUNC(DestroyBuffer);
    LOAD_DEVICE_FUNC(CreateImage);
    LOAD_DEVICE_FUNC(DestroyImage);
    LOAD_DEVICE_FUNC(AllocateMemory);
    LOAD_DEVICE_FUNC(FreeMemory);
    LOAD_DEVICE_FUNC(BindBufferMemory);
    LOAD_DEVICE_FUNC(BindImageMemory);
    LOAD_DEVICE_FUNC(GetBufferMemoryRequirements);
    LOAD_DEVICE_FUNC(GetImageMemoryRequirements);
    
    // Command buffer functions
    LOAD_DEVICE_FUNC(CreateCommandPool);
    LOAD_DEVICE_FUNC(DestroyCommandPool);
    LOAD_DEVICE_FUNC(AllocateCommandBuffers);
    LOAD_DEVICE_FUNC(FreeCommandBuffers);
    LOAD_DEVICE_FUNC(BeginCommandBuffer);
    LOAD_DEVICE_FUNC(EndCommandBuffer);
    LOAD_DEVICE_FUNC(QueueSubmit);
    LOAD_DEVICE_FUNC(QueueWaitIdle);
    
    // Graphics commands
    LOAD_DEVICE_FUNC(CmdBindPipeline);
    LOAD_DEVICE_FUNC(CmdDraw);
    LOAD_DEVICE_FUNC(CmdDrawIndexed);
    LOAD_DEVICE_FUNC(CmdBindVertexBuffers);
    LOAD_DEVICE_FUNC(CmdBindIndexBuffer);
    LOAD_DEVICE_FUNC(CmdSetViewport);
    LOAD_DEVICE_FUNC(CmdSetScissor);
    
    // Synchronization
    LOAD_DEVICE_FUNC(CreateSemaphore);
    LOAD_DEVICE_FUNC(DestroySemaphore);
    LOAD_DEVICE_FUNC(CreateFence);
    LOAD_DEVICE_FUNC(DestroyFence);
    LOAD_DEVICE_FUNC(WaitForFences);
    LOAD_DEVICE_FUNC(ResetFences);
    
    // Dynamic Rendering - Try VK 1.3 core first, then VK 1.2 extension
    g_vk_dispatch.CmdBeginRendering = (PFN_vkCmdBeginRendering)vkGetDeviceProcAddr(device, "vkCmdBeginRendering");
    g_vk_dispatch.CmdEndRendering = (PFN_vkCmdEndRendering)vkGetDeviceProcAddr(device, "vkCmdEndRendering");
    
    if (g_vk_dispatch.CmdBeginRendering && g_vk_dispatch.CmdEndRendering) {
        g_vk_dispatch.has_dynamic_rendering_core = true;
        LOG_INFO("Vulkan 1.3 Dynamic Rendering (core) available");
    } else {
        // Fallback to VK_KHR_dynamic_rendering extension
        g_vk_dispatch.CmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR");
        g_vk_dispatch.CmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR");
        
        if (g_vk_dispatch.CmdBeginRenderingKHR && g_vk_dispatch.CmdEndRenderingKHR) {
            g_vk_dispatch.has_dynamic_rendering_ext = true;
            LOG_INFO("VK_KHR_dynamic_rendering extension available");
        } else {
            LOG_WARN("Dynamic Rendering not available (neither core nor extension)");
        }
    }
    
    #undef LOAD_DEVICE_FUNC
    
    LOG_INFO("Vulkan device functions loaded");
    return true;
}

void vulkan_dispatch_unload(void) {
    if (g_vulkan_lib) {
        FREE_LIB(g_vulkan_lib);
        g_vulkan_lib = NULL;
    }
    memset(&g_vk_dispatch, 0, sizeof(g_vk_dispatch));
    LOG_INFO("Vulkan dispatch table unloaded");
}

bool vulkan_has_dynamic_rendering(void) {
    return g_vk_dispatch.has_dynamic_rendering_core || g_vk_dispatch.has_dynamic_rendering_ext;
}

bool vulkan_has_buffer_device_address(void) {
    return g_vk_dispatch.has_buffer_device_address;
}

bool vulkan_has_ray_query(void) {
    return g_vk_dispatch.has_ray_query;
}

#endif // VULKAN_BUILD
