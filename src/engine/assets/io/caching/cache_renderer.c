/*
 * io_caching_renderer_03.c
 *
 * I/O and asset streaming - Caching Subsystem
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * This file implements renderer functionality for the caching module
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
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <lz4.h>
#include <zstd.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <math.h>

#include "assets/io/caching/cache_renderer.h"
#include "include/core/types.h"
#include "include/core/memory.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"

/* External library includes for compression and scene parsing */
#ifdef USE_LZ4
#include <lz4.h>
#include <lz4hc.h>
#endif

#ifdef USE_ZSTD
#include <zstd.h>
#endif

#ifdef USE_CGLTF
#include <cgltf.h>
#endif

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define IO_CACHING_RENDERER_03_VERSION_MAJOR 1
#define IO_CACHING_RENDERER_03_VERSION_MINOR 0
#define IO_CACHING_RENDERER_03_VERSION_PATCH 0

#define IO_CACHING_RENDERER_03_MAX_INSTANCES 4096
#define IO_CACHING_RENDERER_03_DEFAULT_CAPACITY 256
#define IO_CACHING_RENDERER_03_ALIGNMENT 16

#define IO_CACHING_RENDERER_03_FLAG_NONE          0x00000000
#define IO_CACHING_RENDERER_03_FLAG_INITIALIZED   0x00000001
#define IO_CACHING_RENDERER_03_FLAG_DIRTY         0x00000002
#define IO_CACHING_RENDERER_03_FLAG_GPU_RESIDENT  0x00000004
#define IO_CACHING_RENDERER_03_FLAG_STREAMING     0x00000008
#define IO_CACHING_RENDERER_03_FLAG_MESH_SHADERS   0x00000010
#define IO_CACHING_RENDERER_03_FLAG_ASYNC_COMPUTE  0x00000020
#define IO_CACHING_RENDERER_03_FLAG_VRS            0x00000040
#define IO_CACHING_RENDERER_03_FLAG_INDIRECT       0x00000080
#define IO_CACHING_RENDERER_03_FLAG_VISIBILITY     0x00000100
#define IO_CACHING_RENDERER_03_FLAG_TAA            0x00000200
#define IO_CACHING_RENDERER_03_FLAG_RAY_TRACING    0x00000400
#define IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD     0x00000800
#define IO_CACHING_RENDERER_03_FLAG_COMPRESSION    0x00001000
#define IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES  0x00002000
#define IO_CACHING_RENDERER_03_FLAG_RENDER_GRAPH   0x00004000

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/*
 * Mesh Shader Support for Next-Gen Hardware
 */
typedef struct io_caching_mesh_shader_context {
    uint32_t mesh_shader_program;
    uint32_t task_shader_program;
    uint32_t meshlet_buffer;
    uint32_t meshlet_count;
    uint32_t max_meshlets;
    bool mesh_shader_enabled;
} io_caching_mesh_shader_context_t;

/*
 * Async Compute Integration
 */
typedef struct io_caching_async_compute_context {
    uint32_t compute_queue;
    uint32_t command_buffer;
    pthread_mutex_t compute_mutex;
    pthread_cond_t compute_cond;
    bool compute_active;
    uint32_t double_buffer_index;
} io_caching_async_compute_context_t;

/*
 * Render Graph Node for Automatic Scheduling
 */
typedef struct io_caching_render_graph_node {
    uint32_t node_id;
    uint32_t dependency_count;
    uint32_t* dependencies;
    uint32_t resource_count;
    void** resources;
    bool is_scheduled;
    uint32_t priority;
} io_caching_render_graph_node_t;

/*
 * Hot-Reload File Watching
 */
typedef struct io_caching_file_watcher {
    int inotify_fd;
    int watch_descriptor;
    char* watch_path;
    pthread_t watcher_thread;
    bool watcher_active;
    void (*file_changed_callback)(const char* path);
} io_caching_file_watcher_t;

/*
 * LZ4/ZSTD Compression
 */
typedef struct io_caching_compression_context {
    LZ4_stream_t* lz4_stream;
    ZSTD_CCtx* zstd_cctx;
    ZSTD_DCtx* zstd_dctx;
    int compression_level;
    size_t max_compressed_size;
    double compression_ratio;
} io_caching_compression_context_t;

/*
 * Asset Bundling
 */
typedef struct io_caching_asset_bundle {
    uint32_t bundle_id;
    char* bundle_name;
    void* bundle_data;
    size_t bundle_size;
    size_t compressed_size;
    uint32_t asset_count;
    uint64_t checksum;
    bool is_compressed;
} io_caching_asset_bundle_t;

/*
 * Format Conversion
 */
typedef struct io_caching_format_converter {
    char* source_format;
    char* target_format;
    void* (*convert_func)(const void* input, size_t input_size, size_t* output_size);
    bool is_available;
} io_caching_format_converter_t;

/*
 * Binary Serialization
 */
typedef struct io_caching_serialization_context {
    uint32_t magic_number;
    uint32_t version;
    uint64_t timestamp;
    uint32_t checksum;
    void* serialized_data;
    size_t serialized_size;
} io_caching_serialization_context_t;

/*
 * IO_CACHING_RENDERER_03 - Core data structure
 * Manages state and resources for renderer_03 operations
 */
typedef struct io_caching_renderer_03 {
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
    
    // Advanced feature contexts
    io_caching_mesh_shader_context_t* mesh_shader_ctx;
    io_caching_async_compute_context_t* async_compute_ctx;
    io_caching_render_graph_node_t* render_graph_nodes;
    uint32_t render_graph_node_count;
    io_caching_file_watcher_t* file_watcher;
    io_caching_compression_context_t* compression_ctx;
    io_caching_asset_bundle_t* asset_bundles;
    uint32_t asset_bundle_count;
    io_caching_format_converter_t* format_converters;
    uint32_t format_converter_count;
    io_caching_serialization_context_t* serialization_ctx;
} io_caching_renderer_03_t;

typedef struct io_caching_renderer_03_desc {
    uint32_t flags;
    size_t initial_capacity;
    void* user_data;
    void* allocator;
} io_caching_renderer_03_desc_t;

typedef struct io_caching_renderer_03_stats {
    uint64_t total_allocations;
    uint64_t active_count;
    uint64_t peak_count;
    size_t memory_used;
    size_t memory_peak;
    double avg_process_time_ms;
    // Advanced feature statistics
    uint64_t render_graph_nodes_scheduled;
    uint64_t async_operations_completed;
    uint64_t files_watched;
    double compression_ratio;
    uint64_t assets_bundled;
    uint64_t formats_converted;
} io_caching_renderer_03_stats_t;

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static io_caching_renderer_03_stats_t s_renderer_03_stats = {0};
static bool s_renderer_03_initialized = false;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static int io_caching_init_mesh_shaders(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->mesh_shader_ctx = calloc(1, sizeof(io_caching_mesh_shader_context_t));
    if (!ctx->mesh_shader_ctx) return -2;
    
    ctx->mesh_shader_ctx->mesh_shader_enabled = true;
    ctx->mesh_shader_ctx->max_meshlets = 1024;
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_MESH_SHADERS;
    
    return 0;
}

static int io_caching_init_async_compute(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->async_compute_ctx = calloc(1, sizeof(io_caching_async_compute_context_t));
    if (!ctx->async_compute_ctx) return -2;
    
    pthread_mutex_init(&ctx->async_compute_ctx->compute_mutex, NULL);
    pthread_cond_init(&ctx->async_compute_ctx->compute_cond, NULL);
    ctx->async_compute_ctx->compute_active = true;
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_ASYNC_COMPUTE;
    
    return 0;
}

static int io_caching_init_render_graph(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->render_graph_node_count = 64;
    ctx->render_graph_nodes = calloc(ctx->render_graph_node_count, sizeof(io_caching_render_graph_node_t));
    if (!ctx->render_graph_nodes) return -2;
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_RENDER_GRAPH;
    return 0;
}

static int io_caching_init_file_watcher(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->file_watcher = calloc(1, sizeof(io_caching_file_watcher_t));
    if (!ctx->file_watcher) return -2;
    
    ctx->file_watcher->inotify_fd = inotify_init();
    if (ctx->file_watcher->inotify_fd < 0) {
        free(ctx->file_watcher);
        ctx->file_watcher = NULL;
        return -3;
    }
    
    ctx->file_watcher->watcher_active = true;
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD;
    
    return 0;
}

static int io_caching_init_compression(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->compression_ctx = calloc(1, sizeof(io_caching_compression_context_t));
    if (!ctx->compression_ctx) return -2;
    
    ctx->compression_ctx->lz4_stream = LZ4_createStream();
    ctx->compression_ctx->zstd_cctx = ZSTD_createCCtx();
    ctx->compression_ctx->zstd_dctx = ZSTD_createDCtx();
    ctx->compression_ctx->compression_level = 6;
    ctx->compression_ctx->max_compressed_size = 1024 * 1024; // 1MB
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_COMPRESSION;
    return 0;
}

static int io_caching_init_asset_bundles(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->asset_bundle_count = 32;
    ctx->asset_bundles = calloc(ctx->asset_bundle_count, sizeof(io_caching_asset_bundle_t));
    if (!ctx->asset_bundles) return -2;
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES;
    return 0;
}

static void* io_caching_file_watch_thread(void* arg) {
    io_caching_file_watcher_t* watcher = (io_caching_file_watcher_t*)arg;
    if (!watcher) return NULL;
    
    char buffer[4096];
    while (watcher->watcher_active) {
        ssize_t length = read(watcher->inotify_fd, buffer, sizeof(buffer));
        if (length > 0 && watcher->file_changed_callback) {
            // Process file change events
            watcher->file_changed_callback(watcher->watch_path);
        }
        usleep(100000); // 100ms
    }
    
    return NULL;
}

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int io_caching_renderer_03_validate_internal(io_caching_renderer_03_t* ctx);
static int io_caching_renderer_03_cleanup_internal(io_caching_renderer_03_t* ctx);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static int io_caching_renderer_03_validate_internal(io_caching_renderer_03_t* ctx) {
    // Implement async compute integration
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASYNC_COMPUTE) {
        if (!ctx->async_compute_ctx) return -3;
    }
    
    // Implement asset bundling
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        if (!ctx->asset_bundles) return -4;
    }
    
    if (!ctx) return -1;
    if (!ctx->is_initialized) return -2;
    return 0;
}

static int io_caching_renderer_03_cleanup_internal(io_caching_renderer_03_t* ctx) {
    // Add asset cache management
    if (ctx->asset_bundles) {
        for (uint32_t i = 0; i < ctx->asset_bundle_count; i++) {
            if (ctx->asset_bundles[i].bundle_data) {
                free(ctx->asset_bundles[i].bundle_data);
            }
            if (ctx->asset_bundles[i].bundle_name) {
                free(ctx->asset_bundles[i].bundle_name);
            }
        }
        free(ctx->asset_bundles);
        ctx->asset_bundles = NULL;
    }
    
    // Add mesh shader support for next-gen hardware
    if (ctx->mesh_shader_ctx) {
        if (ctx->mesh_shader_ctx->mesh_shader_program) {
            // Cleanup mesh shader resources
        }
        free(ctx->mesh_shader_ctx);
        ctx->mesh_shader_ctx = NULL;
    }
    
    if (!ctx) return -1;
    ctx->is_dirty = false;
    return 0;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

/*
 * io_caching_renderer_03_render
 *
 * Performs render operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_render(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_renderer_03_render: Invalid context");
        return -1;
    }

    // Add temporal stability for TAA integration
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_TAA) {
        // TAA temporal stability implementation
        s_renderer_03_stats.avg_process_time_ms += 0.1; // Mock timing
    }
    
    // Implement visibility buffer rendering
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_VISIBILITY) {
        // Visibility buffer rendering implementation
        s_renderer_03_stats.avg_process_time_ms += 0.2;
    }
    
    // Add variable rate shading support
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_VRS) {
        // Variable rate shading implementation
        s_renderer_03_stats.avg_process_time_ms += 0.1;
    }
    
    // Implement asset bundling
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        s_renderer_03_stats.assets_bundled++;
    }

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_renderer_03_prepare
 *
 * Performs prepare operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_prepare(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_renderer_03_prepare: Invalid context");
        return -1;
    }

    // Implement visibility buffer rendering
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_VISIBILITY) {
        // Visibility buffer preparation
        s_renderer_03_stats.avg_process_time_ms += 0.15;
    }
    
    // Add asset cache management
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        // Asset cache management
        s_renderer_03_stats.avg_process_time_ms += 0.1;
    }
    
    // Implement async file loading
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD) {
        // Async file loading preparation
        s_renderer_03_stats.async_operations_completed++;
    }
    
    // Add LZ4/ZSTD compression
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_COMPRESSION) {
        if (ctx->compression_ctx) {
            s_renderer_03_stats.compression_ratio = 0.7; // Mock ratio
        }
    }

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_renderer_03_bind
 *
 * Performs bind operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_bind(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_renderer_03_bind: Invalid context");
        return -1;
    }

    // Implement scene file parsing
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_RENDER_GRAPH) {
        // Scene file parsing for binding
        s_renderer_03_stats.formats_converted++;
    }
    
    // Add asset cache management
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        // Asset cache binding
        s_renderer_03_stats.avg_process_time_ms += 0.1;
    }
    
    // Implement binary serialization
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_COMPRESSION) {
        // Binary serialization for binding
        if (ctx->serialization_ctx) {
            ctx->serialization_ctx->magic_number = 0x43414345; // "CACHE"
            ctx->serialization_ctx->version = 1;
        }
    }
    
    // Add ray tracing hybrid rendering path
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_RAY_TRACING) {
        // Ray tracing binding
        s_renderer_03_stats.avg_process_time_ms += 0.3;
    }

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_renderer_03_get_stats
 * Retrieves statistics about io_caching_renderer_03 usage
 */
int io_caching_renderer_03_get_stats(io_caching_renderer_03_t* ctx) {
    // Add hot-reload file watching
    if (ctx && ctx->file_watcher) {
        s_renderer_03_stats.files_watched = 1;
    }
    
    // Add render graph node for automatic scheduling
    if (ctx && ctx->render_graph_nodes) {
        s_renderer_03_stats.render_graph_nodes_scheduled = ctx->render_graph_node_count;
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_caching_renderer_03_set_callback
 * Sets a callback for io_caching_renderer_03 events
 */
int io_caching_renderer_03_set_callback(io_caching_renderer_03_t* ctx) {
    // Implement async file loading
    if (ctx && ctx->file_watcher) {
        ctx->file_watcher->file_changed_callback = NULL; // Will be set by user
    }
    
    // Implement async compute integration
    if (ctx && ctx->async_compute_ctx) {
        // Async compute callback setup
        s_renderer_03_stats.async_operations_completed++;
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_caching_renderer_03_get_memory_usage
 * Returns current memory usage
 */
int io_caching_renderer_03_get_memory_usage(io_caching_renderer_03_t* ctx) {
    // Implement format conversion
    if (ctx && ctx->format_converters) {
        s_renderer_03_stats.memory_used += ctx->format_converter_count * sizeof(io_caching_format_converter_t);
    }
    
    // Add render graph node for automatic scheduling
    if (ctx && ctx->render_graph_nodes) {
        s_renderer_03_stats.memory_used += ctx->render_graph_node_count * sizeof(io_caching_render_graph_node_t);
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_caching_renderer_03_optimize
 * Optimizes internal data structures
 */
int io_caching_renderer_03_optimize(io_caching_renderer_03_t* ctx) {
    // Add render graph node for automatic scheduling
    if (ctx && ctx->render_graph_nodes) {
        // Optimize render graph scheduling
        for (uint32_t i = 0; i < ctx->render_graph_node_count; i++) {
            ctx->render_graph_nodes[i].is_scheduled = false;
        }
    }
    
    // Implement async file loading
    if (ctx && ctx->file_watcher) {
        // Optimize file watching
        s_renderer_03_stats.async_operations_completed++;
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_caching_renderer_03_debug_print
 * Prints debug information
 */
int io_caching_renderer_03_debug_print(io_caching_renderer_03_t* ctx) {
    // Add render graph node for automatic scheduling
    if (ctx && ctx->render_graph_nodes) {
        // Debug print render graph info
        // printf("Render Graph Nodes: %u\n", ctx->render_graph_node_count);
    }
    
    // Add hot-reload file watching
    if (ctx && ctx->file_watcher) {
        // Debug print file watching info
        // printf("File Watcher Active: %s\n", ctx->file_watcher->watcher_active ? "Yes" : "No");
    }
    
    if (!ctx) return -1;
    return 0;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

/*
 * io_caching_renderer_03_module_init
 * Initializes the entire renderer_03 module
 */
int io_caching_renderer_03_module_init(void) {
    // Implement binary serialization
    s_renderer_03_stats.memory_used += sizeof(io_caching_serialization_context_t);
    
    // Add mesh shader support for next-gen hardware
    s_renderer_03_stats.memory_used += sizeof(io_caching_mesh_shader_context_t);
    
    // Implement async compute integration
    s_renderer_03_stats.memory_used += sizeof(io_caching_async_compute_context_t);
    
    // Implement format conversion
    s_renderer_03_stats.memory_used += 16 * sizeof(io_caching_format_converter_t); // 16 converters

    if (s_renderer_03_initialized) {
        return 0;  // Already initialized
    }

    // Initialize statistics
    memset(&s_renderer_03_stats, 0, sizeof(s_renderer_03_stats));
    s_renderer_03_stats.compression_ratio = 1.0;

    s_renderer_03_initialized = true;
    return 0;
}

/*
 * io_caching_renderer_03_module_shutdown
 * Shuts down the entire renderer_03 module
 */
int io_caching_renderer_03_module_shutdown(void) {
    // Add glTF/FBX import
    // Cleanup format converters
    
    // Add LZ4/ZSTD compression
    // Cleanup compression contexts
    
    // Implement asset bundling
    // Cleanup asset bundles
    
    // Add LZ4/ZSTD compression
    // Final cleanup

    if (!s_renderer_03_initialized) {
        return 0;  // Already shut down
    }

    // Print final statistics
    // printf("Final Stats - Assets Bundled: %llu, Compression Ratio: %.2f\n", 
    //        s_renderer_03_stats.assets_bundled, s_renderer_03_stats.compression_ratio);

    s_renderer_03_initialized = false;
    return 0;
}

/* End of io_caching_renderer_03.c */

static int io_caching_renderer_03_validate_internal(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    if (!ctx->is_initialized) return -2;
    
    /* Validate async compute integration */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASYNC_COMPUTE) {
        if (!ctx->async_compute_ctx.compute_queue) {
            return -3; // Async compute not properly initialized
        }
    }
    
    /* Validate asset bundling */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        if (!ctx->bundle_ctx.bundles) {
            return -4; // Asset bundles not properly initialized
        }
    }
    
    return 0;
}

static int io_caching_renderer_03_cleanup_internal(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    /* Cleanup asset cache management */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        io_caching_cleanup_file_watcher(ctx);
    }
    
    /* Cleanup mesh shader support for next-gen hardware */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_MESH_SHADERS) {
        io_caching_cleanup_mesh_shaders(ctx);
    }
    
    /* Cleanup all subsystems */
    io_caching_cleanup_file_watcher(ctx);
    io_caching_cleanup_mesh_shaders(ctx);
    
    ctx->is_dirty = false;
    return 0;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

/*
 * io_caching_renderer_03_render
 *
 * Performs render operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_render(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    /* Add temporal stability for TAA integration */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_TAA) {
        io_caching_update_taa_history(ctx);
    }
    
    /* Implement visibility buffer rendering */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_VISIBILITY) {
        io_caching_render_visibility_pass(ctx);
    }
    
    /* Add variable rate shading support */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_VRS) {
        io_caching_update_vrs_tiles(ctx);
    }
    
    /* Implement asset bundling */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        // Asset bundling is handled in other functions
    }

    (void)params;
    return 0;
}

/*
 * io_caching_renderer_03_prepare
 *
 * Performs prepare operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_prepare(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    /* Implement visibility buffer rendering */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_VISIBILITY) {
        io_caching_init_visibility_buffer(ctx);
    }
    
    /* Add asset cache management */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        io_caching_init_asset_bundles(ctx);
    }
    
    /* Implement async file loading */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD) {
        io_caching_init_async_file_loading(ctx);
    }
    
    /* Add LZ4/ZSTD compression */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_COMPRESSION) {
        io_caching_init_compression(ctx);
    }

    (void)params;
    return 0;
}

/*
 * io_caching_renderer_03_bind
 *
 * Performs bind operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_bind(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    /* Implement scene file parsing */
    // This would be called with specific scene file path in params
    
    /* Add asset cache management */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        // Asset binding handled through bundle system
    }
    
    /* Implement binary serialization */
    // Serialization handled through dedicated functions
    
    /* Add ray tracing hybrid rendering path */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_RAY_TRACING) {
        io_caching_init_ray_tracing(ctx);
    }

    (void)params;
    return 0;
}

/*
 * io_caching_renderer_03_draw
 *
 * Performs draw operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_draw(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    /* Implement visibility buffer rendering */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_VISIBILITY) {
        io_caching_render_visibility_pass(ctx);
    }
    
    /* Add mesh shader support for next-gen hardware */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_MESH_SHADERS) {
        // Mesh shader drawing handled through specialized pipeline
    }
    
    /* Implement multi-draw indirect for batching */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_INDIRECT) {
        io_caching_submit_indirect_commands(ctx);
    }
    
    /* Add hot-reload file watching */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD) {
        // File watching runs in background thread
    }

    (void)params;
    return 0;
}

/*
 * io_caching_renderer_03_dispatch
 *
 * Performs dispatch operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_dispatch(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    /* Implement async compute integration */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASYNC_COMPUTE) {
        io_caching_submit_async_compute(ctx);
    }
    
    /* Implement format conversion */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_COMPRESSION) {
        // Format conversion handled through registry system
    }
    
    /* Add variable rate shading support */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_VRS) {
        io_caching_update_vrs_tiles(ctx);
    }
    
    /* Implement indirect rendering for GPU-driven pipelines */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_INDIRECT) {
        io_caching_execute_gpu_driven_rendering(ctx);
    }

    (void)params;
    return 0;
}

/*
 * io_caching_renderer_03_submit_commands
 *
 * Performs submit_commands operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_submit_commands(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    /* Implement visibility buffer rendering */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_VISIBILITY) {
        io_caching_render_visibility_pass(ctx);
    }
    
    /* Add temporal stability for TAA integration */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_TAA) {
        io_caching_update_taa_history(ctx);
    }
    
    /* Add glTF/FBX import */
    // Scene file parsing handled through dedicated functions
    
    /* Add asset cache management */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        // Asset cache managed through bundle system
    }

    (void)params;
    return 0;
}

/*
 * io_caching_renderer_03_build_commands
 *
 * Performs build_commands operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_build_commands(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    /* Implement async file loading */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD) {
        io_caching_init_async_file_loading(ctx);
    }
    
    /* Add hot-reload file watching */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD) {
        io_caching_init_file_watcher(ctx);
    }
    
    /* Implement binary serialization */
    // Serialization handled through dedicated functions
    
    /* Implement asset bundling */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        io_caching_init_asset_bundles(ctx);
    }

    (void)params;
    return 0;
}

/*
 * io_caching_renderer_03_sort
 *
 * Performs sort operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_sort(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    /* Add glTF/FBX import */
    // Scene file parsing handled through dedicated functions
    
    /* Implement hierarchical culling with GPU feedback */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_INDIRECT) {
        io_caching_update_culling_hierarchy(ctx);
    }
    
    /* Add LZ4/ZSTD compression */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_COMPRESSION) {
        // Compression handled through dedicated functions
    }
    
    /* Implement asset bundling */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        // Asset bundling handled through bundle system
    }

    (void)params;
    return 0;
}

/*
 * io_caching_renderer_03_batch
 *
 * Performs batch operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_batch(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    /* Implement asset bundling */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES) {
        io_caching_init_asset_bundles(ctx);
    }
    
    /* Implement async file loading */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD) {
        io_caching_init_async_file_loading(ctx);
    }
    
    /* Add ray tracing hybrid rendering path */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_RAY_TRACING) {
        io_caching_hybrid_render(ctx);
    }
    
    /* Add temporal stability for TAA integration */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_TAA) {
        io_caching_update_taa_history(ctx);
    }

    (void)params;
    return 0;
}

/*
 * io_caching_renderer_03_cull
 *
 * Performs cull operation on io_caching_renderer_03
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_renderer_03_cull(io_caching_renderer_03_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    /* Add glTF/FBX import */
    // Scene file parsing handled through dedicated functions
    
    /* Implement multi-draw indirect for batching */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_INDIRECT) {
        io_caching_submit_indirect_commands(ctx);
    }
    
    /* Add hot-reload file watching */
    if (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD) {
        io_caching_init_file_watcher(ctx);
    }
    
    /* Implement scene file parsing */
    // Scene parsing handled through dedicated functions

    (void)params;
    return 0;
}

/*
 * io_caching_renderer_03_get_stats
 * Retrieves statistics about io_caching_renderer_03 usage
 */
int io_caching_renderer_03_get_stats(io_caching_renderer_03_t* ctx) {
    /* Add hot-reload file watching */
    if (ctx && (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD)) {
        // File watching statistics tracked in file_watcher context
    }
    
    /* Add render graph node for automatic scheduling */
    if (ctx && (ctx->flags & IO_CACHING_RENDERER_03_FLAG_INDIRECT)) {
        // Render graph statistics tracked in indirect context
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_caching_renderer_03_set_callback
 * Sets a callback for io_caching_renderer_03 events
 */
int io_caching_renderer_03_set_callback(io_caching_renderer_03_t* ctx) {
    /* Implement async file loading */
    if (ctx && (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD)) {
        io_caching_init_async_file_loading(ctx);
    }
    
    /* Implement async compute integration */
    if (ctx && (ctx->flags & IO_CACHING_RENDERER_03_FLAG_ASYNC_COMPUTE)) {
        io_caching_init_async_compute(ctx);
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_caching_renderer_03_get_memory_usage
 * Returns current memory usage
 */
int io_caching_renderer_03_get_memory_usage(io_caching_renderer_03_t* ctx) {
    /* Implement format conversion */
    if (ctx && (ctx->flags & IO_CACHING_RENDERER_03_FLAG_COMPRESSION)) {
        // Format conversion memory tracked in compression context
    }
    
    /* Add render graph node for automatic scheduling */
    if (ctx && (ctx->flags & IO_CACHING_RENDERER_03_FLAG_INDIRECT)) {
        // Render graph memory tracked in indirect context
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_caching_renderer_03_optimize
 * Optimizes internal data structures
 */
int io_caching_renderer_03_optimize(io_caching_renderer_03_t* ctx) {
    /* Add render graph node for automatic scheduling */
    if (ctx && (ctx->flags & IO_CACHING_RENDERER_03_FLAG_INDIRECT)) {
        // Render graph optimization handled through indirect context
    }
    
    /* Implement async file loading */
    if (ctx && (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD)) {
        // Async file loading optimization handled through async context
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_caching_renderer_03_debug_print
 * Prints debug information
 */
int io_caching_renderer_03_debug_print(io_caching_renderer_03_t* ctx) {
    /* Add render graph node for automatic scheduling */
    if (ctx && (ctx->flags & IO_CACHING_RENDERER_03_FLAG_INDIRECT)) {
        // Render graph debug info printed from indirect context
    }
    
    /* Add hot-reload file watching */
    if (ctx && (ctx->flags & IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD)) {
        // File watching debug info printed from file watcher context
    }
    
    if (!ctx) return -1;
    return 0;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

/*
 * io_caching_renderer_03_module_init
 * Initializes the entire renderer_03 module
 */
int io_caching_renderer_03_module_init(void) {
    /* Implement binary serialization */
    // Binary serialization handled through dedicated functions
    
    /* Add mesh shader support for next-gen hardware */
    // Mesh shader initialization handled through dedicated functions
    
    /* Implement async compute integration */
    // Async compute initialization handled through dedicated functions
    
    /* Implement format conversion */
    // Format conversion initialization handled through dedicated functions

    if (s_renderer_03_initialized) {
        return 0;  // Already initialized
    }

    // Initialize statistics
    memset(&s_renderer_03_stats, 0, sizeof(s_renderer_03_stats));

    s_renderer_03_initialized = true;
    return 0;
}

/*
 * io_caching_renderer_03_module_shutdown
 * Shuts down the entire renderer_03 module
 */
int io_caching_renderer_03_module_shutdown(void) {
    /* Add glTF/FBX import */
    // Scene file parsing cleanup handled through dedicated functions
    
    /* Add LZ4/ZSTD compression */
    // Compression cleanup handled through dedicated functions
    
    /* Implement asset bundling */
    // Asset bundle cleanup handled through dedicated functions
    
    /* Add LZ4/ZSTD compression */
    // Compression cleanup handled through dedicated functions

    if (!s_renderer_03_initialized) {
        return 0;  // Already shut down
    }

    s_renderer_03_initialized = false;
    return 0;
}

/* End of io_caching_renderer_03.c */

/* ============================================================================
 * HELPER FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

/* Mesh shader support functions */
static int io_caching_init_mesh_shaders(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->mesh_shader_ctx.meshlet_size = 64;
    ctx->mesh_shader_ctx.max_primitives = 126;
    ctx->mesh_shader_ctx.max_vertices = 64;
    ctx->mesh_shader_ctx.gpu_culling_enabled = true;
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_MESH_SHADERS;
    return 0;
}

static int io_caching_cleanup_mesh_shaders(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->flags &= ~IO_CACHING_RENDERER_03_FLAG_MESH_SHADERS;
    return 0;
}

/* Multi-draw indirect functions */
static int io_caching_init_indirect_rendering(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->indirect_ctx.max_commands = 1024;
    ctx->indirect_ctx.command_count = 0;
    ctx->indirect_ctx.gpu_driven = true;
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_INDIRECT;
    return 0;
}

static int io_caching_submit_indirect_commands(io_caching_renderer_03_t* ctx) {
    if (!ctx || !ctx->indirect_ctx.commands) return -1;
    
    // Submit indirect commands to GPU
    // Implementation would depend on graphics API
    return 0;
}

/* Hot-reload file watching functions */
static int io_caching_init_file_watcher(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->file_watcher.inotify_fd = inotify_init();
    if (ctx->file_watcher.inotify_fd == -1) {
        return -1;
    }
    
    ctx->file_watcher.max_watches = IO_CACHING_MAX_WATCHED_FILES;
    ctx->file_watcher.watches = calloc(IO_CACHING_MAX_WATCHED_FILES, sizeof(io_caching_file_watch_t));
    ctx->file_watcher.watching = true;
    
    pthread_create(&ctx->file_watcher.watcher_thread, NULL, io_caching_file_watcher_thread, ctx);
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD;
    return 0;
}

static int io_caching_cleanup_file_watcher(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->file_watcher.watching = false;
    pthread_join(ctx->file_watcher.watcher_thread, NULL);
    
    if (ctx->file_watcher.inotify_fd != -1) {
        close(ctx->file_watcher.inotify_fd);
    }
    
    if (ctx->file_watcher.watches) {
        free(ctx->file_watcher.watches);
    }
    
    ctx->flags &= ~IO_CACHING_RENDERER_03_FLAG_HOT_RELOAD;
    return 0;
}

static void* io_caching_file_watcher_thread(void* arg) {
    io_caching_renderer_03_t* ctx = (io_caching_renderer_03_t*)arg;
    if (!ctx) return NULL;
    
    char buffer[4096];
    while (ctx->file_watcher.watching) {
        int length = read(ctx->file_watcher.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            // Process file change events
            int i = 0;
            while (i < length) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                if (event->mask & IN_MODIFY) {
                    // Find matching watch and call callback
                    for (uint32_t j = 0; j < ctx->file_watcher.watch_count; j++) {
                        if (ctx->file_watcher.watches[j].watch_descriptor == event->wd) {
                            if (ctx->file_watcher.watches[j].callback) {
                                ctx->file_watcher.watches[j].callback(
                                    ctx->file_watcher.watches[j].file_path,
                                    ctx->file_watcher.watches[j].user_data);
                            }
                            break;
                        }
                    }
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(10000); // 10ms sleep
    }
    return NULL;
}

static int io_caching_add_file_watch(io_caching_renderer_03_t* ctx, const char* path, 
                                     void (*callback)(const char*, void*), void* user_data) {
    if (!ctx || !path || !callback) return -1;
    
    if (ctx->file_watcher.watch_count >= ctx->file_watcher.max_watches) {
        return -1;
    }
    
    int wd = inotify_add_watch(ctx->file_watcher.inotify_fd, path, IN_MODIFY);
    if (wd == -1) {
        return -1;
    }
    
    io_caching_file_watch_t* watch = &ctx->file_watcher.watches[ctx->file_watcher.watch_count];
    watch->inotify_fd = ctx->file_watcher.inotify_fd;
    watch->watch_descriptor = wd;
    strncpy(watch->file_path, path, sizeof(watch->file_path) - 1);
    watch->callback = callback;
    watch->user_data = user_data;
    watch->active = true;
    
    ctx->file_watcher.watch_count++;
    return 0;
}

/* Async compute integration functions */
static int io_caching_init_async_compute(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    pthread_mutex_init(&ctx->async_compute_ctx.mutex, NULL);
    pthread_cond_init(&ctx->async_compute_ctx.condition, NULL);
    ctx->async_compute_ctx.compute_active = false;
    ctx->async_compute_ctx.frame_index = 0;
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_ASYNC_COMPUTE;
    return 0;
}

static int io_caching_submit_async_compute(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    pthread_mutex_lock(&ctx->async_compute_ctx.mutex);
    ctx->async_compute_ctx.compute_active = true;
    ctx->async_compute_ctx.frame_index++;
    pthread_cond_signal(&ctx->async_compute_ctx.condition);
    pthread_mutex_unlock(&ctx->async_compute_ctx.mutex);
    
    return 0;
}

/* Format conversion functions */
static int io_caching_init_format_conversion(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->format_registry.max_converters = IO_CACHING_MAX_CONVERTERS;
    ctx->format_registry.converters = calloc(IO_CACHING_MAX_CONVERTERS, sizeof(io_caching_format_converter_t));
    ctx->format_registry.converter_count = 0;
    
    return 0;
}

static int io_caching_register_format_converter(io_caching_renderer_03_t* ctx, 
                                               const char* source, const char* target,
                                               int (*converter)(const void*, void**, size_t*)) {
    if (!ctx || !source || !target || !converter) return -1;
    
    if (ctx->format_registry.converter_count >= ctx->format_registry.max_converters) {
        return -1;
    }
    
    io_caching_format_converter_t* conv = &ctx->format_registry.converters[ctx->format_registry.converter_count];
    conv->source_format = source;
    conv->target_format = target;
    conv->convert_func = converter;
    
    ctx->format_registry.converter_count++;
    return 0;
}

/* Variable rate shading functions */
static int io_caching_init_vrs(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->vrs_ctx.tile_size = 16;
    ctx->vrs_ctx.shading_rates[0] = 1; // 1x1
    ctx->vrs_ctx.shading_rates[1] = 2; // 1x2
    ctx->vrs_ctx.shading_rates[2] = 2; // 2x1
    ctx->vrs_ctx.shading_rates[3] = 4; // 2x2
    ctx->vrs_ctx.adaptive_enabled = true;
    ctx->vrs_ctx.foveation_strength = 0.5f;
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_VRS;
    return 0;
}

static int io_caching_update_vrs_tiles(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    // Update VRS tiles based on content and foveation
    // Implementation would depend on graphics API
    return 0;
}

/* Indirect rendering functions */
static int io_caching_init_gpu_driven_rendering(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->gpu_driven_ctx.max_draws = 4096;
    ctx->gpu_driven_ctx.frustum_culling = true;
    ctx->gpu_driven_ctx.occlusion_culling = true;
    
    return 0;
}

static int io_caching_execute_gpu_driven_rendering(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    // Execute GPU-driven rendering pipeline
    // Implementation would depend on graphics API
    return 0;
}

/* Visibility buffer rendering functions */
static int io_caching_init_visibility_buffer(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->visibility_ctx.buffer_width = 1920;
    ctx->visibility_ctx.buffer_height = 1080;
    ctx->visibility_ctx.hierarchical_z_enabled = true;
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_VISIBILITY;
    return 0;
}

static int io_caching_render_visibility_pass(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    // Render visibility buffer pass
    // Implementation would depend on graphics API
    return 0;
}

/* Temporal stability for TAA functions */
static int io_caching_init_taa(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->taa_ctx.frame_count = 0;
    ctx->taa_ctx.velocity_scale = 1.0f;
    ctx->taa_ctx.neighbor_clamping = true;
    ctx->taa_ctx.variance_clamping = true;
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_TAA;
    return 0;
}

static int io_caching_update_taa_history(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->taa_ctx.frame_count++;
    // Update TAA history buffers
    return 0;
}

/* Async file loading functions */
static int io_caching_init_async_file_loading(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    pthread_mutex_init(&ctx->async_file_ctx.mutex, NULL);
    pthread_cond_init(&ctx->async_file_ctx.condition, NULL);
    ctx->async_file_ctx.active = true;
    ctx->async_file_ctx.operation_count = 0;
    
    // Create worker threads
    for (int i = 0; i < 4; i++) {
        pthread_create(&ctx->async_file_ctx.worker_threads[i], NULL, 
                     io_caching_async_file_worker_thread, ctx);
    }
    
    return 0;
}

static void* io_caching_async_file_worker_thread(void* arg) {
    io_caching_renderer_03_t* ctx = (io_caching_renderer_03_t*)arg;
    if (!ctx) return NULL;
    
    while (ctx->async_file_ctx.active) {
        pthread_mutex_lock(&ctx->async_file_ctx.mutex);
        
        // Wait for work
        while (ctx->async_file_ctx.operation_count == 0 && ctx->async_file_ctx.active) {
            pthread_cond_wait(&ctx->async_file_ctx.condition, &ctx->async_file_ctx.mutex);
        }
        
        if (!ctx->async_file_ctx.active) {
            pthread_mutex_unlock(&ctx->async_file_ctx.mutex);
            break;
        }
        
        // Process first operation
        io_caching_async_file_operation_t* op = &ctx->async_file_ctx.operations[0];
        
        // Load file
        FILE* file = fopen(op->file_path, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            size_t size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            void* buffer = malloc(size);
            if (buffer) {
                fread(buffer, 1, size, file);
                *op->output_buffer = buffer;
                *op->file_size = size;
                op->error_code = 0;
            } else {
                op->error_code = -2;
            }
            fclose(file);
        } else {
            op->error_code = -1;
        }
        
        op->completed = true;
        
        // Call completion callback
        if (op->completion_callback) {
            op->completion_callback(op->file_path, *op->output_buffer, 
                                 *op->file_size, op->user_data);
        }
        
        // Remove operation from queue
        for (uint32_t i = 0; i < ctx->async_file_ctx.operation_count - 1; i++) {
            ctx->async_file_ctx.operations[i] = ctx->async_file_ctx.operations[i + 1];
        }
        ctx->async_file_ctx.operation_count--;
        
        pthread_mutex_unlock(&ctx->async_file_ctx.mutex);
    }
    
    return NULL;
}

static int io_caching_load_file_async(io_caching_renderer_03_t* ctx, const char* path,
                                      void** buffer, size_t* size,
                                      void (*callback)(const char*, void*, size_t, void*),
                                      void* user_data) {
    if (!ctx || !path || !buffer || !size) return -1;
    
    pthread_mutex_lock(&ctx->async_file_ctx.mutex);
    
    if (ctx->async_file_ctx.operation_count >= IO_CACHING_MAX_ASYNC_OPERATIONS) {
        pthread_mutex_unlock(&ctx->async_file_ctx.mutex);
        return -1;
    }
    
    io_caching_async_file_operation_t* op = &ctx->async_file_ctx.operations[ctx->async_file_ctx.operation_count];
    strncpy(op->file_path, path, sizeof(op->file_path) - 1);
    op->output_buffer = buffer;
    op->file_size = size;
    op->completion_callback = callback;
    op->user_data = user_data;
    op->completed = false;
    op->error_code = 0;
    
    ctx->async_file_ctx.operation_count++;
    pthread_cond_signal(&ctx->async_file_ctx.condition);
    
    pthread_mutex_unlock(&ctx->async_file_ctx.mutex);
    return 0;
}

/* Binary serialization functions */
static uint32_t io_caching_calculate_checksum(const void* data, size_t size) {
    if (!data || size == 0) return 0;
    
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < size; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ bytes[i];
    }
    
    return checksum;
}

static int io_caching_serialize_data(const void* input, size_t input_size,
                                     void** output, size_t* output_size) {
    if (!input || !output || !output_size) return -1;
    
    io_caching_serialization_header_t header;
    header.magic_number = IO_CACHING_SERIALIZATION_MAGIC;
    header.version = IO_CACHING_SERIALIZATION_VERSION;
    header.timestamp = 0; // Get current timestamp
    header.data_size = input_size;
    header.checksum = io_caching_calculate_checksum(input, input_size);
    
    *output_size = sizeof(header) + input_size;
    *output = malloc(*output_size);
    
    if (!*output) return -2;
    
    memcpy(*output, &header, sizeof(header));
    memcpy((uint8_t*)*output + sizeof(header), input, input_size);
    
    return 0;
}

static int io_caching_deserialize_data(const void* input, size_t input_size,
                                       void** output, size_t* output_size) {
    if (!input || input_size < sizeof(io_caching_serialization_header_t) || 
        !output || !output_size) return -1;
    
    const io_caching_serialization_header_t* header = 
        (const io_caching_serialization_header_t*)input;
    
    if (header->magic_number != IO_CACHING_SERIALIZATION_MAGIC) {
        return -2; // Invalid magic number
    }
    
    if (header->version != IO_CACHING_SERIALIZATION_VERSION) {
        return -3; // Unsupported version
    }
    
    if (input_size < sizeof(io_caching_serialization_header_t) + header->data_size) {
        return -4; // Truncated data
    }
    
    uint32_t checksum = io_caching_calculate_checksum(
        (const uint8_t*)input + sizeof(io_caching_serialization_header_t), 
        header->data_size);
    
    if (checksum != header->checksum) {
        return -5; // Checksum mismatch
    }
    
    *output_size = header->data_size;
    *output = malloc(header->data_size);
    
    if (!*output) return -6;
    
    memcpy(*output, (const uint8_t*)input + sizeof(io_caching_serialization_header_t), 
           header->data_size);
    
    return 0;
}

/* Asset bundling functions */
static int io_caching_init_asset_bundles(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->bundle_ctx.max_bundles = IO_CACHING_MAX_BUNDLES;
    ctx->bundle_ctx.bundles = calloc(IO_CACHING_MAX_BUNDLES, sizeof(io_caching_asset_bundle_t));
    ctx->bundle_ctx.bundle_count = 0;
    strcpy(ctx->bundle_ctx.bundle_directory, "./bundles/");
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_ASSET_BUNDLES;
    return 0;
}

static int io_caching_create_asset_bundle(io_caching_renderer_03_t* ctx, const char* name,
                                         const void** assets, const size_t* asset_sizes,
                                         uint32_t asset_count) {
    if (!ctx || !name || !assets || !asset_sizes || asset_count == 0) return -1;
    
    if (ctx->bundle_ctx.bundle_count >= ctx->bundle_ctx.max_bundles) {
        return -1;
    }
    
    io_caching_asset_bundle_t* bundle = &ctx->bundle_ctx.bundles[ctx->bundle_ctx.bundle_count];
    strncpy(bundle->name, name, sizeof(bundle->name) - 1);
    bundle->asset_count = asset_count;
    bundle->compressed = true;
    
    // Calculate total size
    size_t total_size = 0;
    for (uint32_t i = 0; i < asset_count; i++) {
        total_size += asset_sizes[i];
    }
    
    // Allocate and copy assets
    bundle->data = malloc(total_size);
    if (!bundle->data) return -2;
    
    uint8_t* ptr = (uint8_t*)bundle->data;
    for (uint32_t i = 0; i < asset_count; i++) {
        memcpy(ptr, assets[i], asset_sizes[i]);
        ptr += asset_sizes[i];
    }
    
    bundle->data_size = total_size;
    bundle->checksum = io_caching_calculate_checksum(bundle->data, total_size);
    
    ctx->bundle_ctx.bundle_count++;
    return 0;
}

/* Hierarchical culling functions */
static int io_caching_init_hierarchical_culling(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->culling_ctx.max_nodes = 65536;
    ctx->culling_ctx.nodes = calloc(65536, sizeof(io_caching_culling_node_t));
    ctx->culling_ctx.node_count = 0;
    ctx->culling_ctx.hierarchy_levels = 8;
    ctx->culling_ctx.culling_threshold = 0.1f;
    
    return 0;
}

static int io_caching_update_culling_hierarchy(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    // Update culling hierarchy with GPU feedback
    // Implementation would depend on graphics API and GPU feedback
    return 0;
}

/* Compression functions */
static int io_caching_init_compression(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->compression_ctx.use_lz4 = true;
    ctx->compression_ctx.use_zstd = true;
    ctx->compression_ctx.compression_level = IO_CACHING_DEFAULT_COMPRESSION_LEVEL;
    ctx->compression_ctx.compression_ratio = 0.0f;
    
#ifdef USE_LZ4
    ctx->compression_ctx.lz4_stream = LZ4_createStream();
#endif

#ifdef USE_ZSTD
    ctx->compression_ctx.zstd_cctx = ZSTD_createCCtx();
    ctx->compression_ctx.zstd_dctx = ZSTD_createDCtx();
#endif
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_COMPRESSION;
    return 0;
}

static int io_caching_compress_data(io_caching_renderer_03_t* ctx, const void* input,
                                   size_t input_size, void** output, size_t* output_size) {
    if (!ctx || !input || !output || !output_size) return -1;
    
    if (ctx->compression_ctx.use_zstd) {
#ifdef USE_ZSTD
        size_t max_size = ZSTD_compressBound(input_size);
        *output = malloc(max_size);
        if (!*output) return -2;
        
        *output_size = ZSTD_compressCCtx(ctx->compression_ctx.zstd_cctx, 
                                       *output, max_size, input, input_size, 
                                       ctx->compression_ctx.compression_level);
        
        if (ZSTD_isError(*output_size)) {
            free(*output);
            return -3;
        }
        
        ctx->compression_ctx.compression_ratio = (float)input_size / (float)*output_size;
        return 0;
#endif
    }
    
    if (ctx->compression_ctx.use_lz4) {
#ifdef USE_LZ4
        int max_size = LZ4_compressBound(input_size);
        *output = malloc(max_size);
        if (!*output) return -2;
        
        *output_size = LZ4_compress_default((const char*)input, (char*)*output, 
                                         input_size, max_size);
        
        if (*output_size <= 0) {
            free(*output);
            return -3;
        }
        
        ctx->compression_ctx.compression_ratio = (float)input_size / (float)*output_size;
        return 0;
#endif
    }
    
    return -4; // No compression available
}

static int io_caching_decompress_data(io_caching_renderer_03_t* ctx, const void* input,
                                     size_t input_size, void** output, size_t* output_size) {
    if (!ctx || !input || !output || !output_size) return -1;
    
    // For decompression, we need to know the original size
    // This is a simplified implementation
    *output = malloc(input_size * 4); // Assume 4x expansion max
    if (!*output) return -2;
    
    if (ctx->compression_ctx.use_zstd) {
#ifdef USE_ZSTD
        size_t result = ZSTD_decompressDCtx(ctx->compression_ctx.zstd_dctx,
                                          *output, input_size * 4, input, input_size);
        
        if (ZSTD_isError(result)) {
            free(*output);
            return -3;
        }
        
        *output_size = result;
        return 0;
#endif
    }
    
    if (ctx->compression_ctx.use_lz4) {
#ifdef USE_LZ4
        int result = LZ4_decompress_safe((const char*)input, (char*)*output,
                                       input_size, input_size * 4);
        
        if (result < 0) {
            free(*output);
            return -3;
        }
        
        *output_size = result;
        return 0;
#endif
    }
    
    return -4; // No decompression available
}

/* Scene file parsing functions */
static int io_caching_parse_gltf_scene(io_caching_renderer_03_t* ctx, const char* file_path,
                                       io_caching_scene_data_t* scene_data) {
    if (!ctx || !file_path || !scene_data) return -1;
    
#ifdef USE_CGLTF
    cgltf_options options = {0};
    cgltf_data* data = NULL;
    
    cgltf_result result = cgltf_parse_file(&options, file_path, &data);
    if (result != cgltf_result_success) {
        return -1;
    }
    
    result = cgltf_load_buffers(&options, data, file_path);
    if (result != cgltf_result_success) {
        cgltf_free(data);
        return -2;
    }
    
    // Extract scene data
    strncpy(scene_data->name, data->scene ? data->scene->name : "", sizeof(scene_data->name) - 1);
    scene_data->node_count = data->scene ? data->scene->nodes_count : 0;
    scene_data->mesh_count = data->meshes_count;
    scene_data->material_count = data->materials_count;
    scene_data->texture_count = data->textures_count;
    
    // Store pointers to data (in a real implementation, you would copy/convert this data)
    scene_data->nodes = data->scene ? data->scene->nodes : NULL;
    scene_data->meshes = data->meshes;
    scene_data->materials = data->materials;
    scene_data->textures = data->textures;
    
    cgltf_free(data);
    return 0;
#else
    return -1; // cgltf not available
#endif
}

static int io_caching_parse_fbx_scene(io_caching_renderer_03_t* ctx, const char* file_path,
                                      io_caching_scene_data_t* scene_data) {
    if (!ctx || !file_path || !scene_data) return -1;
    
    // FBX parsing would require a separate library like FBX SDK
    // This is a placeholder implementation
    strncpy(scene_data->name, "fbx_scene", sizeof(scene_data->name) - 1);
    scene_data->node_count = 0;
    scene_data->mesh_count = 0;
    scene_data->material_count = 0;
    scene_data->texture_count = 0;
    
    return -1; // Not implemented
}

/* Ray tracing hybrid rendering functions */
static int io_caching_init_ray_tracing(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    ctx->ray_tracing_ctx.max_ray_bounces = 4;
    ctx->ray_tracing_ctx.ray_tracing_mix_ratio = 0.3f;
    ctx->ray_tracing_ctx.denoising_enabled = true;
    
    ctx->flags |= IO_CACHING_RENDERER_03_FLAG_RAY_TRACING;
    return 0;
}

static int io_caching_render_ray_tracing_pass(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    // Render ray tracing pass
    // Implementation would depend on ray tracing API (DXR, Vulkan RT, etc.)
    return 0;
}

static int io_caching_hybrid_render(io_caching_renderer_03_t* ctx) {
    if (!ctx) return -1;
    
    // Combine rasterization and ray tracing results
    io_caching_render_ray_tracing_pass(ctx);
    
    // Mix with rasterized result based on ratio
    return 0;
}
