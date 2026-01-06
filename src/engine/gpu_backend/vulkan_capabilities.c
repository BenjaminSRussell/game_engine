// Vulkan GPU capabilities and device feature detection
// VULKAN-CAPS-001: Implement capability detection optimization
// VULKAN-CAPS-002: Add capability caching system
// VULKAN-CAPS-003: Implement capability validation system
// VULKAN-CAPS-004: Add capability statistics tracking
// VULKAN-CAPS-005: Implement capability debugging tools
// VULKAN-CAPS-006: Add capability performance profiling
// VULKAN-CAPS-007: Implement capability configuration system
// VULKAN-CAPS-008: Add capability unit testing framework
// VULKAN-CAPS-009: Implement capability documentation system
// VULKAN-CAPS-010: Add capability optimization suggestions
#include "../../include/config/config.h"
#include "../../include/core/logger.h"
#include "../../include/render/vulkan.h"
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
  
  // VULKAN-CAPS-004: Capability statistics tracking
  u64 query_time_ms;
  u32 extension_count;
  u32 feature_check_count;
  bool capabilities_cached;
} GPUCapabilities;

static GPUCapabilities g_gpu_caps = {0};

// VULKAN-CAPS-002: Capability caching system
static VkPhysicalDevice g_cached_device = VK_NULL_HANDLE;
static bool g_cache_valid = false;

// VULKAN-CAPS-006: Performance profiling
static struct {
  u64 total_query_time;
  u32 query_count;
  u64 cache_hits;
  u64 cache_misses;
} g_perf_stats = {0};

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
  g_gpu_caps.feature_check_count++;

  // Check for ray tracing extensions
  u32 extension_count = 0;
  vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
  g_gpu_caps.extension_count = extension_count;

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

    // Query ray tracing features if extensions are available
    if (has_ray_tracing_pipeline && has_acceleration_structure) {
      VkPhysicalDeviceRayTracingFeaturesKHR rt_features = {0};
      rt_features.sType =
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;

      VkPhysicalDeviceFeatures2 features2 = {0};
      features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
      features2.pNext = &rt_features;

      vkGetPhysicalDeviceFeatures2(device, &features2);

      g_gpu_caps.supports_ray_tracing = rt_features.rayTracing;
      g_gpu_caps.supports_ray_tracing_indirect =
          rt_features.rayTracingIndirectRays;
      g_gpu_caps.supports_acceleration_structures = has_acceleration_structure;
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

// VULKAN-CAPS-001: Optimized capability query with timing
bool vulkan_query_capabilities(VkPhysicalDevice device) {
#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
  if (!device) {
    LOG_ERROR("Invalid physical device for capability query");
    return false;
  }

  // VULKAN-CAPS-002: Check cache first
  if (g_cache_valid && g_cached_device == device) {
    g_perf_stats.cache_hits++;
    LOG_INFO("Using cached GPU capabilities");
    return true;
  }
  
  g_perf_stats.cache_misses++;
  
  // VULKAN-CAPS-006: Start timing
  u64 start_time = get_current_time_ms();

  // Reset capabilities
  memset(&g_gpu_caps, 0, sizeof(GPUCapabilities));

  // Query all capabilities
  query_device_properties(device);
  query_ray_tracing_capabilities(device);
  query_memory_info(device);
  
  // VULKAN-CAPS-006: End timing
  u64 end_time = get_current_time_ms();
  g_gpu_caps.query_time_ms = end_time - start_time;
  g_perf_stats.total_query_time += g_gpu_caps.query_time_ms;
  g_perf_stats.query_count++;
  
  // VULKAN-CAPS-002: Update cache
  g_cached_device = device;
  g_cache_valid = true;
  g_gpu_caps.capabilities_cached = true;

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
           
  LOG_INFO("Query Statistics:");
  LOG_INFO("  Query Time: %llu ms", g_gpu_caps.query_time_ms);
  LOG_INFO("  Extensions Found: %u", g_gpu_caps.extension_count);
  LOG_INFO("  Feature Checks: %u", g_gpu_caps.feature_check_count);

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

// VULKAN-CAPS-003: Capability validation system
bool vulkan_validate_capabilities(VkPhysicalDevice device) {
#if defined(VULKAN_BUILD) && __has_include(<vulkan/vulkan.h>)
  if (!device) {
    LOG_ERROR("Invalid device for validation");
    return false;
  }
  
  // Validate minimum required capabilities
  if (g_gpu_caps.max_texture_size < 512) {
    LOG_ERROR("GPU texture size too small: %d (minimum: 512)", g_gpu_caps.max_texture_size);
    return false;
  }
  
  if (g_gpu_caps.max_render_targets < 4) {
    LOG_ERROR("GPU render targets too few: %d (minimum: 4)", g_gpu_caps.max_render_targets);
    return false;
  }
  
  if (g_gpu_caps.total_memory < 256ULL * 1024 * 1024) {
    LOG_ERROR("GPU memory too low: %.1f MB (minimum: 256 MB)", 
             g_gpu_caps.total_memory / (1024.0 * 1024.0));
    return false;
  }
  
  LOG_INFO("GPU capabilities validation passed");
  return true;
#else
  (void)device;
  return false;
#endif
}

// VULKAN-CAPS-005: Capability debugging tools
void vulkan_debug_capabilities(void) {
  LOG_INFO("=== GPU Capability Debug Info ===");
  LOG_INFO("Cache Status: %s", g_cache_valid ? "Valid" : "Invalid");
  LOG_INFO("Cached Device: %p", (void*)g_cached_device);
  LOG_INFO("Performance Stats:");
  LOG_INFO("  Total Queries: %u", g_perf_stats.query_count);
  LOG_INFO("  Cache Hits: %llu", g_perf_stats.cache_hits);
  LOG_INFO("  Cache Misses: %llu", g_perf_stats.cache_misses);
  LOG_INFO("  Average Query Time: %.2f ms", 
           g_perf_stats.query_count > 0 ? 
           (double)g_perf_stats.total_query_time / g_perf_stats.query_count : 0.0);
  LOG_INFO("  Cache Hit Rate: %.1f%%", 
           (g_perf_stats.cache_hits + g_perf_stats.cache_misses) > 0 ?
           (double)g_perf_stats.cache_hits / (g_perf_stats.cache_hits + g_perf_stats.cache_misses) * 100.0 : 0.0);
}

// VULKAN-CAPS-007: Capability configuration system
void vulkan_configure_capabilities(const VulkanCapabilityConfig* config) {
  if (!config) return;
  
  // Apply configuration settings
  if (config->force_cache_invalidate) {
    g_cache_valid = false;
    g_cached_device = VK_NULL_HANDLE;
    LOG_INFO("Capability cache invalidated by configuration");
  }
  
  if (config->enable_detailed_logging) {
    LOG_INFO("Detailed capability logging enabled");
  }
}

// VULKAN-CAPS-006: Get performance statistics
void vulkan_get_capability_stats(VulkanCapabilityStats* out_stats) {
  if (!out_stats) return;
  
  out_stats->total_query_time = g_perf_stats.total_query_time;
  out_stats->query_count = g_perf_stats.query_count;
  out_stats->cache_hits = g_perf_stats.cache_hits;
  out_stats->cache_misses = g_perf_stats.cache_misses;
  out_stats->average_query_time = g_perf_stats.query_count > 0 ? 
    (double)g_perf_stats.total_query_time / g_perf_stats.query_count : 0.0;
  out_stats->cache_hit_rate = (g_perf_stats.cache_hits + g_perf_stats.cache_misses) > 0 ?
    (double)g_perf_stats.cache_hits / (g_perf_stats.cache_hits + g_perf_stats.cache_misses) * 100.0 : 0.0;
}

// VULKAN-CAPS-002: Clear capability cache
void vulkan_clear_capability_cache(void) {
  g_cache_valid = false;
  g_cached_device = VK_NULL_HANDLE;
  memset(&g_gpu_caps, 0, sizeof(GPUCapabilities));
  LOG_INFO("Capability cache cleared");
}

// VULKAN-CAPS-010: Get optimization suggestions
void vulkan_get_optimization_suggestions(VulkanOptimizationSuggestions* suggestions) {
  if (!suggestions) return;
  
  memset(suggestions, 0, sizeof(VulkanOptimizationSuggestions));
  
  // Memory-based suggestions
  if (g_gpu_caps.total_memory < 512ULL * 1024 * 1024) {
    suggestions->reduce_texture_quality = true;
    suggestions->reduce_render_distance = true;
    suggestions->disable_fancy_graphics = true;
  }
  
  // Feature-based suggestions
  if (!g_gpu_caps.supports_ray_tracing) {
    suggestions->use_alternative_lighting = true;
  }
  
  if (!g_gpu_caps.supports_variable_rate_shading) {
    suggestions->disable_vrs = true;
  }
  
  // Performance-based suggestions
  if (g_gpu_caps.query_time_ms > 100) {
    suggestions->enable_caching = true;
  }
}
