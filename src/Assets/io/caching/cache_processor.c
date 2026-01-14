/*
 * io_caching_processor_04.c
 *
 * I/O and asset streaming - Caching Subsystem
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * This file implements processor functionality for the caching module
 * within the io subsystem of the rendering engine.
 *
 * Key Features:
 *   - High-performance processor operations
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
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <immintrin.h>  // For SIMD intrinsics
#include <lz4.h>
#include <zstd.h>
#include <cgltf.h>

#include "assets/io/caching/cache_processor.h"
#include "include/core/types.h"
#include "include/core/memory.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define IO_CACHING_PROCESSOR_04_VERSION_MAJOR 1
#define IO_CACHING_PROCESSOR_04_VERSION_MINOR 0
#define IO_CACHING_PROCESSOR_04_VERSION_PATCH 0

#define IO_CACHING_PROCESSOR_04_MAX_INSTANCES 4096
#define IO_CACHING_PROCESSOR_04_DEFAULT_CAPACITY 256
#define IO_CACHING_PROCESSOR_04_ALIGNMENT 16

#define IO_CACHING_PROCESSOR_04_FLAG_NONE          0x00000000
#define IO_CACHING_PROCESSOR_04_FLAG_INITIALIZED   0x00000001
#define IO_CACHING_PROCESSOR_04_FLAG_DIRTY         0x00000002
#define IO_CACHING_PROCESSOR_04_FLAG_GPU_RESIDENT  0x00000004
#define IO_CACHING_PROCESSOR_04_FLAG_STREAMING     0x00000008
#define IO_CACHING_PROCESSOR_04_FLAG_SIMD         0x00000010
#define IO_CACHING_PROCESSOR_04_FLAG_COMPRESSION   0x00000020
#define IO_CACHING_PROCESSOR_04_FLAG_ASYNC         0x00000040
#define IO_CACHING_PROCESSOR_04_FLAG_CHECKPOINT    0x00000080
#define IO_CACHING_PROCESSOR_04_FLAG_CANCELLATION 0x00000100
#define IO_CACHING_PROCESSOR_04_FLAG_WORK_STEAL  0x00000200
#define IO_CACHING_PROCESSOR_04_FLAG_PROGRESS     0x00000400
#define IO_CACHING_PROCESSOR_04_FLAG_GPU_COMPUTE  0x00000800
#define IO_CACHING_PROCESSOR_04_FLAG_MEMORY_MAP   0x00001000
#define IO_CACHING_PROCESSOR_04_FLAG_FORMAT_CONV  0x00002000
#define IO_CACHING_PROCESSOR_04_FLAG_HOT_RELOAD  0x00004000
#define IO_CACHING_PROCESSOR_04_FLAG_COMPRESSION   0x00000010
#define IO_CACHING_PROCESSOR_04_FLAG_BUNDLING      0x00000020
#define IO_CACHING_PROCESSOR_04_FLAG_HOT_RELOAD    0x00000040
#define IO_CACHING_PROCESSOR_04_FLAG_WORK_STEALING 0x00000080
#define IO_CACHING_PROCESSOR_04_FLAG_CANCELLED     0x00000010
#define IO_CACHING_PROCESSOR_04_FLAG_COMPRESSED    0x00000020
#define IO_CACHING_PROCESSOR_04_FLAG_GPU_COMPUTE   0x00000040
#define IO_CACHING_PROCESSOR_04_FLAG_HOT_RELOAD    0x00000080

// Additional constants for new features
#define IO_CACHING_PROCESSOR_04_MAX_WORKERS 8
#define IO_CACHING_PROCESSOR_04_MAX_CACHE_ITEMS 1024
#define IO_CACHING_PROCESSOR_04_MAX_BUNDLE_SIZE (64 * 1024 * 1024)  // 64MB
#define IO_CACHING_PROCESSOR_04_MAX_MAPPED_FILES 64
#define IO_CACHING_PROCESSOR_04_WATCH_DESCRIPTOR_SIZE (sizeof(struct inotify_event) + 256)
#define IO_CACHING_PROCESSOR_04_SIMD_ALIGNMENT 32
#define IO_CACHING_PROCESSOR_04_PROGRESS_UPDATE_INTERVAL_MS 100

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */


/*
 * IO_CACHING_PROCESSOR_04 - Core data structure
 * Manages state and resources for processor_04 operations
 */
typedef struct io_caching_processor_04 {
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
    
    // Advanced processing subsystems
    void* simd_context;
    void* compression_context;
    void* async_context;
    void* checkpoint_context;
    void* progress_context;
    void* work_steal_context;
    void* scene_parser;
    void* memory_map_context;
    void* gpu_compute_context;
    void* format_converter;
    void* file_watch_context;
    
    // Processing state
    bool is_cancelled;
    uint32_t current_progress;
    char current_operation[256];
} io_caching_processor_04_t;

typedef struct io_caching_processor_04_desc {
    uint32_t flags;
    size_t initial_capacity;
    void* user_data;
    void* allocator;
    
    // Extended descriptor for new features
    int compression_level;
    bool enable_gpu_compute;
    bool enable_simd;
    bool enable_hot_reload;
    const char* watch_path;
    void (*reload_callback)(const char* path);
} io_caching_processor_04_desc_t;

typedef struct io_caching_processor_04_stats {
    uint64_t total_allocations;
    uint64_t active_count;
    uint64_t peak_count;
    size_t memory_used;
    size_t memory_peak;
    double avg_process_time_ms;
    
    // Advanced processing statistics
    uint64_t simd_operations;
    uint64_t compression_ratio;
    uint64_t files_watched;
    uint64_t async_operations;
    uint64_t bundles_created;
    uint64_t scenes_parsed;
    uint64_t cancellation_count;
    double avg_progress_time_ms;
} io_caching_processor_04_stats_t;

// Format converter structure
typedef struct {
    char source_format[32];
    char target_format[32];
    int (*convert_func)(const void* input, size_t input_size, void** output, size_t* output_size);
} io_caching_format_converter_t;

// Work item structure for work stealing
typedef struct {
    void* data;
    size_t data_size;
    int (*process_func)(void* data, size_t size);
    uint32_t priority;
    uint64_t submit_time;
} io_caching_work_item_t;


/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static io_caching_processor_04_stats_t s_processor_04_stats = {0};
static bool s_processor_04_initialized = false;

/* ============================================================================
 * HELPER FUNCTION DECLARATIONS
 * ============================================================================ */

// Compression helpers
static int io_caching_processor_04_init_compression(io_caching_processor_04_t* ctx);
static int io_caching_processor_04_compress_data(io_caching_processor_04_t* ctx, 
                                               const void* input, size_t input_size,
                                               void** output, size_t* output_size);
static int io_caching_processor_04_decompress_data(io_caching_processor_04_t* ctx,
                                                 const void* input, size_t input_size,
                                                 void** output, size_t* output_size);
static void io_caching_processor_04_cleanup_compression(io_caching_processor_04_t* ctx);

// Asset bundling helpers
static int io_caching_processor_04_create_bundle(io_caching_processor_04_t* ctx,
                                                const char* name,
                                                const void** assets,
                                                const size_t* asset_sizes,
                                                uint32_t asset_count);
static int io_caching_processor_04_load_bundle(io_caching_processor_04_t* ctx,
                                              const char* name);
static void io_caching_processor_04_cleanup_bundles(io_caching_processor_04_t* ctx);

// Scene parsing helpers
static int io_caching_processor_04_parse_gltf(io_caching_processor_04_t* ctx,
                                              const char* filename);
static int io_caching_processor_04_parse_fbx(io_caching_processor_04_t* ctx,
                                             const char* filename);
static int io_caching_processor_04_parse_obj(io_caching_processor_04_t* ctx,
                                             const char* filename);
static void io_caching_processor_04_cleanup_scene_data(io_caching_processor_04_t* ctx);

// Work stealing helpers
static int io_caching_processor_04_init_work_stealing(io_caching_processor_04_t* ctx);
static void* io_caching_processor_04_worker_thread(void* arg);
static int io_caching_processor_04_add_work(io_caching_processor_04_t* ctx, void* work_item);
static void* io_caching_processor_04_steal_work(io_caching_processor_04_t* ctx);
static void io_caching_processor_04_cleanup_work_stealing(io_caching_processor_04_t* ctx);

// Progress reporting helpers
static int io_caching_processor_04_init_progress(io_caching_processor_04_t* ctx);
static void io_caching_processor_04_update_progress(io_caching_processor_04_t* ctx,
                                                   float percentage,
                                                   const char* operation);
static void io_caching_processor_04_cleanup_progress(io_caching_processor_04_t* ctx);

// File watching helpers
static int io_caching_processor_04_init_file_watcher(io_caching_processor_04_t* ctx);
static void* io_caching_processor_04_file_watch_thread(void* arg);
static int io_caching_processor_04_add_file_watch(io_caching_processor_04_t* ctx,
                                                 const char* filename);
static void io_caching_processor_04_cleanup_file_watcher(io_caching_processor_04_t* ctx);

// Memory mapping helpers
static int io_caching_processor_04_map_file(io_caching_processor_04_t* ctx,
                                           const char* filename);
static void io_caching_processor_04_unmap_file(io_caching_processor_04_t* ctx,
                                              uint32_t file_index);

// Format conversion helpers
static int io_caching_processor_04_init_format_converters(io_caching_processor_04_t* ctx);
static int io_caching_processor_04_convert_format(io_caching_processor_04_t* ctx,
                                                 const char* source_format,
                                                 const char* target_format,
                                                 const void* input,
                                                 void* output,
                                                 size_t input_size,
                                                 size_t* output_size);

// Utility helpers
static uint32_t io_caching_processor_04_calculate_checksum(const void* data, size_t size);
static int io_caching_processor_04_simd_process(const void* input, void* output, size_t size);
static bool io_caching_processor_04_is_cancelled(io_caching_processor_04_t* ctx);

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int io_caching_processor_04_validate_internal(io_caching_processor_04_t* ctx);
static int io_caching_processor_04_cleanup_internal(io_caching_processor_04_t* ctx);

// Helper function declarations for new features
static int io_caching_processor_04_init_work_stealing(io_caching_processor_04_t* ctx);
static int io_caching_processor_04_shutdown_work_stealing(io_caching_processor_04_t* ctx);
static void* io_caching_processor_04_worker_thread(void* arg);
static int io_caching_processor_04_compress_data_lz4(io_caching_processor_04_t* ctx, const void* input, size_t input_size);
static int io_caching_processor_04_compress_data_zstd(io_caching_processor_04_t* ctx, const void* input, size_t input_size);
static int io_caching_processor_04_serialize_data(io_caching_processor_04_t* ctx, const void* data, size_t size);
static int io_caching_processor_04_deserialize_data(io_caching_processor_04_t* ctx, void** data, size_t* size);
static int io_caching_processor_04_init_gpu_compute(io_caching_processor_04_t* ctx);
static int io_caching_processor_04_gpu_compute_fallback(io_caching_processor_04_t* ctx, void* data, size_t size);
static int io_caching_processor_04_sort_cache_aware(io_caching_processor_04_t* ctx);
static int io_caching_processor_04_check_cancellation(io_caching_processor_04_t* ctx);
static int io_caching_processor_04_create_asset_bundle(io_caching_processor_04_t* ctx);
static int io_caching_processor_04_update_progress(io_caching_processor_04_t* ctx, float progress, const char* operation);
static int io_caching_processor_04_init_file_watcher(io_caching_processor_04_t* ctx, const char* path);
static void* io_caching_processor_04_file_watch_thread(void* arg);
static int io_caching_processor_04_map_file(io_caching_processor_04_t* ctx, const char* path);
static int io_caching_processor_04_unmap_file(io_caching_processor_04_t* ctx, int fd);
static int io_caching_processor_04_simd_process(io_caching_processor_04_t* ctx, void* data, size_t size);
static void* io_caching_processor_04_async_load_thread(void* arg);
static int io_caching_processor_04_parse_scene_gltf(io_caching_processor_04_t* ctx, const char* path);
static int io_caching_processor_04_convert_format(io_caching_processor_04_t* ctx, const char* source_format, const char* target_format);
static uint32_t io_caching_processor_04_calculate_checksum(const void* data, size_t size);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static int io_caching_processor_04_validate_internal(io_caching_processor_04_t* ctx) {
    // Implement scene file parsing
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_FORMAT_CONV) {
        if (!ctx->scene_parser) {
            return -3; // Scene parser not initialized
        }
    }
    
    // Implement work stealing for load balancing
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_WORK_STEAL) {
        if (!ctx->work_steal_context) {
            return -4; // Work stealing not initialized
        }
    }
    
    if (!ctx) return -1;
    if (!ctx->is_initialized) return -2;
    
    // Scene file parsing implementation
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_BUNDLING) {
        if (!ctx->scene_data.is_loaded && ctx->bundle_count > 0) {
            // Validate scene data for bundles
            for (uint32_t i = 0; i < ctx->bundle_count; i++) {
                if (ctx->bundles[i].asset_count == 0) {
                    return -3; // Invalid bundle
                }
            }
        }
    }
    
    // Work stealing validation
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_WORK_STEAL) {
        if (ctx->work_steal_context->workers[0] == 0) {
            return -4; // Work stealing not initialized
        }
    }
    
    // Scene file parsing validation
    if (ctx->scene_parser->scene_loaded && !ctx->scene_parser->scene_data) {
        return -3; // Scene marked as loaded but no data present
    }
    
    // Work stealing validation
    if (ctx->work_steal_context->workers[0] != 0 && !ctx->work_steal_context->work_queue) {
        return -4; // Workers initialized but no work queue
    }
    
    return 0;
}

static int io_caching_processor_04_cleanup_internal(io_caching_processor_04_t* ctx) {
    // TODO: Implement incremental processing for streaming
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_STREAMING) {
        // Clean up streaming buffers
        if (ctx->internal_data) {
            free(ctx->internal_data);
            ctx->internal_data = NULL;
        }
    }
    
    // Implement scene file parsing
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_FORMAT_CONV) {
        if (ctx->scene_parser) {
            cache_scene_parser_t* parser = (cache_scene_parser_t*)ctx->scene_parser;
            if (parser->gltf_data) {
                cgltf_free(parser->gltf_data);
                parser->gltf_data = NULL;
            }
            free(ctx->scene_parser);
            ctx->scene_parser = NULL;
        }
    }
    
    // Implement work stealing for load balancing
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_WORK_STEAL) {
        if (ctx->work_steal_context) {
            // Clean up work stealing context
            for (int i = 0; i < IO_CACHING_PROCESSOR_04_MAX_WORKERS; i++) {
                pthread_join(ctx->work_steal_context->workers[i], NULL);
            }
            free(ctx->work_steal_context->work_queue);
            ctx->work_steal_context->work_queue = NULL;
            free(ctx->work_steal_context);
            ctx->work_steal_context = NULL;
        }
    }
    
    if (!ctx) return -1;
    ctx->is_dirty = false;
    return 0;
}
        // Set cache management callback
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_caching_processor_04_get_memory_usage
 * Returns current memory usage
 */
int io_caching_processor_04_get_memory_usage(io_caching_processor_04_t* ctx) {
    // Memory-mapped file support for large datasets
    size_t total_memory = ctx->data_size;
    
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_HOT_RELOAD) {
        // Add memory-mapped file sizes
        for (uint32_t i = 0; i < ctx->mapped_file_count; i++) {
            if (ctx->mapped_files[i].is_mapped) {
                total_memory += ctx->mapped_files[i].file_size;
            }
        }
    }
    
    // Compression during processing memory usage
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_COMPRESSION) {
        // Add compression buffer sizes
        total_memory += ctx->compression.total_compressed;
        total_memory += ctx->compression.total_uncompressed;
    }
    
    if (!ctx) return -1;
    return (int)total_memory;
}

/*
 * io_caching_processor_04_optimize
 * Optimizes internal data structures
 */
int io_caching_processor_04_optimize(io_caching_processor_04_t* ctx) {
    // Asset bundling optimization
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_BUNDLING) {
        // Optimize bundle layout for better compression
        for (uint32_t i = 0; i < ctx->bundle_count; i++) {
            // Reorganize bundle data for optimal access
            // Sort assets by access frequency
        }
    }
    
    // Scene file parsing optimization
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_BUNDLING && ctx->scene_data.is_loaded) {
        // Optimize scene data structure
        // Remove unused nodes, merge similar materials
        if (ctx->scene_data.gltf_data) {
            // Optimize glTF data structure
        }
    }
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_caching_processor_04_debug_print
 * Prints debug information
 */
int io_caching_processor_04_debug_print(io_caching_processor_04_t* ctx) {
    // Asset streaming priority debug info
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_STREAMING) {
        // Print streaming priority information
        // LOG_INFO("Streaming priority: HIGH");
    }
    
    // Hot-reload file watching debug info
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_HOT_RELOAD) {
        // Print file watching status
        printf("File watching: %s\n", ctx->file_watcher.watch_path);
    }
    
    if (!ctx) return -1;
    return 0;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

/*
 * io_caching_processor_04_module_init
 * Initializes the entire processor_04 module
 */
int io_caching_processor_04_module_init(void) {
    // LZ4/ZSTD compression
    // Initialize compression libraries
    
    // Work stealing for load balancing
    // Initialize global work stealing system
    
    // Scene file parsing
    // Initialize scene parsing libraries
    
    // Progress reporting for long operations
    // Initialize progress reporting system

    if (s_processor_04_initialized) {
        return 0;  // Already initialized
    }

    // Initialize statistics
    memset(&s_processor_04_stats, 0, sizeof(s_processor_04_stats));

    s_processor_04_initialized = true;
    return 0;
}

/*
 * io_caching_processor_04_module_shutdown
 * Shuts down the entire processor_04 module
 */
int io_caching_processor_04_module_shutdown(void) {
    // glTF/FBX import
    // Cleanup scene import libraries
    
    // Asset cache management
    // Cleanup global cache
    
    // Asset bundling
    // Cleanup bundling system
    
    // Work stealing for load balancing
    // Shutdown global work stealing system

    if (!s_processor_04_initialized) {
        return 0;  // Already shut down
    }

    s_processor_04_initialized = false;
    return 0;
}

/* End of io_caching_processor_04.c */

/* ============================================================================
 * HELPER FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

// Work stealing implementation
static int io_caching_processor_04_init_work_stealing(io_caching_processor_04_t* ctx) {
    if (!ctx) return -1;
    
    pthread_mutex_init(&ctx->work_stealing.work_mutex, NULL);
    pthread_cond_init(&ctx->work_stealing.work_cond, NULL);
    
    ctx->work_stealing.queue_size = 1024;
    ctx->work_stealing.work_queue = malloc(ctx->work_stealing.queue_size * sizeof(void*));
    ctx->work_stealing.queue_head = 0;
    ctx->work_stealing.queue_tail = 0;
    ctx->work_stealing.shutdown = false;
    
    // Create worker threads
    for (int i = 0; i < IO_CACHING_PROCESSOR_04_MAX_WORKERS; i++) {
        pthread_create(&ctx->work_stealing.workers[i], NULL, 
                       io_caching_processor_04_worker_thread, ctx);
    }
    
    return 0;
}

static int io_caching_processor_04_shutdown_work_stealing(io_caching_processor_04_t* ctx) {
    if (!ctx) return -1;
    
    ctx->work_stealing.shutdown = true;
    pthread_cond_broadcast(&ctx->work_stealing.work_cond);
    
    // Join worker threads
    for (int i = 0; i < IO_CACHING_PROCESSOR_04_MAX_WORKERS; i++) {
        pthread_join(ctx->work_stealing.workers[i], NULL);
    }
    
    pthread_mutex_destroy(&ctx->work_stealing.work_mutex);
    pthread_cond_destroy(&ctx->work_stealing.work_cond);
    
    if (ctx->work_stealing.work_queue) {
        free(ctx->work_stealing.work_queue);
        ctx->work_stealing.work_queue = NULL;
    }
    
    return 0;
}

static void* io_caching_processor_04_worker_thread(void* arg) {
    io_caching_processor_04_t* ctx = (io_caching_processor_04_t*)arg;
    
    while (!ctx->work_stealing.shutdown) {
        pthread_mutex_lock(&ctx->work_stealing.work_mutex);
        
        // Wait for work
        while (ctx->work_stealing.queue_head == ctx->work_stealing.queue_tail && 
               !ctx->work_stealing.shutdown) {
            pthread_cond_wait(&ctx->work_stealing.work_cond, &ctx->work_stealing.work_mutex);
        }
        
        if (ctx->work_stealing.shutdown) {
            pthread_mutex_unlock(&ctx->work_stealing.work_mutex);
            break;
        }
        
        // Get work item
        void* work_item = ctx->work_stealing.work_queue[ctx->work_stealing.queue_head];
        ctx->work_stealing.queue_head = (ctx->work_stealing.queue_head + 1) % ctx->work_stealing.queue_size;
        
        pthread_mutex_unlock(&ctx->work_stealing.work_mutex);
        
        // Process work item (placeholder)
        usleep(100); // Simulate work
    }
    
    return NULL;
}

// Compression implementations
static int io_caching_processor_04_compress_data_lz4(io_caching_processor_04_t* ctx, const void* input, size_t input_size) {
    if (!ctx || !input || input_size == 0) return -1;
    
    int max_compressed_size = LZ4_compressBound(input_size);
    ctx->compression.compressed_data = malloc(max_compressed_size);
    
    if (!ctx->compression.compressed_data) return -2;
    
    ctx->compression.compressed_size = LZ4_compress_default(
        input, input_size,
        ctx->compression.compressed_data, max_compressed_size
    );
    
    ctx->compression.original_size = input_size;
    ctx->compression.use_lz4 = true;
    
    if (ctx->compression.compressed_size <= 0) {
        free(ctx->compression.compressed_data);
        ctx->compression.compressed_data = NULL;
        return -3;
    }
    
    s_processor_04_stats.compression_ratio = 
        (uint64_t)ctx->compression.compressed_size * 100 / input_size;
    
    return 0;
}

static int io_caching_processor_04_compress_data_zstd(io_caching_processor_04_t* ctx, const void* input, size_t input_size) {
    if (!ctx || !input || input_size == 0) return -1;
    
    size_t max_compressed_size = ZSTD_compressBound(input_size);
    ctx->compression.compressed_data = malloc(max_compressed_size);
    
    if (!ctx->compression.compressed_data) return -2;
    
    ctx->compression.compressed_size = ZSTD_compress(
        ctx->compression.compressed_data, max_compressed_size,
        input, input_size,
        ctx->compression.compression_level
    );
    
    ctx->compression.original_size = input_size;
    ctx->compression.use_lz4 = false;
    
    if (ZSTD_isError(ctx->compression.compressed_size)) {
        free(ctx->compression.compressed_data);
        ctx->compression.compressed_data = NULL;
        return -3;
    }
    
    s_processor_04_stats.compression_ratio = 
        (uint64_t)ctx->compression.compressed_size * 100 / input_size;
    
    return 0;
}

// Serialization implementation
static int io_caching_processor_04_serialize_data(io_caching_processor_04_t* ctx, const void* data, size_t size) {
    if (!ctx || !data || size == 0) return -1;
    
    ctx->serialization.magic = 0x43414345; // "CAKE"
    ctx->serialization.version = 1;
    ctx->serialization.timestamp = time(NULL);
    ctx->serialization.checksum = io_caching_processor_04_calculate_checksum(data, size);
    
    // Allocate space for header + data
    ctx->serialization.data_size = sizeof(uint32_t) * 4 + size;
    ctx->serialization.data = malloc(ctx->serialization.data_size);
    
    if (!ctx->serialization.data) return -2;
    
    // Write header
    uint8_t* ptr = (uint8_t*)ctx->serialization.data;
    memcpy(ptr, &ctx->serialization.magic, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(ptr, &ctx->serialization.version, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(ptr, &ctx->serialization.timestamp, sizeof(uint64_t)); ptr += sizeof(uint64_t);
    memcpy(ptr, &ctx->serialization.checksum, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    
    // Write data
    memcpy(ptr, data, size);
    
    return 0;
}

static int io_caching_processor_04_deserialize_data(io_caching_processor_04_t* ctx, void** data, size_t* size) {
    if (!ctx || !data || !size) return -1;
    
    if (!ctx->serialization.data) return -2;
    
    uint8_t* ptr = (uint8_t*)ctx->serialization.data;
    
    // Read and validate header
    uint32_t magic, version, checksum;
    uint64_t timestamp;
    
    memcpy(&magic, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(&version, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(&timestamp, ptr, sizeof(uint64_t)); ptr += sizeof(uint64_t);
    memcpy(&checksum, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    
    if (magic != ctx->serialization.magic) return -3; // Invalid magic
    if (version != ctx->serialization.version) return -4; // Unsupported version
    
    size_t data_size = ctx->serialization.data_size - (sizeof(uint32_t) * 4 + sizeof(uint64_t));
    
    // Verify checksum
    uint32_t calculated_checksum = io_caching_processor_04_calculate_checksum(ptr, data_size);
    if (calculated_checksum != checksum) return -5; // Corrupted data
    
    // Allocate and copy data
    *data = malloc(data_size);
    if (!*data) return -6;
    
    memcpy(*data, ptr, data_size);
    *size = data_size;
    
    return 0;
}

// GPU compute implementation
static int io_caching_processor_04_init_gpu_compute(io_caching_processor_04_t* ctx) {
    if (!ctx) return -1;
    
    // Check for GPU availability (placeholder)
    ctx->gpu_compute.gpu_available = true; // Assume available
    
    if (ctx->gpu_compute.gpu_available) {
        // Initialize GPU context (placeholder)
        ctx->gpu_compute.gpu_context = malloc(1024); // Mock GPU context
    } else {
        // Initialize fallback buffer
        ctx->gpu_compute.fallback_size = 1024 * 1024; // 1MB
        ctx->gpu_compute.fallback_buffer = malloc(ctx->gpu_compute.fallback_size);
    }
    
    return 0;
}

static int io_caching_processor_04_gpu_compute_fallback(io_caching_processor_04_t* ctx, void* data, size_t size) {
    if (!ctx || !data || size == 0) return -1;
    
    if (!ctx->gpu_compute.fallback_buffer) {
        ctx->gpu_compute.fallback_size = size;
        ctx->gpu_compute.fallback_buffer = malloc(size);
    }
    
    if (size > ctx->gpu_compute.fallback_size) {
        free(ctx->gpu_compute.fallback_buffer);
        ctx->gpu_compute.fallback_size = size;
        ctx->gpu_compute.fallback_buffer = malloc(size);
    }
    
    // Copy data to fallback buffer and process (placeholder)
    memcpy(ctx->gpu_compute.fallback_buffer, data, size);
    
    // Simulate GPU processing
    for (size_t i = 0; i < size; i++) {
        ((uint8_t*)ctx->gpu_compute.fallback_buffer)[i] = ((uint8_t*)data)[i] ^ 0xFF;
    }
    
    return 0;
}

// Cache-aware processing
static int io_caching_processor_04_sort_cache_aware(io_caching_processor_04_t* ctx) {
    if (!ctx || !ctx->cache_aware.cache_items) return -1;
    
    // Sort by access time and priority (simple bubble sort for demonstration)
    for (size_t i = 0; i < ctx->cache_aware.cache_size - 1; i++) {
        for (size_t j = 0; j < ctx->cache_aware.cache_size - i - 1; j++) {
            // Compare by priority first, then by access time
            if (ctx->cache_aware.priorities[j] < ctx->cache_aware.priorities[j + 1] ||
                (ctx->cache_aware.priorities[j] == ctx->cache_aware.priorities[j + 1] &&
                 ctx->cache_aware.access_times[j] < ctx->cache_aware.access_times[j + 1])) {
                
                // Swap items
                void* temp_item = ctx->cache_aware.cache_items[j];
                ctx->cache_aware.cache_items[j] = ctx->cache_aware.cache_items[j + 1];
                ctx->cache_aware.cache_items[j + 1] = temp_item;
                
                size_t temp_time = ctx->cache_aware.access_times[j];
                ctx->cache_aware.access_times[j] = ctx->cache_aware.access_times[j + 1];
                ctx->cache_aware.access_times[j + 1] = temp_time;
                
                size_t temp_priority = ctx->cache_aware.priorities[j];
                ctx->cache_aware.priorities[j] = ctx->cache_aware.priorities[j + 1];
                ctx->cache_aware.priorities[j + 1] = temp_priority;
            }
        }
    }
    
    return 0;
}

// Cancellation support
static int io_caching_processor_04_check_cancellation(io_caching_processor_04_t* ctx) {
    if (!ctx) return -1;
    
    pthread_mutex_lock(&ctx->cancellation.cancel_mutex);
    bool cancelled = ctx->cancellation.cancelled;
    pthread_mutex_unlock(&ctx->cancellation.cancel_mutex);
    
    return cancelled ? -2 : 0;
}

// Asset bundling
static int io_caching_processor_04_create_asset_bundle(io_caching_processor_04_t* ctx) {
    if (!ctx) return -1;
    
    // Calculate total bundle size
    ctx->asset_bundle.bundle_size = 0;
    for (size_t i = 0; i < ctx->asset_bundle.asset_count; i++) {
        ctx->asset_bundle.bundle_size += ctx->asset_bundle.asset_sizes[i];
    }
    
    // Add metadata overhead
    ctx->asset_bundle.bundle_size += 1024; // 1KB for metadata
    
    if (ctx->asset_bundle.bundle_size > IO_CACHING_PROCESSOR_04_MAX_BUNDLE_SIZE) {
        return -2; // Bundle too large
    }
    
    // Allocate bundle data
    ctx->asset_bundle.bundle_data = malloc(ctx->asset_bundle.bundle_size);
    if (!ctx->asset_bundle.bundle_data) return -3;
    
    // Serialize assets into bundle (placeholder)
    uint8_t* ptr = (uint8_t*)ctx->asset_bundle.bundle_data;
    
    // Write bundle header
    uint32_t asset_count = (uint32_t)ctx->asset_bundle.asset_count;
    memcpy(ptr, &asset_count, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    
    // Write asset data
    for (size_t i = 0; i < ctx->asset_bundle.asset_count; i++) {
        // Write asset name length and name
        uint32_t name_len = (uint32_t)strlen(ctx->asset_bundle.asset_names[i]);
        memcpy(ptr, &name_len, sizeof(uint32_t)); ptr += sizeof(uint32_t);
        memcpy(ptr, ctx->asset_bundle.asset_names[i], name_len); ptr += name_len;
        
        // Write asset size and data
        uint32_t asset_size = (uint32_t)ctx->asset_bundle.asset_sizes[i];
        memcpy(ptr, &asset_size, sizeof(uint32_t)); ptr += sizeof(uint32_t);
        memcpy(ptr, ctx->asset_bundle.assets[i], asset_size); ptr += asset_size;
    }
    
    return 0;
}

// Progress reporting
static int io_caching_processor_04_update_progress(io_caching_processor_04_t* ctx, float progress, const char* operation) {
    if (!ctx || !operation) return -1;
    
    pthread_mutex_lock(&ctx->progress.progress_mutex);
    
    ctx->progress.progress = progress;
    strncpy(ctx->progress.current_operation, operation, 
            sizeof(ctx->progress.current_operation) - 1);
    ctx->progress.last_update_time = time(NULL);
    
    pthread_mutex_unlock(&ctx->progress.progress_mutex);
    
    // Print progress (for debugging)
    printf("Progress: %.1f%% - %s\n", progress * 100.0f, operation);
    
    return 0;
}

// File watching
static int io_caching_processor_04_init_file_watcher(io_caching_processor_04_t* ctx, const char* path) {
    if (!ctx || !path) return -1;
    
    ctx->file_watcher.inotify_fd = inotify_init();
    if (ctx->file_watcher.inotify_fd < 0) return -2;
    
    ctx->file_watcher.watch_descriptor = inotify_add_watch(
        ctx->file_watcher.inotify_fd, path, 
        IN_MODIFY | IN_CREATE | IN_DELETE
    );
    
    if (ctx->file_watcher.watch_descriptor < 0) {
        close(ctx->file_watcher.inotify_fd);
        return -3;
    }
    
    strncpy(ctx->file_watcher.watch_path, path, 
            sizeof(ctx->file_watcher.watch_path) - 1);
    
    // Start watch thread
    pthread_create(&ctx->file_watcher.watch_thread, NULL,
                   io_caching_processor_04_file_watch_thread, ctx);
    
    return 0;
}

static void* io_caching_processor_04_file_watch_thread(void* arg) {
    io_caching_processor_04_t* ctx = (io_caching_processor_04_t*)arg;
    
    char buffer[IO_CACHING_PROCESSOR_04_WATCH_DESCRIPTOR_SIZE];
    
    while (true) {
        ssize_t length = read(ctx->file_watcher.inotify_fd, buffer, sizeof(buffer));
        
        if (length < 0) break;
        
        size_t i = 0;
        while (i < length) {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];
            
            if (event->len > 0 && ctx->file_watcher.reload_callback) {
                ctx->file_watcher.reload_callback(event->name);
            }
            
            i += sizeof(struct inotify_event) + event->len;
        }
    }
    
    return NULL;
}

// Memory mapping
static int io_caching_processor_04_map_file(io_caching_processor_04_t* ctx, const char* path) {
    if (!ctx || !path) return -1;
    
    if (ctx->memory_mapped.mapped_count >= IO_CACHING_PROCESSOR_04_MAX_MAPPED_FILES) {
        return -2; // Too many mapped files
    }
    
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -3;
    
    size_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == 0) {
        close(fd);
        return -4;
    }
    
    void* addr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return -5;
    }
    
    size_t index = ctx->memory_mapped.mapped_count;
    ctx->memory_mapped.mapped_fds[index] = fd;
    ctx->memory_mapped.mapped_addrs[index] = addr;
    ctx->memory_mapped.mapped_sizes[index] = file_size;
    ctx->memory_mapped.mapped_paths[index] = strdup(path);
    ctx->memory_mapped.mapped_count++;
    
    return 0;
}

static int io_caching_processor_04_unmap_file(io_caching_processor_04_t* ctx, int fd) {
    if (!ctx) return -1;
    
    for (size_t i = 0; i < ctx->memory_mapped.mapped_count; i++) {
        if (ctx->memory_mapped.mapped_fds[i] == fd) {
            munmap(ctx->memory_mapped.mapped_addrs[i], 
                   ctx->memory_mapped.mapped_sizes[i]);
            close(fd);
            
            if (ctx->memory_mapped.mapped_paths[i]) {
                free(ctx->memory_mapped.mapped_paths[i]);
            }
            
            // Shift remaining entries
            for (size_t j = i; j < ctx->memory_mapped.mapped_count - 1; j++) {
                ctx->memory_mapped.mapped_fds[j] = ctx->memory_mapped.mapped_fds[j + 1];
                ctx->memory_mapped.mapped_addrs[j] = ctx->memory_mapped.mapped_addrs[j + 1];
                ctx->memory_mapped.mapped_sizes[j] = ctx->memory_mapped.mapped_sizes[j + 1];
                ctx->memory_mapped.mapped_paths[j] = ctx->memory_mapped.mapped_paths[j + 1];
            }
            
            ctx->memory_mapped.mapped_count--;
            return 0;
        }
    }
    
    return -2; // File not found
}

// SIMD processing
static int io_caching_processor_04_simd_process(io_caching_processor_04_t* ctx, void* data, size_t size) {
    if (!ctx || !data || size == 0) return -1;
    
    if (!ctx->simd_processing.simd_available) {
        return -2; // SIMD not available
    }
    
    // Align data to SIMD boundary
    size_t aligned_size = (size + IO_CACHING_PROCESSOR_04_SIMD_ALIGNMENT - 1) & 
                        ~(IO_CACHING_PROCESSOR_04_SIMD_ALIGNMENT - 1);
    
    if (ctx->simd_processing.simd_size < aligned_size) {
        free(ctx->simd_processing.simd_buffer);
        ctx->simd_processing.simd_buffer = aligned_alloc(IO_CACHING_PROCESSOR_04_SIMD_ALIGNMENT, 
                                                     aligned_size);
        ctx->simd_processing.simd_size = aligned_size;
    }
    
    if (!ctx->simd_processing.simd_buffer) return -3;
    
    // Copy data to aligned buffer
    memcpy(ctx->simd_processing.simd_buffer, data, size);
    
    // Perform SIMD operations (placeholder - simple vector addition)
    float* float_data = (float*)ctx->simd_processing.simd_buffer;
    size_t float_count = size / sizeof(float);
    
    __m256 multiplier = _mm256_set1_ps(2.0f);
    
    for (size_t i = 0; i < float_count - 7; i += 8) {
        __m256 vec = _mm256_load_ps(&float_data[i]);
        vec = _mm256_mul_ps(vec, multiplier);
        _mm256_store_ps(&float_data[i], vec);
    }
    
    // Handle remaining elements
    for (size_t i = (float_count / 8) * 8; i < float_count; i++) {
        float_data[i] *= 2.0f;
    }
    
    return 0;
}

// Async file loading
static void* io_caching_processor_04_async_load_thread(void* arg) {
    io_caching_processor_04_t* ctx = (io_caching_processor_04_t*)arg;
    
    while (true) {
        pthread_mutex_lock(&ctx->async_file.async_mutex);
        
        // Wait for work
        while (ctx->async_file.async_file_path[0] == '\0' && !ctx->async_file.async_error) {
            pthread_cond_wait(&ctx->async_file.async_cond, &ctx->async_file.async_mutex);
        }
        
        if (ctx->async_file.async_error) {
            pthread_mutex_unlock(&ctx->async_file.async_mutex);
            break;
        }
        
        // Load file
        FILE* file = fopen(ctx->async_file.async_file_path, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            ctx->async_file.async_result = malloc(file_size);
            if (ctx->async_file.async_result) {
                ctx->async_file.async_result_size = fread(ctx->async_file.async_result, 1, file_size, file);
                ctx->async_file.async_complete = true;
            } else {
                ctx->async_file.async_error = true;
            }
            
            fclose(file);
        } else {
            ctx->async_file.async_error = true;
        }
        
        // Clear path
        ctx->async_file.async_file_path[0] = '\0';
        
        pthread_mutex_unlock(&ctx->async_file.async_mutex);
    }
    return NULL;
}

// Scene parsing
static int io_caching_processor_04_process_batch(io_caching_processor_04_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_processor_04_process_batch: Invalid context");
        return -1;
    }

    // Implement SIMD-optimized processing paths
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_SIMD) {
        // Use SIMD instructions for batch processing
        io_caching_processor_04_simd_process(ctx, params, ctx->data_size);
        s_processor_04_stats.simd_operations++;
    }
    
    // Implement async file loading
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_ASYNC) {
        // Process async file operations in batch
        // Handle completed async operations
        s_processor_04_stats.async_operations++;
    }
    
    // Implement compression during processing
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_COMPRESSION) {
        void* compressed_data = NULL;
        size_t compressed_size = 0;
        if (io_caching_processor_04_compress_data(ctx, params, ctx->data_size, 
                                                 &compressed_data, &compressed_size) == 0) {
            // Store compressed data and update statistics
            s_processor_04_stats.compression_ratio = (ctx->data_size * 100) / compressed_size;
        }
    }
    
    // Implement binary serialization
    if (ctx->flags & IO_CACHING_PROCESSOR_04_FLAG_CHECKPOINT) {
        // Serialize batch data for checkpointing
        uint32_t checksum = io_caching_processor_04_calculate_checksum(params, ctx->data_size);
        // Store checkpoint data
        s_processor_04_stats.checkpoints_created++;
    }

    // Placeholder implementation
    (void)params;

    return 0;
}

static int io_caching_processor_04_parse_scene_gltf(io_caching_processor_04_t* ctx, const char* path) {
    if (!ctx || !path) return -1;
    
    // Mock glTF parsing (in real implementation, use cgltf)
    FILE* file = fopen(path, "rb");
    if (!file) return -2;
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    ctx->scene_parser.scene_data = malloc(file_size);
    if (!ctx->scene_parser.scene_data) {
        fclose(file);
        return -3;
    }
    
    ctx->scene_parser.scene_size = fread(ctx->scene_parser.scene_data, 1, file_size, file);
    fclose(file);
    
    strncpy(ctx->scene_parser.scene_format, "gltf", 
            sizeof(ctx->scene_parser.scene_format) - 1);
    ctx->scene_parser.scene_loaded = true;
    
    return 0;
}

// Format conversion
static int io_caching_processor_04_convert_format(io_caching_processor_04_t* ctx, const char* source_format, const char* target_format) {
    if (!ctx || !source_format || !target_format) return -1;
    
    // Mock format conversion (in real implementation, use format converters)
    printf("Converting from %s to %s\n", source_format, target_format);
    
    // Simulate conversion time
    usleep(10000); // 10ms
    
    return 0;
}

// Checksum calculation
static uint32_t io_caching_processor_04_calculate_checksum(const void* data, size_t size) {
    if (!data || size == 0) return 0;
    
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    // Simple CRC32-like checksum
    for (size_t i = 0; i < size; i++) {
        checksum ^= bytes[i];
        checksum = (checksum << 1) | (checksum >> 31); // Rotate left
    }
    
    return checksum;
}
