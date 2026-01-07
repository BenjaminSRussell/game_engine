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

typedef struct mtl_device_caps {
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
    
    // Limits
    uint32_t max_threads_per_group[3]; // width, height, depth
    uint32_t max_buffer_size;
    uint32_t max_texture_width_2d;
    uint32_t max_texture_height_2d;
    uint32_t max_texture_width_3d;
    uint32_t max_texture_height_3d;
    uint32_t max_texture_depth_3d;
    uint32_t max_texture_dimension_cube;
    uint32_t max_texture_layers;
    
    // Texture Features
    bool supports_texture_compression_astc;
    bool supports_texture_compression_bc;
    bool supports_texture_compression_pvrtc;
    bool supports_32bit_stencil;
    
    // Memory
    bool is_unified_memory;
    bool supports_memoryless_render_targets;
    
    // GPU Family (simplified enum or flags could be used here)
    bool is_apple_silicon;
    bool is_discrete_gpu;
    bool is_low_power;
    bool is_headless;
    
    // Meta
    char device_name[256];
    uint64_t registry_id;
    
} mtl_device_caps_t;

/* ============================================================================
 * API
 * ============================================================================ */

// Note: Implementation will be internal to the module, exposed via device creation
// or dedicated query functions if needed.
// These declarations are primarily for the structs to be visible.

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_DEVICE_CAPS_H */
