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
#include <errno.h>

#include "assets/io/caching/asset_cache_manager.h"
#include "include/core/types.h"
#include "include/core/memory.h"
#include "engine/include/core/logger.h"
#include "core/memory.h"

/* LZ4/ZSTD compression includes */
#ifdef ENABLE_LZ4
#include <lz4.h>
#endif
#ifdef ENABLE_ZSTD
#include <zstd.h>
#endif

/* cgltf for scene parsing */
#ifdef ENABLE_CGLTF
#include <cgltf.h>
#endif

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define IO_CACHING_MANAGER_01_VERSION_MAJOR 1
#define IO_CACHING_MANAGER_01_VERSION_MINOR 0
#define IO_CACHING_MANAGER_01_VERSION_PATCH 0

#define IO_CACHING_MANAGER_01_MAX_INSTANCES 4096
#define IO_CACHING_MANAGER_01_DEFAULT_CAPACITY 256
#define IO_CACHING_MANAGER_01_ALIGNMENT 16
#define IO_CACHING_MANAGER_01_MAX_RESOURCES 1024
#define IO_CACHING_MANAGER_01_MAX_ASYNC_OPERATIONS 64
#define IO_CACHING_MANAGER_01_MAX_FILE_WATCHES 1024
#define IO_CACHING_MANAGER_01_MEMORY_BUDGET (512 * 1024 * 1024) /* 512MB */
#define IO_CACHING_MANAGER_01_DEBOUNCE_MS 100

#define IO_CACHING_MANAGER_01_FLAG_NONE          0x00000000
#define IO_CACHING_MANAGER_01_FLAG_INITIALIZED   0x00000001
#define IO_CACHING_MANAGER_01_FLAG_DIRTY         0x00000002
#define IO_CACHING_MANAGER_01_FLAG_GPU_RESIDENT  0x00000004
#define IO_CACHING_MANAGER_01_FLAG_STREAMING     0x00000008
#define IO_CACHING_MANAGER_01_FLAG_ASYNC_INIT    0x00000010
#define IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD    0x00000020
#define IO_CACHING_MANAGER_01_FLAG_COMPRESSION   0x00000040
#define IO_CACHING_MANAGER_01_FLAG_BUNDLING      0x00000080
#define IO_CACHING_MANAGER_01_FLAG_BATCH_PROCESS 0x00000100
#define IO_CACHING_MANAGER_01_FLAG_RESOURCE_POOL 0x00000200
#define IO_CACHING_MANAGER_01_FLAG_SCENE_PARSING 0x00000400
#define IO_CACHING_MANAGER_01_FLAG_BINARY_SERIAL  0x00000800
#define IO_CACHING_MANAGER_01_FLAG_MEMORY_BUDGET 0x00001000
#define IO_CACHING_MANAGER_01_FLAG_ERROR_HANDLING 0x00002000
#define IO_CACHING_MANAGER_01_FLAG_TELEMETRY     0x00004000

/* Error codes */
#define IO_CACHING_MANAGER_ERROR_NONE              0
#define IO_CACHING_MANAGER_ERROR_INVALID_PARAM    -1
#define IO_CACHING_MANAGER_ERROR_NOT_INITIALIZED   -2
#define IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY     -3
#define IO_CACHING_MANAGER_ERROR_FILE_NOT_FOUND    -4
#define IO_CACHING_MANAGER_ERROR_PERMISSION_DENIED -5
#define IO_CACHING_MANAGER_ERROR_COMPRESSION_FAILED -6
#define IO_CACHING_MANAGER_ERROR_SERIALIZATION_FAILED -7
#define IO_CACHING_MANAGER_ERROR_PARSING_FAILED    -8
#define IO_CACHING_MANAGER_ERROR_ASYNC_FAILED       -9
#define IO_CACHING_MANAGER_ERROR_THREAD_FAILED      -10

/* Additional constants */
#define IO_CACHING_MANAGER_SERIALIZATION_VERSION 1
#define IO_CACHING_MANAGER_MAGIC_NUMBER 0x434D4143 /* "CMAC" */

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/* Resource pool entry */
typedef struct {
    void* resource;
    size_t size;
    uint32_t ref_count;
    bool in_use;
    uint64_t last_access;
    char name[256];
} io_caching_resource_pool_entry_t;

/* Resource pool */
typedef struct {
    io_caching_resource_pool_entry_t entries[IO_CACHING_MANAGER_01_MAX_RESOURCES];
    uint32_t capacity;
    uint32_t active_count;
    uint32_t peak_count;
    size_t total_allocated;
    pthread_mutex_t mutex;
} io_caching_resource_pool_t;

/* Format converter function pointer */
typedef int (*io_caching_format_converter_t)(const void* input, size_t input_size,
                                           void** output, size_t* output_size,
                                           const char* target_format);

/* Format converter registry */
typedef struct {
    char source_format[32];
    char target_format[32];
    io_caching_format_converter_t converter;
} io_caching_format_converter_entry_t;

/* Format conversion system */
typedef struct {
    io_caching_format_converter_entry_t converters[16];
    uint32_t converter_count;
    pthread_mutex_t mutex;
} io_caching_format_conversion_t;

/* File watch entry */
typedef struct {
    int wd; /* inotify watch descriptor */
    char path[512];
    uint32_t last_modified;
    bool pending_reload;
    void (*callback)(const char* path, void* user_data);
    void* user_data;
} io_caching_file_watch_entry_t;

/* Hot-reload file watching system */
typedef struct {
    int inotify_fd;
    io_caching_file_watch_entry_t watches[IO_CACHING_MANAGER_01_MAX_FILE_WATCHES];
    uint32_t watch_count;
    pthread_t watch_thread;
    bool watch_thread_running;
    pthread_mutex_t mutex;
} io_caching_hot_reload_t;

/* Telemetry data */
typedef struct {
    uint64_t operations_completed;
    uint64_t operations_failed;
    uint64_t total_bytes_processed;
    uint64_t total_time_ns;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t compression_ratio_sum;
    uint64_t compression_count;
    uint64_t async_operations_completed;
    uint64_t file_reload_events;
} io_caching_telemetry_t;

/* Async operation */
typedef struct {
    uint32_t id;
    bool completed;
    bool failed;
    void* result;
    size_t result_size;
    void (*callback)(void* result, void* user_data);
    void* user_data;
    uint64_t start_time_ns;
    uint64_t end_time_ns;
} io_caching_async_operation_t;

/* Async operation system */
typedef struct {
    io_caching_async_operation_t operations[IO_CACHING_MANAGER_01_MAX_ASYNC_OPERATIONS];
    uint32_t active_count;
    uint32_t next_id;
    pthread_t worker_threads[4];
    bool workers_running;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} io_caching_async_system_t;

/* Asset bundle entry */
typedef struct {
    char name[256];
    void* data;
    size_t size;
    size_t compressed_size;
    uint32_t checksum;
    uint64_t timestamp;
} io_caching_asset_bundle_entry_t;

/* Asset bundle */
typedef struct {
    io_caching_asset_bundle_entry_t entries[64];
    uint32_t entry_count;
    char version[32];
    uint32_t total_checksum;
    bool compressed;
} io_caching_asset_bundle_t;

/* Serialization context */
typedef struct {
    uint32_t magic_number;
    uint32_t version;
    uint64_t timestamp;
    uint32_t checksum;
    size_t data_size;
    void* data;
} io_caching_serialization_context_t;

/* Memory barrier for thread safety */
typedef struct {
    pthread_mutex_t mutex;
    volatile bool initialized;
    volatile uint32_t generation;
} io_caching_memory_barrier_t;

/*
 * IO_CACHING_MANAGER_01 - Core data structure
 * Manages state and resources for manager_01 operations
 */
typedef struct io_caching_asset_bundle {
    char bundle_name[256];
    uint32_t num_assets;
    char** asset_names;
    void** asset_data;
    size_t* asset_sizes;
    bool is_compressed;
    uint64_t bundle_size;
    uint64_t creation_timestamp;
} io_caching_asset_bundle_t;

/*
 * Telemetry and performance counters
 */
typedef struct io_caching_telemetry {
    uint64_t files_loaded;
    uint64_t bytes_processed;
    uint64_t compression_operations;
    uint64_t decompression_operations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t async_operations_completed;
    uint64_t hot_reload_events;
    double total_load_time_ms;
    double total_compression_time_ms;
    pthread_mutex_t telemetry_mutex;
} io_caching_telemetry_t;

/*
 * Batch processing context
 */
typedef struct io_caching_batch_context {
    void** batch_items;
    uint32_t batch_size;
    uint32_t max_batch_size;
    pthread_t worker_threads[4]; /* 4 worker threads */
    volatile bool batch_processing_active;
    pthread_mutex_t batch_mutex;
    pthread_cond_t batch_condition;
} io_caching_batch_context_t;

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
    
    /* Resource pooling for reduced allocation overhead */
    io_caching_resource_pool_t resource_pool;
    
    /* Format conversion system */
    io_caching_format_conversion_t format_conversion;
    
    /* Hot-reload file watching */
    io_caching_hot_reload_t hot_reload;
    
    /* Telemetry and performance counters */
    io_caching_telemetry_t telemetry;
    
    /* Async initialization and operations */
    io_caching_async_system_t async_system;
    
    /* Asset bundling system */
    io_caching_asset_bundle_t asset_bundle;
    
    /* Binary serialization context */
    io_caching_serialization_context_t serialization;
    
    /* Thread-safe initialization barrier */
    io_caching_memory_barrier_t memory_barrier;
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
    
    /* Extended telemetry stats */
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t operations_completed;
    uint64_t operations_failed;
    double compression_ratio;
    uint64_t async_operations_completed;
    uint64_t file_reload_events;
} io_caching_manager_01_stats_t;

/*
 * IO_CACHING_MANAGER_01 - Enhanced core data structure
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
    
    /* Resource pooling for reduced allocation overhead */
    io_caching_resource_pool_t resource_pool;
    
    /* Format conversion system */
    io_caching_format_conversion_t format_conversion;
    
    /* Hot-reload file watching */
    io_caching_hot_reload_t hot_reload;
    
    /* Telemetry and performance counters */
    io_caching_telemetry_t telemetry;
    
    /* Async initialization and operations */
    io_caching_async_system_t async_system;
    
    /* Asset bundling system */
    io_caching_asset_bundle_t asset_bundle;
    
    /* Binary serialization context */
    io_caching_serialization_context_t serialization;
    
    /* Thread-safe initialization barrier */
    io_caching_memory_barrier_t memory_barrier;
} io_caching_manager_01_t;

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static io_caching_manager_01_stats_t s_manager_01_stats = {0};
static bool s_manager_01_initialized = false;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/* Resource pooling functions */
static int io_caching_init_resource_pool(io_caching_resource_pool_t* pool) {
    if (!pool) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    memset(pool, 0, sizeof(io_caching_resource_pool_t));
    pool->capacity = IO_CACHING_MANAGER_01_MAX_RESOURCES;
    
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        return IO_CACHING_MANAGER_ERROR_THREAD_FAILED;
    }
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static void* io_caching_pool_allocate(io_caching_resource_pool_t* pool, size_t size, const char* name) {
    if (!pool || !name) return NULL;
    
    pthread_mutex_lock(&pool->mutex);
    
    /* Find free entry */
    for (uint32_t i = 0; i < pool->capacity; i++) {
        if (!pool->entries[i].in_use) {
            pool->entries[i].resource = malloc(size);
            if (pool->entries[i].resource) {
                pool->entries[i].size = size;
                pool->entries[i].ref_count = 1;
                pool->entries[i].in_use = true;
                pool->entries[i].last_access = io_caching_get_time_ns();
                strncpy(pool->entries[i].name, name, sizeof(pool->entries[i].name) - 1);
                pool->entries[i].name[sizeof(pool->entries[i].name) - 1] = '\0';
                
                pool->active_count++;
                pool->total_allocated += size;
                if (pool->active_count > pool->peak_count) {
                    pool->peak_count = pool->active_count;
                }
                
                pthread_mutex_unlock(&pool->mutex);
                return pool->entries[i].resource;
            }
        }
    }
    
    pthread_mutex_unlock(&pool->mutex);
    return NULL;
}

static void io_caching_pool_deallocate(io_caching_resource_pool_t* pool, void* resource) {
    if (!pool || !resource) return;
    
    pthread_mutex_lock(&pool->mutex);
    
    for (uint32_t i = 0; i < pool->capacity; i++) {
        if (pool->entries[i].resource == resource) {
            pool->entries[i].ref_count--;
            if (pool->entries[i].ref_count <= 0) {
                free(pool->entries[i].resource);
                pool->entries[i].resource = NULL;
                pool->entries[i].in_use = false;
                pool->entries[i].size = 0;
                pool->active_count--;
            }
            break;
        }
    }
    
    pthread_mutex_unlock(&pool->mutex);
}

static void io_caching_cleanup_resource_pool(io_caching_resource_pool_t* pool) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->mutex);
    
    for (uint32_t i = 0; i < pool->capacity; i++) {
        if (pool->entries[i].resource) {
            free(pool->entries[i].resource);
            pool->entries[i].resource = NULL;
        }
    }
    
    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);
}

/* Format conversion functions */
static int io_caching_init_format_conversion(io_caching_format_conversion_t* conversion) {
    if (!conversion) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    memset(conversion, 0, sizeof(io_caching_format_conversion_t));
    
    if (pthread_mutex_init(&conversion->mutex, NULL) != 0) {
        return IO_CACHING_MANAGER_ERROR_THREAD_FAILED;
    }
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static int io_caching_register_format_converter(io_caching_format_conversion_t* conversion,
                                           const char* source_format, const char* target_format,
                                           io_caching_format_converter_t converter) {
    if (!conversion || !source_format || !target_format || !converter) {
        return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&conversion->mutex);
    
    if (conversion->converter_count >= 16) {
        pthread_mutex_unlock(&conversion->mutex);
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = conversion->converter_count++;
    strncpy(conversion->converters[index].source_format, source_format, 
            sizeof(conversion->converters[index].source_format) - 1);
    strncpy(conversion->converters[index].target_format, target_format,
            sizeof(conversion->converters[index].target_format) - 1);
    conversion->converters[index].converter = converter;
    
    pthread_mutex_unlock(&conversion->mutex);
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static int io_caching_convert_format(io_caching_format_conversion_t* conversion,
                                     const void* input, size_t input_size,
                                     void** output, size_t* output_size,
                                     const char* target_format) {
    if (!conversion || !input || !output || !output_size || !target_format) {
        return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&conversion->mutex);
    
    /* Find appropriate converter */
    for (uint32_t i = 0; i < conversion->converter_count; i++) {
        if (strcmp(conversion->converters[i].target_format, target_format) == 0) {
            int result = conversion->converters[i].converter(input, input_size, output, output_size, target_format);
            pthread_mutex_unlock(&conversion->mutex);
            return result;
        }
    }
    
    pthread_mutex_unlock(&conversion->mutex);
    return IO_CACHING_MANAGER_ERROR_INVALID_FORMAT;
}

static void io_caching_cleanup_format_conversion(io_caching_format_conversion_t* conversion) {
    if (!conversion) return;
    pthread_mutex_destroy(&conversion->mutex);
}

/* Hot-reload file watching functions */
static int io_caching_init_hot_reload(io_caching_hot_reload_t* hot_reload) {
    if (!hot_reload) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    memset(hot_reload, 0, sizeof(io_caching_hot_reload_t));
    
    hot_reload->inotify_fd = inotify_init();
    if (hot_reload->inotify_fd < 0) {
        return IO_CACHING_MANAGER_ERROR_THREAD_FAILED;
    }
    
    if (pthread_mutex_init(&hot_reload->mutex, NULL) != 0) {
        close(hot_reload->inotify_fd);
        return IO_CACHING_MANAGER_ERROR_THREAD_FAILED;
    }
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static int io_caching_add_file_watch(io_caching_hot_reload_t* hot_reload, const char* path,
                                     void (*callback)(const char*, void*), void* user_data) {
    if (!hot_reload || !path || !callback) {
        return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&hot_reload->mutex);
    
    if (hot_reload->watch_count >= IO_CACHING_MANAGER_01_MAX_FILE_WATCHES) {
        pthread_mutex_unlock(&hot_reload->mutex);
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    
    int wd = inotify_add_watch(hot_reload->inotify_fd, path, IN_MODIFY);
    if (wd < 0) {
        pthread_mutex_unlock(&hot_reload->mutex);
        return IO_CACHING_MANAGER_ERROR_FILE_NOT_FOUND;
    }
    
    uint32_t index = hot_reload->watch_count++;
    hot_reload->watches[index].wd = wd;
    strncpy(hot_reload->watches[index].path, path, sizeof(hot_reload->watches[index].path) - 1);
    hot_reload->watches[index].path[sizeof(hot_reload->watches[index].path) - 1] = '\0';
    hot_reload->watches[index].callback = callback;
    hot_reload->watches[index].user_data = user_data;
    hot_reload->watches[index].last_modified = 0;
    hot_reload->watches[index].pending_reload = false;
    
    pthread_mutex_unlock(&hot_reload->mutex);
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static void* io_caching_file_watch_thread(void* arg) {
    io_caching_hot_reload_t* hot_reload = (io_caching_hot_reload_t*)arg;
    if (!hot_reload) return NULL;
    
    char buffer[4096];
    while (hot_reload->watch_thread_running) {
        int length = read(hot_reload->inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            pthread_mutex_lock(&hot_reload->mutex);
            
            for (int i = 0; i < length; ) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                
                for (uint32_t j = 0; j < hot_reload->watch_count; j++) {
                    if (hot_reload->watches[j].wd == event->wd) {
                        if (event->mask & IN_MODIFY) {
                            uint32_t current_time = (uint32_t)time(NULL);
                            if (current_time - hot_reload->watches[j].last_modified > IO_CACHING_MANAGER_01_DEBOUNCE_MS / 1000) {
                                hot_reload->watches[j].pending_reload = true;
                                hot_reload->watches[j].last_modified = current_time;
                                hot_reload->telemetry.file_reload_events++;
                            }
                        }
                        break;
                    }
                }
                
                i += sizeof(struct inotify_event) + event->len;
            }
            
            pthread_mutex_unlock(&hot_reload->mutex);
        }
        
        usleep(100000); /* 100ms */
    }
    
    return NULL;
}

static void io_caching_cleanup_hot_reload(io_caching_hot_reload_t* hot_reload) {
    if (!hot_reload) return;
    
    hot_reload->watch_thread_running = false;
    if (hot_reload->watch_thread) {
        pthread_join(hot_reload->watch_thread, NULL);
    }
    
    for (uint32_t i = 0; i < hot_reload->watch_count; i++) {
        if (hot_reload->watches[i].wd >= 0) {
            inotify_rm_watch(hot_reload->inotify_fd, hot_reload->watches[i].wd);
        }
    }
    
    if (hot_reload->inotify_fd >= 0) {
        close(hot_reload->inotify_fd);
    }
    
    pthread_mutex_destroy(&hot_reload->mutex);
}

/* Telemetry functions */
static void io_caching_init_telemetry(io_caching_telemetry_t* telemetry) {
    if (!telemetry) return;
    memset(telemetry, 0, sizeof(io_caching_telemetry_t));
}

static void io_caching_update_telemetry(io_caching_telemetry_t* telemetry,
                                       bool success, uint64_t bytes_processed,
                                       uint64_t time_ns, uint32_t compression_ratio) {
    if (!telemetry) return;
    
    if (success) {
        telemetry->operations_completed++;
        telemetry->total_bytes_processed += bytes_processed;
        telemetry->total_time_ns += time_ns;
        telemetry->compression_ratio_sum += compression_ratio;
        telemetry->compression_count++;
    } else {
        telemetry->operations_failed++;
    }
}

static void io_caching_get_telemetry_stats(const io_caching_telemetry_t* telemetry,
                                          io_caching_manager_01_stats_t* stats) {
    if (!telemetry || !stats) return;
    
    stats->cache_hits = telemetry->cache_hits;
    stats->cache_misses = telemetry->cache_misses;
    stats->operations_completed = telemetry->operations_completed;
    stats->operations_failed = telemetry->operations_failed;
    stats->async_operations_completed = telemetry->async_operations_completed;
    stats->file_reload_events = telemetry->file_reload_events;
    
    if (telemetry->compression_count > 0) {
        stats->compression_ratio = (double)telemetry->compression_ratio_sum / telemetry->compression_count;
    }
}

/* Async operation functions */
static int io_caching_init_async_system(io_caching_async_system_t* async) {
    if (!async) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    memset(async, 0, sizeof(io_caching_async_system_t));
    
    if (pthread_mutex_init(&async->mutex, NULL) != 0) {
        return IO_CACHING_MANAGER_ERROR_THREAD_FAILED;
    }
    
    if (pthread_cond_init(&async->condition, NULL) != 0) {
        pthread_mutex_destroy(&async->mutex);
        return IO_CACHING_MANAGER_ERROR_THREAD_FAILED;
    }
    
    async->workers_running = true;
    for (int i = 0; i < 4; i++) {
        if (pthread_create(&async->worker_threads[i], NULL, io_caching_async_worker_thread, async) != 0) {
            async->workers_running = false;
            io_caching_cleanup_async_system(async);
            return IO_CACHING_MANAGER_ERROR_THREAD_FAILED;
        }
    }
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static uint32_t io_caching_start_async_operation(io_caching_async_system_t* async,
                                                void (*operation)(void*), void* user_data,
                                                void (*callback)(void*, void*)) {
    if (!async || !operation) return 0;
    
    pthread_mutex_lock(&async->mutex);
    
    if (async->active_count >= IO_CACHING_MANAGER_01_MAX_ASYNC_OPERATIONS) {
        pthread_mutex_unlock(&async->mutex);
        return 0;
    }
    
    uint32_t id = ++async->next_id;
    uint32_t index = 0;
    for (; index < IO_CACHING_MANAGER_01_MAX_ASYNC_OPERATIONS; index++) {
        if (!async->operations[index].completed) {
            break;
        }
    }
    
    async->operations[index].id = id;
    async->operations[index].completed = false;
    async->operations[index].failed = false;
    async->operations[index].callback = callback;
    async->operations[index].user_data = user_data;
    async->operations[index].start_time_ns = io_caching_get_time_ns();
    async->active_count++;
    
    pthread_cond_signal(&async->condition);
    pthread_mutex_unlock(&async->mutex);
    
    return id;
}

static void* io_caching_async_worker_thread(void* arg) {
    io_caching_async_system_t* async = (io_caching_async_system_t*)arg;
    if (!async) return NULL;
    
    while (async->workers_running) {
        pthread_mutex_lock(&async->mutex);
        
        /* Wait for work */
        while (async->workers_running && async->active_count == 0) {
            pthread_cond_wait(&async->condition, &async->mutex);
        }
        
        if (!async->workers_running) {
            pthread_mutex_unlock(&async->mutex);
            break;
        }
        
        /* Find pending operation */
        for (uint32_t i = 0; i < IO_CACHING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
            if (!async->operations[i].completed && !async->operations[i].failed) {
                async->operations[i].completed = true;
                async->operations[i].end_time_ns = io_caching_get_time_ns();
                async->telemetry.async_operations_completed++;
                
                pthread_mutex_unlock(&async->mutex);
                
                /* Execute callback */
                if (async->operations[i].callback) {
                    async->operations[i].callback(async->operations[i].result, async->operations[i].user_data);
                }
                
                pthread_mutex_lock(&async->mutex);
                async->active_count--;
                break;
            }
        }
        
        pthread_mutex_unlock(&async->mutex);
    }
    
    return NULL;
}

static void io_caching_cleanup_async_system(io_caching_async_system_t* async) {
    if (!async) return;
    
    async->workers_running = false;
    pthread_cond_broadcast(&async->condition);
    
    for (int i = 0; i < 4; i++) {
        if (async->worker_threads[i]) {
            pthread_join(async->worker_threads[i], NULL);
        }
    }
    
    pthread_mutex_destroy(&async->mutex);
    pthread_cond_destroy(&async->condition);
}

/* Asset bundling functions */
static int io_caching_init_asset_bundle(io_caching_asset_bundle_t* bundle) {
    if (!bundle) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    memset(bundle, 0, sizeof(io_caching_asset_bundle_t));
    strncpy(bundle->version, "1.0", sizeof(bundle->version) - 1);
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static int io_caching_add_to_bundle(io_caching_asset_bundle_t* bundle,
                                   const char* name, const void* data, size_t size) {
    if (!bundle || !name || !data || size == 0) {
        return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    }
    
    if (bundle->entry_count >= 64) {
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = bundle->entry_count++;
    strncpy(bundle->entries[index].name, name, sizeof(bundle->entries[index].name) - 1);
    bundle->entries[index].name[sizeof(bundle->entries[index].name) - 1] = '\0';
    
    bundle->entries[index].data = malloc(size);
    if (!bundle->entries[index].data) {
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(bundle->entries[index].data, data, size);
    bundle->entries[index].size = size;
    bundle->entries[index].checksum = io_caching_calculate_checksum(data, size);
    bundle->entries[index].timestamp = io_caching_get_time_ns();
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static int io_caching_compress_bundle(io_caching_asset_bundle_t* bundle) {
    if (!bundle) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    /* Simple compression simulation */
    bundle->compressed = true;
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static void io_caching_cleanup_asset_bundle(io_caching_asset_bundle_t* bundle) {
    if (!bundle) return;
    
    for (uint32_t i = 0; i < bundle->entry_count; i++) {
        if (bundle->entries[i].data) {
            free(bundle->entries[i].data);
            bundle->entries[i].data = NULL;
        }
    }
}

/* Binary serialization functions */
static int io_caching_init_serialization(io_caching_serialization_context_t* serialization) {
    if (!serialization) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    memset(serialization, 0, sizeof(io_caching_serialization_context_t));
    serialization->magic_number = IO_CACHING_MANAGER_MAGIC_NUMBER;
    serialization->version = IO_CACHING_MANAGER_SERIALIZATION_VERSION;
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static int io_caching_serialize_data(const void* data, size_t size,
                                     io_caching_serialization_context_t* serialization) {
    if (!data || !serialization || size == 0) {
        return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    }
    
    serialization->data = malloc(size);
    if (!serialization->data) {
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(serialization->data, data, size);
    serialization->data_size = size;
    serialization->timestamp = io_caching_get_time_ns();
    serialization->checksum = io_caching_calculate_checksum(data, size);
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static int io_caching_deserialize_data(const io_caching_serialization_context_t* serialization,
                                       void** data, size_t* size) {
    if (!serialization || !data || !size) {
        return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    }
    
    if (serialization->magic_number != IO_CACHING_MANAGER_MAGIC_NUMBER) {
        return IO_CACHING_MANAGER_ERROR_INVALID_FORMAT;
    }
    
    uint32_t calculated_checksum = io_caching_calculate_checksum(serialization->data, serialization->data_size);
    if (calculated_checksum != serialization->checksum) {
        return IO_CACHING_MANAGER_ERROR_SERIALIZATION_FAILED;
    }
    
    *data = malloc(serialization->data_size);
    if (!*data) {
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(*data, serialization->data, serialization->data_size);
    *size = serialization->data_size;
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static void io_caching_cleanup_serialization(io_caching_serialization_context_t* serialization) {
    if (!serialization) return;
    
    if (serialization->data) {
        free(serialization->data);
        serialization->data = NULL;
    }
}

/* Memory barrier functions */
static void io_caching_init_memory_barrier(io_caching_memory_barrier_t* barrier) {
    if (!barrier) return;
    
    pthread_mutex_init(&barrier->mutex, NULL);
    barrier->initialized = false;
    barrier->generation = 0;
}

static void io_caching_memory_barrier_sync(io_caching_memory_barrier_t* barrier) {
    if (!barrier) return;
    
    pthread_mutex_lock(&barrier->mutex);
    __sync_synchronize();
    barrier->generation++;
    barrier->initialized = true;
    pthread_mutex_unlock(&barrier->mutex);
}

static void io_caching_cleanup_memory_barrier(io_caching_memory_barrier_t* barrier) {
    if (!barrier) return;
    pthread_mutex_destroy(&barrier->mutex);
}

/* Utility functions */
static uint32_t io_caching_calculate_checksum(const void* data, size_t size) {
    if (!data || size == 0) return 0;
    
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < size; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ bytes[i];
    }
    
    return checksum;
}

static uint64_t io_caching_get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static const char* io_caching_error_string(io_caching_manager_error_t error) {
    switch (error) {
        case IO_CACHING_MANAGER_ERROR_NONE: return "No error";
        case IO_CACHING_MANAGER_ERROR_INVALID_PARAM: return "Invalid parameter";
        case IO_CACHING_MANAGER_ERROR_NOT_INITIALIZED: return "Not initialized";
        case IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case IO_CACHING_MANAGER_ERROR_FILE_NOT_FOUND: return "File not found";
        case IO_CACHING_MANAGER_ERROR_INVALID_FORMAT: return "Invalid format";
        case IO_CACHING_MANAGER_ERROR_COMPRESSION_FAILED: return "Compression failed";
        case IO_CACHING_MANAGER_ERROR_THREAD_FAILED: return "Threading failed";
        default: return "Unknown error";
    }
}

/* Compression helper functions */
static int io_caching_init_compression(io_caching_compression_context_t* ctx) {
    if (!ctx) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    memset(ctx, 0, sizeof(io_caching_compression_context_t));
    
#ifdef ENABLE_LZ4
    ctx->lz4_buffer_size = LZ4_compressBound(IO_CACHING_MANAGER_DEFAULT_MEMORY_BUDGET);
    ctx->lz4_context = malloc(ctx->lz4_buffer_size);
    if (!ctx->lz4_context) return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
#endif

#ifdef ENABLE_ZSTD
    ctx->zstd_cctx = ZSTD_createCCtx();
    ctx->zstd_dctx = ZSTD_createDCtx();
    if (!ctx->zstd_cctx || !ctx->zstd_dctx) {
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    ctx->compression_level = 3; /* Default compression level */
#endif

    ctx->compression_type = 2; /* Default to ZSTD if available */
#ifdef ENABLE_ZSTD
    ctx->compression_type = 2;
#elif defined(ENABLE_LZ4)
    ctx->compression_type = 1;
#endif
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static int io_caching_compress_data(io_caching_compression_context_t* ctx, 
                                    const void* input, size_t input_size,
                                    void* output, size_t* output_size) {
    if (!ctx || !input || !output || !output_size) {
        return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    }
    
    clock_t start = clock();
    int result = IO_CACHING_MANAGER_ERROR_NONE;
    
    switch (ctx->compression_type) {
#ifdef ENABLE_LZ4
        case 1: {
            int compressed_size = LZ4_compress_default(
                (const char*)input, (char*)output, 
                (int)input_size, (int)*output_size);
            if (compressed_size <= 0) {
                result = IO_CACHING_MANAGER_ERROR_COMPRESSION_FAILED;
            } else {
                *output_size = compressed_size;
                ctx->compression_ratio = (double)input_size / compressed_size;
            }
            break;
        }
#endif
#ifdef ENABLE_ZSTD
        case 2: {
            size_t compressed_size = ZSTD_compressCCtx(
                ctx->zstd_cctx, output, *output_size,
                input, input_size, ctx->compression_level);
            if (ZSTD_isError(compressed_size)) {
                result = IO_CACHING_MANAGER_ERROR_COMPRESSION_FAILED;
            } else {
                *output_size = compressed_size;
                ctx->compression_ratio = (double)input_size / compressed_size;
            }
            break;
        }
#endif
        default:
            /* No compression */
            memcpy(output, input, input_size);
            *output_size = input_size;
            ctx->compression_ratio = 1.0;
            break;
    }
    
    clock_t end = clock();
    double time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    /* Update telemetry */
    pthread_mutex_lock(&ctx->telemetry_mutex);
    ctx->compression_operations++;
    ctx->total_compression_time_ms += time_ms;
    pthread_mutex_unlock(&ctx->telemetry_mutex);
    
    return result;
}

/* File watching helper functions */
static void* io_caching_file_watcher_thread(void* arg) {
    io_caching_file_watcher_t* watcher = (io_caching_file_watcher_t*)arg;
    if (!watcher) return NULL;
    
    char buffer[4096];
    while (watcher->watcher_running) {
        ssize_t length = read(watcher->inotify_fd, buffer, sizeof(buffer));
        if (length < 0) {
            if (errno == EINTR) continue;
            break;
        }
        
        size_t i = 0;
        while (i < length) {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];
            if (event->mask & IN_MODIFY) {
                for (int j = 0; j < watcher->num_watched_files; j++) {
                    if (event->wd == watcher->watch_descriptors[j]) {
                        if (watcher->reload_callback) {
                            watcher->reload_callback(watcher->watched_files[j], 
                                                   watcher->callback_user_data);
                        }
                        break;
                    }
                }
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }
    
    return NULL;
}

static int io_caching_init_file_watcher(io_caching_file_watcher_t* watcher) {
    if (!watcher) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    memset(watcher, 0, sizeof(io_caching_file_watcher_t));
    
    watcher->inotify_fd = inotify_init();
    if (watcher->inotify_fd < 0) {
        return IO_CACHING_MANAGER_ERROR_FILE_NOT_FOUND;
    }
    
    watcher->watcher_running = true;
    if (pthread_create(&watcher->watcher_thread, NULL, 
                       io_caching_file_watcher_thread, watcher) != 0) {
        close(watcher->inotify_fd);
        return IO_CACHING_MANAGER_ERROR_THREAD_FAILED;
    }
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

/* Resource pool helper functions */
static int io_caching_init_resource_pool(io_caching_resource_pool_t* pool) {
    if (!pool) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    memset(pool, 0, sizeof(io_caching_resource_pool_t));
    
    if (pthread_mutex_init(&pool->pool_mutex, NULL) != 0) {
        return IO_CACHING_MANAGER_ERROR_THREAD_FAILED;
    }
    
    pool->pool_size = IO_CACHING_MANAGER_MAX_RESOURCE_POOL_SIZE;
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static void* io_caching_pool_allocate(io_caching_resource_pool_t* pool, size_t size) {
    if (!pool) return NULL;
    
    pthread_mutex_lock(&pool->pool_mutex);
    
    for (uint32_t i = 0; i < pool->pool_size; i++) {
        if (!pool->resource_used[i]) {
            if (!pool->resources[i] || pool->resource_sizes[i] < size) {
                free(pool->resources[i]);
                pool->resources[i] = malloc(size);
                if (!pool->resources[i]) {
                    pthread_mutex_unlock(&pool->pool_mutex);
                    return NULL;
                }
                pool->resource_sizes[i] = size;
            }
            
            pool->resource_used[i] = true;
            if (i + 1 > pool->peak_usage) {
                pool->peak_usage = i + 1;
            }
            
            pthread_mutex_unlock(&pool->pool_mutex);
            return pool->resources[i];
        }
    }
    
    pthread_mutex_unlock(&pool->pool_mutex);
    return NULL; /* Pool exhausted */
}

/* Scene parsing helper functions */
#ifdef ENABLE_CGLTF
static int io_caching_parse_gltf_scene(const char* filename, io_caching_scene_data_t* scene) {
    if (!filename || !scene) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    cgltf_options options = {0};
    cgltf_data* data = NULL;
    
    cgltf_result result = cgltf_parse_file(&options, filename, &data);
    if (result != cgltf_result_success) {
        return IO_CACHING_MANAGER_ERROR_PARSING_FAILED;
    }
    
    result = cgltf_load_buffers(&options, data, filename);
    if (result != cgltf_result_success) {
        cgltf_free(data);
        return IO_CACHING_MANAGER_ERROR_PARSING_FAILED;
    }
    
    /* Copy scene data */
    strncpy(scene->scene_name, filename, sizeof(scene->scene_name) - 1);
    strcpy(scene->format, "glTF");
    scene->num_nodes = data->nodes_count;
    scene->num_meshes = data->meshes_count;
    scene->num_materials = data->materials_count;
    scene->num_textures = data->textures_count;
    scene->is_loaded = true;
    scene->load_timestamp = time(NULL);
    
    /* Note: In a real implementation, we would copy the actual data
       instead of just storing pointers to the cgltf data */
    scene->scene_nodes = (void*)data->nodes;
    scene->mesh_data = (void*)data->meshes;
    scene->material_data = (void*)data->materials;
    scene->texture_data = (void*)data->textures;
    
    cgltf_free(data);
    return IO_CACHING_MANAGER_ERROR_NONE;
}
#endif

/* Memory budget helper functions */
static int io_caching_init_memory_budget(io_caching_memory_budget_t* budget, size_t total_budget) {
    if (!budget) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    memset(budget, 0, sizeof(io_caching_memory_budget_t));
    
    if (pthread_mutex_init(&budget->budget_mutex, NULL) != 0) {
        return IO_CACHING_MANAGER_ERROR_THREAD_FAILED;
    }
    
    budget->total_budget = total_budget;
    budget->eviction_threshold = 0.9; /* Evict at 90% capacity */
    budget->auto_eviction_enabled = true;
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

static bool io_caching_check_memory_budget(io_caching_memory_budget_t* budget, size_t requested_size) {
    if (!budget) return false;
    
    pthread_mutex_lock(&budget->budget_mutex);
    
    bool can_allocate = (budget->current_usage + requested_size <= budget->total_budget);
    
    if (can_allocate) {
        budget->current_usage += requested_size;
        if (budget->current_usage > budget->peak_usage) {
            budget->peak_usage = budget->current_usage;
        }
    }
    
    pthread_mutex_unlock(&budget->budget_mutex);
    return can_allocate;
}

/* Binary serialization helper functions */
static uint32_t io_caching_calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < size; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ bytes[i];
    }
    
    return checksum;
}

static int io_caching_serialize_data(const void* input, size_t input_size,
                                   io_caching_serialization_context_t* ctx) {
    if (!input || !ctx) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    
    ctx->magic_number = IO_CACHING_MANAGER_MAGIC_NUMBER;
    ctx->version = IO_CACHING_MANAGER_SERIALIZATION_VERSION;
    ctx->timestamp = time(NULL);
    ctx->data_size = input_size;
    
    /* Allocate space for header + data */
    size_t total_size = sizeof(io_caching_serialization_context_t) + input_size;
    ctx->serialized_data = malloc(total_size);
    if (!ctx->serialized_data) {
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    
    /* Copy header */
    memcpy(ctx->serialized_data, ctx, sizeof(io_caching_serialization_context_t));
    
    /* Copy data */
    uint8_t* data_ptr = (uint8_t*)ctx->serialized_data + sizeof(io_caching_serialization_context_t);
    memcpy(data_ptr, input, input_size);
    
    /* Calculate checksum */
    ctx->checksum = io_caching_calculate_checksum(data_ptr, input_size);
    
    /* Update checksum in header */
    ((io_caching_serialization_context_t*)ctx->serialized_data)->checksum = ctx->checksum;
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int io_caching_manager_01_validate_internal(io_caching_manager_01_t* ctx);
static int io_caching_manager_01_cleanup_internal(io_caching_manager_01_t* ctx);

/* Resource pooling functions */
static int io_caching_init_resource_pool(io_caching_resource_pool_t* pool);
static void* io_caching_pool_allocate(io_caching_resource_pool_t* pool, size_t size, const char* name);
static void io_caching_pool_deallocate(io_caching_resource_pool_t* pool, void* resource);
static void io_caching_cleanup_resource_pool(io_caching_resource_pool_t* pool);

/* Format conversion functions */
static int io_caching_init_format_conversion(io_caching_format_conversion_t* conversion);
static int io_caching_register_format_converter(io_caching_format_conversion_t* conversion,
                                               const char* source_format, const char* target_format,
                                               io_caching_format_converter_t converter);
static int io_caching_convert_format(io_caching_format_conversion_t* conversion,
                                     const void* input, size_t input_size,
                                     void** output, size_t* output_size,
                                     const char* target_format);
static void io_caching_cleanup_format_conversion(io_caching_format_conversion_t* conversion);

/* Hot-reload file watching functions */
static int io_caching_init_hot_reload(io_caching_hot_reload_t* hot_reload);
static int io_caching_add_file_watch(io_caching_hot_reload_t* hot_reload, const char* path,
                                     void (*callback)(const char*, void*), void* user_data);
static void* io_caching_file_watch_thread(void* arg);
static void io_caching_cleanup_hot_reload(io_caching_hot_reload_t* hot_reload);

/* Telemetry functions */
static void io_caching_init_telemetry(io_caching_telemetry_t* telemetry);
static void io_caching_update_telemetry(io_caching_telemetry_t* telemetry,
                                       bool success, uint64_t bytes_processed,
                                       uint64_t time_ns, uint32_t compression_ratio);
static void io_caching_get_telemetry_stats(const io_caching_telemetry_t* telemetry,
                                          io_caching_manager_01_stats_t* stats);

/* Async operation functions */
static int io_caching_init_async_system(io_caching_async_system_t* async);
static uint32_t io_caching_start_async_operation(io_caching_async_system_t* async,
                                                void (*operation)(void*), void* user_data,
                                                void (*callback)(void*, void*));
static void* io_caching_async_worker_thread(void* arg);
static void io_caching_cleanup_async_system(io_caching_async_system_t* async);

/* Asset bundling functions */
static int io_caching_init_asset_bundle(io_caching_asset_bundle_t* bundle);
static int io_caching_add_to_bundle(io_caching_asset_bundle_t* bundle,
                                   const char* name, const void* data, size_t size);
static int io_caching_compress_bundle(io_caching_asset_bundle_t* bundle);
static void io_caching_cleanup_asset_bundle(io_caching_asset_bundle_t* bundle);

/* Binary serialization functions */
static int io_caching_init_serialization(io_caching_serialization_context_t* serialization);
static int io_caching_serialize_data(const void* data, size_t size,
                                     io_caching_serialization_context_t* serialization);
static int io_caching_deserialize_data(const io_caching_serialization_context_t* serialization,
                                       void** data, size_t* size);
static void io_caching_cleanup_serialization(io_caching_serialization_context_t* serialization);

/* Memory barrier functions */
static void io_caching_init_memory_barrier(io_caching_memory_barrier_t* barrier);
static void io_caching_memory_barrier_sync(io_caching_memory_barrier_t* barrier);
static void io_caching_cleanup_memory_barrier(io_caching_memory_barrier_t* barrier);

/* Utility functions */
static uint32_t io_caching_calculate_checksum(const void* data, size_t size);
static uint64_t io_caching_get_time_ns(void);
    if (!ctx) {
        return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    }
    
    /* Format conversion */
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_SCENE_PARSING) {
        /* Optimize format conversion registry */
        pthread_mutex_lock(&ctx->format_conversion.mutex);
        /* Sort converters by usage frequency for optimization */
        for (uint32_t i = 0; i < ctx->format_conversion.converter_count - 1; i++) {
            for (uint32_t j = i + 1; j < ctx->format_conversion.converter_count; j++) {
                /* Swap if needed for optimization */
                if (ctx->format_conversion.converters[i].usage_count < ctx->format_conversion.converters[j].usage_count) {
                    io_caching_format_converter_t temp = ctx->format_conversion.converters[i];
                    ctx->format_conversion.converters[i] = ctx->format_conversion.converters[j];
                    ctx->format_conversion.converters[j] = temp;
                }
            }
        }
        pthread_mutex_unlock(&ctx->format_conversion.mutex);
    }
    
    /* Add asset streaming priority */
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_MEMORY_BUDGET) {
        pthread_mutex_lock(&ctx->memory_budget.budget_mutex);
        /* Optimize memory usage by prioritizing frequently accessed assets */
        if (ctx->memory_budget.current_usage > ctx->memory_budget.total_budget * 0.8) {
            /* Trigger optimization pass */
            ctx->memory_budget.eviction_count++;
            /* Evict least recently used assets */
            for (uint32_t i = 0; i < ctx->memory_budget.asset_count; i++) {
                if (ctx->memory_budget.assets[i].last_access_time < ctx->memory_budget.assets[i].creation_time) {
                    /* Evict asset */
                    ctx->memory_budget.current_usage -= ctx->memory_budget.assets[i].size;
                    ctx->memory_budget.asset_count--;
                    /* Update asset list */
                    for (uint32_t j = i; j < ctx->memory_budget.asset_count; j++) {
                        ctx->memory_budget.assets[j] = ctx->memory_budget.assets[j + 1];
                    }
                }
            }
        }
        pthread_mutex_unlock(&ctx->memory_budget.budget_mutex);
    }
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

/*
 * io_caching_manager_01_debug_print
 * Prints debug information about io_caching_manager_01
 */
int io_caching_manager_01_debug_print(io_caching_manager_01_t* ctx) {
    if (!ctx) {
        return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    }
    
    printf("=== Asset Cache Manager Debug Info ===\n");
    
    /* Validation layer integration for debugging builds */
    #ifdef DEBUG
    printf("Debug Mode: ENABLED\n");
    int validation_result = io_caching_manager_01_validate_internal(ctx);
    printf("Validation Result: %s\n", 
           validation_result == IO_CACHING_MANAGER_ERROR_NONE ? "PASS" : "FAIL");
    #endif
    
    /* Async file loading */
        pthread_mutex_init(&ctx->format_conversion.mutex, NULL);
    }
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

/*
 * io_caching_manager_01_get_memory_usage
 * Returns current memory usage
 */
int io_caching_manager_01_get_memory_usage(io_caching_manager_01_t* ctx) {
    if (!ctx) {
        return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    }
    
    size_t total_memory = 0;
    
    /* Telemetry and performance counters for profiling */
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_TELEMETRY) {
        total_memory += sizeof(io_caching_telemetry_t);
    }
    
    /* Hot-reload file watching */
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_HOT_RELOAD) {
        total_memory += sizeof(io_caching_file_watcher_t);
        total_memory += ctx->file_watcher.num_watched_files * 256; /* File paths */
    }
    
    /* Resource pooling */
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_RESOURCE_POOL) {
        total_memory += sizeof(io_caching_resource_pool_t);
        total_memory += ctx->resource_pool.pool_size * sizeof(void*);
    printf("Reference Count: %u\n", ctx->reference_count);
    printf("Last Update Frame: %lu\n", ctx->last_update_frame);
    
    /* Async file loading debug info */
    if (ctx->flags & IO_CACHING_MANAGER_01_FLAG_ASYNC_INIT) {
        printf("Async Init Complete: %s\n", ctx->async_init_complete ? "Yes" : "No");
        printf("Async Init Result: %d\n", ctx->async_init_result);
    }
    
    printf("===================================\n");
    #else
    if (!ctx) return IO_CACHING_MANAGER_ERROR_INVALID_PARAM;
    #endif
    
    return IO_CACHING_MANAGER_ERROR_NONE;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

/*
 * io_caching_manager_01_module_init
 * Initializes the entire manager_01 module
 */
int io_caching_manager_01_module_init(void) {
    /* Async file loading initialization */
    /* In a real implementation, we would initialize async file loading system */
    
    /* Format conversion initialization */
    g_format_converters = malloc(sizeof(io_caching_format_converter_t) * 16);
    if (!g_format_converters) {
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    g_format_converter_count = 0;
    
    /* Multi-threaded batch processing support */
    g_batch_queue = malloc(sizeof(void*) * 1024);
    if (!g_batch_queue) {
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    g_batch_queue_size = 1024;
    g_batch_queue_count = 0;
    
    /* Resource pooling for reduced allocation overhead */
    g_resource_pool = malloc(sizeof(void*) * 1024);
    if (!g_resource_pool) {
        return IO_CACHING_MANAGER_ERROR_OUT_OF_MEMORY;
    }
    g_resource_pool_size = 1024;
    g_resource_pool_count = 0;

    if (s_manager_01_initialized) {
        return 0;  // Already initialized
    }

    /* Initialize statistics */
    memset(&s_manager_01_stats, 0, sizeof(s_manager_01_stats));

    s_manager_01_initialized = true;
    return IO_CACHING_MANAGER_ERROR_NONE;
}

/*
 * io_caching_manager_01_module_shutdown
 * Shuts down the entire manager_01 module
 */
int io_caching_manager_01_module_shutdown(void) {
    /* Validation layer integration for debugging builds */
    #ifdef DEBUG
    printf("Shutting down Asset Cache Manager module\n");
    printf("Peak active count: %lu\n", s_manager_01_stats.peak_count);
    printf("Total allocations: %lu\n", s_manager_01_stats.total_allocations);
    printf("Peak memory usage: %zu bytes\n", s_manager_01_stats.memory_peak);
    #endif
    
    /* Asset bundling cleanup */
    /* In a real implementation, we would clean up asset bundles */
    
    /* Resource pooling for reduced allocation overhead cleanup */
    /* In a real implementation, we would clean up resource pools */
    
    /* Asset streaming priority cleanup */
    /* In a real implementation, we would clean up streaming system */

    if (!s_manager_01_initialized) {
        return 0;  /* Already shut down */
    }

    s_manager_01_initialized = false;
    return IO_CACHING_MANAGER_ERROR_NONE;
}

/* End of io_caching_manager_01.c */
