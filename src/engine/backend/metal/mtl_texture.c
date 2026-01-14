/*
 * mtl_texture.c
 * Metal texture implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#import <Metal/Metal.h>
#include "mtl_texture.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math_all.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

// Forward declaration for device structure
typedef struct metal_device {
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
} metal_device_t;

/* ============================================================================
 * HELPERS
 * ============================================================================ */

static inline MTLPixelFormat convert_format(metal_pixel_format_t fmt) {
    switch (fmt) {
        case METAL_PIXEL_FORMAT_RGBA8_UNORM: return MTLPixelFormatRGBA8Unorm;
        case METAL_PIXEL_FORMAT_RGBA8_SRGB: return MTLPixelFormatRGBA8Unorm_sRGB;
        case METAL_PIXEL_FORMAT_BGRA8_UNORM: return MTLPixelFormatBGRA8Unorm;
        case METAL_PIXEL_FORMAT_BGRA8_SRGB: return MTLPixelFormatBGRA8Unorm_sRGB;
        case METAL_PIXEL_FORMAT_RGBA16_FLOAT: return MTLPixelFormatRGBA16Float;
        case METAL_PIXEL_FORMAT_RGBA32_FLOAT: return MTLPixelFormatRGBA32Float;
        case METAL_PIXEL_FORMAT_R8_UNORM: return MTLPixelFormatR8Unorm;
        case METAL_PIXEL_FORMAT_R16_FLOAT: return MTLPixelFormatR16Float;
        case METAL_PIXEL_FORMAT_R32_FLOAT: return MTLPixelFormatR32Float;
        case METAL_PIXEL_FORMAT_RG16_FLOAT: return MTLPixelFormatRG16Float;
        case METAL_PIXEL_FORMAT_DEPTH32_FLOAT: return MTLPixelFormatDepth32Float;
        case METAL_PIXEL_FORMAT_DEPTH24_STENCIL8:
#if TARGET_OS_OSX
            return MTLPixelFormatDepth24Unorm_Stencil8;
#else
            return MTLPixelFormatDepth32Float_Stencil8;  // iOS doesn't support Depth24
#endif
        // Compressed formats
        case METAL_PIXEL_FORMAT_BC1_RGBA:
            return MTLPixelFormatBC1_RGBA;
        case METAL_PIXEL_FORMAT_BC3_RGBA:
            return MTLPixelFormatBC3_RGBA;
        case METAL_PIXEL_FORMAT_BC5_RG:
            return MTLPixelFormatBC5_RGUnorm;
        case METAL_PIXEL_FORMAT_BC7_RGBA:
            return MTLPixelFormatBC7_RGBAUnorm;
        case METAL_PIXEL_FORMAT_ASTC_4x4_SRGB:
            return MTLPixelFormatASTC_4x4_sRGB;
        case METAL_PIXEL_FORMAT_ASTC_8x8_SRGB:
            return MTLPixelFormatASTC_8x8_sRGB;
        default:
            NSLog(@"Unsupported pixel format: %d, using RGBA8Unorm as fallback", fmt);
            return MTLPixelFormatRGBA8Unorm;
    }
}

static MTLTextureType texture_type_to_mtl(metal_texture_type_t type) {
    switch (type) {
        case METAL_TEXTURE_TYPE_2D:
            return MTLTextureType2D;
        case METAL_TEXTURE_TYPE_3D:
            return MTLTextureType3D;
        case METAL_TEXTURE_TYPE_CUBE:
            return MTLTextureTypeCube;
        case METAL_TEXTURE_TYPE_2D_ARRAY:
            return MTLTextureType2DArray;
        case METAL_TEXTURE_TYPE_CUBE_ARRAY:
            return MTLTextureTypeCubeArray;
        default:
            return MTLTextureType2D;
    }
}

static MTLTextureUsage texture_usage_to_mtl(uint32_t usage) {
    MTLTextureUsage mtl_usage = 0;
    
    if (usage & METAL_TEXTURE_USAGE_SHADER_READ) {
        mtl_usage |= MTLTextureUsageShaderRead;
    }
    if (usage & METAL_TEXTURE_USAGE_SHADER_WRITE) {
        mtl_usage |= MTLTextureUsageShaderWrite;
    }
    if (usage & METAL_TEXTURE_USAGE_RENDER_TARGET) {
        mtl_usage |= MTLTextureUsageRenderTarget;
    }
    if (usage & METAL_TEXTURE_USAGE_PIXEL_FORMAT_VIEW) {
        mtl_usage |= MTLTextureUsagePixelFormatView;
    }
    
    return mtl_usage;
}

static uint32_t calculate_mip_levels(uint32_t width, uint32_t height, uint32_t depth) {
    uint32_t max_dim = width > height ? width : height;
    max_dim = max_dim > depth ? max_dim : depth;
    
    uint32_t mip_levels = 1;
    while (max_dim > 1) {
        max_dim >>= 1;
        mip_levels++;
    }
    
    return mip_levels;
}

/* ============================================================================
 * TEXTURE MANAGEMENT
 * ============================================================================ */

metal_texture_t* metal_texture_create(metal_device_t* device, const metal_texture_desc_t* desc) {
    if (!device || !desc || desc->width == 0 || desc->height == 0) {
        return NULL;
    }

    metal_texture_t* texture = (metal_texture_t*)calloc(1, sizeof(metal_texture_t));
    if (!texture) {
        return NULL;
    }

    // Convert formats
    MTLPixelFormat mtl_format = metal_pixel_format_to_mtl(desc->format);
    MTLTextureType mtl_type = texture_type_to_mtl(desc->type);
    MTLTextureUsage mtl_usage = texture_usage_to_mtl(desc->usage);

    // Calculate mip levels
    uint32_t mip_levels = desc->mip_levels;
    if (mip_levels == 0) {
        mip_levels = calculate_mip_levels(desc->width, desc->height, 
                                         desc->type == METAL_TEXTURE_TYPE_3D ? desc->depth : 1);
    }

    // Create descriptor
    MTLTextureDescriptor* mtl_desc = [[MTLTextureDescriptor alloc] init];
    if (!mtl_desc) {
        NSLog(@"Failed to allocate MTLTextureDescriptor");
        free(texture);
        return NULL;
    }

    mtl_desc.textureType = mtl_type;
    mtl_desc.pixelFormat = mtl_format;
    mtl_desc.width = desc->width;
    mtl_desc.height = desc->height;
    mtl_desc.mipmapLevelCount = mip_levels;
    mtl_desc.usage = mtl_usage;
    mtl_desc.storageMode = MTLStorageModePrivate;  // Default to private for GPU-only access

    // Set depth for 3D textures
    if (desc->type == METAL_TEXTURE_TYPE_3D) {
        mtl_desc.depth = desc->depth;
    } else {
        mtl_desc.depth = 1;
    }

    // Set array length for array textures
    if (desc->type == METAL_TEXTURE_TYPE_2D_ARRAY || desc->type == METAL_TEXTURE_TYPE_CUBE_ARRAY) {
        mtl_desc.arrayLength = desc->array_length;
    } else if (desc->type == METAL_TEXTURE_TYPE_CUBE) {
        mtl_desc.arrayLength = 6;  // Cube maps always have 6 faces
    } else {
        mtl_desc.arrayLength = 1;
    }

    // Create the texture
    texture->texture = [device->device newTextureWithDescriptor:mtl_desc];
    if (!texture->texture) {
        free(texture);
        return NULL;
    }

    // Store metadata
    texture->format = desc->format;
    texture->type = desc->type;
    texture->width = desc->width;
    texture->height = desc->height;
    texture->depth = desc->depth;
    texture->mip_levels = mip_levels;
    texture->array_length = mtl_desc.arrayLength;
    texture->usage = desc->usage;

    return texture;
}

void metal_texture_destroy(metal_texture_t* texture) {
    if (!texture) {
        return;
    }

    if (texture->texture) {
        texture->texture = nil;  // ARC will handle release
    }

    free(texture);
}

void metal_texture_upload(metal_texture_t* texture, const void* data, size_t data_size,
                         const metal_texture_region_t* region) {
    if (!texture || !data || data_size == 0) {
        return;
    }

    MTLRegion mtl_region;
    uint32_t mip_level = 0;
    uint32_t array_slice = 0;

    if (region) {
        mtl_region = MTLRegionMake3D(region->x, region->y, region->z,
                                     region->width, region->height, region->depth);
        mip_level = region->mip_level;
        array_slice = region->array_slice;
    } else {
        // Upload to entire texture at mip 0
        mtl_region = MTLRegionMake3D(0, 0, 0, texture->width, texture->height, 
                                     texture->type == METAL_TEXTURE_TYPE_3D ? texture->depth : 1);
    }

    // Calculate bytes per row
    size_t bytes_per_pixel = metal_pixel_format_bytes_per_pixel(texture->format);
    size_t bytes_per_row = mtl_region.size.width * bytes_per_pixel;
    size_t bytes_per_image = bytes_per_row * mtl_region.size.height;

    // Upload data
    [texture->texture replaceRegion:mtl_region
                        mipmapLevel:mip_level
                              slice:array_slice
                          withBytes:data
                        bytesPerRow:bytes_per_row
                      bytesPerImage:bytes_per_image];
}

metal_texture_t* metal_texture_create_view(metal_texture_t* source, metal_pixel_format_t format,
                                          uint32_t base_mip, uint32_t mip_count) {
    if (!source) {
        return NULL;
    }

    metal_texture_t* view = (metal_texture_t*)calloc(1, sizeof(metal_texture_t));
    if (!view) {
        return NULL;
    }

    MTLPixelFormat mtl_format = metal_pixel_format_to_mtl(format);
    
    // Create texture view
    view->texture = [source->texture newTextureViewWithPixelFormat:mtl_format
                                                       textureType:source->texture.textureType
                                                            levels:NSMakeRange(base_mip, mip_count)
                                                            slices:NSMakeRange(0, source->array_length)];

    if (!view->texture) {
        free(view);
        return NULL;
    }

    // Copy metadata
    view->format = format;
    view->type = source->type;
    view->width = source->width >> base_mip;
    view->height = source->height >> base_mip;
    view->depth = source->depth;
    view->mip_levels = mip_count;
    view->array_length = source->array_length;
    view->usage = source->usage;

    return view;
}

void metal_texture_generate_mipmaps(metal_texture_t* texture, void* command_buffer) {
    if (!texture || !command_buffer) {
        return;
    }

    id<MTLCommandBuffer> mtl_cmd_buffer = (__bridge id<MTLCommandBuffer>)command_buffer;
    
    // Create blit command encoder
    id<MTLBlitCommandEncoder> blit_encoder = [mtl_cmd_buffer blitCommandEncoder];
    
    // Generate mipmaps
    [blit_encoder generateMipmapsForTexture:texture->texture];
    
    // End encoding
    [blit_encoder endEncoding];
}

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

size_t metal_pixel_format_bytes_per_pixel(metal_pixel_format_t format) {
    switch (format) {
        case METAL_PIXEL_FORMAT_RGBA8_UNORM:
        case METAL_PIXEL_FORMAT_RGBA8_SRGB:
        case METAL_PIXEL_FORMAT_BGRA8_UNORM:
        case METAL_PIXEL_FORMAT_BGRA8_SRGB:
        case METAL_PIXEL_FORMAT_DEPTH32_FLOAT:
        case METAL_PIXEL_FORMAT_R32_FLOAT:
            return 4;
            
        case METAL_PIXEL_FORMAT_RGBA16_FLOAT:
            return 8;
            
        case METAL_PIXEL_FORMAT_RGBA32_FLOAT:
            return 16;
            
        case METAL_PIXEL_FORMAT_R8_UNORM:
            return 1;
            
        case METAL_PIXEL_FORMAT_R16_FLOAT:
            return 2;

        case METAL_PIXEL_FORMAT_RG16_FLOAT:
            return 4;
            
        case METAL_PIXEL_FORMAT_DEPTH24_STENCIL8:
            return 4;
            
        // Compressed formats - return 0 (need special handling)
        case METAL_PIXEL_FORMAT_BC1_RGBA:
        case METAL_PIXEL_FORMAT_BC3_RGBA:
        case METAL_PIXEL_FORMAT_BC5_RG:
        case METAL_PIXEL_FORMAT_BC7_RGBA:
        case METAL_PIXEL_FORMAT_ASTC_4x4_SRGB:
        case METAL_PIXEL_FORMAT_ASTC_8x8_SRGB:
            return 0;  // Compressed formats need block-based calculation
            
        default:
            return 4;
    }
}

bool metal_pixel_format_is_compressed(metal_pixel_format_t format) {
    switch (format) {
        case METAL_PIXEL_FORMAT_BC1_RGBA:
        case METAL_PIXEL_FORMAT_BC3_RGBA:
        case METAL_PIXEL_FORMAT_BC5_RG:
        case METAL_PIXEL_FORMAT_BC7_RGBA:
        case METAL_PIXEL_FORMAT_ASTC_4x4_SRGB:
        case METAL_PIXEL_FORMAT_ASTC_8x8_SRGB:
            return true;
        default:
            return false;
    }
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATIONS
 * ============================================================================ */

MTLPixelFormat metal_pixel_format_to_mtl(metal_pixel_format_t format) {
    return convert_format(format);
}

/* End of mtl_texture.c */
