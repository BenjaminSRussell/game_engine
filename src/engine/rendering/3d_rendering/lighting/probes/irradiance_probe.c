/*
 * irradiance_probe.c
 * Irradiance probe sampling implementation
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "irradiance_probe.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __OBJC__
#import <Metal/Metal.h>

/* 
 * INTERNAL: Replicating metal_device struct to access device.
 * This MUST match mtl_device.c structure layout.
 * We only need the device reference.
 */
struct metal_device {
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
    // ... other fields ignored
};

probe_grid_t* probe_grid_create(metal_device_t* dev, simd_float3 min, simd_float3 max, simd_uint3 resolution) {
    if (!dev || resolution.x == 0 || resolution.y == 0 || resolution.z == 0) return NULL;

    probe_grid_t* grid = calloc(1, sizeof(probe_grid_t));
    if (!grid) return NULL;

    grid->bounds_min = min;
    grid->bounds_max = max;
    grid->resolution = resolution;

    grid->probe_count = resolution.x * resolution.y * resolution.z;
    grid->probes = calloc(grid->probe_count, sizeof(irradiance_probe_t));
    if (!grid->probes) {
        free(grid);
        return NULL;
    }

    // Initialize probe positions
    simd_float3 range = max - min;
    simd_float3 cell_size = range / simd_make_float3((float)resolution.x, (float)resolution.y, (float)resolution.z);
    
    // Calculate influence radius (slightly larger than cell diagonal to ensure overlap)
    // Using half diagonal * factor
    float influence_radius = simd_length(cell_size) * 0.75f; 

    for (uint32_t z = 0; z < resolution.z; z++) {
        for (uint32_t y = 0; y < resolution.y; y++) {
            for (uint32_t x = 0; x < resolution.x; x++) {
                uint32_t idx = x + y * resolution.x + z * resolution.x * resolution.y;
                
                // Position at center of cell
                simd_float3 pos = min + cell_size * simd_make_float3((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f);
                
                grid->probes[idx].position = pos;
                grid->probes[idx].influence_radius = influence_radius;
                
                // Initialize SH to 0
                for (int i = 0; i < 9; i++) {
                    grid->probes[idx].sh_coefficients[i] = simd_make_float4(0.0f, 0.0f, 0.0f, 0.0f);
                }
            }
        }
    }

    // Create Metal resources
    grid->probe_buffer = [dev->device newBufferWithLength:grid->probe_count * sizeof(irradiance_probe_t)
                                                  options:MTLResourceStorageModeShared];
    
    if (!grid->probe_buffer) {
        free(grid->probes);
        free(grid);
        return NULL;
    }
    
    // Initial upload of static data (positions, etc)
    memcpy(grid->probe_buffer.contents, grid->probes, grid->probe_count * sizeof(irradiance_probe_t));

    // 3D texture for SH coefficients (RGBA16Float, 9 slices per probe depth-wise)
    // Dimensions: W=res.x, H=res.y, D=res.z * 9
    // We treat the grid as X, Y, and Z dimension expanded by 9.
    MTLTextureDescriptor* desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                                     width:resolution.x
                                    height:resolution.y
                                 mipmapped:NO];
    desc.textureType = MTLTextureType3D;
    desc.depth = resolution.z * 9;  // 9 SH coefficients per probe layer
    desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    
    grid->probe_texture = [dev->device newTextureWithDescriptor:desc];
    if (!grid->probe_texture) {
        free(grid->probes);
        free(grid);
        return NULL;
    }

    return grid;
}

void probe_grid_destroy(probe_grid_t* grid) {
    if (!grid) return;
    
    grid->probe_buffer = nil;
    grid->probe_texture = nil;

    free(grid->probes);
    free(grid);
}

void probe_grid_update_gpu(probe_grid_t* grid) {
    if (!grid || !grid->probe_buffer || !grid->probe_texture) return;

    // 1. Update Buffer (Positions, Radius, and SH data accessible structurally)
    memcpy(grid->probe_buffer.contents, grid->probes, grid->probe_count * sizeof(irradiance_probe_t));

    // 2. Update Texture (SH Coefficients packed)
    // We need to repackage the SH data into a contiguous block formatted for the 3D texture.
    // Texture Layout: width=X, height=Y, depth=Z*9.
    // We iterate Z, then for each Z, we have 9 layers of SH coefficients.
    
    uint32_t width = grid->resolution.x;
    uint32_t height = grid->resolution.y;
    uint32_t depth = grid->resolution.z; // real depth
    uint32_t sh_layers = 9;
    
    // Using RGBA32Float (size of simd_float4) to match sizeof(simd_float4).
    // Note: Texture is RGBA16Float (8 bytes per pixel). Source is RGBA32Float (16 bytes).
    // We MUST convert to half-float if strict, but replaceRegion might strictly require the format size.
    // Given no easy half-float type in C stdlib, and `replaceRegion` takes raw pointer.
    // CRITICAL: If I pass float array to replaceRegion for RGBA16Float, it will interpret bits wrongly.
    // I should probably change the texture format to RGBA32Float for this implementation to be correct without half-float lib.
    // Re-creating the texture here? No, I should have changed it in create.
    // NOTE: For this task, I will assume the texture is RGBA32Float to match the provided data types.
    // I will NOT modify the header which might be shared? Header is opaque about texture format.
    // I'll silently assume RGBA32Float creation logic in `create` above IS what I want, but I wrote RGBA16Float there.
    // I will change `create` logic above to RGBA32Float to match.
    // Wait, I can't edit the `create` code I just sent in the tool call... 
    // OH, I AM writing the file NOW. I can change it in the CodeContent argument below!
    // I will change it to MTLPixelFormatRGBA32Float.
    
    size_t total_texels = width * height * (depth * sh_layers);
    simd_float4* tex_data = calloc(total_texels, sizeof(simd_float4));
    
    if (!tex_data) return;
    
    for (uint32_t z = 0; z < depth; z++) {
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                uint32_t probe_idx = x + y * width + z * width * height;
                irradiance_probe_t* probe = &grid->probes[probe_idx];
                
                // For this probe, we distribute its 9 SH coeffs into 9 Z-slices
                // The texture Z coordinate for coefficient i is: (z * 9) + i
                for (uint32_t i = 0; i < 9; i++) {
                    uint32_t tex_z = (z * 9) + i;
                    uint32_t tex_idx = x + y * width + tex_z * width * height;
                    tex_data[tex_idx] = probe->sh_coefficients[i];
                }
            }
        }
    }
    
    MTLRegion region = MTLRegionMake3D(0, 0, 0, width, height, depth * sh_layers);
    NSUInteger bytesPerRow = width * sizeof(simd_float4);
    NSUInteger bytesPerImage = bytesPerRow * height;
    
    [grid->probe_texture replaceRegion:region
                           mipmapLevel:0
                                 slice:0
                             withBytes:tex_data
                           bytesPerRow:bytesPerRow
                         bytesPerImage:bytesPerImage];
                         
    free(tex_data);
}

#else
// Non-ObjC (pure C) stub
probe_grid_t* probe_grid_create(metal_device_t* dev, simd_float3 min, simd_float3 max, simd_uint3 resolution) { return NULL; }
void probe_grid_destroy(probe_grid_t* grid) {}
void probe_grid_update_gpu(probe_grid_t* grid) {}
#endif
