/*
 * mtl_device_caps.c
 * Metal device capabilities implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_device_caps.h"
#include <string.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

/* ============================================================================
 * MACROS helpers
 * ============================================================================ */

static void query_os_version(mtl_os_version_t* out_version) {
    if (@available(macOS 10.10, *)) {
        NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
        out_version->major = (uint32_t)version.majorVersion;
        out_version->minor = (uint32_t)version.minorVersion;
        out_version->patch = (uint32_t)version.patchVersion;
    } else {
        // Fallback for very old systems
        out_version->major = 10;
        out_version->minor = 0;
        out_version->patch = 0;
    }
}

static mtl_feature_level_t determine_feature_level(const mtl_os_version_t* os_ver, id<MTLDevice> device) {
    // Metal 3.0 requires macOS 13.0+
    if (os_ver->major >= 13) {
        return MTL_FEATURE_LEVEL_3_0;
    }
    // Metal 2.3 requires macOS 10.15+
    if (os_ver->major >= 11 || (os_ver->major == 10 && os_ver->minor >= 15)) {
        return MTL_FEATURE_LEVEL_2_3;
    }
    // Metal 2.0 requires macOS 10.13+
    if (os_ver->major >= 11 || (os_ver->major == 10 && os_ver->minor >= 13)) {
        return MTL_FEATURE_LEVEL_2_0;
    }
    // Metal 1.0 is 10.11+
    return MTL_FEATURE_LEVEL_1_0;
}

static uint32_t query_gpu_families(id<MTLDevice> device) {
    uint32_t families = 0;
    
    if (@available(macOS 11.0, *)) {
        // Apple families
        if ([device supportsFamily:MTLGPUFamilyApple1]) families |= MTL_GPU_FAMILY_APPLE_1;
        if ([device supportsFamily:MTLGPUFamilyApple2]) families |= MTL_GPU_FAMILY_APPLE_2;
        if ([device supportsFamily:MTLGPUFamilyApple3]) families |= MTL_GPU_FAMILY_APPLE_3;
        if ([device supportsFamily:MTLGPUFamilyApple4]) families |= MTL_GPU_FAMILY_APPLE_4;
        if ([device supportsFamily:MTLGPUFamilyApple5]) families |= MTL_GPU_FAMILY_APPLE_5;
        if ([device supportsFamily:MTLGPUFamilyApple6]) families |= MTL_GPU_FAMILY_APPLE_6;
        if ([device supportsFamily:MTLGPUFamilyApple7]) families |= MTL_GPU_FAMILY_APPLE_7;
        
        if (@available(macOS 13.0, *)) {
            if ([device supportsFamily:MTLGPUFamilyApple8]) families |= MTL_GPU_FAMILY_APPLE_8;
        }
        
        // Mac families
        if ([device supportsFamily:MTLGPUFamilyMac2]) families |= MTL_GPU_FAMILY_MAC_2;
        else if ([device supportsFamily:MTLGPUFamilyMac1]) families |= MTL_GPU_FAMILY_MAC_1;
    }
    
    return families;
}

static void query_texture_limits(id<MTLDevice> device, uint32_t gpu_families, mtl_device_caps_t* out_caps) {
    // These are fairly standard across Metal implementations
    // Apple Silicon has higher limits than Intel/AMD discrete GPUs in some cases
    
    bool is_apple_silicon = (gpu_families & (MTL_GPU_FAMILY_APPLE_1 | MTL_GPU_FAMILY_APPLE_2 | 
                                              MTL_GPU_FAMILY_APPLE_3 | MTL_GPU_FAMILY_APPLE_4 |
                                              MTL_GPU_FAMILY_APPLE_5 | MTL_GPU_FAMILY_APPLE_6 |
                                              MTL_GPU_FAMILY_APPLE_7 | MTL_GPU_FAMILY_APPLE_8)) != 0;
    
    if (is_apple_silicon) {
        // Apple Silicon limits (generally higher)
        out_caps->max_texture_width_2d = 16384;
        out_caps->max_texture_height_2d = 16384;
        out_caps->max_texture_width_3d = 2048;
        out_caps->max_texture_height_3d = 2048;
        out_caps->max_texture_depth_3d = 2048;
        out_caps->max_texture_dimension_cube = 16384;
        out_caps->max_texture_layers = 2048;
    } else {
        // Mac (Intel/AMD) limits
        out_caps->max_texture_width_2d = 16384;
        out_caps->max_texture_height_2d = 16384;
        out_caps->max_texture_width_3d = 2048;
        out_caps->max_texture_height_3d = 2048;
        out_caps->max_texture_depth_3d = 2048;
        out_caps->max_texture_dimension_cube = 16384;
        out_caps->max_texture_layers = 2048;
    }
}

void metal_device_query_capabilities(id<MTLDevice> device, mtl_device_caps_t* out_caps) {
    if (!device || !out_caps) return;
    
    memset(out_caps, 0, sizeof(mtl_device_caps_t));
    
    // OS Version
    query_os_version(&out_caps->os_version);
    
    // GPU Families
    out_caps->gpu_family_flags = query_gpu_families(device);
    
    // Feature Level
    out_caps->feature_level = determine_feature_level(&out_caps->os_version, device);
    
    // ===== Limits - Threads =====
    MTLSize maxThreads = [device maxThreadsPerThreadgroup];
    out_caps->max_threads_per_group[0] = (uint32_t)maxThreads.width;
    out_caps->max_threads_per_group[1] = (uint32_t)maxThreads.height;
    out_caps->max_threads_per_group[2] = (uint32_t)maxThreads.depth;
    
    if (@available(macOS 10.13, *)) {
        out_caps->max_total_threadgroup_memory = (uint32_t)[device maxThreadgroupMemoryLength];
    }
    
    // ===== Limits - Buffers =====
    out_caps->max_buffer_size = [device maxBufferLength];
    
    if (@available(macOS 10.13, *)) {
        NSUInteger argBufferTier = [device argumentBuffersSupport];
        out_caps->max_argument_buffer_entries = (argBufferTier == MTLArgumentBuffersTier2) ? 500000 : 31;
    }
    
    // ===== Texture Limits =====
    query_texture_limits(device, out_caps->gpu_family_flags, out_caps);
    
    // ===== Feature Support =====
    
    // Ray Tracing
    if (@available(macOS 10.15, *)) {
        out_caps->supports_raytracing = [device supportsRaytracing];
    }
    
    // Mesh Shaders (Apple7+)
    if (@available(macOS 12.0, *)) {
        out_caps->supports_mesh_shaders = (out_caps->gpu_family_flags & MTL_GPU_FAMILY_APPLE_7) != 0;
    }
    
    // Indirect Rendering (Metal 2.0+)
    if (@available(macOS 10.13, *)) {
        out_caps->supports_indirect_rendering = true;
    }
    
    // Variable Rate Shading (not directly exposed in Metal, but use as proxy for modern features)
    out_caps->supports_variable_rate_shading = false; // Metal doesn't have traditional VRS
    
    // Sparse Textures
    if (@available(macOS 11.0, *)) {
        out_caps->supports_sparse_textures = [device supportsFamily:MTLGPUFamilyApple6] || 
                                             [device supportsFamily:MTLGPUFamilyMac2];
    }
    
    // Argument Buffers
    if (@available(macOS 10.13, *)) {
        out_caps->supports_argument_buffers = [device argumentBuffersSupport] != MTLArgumentBuffersTier1;
    }
    
    // Raster Order Groups
    if (@available(macOS 10.13, *)) {
        out_caps->supports_raster_order_groups = [device areRasterOrderGroupsSupported];
    }
    
    // Barycentric Coordinates
    if (@available(macOS 10.15, *)) {
        out_caps->supports_barycentric_coords = [device supportsShaderBarycentricCoordinates];
        out_caps->supports_shader_barycentric_coords = [device supportsShaderBarycentricCoordinates];
    }
    
    // Conservative Rasterization (not directly exposed, assume false)
    out_caps->supports_conservative_rasterization = false;
    
    // Tessellation (Metal uses compute-based approach)
    out_caps->supports_tessellation = true; // Can be done via compute shaders
    
    // Primitive Shaders / Vertex Amplification
    if (@available(macOS 12.0, *)) {
        out_caps->supports_primitive_shaders = (out_caps->gpu_family_flags & MTL_GPU_FAMILY_APPLE_7) != 0;
    }
    
    // Sample Rate Shading
    if (@available(macOS 10.14, *)) {
        out_caps->supports_sample_rate_shading = true;
    }
    
    // Fetch Textures (pull-model sampling)
    if (@available(macOS 10.14, *)) {
        out_caps->supports_fetch_textures = true;
    }
    
    // Read-Write Textures
    if (@available(macOS 10.13, *)) {
        out_caps->supports_read_write_textures = true;
    }
    
    // Float16 Filtering
    if (@available(macOS 11.0, *)) {
        out_caps->supports_float16_filtering = [device supportsFamily:MTLGPUFamilyApple3] ||
                                               [device supportsFamily:MTLGPUFamilyMac2];
    }
    
    // ===== Texture Compression =====
    
    // ASTC (Apple Silicon)
    if (@available(macOS 11.0, *)) {
        out_caps->supports_texture_compression_astc = (out_caps->gpu_family_flags & MTL_GPU_FAMILY_APPLE_1) != 0;
    }
    
    // BC (Intel/AMD)
    if (@available(macOS 11.0, *)) {
        out_caps->supports_texture_compression_bc = [device supportsBCTextureCompression];
    }
    
    // PVRTC (mostly iOS, deprecated)
    out_caps->supports_texture_compression_pvrtc = false;
    
    // 32-bit MSAA
    if (@available(macOS 11.0, *)) {
        out_caps->supports_32bit_msaa = [device supports32BitMSAA];
    }
    
    // Depth Clip Mode
    if (@available(macOS 10.14, *)) {
        out_caps->supports_depth_clip_mode = true;
    }
    
    // ===== Memory =====
    
    if (@available(macOS 10.15, *)) {
        out_caps->is_unified_memory = [device hasUnifiedMemory];
    }
    
    if (@available(macOS 11.0, *)) {
        out_caps->supports_memoryless_render_targets = out_caps->is_unified_memory &&
                                                        (out_caps->gpu_family_flags & MTL_GPU_FAMILY_APPLE_1) != 0;
    }
    
    // ===== GPU Properties =====
    
    out_caps->is_apple_silicon = (out_caps->gpu_family_flags & (MTL_GPU_FAMILY_APPLE_1 | MTL_GPU_FAMILY_APPLE_2 | 
                                                                 MTL_GPU_FAMILY_APPLE_3 | MTL_GPU_FAMILY_APPLE_4 |
                                                                 MTL_GPU_FAMILY_APPLE_5 | MTL_GPU_FAMILY_APPLE_6 |
                                                                 MTL_GPU_FAMILY_APPLE_7 | MTL_GPU_FAMILY_APPLE_8)) != 0;
    
    out_caps->is_discrete_gpu = !out_caps->is_unified_memory;
    out_caps->is_low_power = [device isLowPower];
    out_caps->is_headless = [device isHeadless];
    out_caps->is_removable = [device isRemovable];
    
    // ===== Display Capabilities =====
    
    // HDR/EDR support (requires additional display queries, set conservatively)
    if (@available(macOS 10.15, *)) {
        out_caps->supports_hdr = true;  // Device capability, actual display support needs CAMetalLayer query
        out_caps->supports_edr = true;
        out_caps->supports_wide_color = true;
    }
    
    // ProMotion (120Hz+) - requires display query
    out_caps->supports_promotion = false;  // Display-specific, needs NSScreen query
    out_caps->max_refresh_rate_hz = 60;    // Default assumption
    
    // Try to detect ProMotion if available
    if (@available(macOS 12.0, *)) {
        NSArray<NSScreen *>* screens = [NSScreen screens];
        if ([screens count] > 0) {
            NSScreen* mainScreen = screens[0];
            if ([mainScreen respondsToSelector:@selector(maximumFramesPerSecond)]) {
                out_caps->max_refresh_rate_hz = (uint32_t)[mainScreen maximumFramesPerSecond];
                out_caps->supports_promotion = (out_caps->max_refresh_rate_hz > 60);
            }
        }
    }
    
    // ===== Meta =====
    
    const char* name = [[device name] UTF8String];
    if (name) {
        strncpy(out_caps->device_name, name, sizeof(out_caps->device_name) - 1);
        out_caps->device_name[sizeof(out_caps->device_name) - 1] = '\0';
    }
    
    out_caps->registry_id = [device registryID];
    
    if (@available(macOS 10.13, *)) {
        out_caps->location_number = (uint32_t)[device locationNumber];
    }
}

#endif
