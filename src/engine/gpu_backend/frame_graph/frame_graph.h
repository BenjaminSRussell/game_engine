// Frame Graph System - Public API
// Modern render graph with automatic resource management and dependency tracking
// 
// Usage example:
//   RenderGraph *rg = rg_create();
//   
//   RGResourceHandle depth = rg_create_texture(rg, &depth_desc);
//   RGResourceHandle gbuffer = rg_create_texture(rg, &gbuffer_desc);
//   
//   RGPassHandle shadow_pass = rg_add_pass(rg, &shadow_desc);
//   rg_pass_write(rg, shadow_pass, depth);
//   
//   RGPassHandle gbuffer_pass = rg_add_pass(rg, &gbuffer_desc);
//   rg_pass_read(rg, gbuffer_pass, depth);
//   rg_pass_write(rg, gbuffer_pass, gbuffer);
//   
//   rg_compile(rg);
//   rg_execute(rg, cmd);
//   rg_destroy(rg);

#ifndef FRAME_GRAPH_H
#define FRAME_GRAPH_H

#include "common.h"
#include "core/types.h"
#include "renderer/core/texture.h"
#include "renderer/core/buffer.h"
#include "renderer/core/command_buffer.h"
#include "renderer/pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque type for the render graph
typedef struct RenderGraph RenderGraph;

// Resource and pass handles (opaque IDs)
typedef struct RGResourceHandle {
    u32 id;
} RGResourceHandle;

typedef struct RGPassHandle {
    u32 id;
} RGPassHandle;

// Invalid handle constants
#define RG_INVALID_RESOURCE ((RGResourceHandle){0xFFFFFFFF})
#define RG_INVALID_PASS ((RGPassHandle){0xFFFFFFFF})

// Resource types
typedef enum RGResourceType {
    RG_RESOURCE_TEXTURE,
    RG_RESOURCE_BUFFER
} RGResourceType;

// Texture descriptor for render graph resources
typedef struct RGTextureDesc {
    u32 width;
    u32 height;
    u32 depth;           // For 3D textures, 1 for 2D
    TextureFormat format;
    TextureUsageFlags usage;
    const char *name;    // Debug name
} RGTextureDesc;

// Buffer descriptor for render graph resources
typedef struct RGBufferDesc {
    u32 size;
    BufferUsageFlags usage;
    const char *name;    // Debug name
} RGBufferDesc;

// Pass execution context (passed to execute callback)
typedef struct RGPassContext {
    RenderGraph *graph;
    CommandBuffer *cmd;
    u32 pass_index;
} RGPassContext;

// Pass execution callback
typedef void (*RGPassExecuteFunc)(RGPassContext *ctx, void *user_data);

// Pass descriptor
typedef struct RGPassDesc {
    const char *name;
    RGPassExecuteFunc execute;
    void *user_data;
} RGPassDesc;

// === Graph Lifecycle ===

// Create a new render graph for this frame
RenderGraph *rg_create(void);

// Destroy the render graph and free all resources
void rg_destroy(RenderGraph *rg);

// Reset the graph for reuse next frame (cheaper than destroy + create)
void rg_reset(RenderGraph *rg);

// === Resource Declaration ===

// Create a transient texture resource
// Lifetime is calculated automatically, memory may be aliased
RGResourceHandle rg_create_texture(RenderGraph *rg, const RGTextureDesc *desc);

// Create a transient buffer resource
RGResourceHandle rg_create_buffer(RenderGraph *rg, const RGBufferDesc *desc);

// Import an external texture (e.g., swapchain image)
// External resources are not aliased or destroyed
RGResourceHandle rg_import_texture(RenderGraph *rg, TextureID external, const char *name);

// Import an external buffer
RGResourceHandle rg_import_buffer(RenderGraph *rg, BufferID external, const char *name);

// === Pass Declaration ===

// Add a render pass to the graph
RGPassHandle rg_add_pass(RenderGraph *rg, const RGPassDesc *desc);

// Declare that a pass reads from a resource
void rg_pass_read(RenderGraph *rg, RGPassHandle pass, RGResourceHandle resource);

// Declare that a pass writes to a resource
void rg_pass_write(RenderGraph *rg, RGPassHandle pass, RGResourceHandle resource);

// === Compilation & Execution ===

// Compile the graph: build DAG, cull dead passes, calculate lifetimes, generate barriers
// Returns true on success, false if graph has errors (cycles, missing resources, etc.)
bool rg_compile(RenderGraph *rg);

// Execute all passes in dependency order
void rg_execute(RenderGraph *rg, CommandBuffer *cmd);

// === Pass Context API (called from pass execute callbacks) ===

// Get the physical texture for a resource handle
TextureID rg_ctx_get_texture(RGPassContext *ctx, RGResourceHandle handle);

// Get the physical buffer for a resource handle
BufferID rg_ctx_get_buffer(RGPassContext *ctx, RGResourceHandle handle);

// === Debugging & Profiling ===

// Export graph to GraphViz DOT format for visualization
void rg_export_dot(RenderGraph *rg, const char *filepath);

// Enable/disable GPU profiling (timestamp queries)
void rg_enable_profiling(RenderGraph *rg, bool enable);

// Pass timing info
typedef struct RGPassTimings {
    const char *pass_name;
    f32 gpu_time_ms;
} RGPassTimings;

// Get GPU timings for all passes (after execution, N frames delayed)
void rg_get_pass_timings(RenderGraph *rg, RGPassTimings *out, u32 *count);

// === Statistics ===

typedef struct RGStats {
    u32 total_passes;
    u32 executed_passes;
    u32 culled_passes;
    u32 total_resources;
    u32 transient_resources;
    u32 imported_resources;
    u64 transient_memory_allocated;  // Bytes
    u64 transient_memory_aliased;    // Bytes saved via aliasing
} RGStats;

// Get graph statistics (call after compile)
void rg_get_stats(RenderGraph *rg, RGStats *out);

#ifdef __cplusplus
}
#endif

#endif // FRAME_GRAPH_H
