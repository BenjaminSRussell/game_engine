/*
 * mtl_device_caps.h
 * Metal device capabilities interface
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_DEVICE_CAPS_H
#define PLATFORM_MTL_DEVICE_CAPS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* GPU Family Flags */
typedef enum mtl_gpu_family {
    MTL_GPU_FAMILY_UNKNOWN = 0,
    
    /* Apple Silicon Families */
    MTL_GPU_FAMILY_APPLE_1 = (1 << 0),
    MTL_GPU_FAMILY_APPLE_2 = (1 << 1),
    MTL_GPU_FAMILY_APPLE_3 = (1 << 2),
    MTL_GPU_FAMILY_APPLE_4 = (1 << 3),
    MTL_GPU_FAMILY_APPLE_5 = (1 << 4),
    MTL_GPU_FAMILY_APPLE_6 = (1 << 5),
    MTL_GPU_FAMILY_APPLE_7 = (1 << 6),
    MTL_GPU_FAMILY_APPLE_8 = (1 << 7),
    
    /* Mac Families */
    MTL_GPU_FAMILY_MAC_1 = (1 << 8),
    MTL_GPU_FAMILY_MAC_2 = (1 << 9),
} mtl_gpu_family_t;

/* Feature Level */
typedef enum mtl_feature_level {
    MTL_FEATURE_LEVEL_1_0 = 10,  /* Metal 1.0 - macOS 10.11 */
    MTL_FEATURE_LEVEL_2_0 = 20,  /* Metal 2.0 - macOS 10.13 */
    MTL_FEATURE_LEVEL_2_3 = 23,  /* Metal 2.3 - macOS 10.15 */
    MTL_FEATURE_LEVEL_3_0 = 30,  /* Metal 3.0 - macOS 13.0 */
} mtl_feature_level_t;

/* macOS Version */
typedef struct mtl_os_version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} mtl_os_version_t;

typedef struct mtl_device_caps {
    // Operating System Version
    mtl_os_version_t os_version;
    mtl_feature_level_t feature_level;
    
    // GPU Family
    uint32_t gpu_family_flags;  /* Bitmask of mtl_gpu_family_t */
    
    // Feature Support
    bool supports_raytracing;
    bool supports_mesh_shaders;
    bool supports_indirect_rendering;
    bool supports_variable_rate_shading;
    bool supports_sparse_textures;
    bool supports_argument_buffers;
    bool supports_raster_order_groups;
    bool supports_barycentric_coords;
    bool supports_shader_barycentric_coords;
    bool supports_conservative_rasterization;
    bool supports_tessellation;          // Note: Metal uses compute-based tessellation
    bool supports_primitive_shaders;     // Apple's vertex amplification
    bool supports_sample_rate_shading;
    bool supports_fetch_textures;        // Pull-model texture sampling
    bool supports_read_write_textures;   // Read-write texture support
    bool supports_float16_filtering;     // 16-bit float texture filtering
    
    // Limits - Threads
    uint32_t max_threads_per_group[3]; // width, height, depth
    uint32_t max_total_threadgroup_memory;
    
    // Limits - Buffers
    uint64_t max_buffer_size;
    uint32_t max_argument_buffer_entries;
    
    // Limits - Textures 2D
    uint32_t max_texture_width_2d;
    uint32_t max_texture_height_2d;
    
    // Limits - Textures 3D
    uint32_t max_texture_width_3d;
    uint32_t max_texture_height_3d;
    uint32_t max_texture_depth_3d;
    
    // Limits - Cube & Arrays
    uint32_t max_texture_dimension_cube;
    uint32_t max_texture_layers;
    
    // Texture Features
    bool supports_texture_compression_astc;
    bool supports_texture_compression_bc;
    bool supports_texture_compression_pvrtc;
    bool supports_32bit_msaa;
    bool supports_depth_clip_mode;
    
    // Memory
    bool is_unified_memory;
    bool supports_memoryless_render_targets;
    
    // GPU Properties
    bool is_apple_silicon;
    bool is_discrete_gpu;
    bool is_low_power;
    bool is_headless;
    bool is_removable;  // eGPU detection
    
    // Display Capabilities
    bool supports_hdr;
    bool supports_edr;              // Extended Dynamic Range
    bool supports_wide_color;       // Display P3
    bool supports_promotion;        // Variable refresh (120Hz+)
    uint32_t max_refresh_rate_hz;
    
    // Meta
    char device_name[256];
    uint64_t registry_id;
    uint32_t location_number;       // For multi-GPU systems
    
} mtl_device_caps_t;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Checks if a specific GPU family is supported.
 * @param caps Capabilities structure.
 * @param family GPU family to check.
 * @return true if supported.
 */
static inline bool mtl_caps_supports_family(const mtl_device_caps_t* caps, mtl_gpu_family_t family) {
    return (caps->gpu_family_flags & family) != 0;
}

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_DEVICE_CAPS_H */
