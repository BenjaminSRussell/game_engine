/**
 * @file render_resource.h
 * @brief GPU resource management (buffers, textures, etc.)
 */

#ifndef RENDER_RESOURCE_H
#define RENDER_RESOURCE_H

#include <core/resource/resource_handle.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RESOURCE_TYPE_BUFFER = 0,
    RESOURCE_TYPE_IMAGE = 1,
    RESOURCE_TYPE_SAMPLER = 2,
    RESOURCE_TYPE_ACCELERATION_STRUCTURE = 3,
} ResourceType;

typedef enum {
    BUFFER_USAGE_TRANSFER_SRC = 0x01,
    BUFFER_USAGE_TRANSFER_DST = 0x02,
    BUFFER_USAGE_UNIFORM      = 0x04,
    BUFFER_USAGE_STORAGE      = 0x08,
    BUFFER_USAGE_INDEX        = 0x10,
    BUFFER_USAGE_VERTEX       = 0x20,
    BUFFER_USAGE_INDIRECT     = 0x40,
} BufferUsageFlags;

typedef enum {
    FORMAT_UNDEFINED = 0,
    FORMAT_R8_UNORM,
    FORMAT_R8G8B8A8_UNORM,
    FORMAT_R8G8B8A8_SRGB,
    FORMAT_R16_SFLOAT,
    FORMAT_R16G16B16A16_SFLOAT,
    FORMAT_R32_SFLOAT,
    FORMAT_R32G32B32A32_SFLOAT,
    FORMAT_D32_SFLOAT,
    FORMAT_D24_UNORM_S8_UINT,
} ImageFormat;

typedef enum {
    IMAGE_USAGE_TRANSFER_SRC = 0x01,
    IMAGE_USAGE_TRANSFER_DST = 0x02,
    IMAGE_USAGE_SAMPLED      = 0x04,
    IMAGE_USAGE_STORAGE      = 0x08,
    IMAGE_USAGE_COLOR_ATTACHMENT = 0x10,
    IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 0x20,
} ImageUsageFlags;

typedef enum {
    MEMORY_PROPERTY_DEVICE_LOCAL = 0x01,
    MEMORY_PROPERTY_HOST_VISIBLE = 0x02,
    MEMORY_PROPERTY_HOST_COHERENT = 0x04,
    MEMORY_PROPERTY_HOST_CACHED   = 0x08,
} MemoryPropertyFlags;

/* 
 * Opaque Manager 
 */
typedef struct RenderResourceManager RenderResourceManager;

/* 
 * Lifecycle 
 */
RenderResourceManager* render_resource_manager_create(uint32_t max_resources_per_type);
void render_resource_manager_destroy(RenderResourceManager* manager);

/* 
 * Buffers 
 */
buffer_handle_t render_buffer_create(RenderResourceManager* manager,
                                     size_t size,
                                     uint32_t usage,
                                     uint32_t memory_flags);

void render_buffer_destroy(RenderResourceManager* manager, buffer_handle_t buffer);

// Note: Mapping might return a pointer, but the handle is still used to identify the resource
void* render_buffer_map(RenderResourceManager* manager, buffer_handle_t buffer);
void render_buffer_unmap(RenderResourceManager* manager, buffer_handle_t buffer);

/* 
 * Images 
 */
texture_handle_t render_image_create(RenderResourceManager* manager,
                                     uint32_t width,
                                     uint32_t height,
                                     ImageFormat format,
                                     uint32_t usage);

void render_image_destroy(RenderResourceManager* manager, texture_handle_t image);

/* 
 * Statistics 
 */
void render_resource_manager_get_stats(RenderResourceManager* manager,
                                       size_t* total_allocated,
                                       size_t* total_used,
                                       uint32_t* resource_count);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_RESOURCE_H */
