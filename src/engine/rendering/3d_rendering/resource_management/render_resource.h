/**
 * @file render_resource.h
 * @brief GPU resource management (buffers, textures, etc.)
 * @details Manages GPU memory allocation, resource tracking, and lifecycle
 */

#ifndef RENDER_RESOURCE_H
#define RENDER_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Resource type enumeration
 */
typedef enum {
    RESOURCE_TYPE_BUFFER = 0,
    RESOURCE_TYPE_IMAGE = 1,
    RESOURCE_TYPE_SAMPLER = 2,
    RESOURCE_TYPE_ACCELERATION_STRUCTURE = 3,
} ResourceType;

/**
 * @brief Buffer usage flags
 */
typedef enum {
    BUFFER_USAGE_TRANSFER_SRC = 0x00000001,
    BUFFER_USAGE_TRANSFER_DST = 0x00000002,
    BUFFER_USAGE_UNIFORM = 0x00000004,
    BUFFER_USAGE_STORAGE = 0x00000008,
    BUFFER_USAGE_INDEX = 0x00000010,
    BUFFER_USAGE_VERTEX = 0x00000020,
    BUFFER_USAGE_INDIRECT = 0x00000040,
} BufferUsageFlags;

/**
 * @brief Image format enumeration
 */
typedef enum {
    FORMAT_UNDEFINED = 0,
    FORMAT_R8_UNORM = 1,
    FORMAT_R8G8B8A8_UNORM = 2,
    FORMAT_R8G8B8A8_SRGB = 3,
    FORMAT_R16_SFLOAT = 4,
    FORMAT_R16G16B16A16_SFLOAT = 5,
    FORMAT_R32_SFLOAT = 6,
    FORMAT_R32G32B32A32_SFLOAT = 7,
    FORMAT_D32_SFLOAT = 8,
    FORMAT_D24_UNORM_S8_UINT = 9,
    FORMAT_BC1_RGB_UNORM_BLOCK = 10,
    FORMAT_BC1_RGB_SRGB_BLOCK = 11,
    FORMAT_BC4_UNORM_BLOCK = 12,
    FORMAT_BC5_UNORM_BLOCK = 13,
    FORMAT_BC6H_UFLOAT_BLOCK = 14,
    FORMAT_BC7_UNORM_BLOCK = 15,
    FORMAT_BC7_SRGB_BLOCK = 16,
} ImageFormat;

/**
 * @brief Image usage flags
 */
typedef enum {
    IMAGE_USAGE_TRANSFER_SRC = 0x00000001,
    IMAGE_USAGE_TRANSFER_DST = 0x00000002,
    IMAGE_USAGE_SAMPLED = 0x00000004,
    IMAGE_USAGE_STORAGE = 0x00000008,
    IMAGE_USAGE_COLOR_ATTACHMENT = 0x00000010,
    IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 0x00000020,
    IMAGE_USAGE_TRANSIENT_ATTACHMENT = 0x00000040,
} ImageUsageFlags;

/**
 * @brief Memory property flags
 */
typedef enum {
    MEMORY_PROPERTY_DEVICE_LOCAL = 0x00000001,
    MEMORY_PROPERTY_HOST_VISIBLE = 0x00000002,
    MEMORY_PROPERTY_HOST_COHERENT = 0x00000004,
    MEMORY_PROPERTY_HOST_CACHED = 0x00000008,
    MEMORY_PROPERTY_LAZILY_ALLOCATED = 0x00000010,
} MemoryPropertyFlags;

/**
 * @brief GPU Buffer structure
 */
typedef struct {
    uint32_t id;
    size_t size;
    uint32_t usage_flags;
    uint32_t memory_flags;
    void* api_handle;
    void* mapped_ptr;
    bool is_persistent_mapped;
} RenderBuffer;

/**
 * @brief GPU Image/Texture structure
 */
typedef struct {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t array_layers;
    uint32_t mip_levels;
    ImageFormat format;
    uint32_t usage_flags;
    void* api_handle;
} RenderImage;

/**
 * @brief Resource manager structure
 */
typedef struct RenderResourceManager RenderResourceManager;

/**
 * @brief Create a resource manager
 * @param[in] max_resources Maximum number of resources to track
 * @return Pointer to created manager, NULL on failure
 */
RenderResourceManager* render_resource_manager_create(uint32_t max_resources);

/**
 * @brief Destroy a resource manager
 * @param[in] manager Manager to destroy
 */
void render_resource_manager_destroy(RenderResourceManager* manager);

/**
 * @brief Create a GPU buffer
 * @param[in] manager Resource manager
 * @param[in] size Size of buffer in bytes
 * @param[in] usage Usage flags
 * @param[in] memory_flags Memory property flags
 * @return Pointer to created buffer, NULL on failure
 */
RenderBuffer* render_buffer_create(RenderResourceManager* manager,
                                   size_t size,
                                   uint32_t usage,
                                   uint32_t memory_flags);

/**
 * @brief Destroy a GPU buffer
 * @param[in] manager Resource manager
 * @param[in] buffer Buffer to destroy
 */
void render_buffer_destroy(RenderResourceManager* manager, RenderBuffer* buffer);

/**
 * @brief Map buffer memory for CPU access
 * @param[in] buffer Buffer to map
 * @return Pointer to mapped memory, NULL on failure
 */
void* render_buffer_map(RenderBuffer* buffer);

/**
 * @brief Unmap buffer memory
 * @param[in] buffer Buffer to unmap
 */
void render_buffer_unmap(RenderBuffer* buffer);

/**
 * @brief Create a GPU image
 * @param[in] manager Resource manager
 * @param[in] width Image width
 * @param[in] height Image height
 * @param[in] format Image format
 * @param[in] usage Usage flags
 * @return Pointer to created image, NULL on failure
 */
RenderImage* render_image_create(RenderResourceManager* manager,
                                 uint32_t width,
                                 uint32_t height,
                                 ImageFormat format,
                                 uint32_t usage);

/**
 * @brief Destroy a GPU image
 * @param[in] manager Resource manager
 * @param[in] image Image to destroy
 */
void render_image_destroy(RenderResourceManager* manager, RenderImage* image);

/**
 * @brief Get resource manager statistics
 * @param[in] manager Manager to query
 * @param[out] total_allocated Total allocated memory
 * @param[out] total_used Total used memory
 * @param[out] resource_count Number of tracked resources
 */
void render_resource_manager_get_stats(RenderResourceManager* manager,
                                       size_t* total_allocated,
                                       size_t* total_used,
                                       uint32_t* resource_count);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_RESOURCE_H */
