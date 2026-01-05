// Frame Graph - Internal Data Structures
// Private header for render graph implementation

#ifndef FRAME_GRAPH_INTERNAL_H
#define FRAME_GRAPH_INTERNAL_H

#include "frame_graph.h"
#include "core/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RG_MAX_RESOURCES 256
#define RG_MAX_PASSES 128
#define RG_MAX_DEPENDENCIES 512

// Resource state tracking
typedef enum RGResourceState {
    RG_STATE_UNDEFINED,
    RG_STATE_RENDER_TARGET,
    RG_STATE_DEPTH_STENCIL,
    RG_STATE_SHADER_READ,
    RG_STATE_SHADER_WRITE,
    RG_STATE_TRANSFER_SRC,
    RG_STATE_TRANSFER_DST,
    RG_STATE_PRESENT
} RGResourceState;

// Internal resource representation
typedef struct RGResource {
    u32 id;
    RGResourceType type;
    
    // Descriptor
    union {
        RGTextureDesc texture;
        RGBufferDesc buffer;
    } desc;
    
    // Lifetime tracking (pass indices)
    u32 first_use_pass;
    u32 last_use_pass;
    
    // Physical resource
    bool is_imported;
    union {
        TextureID texture_id;
        BufferID buffer_id;
    } physical;
    
    // Aliasing (for transients)
    u32 alias_pool_index;
    
    // Current state (for barrier generation)
    RGResourceState current_state;
    
    char name[64];
} RGResource;

// Internal pass representation
typedef struct RGPass {
    u32 id;
    char name[64];
    
    // Execution
    RGPassExecuteFunc execute;
    void *user_data;
    
    // Dependencies
    RGResourceHandle reads[32];
    u32 read_count;
    RGResourceHandle writes[32];
    u32 write_count;
    
    // Graph analysis
    bool is_culled;
    u32 ref_count;  // How many passes depend on this
    
    // Profiling
    u64 gpu_timestamp_start;
    u64 gpu_timestamp_end;
    f32 gpu_time_ms;
} RGPass;

// Barrier between passes
typedef struct RGBarrier {
    RGResourceHandle resource;
    RGResourceState old_state;
    RGResourceState new_state;
    PipelineStage src_stage;
    PipelineStage dst_stage;
} RGBarrier;

// Main render graph structure
struct RenderGraph {
    // Resources
    RGResource resources[RG_MAX_RESOURCES];
    u32 resource_count;
    
    // Passes
    RGPass passes[RG_MAX_PASSES];
    u32 pass_count;
    
    // Execution order (after compile)
    u32 execution_order[RG_MAX_PASSES];
    u32 execution_count;
    
    // Barriers (inserted between passes)
    RGBarrier barriers[RG_MAX_DEPENDENCIES];
    u32 barrier_count;
    
    // State
    bool is_compiled;
    bool profiling_enabled;
    
    // Statistics
    RGStats stats;
};

// === Internal Functions ===

// Resource management
RGResource *rg_get_resource(RenderGraph *rg, RGResourceHandle handle);
RGPass *rg_get_pass(RenderGraph *rg, RGPassHandle handle);

// Graph compilation steps
bool rg_build_dependency_graph(RenderGraph *rg);
bool rg_topological_sort(RenderGraph *rg);
void rg_cull_unused_passes(RenderGraph *rg);
void rg_calculate_resource_lifetimes(RenderGraph *rg);
void rg_generate_barriers(RenderGraph *rg);

// Resource pool (for transient allocation)
typedef struct RGResourcePool RGResourcePool;
RGResourcePool *rg_pool_create(void);
void rg_pool_destroy(RGResourcePool *pool);
TextureID rg_pool_allocate_texture(RGResourcePool *pool, const RGTextureDesc *desc);
BufferID rg_pool_allocate_buffer(RGResourcePool *pool, const RGBufferDesc *desc);
void rg_pool_reset(RGResourcePool *pool);

#ifdef __cplusplus
}
#endif

#endif // FRAME_GRAPH_INTERNAL_H
