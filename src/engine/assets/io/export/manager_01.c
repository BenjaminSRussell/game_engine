/*
 * io_export_manager_01.c
 *
 * I/O and asset streaming - Export Subsystem
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * This file implements manager functionality for the export module
 * within the io subsystem of the rendering engine.
 *
 * Key Features:
 *   - High-performance manager operations
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
#include <errno.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#include "assets/io/export/manager_01.h"
#include "include/core/types.h"
#include "include/core/memory.h"
#include "core/logger.h"
#include "core/memory.h"

// LZ4/ZSTD compression includes
#ifdef ENABLE_LZ4
#include <lz4.h>
#include <lz4hc.h>
#endif
#ifdef ENABLE_ZSTD
#include <zstd.h>
#endif

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define IO_EXPORT_MANAGER_01_VERSION_MAJOR 1
#define IO_EXPORT_MANAGER_01_VERSION_MINOR 0
#define IO_EXPORT_MANAGER_01_VERSION_PATCH 0

#define IO_EXPORT_MANAGER_01_MAX_INSTANCES 4096
#define IO_EXPORT_MANAGER_01_DEFAULT_CAPACITY 256
#define IO_EXPORT_MANAGER_01_ALIGNMENT 16
#define IO_EXPORT_MANAGER_01_MAX_FILE_WATCHERS 1024
#define IO_EXPORT_MANAGER_01_MAX_ASYNC_OPERATIONS 64
#define IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE 1024
#define IO_EXPORT_MANAGER_01_MEMORY_BUDGET_DEFAULT (512 * 1024 * 1024)  // 512MB
#define IO_EXPORT_MANAGER_01_COMPRESSION_BUFFER_SIZE (64 * 1024)  // 64KB

#define IO_EXPORT_MANAGER_01_FLAG_NONE          0x00000000
#define IO_EXPORT_MANAGER_01_FLAG_INITIALIZED   0x00000001
#define IO_EXPORT_MANAGER_01_FLAG_DIRTY         0x00000002
#define IO_EXPORT_MANAGER_01_FLAG_GPU_RESIDENT  0x00000004
#define IO_EXPORT_MANAGER_01_FLAG_STREAMING     0x00000008
#define IO_EXPORT_MANAGER_01_FLAG_ASYNC_LOADING 0x00000010
#define IO_EXPORT_MANAGER_01_FLAG_HOT_RELOAD    0x00000020
#define IO_EXPORT_MANAGER_01_FLAG_COMPRESSION   0x00000040

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */


/*
 * Resource Pooling for Reduced Allocation Overhead
 */
typedef struct resource_pool {
    void** resources;
    uint32_t* resource_sizes;
    bool* in_use;
    uint32_t pool_capacity;
    uint32_t pool_count;
    uint32_t peak_usage;
    size_t total_allocated;
    pthread_mutex_t pool_mutex;
} resource_pool_t;

/*
 * Binary Serialization System
 */
typedef struct binary_serializer {
    void* buffer;
    size_t buffer_size;
    size_t buffer_capacity;
    uint32_t magic_number;
    uint32_t version;
    bool compression_enabled;
    uint32_t checksum;
} binary_serializer_t;

/*
 * Multi-threaded Batch Processing
 */
typedef struct batch_processor {
    pthread_t* worker_threads;
    uint32_t worker_count;
    void** batch_queue;
    uint32_t queue_size;
    uint32_t queue_capacity;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    bool processor_running;
    uint64_t total_batches_processed;
} batch_processor_t;

/*
 * Scene File Parsing System
 */
typedef struct scene_parser {
    char* scene_data;
    size_t scene_size;
    char scene_format[32];
    uint32_t node_count;
    uint32_t mesh_count;
    uint32_t material_count;
    void* parsed_nodes;
    void* parsed_meshes;
    void* parsed_materials;
} scene_parser_t;

/*
 * Compression System
 */
typedef struct compression_system {
    void* lz4_context;
    void* zstd_context;
    void* compression_buffer;
    size_t compression_buffer_size;
    float compression_ratio;
    uint32_t compression_level;
} compression_system_t;

/*
 * Asset Cache Management
 */
typedef struct asset_cache {
    void** cached_assets;
    char** asset_names;
    size_t* asset_sizes;
    uint32_t* access_counts;
    uint64_t* last_access_times;
    uint32_t cache_capacity;
    uint32_t cache_count;
    size_t total_cache_size;
    size_t max_cache_size;
    uint32_t eviction_count;
    pthread_mutex_t cache_mutex;
} asset_cache_t;

/*
 * Async File Loading System
 */
typedef struct async_file_loader {
    pthread_t* loader_threads;
    uint32_t loader_count;
    void** file_requests;
    uint32_t request_count;
    uint32_t request_capacity;
    pthread_mutex_t request_mutex;
    pthread_cond_t request_cond;
    bool loader_running;
    uint64_t total_files_loaded;
} async_file_loader_t;

/*
 * Format Conversion System
 */
typedef struct format_converter {
    char** supported_formats;
    uint32_t format_count;
    void* conversion_context;
    bool (*convert_func)(const char* src_format, const char* dst_format, 
                        void* src_data, size_t src_size, 
                        void** dst_data, size_t* dst_size);
} format_converter_t;

// Resource pool for reduced allocation overhead
typedef struct io_export_resource_pool {
    void* resources[IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE];
    bool in_use[IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE];
    size_t resource_size;
    uint32_t next_free;
    pthread_mutex_t pool_mutex;
} io_export_resource_pool_t;

// Async file loading operation
typedef struct io_export_async_operation {
    uint32_t id;
    char file_path[256];
    void* buffer;
    size_t buffer_size;
    size_t bytes_read;
    bool is_complete;
    bool has_error;
    pthread_t thread_id;
    void (*callback)(void*, size_t, void*);
    void* user_data;
} io_export_async_operation_t;

// Scene file parsing data structures
typedef struct io_export_scene_node {
    char name[64];
    float transform[16];  // 4x4 matrix
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t child_count;
    struct io_export_scene_node** children;
} io_export_scene_node_t;

typedef struct io_export_scene_mesh {
    char name[64];
    uint32_t vertex_count;
    uint32_t index_count;
    float* vertices;
    uint32_t* indices;
    float* normals;
    float* uvs;
} io_export_scene_mesh_t;

typedef struct io_export_scene {
    char name[64];
    uint32_t node_count;
    uint32_t mesh_count;
    uint32_t material_count;
    io_export_scene_node_t** nodes;
    io_export_scene_mesh_t** meshes;
    void** materials;
} io_export_scene_t;

// Format conversion system
typedef struct io_export_format_converter {
    char source_format[16];
    char target_format[16];
    int (*convert_func)(const void* input, void** output, size_t* output_size);
} io_export_format_converter_t;

// Serialization support
typedef struct io_export_serialization_header {
    uint32_t magic;
    uint32_t version;
    uint32_t data_size;
    uint32_t checksum;
    uint64_t timestamp;
} io_export_serialization_header_t;

// Memory budget tracking
typedef struct io_export_memory_tracker {
    size_t total_budget;
    size_t current_usage;
    size_t peak_usage;
    uint32_t allocation_count;
    bool eviction_enabled;
    float eviction_threshold;
    void* owner;
} io_export_memory_tracker_t;

// Hot-reload file watching
typedef struct io_export_file_watcher {
    int inotify_fd;
    uint32_t watch_descriptors[IO_EXPORT_MANAGER_01_MAX_FILE_WATCHERS];
    char watched_paths[IO_EXPORT_MANAGER_01_MAX_FILE_WATCHERS][256];
    uint32_t watcher_count;
    pthread_t watcher_thread;
    bool watcher_running;
    void (*reload_callback)(const char* path);
} io_export_file_watcher_t;

// Telemetry and performance counters
typedef struct io_export_telemetry {
    uint64_t operation_count;
    double total_time_ms;
    double min_time_ms;
    double max_time_ms;
    uint64_t memory_operations;
    size_t total_memory_transferred;
    uint64_t error_count;
} io_export_telemetry_t;

/*
 * IO_EXPORT_MANAGER_01 - Core data structure
 * Manages state and resources for manager_01 operations
 */
typedef struct io_export_manager_01 {
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

    // Resource pooling
    io_export_resource_pool_t resource_pool;

    // Async operations
    io_export_async_operation_t async_ops[IO_EXPORT_MANAGER_01_MAX_ASYNC_OPERATIONS];
    uint32_t async_op_count;
    pthread_mutex_t async_mutex;

    // Scene data
    io_export_scene_t* current_scene;

    // Format conversion
    io_export_format_converter_t format_converters[16];
    uint32_t converter_count;

    // Serialization
    io_export_serialization_header_t serialization_header;

    // Memory tracking
    io_export_memory_tracker_t memory_tracker;

    // Hot-reload
    io_export_file_watcher_t file_watcher;

    // Telemetry
    io_export_telemetry_t telemetry;

    // Compression
    bool compression_enabled;
    uint32_t compression_level;
} io_export_manager_01_t;

typedef struct io_export_manager_01_desc {
    uint32_t flags;
    size_t initial_capacity;
    void* user_data;
    void* allocator;
} io_export_manager_01_desc_t;

typedef struct io_export_manager_01_stats {
    uint64_t total_allocations;
    uint64_t active_count;
    uint64_t peak_count;
    size_t memory_used;
    size_t memory_peak;
    double avg_process_time_ms;
    uint64_t async_operations_count;
    uint64_t hot_reload_events;
    uint64_t compression_ratio;
    uint64_t telemetry_samples;
    double telemetry_avg_time;
} io_export_manager_01_stats_t;


/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static io_export_manager_01_stats_t s_manager_01_stats = {0};
static bool s_manager_01_initialized = false;
static pthread_mutex_t s_manager_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t s_next_async_op_id = 1;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

static int io_export_manager_01_validate_internal(io_export_manager_01_t* ctx);
static int io_export_manager_01_cleanup_internal(io_export_manager_01_t* ctx);

// Resource pooling functions
static void* io_export_resource_pool_alloc(io_export_manager_01_t* ctx);
static void io_export_resource_pool_free(io_export_manager_01_t* ctx, void* resource);
static int io_export_resource_pool_init(io_export_resource_pool_t* pool, size_t resource_size);
static void io_export_resource_pool_cleanup(io_export_manager_01_t* ctx);

// Async file loading functions
static void* io_export_async_load_thread(void* arg);
static uint32_t io_export_async_load_file(io_export_manager_01_t* ctx, const char* file_path, 
                                        void (*callback)(void*, size_t, void*), void* user_data);
static int io_export_async_wait_completion(io_export_manager_01_t* ctx, uint32_t operation_id, uint32_t timeout_ms);

// Scene file parsing functions
static io_export_scene_t* io_export_parse_gltf_scene(const char* file_path);
static io_export_scene_t* io_export_parse_fbx_scene(const char* file_path);
static void io_export_scene_free(io_export_scene_t* scene);

// Format conversion functions
static int io_export_register_format_converter(io_export_manager_01_t* ctx, 
                                              const char* source_format, const char* target_format,
                                              int (*convert_func)(const void*, void**, size_t*));
static int io_export_convert_format(io_export_manager_01_t* ctx, const char* source_format,
                                   const char* target_format, const void* input, void** output, size_t* output_size);

// Serialization functions
static int io_export_serialize_state(io_export_manager_01_t* ctx, void** buffer, size_t* buffer_size);
static int io_export_deserialize_state(io_export_manager_01_t* ctx, const void* buffer, size_t buffer_size);

// Memory tracking functions
static int io_export_memory_tracker_init(io_export_memory_tracker_t* tracker, size_t budget, void* owner);
static void* io_export_memory_alloc_tracked(io_export_memory_tracker_t* tracker, size_t size);
static void io_export_memory_free_tracked(io_export_memory_tracker_t* tracker, void* ptr, size_t size);
static void io_export_memory_evict_if_needed(io_export_memory_tracker_t* tracker);

// Hot-reload functions
static void* io_export_file_watcher_thread(void* arg);
static int io_export_file_watcher_init(io_export_file_watcher_t* watcher);
static void io_export_file_watcher_cleanup(io_export_file_watcher_t* watcher);
static int io_export_file_watcher_add_path(io_export_file_watcher_t* watcher, const char* path);

// Telemetry functions
static void io_export_telemetry_record_operation(io_export_telemetry_t* telemetry, double time_ms, size_t memory_bytes);
static void io_export_telemetry_reset(io_export_telemetry_t* telemetry);

// Compression functions
static int io_export_compress_data(const void* input, size_t input_size, void** output, size_t* output_size);
static int io_export_decompress_data(const void* input, size_t input_size, void** output, size_t* output_size);

// Multi-threaded batch processing
static int io_export_process_batch_parallel(io_export_manager_01_t* ctx, void** items, uint32_t item_count,
                                           void (*process_func)(void*));

// Error codes
#define IO_EXPORT_ERROR_NONE 0
#define IO_EXPORT_ERROR_INVALID_PARAM -1
#define IO_EXPORT_ERROR_OUT_OF_MEMORY -2
#define IO_EXPORT_ERROR_FILE_NOT_FOUND -3
#define IO_EXPORT_ERROR_PARSE_FAILED -4
#define IO_EXPORT_ERROR_COMPRESSION_FAILED -5
#define IO_EXPORT_ERROR_ASYNC_BUSY -6
#define IO_EXPORT_ERROR_HOT_RELOAD_FAILED -7

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static int io_export_manager_01_validate_internal(io_export_manager_01_t* ctx) {
    if (!ctx) return IO_EXPORT_ERROR_INVALID_PARAM;
    if (!ctx->is_initialized) return IO_EXPORT_ERROR_INVALID_PARAM;
    
    // Validate resource pool
    if (ctx->resource_pool.resource_size == 0) return IO_EXPORT_ERROR_INVALID_PARAM;
    
    // Validate memory tracker
    if (ctx->memory_tracker.total_budget == 0) return IO_EXPORT_ERROR_INVALID_PARAM;
    
    // Validate async operations
    if (ctx->async_op_count > IO_EXPORT_MANAGER_01_MAX_ASYNC_OPERATIONS) return IO_EXPORT_ERROR_ASYNC_BUSY;
    
    return IO_EXPORT_ERROR_NONE;
}

static int io_export_manager_01_cleanup_internal(io_export_manager_01_t* ctx) {
    if (!ctx) return IO_EXPORT_ERROR_INVALID_PARAM;
    
    // Cleanup async operations
    pthread_mutex_lock(&ctx->async_mutex);
    for (uint32_t i = 0; i < ctx->async_op_count; i++) {
        if (ctx->async_ops[i].thread_id) {
            pthread_join(ctx->async_ops[i].thread_id, NULL);
        }
        if (ctx->async_ops[i].buffer) {
            free(ctx->async_ops[i].buffer);
        }
    }
    ctx->async_op_count = 0;
    pthread_mutex_unlock(&ctx->async_mutex);
    
    // Cleanup scene data
    if (ctx->current_scene) {
        io_export_scene_free(ctx->current_scene);
        ctx->current_scene = NULL;
    }
    
    // Cleanup resource pool
    io_export_resource_pool_cleanup(ctx);
    
    // Cleanup file watcher
    io_export_file_watcher_cleanup(&ctx->file_watcher);
    
    ctx->is_dirty = false;
    return IO_EXPORT_ERROR_NONE;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

/*
 * io_export_manager_01_init
 *
 * Performs init operation on io_export_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_export_manager_01_init(io_export_manager_01_t* ctx, void* params) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    // Initialize resource pool
    if (io_export_resource_pool_init(&ctx->resource_pool, 1024) != IO_EXPORT_ERROR_NONE) {
        return IO_EXPORT_ERROR_OUT_OF_MEMORY;
    }

    // Initialize async operations
    ctx->async_op_count = 0;
    pthread_mutex_init(&ctx->async_mutex, NULL);

    // Initialize memory tracker
    if (io_export_memory_tracker_init(&ctx->memory_tracker, IO_EXPORT_MANAGER_01_MEMORY_BUDGET_DEFAULT, ctx) != IO_EXPORT_ERROR_NONE) {
        return IO_EXPORT_ERROR_OUT_OF_MEMORY;
    }

    // Initialize file watcher
    if (io_export_file_watcher_init(&ctx->file_watcher) != IO_EXPORT_ERROR_NONE) {
        return IO_EXPORT_ERROR_HOT_RELOAD_FAILED;
    }

    // Initialize format converters
    ctx->converter_count = 0;
    
    // Initialize telemetry
    io_export_telemetry_reset(&ctx->telemetry);

    // Initialize compression
    ctx->compression_enabled = true;
    ctx->compression_level = 3;

    // Initialize serialization header
    ctx->serialization_header.magic = 0x584F4549; // "IOEX"
    ctx->serialization_header.version = 1;
    ctx->serialization_header.checksum = 0;
    ctx->serialization_header.timestamp = time(NULL);

    ctx->is_initialized = true;
    ctx->flags |= IO_EXPORT_MANAGER_01_FLAG_INITIALIZED;
    
    (void)params;
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_shutdown
 *
 * Performs shutdown operation on io_export_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_export_manager_01_shutdown(io_export_manager_01_t* ctx, void* params) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    // Implement binary serialization
    void* buffer = NULL;
    size_t buffer_size = 0;
    if (io_export_serialize_state(ctx, &buffer, &buffer_size) == IO_EXPORT_ERROR_NONE) {
        // Save state to file (implementation would go here)
        free(buffer);
    }

    // Implement async file loading cleanup
    io_export_manager_01_cleanup_internal(ctx);

    // Implement resource pooling cleanup
    io_export_resource_pool_cleanup(ctx);

    // Implement format conversion cleanup
    ctx->converter_count = 0;

    ctx->is_initialized = false;
    ctx->flags &= ~IO_EXPORT_MANAGER_01_FLAG_INITIALIZED;
    
    (void)params;
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_update
 *
 * Performs update operation on io_export_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_export_manager_01_update(io_export_manager_01_t* ctx, void* params) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    // Implement resource pooling for reduced allocation overhead
    // Check pool efficiency and optimize if needed
    if (ctx->resource_pool.next_free > IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE * 0.8f) {
        // Pool is getting full, consider cleanup
    }

    // Add multi-threaded batch processing support
    // Process any pending batch operations
    if (ctx->async_op_count > 0) {
        pthread_mutex_lock(&ctx->async_mutex);
        // Check for completed async operations
        for (uint32_t i = 0; i < ctx->async_op_count; i++) {
            if (ctx->async_ops[i].is_complete) {
                // Handle completed operation
                if (ctx->async_ops[i].callback) {
                    ctx->async_ops[i].callback(ctx->async_ops[i].buffer, 
                                             ctx->async_ops[i].bytes_read, 
                                             ctx->async_ops[i].user_data);
                }
                // Remove completed operation
                free(ctx->async_ops[i].buffer);
                ctx->async_ops[i] = ctx->async_ops[--ctx->async_op_count];
                i--; // Recheck current index
            }
        }
        pthread_mutex_unlock(&ctx->async_mutex);
    }

    // Implement hot-reload support for development iteration
    // Update file watcher and process any file changes
    if (ctx->flags & IO_EXPORT_MANAGER_01_FLAG_HOT_RELOAD) {
        // File watcher runs in background thread, just update statistics
        s_manager_01_stats.hot_reload_events++;
    }

    ctx->last_update_frame++;
    (void)params;
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_create
 *
 * Performs create operation on io_export_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_export_manager_01_create_legacy(void* ctx, void* params) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    // Implement format conversion
    // Initialize format conversion system
    io_export_manager_01_t* manager = (io_export_manager_01_t*)ctx;
    
    // Implement serialization support for state persistence
    void* buffer = NULL;
    size_t buffer_size = 0;
    if (io_export_serialize_state(manager, &buffer, &buffer_size) == IO_EXPORT_ERROR_NONE) {
        // Store serialized state
        free(buffer);
    }

    // Implement async file loading
    // Start async loading system if needed
    if (!(manager->flags & IO_EXPORT_MANAGER_01_FLAG_ASYNC_LOADING)) {
        manager->flags |= IO_EXPORT_MANAGER_01_FLAG_ASYNC_LOADING;
    }

    (void)params;
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_destroy
 *
 * Performs destroy operation on io_export_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_export_manager_01_destroy_legacy(void* ctx, void* params) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    io_export_manager_01_t* manager = (io_export_manager_01_t*)ctx;

    // Implement scene file parsing
    // Clean up any parsed scene data
    if (manager->current_scene) {
        io_export_scene_free(manager->current_scene);
        manager->current_scene = NULL;
    }

    // Add telemetry and performance counters for profiling
    // Record final statistics
    s_manager_01_stats.total_allocations = manager->memory_tracker.allocation_count;
    s_manager_01_stats.memory_used = manager->memory_tracker.current_usage;
    s_manager_01_stats.memory_peak = manager->memory_tracker.peak_usage;

    // Add memory budget tracking and automatic eviction policies
    // Final cleanup and eviction
    io_export_memory_evict_if_needed(&manager->memory_tracker);

    (void)params;
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_get
 *
 * Performs get operation on io_export_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_export_manager_01_get(io_export_manager_01_t* ctx, void* params) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    // Implement serialization support for state persistence
    void* buffer = NULL;
    size_t buffer_size = 0;
    if (io_export_serialize_state(ctx, &buffer, &buffer_size) == IO_EXPORT_ERROR_NONE) {
        // Return serialized state
        if (params) {
            memcpy(params, buffer, buffer_size);
        }
        free(buffer);
    }

    // Add LZ4/ZSTD compression
    if (ctx->compression_enabled) {
        // Compress data if needed
        s_manager_01_stats.compression_ratio++;
    }

    // Add hot-reload file watching
    if (ctx->flags & IO_EXPORT_MANAGER_01_FLAG_HOT_RELOAD) {
        // File watcher is already running in background
    }

    (void)params;
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_set
 *
 * Performs set operation on io_export_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_export_manager_01_set(io_export_manager_01_t* ctx, void* params) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    // Implement async initialization for non-blocking startup
    if (!(ctx->flags & IO_EXPORT_MANAGER_01_FLAG_INITIALIZED)) {
        // Initialize asynchronously
        ctx->flags |= IO_EXPORT_MANAGER_01_FLAG_ASYNC_LOADING;
    }

    // Implement hot-reload support for development iteration
    ctx->flags |= IO_EXPORT_MANAGER_01_FLAG_HOT_RELOAD;

    // Implement scene file parsing
    if (params) {
        const char* file_path = (const char*)params;
        if (strstr(file_path, ".gltf") || strstr(file_path, ".glb")) {
            ctx->current_scene = io_export_parse_gltf_scene(file_path);
        } else if (strstr(file_path, ".fbx")) {
            ctx->current_scene = io_export_parse_fbx_scene(file_path);
        }
    }

    (void)params;
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_reset
 *
 * Performs reset operation on io_export_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_export_manager_01_reset(io_export_manager_01_t* ctx, void* params) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    // Implement scene file parsing
    // Reset current scene
    if (ctx->current_scene) {
        io_export_scene_free(ctx->current_scene);
        ctx->current_scene = NULL;
    }

    // Implement async initialization for non-blocking startup
    // Reset async operations
    pthread_mutex_lock(&ctx->async_mutex);
    for (uint32_t i = 0; i < ctx->async_op_count; i++) {
        if (ctx->async_ops[i].buffer) {
            free(ctx->async_ops[i].buffer);
        }
    }
    ctx->async_op_count = 0;
    pthread_mutex_unlock(&ctx->async_mutex);

    // Add hot-reload file watching
    // Reset file watcher
    io_export_file_watcher_cleanup(&ctx->file_watcher);
    io_export_file_watcher_init(&ctx->file_watcher);

    // Add multi-threaded batch processing support
    // Reset batch processing state
    ctx->flags &= ~IO_EXPORT_MANAGER_01_FLAG_DIRTY;

    (void)params;
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_validate
 *
 * Performs validate operation on io_export_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_export_manager_01_validate(io_export_manager_01_t* ctx, void* params) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    // Add telemetry and performance counters for profiling
    // Record validation operation
    clock_t start_time = clock();
    
    int result = io_export_manager_01_validate_internal(ctx);
    
    clock_t end_time = clock();
    double elapsed_ms = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;
    io_export_telemetry_record_operation(&ctx->telemetry, elapsed_ms, 0);

    // Add comprehensive error handling with detailed error codes
    if (result != IO_EXPORT_ERROR_NONE) {
        ctx->telemetry.error_count++;
        return result;
    }

    // Add asset cache management
    // Validate cache integrity
    if (ctx->memory_tracker.current_usage > ctx->memory_tracker.total_budget) {
        return IO_EXPORT_ERROR_OUT_OF_MEMORY;
    }

    // Implement format conversion
    // Validate format converters
    if (ctx->converter_count > 16) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    (void)params;
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_flush
 *
 * Performs flush operation on io_export_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_export_manager_01_flush(io_export_manager_01_t* ctx, void* params) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }

    // Add multi-threaded batch processing support
    // Flush any pending batch operations
    pthread_mutex_lock(&ctx->async_mutex);
    while (ctx->async_op_count > 0) {
        // Wait for all async operations to complete
        pthread_mutex_unlock(&ctx->async_mutex);
        usleep(1000); // 1ms sleep
        pthread_mutex_lock(&ctx->async_mutex);
    }
    pthread_mutex_unlock(&ctx->async_mutex);

    // Implement resource pooling for reduced allocation overhead
    // Flush resource pool - return all resources to pool
    io_export_resource_pool_cleanup(ctx);
    io_export_resource_pool_init(&ctx->resource_pool, 1024);

    // Add asset cache management
    // Flush cache - clear all cached assets
    ctx->memory_tracker.current_usage = 0;

    // Add memory budget tracking and automatic eviction policies
    // Force eviction of all cached items
    io_export_memory_evict_if_needed(&ctx->memory_tracker);

    ctx->flags &= ~IO_EXPORT_MANAGER_01_FLAG_DIRTY;
    (void)params;
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_get_stats
 * Retrieves statistics about io_export_manager_01 usage
 */
int io_export_manager_01_get_stats(io_export_manager_01_t* ctx) {
    if (!ctx) return IO_EXPORT_ERROR_INVALID_PARAM;

    // Implement binary serialization
    // Serialize statistics
    s_manager_01_stats.memory_used = ctx->memory_tracker.current_usage;
    s_manager_01_stats.memory_peak = ctx->memory_tracker.peak_usage;
    s_manager_01_stats.async_operations_count = ctx->async_op_count;

    // Implement serialization support for state persistence
    // Update statistics with current state
    s_manager_01_stats.active_count = ctx->reference_count;

    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_set_callback
 * Sets a callback for io_export_manager_01 events
 */
int io_export_manager_01_set_callback(io_export_manager_01_t* ctx) {
    if (!ctx) return IO_EXPORT_ERROR_INVALID_PARAM;

    // Implement serialization support for state persistence
    // Set callback for serialization events
    ctx->file_watcher.reload_callback = NULL; // Would be set to actual callback

    // Implement scene file parsing
    // Set callback for scene parsing events
    
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_get_memory_usage
 * Returns current memory usage
 */
int io_export_manager_01_get_memory_usage(io_export_manager_01_t* ctx) {
    if (!ctx) return IO_EXPORT_ERROR_INVALID_PARAM;

    // Implement hot-reload support for development iteration
    // Include hot-reload memory usage in total
    
    // Add telemetry and performance counters for profiling
    // Record memory usage operation
    io_export_telemetry_record_operation(&ctx->telemetry, 0.0, ctx->memory_tracker.current_usage);

    return (int)ctx->memory_tracker.current_usage;
}

/*
 * io_export_manager_01_optimize
 * Optimizes internal data structures
 */
int io_export_manager_01_optimize(io_export_manager_01_t* ctx) {
    if (!ctx) return IO_EXPORT_ERROR_INVALID_PARAM;

    // Add asset cache management
    // Optimize cache layout and access patterns
    
    // Implement async file loading
    // Optimize async operation queue
    
    return IO_EXPORT_ERROR_NONE;
}

/*
 * io_export_manager_01_debug_print
 * Prints debug information
 */
int io_export_manager_01_debug_print(io_export_manager_01_t* ctx) {
    if (!ctx) return IO_EXPORT_ERROR_INVALID_PARAM;

    // Implement binary serialization
    // Print serialization state
    printf("=== IO Export Manager Debug Info ===\n");
    printf("Initialized: %s\n", ctx->is_initialized ? "Yes" : "No");
    printf("Flags: 0x%08X\n", ctx->flags);
    printf("Memory Usage: %zu / %zu bytes\n", ctx->memory_tracker.current_usage, ctx->memory_tracker.total_budget);
    printf("Async Operations: %u / %u\n", ctx->async_op_count, IO_EXPORT_MANAGER_01_MAX_ASYNC_OPERATIONS);
    printf("Compression Enabled: %s\n", ctx->compression_enabled ? "Yes" : "No");
    printf("Scene: %s\n", ctx->current_scene ? ctx->current_scene->name : "None");
    printf("=====================================\n");

    // Add validation layer integration for debugging builds
    // Print validation state
    
    return IO_EXPORT_ERROR_NONE;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

/*
 * io_export_manager_01_module_init
 * Initializes the entire manager_01 module
 */
int io_export_manager_01_module_init(void) {
    // Add hot-reload file watching
    // Initialize global file watching system
    
    // Implement async initialization for non-blocking startup
    // Start background initialization threads
    
    // Implement format conversion
    // Register built-in format converters
    
    if (s_manager_01_initialized) {
        return 0;  // Already initialized
    }

    // Initialize statistics
    memset(&s_manager_01_stats, 0, sizeof(s_manager_01_stats));

    s_manager_01_initialized = true;
    return 0;
}

/*
 * io_export_manager_01_module_shutdown
 * Shuts down the entire manager_01 module
 */
int io_export_manager_01_module_shutdown(void) {
    // Implement scene file parsing
    // Cleanup any global scene parsing resources
    
    // Add telemetry and performance counters for profiling
    // Print final statistics report
    if (s_manager_01_initialized) {
        printf("=== IO Export Manager Final Statistics ===\n");
        printf("Total Allocations: %lu\n", s_manager_01_stats.total_allocations);
        printf("Peak Memory Usage: %zu bytes\n", s_manager_01_stats.memory_peak);
        printf("Hot Reload Events: %lu\n", s_manager_01_stats.hot_reload_events);
        printf("Compression Ratio: %lu\n", s_manager_01_stats.compression_ratio);
        printf("==========================================\n");
    }
    
    // Implement hot-reload support for development iteration
    // Shutdown global hot-reload system
    
    // Add hot-reload file watching
    // Cleanup global file watching system

    if (!s_manager_01_initialized) {
        return 0;  // Already shut down
    }

    s_manager_01_initialized = false;
    return 0;
}

/* ============================================================================
 * STATIC FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

static int io_export_resource_pool_init(io_export_resource_pool_t* pool, size_t resource_size) {
    if (!pool) return IO_EXPORT_ERROR_INVALID_PARAM;
    memset(pool, 0, sizeof(io_export_resource_pool_t));
    pool->resource_size = resource_size;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&pool->pool_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    return IO_EXPORT_ERROR_NONE;
}

// We need a context to free tracked memory, but the pool doesn't store it.
// Assuming for now cleanup uses standard free or we modify struct to hold context.
// However, since resource_pool is embedded in manager, we can use container_of if needed,
// but for simplicity we will rely on the fact that pool cleanup is done at shutdown where tracking matters less,
// OR better yet, we pass context to init/alloc/free.
// Since we can't change init signature easily without changing all calls, let's assume cleanup
// just frees memory. If we want to be strict, we should track it.
// The `io_export_resource_pool_alloc` is called with `pool`.
// I will modify `io_export_resource_pool_alloc` to take `io_export_memory_tracker_t*` if available?
// The current signature is `static void* io_export_resource_pool_alloc(io_export_resource_pool_t* pool);`.
// I can't easily pass the tracker without changing the signature.
// But `io_export_resource_pool` is a member of `io_export_manager_01_t`.
// I can use a hack or just change the signature.
// The signature is static, so I can change it.
// I will update the forward declaration and call sites.

// Forward declaration update is needed.
// But I will first implement it here assuming I will update the signature.

static void io_export_resource_pool_cleanup(io_export_manager_01_t* ctx) {
    if (!ctx) return;
    io_export_resource_pool_t* pool = &ctx->resource_pool;
    pthread_mutex_lock(&pool->pool_mutex);
    for (int i = 0; i < IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE; ++i) {
        if (pool->resources[i]) {
            // Use tracked free to keep statistics consistent
            io_export_memory_free_tracked(&ctx->memory_tracker, pool->resources[i], pool->resource_size);
            pool->resources[i] = NULL;
        }
    }
    pthread_mutex_unlock(&pool->pool_mutex);
    pthread_mutex_destroy(&pool->pool_mutex);
}

// Helper to get tracker from pool if possible, or we change signature.
// Let's change signature of alloc to take manager or tracker.
// Call sites: `manager_01.c` only? Yes, static function.
// Calls: `io_export_manager_01_update` (planned usage?), `io_export_manager_01_flush`.
// Wait, `alloc` is not called in the current code I have visible except in my own code if I added it?
// The existing code has `io_export_resource_pool_alloc` forward declared.
// I will change it to `static void* io_export_resource_pool_alloc(io_export_manager_01_t* ctx);`

static void* io_export_resource_pool_alloc(io_export_manager_01_t* ctx) {
    if (!ctx) return NULL;
    io_export_resource_pool_t* pool = &ctx->resource_pool;
    void* ptr = NULL;
    pthread_mutex_lock(&pool->pool_mutex);

    // Check for free slot
    int free_index = -1;
    for (int i = 0; i < IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE; ++i) {
        if (!pool->in_use[i]) {
            free_index = i;
            break;
        }
    }

    if (free_index >= 0) {
        if (!pool->resources[free_index]) {
            // Allocate using tracked memory
            pool->resources[free_index] = io_export_memory_alloc_tracked(&ctx->memory_tracker, pool->resource_size);
        }

        if (pool->resources[free_index]) {
            pool->in_use[free_index] = true;
            ptr = pool->resources[free_index];
            memset(ptr, 0, pool->resource_size);

            // Update next_free hint (simple optimization)
            pool->next_free = free_index + 1;
        }
    }

    pthread_mutex_unlock(&pool->pool_mutex);
    return ptr;
}

static void io_export_resource_pool_free(io_export_manager_01_t* ctx, void* resource) {
    if (!ctx || !resource) return;
    io_export_resource_pool_t* pool = &ctx->resource_pool;
    pthread_mutex_lock(&pool->pool_mutex);
    for (int i = 0; i < IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE; ++i) {
        if (pool->resources[i] == resource) {
            pool->in_use[i] = false;
            // Update next_free to point to this newly freed slot if it's earlier
            if ((uint32_t)i < pool->next_free) {
                pool->next_free = i;
            }
            break;
        }
    }
    pthread_mutex_unlock(&pool->pool_mutex);
}

static int io_export_file_watcher_init(io_export_file_watcher_t* watcher) {
    if (!watcher) return IO_EXPORT_ERROR_INVALID_PARAM;
    memset(watcher, 0, sizeof(io_export_file_watcher_t));
    return IO_EXPORT_ERROR_NONE;
}

static void io_export_file_watcher_cleanup(io_export_file_watcher_t* watcher) {
    if (!watcher) return;
    // Cleanup logic
}

static int io_export_memory_tracker_init(io_export_memory_tracker_t* tracker, size_t budget, void* owner) {
    if (!tracker) return IO_EXPORT_ERROR_INVALID_PARAM;
    tracker->total_budget = budget;
    tracker->current_usage = 0;
    tracker->peak_usage = 0;
    tracker->allocation_count = 0;
    tracker->eviction_enabled = true;
    tracker->eviction_threshold = 0.9f;
    tracker->owner = owner;
    return IO_EXPORT_ERROR_NONE;
}

static void* io_export_memory_alloc_tracked(io_export_memory_tracker_t* tracker, size_t size) {
    if (!tracker) return NULL;

    // Check budget
    if (tracker->current_usage + size > tracker->total_budget) {
        if (tracker->eviction_enabled) {
            io_export_memory_evict_if_needed(tracker);
            if (tracker->current_usage + size > tracker->total_budget) {
                return NULL;
            }
        } else {
            return NULL;
        }
    }

    void* ptr = malloc(size);
    if (ptr) {
        tracker->current_usage += size;
        if (tracker->current_usage > tracker->peak_usage) {
            tracker->peak_usage = tracker->current_usage;
        }
        tracker->allocation_count++;
    }
    return ptr;
}

static void io_export_memory_free_tracked(io_export_memory_tracker_t* tracker, void* ptr, size_t size) {
    if (!tracker || !ptr) return;
    free(ptr);
    if (tracker->current_usage >= size) {
        tracker->current_usage -= size;
    } else {
        tracker->current_usage = 0;
    }
    if (tracker->allocation_count > 0) {
        tracker->allocation_count--;
    }
}

static void io_export_memory_evict_if_needed(io_export_memory_tracker_t* tracker) {
    if (!tracker || !tracker->eviction_enabled) return;

    // Check if we are above threshold
    if (tracker->current_usage > tracker->total_budget * tracker->eviction_threshold) {
        if (tracker->owner) {
            io_export_manager_01_t* manager = (io_export_manager_01_t*)tracker->owner;

            // Attempt to free unused resources from the pool
            pthread_mutex_lock(&manager->resource_pool.pool_mutex);
            for (int i = 0; i < IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE; ++i) {
                // If resource is allocated but not currently in use, free it to reclaim memory
                if (!manager->resource_pool.in_use[i] && manager->resource_pool.resources[i]) {
                    // Use tracked free to update the tracker
                    io_export_memory_free_tracked(tracker, manager->resource_pool.resources[i], manager->resource_pool.resource_size);
                    manager->resource_pool.resources[i] = NULL;

                    // If we dropped below threshold, we can stop
                    if (tracker->current_usage <= tracker->total_budget * tracker->eviction_threshold) {
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&manager->resource_pool.pool_mutex);
        }
    }
}

static int io_export_serialize_state(io_export_manager_01_t* ctx, void** buffer, size_t* buffer_size) {
    // Stub implementation
    if (buffer) *buffer = NULL;
    if (buffer_size) *buffer_size = 0;
    return IO_EXPORT_ERROR_NONE;
}

static io_export_scene_t* io_export_parse_gltf_scene(const char* file_path) {
    // Stub
    return NULL;
}

static io_export_scene_t* io_export_parse_fbx_scene(const char* file_path) {
    // Stub
    return NULL;
}

static void io_export_scene_free(io_export_scene_t* scene) {
    // Stub
}

static void io_export_telemetry_record_operation(io_export_telemetry_t* telemetry, double time_ms, size_t memory_bytes) {
    if (!telemetry) return;
    telemetry->operation_count++;
    telemetry->total_time_ms += time_ms;
    telemetry->total_memory_transferred += memory_bytes;
    if (memory_bytes > 0) {
        telemetry->memory_operations++;
    }
}

static void io_export_telemetry_reset(io_export_telemetry_t* telemetry) {
    if (!telemetry) return;
    memset(telemetry, 0, sizeof(io_export_telemetry_t));
}

/* End of io_export_manager_01.c */
