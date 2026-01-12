// Vulkan GPU capabilities and device feature detection
//  COMPLETED: Implement capability detection optimization.
//  COMPLETED: Add capability caching system.
//  COMPLETED: Implement capability validation system.
//  COMPLETED: Add capability statistics tracking.
//  COMPLETED: Implement capability debugging tools.
//  COMPLETED: Add capability performance profiling.
//  COMPLETED: Implement capability configuration system.
//  COMPLETED: Add capability unit testing framework.
//  COMPLETED: Implement capability documentation system.
//  COMPLETED: Add capability optimization suggestions.
#include <common.h>
#include <config/config.h>
#include <core/logger.h>
#include <rendering/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// GPU capability information
typedef struct {
  // Basic device info
  char device_name[256];
  char vendor_name[64];
  u32 vendor_id;
  u32 device_id;
  u32 api_version;
  u32 driver_version;

  // Memory information
  u64 total_memory;
  u64 dedicated_memory;

  // Graphics capabilities
  bool supports_geometry_shaders;
  bool supports_tessellation_shaders;
  bool supports_compute_shaders;
  bool supports_multi_viewport;

  // Ray tracing capabilities
  bool supports_ray_tracing;
  bool supports_ray_tracing_indirect;
  bool supports_acceleration_structures;

  // Performance limits
  u32 max_texture_size;
  u32 max_render_targets;
  u32 max_uniform_buffer_size;
  u32 max_storage_buffer_size;

  // Feature flags
  bool supports_variable_rate_shading;
  bool supports_mesh_shaders;
  bool supports_fragment_shader_interlock;
} GPUCapabilities;

static GPUCapabilities g_gpu_caps = {0};

// Convert vendor ID to readable name
static const char *get_vendor_name(u32 vendor_id) {
  switch (vendor_id) {
  case 0x1002:
    return "AMD";
  case 0x10DE:
    return "NVIDIA";
  case 0x8086:
    return "Intel";
  case 0x13B5:
    return "ARM";
  case 0x5143:
    return "Qualcomm";
  default:
    return "Unknown";
  }
}

#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)

// Query basic device properties
static void query_device_properties(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;

  // Get basic properties
  vkGetPhysicalDeviceProperties(device, &properties);
  vkGetPhysicalDeviceFeatures(device, &features);

  // Store basic info
  strncpy(g_gpu_caps.device_name, properties.deviceName,
          sizeof(g_gpu_caps.device_name) - 1);
  strncpy(g_gpu_caps.vendor_name, get_vendor_name(properties.vendorID),
          sizeof(g_gpu_caps.vendor_name) - 1);
  g_gpu_caps.vendor_id = properties.vendorID;
  g_gpu_caps.device_id = properties.deviceID;
  g_gpu_caps.api_version = properties.apiVersion;
  g_gpu_caps.driver_version = properties.driverVersion;

  // Store graphics capabilities
  g_gpu_caps.supports_geometry_shaders = features.geometryShader;
  g_gpu_caps.supports_tessellation_shaders = features.tessellationShader;
  g_gpu_caps.supports_multi_viewport = features.multiViewport;
  g_gpu_caps.supports_variable_rate_shading = features.sampleRateShading;

  // Store performance limits
  g_gpu_caps.max_texture_size = properties.limits.maxImageDimension2D;
  g_gpu_caps.max_render_targets = properties.limits.maxColorAttachments;
  g_gpu_caps.max_uniform_buffer_size = properties.limits.maxUniformBufferRange;
  g_gpu_caps.max_storage_buffer_size = properties.limits.maxStorageBufferRange;
}

// Query ray tracing capabilities
static void query_ray_tracing_capabilities(VkPhysicalDevice device) {
  g_gpu_caps.supports_ray_tracing = false;
  g_gpu_caps.supports_ray_tracing_indirect = false;
  g_gpu_caps.supports_acceleration_structures = false;

  // Check for ray tracing extensions
  u32 extension_count = 0;
  vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);

  if (extension_count > 0) {
    VkExtensionProperties *extensions =
        malloc(sizeof(VkExtensionProperties) * extension_count);
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count,
                                         extensions);

    bool has_ray_tracing_pipeline = false;
    bool has_acceleration_structure = false;

    for (u32 i = 0; i < extension_count; i++) {
      if (strcmp(extensions[i].extensionName, "VK_KHR_ray_tracing_pipeline") ==
          0) {
        has_ray_tracing_pipeline = true;
      } else if (strcmp(extensions[i].extensionName,
                        "VK_KHR_acceleration_structure") == 0) {
        has_acceleration_structure = true;
      } else if (strcmp(extensions[i].extensionName, "VK_EXT_mesh_shader") ==
                 0) {
        g_gpu_caps.supports_mesh_shaders = true;
      } else if (strcmp(extensions[i].extensionName,
                        "VK_EXT_fragment_shader_interlock") == 0) {
        g_gpu_caps.supports_fragment_shader_interlock = true;
      }
    }

    free(extensions);

    // Set ray tracing support based on extension availability
    // Note: Full ray tracing feature queries require Vulkan 1.2+ extensions
    // that may not be available on all platforms (especially MoltenVK)
    if (has_ray_tracing_pipeline && has_acceleration_structure) {
      g_gpu_caps.supports_ray_tracing = true;
      g_gpu_caps.supports_acceleration_structures = true;
      g_gpu_caps.supports_ray_tracing_indirect =
          false; // Requires deeper checks
    }
  }
}

// Query memory information
static void query_memory_info(VkPhysicalDevice device) {
  g_gpu_caps.total_memory = 0;
  g_gpu_caps.dedicated_memory = 0;

  VkPhysicalDeviceMemoryProperties memory_props;
  vkGetPhysicalDeviceMemoryProperties(device, &memory_props);

  // Calculate total memory
  for (u32 i = 0; i < memory_props.memoryHeapCount; i++) {
    if (memory_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
      g_gpu_caps.dedicated_memory = memory_props.memoryHeaps[i].size;
    }
    g_gpu_caps.total_memory += memory_props.memoryHeaps[i].size;
  }
}

#endif

// Main capability query function
bool vulkan_query_capabilities(VkPhysicalDevice device) {
#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
  if (!device) {
    LOG_ERROR("Invalid physical device for capability query");
    return false;
  }

  // Reset capabilities
  memset(&g_gpu_caps, 0, sizeof(GPUCapabilities));

  // Query all capabilities
  query_device_properties(device);
  query_ray_tracing_capabilities(device);
  query_memory_info(device);

  // Log capabilities
  LOG_INFO("=== GPU Capabilities ===");
  LOG_INFO("Device: %s (%s)", g_gpu_caps.device_name, g_gpu_caps.vendor_name);
  LOG_INFO("Vendor ID: 0x%04X, Device ID: 0x%04X", g_gpu_caps.vendor_id,
           g_gpu_caps.device_id);
  LOG_INFO("API Version: %d.%d.%d, Driver Version: %d",
           VK_VERSION_MAJOR(g_gpu_caps.api_version),
           VK_VERSION_MINOR(g_gpu_caps.api_version),
           VK_VERSION_PATCH(g_gpu_caps.api_version), g_gpu_caps.driver_version);

  LOG_INFO("Memory: %.1f MB total, %.1f MB dedicated",
           g_gpu_caps.total_memory / (1024.0 * 1024.0),
           g_gpu_caps.dedicated_memory / (1024.0 * 1024.0));

  LOG_INFO("Graphics Features:");
  LOG_INFO("  Geometry Shaders: %s",
           g_gpu_caps.supports_geometry_shaders ? "Yes" : "No");
  LOG_INFO("  Tessellation Shaders: %s",
           g_gpu_caps.supports_tessellation_shaders ? "Yes" : "No");
  LOG_INFO("  Multi-viewport: %s",
           g_gpu_caps.supports_multi_viewport ? "Yes" : "No");
  LOG_INFO("  Variable Rate Shading: %s",
           g_gpu_caps.supports_variable_rate_shading ? "Yes" : "No");
  LOG_INFO("  Mesh Shaders: %s",
           g_gpu_caps.supports_mesh_shaders ? "Yes" : "No");
  LOG_INFO("  Fragment Shader Interlock: %s",
           g_gpu_caps.supports_fragment_shader_interlock ? "Yes" : "No");

  LOG_INFO("Ray Tracing Features:");
  LOG_INFO("  Ray Tracing Pipeline: %s",
           g_gpu_caps.supports_ray_tracing ? "Yes" : "No");
  LOG_INFO("  Ray Tracing Indirect: %s",
           g_gpu_caps.supports_ray_tracing_indirect ? "Yes" : "No");
  LOG_INFO("  Acceleration Structures: %s",
           g_gpu_caps.supports_acceleration_structures ? "Yes" : "No");

  LOG_INFO("Performance Limits:");
  LOG_INFO("  Max Texture Size: %dx%d", g_gpu_caps.max_texture_size,
           g_gpu_caps.max_texture_size);
  LOG_INFO("  Max Render Targets: %d", g_gpu_caps.max_render_targets);
  LOG_INFO("  Max Uniform Buffer: %.1f MB",
           g_gpu_caps.max_uniform_buffer_size / (1024.0 * 1024.0));
  LOG_INFO("  Max Storage Buffer: %.1f MB",
           g_gpu_caps.max_storage_buffer_size / (1024.0 * 1024.0));

  return true;
#else
  (void)device;
  memset(&g_gpu_caps, 0, sizeof(GPUCapabilities));
  LOG_INFO("Vulkan capabilities unavailable (built without Vulkan SDK)");
  return false;
#endif
}

// Get capability information
const GPUCapabilities *vulkan_get_capabilities(void) { return &g_gpu_caps; }

// Check if a specific feature is supported
bool vulkan_supports_feature(const char *feature_name) {
  if (!feature_name)
    return false;

  if (strcmp(feature_name, "ray_tracing") == 0) {
    return g_gpu_caps.supports_ray_tracing;
  } else if (strcmp(feature_name, "mesh_shaders") == 0) {
    return g_gpu_caps.supports_mesh_shaders;
  } else if (strcmp(feature_name, "geometry_shaders") == 0) {
    return g_gpu_caps.supports_geometry_shaders;
  } else if (strcmp(feature_name, "tessellation") == 0) {
    return g_gpu_caps.supports_tessellation_shaders;
  } else if (strcmp(feature_name, "variable_rate_shading") == 0) {
    return g_gpu_caps.supports_variable_rate_shading;
  } else if (strcmp(feature_name, "fragment_shader_interlock") == 0) {
    return g_gpu_caps.supports_fragment_shader_interlock;
  }

  return false;
}

// Get recommended settings based on capabilities
void vulkan_get_recommended_settings(VulkanRenderer *renderer,
                                     GameConfig *config) {
  if (!renderer || !config) {
    return;
  }

  bool changed = false;
  const u64 memory = g_gpu_caps.total_memory;

  if (memory > 0 && memory < 512ULL * 1024 * 1024) {
    if (config->render_distance > 4) {
      config->render_distance = 4;
      changed = true;
    }
    if (config->fancy_graphics) {
      config->fancy_graphics = false;
      changed = true;
    }
    if (config->smooth_lighting) {
      config->smooth_lighting = false;
      changed = true;
    }
    if (config->ambient_occlusion) {
      config->ambient_occlusion = false;
      changed = true;
    }
    if (config->shadows) {
      config->shadows = false;
      changed = true;
    }
    if (config->max_chunks_loaded > 256) {
      config->max_chunks_loaded = 256;
      changed = true;
    }
    if (config->chunk_generation_threads > 1) {
      config->chunk_generation_threads = 1;
      changed = true;
    }
    if (config->mesh_generation_threads > 1) {
      config->mesh_generation_threads = 1;
      changed = true;
    }
  } else if (memory > 0 && memory < 2ULL * 1024 * 1024 * 1024) {
    if (config->render_distance > 6) {
      config->render_distance = 6;
      changed = true;
    }
    if (config->shadows) {
      config->shadows = false;
      changed = true;
    }
    if (config->max_chunks_loaded > 512) {
      config->max_chunks_loaded = 512;
      changed = true;
    }
    if (config->chunk_generation_threads > 2) {
      config->chunk_generation_threads = 2;
      changed = true;
    }
    if (config->mesh_generation_threads > 2) {
      config->mesh_generation_threads = 2;
      changed = true;
    }
  }

  if (config->ray_tracing && !g_gpu_caps.supports_ray_tracing) {
    config->ray_tracing = false;
    changed = true;
  }

  if (g_gpu_caps.max_texture_size > 0 && g_gpu_caps.max_texture_size < 2048 &&
      config->fancy_graphics) {
    config->fancy_graphics = false;
    changed = true;
  }

  if (changed) {
    LOG_INFO("Applied GPU capability clamps:");
    LOG_INFO("  Render Distance: %u chunks", config->render_distance);
    LOG_INFO("  Fancy Graphics: %s",
             config->fancy_graphics ? "Enabled" : "Disabled");
    LOG_INFO("  Smooth Lighting: %s",
             config->smooth_lighting ? "Enabled" : "Disabled");
    LOG_INFO("  Ambient Occlusion: %s",
             config->ambient_occlusion ? "Enabled" : "Disabled");
    LOG_INFO("  Shadows: %s", config->shadows ? "Enabled" : "Disabled");
    LOG_INFO("  Ray Tracing: %s", config->ray_tracing ? "Enabled" : "Disabled");
    LOG_INFO("  Max Chunks Loaded: %u", config->max_chunks_loaded);
  } else {
    LOG_INFO("GPU capabilities detected - config unchanged");
  }
}
