/*
 * mtl_resource_pool.c
 * Metal resource pooling implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_resource_pool.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

#define MAX_POOLED_BUFFERS 256
#define MAX_POOLED_TEXTURES 256

typedef struct cached_buffer {
    metal_buffer_t* buffer;
    uint32_t last_used_frame;
    bool in_use;
} cached_buffer_t;

typedef struct cached_texture {
    metal_texture_t* texture;
    uint32_t last_used_frame;
    bool in_use;
} cached_texture_t;

struct metal_resource_pool {
    metal_device_t* device;
    
    // Simple fixed-size pools for now
    cached_buffer_t buffers[MAX_POOLED_BUFFERS];
    uint32_t buffer_count;
    
    cached_texture_t textures[MAX_POOLED_TEXTURES];
    uint32_t texture_count;
    
    uint32_t current_frame;
    metal_pool_stats_t stats;
};

/* ============================================================================
 * API IMPLEMENTATION
 * ============================================================================ */

metal_resource_pool_t* metal_resource_pool_create(metal_device_t* device) {
    if (!device) return NULL;
    
    metal_resource_pool_t* pool = (metal_resource_pool_t*)calloc(1, sizeof(metal_resource_pool_t));
    if (!pool) return NULL;
    
    pool->device = device;
    return pool;
}

void metal_resource_pool_destroy(metal_resource_pool_t* pool) {
    if (!pool) return;
    
    // Destroy all buffers
    for (uint32_t i = 0; i < pool->buffer_count; i++) {
        if (pool->buffers[i].buffer) {
            metal_buffer_destroy(pool->buffers[i].buffer);
        }
    }
    
    // Destroy all textures
    for (uint32_t i = 0; i < pool->texture_count; i++) {
        if (pool->textures[i].texture) {
            metal_texture_destroy(pool->textures[i].texture);
        }
    }
    
    free(pool);
}

metal_buffer_t* metal_pool_acquire_buffer(metal_resource_pool_t* pool, const metal_buffer_desc_t* desc) {
    if (!pool || !desc) return NULL;
    
    // Search for compatible existing buffer
    for (uint32_t i = 0; i < pool->buffer_count; i++) {
        cached_buffer_t* cached = &pool->buffers[i];
        if (!cached->in_use && cached->buffer) {
            // Check compatibility
            // Needs to be at least the requested size
            // Must match storage mode and usage
            if (cached->buffer->size >= desc->size &&
                cached->buffer->storage_mode == desc->storage_mode &&
                (cached->buffer->usage & desc->usage) == desc->usage) {
                
                cached->in_use = true;
                cached->last_used_frame = pool->current_frame;
                
                // Update stats
                pool->stats.active_buffers++;
                pool->stats.pooled_buffers--;
                pool->stats.active_memory_used += cached->buffer->size;
                
                return cached->buffer;
            }
        }
    }
    
    // Create new buffer
    metal_buffer_t* new_buffer = metal_buffer_create(pool->device, desc);
    if (!new_buffer) return NULL;
    
    // Add to pool tracking if space available
    // (If pool full, we just return the buffer but don't track it, 
    // effectively treating it as unpooled/unmanaged or we could expand pool)
    if (pool->buffer_count < MAX_POOLED_BUFFERS) {
        cached_buffer_t* slot = &pool->buffers[pool->buffer_count++];
        slot->buffer = new_buffer;
        slot->in_use = true;
        slot->last_used_frame = pool->current_frame;
        
        pool->stats.total_memory_managed += new_buffer->size;
        pool->stats.active_buffers++;
        pool->stats.active_memory_used += new_buffer->size;
    }
    
    return new_buffer;
}

void metal_pool_return_buffer(metal_resource_pool_t* pool, metal_buffer_t* buffer) {
    if (!pool || !buffer) return;
    
    // Find buffer in pool
    bool found = false;
    for (uint32_t i = 0; i < pool->buffer_count; i++) {
        if (pool->buffers[i].buffer == buffer) {
            pool->buffers[i].in_use = false;
            
            pool->stats.active_buffers--;
            pool->stats.pooled_buffers++;
            pool->stats.active_memory_used -= buffer->size;
            
            found = true;
            break;
        }
    }
    
    if (!found) {
        // Was not tracked in pool (maybe pool was full)
        // Just destroy it
        metal_buffer_destroy(buffer);
    }
}

metal_texture_t* metal_pool_acquire_texture(metal_resource_pool_t* pool, const metal_texture_desc_t* desc) {
    if (!pool || !desc) return NULL;
    
    // Search pool
    for (uint32_t i = 0; i < pool->texture_count; i++) {
        cached_texture_t* cached = &pool->textures[i];
        if (!cached->in_use && cached->texture) {
            // Check exact match for textures usually required
            if (cached->texture->width == desc->width &&
                cached->texture->height == desc->height &&
                cached->texture->depth == desc->depth &&
                cached->texture->format == desc->format &&
                cached->texture->type == desc->type &&
                cached->texture->mip_levels == desc->mip_levels &&
                cached->texture->array_length == desc->array_length) {
                
                cached->in_use = true;
                cached->last_used_frame = pool->current_frame;
                
                pool->stats.active_textures++;
                pool->stats.pooled_textures--;
                
                return cached->texture;
            }
        }
    }
    
    // Create new
    metal_texture_t* new_tex = metal_texture_create(pool->device, desc);
    if (!new_tex) return NULL;
    
    if (pool->texture_count < MAX_POOLED_TEXTURES) {
        cached_texture_t* slot = &pool->textures[pool->texture_count++];
        slot->texture = new_tex;
        slot->in_use = true;
        slot->last_used_frame = pool->current_frame;
        
        pool->stats.active_textures++;
    }
    
    return new_tex;
}

void metal_pool_return_texture(metal_resource_pool_t* pool, metal_texture_t* texture) {
    if (!pool || !texture) return;
    
    bool found = false;
    for (uint32_t i = 0; i < pool->texture_count; i++) {
        if (pool->textures[i].texture == texture) {
            pool->textures[i].in_use = false;
            
            pool->stats.active_textures--;
            pool->stats.pooled_textures++;
            
            found = true;
            break;
        }
    }
    
    if (!found) {
        metal_texture_destroy(texture);
    }
}

void metal_pool_cleanup(metal_resource_pool_t* pool, uint32_t max_age_frames) {
    if (!pool) return;
    
    pool->current_frame++;
    
    // Actual cleanup logic: destroy resources not used for N frames
    u32 buffers_freed = 0;
    u32 textures_freed = 0;
    
    // Cleanup unused buffers
    for (u32 i = 0; i < pool->buffer_count; i++) {
        if (pool->buffers[i].buffer && !pool->buffers[i].in_use) {
            u32 age = pool->current_frame - pool->buffers[i].last_used_frame;
            if (age > max_age_frames) {
                metal_buffer_destroy(pool->buffers[i].buffer);
                pool->buffers[i].buffer = NULL;
                pool->buffers[i].in_use = false;
                buffers_freed++;
            }
        }
    }
    
    // Cleanup unused textures
    for (u32 i = 0; i < pool->texture_count; i++) {
        if (pool->textures[i].texture && !pool->textures[i].in_use) {
            u32 age = pool->current_frame - pool->textures[i].last_used_frame;
            if (age > max_age_frames) {
                metal_texture_destroy(pool->textures[i].texture);
                pool->textures[i].texture = NULL;
                pool->textures[i].in_use = false;
                textures_freed++;
            }
        }
    }
    
    // Update statistics
    pool->stats.buffers_freed += buffers_freed;
    pool->stats.textures_freed += textures_freed;
    pool->stats.total_cleanups++;
}

metal_pool_stats_t metal_pool_get_stats(metal_resource_pool_t* pool) {
    if (pool) return pool->stats;
    metal_pool_stats_t empty = {0};
    return empty;
}
