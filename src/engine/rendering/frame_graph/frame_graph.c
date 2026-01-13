// src/engine/rendering/frame_graph/frame_graph.c
// Frame Graph System Implementation - Modern render graph with automatic resource management

#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <core/string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "frame_graph.h"
#include "frame_graph_internal.h"
#include "../core/renderer.h"
#include "../core/texture.h"
#include "../core/buffer.h"

// ============================================================================
// Constants and Limits
// ============================================================================

#define MAX_RESOURCES 1024
#define MAX_PASSES 512
#define MAX_DEPENDENCIES_PER_RESOURCE 16
#define MAX_DEPENDENCIES_PER_PASS 32

// ============================================================================
// Internal Resource Structure
// ============================================================================

typedef struct RGResource {
    RGResourceHandle handle;
    RGResourceType type;
    char name[256];
    
    // Resource descriptor
    union {
        RGTextureDesc texture_desc;
        RGBufferDesc buffer_desc;
    } desc;
    
    // Physical resource (after compilation)
    union {
        TextureID texture_id;
        BufferID buffer_id;
    } physical;
    
    // Lifetime tracking
    uint32_t first_use_pass;
    uint32_t last_use_pass;
    bool is_imported;
    bool is_culled;
    
    // Versioning for read-modify-write
    uint32_t version;
    RGResourceHandle base_resource;
} RGResource;

// ============================================================================
// Internal Pass Structure
// ============================================================================

typedef struct RGPass {
    RGPassHandle handle;
    char name[256];
    RGPassExecuteFunc execute;
    void* user_data;
    RGQueueType queue_type;
    uint32_t priority;
    
    // Resource dependencies
    RGResourceHandle reads[MAX_DEPENDENCIES_PER_PASS];
    uint32_t read_count;
    
    RGResourceHandle writes[MAX_DEPENDENCIES_PER_PASS];
    uint32_t write_count;
    
    // Execution state
    bool is_culled;
    uint32_t execution_order;
    
    // Profiling
    uint32_t timestamp_query_begin;
    uint32_t timestamp_query_end;
    float gpu_time_ms;
} RGPass;

// ============================================================================
// Render Graph Internal Structure
// ============================================================================

struct RenderGraph {
    // Resources and passes
    RGResource resources[MAX_RESOURCES];
    uint32_t resource_count;
    uint32_t next_resource_id;
    
    RGPass passes[MAX_PASSES];
    uint32_t pass_count;
    uint32_t next_pass_id;
    
    // Compilation state
    bool is_compiled;
    bool has_errors;
    char error_message[1024];
    
    // Execution state
    uint32_t current_frame;
    bool profiling_enabled;
    
    // Statistics
    RGStats stats;
    
    // Memory pools for transient resources
    struct {
        uint64_t total_size;
        uint64_t used_size;
        uint64_t aliased_size;
    } memory_pool;
    
    // Scheduling and execution validation
    bool validation_enabled;
    uint64_t validation_errors;
    uint64_t execution_count;
    uint64_t last_validation_time;
    uint64_t total_execution_time;
    uint64_t max_pass_execution_time;
    uint64_t min_pass_execution_time;
};

// ============================================================================
// Utility Functions
// ============================================================================

static RGResource* find_resource(RenderGraph* rg, RGResourceHandle handle);
static RGPass* find_pass(RenderGraph* rg, RGPassHandle handle);
static bool add_pass_dependency(RGPass* pass, RGResourceHandle resource, bool is_write);
static bool rg_validate_execution_state(RenderGraph* rg);
static void rg_log_validation_error(RenderGraph* rg, const char* error);
static uint64_t rg_get_timestamp(void);
static bool rg_validate_pass_dependencies(RenderGraph* rg);
static bool rg_validate_resource_lifetimes(RenderGraph* rg);

// ============================================================================
// Graph Lifecycle
// ============================================================================

RenderGraph* rg_create(void) {
    RenderGraph* rg = calloc(1, sizeof(RenderGraph));
    if (!rg) {
        LOG_ERROR("Failed to allocate render graph");
        return NULL;
    }
    
    rg->profiling_enabled = false;
    rg->current_frame = 0;
    
    // Initialize validation state
    rg->validation_enabled = true;
    rg->validation_errors = 0;
    rg->execution_count = 0;
    rg->last_validation_time = rg_get_timestamp();
    rg->total_execution_time = 0;
    rg->max_pass_execution_time = 0;
    rg->min_pass_execution_time = UINT64_MAX;
    
    LOG_INFO("Render graph created with execution validation enabled");
    return rg;
}

void rg_destroy(RenderGraph* rg) {
    if (!rg) return;
    
    // Report execution statistics before destruction
    if (rg->validation_enabled) {
        LOG_INFO("Frame Graph Execution Statistics:");
        LOG_INFO("  Total executions: %lu", rg->execution_count);
        LOG_INFO("  Validation errors: %lu", rg->validation_errors);
        LOG_INFO("  Total execution time: %.2f ms", rg->total_execution_time / 1000000.0);
        
        if (rg->execution_count > 0) {
            LOG_INFO("  Average execution time: %.2f ms", 
                    rg->total_execution_time / rg->execution_count / 1000000.0);
            LOG_INFO("  Max pass execution time: %.2f ms", rg->max_pass_execution_time / 1000000.0);
            LOG_INFO("  Min pass execution time: %.2f ms", 
                    rg->min_pass_execution_time == UINT64_MAX ? 0.0 : rg->min_pass_execution_time / 1000000.0);
        }
    }
    
    // Cleanup all transient resources
    for (uint32_t i = 0; i < rg->resource_count; i++) {
        RGResource* resource = &rg->resources[i];
        if (!resource->is_imported) {
            if (resource->type == RG_RESOURCE_TEXTURE) {
                texture_destroy(resource->physical.texture_id);
            } else if (resource->type == RG_RESOURCE_BUFFER) {
                buffer_destroy(resource->physical.buffer_id);
            }
        }
    }
    
    free(rg);
    LOG_INFO("Render graph destroyed");
}

void rg_reset(RenderGraph* rg) {
    if (!rg) return;
    
    // Cleanup transient resources
    for (uint32_t i = 0; i < rg->resource_count; i++) {
        RGResource* resource = &rg->resources[i];
        if (!resource->is_imported) {
            if (resource->type == RG_RESOURCE_TEXTURE) {
                texture_destroy(resource->physical.texture_id);
            } else if (resource->type == RG_RESOURCE_BUFFER) {
                buffer_destroy(resource->physical.buffer_id);
            }
        }
    }
    
    // Reset arrays
    rg->resource_count = 0;
    rg->pass_count = 0;
    rg->next_resource_id = 0;
    rg->next_pass_id = 0;
    rg->is_compiled = false;
    rg->has_errors = false;
    rg->error_message[0] = '\0';
    
    memset(&rg->stats, 0, sizeof(RGStats));
    memset(&rg->memory_pool, 0, sizeof(rg->memory_pool));
    
    rg->current_frame++;
}

// ============================================================================
// Resource Declaration
// ============================================================================

RGResourceHandle rg_create_texture(RenderGraph* rg, const RGTextureDesc* desc) {
    if (!rg || !desc || rg->resource_count >= MAX_RESOURCES) {
        return RG_INVALID_RESOURCE;
    }
    
    RGResource* resource = &rg->resources[rg->resource_count];
    memset(resource, 0, sizeof(RGResource));
    
    resource->handle.id = ++rg->next_resource_id;
    resource->type = RG_RESOURCE_TEXTURE;
    resource->desc.texture_desc = *desc;
    resource->is_imported = false;
    resource->first_use_pass = UINT32_MAX;
    resource->last_use_pass = 0;
    
    if (desc->name) {
        strncpy(resource->name, desc->name, sizeof(resource->name) - 1);
    } else {
        snprintf(resource->name, sizeof(resource->name), "texture_%u", resource->handle.id);
    }
    
    rg->resource_count++;
    
    LOG_DEBUG("Created texture resource '%s' (%ux%u)", 
              resource->name, desc->width, desc->height);
    
    return resource->handle;
}

RGResourceHandle rg_create_buffer(RenderGraph* rg, const RGBufferDesc* desc) {
    if (!rg || !desc || rg->resource_count >= MAX_RESOURCES) {
        return RG_INVALID_RESOURCE;
    }
    
    RGResource* resource = &rg->resources[rg->resource_count];
    memset(resource, 0, sizeof(RGResource));
    
    resource->handle.id = ++rg->next_resource_id;
    resource->type = RG_RESOURCE_BUFFER;
    resource->desc.buffer_desc = *desc;
    resource->is_imported = false;
    resource->first_use_pass = UINT32_MAX;
    resource->last_use_pass = 0;
    
    if (desc->name) {
        strncpy(resource->name, desc->name, sizeof(resource->name) - 1);
    } else {
        snprintf(resource->name, sizeof(resource->name), "buffer_%u", resource->handle.id);
    }
    
    rg->resource_count++;
    
    LOG_DEBUG("Created buffer resource '%s' (%u bytes)", 
              resource->name, desc->size);
    
    return resource->handle;
}

RGResourceHandle rg_import_texture(RenderGraph* rg, TextureID external, const char* name) {
    if (!rg || rg->resource_count >= MAX_RESOURCES) {
        return RG_INVALID_RESOURCE;
    }
    
    RGResource* resource = &rg->resources[rg->resource_count];
    memset(resource, 0, sizeof(RGResource));
    
    resource->handle.id = ++rg->next_resource_id;
    resource->type = RG_RESOURCE_TEXTURE;
    resource->physical.texture_id = external;
    resource->is_imported = true;
    resource->first_use_pass = UINT32_MAX;
    resource->last_use_pass = 0;
    
    if (name) {
        strncpy(resource->name, name, sizeof(resource->name) - 1);
    } else {
        snprintf(resource->name, sizeof(resource->name), "imported_texture_%u", resource->handle.id);
    }
    
    rg->resource_count++;
    
    LOG_DEBUG("Imported texture resource '%s'", resource->name);
    
    return resource->handle;
}

RGResourceHandle rg_import_buffer(RenderGraph* rg, BufferID external, const char* name) {
    if (!rg || rg->resource_count >= MAX_RESOURCES) {
        return RG_INVALID_RESOURCE;
    }
    
    RGResource* resource = &rg->resources[rg->resource_count];
    memset(resource, 0, sizeof(RGResource));
    
    resource->handle.id = ++rg->next_resource_id;
    resource->type = RG_RESOURCE_BUFFER;
    resource->physical.buffer_id = external;
    resource->is_imported = true;
    resource->first_use_pass = UINT32_MAX;
    resource->last_use_pass = 0;
    
    if (name) {
        strncpy(resource->name, name, sizeof(resource->name) - 1);
    } else {
        snprintf(resource->name, sizeof(resource->name), "imported_buffer_%u", resource->handle.id);
    }
    
    rg->resource_count++;
    
    LOG_DEBUG("Imported buffer resource '%s'", resource->name);
    
    return resource->handle;
}

// ============================================================================
// Pass Declaration
// ============================================================================

RGPassHandle rg_add_pass(RenderGraph* rg, const RGPassDesc* desc) {
    if (!rg || !desc || rg->pass_count >= MAX_PASSES) {
        return RG_INVALID_PASS;
    }
    
    RGPass* pass = &rg->passes[rg->pass_count];
    memset(pass, 0, sizeof(RGPass));
    
    pass->handle.id = ++rg->next_pass_id;
    pass->execute = desc->execute;
    pass->user_data = desc->user_data;
    pass->queue_type = desc->queue_type;
    pass->priority = desc->priority;
    
    if (desc->name) {
        strncpy(pass->name, desc->name, sizeof(pass->name) - 1);
    } else {
        snprintf(pass->name, sizeof(pass->name), "pass_%u", pass->handle.id);
    }
    
    rg->pass_count++;
    
    LOG_DEBUG("Added pass '%s' (queue: %d, priority: %u)", 
              pass->name, pass->queue_type, pass->priority);
    
    return pass->handle;
}

void rg_pass_read(RenderGraph* rg, RGPassHandle pass, RGResourceHandle resource) {
    if (!rg) return;
    
    RGPass* pass_ptr = find_pass(rg, pass);
    RGResource* resource_ptr = find_resource(rg, resource);
    
    if (!pass_ptr || !resource_ptr) {
        LOG_ERROR("Invalid pass or resource handle");
        return;
    }
    
    if (!add_pass_dependency(pass_ptr, resource, false)) {
        LOG_ERROR("Too many read dependencies for pass '%s'", pass_ptr->name);
        return;
    }
    
    // Update resource lifetime
    uint32_t pass_index = pass_ptr - rg->passes;
    resource_ptr->first_use_pass = (resource_ptr->first_use_pass < pass_index) ? 
                                  resource_ptr->first_use_pass : pass_index;
    resource_ptr->last_use_pass = (resource_ptr->last_use_pass > pass_index) ? 
                                 resource_ptr->last_use_pass : pass_index;
}

void rg_pass_write(RenderGraph* rg, RGPassHandle pass, RGResourceHandle resource) {
    if (!rg) return;
    
    RGPass* pass_ptr = find_pass(rg, pass);
    RGResource* resource_ptr = find_resource(rg, resource);
    
    if (!pass_ptr || !resource_ptr) {
        LOG_ERROR("Invalid pass or resource handle");
        return;
    }
    
    if (!add_pass_dependency(pass_ptr, resource, true)) {
        LOG_ERROR("Too many write dependencies for pass '%s'", pass_ptr->name);
        return;
    }
    
    // Update resource lifetime
    uint32_t pass_index = pass_ptr - rg->passes;
    resource_ptr->first_use_pass = (resource_ptr->first_use_pass < pass_index) ? 
                                  resource_ptr->first_use_pass : pass_index;
    resource_ptr->last_use_pass = (resource_ptr->last_use_pass > pass_index) ? 
                                 resource_ptr->last_use_pass : pass_index;
}

// ============================================================================
// Graph Compilation
// ============================================================================

static bool detect_cycles(RenderGraph* rg) {
    // Simple cycle detection using DFS
    // This is a simplified implementation
    for (uint32_t i = 0; i < rg->pass_count; i++) {
        // TODO: Implement proper cycle detection
        // For now, just check for obvious self-dependencies
        RGPass* pass = &rg->passes[i];
        for (uint32_t j = 0; j < pass->write_count; j++) {
            for (uint32_t k = 0; k < pass->read_count; k++) {
                if (pass->writes[j].id == pass->reads[k].id) {
                    snprintf(rg->error_message, sizeof(rg->error_message),
                            "Pass '%s' has read-write conflict on resource", pass->name);
                    return false;
                }
            }
        }
    }
    return true;
}

static bool cull_unused_passes(RenderGraph* rg) {
    uint32_t culled = 0;
    
    for (uint32_t i = 0; i < rg->pass_count; i++) {
        RGPass* pass = &rg->passes[i];
        
        // Check if pass has any side effects (writes to imported resources)
        bool has_side_effects = false;
        for (uint32_t j = 0; j < pass->write_count; j++) {
            RGResource* resource = find_resource(rg, pass->writes[j]);
            if (resource && resource->is_imported) {
                has_side_effects = true;
                break;
            }
        }
        
        // Cull pass if it has no side effects and no reads
        if (!has_side_effects && pass->read_count == 0) {
            pass->is_culled = true;
            culled++;
        }
    }
    
    rg->stats.culled_passes = culled;
    LOG_DEBUG("Culled %u unused passes", culled);
    return true;
}

static bool allocate_physical_resources(RenderGraph* rg) {
    for (uint32_t i = 0; i < rg->resource_count; i++) {
        RGResource* resource = &rg->resources[i];
        
        if (resource->is_imported || resource->is_culled) {
            continue;
        }
        
        if (resource->type == RG_RESOURCE_TEXTURE) {
            RGTextureDesc* desc = &resource->desc.texture_desc;
            resource->physical.texture_id = texture_create(desc->width, desc->height, 
                                                           desc->format, desc->usage);
            if (!texture_is_valid(resource->physical.texture_id)) {
                snprintf(rg->error_message, sizeof(rg->error_message),
                        "Failed to create texture '%s'", resource->name);
                return false;
            }
        } else if (resource->type == RG_RESOURCE_BUFFER) {
            RGBufferDesc* desc = &resource->desc.buffer_desc;
            resource->physical.buffer_id = buffer_create(desc->size, desc->usage);
            if (!buffer_is_valid(resource->physical.buffer_id)) {
                snprintf(rg->error_message, sizeof(rg->error_message),
                        "Failed to create buffer '%s'", resource->name);
                return false;
            }
        }
    }
    
    return true;
}

static bool calculate_execution_order(RenderGraph* rg) {
    // Simple topological sort
    // This is a basic implementation - a production system would use a more sophisticated algorithm
    
    uint32_t execution_count = 0;
    bool* processed = calloc(rg->pass_count, sizeof(bool));
    
    while (execution_count < rg->pass_count) {
        bool progress = false;
        
        for (uint32_t i = 0; i < rg->pass_count; i++) {
            if (processed[i] || rg->passes[i].is_culled) continue;
            
            RGPass* pass = &rg->passes[i];
            
            // Check if all dependencies are processed
            bool can_execute = true;
            for (uint32_t j = 0; j < pass->read_count; j++) {
                RGResource* resource = find_resource(rg, pass->reads[j]);
                if (resource) {
                    // Find the pass that writes to this resource
                    for (uint32_t k = 0; k < rg->pass_count; k++) {
                        if (k == i || rg->passes[k].is_culled) continue;
                        
                        for (uint32_t l = 0; l < rg->passes[k].write_count; l++) {
                            if (rg->passes[k].writes[l].id == pass->reads[j].id) {
                                if (!processed[k]) {
                                    can_execute = false;
                                    break;
                                }
                            }
                        }
                        if (!can_execute) break;
                    }
                }
                if (!can_execute) break;
            }
            
            if (can_execute) {
                pass->execution_order = execution_count++;
                processed[i] = true;
                progress = true;
            }
        }
        
        if (!progress) {
            snprintf(rg->error_message, sizeof(rg->error_message),
                    "Circular dependency detected in render graph");
            free(processed);
            return false;
        }
    }
    
    free(processed);
    return true;
}

bool rg_compile(RenderGraph* rg) {
    if (!rg) {
        LOG_ERROR("Invalid render graph");
        return false;
    }
    
    if (rg->is_compiled) {
        LOG_WARNING("Graph already compiled");
        return true;
    }
    
    rg->has_errors = false;
    rg->error_message[0] = '\0';
    
    // Reset statistics
    memset(&rg->stats, 0, sizeof(RGStats));
    rg->stats.total_passes = rg->pass_count;
    rg->stats.total_resources = rg->resource_count;
    
    // Validate graph
    if (!detect_cycles(rg)) {
        rg->has_errors = true;
        LOG_ERROR("Graph validation failed: %s", rg->error_message);
        return false;
    }
    
    // Cull unused passes
    if (!cull_unused_passes(rg)) {
        rg->has_errors = true;
        return false;
    }
    
    // Calculate execution order
    if (!calculate_execution_order(rg)) {
        rg->has_errors = true;
        LOG_ERROR("Failed to calculate execution order: %s", rg->error_message);
        return false;
    }
    
    // Allocate physical resources
    if (!allocate_physical_resources(rg)) {
        rg->has_errors = true;
        LOG_ERROR("Failed to allocate physical resources: %s", rg->error_message);
        return false;
    }
    
    // Update statistics
    rg->stats.executed_passes = rg->stats.total_passes - rg->stats.culled_passes;
    
    for (uint32_t i = 0; i < rg->resource_count; i++) {
        if (rg->resources[i].is_imported) {
            rg->stats.imported_resources++;
        } else {
            rg->stats.transient_resources++;
        }
    }
    
    rg->is_compiled = true;
    LOG_INFO("Render graph compiled successfully: %u/%u passes, %u resources", 
             rg->stats.executed_passes, rg->stats.total_passes, rg->stats.total_resources);
    
    return true;
}

// ============================================================================
// Graph Execution
// ============================================================================

void rg_execute(RenderGraph* rg, CommandBuffer* cmd) {
    if (!rg || !cmd) {
        LOG_ERROR("Invalid render graph or command buffer");
        return;
    }
    
    if (!rg->is_compiled) {
        LOG_ERROR("Graph not compiled");
        return;
    }
    
    if (rg->has_errors) {
        LOG_ERROR("Graph has compilation errors");
        return;
    }
    
    // Validate execution state
    if (rg->validation_enabled && !rg_validate_execution_state(rg)) {
        rg_log_validation_error(rg, "Execution state validation failed");
        return;
    }
    
    uint64_t execution_start = rg_get_timestamp();
    rg->execution_count++;
    
    // Create pass context
    RGPassContext ctx = {
        .graph = rg,
        .cmd = cmd,
        .pass_index = 0
    };
    
    // Execute passes in order
    for (uint32_t i = 0; i < rg->pass_count; i++) {
        RGPass* pass = &rg->passes[i];
        
        if (pass->is_culled) {
            continue;
        }
        
        ctx.pass_index = i;
        uint64_t pass_start = rg_get_timestamp();
        
        // Validate pass before execution
        if (rg->validation_enabled) {
            if (!pass->execute) {
                rg_log_validation_error(rg, "Pass has no execute function");
                continue;
            }
            
            // Validate pass dependencies
            for (uint32_t j = 0; j < pass->read_count; j++) {
                RGResource* resource = find_resource(rg, pass->reads[j]);
                if (!resource) {
                    rg_log_validation_error(rg, "Pass references invalid read resource");
                    continue;
                }
            }
            
            for (uint32_t j = 0; j < pass->write_count; j++) {
                RGResource* resource = find_resource(rg, pass->writes[j]);
                if (!resource) {
                    rg_log_validation_error(rg, "Pass references invalid write resource");
                    continue;
                }
            }
        }
        
        // Begin profiling if enabled
        if (rg->profiling_enabled) {
            // TODO: Begin timestamp query
        }
        
        // Execute pass
        pass->execute(&ctx, pass->user_data);
        
        // End profiling if enabled
        if (rg->profiling_enabled) {
            // TODO: End timestamp query
        }
        
        uint64_t pass_end = rg_get_timestamp();
        uint64_t pass_time = pass_end - pass_start;
        
        // Update execution time statistics
        if (rg->validation_enabled) {
            if (pass_time > rg->max_pass_execution_time) {
                rg->max_pass_execution_time = pass_time;
            }
            if (pass_time < rg->min_pass_execution_time) {
                rg->min_pass_execution_time = pass_time;
            }
        }
        
        LOG_DEBUG("Executed pass '%s' (order: %u, time: %.2f ms)", 
                 pass->name, pass->execution_order, pass_time / 1000000.0);
    }
    
    uint64_t execution_end = rg_get_timestamp();
    uint64_t total_time = execution_end - execution_start;
    rg->total_execution_time += total_time;
    
    // Validate final state
    if (rg->validation_enabled) {
        if (!rg_validate_resource_lifetimes(rg)) {
            rg_log_validation_error(rg, "Resource lifetime validation failed after execution");
        }
    }
    
    LOG_DEBUG("Frame graph execution completed in %.2f ms", total_time / 1000000.0);
}

// ============================================================================
// Pass Context API
// ============================================================================

TextureID rg_ctx_get_texture(RGPassContext* ctx, RGResourceHandle handle) {
    if (!ctx || handle.id == 0xFFFFFFFF) {
        return TEXTURE_INVALID;
    }
    
    RGResource* resource = find_resource(ctx->graph, handle);
    if (!resource || resource->type != RG_RESOURCE_TEXTURE) {
        LOG_ERROR("Invalid texture resource handle");
        return TEXTURE_INVALID;
    }
    
    return resource->physical.texture_id;
}

BufferID rg_ctx_get_buffer(RGPassContext* ctx, RGResourceHandle handle) {
    if (!ctx || handle.id == 0xFFFFFFFF) {
        return BUFFER_INVALID;
    }
    
    RGResource* resource = find_resource(ctx->graph, handle);
    if (!resource || resource->type != RG_RESOURCE_BUFFER) {
        LOG_ERROR("Invalid buffer resource handle");
        return BUFFER_INVALID;
    }
    
    return resource->physical.buffer_id;
}

// ============================================================================
// Debugging and Statistics
// ============================================================================

void rg_enable_profiling(RenderGraph* rg, bool enable) {
    if (rg) {
        rg->profiling_enabled = enable;
        LOG_INFO("Render graph profiling %s", enable ? "enabled" : "disabled");
    }
}

void rg_get_stats(RenderGraph* rg, RGStats* out) {
    if (rg && out) {
        *out = rg->stats;
    }
}

bool rg_validate_graph(RenderGraph* rg, char* error_buffer, uint32_t buffer_size) {
    if (!rg) {
        if (error_buffer && buffer_size > 0) {
            strncpy(error_buffer, "Invalid render graph", buffer_size - 1);
            error_buffer[buffer_size - 1] = '\0';
        }
        return false;
    }
    
    // Run validation
    bool is_valid = detect_cycles(rg);
    
    if (!is_valid && error_buffer && buffer_size > 0) {
        strncpy(error_buffer, rg->error_message, buffer_size - 1);
        error_buffer[buffer_size - 1] = '\0';
    }
    
    return is_valid;
}

void rg_export_dot(RenderGraph* rg, const char* filepath) {
    if (!rg || !filepath) return;
    
    FILE* file = fopen(filepath, "w");
    if (!file) {
        LOG_ERROR("Failed to open DOT file '%s'", filepath);
        return;
    }
    
    fprintf(file, "digraph RenderGraph {\n");
    fprintf(file, "  rankdir=LR;\n");
    fprintf(file, "  node [shape=box];\n\n");
    
    // Write passes
    for (uint32_t i = 0; i < rg->pass_count; i++) {
        RGPass* pass = &rg->passes[i];
        const char* color = pass->is_culled ? "red" : "lightblue";
        fprintf(file, "  \"pass_%u\" [label=\"%s\", fillcolor=%s, style=filled];\n",
                pass->handle.id, pass->name, color);
    }
    
    // Write resources and dependencies
    for (uint32_t i = 0; i < rg->resource_count; i++) {
        RGResource* resource = &rg->resources[i];
        fprintf(file, "  \"resource_%u\" [label=\"%s\", shape=ellipse, fillcolor=lightgreen, style=filled];\n",
                resource->handle.id, resource->name);
        
        // Find passes that read this resource
        for (uint32_t j = 0; j < rg->pass_count; j++) {
            RGPass* pass = &rg->passes[j];
            for (uint32_t k = 0; k < pass->read_count; k++) {
                if (pass->reads[k].id == resource->handle.id) {
                    fprintf(file, "  \"resource_%u\" -> \"pass_%u\" [label=\"read\"];\n",
                            resource->handle.id, pass->handle.id);
                }
            }
            
            for (uint32_t k = 0; k < pass->write_count; k++) {
                if (pass->writes[k].id == resource->handle.id) {
                    fprintf(file, "  \"pass_%u\" -> \"resource_%u\" [label=\"write\"];\n",
                            pass->handle.id, resource->handle.id);
                }
            }
        }
    }
    
    fprintf(file, "}\n");
    fclose(file);
    
    LOG_INFO("Exported render graph to '%s'", filepath);
}

// ============================================================================
// Missing Utility Functions Implementation
// ============================================================================

static RGResource* find_resource(RenderGraph* rg, RGResourceHandle handle) {
    if (!rg || handle.id == 0xFFFFFFFF) return NULL;
    
    for (uint32_t i = 0; i < rg->resource_count; i++) {
        if (rg->resources[i].handle.id == handle.id) {
            return &rg->resources[i];
        }
    }
    return NULL;
}

static RGPass* find_pass(RenderGraph* rg, RGPassHandle handle) {
    if (!rg || handle.id == 0xFFFFFFFF) return NULL;
    
    for (uint32_t i = 0; i < rg->pass_count; i++) {
        if (rg->passes[i].handle.id == handle.id) {
            return &rg->passes[i];
        }
    }
    return NULL;
}

static bool add_pass_dependency(RGPass* pass, RGResourceHandle resource, bool is_write) {
    if (!pass) return false;
    
    if (is_write) {
        if (pass->write_count >= MAX_DEPENDENCIES_PER_PASS) {
            return false;
        }
        pass->writes[pass->write_count++] = resource;
    } else {
        if (pass->read_count >= MAX_DEPENDENCIES_PER_PASS) {
            return false;
        }
        pass->reads[pass->read_count++] = resource;
    }
    
    return true;
}

// ============================================================================
// Frame Graph Validation Implementation
// ============================================================================

static bool rg_validate_execution_state(RenderGraph* rg) {
    if (!rg) return false;
    
    // Validate compilation state
    if (!rg->is_compiled) {
        return false;
    }
    
    // Validate error state
    if (rg->has_errors) {
        return false;
    }
    
    // Validate pass dependencies
    if (!rg_validate_pass_dependencies(rg)) {
        return false;
    }
    
    // Validate resource lifetimes
    if (!rg_validate_resource_lifetimes(rg)) {
        return false;
    }
    
    return true;
}

static void rg_log_validation_error(RenderGraph* rg, const char* error) {
    if (!rg || !error) return;
    
    rg->validation_errors++;
    LOG_ERROR("Frame Graph Validation Error [%lu]: %s", 
             rg->validation_errors, error);
}

static uint64_t rg_get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static bool rg_validate_pass_dependencies(RenderGraph* rg) {
    if (!rg) return false;
    
    for (uint32_t i = 0; i < rg->pass_count; i++) {
        RGPass* pass = &rg->passes[i];
        
        if (pass->is_culled) continue;
        
        // Check for read-write conflicts
        for (uint32_t j = 0; j < pass->read_count; j++) {
            for (uint32_t k = 0; k < pass->write_count; k++) {
                if (pass->reads[j].id == pass->writes[k].id) {
                    return false; // Read-write conflict in same pass
                }
            }
        }
        
        // Validate resource handles
        for (uint32_t j = 0; j < pass->read_count; j++) {
            if (!find_resource(rg, pass->reads[j])) {
                return false; // Invalid read resource
            }
        }
        
        for (uint32_t j = 0; j < pass->write_count; j++) {
            if (!find_resource(rg, pass->writes[j])) {
                return false; // Invalid write resource
            }
        }
    }
    
    return true;
}

static bool rg_validate_resource_lifetimes(RenderGraph* rg) {
    if (!rg) return false;
    
    for (uint32_t i = 0; i < rg->resource_count; i++) {
        RGResource* resource = &rg->resources[i];
        
        if (resource->is_culled) continue;
        
        // Check if resource has valid lifetime
        if (resource->first_use_pass == UINT32_MAX && resource->last_use_pass == 0) {
            // Resource never used - this might be OK for imported resources
            if (!resource->is_imported) {
                return false; // Transient resource never used
            }
        }
        
        // Validate lifetime consistency
        if (resource->first_use_pass != UINT32_MAX && resource->last_use_pass != 0) {
            if (resource->first_use_pass > resource->last_use_pass) {
                return false; // Invalid lifetime range
            }
        }
    }
    
    return true;
}

// ============================================================================
// Public Validation API
// ============================================================================

void rg_enable_validation(RenderGraph* rg, bool enabled) {
    if (!rg) return;
    
    rg->validation_enabled = enabled;
    rg->last_validation_time = rg_get_timestamp();
    LOG_INFO("Frame graph validation %s", enabled ? "enabled" : "disabled");
}

uint64_t rg_get_validation_errors(RenderGraph* rg) {
    return rg ? rg->validation_errors : 0;
}

void rg_get_execution_statistics(RenderGraph* rg, uint64_t* execution_count,
                                 uint64_t* total_execution_time, uint64_t* max_pass_time,
                                 uint64_t* min_pass_time, uint64_t* validation_errors) {
    if (!rg) return;
    
    if (execution_count) *execution_count = rg->execution_count;
    if (total_execution_time) *total_execution_time = rg->total_execution_time;
    if (max_pass_time) *max_pass_time = rg->max_pass_execution_time;
    if (min_pass_time) *min_pass_time = rg->min_pass_execution_time;
    if (validation_errors) *validation_errors = rg->validation_errors;
}

void rg_reset_execution_statistics(RenderGraph* rg) {
    if (!rg) return;
    
    rg->execution_count = 0;
    rg->total_execution_time = 0;
    rg->max_pass_execution_time = 0;
    rg->min_pass_execution_time = UINT64_MAX;
    rg->validation_errors = 0;
    rg->last_validation_time = rg_get_timestamp();
    
    LOG_INFO("Frame graph execution statistics reset");
}
