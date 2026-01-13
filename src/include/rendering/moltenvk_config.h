// MoltenVK Configuration and Feature Detection
// Ensures compatibility with Apple Silicon via MoltenVK translation layer

#ifndef MOLTENVK_CONFIG_H
#define MOLTENVK_CONFIG_H

#include "include/common.h"

#ifdef VULKAN_BUILD

#include <include/rendering/vulkan.h>

// MoltenVK-specific configuration
typedef struct {
    bool is_moltenvk;
    bool supports_portability_subset;
    bool supports_geometry_shaders;
    bool supports_tessellation;
    bool supports_ray_query;
    bool supports_ray_tracing_pipeline;
    
    // Alignment requirements (stricter on Metal)
    u64 min_uniform_buffer_offset_alignment;
    u64 min_storage_buffer_offset_alignment;
    u64 buffer_device_address_alignment;
    
    // Feature workarounds
    bool use_compute_for_geometry;  // Replace geometry shaders with compute
    bool use_ray_query_only;         // Avoid ray tracing pipeline
    
} MoltenVKConfig;

// Detect if running on MoltenVK
bool moltenvk_detect(VkPhysicalDevice physical_device);

// Initialize MoltenVK configuration
bool moltenvk_init(VkInstance instance, VkPhysicalDevice physical_device, MoltenVKConfig* config);

// Request required extensions for MoltenVK
const char** moltenvk_get_required_extensions(u32* count);

// Query feature support
bool moltenvk_supports_feature(MoltenVKConfig* config, const char* feature_name);

// Get alignment requirements
u64 moltenvk_get_min_buffer_alignment(MoltenVKConfig* config);

// Print configuration
void moltenvk_print_config(MoltenVKConfig* config);

#endif // VULKAN_BUILD

#endif // MOLTENVK_CONFIG_H
