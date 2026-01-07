/*
 * mtl_resource_pool.h
 * Metal resource pooling system
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_RESOURCE_POOL_H
#define PLATFORM_MTL_RESOURCE_POOL_H

#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_buffer.h"
#include "backend/metal/mtl_texture.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct metal_resource_pool metal_resource_pool_t;

typedef struct metal_pool_stats {
    size_t total_memory_managed;
    size_t active_memory_used;
    uint32_t active_buffers;
    uint32_t pooled_buffers;
    uint32_t active_textures;
    uint32_t pooled_textures;
} metal_pool_stats_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Create a resource pool */
metal_resource_pool_t* metal_resource_pool_create(metal_device_t* device);

/* Destroy a resource pool and all its resources */
void metal_resource_pool_destroy(metal_resource_pool_t* pool);

/* Acquire a reusable buffer from the pool (or create new if none available) */
metal_buffer_t* metal_pool_acquire_buffer(metal_resource_pool_t* pool, const metal_buffer_desc_t* desc);

/* Return a buffer to the pool for reuse */
void metal_pool_return_buffer(metal_resource_pool_t* pool, metal_buffer_t* buffer);

/* Acquire a reusable texture from the pool */
metal_texture_t* metal_pool_acquire_texture(metal_resource_pool_t* pool, const metal_texture_desc_t* desc);

/* Return a texture to the pool */
void metal_pool_return_texture(metal_resource_pool_t* pool, metal_texture_t* texture);

/* Cleanup unused resources that have been in the pool longer than max_age_frames */
void metal_pool_cleanup(metal_resource_pool_t* pool, uint32_t max_age_frames);

/* Get pool statistics */
metal_pool_stats_t metal_pool_get_stats(metal_resource_pool_t* pool);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_RESOURCE_POOL_H */
