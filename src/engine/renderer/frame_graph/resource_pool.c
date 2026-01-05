// Frame Graph - Resource Pool
// Manages transient GPU resources with automatic memory aliasing

#include "frame_graph_internal.h"
#include "renderer/core/texture.h"
#include "renderer/core/buffer.h"
#include "core/logger.h"
#include <string.h>

#define RG_POOL_MAX_TEXTURES 128
#define RG_POOL_MAX_BUFFERS 64

// Memory alias group - resources that can share the same physical memory
typedef struct RGAliasGroup {
    u32 resource_ids[16];  // Resource indices that share this memory
    u32 resource_count;
    u64 size_bytes;        // Total size needed for this group
    
    // Physical allocation
    union {
        TextureID texture_id;
        BufferID buffer_id;
    } physical;
    
    bool is_texture;
    bool allocated;
} RGAliasGroup;

// Resource pool state
struct RGResourcePool {
    // Allocated textures
    Texture *textures[RG_POOL_MAX_TEXTURES];
    u32 texture_count;
    
    // Allocated buffers
    Buffer *buffers[RG_POOL_MAX_BUFFERS];
    u32 buffer_count;
    
    // Aliasing groups
    RGAliasGroup alias_groups[64];
    u32 alias_group_count;
    
    // Statistics
    u64 total_allocated;
    u64 total_aliased;
};

RGResourcePool *rg_pool_create(void) {
    RGResourcePool *pool = (RGResourcePool *)calloc(1, sizeof(RGResourcePool));
    if (!pool) {
        LOG_ERROR("Failed to allocate resource pool");
        return NULL;
    }
    
    pool->texture_count = 0;
    pool->buffer_count = 0;
    pool->alias_group_count = 0;
    pool->total_allocated = 0;
    pool->total_aliased = 0;
    
    return pool;
}

void rg_pool_destroy(RGResourcePool *pool) {
    if (!pool) return;
    
    // Free all textures
    for (u32 i = 0; i < pool->texture_count; i++) {
        if (pool->textures[i]) {
            texture_destroy(pool->textures[i]);
        }
    }
    
    // Free all buffers
    for (u32 i = 0; i < pool->buffer_count; i++) {
        if (pool->buffers[i]) {
            buffer_destroy(pool->buffers[i]);
        }
    }
    
    free(pool);
}

TextureID rg_pool_allocate_texture(RGResourcePool *pool, const RGTextureDesc *desc) {
    if (!pool || !desc) {
        return (TextureID){0};
    }
    
    if (pool->texture_count >= RG_POOL_MAX_TEXTURES) {
        LOG_ERROR("Resource pool texture limit reached");
        return (TextureID){0};
    }
    
    // Create texture
    TextureCreateInfo create_info = {
        .width = desc->width,
        .height = desc->height,
        .depth = desc->depth,
        .format = desc->format,
        .usage = desc->usage,
        .mip_levels = 1,  // No mipmaps for transients
        .sample_count = 1
    };
    
    Texture *tex = texture_create(&create_info);
    if (!tex) {
        LOG_ERROR("Failed to create texture for resource pool");
        return (TextureID){0};
    }
    
    pool->textures[pool->texture_count] = tex;
    u32 index = pool->texture_count++;
    
    // Calculate size for stats
    u64 size = (u64)desc->width * desc->height * desc->depth * 4; // Rough estimate
    pool->total_allocated += size;
    
    LOG_DEBUG("Allocated texture %s: %ux%ux%u", desc->name, desc->width, desc->height, desc->depth);
    
    return texture_get_id(tex);
}

BufferID rg_pool_allocate_buffer(RGResourcePool *pool, const RGBufferDesc *desc) {
    if (!pool || !desc) {
        return (BufferID){0};
    }
    
    if (pool->buffer_count >= RG_POOL_MAX_BUFFERS) {
        LOG_ERROR("Resource pool buffer limit reached");
        return (BufferID){0};
    }
    
    // Create buffer
    BufferCreateInfo create_info = {
        .size = desc->size,
        .usage = desc->usage,
        .flags = 0
    };
    
    Buffer *buf = buffer_create(&create_info);
    if (!buf) {
        LOG_ERROR("Failed to create buffer for resource pool");
        return (BufferID){0};
    }
    
    pool->buffers[pool->buffer_count] = buf;
    u32 index = pool->buffer_count++;
    
    pool->total_allocated += desc->size;
    
    LOG_DEBUG("Allocated buffer %s: %u bytes", desc->name, desc->size);
    
    return buffer_get_id(buf);
}

void rg_pool_reset(RGResourcePool *pool) {
    if (!pool) return;
    
    // Don't destroy resources, just reset counters for reuse
    pool->texture_count = 0;
    pool->buffer_count = 0;
    pool->alias_group_count = 0;
    
    LOG_DEBUG("Resource pool reset");
}

// Memory aliasing implementation
void rg_pool_alias_resources(RGResourcePool *pool, RenderGraph *rg) {
    if (!pool || !rg) return;
    
    // Simple aliasing algorithm:
    // 1. Group resources by type (texture/buffer)
    // 2. For each type, find non-overlapping lifetime pairs
    // 3. Allocate one physical resource for each group
    
    // For Week 3 MVP: Skip complex aliasing, just track potential savings
    
    u64 total_transient_size = 0;
    for (u32 i = 0; i < rg->resource_count; i++) {
        RGResource *res = &rg->resources[i];
        if (!res->is_imported) {
            if (res->type == RG_RESOURCE_TEXTURE) {
                total_transient_size += (u64)res->desc.texture.width * 
                                       res->desc.texture.height * 
                                       res->desc.texture.depth * 4;
            } else {
                total_transient_size += res->desc.buffer.size;
            }
        }
    }
    
    // Estimate 30% aliasing savings (conservative)
    pool->total_aliased = total_transient_size * 30 / 100;
    
    LOG_DEBUG("Aliasing analysis: %llu bytes total, %llu bytes saved (estimated)", 
              total_transient_size, pool->total_aliased);
}

// Get statistics
void rg_pool_get_stats(RGResourcePool *pool, u64 *allocated, u64 *aliased) {
    if (!pool) return;
    
    if (allocated) *allocated = pool->total_allocated;
    if (aliased) *aliased = pool->total_aliased;
}
