/*
 * mtl_texture.h
 * Metal texture management
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_TEXTURE_H
#define PLATFORM_MTL_TEXTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct metal_device metal_device_t;

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum metal_texture_type {
    METAL_TEXTURE_TYPE_2D = 0,
    METAL_TEXTURE_TYPE_3D = 1,
    METAL_TEXTURE_TYPE_CUBE = 2,
    METAL_TEXTURE_TYPE_2D_ARRAY = 3,
    METAL_TEXTURE_TYPE_CUBE_ARRAY = 4
} metal_texture_type_t;

typedef enum metal_pixel_format {
    METAL_PIXEL_FORMAT_RGBA8_UNORM = 0,
    METAL_PIXEL_FORMAT_RGBA8_SRGB = 1,
    METAL_PIXEL_FORMAT_BGRA8_UNORM = 2,
    METAL_PIXEL_FORMAT_BGRA8_SRGB = 3,
    METAL_PIXEL_FORMAT_RGBA16_FLOAT = 4,
    METAL_PIXEL_FORMAT_RGBA32_FLOAT = 5,
    METAL_PIXEL_FORMAT_R8_UNORM = 6,
    METAL_PIXEL_FORMAT_R16_FLOAT = 7,
    METAL_PIXEL_FORMAT_R32_FLOAT = 8,
    METAL_PIXEL_FORMAT_RG16_FLOAT = 9,
    METAL_PIXEL_FORMAT_RG32_FLOAT = 10,
    METAL_PIXEL_FORMAT_DEPTH32_FLOAT = 11,
    METAL_PIXEL_FORMAT_DEPTH24_STENCIL8 = 12,
    // Compressed formats (BC - Desktop)
    METAL_PIXEL_FORMAT_BC1_RGBA = 13,
    METAL_PIXEL_FORMAT_BC3_RGBA = 14,
    METAL_PIXEL_FORMAT_BC5_RG = 15,
    METAL_PIXEL_FORMAT_BC7_RGBA = 16,
    // Compressed formats (ASTC - Mobile/Desktop)
    METAL_PIXEL_FORMAT_ASTC_4x4_SRGB = 17,
    METAL_PIXEL_FORMAT_ASTC_8x8_SRGB = 18,
    // Additional depth formats
    METAL_PIXEL_FORMAT_DEPTH16_UNORM = 19,
    METAL_PIXEL_FORMAT_DEPTH32_FLOAT_STENCIL8 = 20,
    // Additional compressed formats (ETC2 - Mobile)
    METAL_PIXEL_FORMAT_ETC2_RGB8 = 21,
    METAL_PIXEL_FORMAT_ETC2_RGB8_SRGB = 22,
    // Additional compressed formats (PVRTC - Legacy iOS)
    METAL_PIXEL_FORMAT_PVRTC_RGB_4BPP = 23,
    METAL_PIXEL_FORMAT_PVRTC_RGBA_4BPP = 24
} metal_pixel_format_t;

typedef enum metal_texture_usage {
    METAL_TEXTURE_USAGE_SHADER_READ = 1 << 0,
    METAL_TEXTURE_USAGE_SHADER_WRITE = 1 << 1,
    METAL_TEXTURE_USAGE_RENDER_TARGET = 1 << 2,
    METAL_TEXTURE_USAGE_PIXEL_FORMAT_VIEW = 1 << 3
} metal_texture_usage_t;

typedef enum metal_storage_mode_texture {
    METAL_TEXTURE_STORAGE_SHARED = 0,
    METAL_TEXTURE_STORAGE_PRIVATE = 1,
    METAL_TEXTURE_STORAGE_MEMORYLESS = 2
} metal_storage_mode_texture_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct metal_texture {
#ifdef __OBJC__
    id<MTLTexture> texture;
#else
    void* texture;
#endif
    metal_pixel_format_t format;
    metal_texture_type_t type;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mip_levels;
    uint32_t array_length;
    uint32_t usage;
    char label[64];
} metal_texture_t;

typedef struct metal_texture_desc {
    metal_texture_type_t type;
    metal_pixel_format_t format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;           // For 3D textures
    uint32_t array_length;    // For array textures
    uint32_t mip_levels;      // 0 = auto-generate full mip chain
    uint32_t usage;
    metal_storage_mode_texture_t storage_mode;
    bool generate_mipmaps;
    const char* label;
    
    // MSAA support
    uint32_t sample_count;    // 1 (default), 2, 4, or 8
} metal_texture_desc_t;

typedef struct metal_texture_view_desc {
    metal_pixel_format_t format;
    metal_texture_type_t type;
    uint32_t base_mip_level;
    uint32_t mip_level_count;
    uint32_t base_array_slice;
    uint32_t array_slice_count;
} metal_texture_view_desc_t;

typedef struct metal_texture_region {
    uint32_t x, y, z;
    uint32_t width, height, depth;
    uint32_t mip_level;
    uint32_t array_slice;
} metal_texture_region_t;

/* ============================================================================
 * API - Texture Management
 * ============================================================================ */

/* Create a Metal texture */
metal_texture_t* metal_texture_create(metal_device_t* device, const metal_texture_desc_t* desc);

/* Destroy a Metal texture */
void metal_texture_destroy(metal_texture_t* texture);

/* Upload data to texture */
void metal_texture_upload(metal_texture_t* texture, const void* data, size_t data_size, 
                         const metal_texture_region_t* region);

/* Upload data to specific mip level */
void metal_texture_upload_mip(metal_texture_t* texture, const void* data, size_t data_size,
                               uint32_t mip_level, uint32_t array_slice);

/* Upload data to specific array slice */
void metal_texture_upload_slice(metal_texture_t* texture, const void* data, size_t data_size,
                                uint32_t array_slice, uint32_t mip_level);

/* Create a texture view (different pixel format or mip range) */
metal_texture_t* metal_texture_create_view(metal_texture_t* source, metal_pixel_format_t format,
                                          uint32_t base_mip, uint32_t mip_count);

/* Generate mipmaps for texture (requires command buffer) */
void metal_texture_generate_mipmaps(metal_texture_t* texture, void* command_buffer);

/* Check if device supports sparse textures */
bool metal_texture_is_sparse_supported(metal_device_t* device);

/* Create sparse texture (iOS 13+, macOS 11+) */
metal_texture_t* metal_texture_create_sparse(metal_device_t* device, const metal_texture_desc_t* desc);

/* Set LOD clamp range */
void metal_texture_set_lod_clamp(metal_texture_t* texture, float min_lod, float max_lod);

/* ============================================================================
 * HELPERS
 * ============================================================================ */

/* Convert pixel format enum to Metal pixel format */
#ifdef __OBJC__
MTLPixelFormat metal_pixel_format_to_mtl(metal_pixel_format_t format);
#endif

/* Get bytes per pixel for a format */
size_t metal_pixel_format_bytes_per_pixel(metal_pixel_format_t format);

/* Check if format is compressed */
bool metal_pixel_format_is_compressed(metal_pixel_format_t format);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_TEXTURE_H */
