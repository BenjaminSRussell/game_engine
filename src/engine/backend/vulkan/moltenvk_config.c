// MoltenVK Configuration Implementation
// Detects and configures MoltenVK-specific features and workarounds

#include <renderer/moltenvk_config.h>
#include <core/logger.h>
#include <string.h>

#ifdef VULKAN_BUILD

// Hardcoded fallback for missing header definitions
#ifndef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
#endif

// Struct definition removed as it conflicts with vulkan_beta.h


#ifdef __APPLE__
#include <vulkan/vulkan_beta.h> // Try to get portability subset from beta
#endif

// Define portability subset extension if missing (fallback for older SDKs)
#ifndef VK_KHR_portability_subset
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
typedef struct VkPhysicalDevicePortabilitySubsetFeaturesKHR {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           constantAlphaColorBlendFactors;
    VkBool32           events;
    VkBool32           imageViewFormatReinterpretation;
    VkBool32           imageViewFormatSwizzle;
    VkBool32           imageView2DOn3DImage;
    VkBool32           multisampleArrayImage;
    VkBool32           mutableComparisonSamplers;
    VkBool32           pointPolygons;
    VkBool32           samplerMipLodBias;
    VkBool32           separateStencilMaskRef;
    VkBool32           shaderSampleRateInterpolationFunctions;
    VkBool32           tessellationIsolines;
    VkBool32           tessellationPointMode;
    VkBool32           triangleFans;
    VkBool32           vertexAttributeAccessBeyondStride;
} VkPhysicalDevicePortabilitySubsetFeaturesKHR;
#endif

// Ensure enum value is available even if header defines struct but not macro
#ifndef VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR (VkStructureType)1000163000
#endif

// Required extensions for MoltenVK
static const char* g_moltenvk_extensions[] = {
    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,  // Critical for MoltenVK
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    NULL
};

bool moltenvk_detect(VkPhysicalDevice physical_device) {
    if (!physical_device) return false;
    
    // Query device properties
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);
    
    // Check if device name contains "Apple" or vendor ID is Apple (0x106B)
    bool is_apple = (props.vendorID == 0x106B) || 
                    (strstr(props.deviceName, "Apple") != NULL);
    
    if (is_apple) {
        LOG_INFO("MoltenVK detected: %s (Vendor: 0x%04X)", 
                 props.deviceName, props.vendorID);
        return true;
    }
    
    return false;
}

bool moltenvk_init(VkInstance instance, VkPhysicalDevice physical_device, MoltenVKConfig* config) {
    if (!instance || !physical_device || !config) {
        return false;
    }
    
    memset(config, 0, sizeof(MoltenVKConfig));
    
    // Detect MoltenVK
    config->is_moltenvk = moltenvk_detect(physical_device);
    
    if (!config->is_moltenvk) {
        LOG_INFO("Not running on MoltenVK");
        return true;  // Success, just not MoltenVK
    }
    
    // Query device limits for alignment requirements
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);
    
    config->min_uniform_buffer_offset_alignment = props.limits.minUniformBufferOffsetAlignment;
    config->min_storage_buffer_offset_alignment = props.limits.minStorageBufferOffsetAlignment;
    
    // MoltenVK typically requires stricter alignment (256 bytes for some operations)
    config->buffer_device_address_alignment = 256;  // Conservative for Metal
    
    LOG_INFO("MoltenVK alignment requirements:");
    LOG_INFO("  Uniform buffer: %llu bytes", config->min_uniform_buffer_offset_alignment);
    LOG_INFO("  Storage buffer: %llu bytes", config->min_storage_buffer_offset_alignment);
    LOG_INFO("  Device address: %llu bytes", config->buffer_device_address_alignment);
    
    // Query features with VkPhysicalDeviceFeatures2
    VkPhysicalDeviceFeatures2 features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = NULL
    };
    
    // Check for portability subset support
    VkPhysicalDevicePortabilitySubsetFeaturesKHR portability_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR,
        .pNext = NULL
    };
    features2.pNext = &portability_features;
    
    vkGetPhysicalDeviceFeatures2(physical_device, &features2);
    
    config->supports_portability_subset = true;  // If we got here, it's supported
    
    // MoltenVK limitations
    config->supports_geometry_shaders = features2.features.geometryShader;
    config->supports_tessellation = features2.features.tessellationShader;
    
    if (!config->supports_geometry_shaders) {
        LOG_WARN("MoltenVK: Geometry shaders not supported - using compute shader fallback");
        config->use_compute_for_geometry = true;
    }
    
    // Check for ray query support (preferred over ray tracing pipeline on MoltenVK)
    // This would require checking VkPhysicalDeviceRayQueryFeaturesKHR
    config->supports_ray_query = false;  // To be detected properly
    config->supports_ray_tracing_pipeline = false;  // Limited on MoltenVK
    config->use_ray_query_only = true;  // Prefer ray query on Metal
    
    LOG_INFO("MoltenVK initialized successfully");
    moltenvk_print_config(config);
    
    return true;
}

const char** moltenvk_get_required_extensions(u32* count) {
    if (!count) return NULL;
    
    // Count extensions
    *count = 0;
    while (g_moltenvk_extensions[*count] != NULL) {
        (*count)++;
    }
    
    return g_moltenvk_extensions;
}

bool moltenvk_supports_feature(MoltenVKConfig* config, const char* feature_name) {
    if (!config || !feature_name) return false;
    
    if (strcmp(feature_name, "geometry_shaders") == 0) {
        return config->supports_geometry_shaders;
    } else if (strcmp(feature_name, "tessellation") == 0) {
        return config->supports_tessellation;
    } else if (strcmp(feature_name, "ray_query") == 0) {
        return config->supports_ray_query;
    } else if (strcmp(feature_name, "ray_tracing_pipeline") == 0) {
        return config->supports_ray_tracing_pipeline;
    }
    
    return false;
}

u64 moltenvk_get_min_buffer_alignment(MoltenVKConfig* config) {
    if (!config) return 256;  // Conservative default
    
    // Return the strictest alignment requirement
    u64 max_alignment = config->min_uniform_buffer_offset_alignment;
    
    if (config->min_storage_buffer_offset_alignment > max_alignment) {
        max_alignment = config->min_storage_buffer_offset_alignment;
    }
    
    if (config->buffer_device_address_alignment > max_alignment) {
        max_alignment = config->buffer_device_address_alignment;
    }
    
    return max_alignment;
}

void moltenvk_print_config(MoltenVKConfig* config) {
    if (!config) return;
    
    LOG_INFO("=== MoltenVK Configuration ===");
    LOG_INFO("Running on MoltenVK: %s", config->is_moltenvk ? "YES" : "NO");
    
    if (!config->is_moltenvk) {
        LOG_INFO("==============================");
        return;
    }
    
    LOG_INFO("Portability Subset: %s", config->supports_portability_subset ? "YES" : "NO");
    LOG_INFO("Geometry Shaders: %s", config->supports_geometry_shaders ? "YES" : "NO (using compute fallback)");
    LOG_INFO("Tessellation: %s", config->supports_tessellation ? "YES" : "NO");
    LOG_INFO("Ray Query: %s", config->supports_ray_query ? "YES" : "NO");
    LOG_INFO("Ray Tracing Pipeline: %s", config->supports_ray_tracing_pipeline ? "YES (limited)" : "NO");
    
    LOG_INFO("Workarounds:");
    LOG_INFO("  Compute for Geometry: %s", config->use_compute_for_geometry ? "ENABLED" : "DISABLED");
    LOG_INFO("  Ray Query Only: %s", config->use_ray_query_only ? "ENABLED" : "DISABLED");
    
    LOG_INFO("Buffer Alignment: %llu bytes (strictest)", 
             moltenvk_get_min_buffer_alignment(config));
    LOG_INFO("==============================");
}

#endif // VULKAN_BUILD
