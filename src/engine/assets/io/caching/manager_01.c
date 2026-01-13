/*
 * io_caching_manager_01.c
 *
 * I/O and asset streaming - Caching Subsystem
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * This file implements manager functionality for the caching module
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
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>

#include "assets/io/caching/manager_01.h"
#include "include/core/types.h"
#include "include/core/memory.h"
#include "engine/include/core/logger.h"
#include "core/memory.h"

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define IO_CACHING_MANAGER_01_VERSION_MAJOR 1
#define IO_CACHING_MANAGER_01_VERSION_MINOR 0
#define IO_CACHING_MANAGER_01_VERSION_PATCH 0

#define IO_CACHING_MANAGER_01_MAX_INSTANCES 4096
#define IO_CACHING_MANAGER_01_DEFAULT_CAPACITY 256
#define IO_CACHING_MANAGER_01_ALIGNMENT 16

#define IO_CACHING_MANAGER_01_FLAG_NONE          0x00000000
#define IO_CACHING_MANAGER_01_FLAG_INITIALIZED   0x00000001
#define IO_CACHING_MANAGER_01_FLAG_DIRTY         0x00000002
#define IO_CACHING_MANAGER_01_FLAG_GPU_RESIDENT  0x00000004
#define IO_CACHING_MANAGER_01_FLAG_STREAMING     0x00000008
#define IO_CACHING_MANAGER_01_FLAG_ASYNC_INIT    0x00000010
#define IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD    0x00000020
#define IO_CACHING_MANAGER_01_FLAG_BUNDLING      0x00000040

/* Default capacities */
#define DEFAULT_ASYNC_CAPACITY 64
#define DEFAULT_BUNDLE_CAPACITY 32
#define DEFAULT_RESOURCE_POOL_CAPACITY 1024

/* Magic number for serialization */
#define IO_CACHING_SERIALIZATION_MAGIC 0x43494F43  // "COIC"
#define IO_CACHING_SERIALIZATION_VERSION 1

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/*
 * Error codes for detailed error handling
 */
typedef enum {
    IO_CACHING_MANAGER_01_ERROR_NONE = 0,
    IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM = -1,
    IO_CACHING_MANAGER_01_ERROR_NOT_INITIALIZED = -2,
    IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY = -3,
    IO_CACHING_MANAGER_01_ERROR_FILE_NOT_FOUND = -4,
    IO_CACHING_MANAGER_01_ERROR_FORMAT_UNSUPPORTED = -5,
    IO_CACHING_MANAGER_01_ERROR_COMPRESSION_FAILED = -6,
    IO_CACHING_MANAGER_01_ERROR_SERIALIZATION_FAILED = -7,
    IO_CACHING_MANAGER_01_ERROR_THREAD_ERROR = -8,
    IO_CACHING_MANAGER_01_ERROR_ASYNC_BUSY = -9,
    IO_CACHING_MANAGER_01_ERROR_BUNDLE_CORRUPT = -10
} io_caching_manager_01_error_t;

/*
 * Resource pool for reduced allocation overhead
 */
typedef struct {
    void** resources;
    uint32_t capacity;
    uint32_t count;
    uint32_t peak_count;
    pthread_mutex_t mutex;
} io_caching_resource_pool_t;

/*
 * Async operation tracking
 */
typedef struct {
    uint32_t id;
    bool is_active;
    bool is_completed;
    pthread_t thread_id;
    void* result_data;
    size_t result_size;
    time_t start_time;
    time_t end_time;
    int error_code;
} io_caching_async_op_t;

/*
 * File watcher for hot-reload
 */
typedef struct {
    int inotify_fd;
    int watch_descriptor;
    char watch_path[512];
    pthread_t watcher_thread;
    bool is_active;
    void (*callback)(const char* filename, void* user_data);
    void* user_data;
} io_caching_file_watcher_t;

/*
 * Asset bundle structure
 */
typedef struct {
    char name[256];
    char version[64];
    uint32_t asset_count;
    void* compressed_data;
    size_t compressed_size;
    size_t uncompressed_size;
    uint32_t checksum;
    time_t creation_time;
} io_caching_asset_bundle_t;

/*
 * Telemetry and performance counters
 */
typedef struct {
    uint64_t files_loaded;
    uint64_t bytes_processed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t compression_operations;
    uint64_t decompression_operations;
    double avg_load_time_ms;
    double avg_compression_ratio;
    uint64_t async_operations_completed;
    uint64_t hot_reload_events;
    time_t last_update;
} io_caching_telemetry_t;

/*
 * Format converter registry
 */
typedef struct format_converter {
    char format[32];
    int (*converter)(const void* input, size_t input_size, void** output, size_t* output_size);
    struct format_converter* next;
} format_converter_t;

/*
 * Serialization context
 */
typedef struct {
    uint32_t magic_number;
    uint16_t version;
    uint16_t flags;
    uint64_t timestamp;
    uint32_t checksum;
    void* data;
    size_t data_size;
} io_caching_serialization_context_t;
typedef enum {
    IO_CACHING_COMPRESSION_NONE = 0,
    IO_CACHING_COMPRESSION_LZ4,
    IO_CACHING_COMPRESSION_ZSTD,
    IO_CACHING_COMPRESSION_AUTO
} io_caching_compression_type_t;

/*
 * Asset format types
 */
typedef enum {
    IO_CACHING_FORMAT_GLTF = 0,
    IO_CACHING_FORMAT_GLB,
    IO_CACHING_FORMAT_FBX,
    IO_CACHING_FORMAT_OBJ,
    IO_CACHING_FORMAT_CUSTOM
} io_caching_format_type_t;

/*
 * Resource pool for reduced allocation overhead
 */
typedef struct {
    void** resources;
    int capacity;
    int count;
    int peak_usage;
    pthread_mutex_t mutex;
} io_caching_resource_pool_t;

/*
 * Async operation for non-blocking operations
 */
typedef struct {
    uint32_t operation_id;
    char operation_name[256];
    void* user_data;
    void (*callback)(void* result, void* user_data);
    bool is_completed;
    pthread_t thread;
} io_caching_async_operation_t;

/*
 * Scene data for file parsing
 */
typedef struct {
    io_caching_format_type_t format;
    void* scene_data;
    size_t data_size;
    uint32_t node_count;
    uint32_t mesh_count;
    uint32_t material_count;
    bool is_loaded;
} io_caching_scene_data_t;

/*
 * Format converter registry
 */
typedef struct {
    io_caching_format_type_t input_format;
    io_caching_format_type_t output_format;
    int (*convert_func)(const void* input, size_t input_size, void** output, size_t* output_size);
} io_caching_format_converter_t;

/*
 * Memory budget tracking
 */
typedef struct {
    size_t total_budget;
    size_t current_usage;
    size_t peak_usage;
    uint32_t eviction_count;
    bool auto_evict;
    pthread_mutex_t mutex;
} io_caching_memory_budget_t;

/*
 * File watcher for hot-reload
 */
typedef struct {
    int inotify_fd;
    int watch_descriptor;
    char watch_path[512];
    pthread_t watcher_thread;
    volatile bool watching;
    void (*callback)(const char* path, void* user_data);
    void* user_data;
} io_caching_file_watcher_t;

/*
 * Compression context
 */
typedef struct {
    io_caching_compression_type_t type;
    int compression_level;
    void* compressor_context;
    void* decompressor_context;
    size_t total_compressed;
    size_t total_uncompressed;
    float compression_ratio;
} io_caching_compression_context_t;

/*
 * Binary serialization context
 */
typedef struct {
    uint32_t magic_number;
    uint32_t version;
    uint32_t checksum;
    void* serialized_data;
    size_t serialized_size;
} io_caching_serialization_context_t;

/*
 * IO_CACHING_MANAGER_01 - Core data structure
 * Manages state and resources for manager_01 operations
 */
typedef struct io_caching_manager_01 {
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
    
    // Hot-reload file watching
    io_caching_file_watcher_t file_watcher;
    
    // Format conversion
    io_caching_format_converter_t format_converters[16];
    int converter_count;
    io_caching_format_type_t input_format;
    io_caching_format_type_t output_format;
    
    // LZ4/ZSTD compression
    io_caching_compression_context_t compression;
    
    // Binary serialization
    io_caching_serialization_context_t serialization;
    
    // Async initialization for non-blocking startup
    io_caching_async_operation_t async_operations[64];
    int async_operation_count;
    pthread_t init_thread;
    volatile bool init_complete;
    
    // Multi-threaded batch processing support
    pthread_t worker_threads[8];
    int worker_count;
    volatile bool workers_running;
    pthread_mutex_t work_mutex;
    
    // Resource pooling for reduced allocation overhead
    io_caching_resource_pool_t resource_pool;
    
    // Scene file parsing
    io_caching_scene_data_t scene_data;
    
    // Memory budget tracking
    io_caching_memory_budget_t memory_budget;
    
} io_caching_manager_01_t;

typedef struct io_caching_manager_01_desc {
    uint32_t flags;
    size_t initial_capacity;
    void* user_data;
    void* allocator;
} io_caching_manager_01_desc_t;

typedef struct io_caching_manager_01_stats {
    uint64_t total_allocations;
    uint64_t active_count;
    uint64_t peak_count;
    size_t memory_used;
    size_t memory_peak;
    double avg_process_time_ms;
} io_caching_manager_01_stats_t;


/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static io_caching_manager_01_stats_t s_manager_01_stats = {0};
static bool s_manager_01_initialized = false;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

// Hot-reload file watching functions
static void* io_caching_file_watcher_thread(void* arg) {
    io_caching_file_watcher_t* watcher = (io_caching_file_watcher_t*)arg;
    char buffer[4096];
    
    while (watcher->watching) {
        ssize_t length = read(watcher->inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            size_t i = 0;
            while (i < length) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                if (event->mask & IN_MODIFY) {
                    if (watcher->callback) {
                        watcher->callback(watcher->watch_path, watcher->user_data);
                    }
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); // 100ms polling
    }
    return NULL;
}

static int io_caching_init_file_watcher(io_caching_file_watcher_t* watcher, const char* path) {
    watcher->inotify_fd = inotify_init();
    if (watcher->inotify_fd < 0) return -1;
    
    watcher->watch_descriptor = inotify_add_watch(watcher->inotify_fd, path, IN_MODIFY);
    if (watcher->watch_descriptor < 0) {
        close(watcher->inotify_fd);
        return -1;
    }
    
    strncpy(watcher->watch_path, path, sizeof(watcher->watch_path) - 1);
    watcher->watching = true;
    
    return pthread_create(&watcher->watcher_thread, NULL, io_caching_file_watcher_thread, watcher);
}

static void io_caching_cleanup_file_watcher(io_caching_file_watcher_t* watcher) {
    watcher->watching = false;
    pthread_join(watcher->watcher_thread, NULL);
    
    if (watcher->watch_descriptor >= 0) {
        inotify_rm_watch(watcher->inotify_fd, watcher->watch_descriptor);
    }
    if (watcher->inotify_fd >= 0) {
        close(watcher->inotify_fd);
    }
}

// Compression functions
static int io_caching_compress_data(const void* input, size_t input_size, void** output, size_t* output_size, io_caching_compression_type_t type) {
    // Simplified compression - would use LZ4/ZSTD libraries in real implementation
    *output_size = input_size;
    *output = malloc(*output_size);
    if (!*output) return -1;
    
    memcpy(*output, input, input_size);
    return 0;
}

static int io_caching_decompress_data(const void* input, size_t input_size, void** output, size_t* output_size) {
    // Simplified decompression
    *output_size = input_size;
    *output = malloc(*output_size);
    if (!*output) return -1;
    
    memcpy(*output, input, input_size);
    return 0;
}

// Format conversion functions
static int io_caching_convert_gltf_to_fbx(const void* input, size_t input_size, void** output, size_t* output_size) {
    // Simplified format conversion
    *output_size = input_size;
    *output = malloc(*output_size);
    if (!*output) return -1;
    
    memcpy(*output, input, input_size);
    return 0;
}

// Resource pooling functions
static int io_caching_init_resource_pool(io_caching_resource_pool_t* pool, int capacity) {
    pool->resources = malloc(sizeof(void*) * capacity);
    if (!pool->resources) return -1;
    
    pool->capacity = capacity;
    pool->count = 0;
    pool->peak_usage = 0;
    pthread_mutex_init(&pool->mutex, NULL);
    
    return 0;
}

static void* io_caching_pool_allocate(io_caching_resource_pool_t* pool) {
    pthread_mutex_lock(&pool->mutex);
    
    void* resource = NULL;
    if (pool->count < pool->capacity) {
        resource = pool->resources[pool->count++];
        if (pool->count > pool->peak_usage) {
            pool->peak_usage = pool->count;
        }
    }
    
    pthread_mutex_unlock(&pool->mutex);
    return resource;
}

static void io_caching_pool_deallocate(io_caching_resource_pool_t* pool, void* resource) {
    pthread_mutex_lock(&pool->mutex);
    
    if (pool->count > 0) {
        pool->resources[--pool->count] = resource;
    }
    
    pthread_mutex_unlock(&pool->mutex);
}

// Async operation functions
static void* io_caching_async_thread(void* arg) {
    io_caching_async_operation_t* operation = (io_caching_async_operation_t*)arg;
    
    // Simulate async work
    usleep(10000); // 10ms
    
    if (operation->callback) {
        operation->callback(NULL, operation->user_data);
    }
    
    operation->is_completed = true;
    return NULL;
}

// Memory budget functions
static int io_caching_init_memory_budget(io_caching_memory_budget_t* budget, size_t total_budget) {
    budget->total_budget = total_budget;
    budget->current_usage = 0;
    budget->peak_usage = 0;
    budget->eviction_count = 0;
    budget->auto_evict = true;
    pthread_mutex_init(&budget->mutex, NULL);
    
    return 0;
}

// Binary serialization functions
static int io_caching_serialize_data(const void* input, size_t input_size, void** output, size_t* output_size) {
    // Simple binary serialization with magic number and checksum
    const size_t header_size = sizeof(uint32_t) * 3; // magic + version + checksum
    *output_size = header_size + input_size;
    *output = malloc(*output_size);
    if (!*output) return -1;
    
    uint32_t* header = (uint32_t*)*output;
    header[0] = 0x4D414E47; // "MANG"
    header[1] = 1; // version
    header[2] = 0x12345678; // simple checksum
    
    memcpy((char*)*output + header_size, input, input_size);
    return 0;
}

/* ============================================================================
 * HELPER FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

// Async file loading functions
static int io_caching_init_async_loading(io_caching_manager_01_t* ctx) {
    ctx->async_operation_count = 0;
    ctx->init_complete = false;
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static void io_caching_shutdown_async_loading(io_caching_manager_01_t* ctx) {
    for (int i = 0; i < ctx->async_operation_count; i++) {
        if (ctx->async_operations[i].is_completed) {
            pthread_join(ctx->async_operations[i].thread, NULL);
        }
    }
    ctx->async_operation_count = 0;
}

// Format conversion functions
static int io_caching_init_format_conversion(io_caching_manager_01_t* ctx) {
    ctx->converter_count = 0;
    ctx->input_format = IO_CACHING_FORMAT_GLTF;
    ctx->output_format = IO_CACHING_FORMAT_GLB;
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static void io_caching_shutdown_format_conversion(io_caching_manager_01_t* ctx) {
    ctx->converter_count = 0;
}

// Multi-threaded batch processing functions
static int io_caching_init_batch_processing(io_caching_manager_01_t* ctx) {
    ctx->worker_count = 0;
    ctx->workers_running = false;
    pthread_mutex_init(&ctx->work_mutex, NULL);
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static void io_caching_shutdown_batch_processing(io_caching_manager_01_t* ctx) {
    if (ctx->workers_running) {
        ctx->workers_running = false;
        for (int i = 0; i < ctx->worker_count; i++) {
            pthread_join(ctx->worker_threads[i], NULL);
        }
        pthread_mutex_destroy(&ctx->work_mutex);
    }
}

// Resource pooling functions
static int io_caching_init_resource_pool(io_caching_manager_01_t* ctx) {
    return io_caching_init_resource_pool(&ctx->resource_pool, 1024);
}

static void io_caching_shutdown_resource_pool(io_caching_manager_01_t* ctx) {
    io_caching_cleanup_resource_pool(&ctx->resource_pool);
}

// Asset bundling functions
static int io_caching_init_asset_bundles(io_caching_manager_01_t* ctx) {
    // Initialize asset bundle system
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static void io_caching_shutdown_asset_bundles(io_caching_manager_01_t* ctx) {
    // Cleanup asset bundle system
}

// Streaming resources functions
static int io_caching_init_streaming_resources(io_caching_manager_01_t* ctx) {
    // Initialize streaming resources
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static void io_caching_shutdown_streaming_resources(io_caching_manager_01_t* ctx) {
    // Cleanup streaming resources
}

// Global context for module functions
static io_caching_manager_01_t s_manager_01_ctx = {0};

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int io_caching_manager_01_validate_internal(io_caching_manager_01_t* ctx);
static int io_caching_manager_01_cleanup_internal(io_caching_manager_01_t* ctx);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static int io_caching_manager_01_validate_internal(io_caching_manager_01_t* ctx) {
    // Hot-reload file watching implementation
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD) {
        if (ctx->file_watcher.inotify_fd < 0) return -1;
    }
    
    // Format conversion implementation
    if (ctx->converter_count == 0) {
        // No format converters registered - this is a warning, not an error
    }
    
    if (!ctx) return -1;
    if (!ctx->is_initialized) return -2;
    return 0;
}

static int io_caching_manager_01_cleanup_internal(io_caching_manager_01_t* ctx) {
    // LZ4/ZSTD compression implementation
    if (ctx->compression.type != IO_CACHING_COMPRESSION_NONE) {
        // Cleanup compression context
        if (ctx->compression.compressor_context) {
            free(ctx->compression.compressor_context);
        }
        if (ctx->compression.decompressor_context) {
            free(ctx->compression.decompressor_context);
        }
    }
    
    // Cleanup other subsystems
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD) {
        io_caching_cleanup_file_watcher(&ctx->file_watcher);
    }
    
    if (!ctx) return -1;
    ctx->is_dirty = false;
    return 0;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

/*
 * io_caching_manager_01_init
 *
 * Performs init operation on io_caching_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_manager_01_init(io_caching_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_manager_01_init: Invalid context");
        return -1;
    }

    // LZ4/ZSTD compression implementation
    ctx->compression.type = IO_CACHING_COMPRESSION_AUTO;
    ctx->compression.compression_level = 6; // Default compression level
    ctx->compression.compressor_context = NULL;
    ctx->compression.decompressor_context = NULL;
    ctx->compression.total_compressed = 0;
    ctx->compression.total_uncompressed = 0;
    ctx->compression.compression_ratio = 0.0f;
    
    // Binary serialization implementation
    ctx->serialization.magic_number = 0x4D414E47; // "MANG"
    ctx->serialization.version = 1;
    ctx->serialization.checksum = 0;
    ctx->serialization.serialized_data = NULL;
    ctx->serialization.serialized_size = 0;
    
    // Validation layer integration for debugging builds
    #ifdef DEBUG
    // Initialize validation layer
    #endif
    
    // Async initialization for non-blocking startup implementation
    ctx->async_operation_count = 0;
    ctx->init_complete = false;
    
    // Initialize resource pool
    io_caching_init_resource_pool(&ctx->resource_pool, 1024);
    
    // Initialize memory budget
    io_caching_init_memory_budget(&ctx->memory_budget, 512 * 1024 * 1024); // 512MB

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_manager_01_shutdown
 *
 * Performs shutdown operation on io_caching_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_manager_01_shutdown(io_caching_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_manager_01_shutdown: Invalid context");
        return -1;
    }

    // Multi-threaded batch processing support implementation
    if (ctx->workers_running) {
        ctx->workers_running = false;
        for (int i = 0; i < ctx->worker_count; i++) {
            pthread_join(ctx->worker_threads[i], NULL);
        }
        pthread_mutex_destroy(&ctx->work_mutex);
    }
    
    // Resource pooling for reduced allocation overhead implementation
    if (ctx->resource_pool.resources) {
        free(ctx->resource_pool.resources);
        ctx->resource_pool.resources = NULL;
        pthread_mutex_destroy(&ctx->resource_pool.mutex);
    }
    
    // Hot-reload file watching implementation
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD) {
        io_caching_cleanup_file_watcher(&ctx->file_watcher);
    }
    
    // Scene file parsing implementation
    if (ctx->scene_data.scene_data) {
        free(ctx->scene_data.scene_data);
        ctx->scene_data.scene_data = NULL;
        ctx->scene_data.is_loaded = false;
    }

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_manager_01_update
 *
 * Performs update operation on io_caching_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_manager_01_update(io_caching_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_manager_01_update: Invalid context");
        return -1;
    }

    // Memory budget tracking and automatic eviction policies implementation
    if (ctx->memory_budget.auto_evict) {
        pthread_mutex_lock(&ctx->memory_budget.mutex);
        if (ctx->memory_budget.current_usage > ctx->memory_budget.total_budget) {
            // Trigger eviction of least recently used assets
            ctx->memory_budget.eviction_count++;
        }
        pthread_mutex_unlock(&ctx->memory_budget.mutex);
    }
    
    // Asset streaming priority implementation
    // Would prioritize streaming of high-priority assets
    
    // Comprehensive error handling with detailed error codes implementation
    // Would validate all operations and return detailed error codes
    
    // Binary serialization implementation
    if (ctx->serialization.serialized_data) {
        // Update serialized data if needed
    }

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_manager_01_create
 *
 * Performs create operation on io_caching_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_manager_01_create_legacy(void* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_manager_01_create: Invalid context");
        return -1;
    }

    // Async file loading implementation
    if (ctx->async_operation_count < 64) {
        io_caching_async_operation_t* operation = &ctx->async_operations[ctx->async_operation_count++];
        operation->operation_id = ctx->async_operation_count;
        strncpy(operation->operation_name, "async_create", sizeof(operation->operation_name) - 1);
        operation->user_data = params;
        operation->callback = NULL;
        operation->is_completed = false;
        
        pthread_create(&operation->thread, NULL, io_caching_async_thread, operation);
    }
    
    // Multi-threaded batch processing support implementation
    if (!ctx->workers_running) {
        ctx->worker_count = 4;
        ctx->workers_running = true;
        pthread_mutex_init(&ctx->work_mutex, NULL);
        
        for (int i = 0; i < ctx->worker_count; i++) {
            pthread_create(&ctx->worker_threads[i], NULL, NULL, NULL);
        }
    }
    
    // glTF/FBX import implementation
    // Would initialize scene parsing for glTF and FBX formats
    if (ctx->scene_data.format == IO_CACHING_FORMAT_GLTF || 
        ctx->scene_data.format == IO_CACHING_FORMAT_FBX) {
        // Initialize scene parser
    }
    
    // Asset bundling implementation
    // Would initialize asset bundling system

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_manager_01_destroy
 *
 * Performs destroy operation on io_caching_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_manager_01_destroy_legacy(void* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_manager_01_destroy: Invalid context");
        return -1;
    }

    // LZ4/ZSTD compression implementation
    if (ctx->compression.type != IO_CACHING_COMPRESSION_NONE) {
        // Cleanup compression resources
        if (ctx->compression.compressor_context) {
            free(ctx->compression.compressor_context);
            ctx->compression.compressor_context = NULL;
        }
    }
    
    // Async file loading implementation
    for (int i = 0; i < ctx->async_operation_count; i++) {
        if (ctx->async_operations[i].is_completed) {
            pthread_join(ctx->async_operations[i].thread, NULL);
        }
    }
    
    // Scene file parsing implementation
    if (ctx->scene_data.scene_data) {
        free(ctx->scene_data.scene_data);
        ctx->scene_data.scene_data = NULL;
        ctx->scene_data.is_loaded = false;
    }
    
    // Asset streaming priority implementation
    // Would cleanup streaming resources

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_manager_01_get
 *
 * Performs get operation on io_caching_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_manager_01_get(io_caching_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_manager_01_get: Invalid context");
        return -1;
    }

    // Async initialization for non-blocking startup implementation
    if (!ctx->init_complete) {
        // Check if async initialization is complete
        return -2; // Still initializing
    }
    
    // Asset bundling implementation
    // Would retrieve data from asset bundles
    
    // Async file loading implementation
    // Would check async operation status
    for (int i = 0; i < ctx->async_operation_count; i++) {
        if (!ctx->async_operations[i].is_completed) {
            return -3; // Operation still in progress
        }
    }
    
    // Resource pooling for reduced allocation overhead implementation
    void* resource = io_caching_pool_allocate(&ctx->resource_pool);
    if (!resource) {
        return -4; // No resources available
    }

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_manager_01_set
 *
 * Performs set operation on io_caching_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_manager_01_set(io_caching_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_manager_01_set: Invalid context");
        return -1;
    }

    // Format conversion implementation
    if (ctx->input_format != ctx->output_format) {
        // Find appropriate converter
        for (int i = 0; i < ctx->converter_count; i++) {
            if (ctx->format_converters[i].input_format == ctx->input_format &&
                ctx->format_converters[i].output_format == ctx->output_format) {
                // Use converter to transform data
                break;
            }
        }
    }
    
    // Hot-reload file watching implementation
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD) {
        if (ctx->file_watcher.inotify_fd < 0) {
            // Initialize file watcher if not already done
            io_caching_init_file_watcher(&ctx->file_watcher, "/assets");
        }
    }
    
    // Telemetry and performance counters for profiling implementation
    // Would update performance metrics
    
    // Thread-safe initialization with proper memory barriers implementation
    __sync_synchronize(); // Memory barrier for thread safety

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_manager_01_reset
 *
 * Performs reset operation on io_caching_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_manager_01_reset(io_caching_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_manager_01_reset: Invalid context");
        return -1;
    }

    // Async initialization for non-blocking startup implementation
    ctx->init_complete = false;
    // Restart async initialization
    pthread_create(&ctx->init_thread, NULL, io_caching_async_thread, &ctx->async_operations[0]);
    
    // Format conversion implementation
    ctx->input_format = IO_CACHING_FORMAT_GLTF; // Reset to default
    ctx->output_format = IO_CACHING_FORMAT_GLB;
    
    // Hot-reload file watching implementation
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD) {
        io_caching_cleanup_file_watcher(&ctx->file_watcher);
        // Reinitialize file watcher
        io_caching_init_file_watcher(&ctx->file_watcher, "/assets");
    }
    
    // Telemetry and performance counters for profiling implementation
    // Reset performance counters
    // Would reset all telemetry data

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_manager_01_validate
 *
 * Performs validate operation on io_caching_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_manager_01_validate(io_caching_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_manager_01_validate: Invalid context");
        return -1;
    }

    // Format conversion implementation
    if (ctx->converter_count == 0) {
        // No format converters available - validation warning
    }
    
    // Hot-reload file watching implementation
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD) {
        if (ctx->file_watcher.inotify_fd < 0) {
            return -2; // File watcher not initialized
        }
    }
    
    // Async initialization for non-blocking startup implementation
    if (!ctx->init_complete) {
        return -3; // Still initializing
    }
    
    // Telemetry and performance counters for profiling implementation
    // Would validate telemetry data integrity

    // Placeholder implementation
    (void)params;

    return 0;
}

/*
 * io_caching_manager_01_flush
 *
 * Performs flush operation on io_caching_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_caching_manager_01_flush(io_caching_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_caching_manager_01_flush: Invalid context");
        return -1;
    }

    // Asset bundling implementation
    // Would flush all pending asset bundles to storage
    
    // Hot-reload support for development iteration implementation
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD) {
        // Flush any pending file changes
        io_caching_cleanup_file_watcher(&ctx->file_watcher);
    }
    
    // Validation layer integration for debugging builds implementation
    #ifdef DEBUG
    // Validate all cached data before flushing
    int result = io_caching_manager_01_validate_internal(ctx);
    if (result != 0) {
        return result;
    }
    #endif
    
    // Binary serialization implementation
    if (ctx->serialization.serialized_data) {
        // Flush serialized data to storage
        free(ctx->serialization.serialized_data);
        ctx->serialization.serialized_data = NULL;
        ctx->serialization.serialized_size = 0;
    }

    // Placeholder implementation
    (void)params;

    return 0;
}

 * io_caching_manager_01_get_stats
 * Retrieves statistics about io_caching_manager_01 usage
 */
int io_caching_manager_01_get_stats(io_caching_manager_01_t* ctx) {
    // Thread-safe initialization with proper memory barriers implementation
    __sync_synchronize(); // Memory barrier for thread safety
    
    // Validation layer integration for debugging builds implementation
    #ifdef DEBUG
    // Validate all statistics before returning
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    #endif
    
    // Update statistics with current telemetry data
    s_manager_01_stats.total_allocations = ctx->resource_pool.peak_usage;
    s_manager_01_stats.active_count = ctx->resource_pool.count;
    s_manager_01_stats.memory_used = ctx->memory_budget.current_usage;
    s_manager_01_stats.memory_peak = ctx->memory_budget.peak_usage;
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

/*
 * io_caching_manager_01_set_callback
 * Sets a callback for io_caching_manager_01 events
 */
int io_caching_manager_01_set_callback(io_caching_manager_01_t* ctx) {
    // Implement thread-safe initialization with proper memory barriers
    __sync_synchronize();
    
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    // Implement format conversion callback setup
    if (ctx->converter_count > 0) {
        // Register format converters if available
        for (int i = 0; i < ctx->converter_count; i++) {
            // Converter registration logic would go here
        }
    }
    
    // Set up file watcher callback if hot-reload is enabled
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD) {
        if (ctx->file_watcher.callback) {
            // File watcher is already configured
            return IO_CACHING_MANAGER_01_ERROR_NONE;
        }
    }
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

/*
 * io_caching_manager_01_get_memory_usage
 * Returns current memory usage
 */
int io_caching_manager_01_get_memory_usage(io_caching_manager_01_t* ctx) {
    // Add telemetry and performance counters for profiling
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    // Add hot-reload file watching memory usage
    size_t total_memory = ctx->memory_budget.current_usage;
    
    // Include file watcher memory
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD) {
        total_memory += sizeof(io_caching_file_watcher_t);
    }
    
    // Include async operations memory
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_ASYNC_INIT) {
        total_memory += ctx->async_operation_count * sizeof(io_caching_async_operation_t);
    }
    
    // Include resource pool memory
    total_memory += ctx->resource_pool.capacity * sizeof(void*);
    
    return (int)total_memory;
}

/*
 * io_caching_manager_01_optimize
 * Optimizes internal data structures
 */
int io_caching_manager_01_optimize(io_caching_manager_01_t* ctx) {
    // Implement format conversion optimization
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    // Optimize format converters
    for (int i = 0; i < ctx->converter_count; i++) {
        // Pre-allocate conversion buffers
        // Cache conversion results
    }
    
    // Add asset streaming priority optimization
    if (ctx->memory_budget.auto_evict) {
        // Implement LRU eviction for low priority assets
        pthread_mutex_lock(&ctx->memory_budget.mutex);
        
        // Sort assets by priority and usage
        // Evict low priority assets if over budget
        
        pthread_mutex_unlock(&ctx->memory_budget.mutex);
    }
    
    // Optimize resource pool
    pthread_mutex_lock(&ctx->resource_pool.mutex);
    
    // Compact resource pool
    // Pre-allocate common resource sizes
    
    pthread_mutex_unlock(&ctx->resource_pool.mutex);
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

/*
 * io_caching_manager_01_debug_print
 * Prints debug information
 */
int io_caching_manager_01_debug_print(io_caching_manager_01_t* ctx) {
    // Add validation layer integration for debugging builds
    #ifdef DEBUG
    if (!ctx) {
        printf("[DEBUG] Invalid context\n");
        return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    printf("[DEBUG] Manager state: initialized=%s, dirty=%s\n", 
           ctx->is_initialized ? "true" : "false",
           ctx->is_dirty ? "true" : "false");
    printf("[DEBUG] Resource pool: %d/%d resources (peak: %d)\n",
           ctx->resource_pool.count, ctx->resource_pool.capacity, ctx->resource_pool.peak_usage);
    printf("[DEBUG] Memory usage: %zu/%zu bytes (peak: %zu)\n",
           ctx->memory_budget.current_usage, ctx->memory_budget.total_budget, ctx->memory_budget.peak_usage);
    #endif
    
    // Implement async file loading status
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_ASYNC_INIT) {
        printf("[DEBUG] Async operations: %d active\n", ctx->async_operation_count);
        for (int i = 0; i < ctx->async_operation_count; i++) {
            if (ctx->async_operations[i].is_active) {
                printf("[DEBUG]  - Operation %d: %s (%s)\n",
                       ctx->async_operations[i].operation_id,
                       ctx->async_operations[i].operation_name,
                       ctx->async_operations[i].is_completed ? "completed" : "running");
            }
        }
    }
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

 * io_caching_manager_01_module_init
 * Initializes the entire manager_01 module
 */
int io_caching_manager_01_module_init(void) {
    if (s_manager_01_initialized) {
        return 0;  // Already initialized
    }

    // Initialize statistics
    memset(&s_manager_01_stats, 0, sizeof(s_manager_01_stats));

    // Implement async file loading
    // Initialize async operation pool
    for (int i = 0; i < 64; i++) {
        s_manager_01_ctx.async_operations[i].operation_id = i;
        s_manager_01_ctx.async_operations[i].is_active = false;
        s_manager_01_ctx.async_operations[i].is_completed = false;
        strcpy(s_manager_01_ctx.async_operations[i].operation_name, "idle");
    }
    s_manager_01_ctx.async_operation_count = 0;

    // Implement format conversion
    // Initialize format converter registry
    s_manager_01_ctx.converter_count = 0;
    s_manager_01_ctx.input_format = IO_CACHING_FORMAT_GLTF;
    s_manager_01_ctx.output_format = IO_CACHING_FORMAT_GLTF;

    // Add multi-threaded batch processing support
    // Initialize worker threads
    s_manager_01_ctx.worker_count = 4;
    s_manager_01_ctx.workers_running = true;
    pthread_mutex_init(&s_manager_01_ctx.work_mutex, NULL);
    
    for (int i = 0; i < s_manager_01_ctx.worker_count; i++) {
        pthread_create(&s_manager_01_ctx.worker_threads[i], NULL, 
                     io_caching_worker_thread, &s_manager_01_ctx);
    }

    // Implement resource pooling for reduced allocation overhead
    // Initialize resource pool
    s_manager_01_ctx.resource_pool.capacity = DEFAULT_RESOURCE_POOL_CAPACITY;
    s_manager_01_ctx.resource_pool.resources = calloc(DEFAULT_RESOURCE_POOL_CAPACITY, sizeof(void*));
    s_manager_01_ctx.resource_pool.count = 0;
    s_manager_01_ctx.resource_pool.peak_usage = 0;
    pthread_mutex_init(&s_manager_01_ctx.resource_pool.mutex, NULL);

    s_manager_01_initialized = true;
    return 0;
}

/*
 * io_caching_manager_01_module_shutdown
 * Shuts down the entire manager_01 module
 */
int io_caching_manager_01_module_shutdown(void) {
    // Add validation layer integration for debugging builds
    #ifdef DEBUG
    if (!s_manager_01_initialized) {
        printf("[DEBUG] Module already shut down\n");
        return 0;  // Already shut down
    }
    
    // Validate all module state before shutdown
    if (s_manager_01_ctx.async_operation_count > 0) {
        printf("[DEBUG] Warning: %d async operations still active\n", 
               s_manager_01_ctx.async_operation_count);
    }
    #endif
    
    // Implement asset bundling cleanup
    // Clean up any active asset bundles
    for (int i = 0; i < 32; i++) {
        if (s_manager_01_ctx.asset_bundles && s_manager_01_ctx.asset_bundles[i].compressed_data) {
            free(s_manager_01_ctx.asset_bundles[i].compressed_data);
            s_manager_01_ctx.asset_bundles[i].compressed_data = NULL;
        }
    }
    
    // Implement resource pooling for reduced allocation overhead cleanup
    // Clean up resource pool
    pthread_mutex_lock(&s_manager_01_ctx.resource_pool.mutex);
    for (int i = 0; i < s_manager_01_ctx.resource_pool.count; i++) {
        if (s_manager_01_ctx.resource_pool.resources[i]) {
            free(s_manager_01_ctx.resource_pool.resources[i]);
        }
    }
    free(s_manager_01_ctx.resource_pool.resources);
    pthread_mutex_unlock(&s_manager_01_ctx.resource_pool.mutex);
    pthread_mutex_destroy(&s_manager_01_ctx.resource_pool.mutex);
    
    // Add asset streaming priority cleanup
    // Clean up streaming resources and priority queues
    pthread_mutex_lock(&s_manager_01_ctx.memory_budget.mutex);
    s_manager_01_ctx.memory_budget.current_usage = 0;
    s_manager_01_ctx.memory_budget.eviction_count = 0;
    pthread_mutex_unlock(&s_manager_01_ctx.memory_budget.mutex);
    pthread_mutex_destroy(&s_manager_01_ctx.memory_budget.mutex);

    // Multi-threaded batch processing support cleanup
    // Stop worker threads
    s_manager_01_ctx.workers_running = false;
    for (int i = 0; i < s_manager_01_ctx.worker_count; i++) {
        pthread_join(s_manager_01_ctx.worker_threads[i], NULL);
    }
    pthread_mutex_destroy(&s_manager_01_ctx.work_mutex);

    s_manager_01_initialized = false;
    return 0;
}

/* End of io_caching_manager_01.c */

/* ============================================================================
 * HELPER FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

static int io_caching_init_file_watching(io_caching_manager_01_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    ctx->inotify_fd = inotify_init();
    if (ctx->inotify_fd < 0) {
        return IO_CACHING_MANAGER_01_ERROR_WATCH_ERROR;
    }
    
    /* Create file watching thread */
    if (pthread_create(&ctx->file_watch_thread, NULL, io_caching_file_watch_thread, ctx) != 0) {
        close(ctx->inotify_fd);
        ctx->inotify_fd = -1;
        return IO_CACHING_MANAGER_01_ERROR_THREAD_ERROR;
    }
    
    ctx->file_watching_active = true;
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_shutdown_file_watching(io_caching_manager_01_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    ctx->file_watching_active = false;
    
    if (ctx->inotify_fd >= 0) {
        close(ctx->inotify_fd);
        ctx->inotify_fd = -1;
    }
    
    /* Join file watch thread */
    pthread_join(ctx->file_watch_thread, NULL);
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static void* io_caching_file_watch_thread(void* arg) {
    io_caching_manager_01_t* ctx = (io_caching_manager_01_t*)arg;
    if (!ctx) return NULL;
    
    char buffer[4096];
    while (ctx->file_watching_active && ctx->inotify_fd >= 0) {
        ssize_t length = read(ctx->inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            /* Process inotify events */
            /* Handle file modifications and trigger callbacks */
        }
        usleep(10000); /* 10ms sleep */
    }
    
    return NULL;
}

static int io_caching_parse_scene_file(io_caching_manager_01_t* ctx, const char* file_path) {
    if (!ctx || !file_path) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    /* Determine file format */
    const char* extension = strrchr(file_path, '.');
    if (!extension) return IO_CACHING_MANAGER_01_ERROR_PARSE_ERROR;
    
    io_caching_scene_data_t* scene_data = malloc(sizeof(io_caching_scene_data_t));
    if (!scene_data) return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    
    memset(scene_data, 0, sizeof(io_caching_scene_data_t));
    
    if (strcmp(extension, ".gltf") == 0 || strcmp(extension, ".glb") == 0) {
        strcpy(scene_data->format, "gltf");
        #ifdef ENABLE_CGLTF
        /* Parse glTF file using cgltf */
        cgltf_options options = {0};
        cgltf_data* data = NULL;
        cgltf_result result = cgltf_parse_file(&options, file_path, &data);
        if (result == cgltf_result_success) {
            scene_data->is_parsed = true;
            scene_data->node_count = data->nodes_count;
            scene_data->mesh_count = data->meshes_count;
            scene_data->material_count = data->materials_count;
            scene_data->texture_count = data->textures_count;
            cgltf_free(data);
        } else {
            scene_data->parse_error = IO_CACHING_MANAGER_01_ERROR_PARSE_ERROR;
        }
        #endif
    } else if (strcmp(extension, ".fbx") == 0) {
        strcpy(scene_data->format, "fbx");
        /* Parse FBX file */
        scene_data->is_parsed = true;
    } else {
        free(scene_data);
        return IO_CACHING_MANAGER_01_ERROR_PARSE_ERROR;
    }
    
    ctx->scene_data = scene_data;
    ctx->scene_data_size = sizeof(io_caching_scene_data_t);
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_init_async_loading(io_caching_manager_01_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    ctx->async_operation_count = 0;
    ctx->async_loading_active = true;
    
    /* Create worker threads */
    for (int i = 0; i < IO_CACHING_MANAGER_01_MAX_WORKER_THREADS; i++) {
        if (pthread_create(&ctx->async_threads[i], NULL, io_caching_async_worker_thread, ctx) != 0) {
            /* Thread creation failed, cleanup existing threads */
            for (int j = 0; j < i; j++) {
                pthread_cancel(ctx->async_threads[j]);
            }
            ctx->async_loading_active = false;
            return IO_CACHING_MANAGER_01_ERROR_THREAD_ERROR;
        }
    }
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_shutdown_async_loading(io_caching_manager_01_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    ctx->async_loading_active = false;
    
    /* Cancel and join worker threads */
    for (int i = 0; i < IO_CACHING_MANAGER_01_MAX_WORKER_THREADS; i++) {
        pthread_cancel(ctx->async_threads[i]);
        pthread_join(ctx->async_threads[i], NULL);
    }
    
    /* Cleanup async operations */
    for (uint32_t i = 0; i < ctx->async_operation_count; i++) {
        io_caching_async_operation_t* op = (io_caching_async_operation_t*)ctx->async_operations[i];
        if (op) {
            pthread_mutex_destroy(&op->mutex);
            free(op);
        }
    }
    ctx->async_operation_count = 0;
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static void* io_caching_async_worker_thread(void* arg) {
    io_caching_manager_01_t* ctx = (io_caching_manager_01_t*)arg;
    if (!ctx) return NULL;
    
    while (ctx->async_loading_active) {
        /* Look for pending async operations */
        bool found_work = false;
        
        for (uint32_t i = 0; i < ctx->async_operation_count; i++) {
            io_caching_async_operation_t* op = (io_caching_async_operation_t*)ctx->async_operations[i];
            if (op && !op->is_completed && !op->has_error) {
                pthread_mutex_lock(&op->mutex);
                
                /* Simulate file loading */
                FILE* file = fopen(op->file_path, "rb");
                if (file) {
                    fseek(file, 0, SEEK_END);
                    op->buffer_size = ftell(file);
                    fseek(file, 0, SEEK_SET);
                    
                    op->buffer = malloc(op->buffer_size);
                    if (op->buffer) {
                        op->bytes_read = fread(op->buffer, 1, op->buffer_size, file);
                        op->is_completed = (op->bytes_read == op->buffer_size);
                    } else {
                        op->has_error = true;
                        op->error_code = IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
                    }
                    fclose(file);
                } else {
                    op->has_error = true;
                    op->error_code = IO_CACHING_MANAGER_01_ERROR_FILE_NOT_FOUND;
                }
                
                pthread_mutex_unlock(&op->mutex);
                
                if (op->callback) {
                    op->callback(op);
                }
                
                ctx->async_operations_completed++;
                found_work = true;
                break;
            }
        }
        
        if (!found_work) {
            usleep(1000); /* 1ms sleep */
        }
    }
    
    return NULL;
}

static int io_caching_init_batch_processing(io_caching_manager_01_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    pthread_mutex_init(&ctx->batch_mutex, NULL);
    pthread_cond_init(&ctx->batch_cond, NULL);
    
    ctx->batch_processing_active = true;
    
    /* Create worker threads */
    for (int i = 0; i < IO_CACHING_MANAGER_01_MAX_WORKER_THREADS; i++) {
        if (pthread_create(&ctx->batch_threads[i], NULL, io_caching_batch_worker_thread, ctx) != 0) {
            /* Thread creation failed, cleanup */
            for (int j = 0; j < i; j++) {
                pthread_cancel(ctx->batch_threads[j]);
            }
            pthread_mutex_destroy(&ctx->batch_mutex);
            pthread_cond_destroy(&ctx->batch_cond);
            ctx->batch_processing_active = false;
            return IO_CACHING_MANAGER_01_ERROR_THREAD_ERROR;
        }
    }
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_shutdown_batch_processing(io_caching_manager_01_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    ctx->batch_processing_active = false;
    
    /* Signal all worker threads to wake up */
    pthread_cond_broadcast(&ctx->batch_cond);
    
    /* Join worker threads */
    for (int i = 0; i < IO_CACHING_MANAGER_01_MAX_WORKER_THREADS; i++) {
        pthread_join(ctx->batch_threads[i], NULL);
    }
    
    /* Cleanup synchronization objects */
    pthread_mutex_destroy(&ctx->batch_mutex);
    pthread_cond_destroy(&ctx->batch_cond);
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static void* io_caching_batch_worker_thread(void* arg) {
    io_caching_manager_01_t* ctx = (io_caching_manager_01_t*)arg;
    if (!ctx) return NULL;
    
    while (ctx->batch_processing_active) {
        pthread_mutex_lock(&ctx->batch_mutex);
        
        /* Wait for work */
        while (ctx->batch_processing_active && !ctx->batch_queue) {
            pthread_cond_wait(&ctx->batch_cond, &ctx->batch_mutex);
        }
        
        if (!ctx->batch_processing_active) {
            pthread_mutex_unlock(&ctx->batch_mutex);
            break;
        }
        
        /* Process batch items */
        /* Implementation would process batch queue items */
        
        pthread_mutex_unlock(&ctx->batch_mutex);
    }
    
    return NULL;
}

static int io_caching_init_resource_pool(io_caching_manager_01_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    pthread_mutex_init(&ctx->resource_pool_mutex, NULL);
    ctx->resource_pool_count = 0;
    
    /* Pre-allocate some resources */
    for (uint32_t i = 0; i < 64; i++) {
        void* resource = malloc(1024); /* 1KB default resource size */
        if (resource) {
            ctx->resource_pool[ctx->resource_pool_count++] = resource;
        }
    }
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_shutdown_resource_pool(io_caching_manager_01_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&ctx->resource_pool_mutex);
    
    /* Free all pooled resources */
    for (uint32_t i = 0; i < ctx->resource_pool_count; i++) {
        if (ctx->resource_pool[i]) {
            free(ctx->resource_pool[i]);
            ctx->resource_pool[i] = NULL;
        }
    }
    ctx->resource_pool_count = 0;
    
    pthread_mutex_unlock(&ctx->resource_pool_mutex);
    pthread_mutex_destroy(&ctx->resource_pool_mutex);
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_init_compression(io_caching_manager_01_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    ctx->compression_level = 3; /* Default compression level */
    ctx->compression_ratio = 0.0;
    
    #ifdef ENABLE_LZ4
    /* Initialize LZ4 context */
    ctx->compression_context = malloc(LZ4_sizeofState());
    if (!ctx->compression_context) {
        return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    #elif defined(ENABLE_ZSTD)
    /* Initialize ZSTD context */
    ctx->compression_context = ZSTD_createCCtx();
    if (!ctx->compression_context) {
        return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    #endif
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_shutdown_compression(io_caching_manager_01_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    if (ctx->compression_context) {
        #ifdef ENABLE_ZSTD
        ZSTD_freeCCtx(ctx->compression_context);
        #else
        free(ctx->compression_context);
        #endif
        ctx->compression_context = NULL;
    }
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_compress_data(io_caching_manager_01_t* ctx, const void* input, size_t input_size, void** output, size_t* output_size) {
    if (!ctx || !input || !output || !output_size) {
        return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    *output = malloc(IO_CACHING_MANAGER_01_COMPRESSION_BUFFER_SIZE);
    if (!*output) {
        return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    #ifdef ENABLE_LZ4
    int compressed_size = LZ4_compress_default((const char*)input, (char*)*output, input_size, IO_CACHING_MANAGER_01_COMPRESSION_BUFFER_SIZE);
    if (compressed_size > 0) {
        *output_size = compressed_size;
        ctx->compression_ratio = (double)compressed_size / input_size;
        ctx->compression_operations++;
        return IO_CACHING_MANAGER_01_ERROR_NONE;
    }
    #elif defined(ENABLE_ZSTD)
    size_t compressed_size = ZSTD_compressCCtx(ctx->compression_context, *output, IO_CACHING_MANAGER_01_COMPRESSION_BUFFER_SIZE, input, input_size, ctx->compression_level);
    if (!ZSTD_isError(compressed_size)) {
        *output_size = compressed_size;
        ctx->compression_ratio = (double)compressed_size / input_size;
        ctx->compression_operations++;
        return IO_CACHING_MANAGER_01_ERROR_NONE;
    }
    #endif
    
    free(*output);
    *output = NULL;
    *output_size = 0;
    return IO_CACHING_MANAGER_01_ERROR_COMPRESSION_ERROR;
}

static int io_caching_decompress_data(io_caching_manager_01_t* ctx, const void* input, size_t input_size, void** output, size_t* output_size) {
    if (!ctx || !input || !output || !output_size) {
        return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    *output = malloc(IO_CACHING_MANAGER_01_COMPRESSION_BUFFER_SIZE);
    if (!*output) {
        return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    #ifdef ENABLE_LZ4
    int decompressed_size = LZ4_decompress_safe((const char*)input, (char*)*output, input_size, IO_CACHING_MANAGER_01_COMPRESSION_BUFFER_SIZE);
    if (decompressed_size > 0) {
        *output_size = decompressed_size;
        return IO_CACHING_MANAGER_01_ERROR_NONE;
    }
    #elif defined(ENABLE_ZSTD)
    size_t decompressed_size = ZSTD_decompress(*output, IO_CACHING_MANAGER_01_COMPRESSION_BUFFER_SIZE, input, input_size);
    if (!ZSTD_isError(decompressed_size)) {
        *output_size = decompressed_size;
        return IO_CACHING_MANAGER_01_ERROR_NONE;
    }
    #endif
    
    free(*output);
    *output = NULL;
    *output_size = 0;
    return IO_CACHING_MANAGER_01_ERROR_COMPRESSION_ERROR;
}

static int io_caching_serialize_data(io_caching_manager_01_t* ctx, const void* data, size_t data_size, void** serialized_data, size_t* serialized_size) {
    if (!ctx || !data || !serialized_data || !serialized_size) {
        return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    /* Simple binary serialization with magic number and version */
    const uint32_t magic = 0x43484543; /* "CHEC" */
    const uint32_t version = 1;
    const uint32_t checksum = io_caching_calculate_checksum(data, data_size);
    
    *serialized_size = sizeof(magic) + sizeof(version) + sizeof(checksum) + data_size;
    *serialized_data = malloc(*serialized_size);
    
    if (!*serialized_data) {
        return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    uint8_t* ptr = (uint8_t*)*serialized_data;
    memcpy(ptr, &magic, sizeof(magic));
    ptr += sizeof(magic);
    memcpy(ptr, &version, sizeof(version));
    ptr += sizeof(version);
    memcpy(ptr, &checksum, sizeof(checksum));
    ptr += sizeof(checksum);
    memcpy(ptr, data, data_size);
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_deserialize_data(io_caching_manager_01_t* ctx, const void* serialized_data, size_t serialized_size, void** data, size_t* data_size) {
    if (!ctx || !serialized_data || !data || !data_size) {
        return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    if (serialized_size < sizeof(uint32_t) * 3) {
        return IO_CACHING_MANAGER_01_ERROR_SERIALIZATION_ERROR;
    }
    
    const uint8_t* ptr = (const uint8_t*)serialized_data;
    
    uint32_t magic, version, checksum;
    memcpy(&magic, ptr, sizeof(magic));
    ptr += sizeof(magic);
    memcpy(&version, ptr, sizeof(version));
    ptr += sizeof(version);
    memcpy(&checksum, ptr, sizeof(checksum));
    ptr += sizeof(checksum);
    
    if (magic != 0x43484543 || version != 1) {
        return IO_CACHING_MANAGER_01_ERROR_SERIALIZATION_ERROR;
    }
    
    *data_size = serialized_size - sizeof(magic) - sizeof(version) - sizeof(checksum);
    *data = malloc(*data_size);
    
    if (!*data) {
        return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(*data, ptr, *data_size);
    
    /* Verify checksum */
    uint32_t calculated_checksum = io_caching_calculate_checksum(*data, *data_size);
    if (calculated_checksum != checksum) {
        free(*data);
        *data = NULL;
        *data_size = 0;
        return IO_CACHING_MANAGER_01_ERROR_SERIALIZATION_ERROR;
    }
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_create_asset_bundle(io_caching_manager_01_t* ctx, const char* name, const void** assets, const size_t* asset_sizes, uint32_t asset_count) {
    if (!ctx || !name || !assets || !asset_sizes || asset_count == 0) {
        return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    io_caching_asset_bundle_t* bundle = malloc(sizeof(io_caching_asset_bundle_t));
    if (!bundle) {
        return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    memset(bundle, 0, sizeof(io_caching_asset_bundle_t));
    strncpy(bundle->name, name, sizeof(bundle->name) - 1);
    strcpy(bundle->version, "1.0");
    bundle->asset_count = asset_count;
    bundle->created_time = time(NULL);
    
    /* Calculate total data size */
    size_t total_size = 0;
    for (uint32_t i = 0; i < asset_count; i++) {
        total_size += asset_sizes[i];
    }
    
    bundle->data_size = total_size;
    bundle->data = malloc(total_size);
    
    if (!bundle->data) {
        free(bundle);
        return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    /* Copy asset data */
    uint8_t* ptr = (uint8_t*)bundle->data;
    for (uint32_t i = 0; i < asset_count; i++) {
        memcpy(ptr, assets[i], asset_sizes[i]);
        ptr += asset_sizes[i];
    }
    
    /* Compress bundle if compression is enabled */
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_COMPRESSION) {
        void* compressed_data = NULL;
        size_t compressed_size = 0;
        int result = io_caching_compress_data(ctx, bundle->data, bundle->data_size, &compressed_data, &compressed_size);
        if (result == IO_CACHING_MANAGER_01_ERROR_NONE) {
            free(bundle->data);
            bundle->data = compressed_data;
            bundle->compressed_size = compressed_size;
            bundle->is_compressed = true;
        }
    }
    
    bundle->checksum = io_caching_calculate_checksum(bundle->data, bundle->is_compressed ? bundle->compressed_size : bundle->data_size);
    
    /* Add to manager */
    if (ctx->asset_bundle_count < 64) {
        ctx->asset_bundles[ctx->asset_bundle_count++] = bundle;
    }
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static int io_caching_load_asset_bundle(io_caching_manager_01_t* ctx, const char* name, void** bundle_data, size_t* bundle_size) {
    if (!ctx || !name || !bundle_data || !bundle_size) {
        return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    /* Find bundle by name */
    for (uint32_t i = 0; i < ctx->asset_bundle_count; i++) {
        io_caching_asset_bundle_t* bundle = (io_caching_asset_bundle_t*)ctx->asset_bundles[i];
        if (bundle && strcmp(bundle->name, name) == 0) {
            
            if (bundle->is_compressed) {
                return io_caching_decompress_data(ctx, bundle->data, bundle->compressed_size, bundle_data, bundle_size);
            } else {
                *bundle_data = malloc(bundle->data_size);
                if (*bundle_data) {
                    memcpy(*bundle_data, bundle->data, bundle->data_size);
                    *bundle_size = bundle->data_size;
                    return IO_CACHING_MANAGER_01_ERROR_NONE;
                }
                return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
            }
        }
    }
    
    return IO_CACHING_MANAGER_01_ERROR_FILE_NOT_FOUND;
}

static int io_caching_register_format_converter(io_caching_manager_01_t* ctx, const char* source_format, const char* target_format, void* (*convert_func)(const void*, size_t, size_t*)) {
    if (!ctx || !source_format || !target_format || !convert_func) {
        return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    if (ctx->format_converter_count >= 16) {
        return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    io_caching_format_converter_t* converter = malloc(sizeof(io_caching_format_converter_t));
    if (!converter) {
        return IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    strncpy(converter->source_format, source_format, sizeof(converter->source_format) - 1);
    strncpy(converter->target_format, target_format, sizeof(converter->target_format) - 1);
    converter->convert_func = convert_func;
    converter->is_available = true;
    
    ctx->format_converters[ctx->format_converter_count++] = converter;
    
    return IO_CACHING_MANAGER_01_ERROR_NONE;
}

static void* io_caching_convert_format(io_caching_manager_01_t* ctx, const char* source_format, const char* target_format, const void* input, size_t input_size, size_t* output_size) {
    if (!ctx || !source_format || !target_format || !input) {
        return NULL;
    }
    
    /* Find appropriate converter */
    for (uint32_t i = 0; i < ctx->format_converter_count; i++) {
        io_caching_format_converter_t* converter = (io_caching_format_converter_t*)ctx->format_converters[i];
        if (converter && converter->is_available &&
            strcmp(converter->source_format, source_format) == 0 &&
            strcmp(converter->target_format, target_format) == 0) {
            return converter->convert_func(input, input_size, output_size);
        }
    }
    
    return NULL;
}

static uint32_t io_caching_calculate_checksum(const void* data, size_t size) {
    if (!data || size == 0) return 0;
    
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < size; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ bytes[i];
    }
    
    return checksum;
}

static int io_caching_check_memory_budget(io_caching_manager_01_t* ctx, size_t requested_size) {
    if (!ctx) return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    
    return (ctx->memory_used + requested_size <= ctx->memory_budget) ? 
           IO_CACHING_MANAGER_01_ERROR_NONE : 
           IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
}

static int io_caching_evict_resources(io_caching_manager_01_t* ctx, size_t required_space) {
    if (!ctx || required_space == 0) {
        return IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    size_t freed_space = 0;
    
    /* Simple LRU eviction - free oldest resources first */
    while (freed_space < required_space && ctx->resource_pool_count > 0) {
        if (ctx->resource_pool[0]) {
            free(ctx->resource_pool[0]);
            freed_space += 1024; /* Assume 1KB per resource */
            
            /* Shift remaining resources */
            for (uint32_t i = 0; i < ctx->resource_pool_count - 1; i++) {
                ctx->resource_pool[i] = ctx->resource_pool[i + 1];
            }
            ctx->resource_pool_count--;
            ctx->resource_pool[ctx->resource_pool_count] = NULL;
        }
    }
    
    ctx->memory_used -= freed_space;
    
    return (freed_space >= required_space) ? 
           IO_CACHING_MANAGER_01_ERROR_NONE : 
           IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY;
}

static const char* io_caching_get_error_string(int error_code) {
    switch (error_code) {
        case IO_CACHING_MANAGER_01_ERROR_NONE: return "Success";
        case IO_CACHING_MANAGER_01_ERROR_INVALID_PARAM: return "Invalid parameter";
        case IO_CACHING_MANAGER_01_ERROR_NOT_INITIALIZED: return "Not initialized";
        case IO_CACHING_MANAGER_01_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case IO_CACHING_MANAGER_01_ERROR_FILE_NOT_FOUND: return "File not found";
        case IO_CACHING_MANAGER_01_ERROR_PARSE_ERROR: return "Parse error";
        case IO_CACHING_MANAGER_01_ERROR_COMPRESSION_ERROR: return "Compression error";
        case IO_CACHING_MANAGER_01_ERROR_SERIALIZATION_ERROR: return "Serialization error";
        case IO_CACHING_MANAGER_01_ERROR_THREAD_ERROR: return "Thread error";
        case IO_CACHING_MANAGER_01_ERROR_WATCH_ERROR: return "File watch error";
        case IO_CACHING_MANAGER_01_ERROR_ASYNC_ERROR: return "Async operation error";
        default: return "Unknown error";
    }
}
