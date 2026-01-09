/*
 * mtl_device_caps.c
 * Metal device capabilities implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_device_caps.h"
#include <string.h>

#ifdef __OBJC__
#import <Metal/Metal.h>

void metal_device_query_capabilities(id<MTLDevice> device, mtl_device_caps_t* out_caps) {
    if (!device || !out_caps) return;
    
    memset(out_caps, 0, sizeof(mtl_device_caps_t));
    
    // Limits
    MTLSize maxThreads = [device maxThreadsPerThreadgroup];
    out_caps->max_threads_per_group[0] = (uint32_t)maxThreads.width;
    out_caps->max_threads_per_group[1] = (uint32_t)maxThreads.height;
    out_caps->max_threads_per_group[2] = (uint32_t)maxThreads.depth;
    
    out_caps->max_buffer_size = (uint32_t)[device maxBufferLength];
    
    // Texture limits (using safe defaults or querying if API available)
    if (@available(macOS 10.11, *)) {
        // Some of these might be fixed per GPU family on older OS, 
        // but new Metal versions expose more properties or we rely on family checks.
        // For simplicity in this C struct, we might set known constants or minimal safe values
        // if specific API queries aren't direct properties on MTLDevice in older versions.
        // However, most of these are standard for Metal.
    }
    
    // Explicit Feature Support
    if (@available(macOS 10.15, *)) {
        out_caps->supports_raytracing = [device supportsRaytracing];
    } else {
        out_caps->supports_raytracing = false;
    }
    
    if (@available(macOS 11.0, *)) {
        out_caps->supports_mesh_shaders = [device supportsFamily:MTLGPUFamilyApple7];
        out_caps->supports_32bit_stencil = [device supports32BitMSAA];
        out_caps->supports_barycentric_coords = [device supportsShaderBarycentricCoordinates];
        // pull-model interpolation / barycentric
        out_caps->supports_shader_barycentric_coords = [device supportsShaderBarycentricCoordinates];
        out_caps->supports_texture_compression_bc = [device supportsBCTextureCompression];
    }
    
    if (@available(macOS 10.13, *)) {
        out_caps->supports_argument_buffers = [device argumentBuffersSupport] == MTLArgumentBuffersTier2;
        out_caps->supports_raster_order_groups = [device areRasterOrderGroupsSupported];
    }
    
    if (@available(macOS 10.14, *)) {
         out_caps->supports_indirect_rendering = true; // Generally true for Metal 2+
    }
    
    // Sparse Textures
    if (@available(macOS 11.0, *)) {
        out_caps->supports_sparse_textures = [device supportsFamily:MTLGPUFamilyApple6] || [device supportsFamily:MTLGPUFamilyMac2];
    }
    
    // Texture Compression
    // ASTC is generally supported on Apple Silicon and newer discrete capabilities
    #if defined(MTL_GPU_FAMILY_APPLE_1)
    if (@available(macOS 11.0, *)) {
        out_caps->supports_texture_compression_astc = [device supportsFamily:MTLGPUFamilyApple1]; 
    }
    #endif
    
    out_caps->supports_texture_compression_pvrtc = false; // Mostly iOS, deprecated on Mac
    
    // Memory
    if (@available(macOS 10.15, *)) {
        out_caps->is_unified_memory = [device hasUnifiedMemory];
    }
    
    // Device Name
    const char* name = [[device name] UTF8String];
    if (name) {
        strncpy(out_caps->device_name, name, sizeof(out_caps->device_name) - 1);
    }
    
    out_caps->registry_id = [device registryID];
    
    // GPU Family Heuristics
    if (@available(macOS 11.0, *)) {
        out_caps->is_apple_silicon = [device supportsFamily:MTLGPUFamilyApple1];
        // If not unified memory, likely discrete
        out_caps->is_discrete_gpu = !out_caps->is_unified_memory; 
    }
    
    out_caps->is_headless = [device isHeadless];
    out_caps->is_low_power = [device isLowPower];
}

#endif
