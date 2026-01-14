// src/engine/rendering/core/render_graph.c
// Render Graph System - Manages render passes and resource dependencies

#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../framebuffer.h"
#include "../render_pipeline.h"

// ============================================================================
// Render Graph Types
// ============================================================================

typedef enum {
    RESOURCE_TYPE_TEXTURE,
    RESOURCE_TYPE_BUFFER,
    RESOURCE_TYPE_FRAMEBUFFER
} ResourceType;

typedef enum {
    RESOURCE_USAGE_READ,
    RESOURCE_USAGE_WRITE,
    RESOURCE_USAGE_READ_WRITE
} ResourceUsage;

typedef struct {
    char name[64];
    ResourceType type;
    void *resource;
    ResourceUsage usage;
    uint32_t width, height, depth;
    uint32_t format;
    bool is_transient;
} RenderGraphResource;

typedef struct RenderPass RenderPass;
struct RenderPass {
    char name[64];
    void (*execute)(RenderPass *pass);
    RenderGraphResource *resources[16];
    uint32_t resource_count;
    RenderPass *dependencies[8];
    uint32_t dependency_count;
    uint32_t ref_count;
    bool enabled;
};

typedef struct {
    RenderPass *passes[64];
    uint32_t pass_count;
    RenderGraphResource resources[128];
    uint32_t resource_count;
    bool compiled;
    bool executing;
} RenderGraph;

// ============================================================================
// Render Graph API
// ============================================================================

RenderGraph *render_graph_create(void) {
    RenderGraph *graph = calloc(1, sizeof(RenderGraph));
    if (!graph) {
        LOG_ERROR("Failed to allocate render graph");
        return NULL;
    }
    
    LOG_INFO("Render graph created");
    return graph;
}

void render_graph_destroy(RenderGraph *graph) {
    if (!graph)
        return;
    
    // Clean up passes
    for (uint32_t i = 0; i < graph->pass_count; i++) {
        if (graph->passes[i]) {
            free(graph->passes[i]);
        }
    }
    
    // Clean up resources
    for (uint32_t i = 0; i < graph->resource_count; i++) {
        if (graph->resources[i].resource && !graph->resources[i].is_transient) {
            // Don't destroy external resources, just transient ones
            if (graph->resources[i].type == RESOURCE_TYPE_FRAMEBUFFER) {
                framebuffer_destroy(graph->resources[i].resource);
            }
        }
    }
    
    free(graph);
    LOG_INFO("Render graph destroyed");
}

RenderPass *render_graph_add_pass(RenderGraph *graph, const char *name, 
                                  void (*execute)(RenderPass *pass)) {
    if (!graph || !name || !execute || graph->pass_count >= 64) {
        LOG_ERROR("Invalid parameters or too many passes");
        return NULL;
    }
    
    RenderPass *pass = calloc(1, sizeof(RenderPass));
    if (!pass) {
        LOG_ERROR("Failed to allocate render pass");
        return NULL;
    }
    
    strncpy(pass->name, name, sizeof(pass->name) - 1);
    pass->execute = execute;
    pass->enabled = true;
    
    graph->passes[graph->pass_count++] = pass;
    
    LOG_DEBUG("Added render pass: %s", name);
    return pass;
}

RenderGraphResource *render_graph_add_resource(RenderGraph *graph, const char *name) {
    if (!graph || !name || graph->resource_count >= 128) {
        LOG_ERROR("Invalid parameters or too many resources");
        return NULL;
    }
    
    RenderGraphResource *resource = &graph->resources[graph->resource_count++];
    strncpy(resource->name, name, sizeof(resource->name) - 1);
    resource->type = RESOURCE_TYPE_TEXTURE;
    resource->usage = RESOURCE_USAGE_READ;
    resource->is_transient = true;
    
    LOG_DEBUG("Added render graph resource: %s", name);
    return resource;
}

bool render_graph_compile(RenderGraph *graph) {
    if (!graph) {
        LOG_ERROR("Invalid render graph");
        return false;
    }
    
    // Reset dependency counts
    for (uint32_t i = 0; i < graph->pass_count; i++) {
        graph->passes[i]->dependency_count = 0;
        graph->passes[i]->ref_count = 0;
    }
    
    // Build dependency graph based on resource usage
    for (uint32_t i = 0; i < graph->pass_count; i++) {
        RenderPass *pass = graph->passes[i];
        
        for (uint32_t j = 0; j < pass->resource_count; j++) {
            RenderGraphResource *resource = pass->resources[j];
            
            // Find other passes that use this resource
            for (uint32_t k = 0; k < graph->pass_count; k++) {
                if (k == i) continue;
                
                RenderPass *other_pass = graph->passes[k];
                for (uint32_t l = 0; l < other_pass->resource_count; l++) {
                    if (other_pass->resources[l] == resource) {
                        // Add dependency if other pass writes and we read
                        if ((other_pass->resources[l]->usage == RESOURCE_USAGE_WRITE ||
                             other_pass->resources[l]->usage == RESOURCE_USAGE_READ_WRITE) &&
                            (resource->usage == RESOURCE_USAGE_READ ||
                             resource->usage == RESOURCE_USAGE_READ_WRITE)) {
                            
                            if (pass->dependency_count < 8) {
                                pass->dependencies[pass->dependency_count++] = other_pass;
                                other_pass->ref_count++;
                            }
                        }
                    }
                }
            }
        }
    }
    
    graph->compiled = true;
    LOG_INFO("Render graph compiled with %u passes", graph->pass_count);
    return true;
}

void render_graph_execute(RenderGraph *graph) {
    if (!graph || !graph->compiled) {
        LOG_ERROR("Render graph not compiled or invalid");
        return;
    }
    
    if (graph->executing) {
        LOG_WARN("Render graph already executing");
        return;
    }
    
    graph->executing = true;
    
    // Execute passes in dependency order
    uint32_t executed_count = 0;
    while (executed_count < graph->pass_count) {
        for (uint32_t i = 0; i < graph->pass_count; i++) {
            RenderPass *pass = graph->passes[i];
            
            if (!pass->enabled || pass->ref_count > 0)
                continue;
            
            // Execute pass
            if (pass->execute) {
                LOG_DEBUG("Executing pass: %s", pass->name);
                pass->execute(pass);
            }
            
            // Decrease ref count of dependent passes
            for (uint32_t j = 0; j < graph->pass_count; j++) {
                RenderPass *other_pass = graph->passes[j];
                for (uint32_t k = 0; k < other_pass->dependency_count; k++) {
                    if (other_pass->dependencies[k] == pass) {
                        other_pass->ref_count--;
                    }
                }
            }
            
            executed_count++;
        }
    }
    
    graph->executing = false;
    LOG_DEBUG("Render graph execution completed");
}

bool render_graph_validate(RenderGraph *graph) {
    if (!graph) {
        LOG_ERROR("Invalid render graph");
        return false;
    }
    
    // Check for circular dependencies
    for (uint32_t i = 0; i < graph->pass_count; i++) {
        RenderPass *pass = graph->passes[i];
        if (pass->ref_count > 0) {
            LOG_ERROR("Circular dependency detected involving pass: %s", pass->name);
            return false;
        }
    }
    
    // Check resource usage conflicts
    for (uint32_t i = 0; i < graph->resource_count; i++) {
        RenderGraphResource *resource = &graph->resources[i];
        if (!resource->resource && !resource->is_transient) {
            LOG_ERROR("Resource %s has no backing resource", resource->name);
            return false;
        }
    }
    
    LOG_INFO("Render graph validation passed");
    return true;
}

void render_graph_enable_pass(RenderGraph *graph, const char *name, bool enabled) {
    if (!graph || !name)
        return;
    
    for (uint32_t i = 0; i < graph->pass_count; i++) {
        if (strcmp(graph->passes[i]->name, name) == 0) {
            graph->passes[i]->enabled = enabled;
            LOG_DEBUG("Pass %s %s", name, enabled ? "enabled" : "disabled");
            return;
        }
    }
    
    LOG_WARN("Pass not found: %s", name);
}

void render_graph_visualize(RenderGraph *graph) {
    if (!graph)
        return;
    
    LOG_INFO("=== Render Graph Visualization ===");
    LOG_INFO("Passes: %u, Resources: %u", graph->pass_count, graph->resource_count);
    
    for (uint32_t i = 0; i < graph->pass_count; i++) {
        RenderPass *pass = graph->passes[i];
        LOG_INFO("Pass: %s (%s)", pass->name, pass->enabled ? "enabled" : "disabled");
        
        for (uint32_t j = 0; j < pass->dependency_count; j++) {
            LOG_INFO("  Depends on: %s", pass->dependencies[j]->name);
        }
    }
    
    LOG_INFO("=== End Visualization ===");
}
