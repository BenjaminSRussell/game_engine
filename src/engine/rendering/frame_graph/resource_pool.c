// Frame Graph - Resource Pool
// Manages transient GPU resources with automatic memory aliasing

#include "rendering/frame_graph/frame_graph_internal.h"
#include "rendering/core/texture.h"
#include "rendering/core/buffer.h"
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

// Helper to align size up to nearest multiple
static u64 align_size(u64 size, u64 alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Check if two resources have overlapping lifetimes
static bool lifetimes_overlap(RGResource *a, RGResource *b) {
    // Resources overlap if their lifetime intervals intersect
    // Interval [a_first, a_last] overlaps [b_first, b_last] if:
    // a_first <= b_last && b_first <= a_last
    return a->first_use_pass <= b->last_use_pass && 
           b->first_use_pass <= a->last_use_pass;
}

// Check if two resources are compatible provided they DON'T overlap
static bool resources_compatible_types(RGResource *a, RGResource *b) {
    // Must be same type (Buffer/Texture)
    if (a->type != b->type) return false;
    
    // For buffers, generally compatible if just byte arrays
    if (a->type == RG_RESOURCE_BUFFER) return true;
    
    // For textures, strictly we might want to check bind flags or formats
    // A 32-bit float texture might not alias well with R8G8B8A8 depending on hardware
    // For now, allow simple aliasing if type matches
    return true;
}

// Structure to sort resources by size for bin-packing
typedef struct ResourceSortEntry {
    u32 index;
    u64 size;
} ResourceSortEntry;

static int compare_resource_size_desc(const void *a, const void *b) {
    const ResourceSortEntry *ra = (const ResourceSortEntry *)a;
    const ResourceSortEntry *rb = (const ResourceSortEntry *)b;
    if (rb->size > ra->size) return 1;
    if (rb->size < ra->size) return -1;
    return 0;
}

// Memory aliasing implementation: Best-Fit Bin Packing
void rg_pool_alias_resources(RGResourcePool *pool, RenderGraph *rg) {
    if (!pool || !rg) return;
    
    LOG_DEBUG("Starting resource aliasing analysis (Best-Fit)...");
    
    // Reset alias groups
    pool->alias_group_count = 0;
    u64 total_size = 0;
    u64 aliased_size = 0;
    
    // 1. Collect and sort all transient resources
    ResourceSortEntry entries[RG_MAX_RESOURCES];
    u32 entry_count = 0;
    
    for (u32 i = 1; i < rg->resource_count; i++) {
        RGResource *res = &rg->resources[i];
        if (!res->is_imported) {
            res->alias_group_id = 0xFFFFFFFF;
            res->memory_offset = 0;
            
            // Calculate size with alignment
            u64 align = (res->type == RG_RESOURCE_BUFFER) ? 256 : 1024; // 256b buffer, 1KB tex min
            res->memory_size = align_size(calculate_resource_size(res), align);
            
            entries[entry_count].index = i;
            entries[entry_count].size = res->memory_size;
            entry_count++;
            
            total_size += res->memory_size;
        }
    }
    
    // Sort descending by size (Best-Fit Decreasing heuristic)
    qsort(entries, entry_count, sizeof(ResourceSortEntry), compare_resource_size_desc);
    
    // 2. Allocate resources to alias groups
    for (u32 i = 0; i < entry_count; i++) {
        u32 res_idx = entries[i].index;
        RGResource *res = &rg->resources[res_idx];
        
        // Find best fit existing group
        // Best fit = Group where this resource fits (no overlaps) and increases group size the LEAST
        // Since we process largest to smallest, often we fit "inside" existing groups without growing them
        
        u32 best_group_idx = 0xFFFFFFFF;
        u64 min_growth = 0xFFFFFFFFFFFFFFFF;
        
        for (u32 g = 0; g < pool->alias_group_count; g++) {
            RGAliasGroup *group = &pool->alias_groups[g];
            
            if (group->resource_count >= 16) continue;
            if (group->is_texture != (res->type == RG_RESOURCE_TEXTURE)) continue;
            
            // Check conflicts
            bool conflicts = false;
            for (u32 r = 0; r < group->resource_count; r++) {
                RGResource *other = &rg->resources[group->resource_ids[r]];
                if (lifetimes_overlap(res, other) || !resources_compatible_types(res, other)) {
                    conflicts = true;
                    break;
                }
            }
            
            if (!conflicts) {
                // Determine growth
                u64 growth = 0;
                if (res->memory_size > group->size_bytes) {
                    growth = res->memory_size - group->size_bytes;
                }
                
                // Found a candidate, is it better?
                if (growth < min_growth) {
                    min_growth = growth;
                    best_group_idx = g;
                    
                    // Perfect fit (no growth) - take immediately
                    if (growth == 0) break; 
                }
            }
        }
        
        // Assign to best group or create new
        if (best_group_idx != 0xFFFFFFFF) {
            // Assign to existing
            RGAliasGroup *group = &pool->alias_groups[best_group_idx];
            group->resource_ids[group->resource_count++] = res_idx;
            res->alias_group_id = best_group_idx;
            
            if (res->memory_size > group->size_bytes) {
                group->size_bytes = res->memory_size;
            }
            // Stats: we "saved" the size of this resource (minus growth of the group)
            aliased_size += res->memory_size; 
        } else {
            // Create new group
             if (pool->alias_group_count < 64) {
                 RGAliasGroup *group = &pool->alias_groups[pool->alias_group_count];
                 group->resource_count = 1;
                 group->resource_ids[0] = res_idx;
                 group->size_bytes = res->memory_size;
                 group->is_texture = (res->type == RG_RESOURCE_TEXTURE);
                 group->allocated = false;
                 
                 res->alias_group_id = pool->alias_group_count;
                 pool->alias_group_count++;
                 
                 aliased_size += res->memory_size; // Count initial allocation too for total logic below
             } else {
                 LOG_WARN("Alias group limit reached, resource %s orphaned", res->name);
             }
        }
    }
    
    // Correct aliased_size stat: It should be total_size - actual_allocated
    u64 actual_allocated = 0;
    for (u32 g = 0; g < pool->alias_group_count; g++) {
        actual_allocated += pool->alias_groups[g].size_bytes;
    }
    
    // Store correctly
    pool->total_allocated = actual_allocated; // What we ACTUALLY ask GPU for
    pool->total_aliased = total_size;         // What user asked for (virtual total)
    
    float savings = (total_size > 0) ? 
        (1.0f - (float)actual_allocated / (float)total_size) * 100.0f : 0.0f;
        
    LOG_INFO("Aliasing (Best-Fit): %llu -> %llu bytes (%.1f%% savings)",
             total_size, actual_allocated, savings);
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
