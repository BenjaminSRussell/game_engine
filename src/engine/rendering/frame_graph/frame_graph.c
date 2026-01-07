// Frame Graph System - Core Implementation
#include "rendering/frame_graph/frame_graph_internal.h"
#include "core/logger.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// === Graph Lifecycle ===

RenderGraph *rg_create(void) {
    RenderGraph *rg = (RenderGraph *)calloc(1, sizeof(RenderGraph));
    if (!rg) {
        LOG_ERROR("Failed to allocate render graph");
        return NULL;
    }
    
    rg->resource_count = 1;
    rg->pass_count = 1;
    rg->execution_count = 0;
    rg->barrier_count = 0;
    rg->is_compiled = false;
    rg->profiling_enabled = false;
    
    // Create resource pool
    rg->resource_pool = rg_pool_create();
    if (!rg->resource_pool) {
        LOG_ERROR("Failed to create resource pool for render graph");
        free(rg);
        return NULL;
    }
    
    memset(&rg->stats, 0, sizeof(RGStats));
    
    // Phase 1: Initialize profiling and barrier stats
    memset(&rg->profiling, 0, sizeof(RGProfilingState));
    memset(&rg->barrier_stats, 0, sizeof(RGBarrierStats));
    for (u32 i = 0; i < RG_MAX_PASSES; i++) {
        rg->pass_queues[i] = RG_QUEUE_GRAPHICS;  // Default to graphics queue
    }
    
    LOG_INFO("Render graph created");
    return rg;
}

void rg_destroy(RenderGraph *rg) {
    if (!rg) return;
    
    // Free user data from passes (if needed)
    for (u32 i = 0; i < rg->pass_count; i++) {
        // Note: user_data is owned by caller, we just hold a pointer
        rg->passes[i].user_data = NULL;
    }
    
    // Destroy resource pool
    if (rg->resource_pool) {
        rg_pool_destroy(rg->resource_pool);
        rg->resource_pool = NULL;
    }
    
    free(rg);
    LOG_INFO("Render graph destroyed");
}

void rg_reset(RenderGraph *rg) {
    if (!rg) return;
    
    rg->resource_count = 1;
    rg->pass_count = 1;
    rg->execution_count = 0;
    rg->barrier_count = 0;
    rg->is_compiled = false;
    
    // Reset resource pool for reuse
    if (rg->resource_pool) {
        rg_pool_reset(rg->resource_pool);
    }
    
    memset(&rg->stats, 0, sizeof(RGStats));
    
    // Phase 1: Reset profiling and barrier stats
    memset(&rg->profiling, 0, sizeof(RGProfilingState));
    memset(&rg->barrier_stats, 0, sizeof(RGBarrierStats));
}

// === Resource Declaration ===

RGResourceHandle rg_create_texture(RenderGraph *rg, const RGTextureDesc *desc) {
    if (!rg || !desc) {
        return RG_INVALID_RESOURCE;
    }
    
    if (rg->resource_count >= RG_MAX_RESOURCES) {
        LOG_ERROR("Render graph resource limit reached (%d)", RG_MAX_RESOURCES);
        return RG_INVALID_RESOURCE;
    }
    
    u32 id = rg->resource_count++;
    RGResource *res = &rg->resources[id];
    
    res->id = id;
    res->type = RG_RESOURCE_TEXTURE;
    res->desc.texture = *desc;
    res->first_use_pass = 0xFFFFFFFF;
    res->last_use_pass = 0;
    res->is_imported = false;
    res->alias_pool_index = 0xFFFFFFFF;
    res->current_state = RG_STATE_UNDEFINED;
    
    // Copy name
    if (desc->name) {
        strncpy(res->name, desc->name, sizeof(res->name) - 1);
        res->name[sizeof(res->name) - 1] = '\0';
    } else {
        snprintf(res->name, sizeof(res->name), "Texture_%u", id);
    }
    
    LOG_DEBUG("Created texture resource: %s (%ux%u)", res->name, desc->width, desc->height);
    
    return (RGResourceHandle){id};
}

RGResourceHandle rg_create_buffer(RenderGraph *rg, const RGBufferDesc *desc) {
    if (!rg || !desc) {
        return RG_INVALID_RESOURCE;
    }
    
    if (rg->resource_count >= RG_MAX_RESOURCES) {
        LOG_ERROR("Render graph resource limit reached (%d)", RG_MAX_RESOURCES);
        return RG_INVALID_RESOURCE;
    }
    
    u32 id = rg->resource_count++;
    RGResource *res = &rg->resources[id];
    
    res->id = id;
    res->type = RG_RESOURCE_BUFFER;
    res->desc.buffer = *desc;
    res->first_use_pass = 0xFFFFFFFF;
    res->last_use_pass = 0;
    res->is_imported = false;
    res->alias_pool_index = 0xFFFFFFFF;
    res->current_state = RG_STATE_UNDEFINED;
    
    // Copy name
    if (desc->name) {
        strncpy(res->name, desc->name, sizeof(res->name) - 1);
        res->name[sizeof(res->name) - 1] = '\0';
    } else {
        snprintf(res->name, sizeof(res->name), "Buffer_%u", id);
    }
    
    LOG_DEBUG("Created buffer resource: %s (%u bytes)", res->name, desc->size);
    
    return (RGResourceHandle){id};
}

RGResourceHandle rg_import_texture(RenderGraph *rg, TextureID external, const char *name) {
    if (!rg) {
        return RG_INVALID_RESOURCE;
    }
    
    if (rg->resource_count >= RG_MAX_RESOURCES) {
        LOG_ERROR("Render graph resource limit reached (%d)", RG_MAX_RESOURCES);
        return RG_INVALID_RESOURCE;
    }
    
    u32 id = rg->resource_count++;
    RGResource *res = &rg->resources[id];
    
    res->id = id;
    res->type = RG_RESOURCE_TEXTURE;
    res->is_imported = true;
    res->physical.texture_id = external;
    res->first_use_pass = 0xFFFFFFFF;
    res->last_use_pass = 0;
    res->current_state = RG_STATE_UNDEFINED;
    
    // Copy name
    if (name) {
        strncpy(res->name, name, sizeof(res->name) - 1);
        res->name[sizeof(res->name) - 1] = '\0';
    } else {
        snprintf(res->name, sizeof(res->name), "ImportedTexture_%u", id);
    }
    
    LOG_DEBUG("Imported texture resource: %s", res->name);
    
    return (RGResourceHandle){id};
}

RGResourceHandle rg_import_buffer(RenderGraph *rg, BufferID external, const char *name) {
    if (!rg) {
        return RG_INVALID_RESOURCE;
    }
    
    if (rg->resource_count >= RG_MAX_RESOURCES) {
        LOG_ERROR("Render graph resource limit reached (%d)", RG_MAX_RESOURCES);
        return RG_INVALID_RESOURCE;
    }
    
    u32 id = rg->resource_count++;
    RGResource *res = &rg->resources[id];
    
    res->id = id;
    res->type = RG_RESOURCE_BUFFER;
    res->is_imported = true;
    res->physical.buffer_id = external;
    res->first_use_pass = 0xFFFFFFFF;
    res->last_use_pass = 0;
    res->current_state = RG_STATE_UNDEFINED;
    
    // Copy name
    if (name) {
        strncpy(res->name, name, sizeof(res->name) - 1);
        res->name[sizeof(res->name) - 1] = '\0';
    } else {
        snprintf(res->name, sizeof(res->name), "ImportedBuffer_%u", id);
    }
    
    LOG_DEBUG("Imported buffer resource: %s", res->name);
    
    return (RGResourceHandle){id};
}

// === Pass Declaration ===

RGPassHandle rg_add_pass(RenderGraph *rg, const RGPassDesc *desc) {
    if (!rg || !desc) {
        return RG_INVALID_PASS;
    }
    
    if (rg->pass_count >= RG_MAX_PASSES) {
        LOG_ERROR("Render graph pass limit reached (%d)", RG_MAX_PASSES);
        return RG_INVALID_PASS;
    }
    
    u32 id = rg->pass_count++;
    RGPass *pass = &rg->passes[id];
    
    pass->id = id;
    pass->execute = desc->execute;
    pass->user_data = desc->user_data;
    pass->read_count = 0;
    pass->write_count = 0;
    pass->is_culled = false;
    pass->ref_count = 0;
    pass->gpu_time_ms = 0.0f;
    
    // Copy name
    if (desc->name) {
        strncpy(pass->name, desc->name, sizeof(pass->name) - 1);
        pass->name[sizeof(pass->name) - 1] = '\0';
    } else {
        snprintf(pass->name, sizeof(pass->name), "Pass_%u", id);
    }
    
    LOG_DEBUG("Added pass: %s", pass->name);
    
    // Phase 1 addition: multi-queue support
    rg->pass_queues[id] = desc->queue_type;
    
    return (RGPassHandle){id};
}

void rg_pass_read(RenderGraph *rg, RGPassHandle pass_handle, RGResourceHandle resource) {
    if (!rg) return;
    
    RGPass *pass = rg_get_pass(rg, pass_handle);
    if (!pass) {
        LOG_ERROR("Invalid pass handle");
        return;
    }
    
    if (pass->read_count >= 32) {
        LOG_ERROR("Pass %s: too many read dependencies", pass->name);
        return;
    }
    
    pass->reads[pass->read_count++] = resource;
    
    // Update resource lifetime
    RGResource *res = rg_get_resource(rg, resource);
    if (res) {
        if (pass->id < res->first_use_pass) {
            res->first_use_pass = pass->id;
        }
        if (pass->id > res->last_use_pass) {
            res->last_use_pass = pass->id;
        }
    }
}

void rg_pass_write(RenderGraph *rg, RGPassHandle pass_handle, RGResourceHandle resource) {
    if (!rg) return;
    
    RGPass *pass = rg_get_pass(rg, pass_handle);
    if (!pass) {
        LOG_ERROR("Invalid pass handle");
        return;
    }
    
    if (pass->write_count >= 32) {
        LOG_ERROR("Pass %s: too many write dependencies", pass->name);
        return;
    }
    
    pass->writes[pass->write_count++] = resource;
    
    // Update resource lifetime
    RGResource *res = rg_get_resource(rg, resource);
    if (res) {
        if (pass->id < res->first_use_pass) {
            res->first_use_pass = pass->id;
        }
        if (pass->id > res->last_use_pass) {
            res->last_use_pass = pass->id;
        }
    }
}

// === Internal Helpers ===

RGResource *rg_get_resource(RenderGraph *rg, RGResourceHandle handle) {
    if (!rg || handle.id >= rg->resource_count) {
        return NULL;
    }
    return &rg->resources[handle.id];
}

RGPass *rg_get_pass(RenderGraph *rg, RGPassHandle handle) {
    if (!rg || handle.id >= rg->pass_count) {
        return NULL;
    }
    return &rg->passes[handle.id];
}

// === Statistics ===

void rg_get_stats(RenderGraph *rg, RGStats *out) {
    if (!rg || !out) return;
    *out = rg->stats;
}
