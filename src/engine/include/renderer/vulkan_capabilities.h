// include/render/vulkan_capabilities.h
// Vulkan GPU capabilities and device feature detection
#ifndef VULKAN_CAPABILITIES_H
#define VULKAN_CAPABILITIES_H

#include <common.h>
#include "vulkan.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct VulkanRenderer VulkanRenderer;
struct GameConfig;

// GPU capability information structure
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

// VULKAN-CAPS-007: Capability configuration structure
typedef struct {
    bool force_cache_invalidate;
    bool enable_detailed_logging;
    bool enable_performance_profiling;
} VulkanCapabilityConfig;

// VULKAN-CAPS-006: Performance statistics structure
typedef struct {
    u64 total_query_time;
    u32 query_count;
    u64 cache_hits;
    u64 cache_misses;
    double average_query_time;
    double cache_hit_rate;
} VulkanCapabilityStats;

// VULKAN-CAPS-010: Optimization suggestions structure
typedef struct {
    bool reduce_texture_quality;
    bool reduce_render_distance;
    bool disable_fancy_graphics;
    bool use_alternative_lighting;
    bool disable_vrs;
    bool enable_caching;
} VulkanOptimizationSuggestions;

// Query GPU capabilities from physical device
bool vulkan_query_capabilities(VkPhysicalDevice device);

// Get current capability information
const GPUCapabilities* vulkan_get_capabilities(void);

// Check if a specific feature is supported
bool vulkan_supports_feature(const char* feature_name);

// Apply recommended settings based on GPU capabilities
void vulkan_get_recommended_settings(VulkanRenderer *renderer, struct GameConfig *config);

// VULKAN-CAPS-003: Capability validation system
bool vulkan_validate_capabilities(VkPhysicalDevice device);

// VULKAN-CAPS-005: Capability debugging tools
void vulkan_debug_capabilities(void);

// VULKAN-CAPS-007: Capability configuration system
void vulkan_configure_capabilities(const VulkanCapabilityConfig* config);

// VULKAN-CAPS-006: Get performance statistics
void vulkan_get_capability_stats(VulkanCapabilityStats* out_stats);

// VULKAN-CAPS-002: Clear capability cache
void vulkan_clear_capability_cache(void);

// VULKAN-CAPS-010: Get optimization suggestions
void vulkan_get_optimization_suggestions(VulkanOptimizationSuggestions* suggestions);

#ifdef __cplusplus
}
#endif

#endif // VULKAN_CAPABILITIES_H
