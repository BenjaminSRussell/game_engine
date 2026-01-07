// Frame Graph - Resource Pool
// Manages transient GPU resources with automatic memory aliasing

#include "frame_graph_internal.h"
#include "renderer/core/texture.h"
#include "renderer/core/buffer.h"
#include "core/logger.h"
#include "core/types.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    
    LOG_DEBUG("Allocated texture %s: %ux%ux%u (pool index %u)", 
             desc->name, desc->width, desc->height, desc->depth, index);
    
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
    
    LOG_DEBUG("Allocated buffer %s: %u bytes (pool index %u)", desc->name, desc->size, index);
    
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

// === Resource Aliasing Implementation ===

// Helper function to calculate resource size in bytes
static u64 calculate_resource_size(RGResource *res) {
    if (!res) return 0;
    
    if (res->type == RG_RESOURCE_TEXTURE) {
        // Rough estimate: width * height * depth * bytes_per_pixel
        // For more accuracy, would need to look up actual format sizes
        u32 bpp = 4; // Assume RGBA8 for now
        return (u64)res->desc.texture.width * 
               res->desc.texture.height * 
               res->desc.texture.depth * bpp;
    } else {
        return res->desc.buffer.size;
    }
}

// Check if two resources have overlapping lifetimes
static bool lifetimes_overlap(RGResource *a, RGResource *b) {
    // Resources overlap if their lifetime intervals intersect
    // Interval [a_first, a_last] overlaps [b_first, b_last] if:
    // a_first <= b_last && b_first <= a_last
    return a->first_use_pass <= b->last_use_pass && 
           b->first_use_pass <= a->last_use_pass;
}

// Check if two resources are compatible for aliasing
static bool resources_compatible(RGResource *a, RGResource *b) {
    // Must be same type
    if (a->type != b->type) return false;
    
    // Must have similar sizes (within 25% to avoid waste)
    u64 size_a = calculate_resource_size(a);
    u64 size_b = calculate_resource_size(b);
    u64 min_size = size_a < size_b ? size_a : size_b;
    u64 max_size = size_a > size_b ? size_a : size_b;
    
    if (max_size > min_size + min_size / 4) {
        return false; // Size difference too large
    }
    
    // For textures, could also check format compatibility
    // For now, we just check type and size
    
    return true;
}

// Memory aliasing implementation
void rg_pool_alias_resources(RGResourcePool *pool, RenderGraph *rg) {
    if (!pool || !rg) return;
    
    LOG_DEBUG("Starting resource aliasing analysis...");
    
    // Reset alias groups
    pool->alias_group_count = 0;
    u64 total_size = 0;
    u64 aliased_size = 0;
    
    // Initialize all transient resources as unassigned (alias_group_id = 0xFFFFFFFF)
    for (u32 i = 1; i < rg->resource_count; i++) {
        RGResource *res = &rg->resources[i];
        if (!res->is_imported) {
            res->alias_group_id = 0xFFFFFFFF;
            res->memory_offset = 0;
            res->memory_size = calculate_resource_size(res);
            total_size += res->memory_size;
        }
    }
    
    // Greedy interval scheduling algorithm for memory aliasing
    // For each unassigned resource, try to find an existing alias group
    // If none found, create a new group
    
    for (u32 i = 1; i < rg->resource_count; i++) {
        RGResource *res = &rg->resources[i];
        
        // Skip imported resources
        if (res->is_imported) continue;
        
        // Skip already assigned
        if (res->alias_group_id != 0xFFFFFFFF) continue;
        
        // Try to find an existing alias group where this resource fits
        bool found_group = false;
        
        for (u32 g = 0; g < pool->alias_group_count && !found_group; g++) {
            RGAliasGroup *group = &pool->alias_groups[g];
            
            // Check if this resource is compatible with the group
            if (group->is_texture != (res->type == RG_RESOURCE_TEXTURE)) {
                continue; // Type mismatch
            }
            
            // Check if this resource's lifetime overlaps with any resource in the group
            bool conflicts = false;
            for (u32 r = 0; r < group->resource_count; r++) {
                u32 other_id = group->resource_ids[r];
                RGResource *other = &rg->resources[other_id];
                
                if (lifetimes_overlap(res, other) || !resources_compatible(res, other)) {
                    conflicts = true;
                    break;
                }
            }
            
            if (!conflicts && group->resource_count < 16) {
                // Add to this group
                group->resource_ids[group->resource_count++] = i;
                res->alias_group_id = g;
                
                // Update group size to accommodate largest resource
                if (res->memory_size > group->size_bytes) {
                    group->size_bytes = res->memory_size;
                }
                
                found_group = true;
                aliased_size += res->memory_size;
                
                LOG_DEBUG("  Resource %s aliased to group %u", res->name, g);
            }
        }
        
        // If no existing group fits, create a new one
        if (!found_group) {
            if (pool->alias_group_count >= 64) {
                LOG_WARN("Alias group limit reached, resource %s not aliased", res->name);
                continue;
            }
            
            RGAliasGroup *group = &pool->alias_groups[pool->alias_group_count];
            group->resource_count = 1;
            group->resource_ids[0] = i;
            group->size_bytes = res->memory_size;
            group->is_texture = (res->type == RG_RESOURCE_TEXTURE);
            group->allocated = false;
            
            res->alias_group_id = pool->alias_group_count;
            pool->alias_group_count++;
            
            LOG_DEBUG("  Created new alias group %u for resource %s", 
                     pool->alias_group_count - 1, res->name);
        }
    }
    
    // Calculate actual savings
    pool->total_aliased = aliased_size;
    
    LOG_INFO("Aliasing complete: %u groups, %llu / %llu bytes aliased (%.1f%% savings)",
             pool->alias_group_count,
             aliased_size,
             total_size,
             total_size > 0 ? (100.0f * aliased_size / total_size) : 0.0f);
}

// Get statistics
void rg_pool_get_stats(RGResourcePool *pool, u64 *allocated, u64 *aliased) {
    if (!pool) return;
    
    if (allocated) *allocated = pool->total_allocated;
    if (aliased) *aliased = pool->total_aliased;
}

// Public helper function for resource size calculation
u64 rg_calculate_resource_size(RGResource *res) {
    return calculate_resource_size(res);
}
