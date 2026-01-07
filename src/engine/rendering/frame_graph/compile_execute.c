#include "rendering/frame_graph/frame_graph.h"
#include "rendering/frame_graph/frame_graph_internal.h"
#include "core/logger.h"
#include <string.h>
#include <stdio.h>

// === Compilation ===

bool rg_compile(RenderGraph *rg) {
    if (!rg) return false;

    // Phase 1: Validate graph before compilation
    char error_buffer[512] = {0};
    if (!rg_validate_graph(rg, error_buffer, sizeof(error_buffer))) {
        LOG_ERROR("Render graph validation failed: %s", error_buffer);
        // We'll continue for now but mark it as invalid if critical, 
        // or just return false if it's strictly enforced.
        // return false; 
    }
    
    LOG_INFO("Compiling render graph: %u passes, %u resources", 
             rg->pass_count, rg->resource_count);
    
    // Step 1: Build dependency graph
    if (!rg_build_dependency_graph(rg)) {
        LOG_ERROR("Failed to build dependency graph");
        return false;
    }
    
    // Step 2: Topological sort (determine execution order)
    if (!rg_topological_sort(rg)) {
        LOG_ERROR("Failed to sort passes (cycle detected?)");
        return false;
    }

    // Step 2.5: Schedule queues (Phase 2 optimization)
    rg_schedule_queues(rg);
    
    // Step 3: Cull unused passes
    rg_cull_unused_passes(rg);
    
    // Step 4: Calculate resource lifetimes
    rg_calculate_resource_lifetimes(rg);
    
    // Step 5: Perform resource aliasing
    if (rg->resource_pool) {
        rg_pool_alias_resources(rg->resource_pool, rg);
    }
    
    // Step 6: Generate barriers
    rg_generate_barriers(rg);
    
    // Phase 1: Optimize barriers
    u32 before = rg->barrier_count;
    merge_adjacent_barriers(rg);
    rg->barrier_stats.barriers_merged = before - rg->barrier_count;
    rg->barrier_stats.barriers_generated = before;
    
    if (rg->barrier_stats.barriers_merged > 0) {
        LOG_DEBUG("Barrier merging optimized %u barriers down to %u", 
                 before, rg->barrier_count);
    }
    
    // Update stats
    rg->stats.total_passes = rg->pass_count;
    rg->stats.executed_passes = rg->execution_count;
    rg->stats.culled_passes = rg->pass_count - rg->execution_count;
    rg->stats.total_resources = rg->resource_count;
    
    u32 transient_count = 0;
    u32 imported_count = 0;
    for (u32 i = 1; i < rg->resource_count; i++) {
        if (rg->resources[i].is_imported) {
            imported_count++;
        } else {
            transient_count++;
        }
    }
    rg->stats.transient_resources = transient_count;
    rg->stats.imported_resources = imported_count;
    
    // Get aliasing stats
    if (rg->resource_pool) {
        rg_pool_get_stats(rg->resource_pool, 
                         &rg->stats.transient_memory_allocated,
                         &rg->stats.transient_memory_aliased);
    }
    
    rg->is_compiled = true;
    LOG_INFO("Render graph compiled: %u/%u passes will execute", 
             rg->execution_count, rg->pass_count);
    
    return true;
}

// === Execution ===

void rg_execute(RenderGraph *rg, CommandBuffer *cmd) {
    if (!rg || !cmd) return;
    
    if (!rg->is_compiled) {
        LOG_ERROR("Cannot execute uncompiled render graph");
        return;
    }
    
    LOG_DEBUG("Executing render graph: %u passes", rg->execution_count);
    
    // Execute passes in topologically sorted order
    for (u32 i = 0; i < rg->execution_count; i++) {
        u32 pass_idx = rg->execution_order[i];
        RGPass *pass = &rg->passes[pass_idx];
        
        if (pass->is_culled) {
            continue;
        }
        
        LOG_DEBUG("Executing pass: %s", pass->name);
        
        // NOTE: Barrier execution would go here when command buffer API supports it
        // For now, barriers are calculated but not inserted
        // Future: cmd_pipeline_barrier(ctx->cmd, &barrier);
        
        // Execute the pass callback
        if (pass->execute) {
            RGPassContext ctx = {
                .graph = rg,
                .cmd = cmd,
                .pass_index = pass_idx
            };
            
            pass->execute(&ctx, pass->user_data);
        }
        
        // NOTE: GPU profiling would insert timestamp queries here
        // Future: if (rg->profiling_enabled) cmd_write_timestamp(ctx->cmd, &pass->gpu_timestamp_end);
    }
    
    LOG_DEBUG("Render graph execution complete");
}

// === Pass Context API ===

TextureID rg_ctx_get_texture(RGPassContext *ctx, RGResourceHandle handle) {
    if (!ctx || !ctx->graph) {
        return (TextureID){0};
    }
    
    RGResource *res = rg_get_resource(ctx->graph, handle);
    if (!res || res->type != RG_RESOURCE_TEXTURE) {
        LOG_ERROR("Invalid texture handle in pass context");
        return (TextureID){0};
    }
    
    // For imported resources, return directly
    if (res->is_imported) {
        return res->physical.texture_id;
    }
    
    // For transient resources, allocate from pool if not already allocated
    if (res->physical.texture_id.id == 0) {
        if (ctx->graph->resource_pool) {
            res->physical.texture_id = rg_pool_allocate_texture(
                ctx->graph->resource_pool, &res->desc.texture);
        } else {
            LOG_ERROR("No resource pool available for transient allocation");
            return (TextureID){0};
        }
    }
    
    return res->physical.texture_id;
}

BufferID rg_ctx_get_buffer(RGPassContext *ctx, RGResourceHandle handle) {
    if (!ctx || !ctx->graph) {
        return (BufferID){0};
    }
    
    RGResource *res = rg_get_resource(ctx->graph, handle);
    if (!res || res->type != RG_RESOURCE_BUFFER) {
        LOG_ERROR("Invalid buffer handle in pass context");
        return (BufferID){0};
    }
    
    // For imported resources, return directly
    if (res->is_imported) {
        return res->physical.buffer_id;
    }
    
    // For transient resources, allocate from pool if not already allocated
    if (res->physical.buffer_id.id == 0) {
        if (ctx->graph->resource_pool) {
            res->physical.buffer_id = rg_pool_allocate_buffer(
                ctx->graph->resource_pool, &res->desc.buffer);
        } else {
            LOG_ERROR("No resource pool available for transient allocation");
            return (BufferID){0};
        }
    }
    
    return res->physical.buffer_id;
}

// === Debugging ===

void rg_export_dot(RenderGraph *rg, const char *filepath) {
    if (!rg || !filepath) return;
    
    FILE *f = fopen(filepath, "w");
    if (!f) {
        LOG_ERROR("Failed to open %s for writing", filepath);
        return;
    }
    
    fprintf(f, "digraph RenderGraph {\n");
    fprintf(f, "  rankdir=LR;\n");
    fprintf(f, "  node [shape=box];\n\n");
    
    // Passes
    for (u32 i = 0; i < rg->pass_count; i++) {
        RGPass *pass = &rg->passes[i];
        if (pass->is_culled) {
            fprintf(f, "  Pass_%u [label=\"%s (CULLED)\", style=dashed];\n", i, pass->name);
        } else {
            fprintf(f, "  Pass_%u [label=\"%s\"];\n", i, pass->name);
        }
    }
    
    fprintf(f, "\n");
    
    // Resources
    for (u32 i = 1; i < rg->resource_count; i++) {
        RGResource *res = &rg->resources[i];
        const char *shape = res->is_imported ? "ellipse" : "diamond";
        fprintf(f, "  Res_%u [label=\"%s\", shape=%s];\n", i, res->name, shape);
    }
    
    fprintf(f, "\n");
    
    // Dependencies (pass -> resource -> pass)
    for (u32 i = 0; i < rg->pass_count; i++) {
        RGPass *pass = &rg->passes[i];
        
        // Writes
        for (u32 j = 0; j < pass->write_count; j++) {
            fprintf(f, "  Pass_%u -> Res_%u [label=\"write\", color=red];\n", 
                    i, pass->writes[j].id);
        }
        
        // Reads
        for (u32 j = 0; j < pass->read_count; j++) {
            fprintf(f, "  Res_%u -> Pass_%u [label=\"read\", color=blue];\n", 
                    pass->reads[j].id, i);
        }
    }
    
    fprintf(f, "}\n");
    fclose(f);
    
    LOG_INFO("Exported render graph to %s", filepath);
}

// === Profiling ===

void rg_enable_profiling(RenderGraph *rg, bool enable) {
    if (!rg) return;
    rg->profiling_enabled = enable;
}

void rg_get_pass_timings(RenderGraph *rg, RGPassTimings *out, u32 *count) {
    if (!rg || !out || !count) return;
    
    u32 timing_count = 0;
    for (u32 i = 0; i < rg->execution_count && timing_count < *count; i++) {
        u32 pass_idx = rg->execution_order[i];
        RGPass *pass = &rg->passes[pass_idx];
        
        out[timing_count].pass_name = pass->name;
        out[timing_count].gpu_time_ms = pass->gpu_time_ms;
        timing_count++;
    }
    
    *count = timing_count;
}

// === Compilation steps (stubs for Week 2) ===

// === Compilation steps ===

// Temporary compilation context
typedef struct CompilationContext {
    // Adjacency matrix for pass dependencies (pass_i -> pass_j)
    // Bitset or list. Since MAX_PASSES is small (128), we can use a simple array of lists or a bitset.
    // Let's use a fixed max dependency count for simplicity.
    u32 adjacency[RG_MAX_PASSES][RG_MAX_PASSES]; 
    u32 adjacency_count[RG_MAX_PASSES];
    
    // In-degree for topological sort
    u32 in_degree[RG_MAX_PASSES];
    
    // Track last writer for each resource to build dependencies
    u32 resource_last_writer[RG_MAX_RESOURCES];
} CompilationContext;

static CompilationContext s_ctx; // Static for now, strictly single-threaded compilation

bool rg_build_dependency_graph(RenderGraph *rg) {
    memset(&s_ctx, 0, sizeof(CompilationContext));
    
    // Initialize resource writers to "none"
    for (u32 i = 0; i < RG_MAX_RESOURCES; i++) {
        s_ctx.resource_last_writer[i] = RG_INVALID_PASS.id;
    }
    
    // Iterate passes in declaration order to determine dependencies
    for (u32 i = 1; i < rg->pass_count; i++) {
        RGPass *pass = &rg->passes[i];
        
        // 1. Check reads -> dependency on last writer
        for (u32 r = 0; r < pass->read_count; r++) {
            RGResourceHandle res_handle = pass->reads[r];
            if (res_handle.id >= rg->resource_count) continue;
            
            u32 writer_idx = s_ctx.resource_last_writer[res_handle.id];
            
            // If there is a writer, and it's not a previous dependency already
            if (writer_idx != RG_INVALID_PASS.id && writer_idx != i) {
                // Add dependency: writer -> current pass
                bool exists = false;
                for (u32 k = 0; k < s_ctx.adjacency_count[writer_idx]; k++) {
                    if (s_ctx.adjacency[writer_idx][k] == i) {
                        exists = true;
                        break;
                    }
                }
                
                if (!exists) {
                    s_ctx.adjacency[writer_idx][s_ctx.adjacency_count[writer_idx]++] = i;
                    s_ctx.in_degree[i]++;
                    
                    // Update ref count for culling (consumer count)
                    // rg->passes[writer_idx].ref_count++; // Logic for culling is slightly different (side effects)
                }
            }
        }
        
        // 2. Register writes
        for (u32 w = 0; w < pass->write_count; w++) {
            RGResourceHandle res_handle = pass->writes[w];
            if (res_handle.id >= rg->resource_count) continue;
            
            // If this resource was written before, we essentially have a write-after-write dependency
            // implied by declaration order, but for the graph structure, we just update the last writer.
            // Future readers will depend on THIS pass.
            
            s_ctx.resource_last_writer[res_handle.id] = i;
        }
    }
    
    return true;
}

bool rg_topological_sort(RenderGraph *rg) {
    u32 queue[RG_MAX_PASSES];
    u32 head = 0;
    u32 tail = 0;
    
    // Initialize queue with passes having in-degree 0
    for (u32 i = 1; i < rg->pass_count; i++) {
        if (s_ctx.in_degree[i] == 0) {
            queue[tail++] = i;
        }
    }
    
    rg->execution_count = 0;
    
    // Process queue
    while (head != tail) {
        u32 u = queue[head++];
        
        // Add to execution order
        rg->execution_order[rg->execution_count++] = u;
        
        // Decrement in-degree of neighbors
        for (u32 k = 0; k < s_ctx.adjacency_count[u]; k++) {
            u32 v = s_ctx.adjacency[u][k];
            
            if (s_ctx.in_degree[v] > 0) {
                s_ctx.in_degree[v]--;
                if (s_ctx.in_degree[v] == 0) {
                    queue[tail++] = v;
                }
            }
        }
    }
    
    // If execution count < pass count - 1 (excluding unused 0), we have a cycle
    if (rg->execution_count < rg->pass_count - 1) {
        LOG_ERROR("Render graph cycle detected! Executed %u/%u passes", 
                  rg->execution_count, rg->pass_count);
        return false;
    }
    
    return true;
}

void rg_cull_unused_passes(RenderGraph *rg) {
    // Reverse traversal: mark required passes
    // A pass is required if:
    // 1. It writes to an imported resource (side effect)
    // 2. It writes to a resource read by another required pass
    // 3. It has external side effects (marked explicitly? For now assume all valid writes matter if read)

    bool required[RG_MAX_PASSES];
    memset(required, 0, sizeof(required));
    
    // Initial pass: mark passes writing to imported resources as required
    for (u32 i = 1; i < rg->pass_count; i++) {
        RGPass *pass = &rg->passes[i];
        
        // If pass has NO outputs, we assume it has side effects (e.g. debug draw, or swapchain present logic if not using resource)
        // Adjust this logic if strict culling is desired.
        // For safety/Week 1-2: if it writes to imported, it's root.
        
        bool writes_imported = false;
        for (u32 w = 0; w < pass->write_count; w++) {
            RGResource *res = rg_get_resource(rg, pass->writes[w]);
            if (res && res->is_imported) {
                writes_imported = true;
                break;
            }
        }
        
        if (writes_imported) {
            required[i] = true;
        } else if (pass->write_count == 0) {
            // Assume passes with no resource writes have external side effects
            required[i] = true;
        }
    }
    
    // Propagate requirements backwards based on execution order (which is topological)
    // We iterate reverse execution order
    for (int i = (int)rg->execution_count - 1; i >= 0; i--) {
        u32 pass_idx = rg->execution_order[i];
        
        if (required[pass_idx]) {
            // Mark producers of input resources as required
            RGPass *pass = &rg->passes[pass_idx];
            for (u32 r = 0; r < pass->read_count; r++) {
                RGResourceHandle res_handle = pass->reads[r];
                
                // Find who wrote this resource most recently relative to this pass?
                // In our DAG, we have edges. Any pass that has an edge TO this pass is a dependency.
                // We can use the adjacency info we built, but that's forward (producer -> consumer).
                // We need consumer -> producer.
                
                // Re-scan or store reverse adj?
                // Easier: Use the "resource_last_writer" logic again but we need it per-resource flow.
                // Actually, the simple DAG build assumed single timeline.
                
                // Let's use a simpler approach for culling since we have the full graph:
                // Iterate all other passes, if they write to this resource, and we read it...
                // Wait, 'resource_last_writer' context is gone/overwritten.
                
                // Let's rely on the fact that we can just check if any pass writes this resource.
                // Since this is a simple render graph, usually one writer per resource version.
                // If we assume SSA-like usage (new resource handle for new version), it's easy.
                // If we reuse handles (read-modify-write), it's harder.
                // Current API: one handle = one resource. Writes imply modification or initialization.
                
                // Simple implementation: Find strict writer.
                // Limitation: If multiple passes write to same handle (read-modify-write), this simple loop assumes ALL writers are needed?
                // Or just the "last" one before us?
                // FOR WEEK 2 MVP: Simplistic check - check all passes that write this resource.
                
                for (u32 p = 1; p < rg->pass_count; p++) {
                    // Optimization: check if p is topologically before pass_idx
                     RGPass *potential_producer = &rg->passes[p];
                     for (u32 w = 0; w < potential_producer->write_count; w++) {
                         if (potential_producer->writes[w].id == res_handle.id) {
                             required[p] = true;
                         }
                     }
                }
            }
        } else {
            // Pass is not required
            rg->passes[pass_idx].is_culled = true;
        }
    }

    // Update execution count based on culling
    // Re-pack execution order?
    // r->execution_order contains indices. We can just keep them but flag as culled.
    // The execute function checks is_culled.
    // But we might want to remove them from stats.
}

void rg_calculate_resource_lifetimes(RenderGraph *rg) {
    // Lifetimes are already tracked during rg_pass_read/write
    LOG_DEBUG("Calculate resource lifetimes (already tracked during declaration)");
}

// Helper functions for state inference
static RGResourceState infer_read_state(RGResource *res) {
    if (res->type == RG_RESOURCE_TEXTURE) {
        if (res->desc.texture.usage & TEXTURE_USAGE_SAMPLED) return RG_STATE_SHADER_READ;
        if (res->desc.texture.usage & TEXTURE_USAGE_DEPTH_STENCIL) return RG_STATE_DEPTH_STENCIL;
    }
    return RG_STATE_SHADER_READ;
}

static RGResourceState infer_write_state(RGResource *res) {
    if (res->type == RG_RESOURCE_TEXTURE) {
        if (res->desc.texture.usage & TEXTURE_USAGE_RENDER_TARGET) return RG_STATE_RENDER_TARGET;
        if (res->desc.texture.usage & TEXTURE_USAGE_DEPTH_STENCIL) return RG_STATE_DEPTH_STENCIL;
        if (res->desc.texture.usage & TEXTURE_USAGE_STORAGE) return RG_STATE_SHADER_WRITE;
    }
    return RG_STATE_SHADER_WRITE;
}

void rg_generate_barriers(RenderGraph *rg) {
    if (!rg) return;
    
    rg->barrier_count = 0;
    
    // Initialize all resources to undefined state
    for (u32 i = 1; i < rg->resource_count; i++) {
        rg->resources[i].current_state = RG_STATE_UNDEFINED;
    }
    
    // Iterate passes in execution order and track state transitions
    for (u32 i = 0; i < rg->execution_count; i++) {
        u32 pass_idx = rg->execution_order[i];
        RGPass *pass = &rg->passes[pass_idx];
        
        if (pass->is_culled) continue;
        
        // Process all resources (reads and writes) for this pass
        // We need to determine the final state for each resource after this pass.
        // This means checking if it's written, and if so, what state it needs to be in for writing.
        // If it's only read, what state it needs to be in for reading.

        // First, handle reads. These might require a transition *to* a read state.
        for (u32 r = 0; r < pass->read_count; r++) {
            RGResource *res = rg_get_resource(rg, pass->reads[r]);
            if (!res) continue;
            
            RGResourceState desired_state = infer_read_state(res);
            
            // If state doesn't match, insert barrier
            if (res->current_state != desired_state && 
                res->current_state != RG_STATE_UNDEFINED) {
                
                if (rg->barrier_count < RG_MAX_DEPENDENCIES) {
                    RGBarrier *barrier = &rg->barriers[rg->barrier_count++];
                    barrier->resource = pass->reads[r];
                    barrier->old_state = res->current_state;
                    barrier->new_state = desired_state;
                    
                    // Determine pipeline stages (simplified)
                    barrier->src_stage = PIPELINE_STAGE_ALL_GRAPHICS;
                    barrier->dst_stage = PIPELINE_STAGE_FRAGMENT_SHADER;
                }
            }
            
            // Update to read state (reads don't change state for next user)
            // Actually, after read we stay in that state
        }
        
        // Check all writes - definitely need transition to write state
        for (u32 w = 0; w < pass->write_count; w++) {
            RGResource *res = rg_get_resource(rg, pass->writes[w]);
            if (!res) continue;
            
            RGResourceState desired_state = infer_write_state(res);
            
            // If state doesn't match, insert barrier
            if (res->current_state != desired_state) {
                if (rg->barrier_count < RG_MAX_DEPENDENCIES) {
                    RGBarrier *barrier = &rg->barriers[rg->barrier_count++];
                    barrier->resource = pass->writes[w];
                    barrier->old_state = res->current_state;
                    barrier->new_state = desired_state;
                    
                    // Determine pipeline stages
                    if (desired_state == RG_STATE_RENDER_TARGET) {
                        barrier->src_stage = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
                        barrier->dst_stage = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT;
                    } else if (desired_state == RG_STATE_DEPTH_STENCIL) {
                        barrier->src_stage = PIPELINE_STAGE_EARLY_FRAGMENT_TESTS;
                        barrier->dst_stage = PIPELINE_STAGE_EARLY_FRAGMENT_TESTS;
                    } else {
                        barrier->src_stage = PIPELINE_STAGE_ALL_GRAPHICS;
                        barrier->dst_stage = PIPELINE_STAGE_ALL_GRAPHICS;
                    }
                }
            }
            
            // Update resource state
            res->current_state = desired_state;
        }
    }
    
    LOG_DEBUG("Generated %u barriers", rg->barrier_count);
}
// === Multi-Queue Scheduling ===

void rg_schedule_queues(RenderGraph *rg) {
    if (!rg || rg->execution_count == 0) return;
    
    rg->batch_count = 0;
    
    // Start first batch
    u32 current_start = 0;
    u32 current_pass_idx = rg->execution_order[0];
    RGQueueType current_queue = rg->pass_queues[current_pass_idx];
    
    for (u32 i = 1; i < rg->execution_count; i++) {
        u32 pass_idx = rg->execution_order[i];
        RGQueueType queue = rg->pass_queues[pass_idx];
        
        // Check if queue type changed
        if (queue != current_queue) {
            // End current batch
            if (rg->batch_count < RG_MAX_PASSES) {
                RGBatch *batch = &rg->batches[rg->batch_count++];
                batch->queue_type = current_queue;
                batch->start_pass_index = current_start;
                batch->pass_count = i - current_start;
            } else {
                LOG_ERROR("Exceeded maximum batch count");
                return;
            }
            
            // Start new batch
            current_start = i;
            current_queue = queue;
        }
    }
    
    // Finish last batch
    if (rg->batch_count < RG_MAX_PASSES) {
        RGBatch *batch = &rg->batches[rg->batch_count++];
        batch->queue_type = current_queue;
        batch->start_pass_index = current_start;
        batch->pass_count = rg->execution_count - current_start;
    }
    
    LOG_INFO("Scheduled %u batches for %u passes", 
             rg->batch_count, rg->execution_count);
             
    for (u32 i = 0; i < rg->batch_count; i++) {
        RGBatch *b = &rg->batches[i];
        LOG_DEBUG("  Batch %u: Queue %s, Passes %u-%u (Count: %u)",
                 i, 
                 b->queue_type == RG_QUEUE_COMPUTE_ASYNC ? "COMPUTE" : 
                 (b->queue_type == RG_QUEUE_GRAPHICS ? "GRAPHICS" : "TRANSFER"),
                 b->start_pass_index,
                 b->start_pass_index + b->pass_count - 1,
                 b->pass_count);
    }
}
