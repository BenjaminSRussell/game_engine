/*
 * mtl_resources.h
 * Metal resource definitions (buffers, textures)
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MTL_RESOURCES_H
#define MTL_RESOURCES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Enums & Constants
// ============================================================================

typedef enum metal_storage_mode {
    METAL_STORAGE_SHARED,
    METAL_STORAGE_PRIVATE,
    METAL_STORAGE_MANAGED
} metal_storage_mode_t;

typedef enum metal_texture_type {
    METAL_TEXTURE_TYPE_2D,
    METAL_TEXTURE_TYPE_3D,
    METAL_TEXTURE_TYPE_CUBE,
    METAL_TEXTURE_TYPE_2D_ARRAY
} metal_texture_type_t;

// ============================================================================
// Structures
// ============================================================================

/**
 * Metal buffer wrapper.
 */
typedef struct metal_buffer {
#ifdef __OBJC__
    id<MTLBuffer> buffer;
#else
    id buffer;
#endif
    void* mapped_ptr;
    size_t size;
    metal_storage_mode_t storage_mode;
    char* label;
} metal_buffer_t;

/**
 * Metal texture wrapper.
 */
typedef struct metal_texture {
#ifdef __OBJC__
    id<MTLTexture> texture;
    MTLPixelFormat format;
#else
    id texture;
    uint64_t format;
#endif
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mip_levels;
    uint32_t array_layers;
    metal_texture_type_t type;
    char* label;
} metal_texture_t;

/**
 * Texture region for copy operations.
 */
typedef struct metal_texture_region {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mip_level;
    uint32_t array_slice;
} metal_texture_region_t;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Get bytes per pixel for a given pixel format.
 * 
 * @param format Metal pixel format
 * @return Bytes per pixel
 */
#ifdef __OBJC__
size_t metal_pixel_format_bytes_per_pixel(MTLPixelFormat format);
#else
size_t metal_pixel_format_bytes_per_pixel(uint64_t format);
#endif

#ifdef __cplusplus
}
#endif

#endif // MTL_RESOURCES_H
