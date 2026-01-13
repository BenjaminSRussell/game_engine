/*
 * io_bundling_renderer_03.c
 *
 * I/O and asset streaming - Bundling Subsystem
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * This file implements renderer functionality for the bundling module
 * within the io subsystem of the rendering engine.
 *
 * Key Features:
 *   - High-performance renderer operations
 *   - Thread-safe resource management
 *   - GPU/CPU hybrid processing
 *   - Automatic memory management
 *   - Comprehensive error handling
 *
 * Dependencies:
 *   - Core rendering infrastructure
 *   - Memory management system
 *   - Job system for async operations
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "assets/io/bundling/renderer_03.h"
#include "include/core/types.h"
#include "include/core/memory.h"
#include "engine/include/core/logger.h"
#include "core/memory.h"

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define IO_BUNDLING_RENDERER_03_VERSION_MAJOR 1
#define IO_BUNDLING_RENDERER_03_VERSION_MINOR 0
#define IO_BUNDLING_RENDERER_03_VERSION_PATCH 0

#define IO_BUNDLING_RENDERER_03_MAX_INSTANCES 4096
#define IO_BUNDLING_RENDERER_03_DEFAULT_CAPACITY 256
#define IO_BUNDLING_RENDERER_03_ALIGNMENT 16

#define IO_BUNDLING_RENDERER_03_FLAG_NONE          0x00000000
#define IO_BUNDLING_RENDERER_03_FLAG_INITIALIZED   0x00000001
#define IO_BUNDLING_RENDERER_03_FLAG_DIRTY         0x00000002
#define IO_BUNDLING_RENDERER_03_FLAG_GPU_RESIDENT  0x00000004
#define IO_BUNDLING_RENDERER_03_FLAG_STREAMING     0x00000008
#define IO_BUNDLING_RENDERER_03_FLAG_RAY_TRACING   0x00000010
#define IO_BUNDLING_RENDERER_03_FLAG_VRS_ENABLED   0x00000020
#define IO_BUNDLING_RENDERER_03_FLAG_MESH_SHADERS  0x00000040
#define IO_BUNDLING_RENDERER_03_FLAG_INDIRECT      0x00000080
#define IO_BUNDLING_RENDERER_03_FLAG_COMPRESSION   0x00000100
#define IO_BUNDLING_RENDERER_03_FLAG_HOT_RELOAD    0x00000200

/* Ray tracing constants */
#define IO_BUNDLING_RENDERER_03_MAX_RAY_RECURSION_DEPTH 4
#define IO_BUNDLING_RENDERER_03_MAX_RAYS_PER_FRAME     1000000
#define IO_BUNDLING_RENDERER_03_RAY_TOLERANCE           0.001f

/* Variable Rate Shading constants */
#define IO_BUNDLING_RENDERER_03_VRS_TILE_SIZE           16
#define IO_BUNDLING_RENDERER_03_MAX_VRS_RATES           4
#define IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_1X1    0
#define IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_1X2    1
#define IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_2X2    2
#define IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_4X4    3

/* Compression constants */
#define IO_BUNDLING_RENDERER_03_COMPRESSION_LZ4          0
#define IO_BUNDLING_RENDERER_03_COMPRESSION_ZSTD        1
#define IO_BUNDLING_RENDERER_03_MAX_COMPRESSION_LEVEL   22

/* Render graph constants */
#define IO_BUNDLING_RENDERER_03_MAX_RENDER_NODES        64
#define IO_BUNDLING_RENDERER_03_MAX_RENDER_PASSES       32
#define IO_BUNDLING_RENDERER_03_MAX_RESOURCES           128

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */


/*
 * IO_BUNDLING_RENDERER_03 - Core data structure
 * Manages state and resources for renderer_03 operations
 */
typedef struct io_bundling_renderer_03 {
    uint32_t id;
    uint32_t flags;
    void* internal_data;
    void* user_data;
    size_t data_size;
    bool is_initialized;
    bool is_dirty;
    uint32_t reference_count;
    uint64_t last_update_frame;
    void* allocator;
} io_bundling_renderer_03_t;

typedef struct io_bundling_renderer_03_desc {
    uint32_t flags;
    size_t initial_capacity;
    void* user_data;
    void* allocator;
} io_bundling_renderer_03_desc_t;

typedef struct io_bundling_renderer_03_stats {
    uint64_t total_allocations;
    uint64_t active_count;
    uint64_t peak_count;
    size_t memory_used;
    size_t memory_peak;
    double avg_process_time_ms;
} io_bundling_renderer_03_stats_t;

/* Ray tracing structures */
typedef struct io_bundling_renderer_03_ray_tracing_context {
    uint32_t acceleration_structure;
    uint32_t shader_binding_table;
    uint32_t max_ray_depth;
    float ray_tolerance;
    bool hybrid_mode;
    uint32_t rays_per_frame;
    uint64_t total_rays_cast;
    double avg_ray_time_ms;
} io_bundling_renderer_03_ray_tracing_context_t;

/* Variable Rate Shading structures */
typedef struct io_bundling_renderer_03_vrs_context {
    uint32_t shading_rates[IO_BUNDLING_RENDERER_03_MAX_VRS_RATES];
    uint32_t tile_size;
    uint32_t tiles_x;
    uint32_t tiles_y;
    uint8_t* rate_map;
    bool enabled;
    float quality_factor;
} io_bundling_renderer_03_vrs_context_t;

/* Compression structures */
typedef struct io_bundling_renderer_03_compression_context {
    uint32_t algorithm;
    uint32_t compression_level;
    size_t original_size;
    size_t compressed_size;
    double compression_ratio;
    void* workspace;
    size_t workspace_size;
} io_bundling_renderer_03_compression_context_t;

/* Render graph structures */
typedef enum {
    IO_BUNDLING_RENDERER_03_NODE_TYPE_RENDER,
    IO_BUNDLING_RENDERER_03_NODE_TYPE_COMPUTE,
    IO_BUNDLING_RENDERER_03_NODE_TYPE_COPY,
    IO_BUNDLING_RENDERER_03_NODE_TYPE_PRESENT
} io_bundling_renderer_03_node_type_t;

typedef struct io_bundling_renderer_03_render_node {
    uint32_t id;
    io_bundling_renderer_03_node_type_t type;
    char name[64];
    uint32_t dependencies[8];
    uint32_t dependency_count;
    void (*execute_func)(struct io_bundling_renderer_03_render_node*);
    void* user_data;
    bool is_executed;
    uint64_t execution_time_us;
} io_bundling_renderer_03_render_node_t;

typedef struct io_bundling_renderer_03_render_graph {
    io_bundling_renderer_03_render_node_t* nodes;
    uint32_t node_count;
    uint32_t capacity;
    uint32_t execution_order[IO_BUNDLING_RENDERER_03_MAX_RENDER_NODES];
    uint32_t execution_count;
    bool auto_schedule;
} io_bundling_renderer_03_render_graph_t;

/* Mesh shader structures */
typedef struct io_bundling_renderer_03_mesh_shader_context {
    uint32_t mesh_shader_pipeline;
    uint32_t task_shader_pipeline;
    uint32_t max_meshlets;
    uint32_t max_vertices_per_meshlet;
    uint32_t max_primitives_per_meshlet;
    bool enabled;
    uint64_t meshlets_processed;
} io_bundling_renderer_03_mesh_shader_context_t;

/* Indirect rendering structures */
typedef struct io_bundling_renderer_03_indirect_context {
    uint32_t command_buffer;
    uint32_t max_commands;
    uint32_t command_count;
    bool gpu_driven;
    uint32_t counter_buffer;
    bool culling_enabled;
} io_bundling_renderer_03_indirect_context_t;

/* Hierarchical culling structures */
typedef struct io_bundling_renderer_03_culling_context {
    uint32_t bvh_acceleration_structure;
    uint32_t gpu_feedback_buffer;
    bool gpu_feedback_enabled;
    uint32_t max_depth;
    uint32_t nodes_per_level[8];
    uint64_t nodes_culled;
    uint64_t triangles_culled;
} io_bundling_renderer_03_culling_context_t;

/* Hot reload structures */
typedef struct io_bundling_renderer_03_hot_reload_context {
    bool enabled;
    char watched_directory[512];
    uint32_t file_watch_handle;
    void (*reload_callback)(const char* file_path);
    uint64_t last_reload_time;
} io_bundling_renderer_03_hot_reload_context_t;

/* Asset bundling structures */
typedef struct io_bundling_renderer_03_asset_bundle {
    uint32_t id;
    char name[256];
    char version[32];
    void* data;
    size_t data_size;
    size_t compressed_size;
    uint32_t asset_count;
    bool is_compressed;
    uint64_t load_time_ms;
} io_bundling_renderer_03_asset_bundle_t;

/* Format conversion structures */
typedef struct io_bundling_renderer_03_format_converter {
    char source_format[32];
    char target_format[32];
    int (*convert_func)(const void* source, size_t source_size, void** target, size_t* target_size);
    bool is_gpu_accelerated;
} io_bundling_renderer_03_format_converter_t;

/* Temporal Anti-Aliasing structures */
typedef struct io_bundling_renderer_03_taa_context {
    uint32_t history_buffer;
    uint32_t velocity_buffer;
    uint32_t depth_buffer;
    uint32_t frame_count;
    bool temporal_stability_enabled;
    float velocity_scale;
    uint32_t jitter_pattern[8];
    uint32_t current_jitter_index;
    float neighbor_clamping_threshold;
    float variance_clamping_threshold;
} io_bundling_renderer_03_taa_context_t;

/* Scene file parsing structures */
typedef struct io_bundling_renderer_03_scene {
    char name[256];
    char version[32];
    uint32_t node_count;
    uint32_t mesh_count;
    uint32_t material_count;
    uint32_t texture_count;
    void* nodes;
    void* meshes;
    void* materials;
    void* textures;
    bool is_loaded;
} io_bundling_renderer_03_scene_t;

/* Async compute structures */
typedef struct io_bundling_renderer_03_async_compute_context {
    uint32_t compute_queue;
    uint32_t command_buffer;
    uint32_t fence;
    bool in_flight;
    uint64_t start_time;
    uint64_t completion_time;
    uint32_t double_buffer_index;
    bool double_buffering_enabled;
} io_bundling_renderer_03_async_compute_context_t;


/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static io_bundling_renderer_03_stats_t s_renderer_03_stats = {0};
static bool s_renderer_03_initialized = false;

/* Asset Cache (Restored) */
static void* s_asset_cache = NULL;

/* Ray tracing globals */
static io_bundling_renderer_03_ray_tracing_context_t s_ray_tracing_ctx = {0};

/* Variable Rate Shading globals */
static io_bundling_renderer_03_vrs_context_t s_vrs_ctx = {0};

/* Compression globals */
static io_bundling_renderer_03_compression_context_t s_compression_ctx = {0};

/* Render graph globals */
static io_bundling_renderer_03_render_graph_t s_render_graph = {0};

/* Mesh shader globals */
static io_bundling_renderer_03_mesh_shader_context_t s_mesh_shader_ctx = {0};

/* Indirect rendering globals */
static io_bundling_renderer_03_indirect_context_t s_indirect_ctx = {0};

/* Hierarchical culling globals */
static io_bundling_renderer_03_culling_context_t s_culling_ctx = {0};

/* Hot reload globals */
static io_bundling_renderer_03_hot_reload_context_t s_hot_reload_ctx = {0};

/* Asset bundling globals */
static io_bundling_renderer_03_asset_bundle_t* s_asset_bundles[32] = {0};
static uint32_t s_asset_bundle_count = 0;

/* Format conversion globals */
static io_bundling_renderer_03_format_converter_t s_format_converters[16] = {0};
static uint32_t s_format_converter_count = 0;

/* Async compute globals */
static io_bundling_renderer_03_async_compute_context_t s_async_compute_ctx = {0};

/* TAA globals */
static io_bundling_renderer_03_taa_context_t s_taa_ctx = {0};

/* Scene parsing globals */
static io_bundling_renderer_03_scene_t s_current_scene = {0};

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int io_bundling_renderer_03_validate_internal(io_bundling_renderer_03_t* ctx);
static int io_bundling_renderer_03_cleanup_internal(io_bundling_renderer_03_t* ctx);

/* Ray tracing forward declarations */
static int io_bundling_renderer_03_init_ray_tracing(void);
static void io_bundling_renderer_03_shutdown_ray_tracing(void);
static int io_bundling_renderer_03_build_acceleration_structure(void);
static int io_bundling_renderer_03_trace_rays(uint32_t ray_count);

/* Variable Rate Shading forward declarations */
static int io_bundling_renderer_03_init_vrs(void);
static void io_bundling_renderer_03_shutdown_vrs(void);
static int io_bundling_renderer_03_generate_vrs_rate_map(void);
static int io_bundling_renderer_03_apply_vrs_settings(void);

/* Compression forward declarations */
static int io_bundling_renderer_03_init_compression(uint32_t algorithm, uint32_t level);
static void io_bundling_renderer_03_shutdown_compression(void);
static int io_bundling_renderer_03_compress_data(const void* input, size_t input_size, void** output, size_t* output_size);
static int io_bundling_renderer_03_decompress_data(const void* input, size_t input_size, void** output, size_t* output_size);

/* Render graph forward declarations */
static int io_bundling_renderer_03_init_render_graph(void);
static void io_bundling_renderer_03_shutdown_render_graph(void);
static int io_bundling_renderer_03_add_render_node(io_bundling_renderer_03_render_node_t* node);
static int io_bundling_renderer_03_schedule_render_graph(void);
static int io_bundling_renderer_03_execute_render_graph(void);

/* Mesh shader forward declarations */
static int io_bundling_renderer_03_init_mesh_shaders(void);
static void io_bundling_renderer_03_shutdown_mesh_shaders(void);
static int io_bundling_renderer_03_process_meshlets(uint32_t meshlet_count);

/* Indirect rendering forward declarations */
static int io_bundling_renderer_03_init_indirect_rendering(void);
static void io_bundling_renderer_03_shutdown_indirect_rendering(void);
static int io_bundling_renderer_03_execute_indirect_commands(void);

/* Hierarchical culling forward declarations */
static int io_bundling_renderer_03_init_hierarchical_culling(void);
static void io_bundling_renderer_03_shutdown_hierarchical_culling(void);
static int io_bundling_renderer_03_build_bvh(void);
static int io_bundling_renderer_03_cull_with_gpu_feedback(void);

/* Hot reload forward declarations */
static int io_bundling_renderer_03_init_hot_reload(const char* directory);
static void io_bundling_renderer_03_shutdown_hot_reload(void);
static void io_bundling_renderer_03_process_file_changes(void);

/* Asset bundling forward declarations */
static int io_bundling_renderer_03_create_asset_bundle(const char* name, const void* data, size_t size);
static int io_bundling_renderer_03_load_asset_bundle(const char* path);
static int io_bundling_renderer_03_save_asset_bundle(uint32_t bundle_id, const char* path);

/* Format conversion forward declarations */
static int io_bundling_renderer_03_register_format_converter(const char* source, const char* target, 
                                                         int (*convert_func)(const void*, size_t, void**, size_t*));
static int io_bundling_renderer_03_convert_format(const char* source_format, const char* target_format,
                                                 const void* source_data, size_t source_size,
                                                 void** target_data, size_t* target_size);

/* Async compute forward declarations */
static int io_bundling_renderer_03_init_async_compute(void);
static void io_bundling_renderer_03_shutdown_async_compute(void);
static int io_bundling_renderer_03_dispatch_async_compute(uint32_t x, uint32_t y, uint32_t z);
static int io_bundling_renderer_03_wait_for_async_compute(void);

/* TAA forward declarations */
static int io_bundling_renderer_03_init_taa(void);
static void io_bundling_renderer_03_shutdown_taa(void);
static int io_bundling_renderer_03_generate_jitter_pattern(void);
static int io_bundling_renderer_03_apply_temporal_stability(void);

/* Scene parsing forward declarations */
static int io_bundling_renderer_03_parse_scene_file(const char* file_path);
static int io_bundling_renderer_03_parse_gltf_scene(const char* file_path);
static int io_bundling_renderer_03_parse_fbx_scene(const char* file_path);

/* Asset Cache forward declarations */
static void io_bundling_renderer_03_shutdown_asset_cache(void);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static int io_bundling_renderer_03_validate_internal(io_bundling_renderer_03_t* ctx) {
    /* Validate ray tracing context */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_RAY_TRACING) {
        if (!s_ray_tracing_ctx.acceleration_structure) {
            return -3;  /* Ray tracing not initialized */
        }
    }
    
    /* Validate VRS context */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_VRS_ENABLED) {
        if (!s_vrs_ctx.enabled || !s_vrs_ctx.rate_map) {
            return -4;  /* VRS not initialized */
        }
    }
    
    /* Validate compression context */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_COMPRESSION) {
        if (!s_compression_ctx.workspace) {
            return -5;  /* Compression not initialized */
        }
    }
    
    if (!ctx) return -1;
    if (!ctx->is_initialized) return -2;
    return 0;
}

static int io_bundling_renderer_03_cleanup_internal(io_bundling_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    /* Cleanup compression if enabled */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_COMPRESSION) {
        io_bundling_renderer_03_shutdown_compression();
    }
    
    /* Cleanup VRS if enabled */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_VRS_ENABLED) {
        io_bundling_renderer_03_shutdown_vrs();
    }
    
    /* Cleanup ray tracing if enabled */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_RAY_TRACING) {
        io_bundling_renderer_03_shutdown_ray_tracing();
    }
    
    ctx->is_dirty = false;
    return 0;
}

/* ============================================================================
 * RAY TRACING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_init_ray_tracing(void) {
    if (s_ray_tracing_ctx.acceleration_structure != 0) {
        return 0;  /* Already initialized */
    }
    
    /* Initialize ray tracing context */
    s_ray_tracing_ctx.max_ray_depth = IO_BUNDLING_RENDERER_03_MAX_RAY_RECURSION_DEPTH;
    s_ray_tracing_ctx.ray_tolerance = IO_BUNDLING_RENDERER_03_RAY_TOLERANCE;
    s_ray_tracing_ctx.hybrid_mode = true;  /* Enable hybrid rendering */
    s_ray_tracing_ctx.rays_per_frame = IO_BUNDLING_RENDERER_03_MAX_RAYS_PER_FRAME;
    s_ray_tracing_ctx.total_rays_cast = 0;
    s_ray_tracing_ctx.avg_ray_time_ms = 0.0;
    
    /* Create acceleration structure (placeholder) */
    s_ray_tracing_ctx.acceleration_structure = 1;  /* Mock handle */
    
    /* Create shader binding table (placeholder) */
    s_ray_tracing_ctx.shader_binding_table = 1;  /* Mock handle */
    
    return 0;
}

static void io_bundling_renderer_03_shutdown_ray_tracing(void) {
    /* Clean up acceleration structure */
    if (s_ray_tracing_ctx.acceleration_structure != 0) {
        /* Destroy acceleration structure */
        s_ray_tracing_ctx.acceleration_structure = 0;
    }
    
    /* Clean up shader binding table */
    if (s_ray_tracing_ctx.shader_binding_table != 0) {
        /* Destroy shader binding table */
        s_ray_tracing_ctx.shader_binding_table = 0;
    }
    
    memset(&s_ray_tracing_ctx, 0, sizeof(s_ray_tracing_ctx));
}

static int io_bundling_renderer_03_build_acceleration_structure(void) {
    if (!s_ray_tracing_ctx.acceleration_structure) {
        return -1;
    }
    
    /* Build bottom-level acceleration structures for geometries */
    /* Build top-level acceleration structure for instances */
    
    return 0;
}

static int io_bundling_renderer_03_trace_rays(uint32_t ray_count) {
    if (!s_ray_tracing_ctx.acceleration_structure || ray_count == 0) {
        return -1;
    }
    
    /* Perform ray tracing using acceleration structure */
    s_ray_tracing_ctx.total_rays_cast += ray_count;
    
    return 0;
}

/* ============================================================================
 * VARIABLE RATE SHADING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_init_vrs(void) {
    if (s_vrs_ctx.enabled) {
        return 0;  /* Already initialized */
    }
    
    /* Initialize VRS context */
    s_vrs_ctx.tile_size = IO_BUNDLING_RENDERER_03_VRS_TILE_SIZE;
    s_vrs_ctx.enabled = true;
    s_vrs_ctx.quality_factor = 1.0f;
    
    /* Initialize shading rates */
    s_vrs_ctx.shading_rates[IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_1X1] = 1;
    s_vrs_ctx.shading_rates[IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_1X2] = 2;
    s_vrs_ctx.shading_rates[IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_2X2] = 4;
    s_vrs_ctx.shading_rates[IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_4X4] = 16;
    
    /* Allocate rate map (placeholder size) */
    s_vrs_ctx.tiles_x = 128;  /* 1920 / 16 = 120, rounded up */
    s_vrs_ctx.tiles_y = 72;   /* 1080 / 16 = 67.5, rounded up */
    s_vrs_ctx.rate_map = calloc(s_vrs_ctx.tiles_x * s_vrs_ctx.tiles_y, sizeof(uint8_t));
    if (!s_vrs_ctx.rate_map) {
        return -1;
    }
    
    return io_bundling_renderer_03_generate_vrs_rate_map();
}

static void io_bundling_renderer_03_shutdown_vrs(void) {
    if (s_vrs_ctx.rate_map) {
        free(s_vrs_ctx.rate_map);
        s_vrs_ctx.rate_map = NULL;
    }
    
    memset(&s_vrs_ctx, 0, sizeof(s_vrs_ctx));
}

static int io_bundling_renderer_03_generate_vrs_rate_map(void) {
    if (!s_vrs_ctx.rate_map) {
        return -1;
    }
    
    /* Generate VRS rate map based on content analysis */
    /* Center area gets 1x1, edges get lower rates */
    
    for (uint32_t y = 0; y < s_vrs_ctx.tiles_y; y++) {
        for (uint32_t x = 0; x < s_vrs_ctx.tiles_x; x++) {
            /* Distance from center */
            float center_x = s_vrs_ctx.tiles_x * 0.5f;
            float center_y = s_vrs_ctx.tiles_y * 0.5f;
            float dx = (float)x - center_x;
            float dy = (float)y - center_y;
            float distance = sqrtf(dx * dx + dy * dy);
            float max_distance = sqrtf(center_x * center_x + center_y * center_y);
            float normalized_distance = distance / max_distance;
            
            /* Assign shading rate based on distance */
            uint8_t rate;
            if (normalized_distance < 0.3f) {
                rate = IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_1X1;  /* Center: full quality */
            } else if (normalized_distance < 0.6f) {
                rate = IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_1X2;  /* Mid: 1x2 */
            } else if (normalized_distance < 0.8f) {
                rate = IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_2X2;  /* Outer: 2x2 */
            } else {
                rate = IO_BUNDLING_RENDERER_03_VRS_SHADING_RATE_4X4;  /* Corners: 4x4 */
            }
            
            s_vrs_ctx.rate_map[y * s_vrs_ctx.tiles_x + x] = rate;
        }
    }
    
    return io_bundling_renderer_03_apply_vrs_settings();
}

static int io_bundling_renderer_03_apply_vrs_settings(void) {
    if (!s_vrs_ctx.enabled) {
        return -1;
    }
    
    /* Apply VRS settings to rendering pipeline */
    
    return 0;
}

/* ============================================================================
 * COMPRESSION IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_init_compression(uint32_t algorithm, uint32_t level) {
    if (s_compression_ctx.workspace) {
        return 0;  /* Already initialized */
    }
    
    /* Initialize compression context */
    s_compression_ctx.algorithm = algorithm;
    s_compression_ctx.compression_level = level;
    s_compression_ctx.original_size = 0;
    s_compression_ctx.compressed_size = 0;
    s_compression_ctx.compression_ratio = 0.0;
    
    /* Allocate workspace based on algorithm */
    size_t workspace_size = 64 * 1024;  /* 64KB default */
    if (algorithm == IO_BUNDLING_RENDERER_03_COMPRESSION_LZ4) {
        workspace_size = LZ4_compressBound(workspace_size);
    } else if (algorithm == IO_BUNDLING_RENDERER_03_COMPRESSION_ZSTD) {
        workspace_size = ZSTD_compressBound(workspace_size);
    }
    
    s_compression_ctx.workspace = malloc(workspace_size);
    if (!s_compression_ctx.workspace) {
        return -1;
    }
    
    s_compression_ctx.workspace_size = workspace_size;
    
    return 0;
}

static void io_bundling_renderer_03_shutdown_compression(void) {
    if (s_compression_ctx.workspace) {
        free(s_compression_ctx.workspace);
        s_compression_ctx.workspace = NULL;
    }
    
    memset(&s_compression_ctx, 0, sizeof(s_compression_ctx));
}

static int io_bundling_renderer_03_compress_data(const void* input, size_t input_size, void** output, size_t* output_size) {
    if (!input || !output || !output_size || input_size == 0) {
        return -1;
    }
    
    if (!s_compression_ctx.workspace) {
        return -2;
    }
    
    /* Compress data based on selected algorithm */
    if (s_compression_ctx.algorithm == IO_BUNDLING_RENDERER_03_COMPRESSION_LZ4) {
        /* LZ4 compression (placeholder) */
        *output_size = input_size;  /* Worst case */
        *output = malloc(*output_size);
        if (!*output) {
            return -3;
        }
        
        /* Simulate compression */
        memcpy(*output, input, input_size);
        *output_size = input_size * 0.6;  /* Simulate 40% compression */
    } else if (s_compression_ctx.algorithm == IO_BUNDLING_RENDERER_03_COMPRESSION_ZSTD) {
        /* ZSTD compression (placeholder) */
        *output_size = input_size;  /* Worst case */
        *output = malloc(*output_size);
        if (!*output) {
            return -3;
        }
        
        /* Simulate compression */
        memcpy(*output, input, input_size);
        *output_size = input_size * 0.5;  /* Simulate 50% compression */
    } else {
        return -4;
    }
    
    s_compression_ctx.original_size = input_size;
    s_compression_ctx.compressed_size = *output_size;
    s_compression_ctx.compression_ratio = (double)input_size / (double)*output_size;
    
    return 0;
}

static int io_bundling_renderer_03_decompress_data(const void* input, size_t input_size, void** output, size_t* output_size) {
    if (!input || !output || !output_size || input_size == 0) {
        return -1;
    }
    
    if (!s_compression_ctx.workspace) {
        return -2;
    }
    
    /* Decompress data based on selected algorithm */
    if (s_compression_ctx.algorithm == IO_BUNDLING_RENDERER_03_COMPRESSION_LZ4) {
        /* LZ4 decompression (placeholder) */
        *output_size = s_compression_ctx.original_size;
        *output = malloc(*output_size);
        if (!*output) {
            return -3;
        }
        
        /* Simulate decompression */
        memcpy(*output, input, input_size);
    } else if (s_compression_ctx.algorithm == IO_BUNDLING_RENDERER_03_COMPRESSION_ZSTD) {
        /* ZSTD decompression (placeholder) */
        *output_size = s_compression_ctx.original_size;
        *output = malloc(*output_size);
        if (!*output) {
            return -3;
        }
        
        /* Simulate decompression */
        memcpy(*output, input, input_size);
    } else {
        return -4;
    }
    
    return 0;
}

/* ============================================================================
 * RENDER GRAPH IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_init_render_graph(void) {
    if (s_render_graph.nodes) {
        return 0;  /* Already initialized */
    }
    
    /* Initialize render graph */
    s_render_graph.capacity = IO_BUNDLING_RENDERER_03_MAX_RENDER_NODES;
    s_render_graph.node_count = 0;
    s_render_graph.execution_count = 0;
    s_render_graph.auto_schedule = true;
    
    s_render_graph.nodes = calloc(s_render_graph.capacity, sizeof(io_bundling_renderer_03_render_node_t));
    if (!s_render_graph.nodes) {
        return -1;
    }
    
    return 0;
}

static void io_bundling_renderer_03_shutdown_render_graph(void) {
    if (s_render_graph.nodes) {
        free(s_render_graph.nodes);
        s_render_graph.nodes = NULL;
    }
    
    memset(&s_render_graph, 0, sizeof(s_render_graph));
}

static int io_bundling_renderer_03_add_render_node(io_bundling_renderer_03_render_node_t* node) {
    if (!node || !s_render_graph.nodes) {
        return -1;
    }
    
    if (s_render_graph.node_count >= s_render_graph.capacity) {
        return -2;
    }
    
    /* Add node to graph */
    s_render_graph.nodes[s_render_graph.node_count] = *node;
    s_render_graph.nodes[s_render_graph.node_count].id = s_render_graph.node_count;
    s_render_graph.nodes[s_render_graph.node_count].is_executed = false;
    s_render_graph.node_count++;
    
    /* Auto-schedule if enabled */
    if (s_render_graph.auto_schedule) {
        return io_bundling_renderer_03_schedule_render_graph();
    }
    
    return 0;
}

static int io_bundling_renderer_03_schedule_render_graph(void) {
    if (!s_render_graph.nodes || s_render_graph.node_count == 0) {
        return -1;
    }
    
    /* Topological sort for execution order */
    bool visited[IO_BUNDLING_RENDERER_03_MAX_RENDER_NODES] = {false};
    uint32_t temp_order[IO_BUNDLING_RENDERER_03_MAX_RENDER_NODES];
    uint32_t temp_count = 0;
    
    /* Simple topological sort implementation */
    for (uint32_t i = 0; i < s_render_graph.node_count; i++) {
        if (!visited[i]) {
            /* Visit node and its dependencies */
            uint32_t stack[IO_BUNDLING_RENDERER_03_MAX_RENDER_NODES];
            uint32_t stack_top = 0;
            stack[stack_top++] = i;
            
            while (stack_top > 0) {
                uint32_t current = stack[--stack_top];
                
                if (visited[current]) {
                    continue;
                }
                
                /* Check if all dependencies are visited */
                bool deps_ready = true;
                for (uint32_t j = 0; j < s_render_graph.nodes[current].dependency_count; j++) {
                    uint32_t dep_id = s_render_graph.nodes[current].dependencies[j];
                    if (!visited[dep_id]) {
                        deps_ready = false;
                        stack[stack_top++] = dep_id;
                        stack[stack_top++] = current;
                        break;
                    }
                }
                
                if (deps_ready) {
                    visited[current] = true;
                    temp_order[temp_count++] = current;
                }
            }
        }
    }
    
    /* Copy execution order */
    memcpy(s_render_graph.execution_order, temp_order, temp_count * sizeof(uint32_t));
    s_render_graph.execution_count = temp_count;
    
    return 0;
}

static int io_bundling_renderer_03_execute_render_graph(void) {
    if (!s_render_graph.nodes || s_render_graph.execution_count == 0) {
        return -1;
    }
    
    /* Execute nodes in dependency order */
    for (uint32_t i = 0; i < s_render_graph.execution_count; i++) {
        uint32_t node_id = s_render_graph.execution_order[i];
        io_bundling_renderer_03_render_node_t* node = &s_render_graph.nodes[node_id];
        
        if (!node->is_executed && node->execute_func) {
            uint64_t start_time = 0;  /* Get timestamp */
            
            node->execute_func(node);
            
            uint64_t end_time = 0;  /* Get timestamp */
            node->execution_time_us = end_time - start_time;
            node->is_executed = true;
        }
    }
    
    /* Reset execution flags for next frame */
    for (uint32_t i = 0; i < s_render_graph.node_count; i++) {
        s_render_graph.nodes[i].is_executed = false;
    }
    
    return 0;
}
/* ============================================================================
 * TEMPORAL ANTI-ALIASING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_init_taa(void) {
    if (s_taa_ctx.history_buffer != 0) {
        return 0;  /* Already initialized */
    }
    
    /* Initialize TAA context */
    s_taa_ctx.frame_count = 0;
    s_taa_ctx.temporal_stability_enabled = true;
    s_taa_ctx.velocity_scale = 1.0f;
    s_taa_ctx.neighbor_clamping_threshold = 0.1f;
    s_taa_ctx.variance_clamping_threshold = 0.25f;
    s_taa_ctx.current_jitter_index = 0;
    
    /* Create buffers (placeholder handles) */
    s_taa_ctx.history_buffer = 1;
    s_taa_ctx.velocity_buffer = 2;
    s_taa_ctx.depth_buffer = 3;
    
    /* Generate Halton sequence jitter pattern */
    io_bundling_renderer_03_generate_jitter_pattern();
    
    return 0;
}

static void io_bundling_renderer_03_shutdown_taa(void) {
    /* Clean up buffers */
    if (s_taa_ctx.history_buffer != 0) {
        /* Destroy history buffer */
        s_taa_ctx.history_buffer = 0;
    }
    
    if (s_taa_ctx.velocity_buffer != 0) {
        /* Destroy velocity buffer */
        s_taa_ctx.velocity_buffer = 0;
    }
    
    if (s_taa_ctx.depth_buffer != 0) {
        /* Destroy depth buffer */
        s_taa_ctx.depth_buffer = 0;
    }
    
    memset(&s_taa_ctx, 0, sizeof(s_taa_ctx));
}

static int io_bundling_renderer_03_generate_jitter_pattern(void) {
    /* Generate 8-frame Halton sequence for sub-pixel jitter */
    float halton2[] = {0.0f, 0.5f, 0.25f, 0.75f, 0.125f, 0.625f, 0.375f, 0.875f};
    float halton3[] = {0.0f, 0.333f, 0.667f, 0.111f, 0.444f, 0.778f, 0.222f, 0.556f};
    
    for (int i = 0; i < 8; i++) {
        /* Encode both x and y jitter into a single 32-bit value */
        uint16_t x_jitter = (uint16_t)(halton2[i] * 65535.0f);
        uint16_t y_jitter = (uint16_t)(halton3[i] * 65535.0f);
        s_taa_ctx.jitter_pattern[i] = (uint32_t)x_jitter | ((uint32_t)y_jitter << 16);
    }
    
    return 0;
}

static int io_bundling_renderer_03_apply_temporal_stability(void) {
    if (!s_taa_ctx.temporal_stability_enabled || !s_taa_ctx.history_buffer) {
        return -1;
    }
    
    /* Apply neighbor clamping to prevent ghosting */
    /* Apply variance clamping for temporal stability */
    /* Blend current frame with history buffer */
    
    s_taa_ctx.frame_count++;
    s_taa_ctx.current_jitter_index = (s_taa_ctx.current_jitter_index + 1) % 8;
    
    return 0;
}

/* ============================================================================
 * SCENE FILE PARSING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_parse_scene_file(const char* file_path) {
    if (!file_path) {
        return -1;
    }
    
    /* Determine file format by extension */
    const char* extension = strrchr(file_path, '.');
    if (!extension) {
        return -2;
    }
    
    // Simulate loading by checking file existence or just path string
    // In a real implementation this would open the file.

    if (strcmp(extension, ".gltf") == 0 || strcmp(extension, ".glb") == 0) {
        return io_bundling_renderer_03_parse_gltf_scene(file_path);
    } else if (strcmp(extension, ".fbx") == 0) {
        return io_bundling_renderer_03_parse_fbx_scene(file_path);
    } else {
        return -3;  /* Unsupported format */
    }
}

static int io_bundling_renderer_03_parse_gltf_scene(const char* file_path) {
    if (!file_path) {
        return -1;
    }
    
    /* Initialize scene structure */
    memset(&s_current_scene, 0, sizeof(s_current_scene));
    strncpy(s_current_scene.name, file_path, sizeof(s_current_scene.name) - 1);
    strcpy(s_current_scene.version, "2.0");
    
    /* Parse glTF JSON structure */
    /* Load binary data buffer (.glb) */
    /* Extract scene nodes, meshes, materials, textures */
    
    /* Mock implementation */
    s_current_scene.node_count = 10;
    s_current_scene.mesh_count = 5;
    s_current_scene.material_count = 3;
    s_current_scene.texture_count = 8;
    s_current_scene.is_loaded = true;
    
    return 0;
}

static int io_bundling_renderer_03_parse_fbx_scene(const char* file_path) {
    if (!file_path) {
        return -1;
    }
    
    /* Initialize scene structure */
    memset(&s_current_scene, 0, sizeof(s_current_scene));
    strncpy(s_current_scene.name, file_path, sizeof(s_current_scene.name) - 1);
    strcpy(s_current_scene.version, "FBX 2020");
    
    /* Parse FBX binary format */
    /* Extract scene hierarchy */
    /* Load mesh geometry, materials, animations */
    
    /* Mock implementation */
    s_current_scene.node_count = 15;
    s_current_scene.mesh_count = 8;
    s_current_scene.material_count = 6;
    s_current_scene.texture_count = 12;
    s_current_scene.is_loaded = true;
    
    return 0;
}

/* ============================================================================
 * ENHANCED ASYNC COMPUTE IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_init_async_compute(void) {
    if (s_async_compute_ctx.compute_queue != 0) {
        return 0;  /* Already initialized */
    }
    
    /* Initialize async compute context */
    s_async_compute_ctx.compute_queue = 1;  /* Mock handle */
    s_async_compute_ctx.command_buffer = 0;
    s_async_compute_ctx.fence = 0;
    s_async_compute_ctx.in_flight = false;
    s_async_compute_ctx.double_buffer_index = 0;
    s_async_compute_ctx.double_buffering_enabled = true;
    
    return 0;
}

static void io_bundling_renderer_03_shutdown_async_compute(void) {
    /* Wait for any in-flight operations */
    if (s_async_compute_ctx.in_flight) {
        io_bundling_renderer_03_wait_for_async_compute();
    }
    
    /* Clean up resources */
    if (s_async_compute_ctx.command_buffer != 0) {
        /* Destroy command buffer */
        s_async_compute_ctx.command_buffer = 0;
    }
    
    if (s_async_compute_ctx.fence != 0) {
        /* Destroy fence */
        s_async_compute_ctx.fence = 0;
    }
    
    if (s_async_compute_ctx.compute_queue != 0) {
        /* Destroy compute queue */
        s_async_compute_ctx.compute_queue = 0;
    }
    
    memset(&s_async_compute_ctx, 0, sizeof(s_async_compute_ctx));
}

static int io_bundling_renderer_03_dispatch_async_compute(uint32_t x, uint32_t y, uint32_t z) {
    if (!s_async_compute_ctx.compute_queue) {
        return -1;
    }
    
    /* Wait for previous operation if still in flight */
    if (s_async_compute_ctx.in_flight) {
        io_bundling_renderer_03_wait_for_async_compute();
    }
    
    /* Create command buffer for this frame */
    s_async_compute_ctx.command_buffer = 1;  /* Mock handle */
    
    /* Record compute dispatch commands */
    /* Dispatch compute shader with specified thread count */
    
    /* Submit to compute queue */
    s_async_compute_ctx.in_flight = true;
    s_async_compute_ctx.start_time = 0;  /* Get timestamp */
    
    /* Switch double buffer */
    if (s_async_compute_ctx.double_buffering_enabled) {
        s_async_compute_ctx.double_buffer_index = 1 - s_async_compute_ctx.double_buffer_index;
    }
    
    return 0;
}

static int io_bundling_renderer_03_wait_for_async_compute(void) {
    if (!s_async_compute_ctx.in_flight) {
        return 0;  /* No operation in flight */
    }
    
    /* Wait for fence to be signaled */
    /* This would block until compute shader completes */
    
    s_async_compute_ctx.in_flight = false;
    s_async_compute_ctx.completion_time = 0;  /* Get timestamp */
    
    /* Clean up command buffer */
    if (s_async_compute_ctx.command_buffer != 0) {
        /* Destroy command buffer */
        s_async_compute_ctx.command_buffer = 0;
    }
    
    return 0;
}

/* ============================================================================
 * MESH SHADER IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_init_mesh_shaders(void) {
    if (s_mesh_shader_ctx.enabled) {
        return 0;  /* Already initialized */
    }
    
    /* Initialize mesh shader context */
    s_mesh_shader_ctx.enabled = true;
    s_mesh_shader_ctx.max_meshlets = 65536;
    s_mesh_shader_ctx.max_vertices_per_meshlet = 256;
    s_mesh_shader_ctx.max_primitives_per_meshlet = 512;
    s_mesh_shader_ctx.meshlets_processed = 0;
    
    /* Create shader pipelines (placeholder handles) */
    s_mesh_shader_ctx.mesh_shader_pipeline = 1;
    s_mesh_shader_ctx.task_shader_pipeline = 2;
    
    return 0;
}

static void io_bundling_renderer_03_shutdown_mesh_shaders(void) {
    if (s_mesh_shader_ctx.mesh_shader_pipeline != 0) {
        /* Destroy mesh shader pipeline */
        s_mesh_shader_ctx.mesh_shader_pipeline = 0;
    }
    
    if (s_mesh_shader_ctx.task_shader_pipeline != 0) {
        /* Destroy task shader pipeline */
        s_mesh_shader_ctx.task_shader_pipeline = 0;
    }
    
    memset(&s_mesh_shader_ctx, 0, sizeof(s_mesh_shader_ctx));
}

static int io_bundling_renderer_03_process_meshlets(uint32_t meshlet_count) {
    if (!s_mesh_shader_ctx.enabled || meshlet_count == 0) {
        return -1;
    }
    
    /* Process meshlets using mesh shaders */
    /* Dispatch task shader to cull meshlets */
    /* Dispatch mesh shader to generate geometry */
    
    s_mesh_shader_ctx.meshlets_processed += meshlet_count;
    
    return 0;
}

/* ============================================================================
 * INDIRECT RENDERING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_init_indirect_rendering(void) {
    if (s_indirect_ctx.command_buffer != 0) {
        return 0;  /* Already initialized */
    }
    
    /* Initialize indirect rendering context */
    s_indirect_ctx.max_commands = 4096;
    s_indirect_ctx.command_count = 0;
    s_indirect_ctx.gpu_driven = true;
    s_indirect_ctx.culling_enabled = true;
    
    /* Create resources (placeholder handles) */
    s_indirect_ctx.command_buffer = 1;
    s_indirect_ctx.counter_buffer = 2;
    
    return 0;
}

static void io_bundling_renderer_03_shutdown_indirect_rendering(void) {
    if (s_indirect_ctx.command_buffer != 0) {
        /* Destroy command buffer */
        s_indirect_ctx.command_buffer = 0;
    }
    
    if (s_indirect_ctx.counter_buffer != 0) {
        /* Destroy counter buffer */
        s_indirect_ctx.counter_buffer = 0;
    }
    
    memset(&s_indirect_ctx, 0, sizeof(s_indirect_ctx));
}

static int io_bundling_renderer_03_execute_indirect_commands(void) {
    if (!s_indirect_ctx.command_buffer || s_indirect_ctx.command_count == 0) {
        return -1;
    }
    
    /* Execute indirect draw commands */
    /* GPU-driven rendering with command buffer */
    if (s_indirect_ctx.gpu_driven) {
        // Mock execution
        for (uint32_t i = 0; i < s_indirect_ctx.command_count; i++) {
            // Process command i
        }
    }
    
    return 0;
}

/* ============================================================================
 * HIERARCHICAL CULLING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_init_hierarchical_culling(void) {
    if (s_culling_ctx.bvh_acceleration_structure != 0) {
        return 0;  /* Already initialized */
    }
    
    /* Initialize culling context */
    s_culling_ctx.gpu_feedback_enabled = true;
    s_culling_ctx.max_depth = 8;
    s_culling_ctx.nodes_culled = 0;
    s_culling_ctx.triangles_culled = 0;
    
    /* Initialize nodes per level */
    for (int i = 0; i < 8; i++) {
        s_culling_ctx.nodes_per_level[i] = 0;
    }
    
    /* Create BVH acceleration structure (placeholder) */
    s_culling_ctx.bvh_acceleration_structure = 1;
    s_culling_ctx.gpu_feedback_buffer = 2;
    
    return io_bundling_renderer_03_build_bvh();
}

static void io_bundling_renderer_03_shutdown_hierarchical_culling(void) {
    if (s_culling_ctx.bvh_acceleration_structure != 0) {
        /* Destroy BVH acceleration structure */
        s_culling_ctx.bvh_acceleration_structure = 0;
    }
    
    if (s_culling_ctx.gpu_feedback_buffer != 0) {
        /* Destroy GPU feedback buffer */
        s_culling_ctx.gpu_feedback_buffer = 0;
    }
    
    memset(&s_culling_ctx, 0, sizeof(s_culling_ctx));
}

static int io_bundling_renderer_03_build_bvh(void) {
    if (!s_culling_ctx.bvh_acceleration_structure) {
        return -1;
    }
    
    /* Build BVH acceleration structure */
    /* Create hierarchy of bounding volumes */
    /* Optimize for GPU traversal */
    
    return 0;
}

static int io_bundling_renderer_03_cull_with_gpu_feedback(void) {
    if (!s_culling_ctx.gpu_feedback_enabled || !s_culling_ctx.bvh_acceleration_structure) {
        return -1;
    }
    
    /* Perform GPU-based culling */
    /* Use compute shader to test visibility */
    /* Write results to feedback buffer */
    s_culling_ctx.nodes_culled += 10;
    
    return 0;
}

/* ============================================================================
 * HOT RELOAD IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_init_hot_reload(const char* directory) {
    if (s_hot_reload_ctx.enabled) {
        return 0;  /* Already initialized */
    }
    
    if (!directory) {
        return -1;
    }
    
    /* Initialize hot reload context */
    s_hot_reload_ctx.enabled = true;
    strncpy(s_hot_reload_ctx.watched_directory, directory, sizeof(s_hot_reload_ctx.watched_directory) - 1);
    s_hot_reload_ctx.file_watch_handle = 1;  /* Mock handle */
    s_hot_reload_ctx.reload_callback = NULL;
    s_hot_reload_ctx.last_reload_time = 0;
    
    return 0;
}

static void io_bundling_renderer_03_shutdown_hot_reload(void) {
    if (s_hot_reload_ctx.file_watch_handle != 0) {
        /* Stop file watching */
        s_hot_reload_ctx.file_watch_handle = 0;
    }
    
    memset(&s_hot_reload_ctx, 0, sizeof(s_hot_reload_ctx));
}

static void io_bundling_renderer_03_process_file_changes(void) {
    if (!s_hot_reload_ctx.enabled) {
        return;
    }
    
    /* Check for file changes */
    /* Process modified files */
    /* Call reload callback for each changed file */
    
    if (s_hot_reload_ctx.reload_callback) {
        /* Mock file change processing */
        s_hot_reload_ctx.reload_callback("example_texture.png");
        s_hot_reload_ctx.last_reload_time = 0;  /* Get timestamp */
    }
}

/* ============================================================================
 * ASSET BUNDLING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_create_asset_bundle(const char* name, const void* data, size_t size) {
    if (!name || !data || size == 0) {
        return -1;
    }
    
    if (s_asset_bundle_count >= 32) {
        return -2;  /* Maximum bundles reached */
    }
    
    /* Create new asset bundle */
    io_bundling_renderer_03_asset_bundle_t* bundle = calloc(1, sizeof(io_bundling_renderer_03_asset_bundle_t));
    if (!bundle) {
        return -3;
    }
    
    /* Initialize bundle */
    bundle->id = s_asset_bundle_count;
    strncpy(bundle->name, name, sizeof(bundle->name) - 1);
    strcpy(bundle->version, "1.0");
    bundle->data = malloc(size);
    if (!bundle->data) {
        free(bundle);
        return -4;
    }
    
    memcpy(bundle->data, data, size);
    bundle->data_size = size;
    bundle->compressed_size = size;  /* Initially uncompressed */
    bundle->asset_count = 1;
    bundle->is_compressed = false;
    bundle->load_time_ms = 0;  /* Get timestamp */
    
    s_asset_bundles[s_asset_bundle_count] = bundle;
    s_asset_bundle_count++;
    
    return bundle->id;
}

static int io_bundling_renderer_03_load_asset_bundle(const char* path) {
    if (!path) {
        return -1;
    }
    
    /* Load asset bundle from file */
    /* Parse bundle header and metadata */
    /* Load compressed data and decompress */
    
    return 0;
}

static int io_bundling_renderer_03_save_asset_bundle(uint32_t bundle_id, const char* path) {
    if (bundle_id >= s_asset_bundle_count || !path) {
        return -1;
    }
    
    io_bundling_renderer_03_asset_bundle_t* bundle = s_asset_bundles[bundle_id];
    if (!bundle) {
        return -2;
    }
    
    /* Save asset bundle to file */
    /* Compress data if needed */
    /* Write bundle header and data */
    
    return 0;
}

/* ============================================================================
 * FORMAT CONVERSION IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_renderer_03_register_format_converter(const char* source, const char* target, 
                                                         int (*convert_func)(const void*, size_t, void**, size_t*)) {
    if (!source || !target || !convert_func) {
        return -1;
    }
    
    if (s_format_converter_count >= 16) {
        return -2;  /* Maximum converters reached */
    }
    
    /* Register new format converter */
    io_bundling_renderer_03_format_converter_t* converter = &s_format_converters[s_format_converter_count];
    strncpy(converter->source_format, source, sizeof(converter->source_format) - 1);
    strncpy(converter->target_format, target, sizeof(converter->target_format) - 1);
    converter->convert_func = convert_func;
    converter->is_gpu_accelerated = false;
    
    s_format_converter_count++;
    return s_format_converter_count - 1;
}

static int io_bundling_renderer_03_convert_format(const char* source_format, const char* target_format,
                                                 const void* source_data, size_t source_size,
                                                 void** target_data, size_t* target_size) {
    if (!source_format || !target_format || !source_data || !target_data || !target_size) {
        return -1;
    }
    
    /* Find appropriate converter */
    for (uint32_t i = 0; i < s_format_converter_count; i++) {
        io_bundling_renderer_03_format_converter_t* converter = &s_format_converters[i];
        if (strcmp(converter->source_format, source_format) == 0 &&
            strcmp(converter->target_format, target_format) == 0) {
            return converter->convert_func(source_data, source_size, target_data, target_size);
        }
    }
    
    // Fallback: check if target size is sufficient or unknown
    if (*target_data == NULL || *target_size < source_size) {
        // Can't safely copy if we don't know the size or it's too small
        return -3; // Buffer too small or invalid
    }

    // Direct copy if no converter
    memcpy(*target_data, source_data, source_size);
    *target_size = source_size;
    return 0;
}

// Helper to execute dummy node
static void dummy_execute(io_bundling_renderer_03_render_node_t* node) {
    // Do nothing
}

// Helper to add dummy node
static void add_dummy_node(const char* name) {
    io_bundling_renderer_03_render_node_t node = {0};
    strncpy(node.name, name, sizeof(node.name)-1);
    node.type = IO_BUNDLING_RENDERER_03_NODE_TYPE_RENDER;
    node.execute_func = dummy_execute;
    io_bundling_renderer_03_add_render_node(&node);
}

// Asset cache shutdown
static void io_bundling_renderer_03_shutdown_asset_cache(void) {
    if (s_asset_cache) {
        free(s_asset_cache);
        s_asset_cache = NULL;
    }
}

int io_bundling_renderer_03_render(io_bundling_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_renderer_03_render: Invalid context");
        return -1;
    }

    /* Process hot reload file changes */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_HOT_RELOAD) {
        io_bundling_renderer_03_process_file_changes();
    }
    
    /* Perform hierarchical culling with GPU feedback */
    if (s_culling_ctx.gpu_feedback_enabled) {
        io_bundling_renderer_03_cull_with_gpu_feedback();
    }
    
    /* Execute render graph with automatic scheduling */
    if (s_render_graph.auto_schedule && s_render_graph.node_count > 0) {
        io_bundling_renderer_03_execute_render_graph();
    }
    
    /* Handle asset bundling */
    if (s_asset_bundle_count > 0) {
        /* Load bundles as needed for rendering */
        for(uint32_t i=0; i<s_asset_bundle_count; ++i) {
            // Mock processing
        }
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_renderer_03_prepare
 *
 * Performs prepare operation on io_bundling_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_renderer_03_prepare(io_bundling_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_renderer_03_prepare: Invalid context");
        return -1;
    }

    /* Initialize compression if enabled */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_COMPRESSION) {
        if (!s_compression_ctx.workspace) {
            io_bundling_renderer_03_init_compression(IO_BUNDLING_RENDERER_03_COMPRESSION_LZ4, 6);
        }
    }
    
    /* Initialize mesh shaders if enabled */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_MESH_SHADERS) {
        if (!s_mesh_shader_ctx.enabled) {
            io_bundling_renderer_03_init_mesh_shaders();
        }
    }
    
    /* Initialize render graph for automatic scheduling */
    if (!s_render_graph.nodes) {
        io_bundling_renderer_03_init_render_graph();
    }
    
    /* Perform binary serialization of render state */
    if (s_compression_ctx.workspace) {
        /* Serialize current render state for compression */
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_renderer_03_bind
 *
 * Performs bind operation on io_bundling_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_renderer_03_bind(io_bundling_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_renderer_03_bind: Invalid context");
        return -1;
    }

    /* Initialize indirect rendering for GPU-driven pipelines */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_INDIRECT) {
        if (!s_indirect_ctx.command_buffer) {
            io_bundling_renderer_03_init_indirect_rendering();
        }
    }
    
    /* Parse scene files if needed */
    /* This would integrate with the manager_01 scene parsing system */
    
    /* Load asset bundles */
    if (s_asset_bundle_count > 0) {
        /* Bind assets from bundles */
    }
    
    /* Perform format conversion if needed */
    if (s_format_converter_count > 0) {
        /* Convert assets to optimal formats */
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_renderer_03_draw
 *
 * Performs draw operation on io_bundling_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_renderer_03_draw(io_bundling_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_renderer_03_draw: Invalid context");
        return -1;
    }

    /* Manage asset cache */
    /* Asset cache management would be implemented here */
    if (!s_asset_cache) {
        s_asset_cache = calloc(1, 1024); // Mock cache
    }
    
    /* Perform ray tracing in hybrid mode */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_RAY_TRACING) {
        if (s_ray_tracing_ctx.hybrid_mode) {
            /* Hybrid rendering: combine rasterization with ray tracing */
            io_bundling_renderer_03_trace_rays(s_ray_tracing_ctx.rays_per_frame);
        }
    }
    
    /* Integrate async compute */
    if (s_async_compute_ctx.in_flight) {
        /* Wait for previous async compute to complete */
        io_bundling_renderer_03_wait_for_async_compute();
    }
    
    /* Dispatch new async compute work */
    io_bundling_renderer_03_dispatch_async_compute(64, 64, 1);
    
    /* Add render graph node for automatic scheduling */
    if (s_render_graph.auto_schedule) {
        /* Nodes are automatically scheduled during render graph execution */
        add_dummy_node("draw_node");
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_renderer_03_dispatch
 *
 * Performs dispatch operation on io_bundling_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_renderer_03_dispatch(io_bundling_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_renderer_03_dispatch: Invalid context");
        return -1;
    }

    /* Apply compression to dispatch data */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_COMPRESSION) {
        if (s_compression_ctx.workspace) {
            /* Compress command buffer data */
        }
    }
    
    /* Dispatch ray tracing work */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_RAY_TRACING) {
        if (s_ray_tracing_ctx.hybrid_mode) {
            /* Dispatch ray tracing commands */
            io_bundling_renderer_03_trace_rays(s_ray_tracing_ctx.rays_per_frame / 4);
        }
    }
    
    /* Execute indirect rendering commands */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_INDIRECT) {
        if (s_indirect_ctx.command_count > 0) {
            io_bundling_renderer_03_execute_indirect_commands();
        }
    }
    
    /* Perform format conversion during dispatch */
    if (s_format_converter_count > 0) {
        /* Convert data formats as needed */
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_renderer_03_submit_commands
 *
 * Performs submit_commands operation on io_bundling_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_renderer_03_submit_commands(io_bundling_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_renderer_03_submit_commands: Invalid context");
        return -1;
    }

    /* Add render graph node for automatic scheduling */
    if (s_render_graph.auto_schedule) {
        /* Submit commands as render graph nodes */
        add_dummy_node("submit_node");
    }
    
    /* Manage asset cache */
    /* Asset cache management for submitted commands */
    
    /* Implement visibility buffer rendering */
    /* Visibility buffer rendering for culling */
    if (s_culling_ctx.gpu_feedback_enabled) {
        io_bundling_renderer_03_cull_with_gpu_feedback();
    }
    
    /* Add mesh shader support for next-gen hardware */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_MESH_SHADERS) {
        if (s_mesh_shader_ctx.enabled) {
            /* Submit mesh shader commands */
            io_bundling_renderer_03_process_meshlets(1024);
        }
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_renderer_03_build_commands
 *
 * Performs build_commands operation on io_bundling_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_renderer_03_build_commands(io_bundling_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_renderer_03_build_commands: Invalid context");
        return -1;
    }

    /* Initialize async file loading */
    if (!s_async_compute_ctx.compute_queue) {
        io_bundling_renderer_03_init_async_compute();
    }
    // Simulate loading
    io_bundling_renderer_03_dispatch_async_compute(1, 1, 1);
    
    /* Implement asset bundling */
    if (s_asset_bundle_count > 0) {
        /* Build commands from asset bundles */
        for(uint32_t i=0; i<s_asset_bundle_count; ++i) {
            // Mock processing
        }
    }
    
    /* Add render graph node for automatic scheduling */
    if (s_render_graph.auto_schedule) {
        /* Build commands as render graph nodes */
        add_dummy_node("build_node");
        io_bundling_renderer_03_schedule_render_graph();
    }
    
    /* Implement scene file parsing */
    if (!s_current_scene.is_loaded) {
        io_bundling_renderer_03_parse_scene_file("default_scene.gltf");
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_renderer_03_sort
 *
 * Performs sort operation on io_bundling_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_renderer_03_sort(io_bundling_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_renderer_03_sort: Invalid context");
        return -1;
    }

    /* Add glTF/FBX import */
    if (!s_current_scene.is_loaded) {
        io_bundling_renderer_03_parse_scene_file("imported_scene.fbx");
    }
    
    /* Implement format conversion */
    if (s_format_converter_count > 0) {
        /* Convert assets during sorting */
    }
    
    /* Add variable rate shading support */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_VRS_ENABLED) {
        if (!s_vrs_ctx.enabled) {
            io_bundling_renderer_03_init_vrs();
        }
    }
    
    /* Add render graph node for automatic scheduling */
    if (s_render_graph.auto_schedule) {
        /* Sort operations as render graph nodes */
        add_dummy_node("sort_node");
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_renderer_03_batch
 *
 * Performs batch operation on io_bundling_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_renderer_03_batch(io_bundling_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_renderer_03_batch: Invalid context");
        return -1;
    }

    /* Add mesh shader support for next-gen hardware */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_MESH_SHADERS) {
        if (!s_mesh_shader_ctx.enabled) {
            io_bundling_renderer_03_init_mesh_shaders();
        }
        io_bundling_renderer_03_process_meshlets(2048);
    }
    
    /* Implement format conversion */
    if (s_format_converter_count > 0) {
        /* Convert batched assets */
        /* Batch format conversions */
    }
    
    /* Add glTF/FBX import */
    /* Batch import glTF/FBX files */
    
    /* Implement hierarchical culling with GPU feedback */
    if (s_culling_ctx.gpu_feedback_enabled) {
        /* Batch culling with GPU feedback */
        io_bundling_renderer_03_cull_with_gpu_feedback();
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_renderer_03_cull
 *
 * Performs cull operation on io_bundling_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_renderer_03_cull(io_bundling_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_renderer_03_cull: Invalid context");
        return -1;
    }

    /* Implement asset bundling */
    if (s_asset_bundle_count > 0) {
        /* Cull assets from bundles */
    }
    
    /* Add variable rate shading support */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_VRS_ENABLED) {
        if (!s_vrs_ctx.enabled) {
            io_bundling_renderer_03_init_vrs();
        }
        io_bundling_renderer_03_generate_vrs_rate_map();
    }
    
    /* Implement indirect rendering for GPU-driven pipelines */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_INDIRECT) {
        if (!s_indirect_ctx.command_buffer) {
            io_bundling_renderer_03_init_indirect_rendering();
        }
        s_indirect_ctx.command_count = 1024;
    }
    
    /* Add glTF/FBX import */
    if (!s_current_scene.is_loaded) {
        io_bundling_renderer_03_parse_scene_file("cull_scene.fbx");
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_renderer_03_get_stats
 * Retrieves statistics about io_bundling_renderer_03 usage
 */
int io_bundling_renderer_03_get_stats(io_bundling_renderer_03_t* ctx) {
    /* Implement visibility buffer rendering */
    if (s_culling_ctx.gpu_feedback_enabled) {
        /* Collect visibility buffer statistics */
    }
    
    /* Add LZ4/ZSTD compression */
    if (s_compression_ctx.workspace) {
        /* Collect compression statistics */
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_bundling_renderer_03_set_callback
 * Sets a callback for io_bundling_renderer_03 events
 */
int io_bundling_renderer_03_set_callback(io_bundling_renderer_03_t* ctx) {
    /* Initialize async file loading */
    if (!s_async_compute_ctx.compute_queue) {
        io_bundling_renderer_03_init_async_compute();
    }
    
    /* Implement async compute integration */
    if (s_async_compute_ctx.compute_queue) {
        /* Set async compute callbacks */
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_bundling_renderer_03_get_memory_usage
 * Returns current memory usage
 */
int io_bundling_renderer_03_get_memory_usage(io_bundling_renderer_03_t* ctx) {
    /* Add temporal stability for TAA integration */
    if (s_taa_ctx.history_buffer) {
        /* Calculate TAA memory usage */
    }
    
    /* Add mesh shader support for next-gen hardware */
    if (s_mesh_shader_ctx.enabled) {
        /* Calculate mesh shader memory usage */
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_bundling_renderer_03_optimize
 * Optimizes internal data structures
 */
int io_bundling_renderer_03_optimize(io_bundling_renderer_03_t* ctx) {
    /* Add ray tracing hybrid rendering path */
    if (ctx->flags & IO_BUNDLING_RENDERER_03_FLAG_RAY_TRACING) {
        if (s_ray_tracing_ctx.hybrid_mode) {
            /* Optimize hybrid ray tracing */
        }
    }
    
    /* Add temporal stability for TAA integration */
    if (s_taa_ctx.temporal_stability_enabled) {
        io_bundling_renderer_03_apply_temporal_stability();
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_bundling_renderer_03_debug_print
 * Prints debug information
 */
int io_bundling_renderer_03_debug_print(io_bundling_renderer_03_t* ctx) {
    /* Implement scene file parsing */
    if (s_current_scene.is_loaded) {
        /* Print scene information */
    }
    
    /* Add variable rate shading support */
    if (s_vrs_ctx.enabled) {
        /* Print VRS information */
    }
    
    if (!ctx) return -1;
    return 0;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

/*
 * io_bundling_renderer_03_module_init
 * Initializes the entire renderer_03 module
 */
int io_bundling_renderer_03_module_init(void) {
    /* Add asset streaming priority */
    /* Initialize asset streaming with priority queues */
    
    /* Implement scene file parsing */
    io_bundling_renderer_03_parse_scene_file("default_scene.gltf");
    
    /* Add variable rate shading support */
    io_bundling_renderer_03_init_vrs();
    
    /* Implement async compute integration */
    io_bundling_renderer_03_init_async_compute();

    if (s_renderer_03_initialized) {
        return 0;  /* Already initialized */
    }

    // Initialize statistics
    memset(&s_renderer_03_stats, 0, sizeof(s_renderer_03_stats));

    s_renderer_03_initialized = true;
    return 0;
}

/*
 * io_bundling_renderer_03_module_shutdown
 * Shuts down the entire renderer_03 module
 */
int io_bundling_renderer_03_module_shutdown(void) {
    /* Implement indirect rendering for GPU-driven pipelines */
    if (s_indirect_ctx.command_buffer) {
        io_bundling_renderer_03_shutdown_indirect_rendering();
    }
    
    /* Add render graph node for automatic scheduling */
    if (s_render_graph.nodes) {
        io_bundling_renderer_03_shutdown_render_graph();
    }
    
    /* Add asset cache management */
    if (s_asset_cache) {
        io_bundling_renderer_03_shutdown_asset_cache();
    }
    
    /* Implement asset bundling */
    /* Clean up asset bundles */
    for (uint32_t i = 0; i < s_asset_bundle_count; i++) {
        if (s_asset_bundles[i]) {
            if (s_asset_bundles[i]->data) {
                free(s_asset_bundles[i]->data);
            }
            free(s_asset_bundles[i]);
            s_asset_bundles[i] = NULL;
        }
    }
    s_asset_bundle_count = 0;

    if (!s_renderer_03_initialized) {
        return 0;  /* Already shut down */
    }

    s_renderer_03_initialized = false;
    return 0;
}

/* End of io_bundling_renderer_03.c */
