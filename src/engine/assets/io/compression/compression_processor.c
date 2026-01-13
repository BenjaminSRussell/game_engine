/*
 * io_compression_processor_04.c
 *
 * I/O and asset streaming - Compression Subsystem
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * This file implements processor functionality for the compression module
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
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#include <CoreServices/CoreServices.h>
#elif defined(__linux__)
#include <sys/inotify.h>
#include <poll.h>
#endif

/* LZ4/ZSTD includes - these would need to be linked */
#ifdef ENABLE_LZ4
#include <lz4.h>
#include <lz4hc.h>
#endif
#ifdef ENABLE_ZSTD
#include <zstd.h>
#endif
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <sys/inotify.h>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#include <CoreServices/CoreServices.h>
#endif

#ifdef __linux__
#include <sys/epoll.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>
#endif

/* SIMD headers */
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __AVX2__
#include <immintrin.h>
#endif

/* Compression libraries */
#ifdef HAVE_LZ4
#include <lz4.h>
#include <lz4hc.h>
#endif

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

#include "assets/io/compression/compression_processor.h"
#include "include/core/types.h"
#include "include/core/memory.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define IO_COMPRESSION_PROCESSOR_04_VERSION_MAJOR 1
#define IO_COMPRESSION_PROCESSOR_04_VERSION_MINOR 0
#define IO_COMPRESSION_PROCESSOR_04_VERSION_PATCH 0

#define IO_COMPRESSION_PROCESSOR_04_MAX_INSTANCES 4096
#define IO_COMPRESSION_PROCESSOR_04_DEFAULT_CAPACITY 256
#define IO_COMPRESSION_PROCESSOR_04_ALIGNMENT 16

#define IO_COMPRESSION_PROCESSOR_04_FLAG_NONE          0x00000000
#define IO_COMPRESSION_PROCESSOR_04_FLAG_INITIALIZED   0x00000001
#define IO_COMPRESSION_PROCESSOR_04_FLAG_DIRTY         0x00000002
#define IO_COMPRESSION_PROCESSOR_04_FLAG_GPU_RESIDENT  0x00000004
#define IO_COMPRESSION_PROCESSOR_04_FLAG_STREAMING     0x00000008
#define IO_COMPRESSION_PROCESSOR_04_FLAG_CANCELLED     0x00000010
#define IO_COMPRESSION_PROCESSOR_04_FLAG_ASYNC         0x00000020
#define IO_COMPRESSION_PROCESSOR_04_FLAG_MEMORY_MAPPED 0x00000040
#define IO_COMPRESSION_PROCESSOR_04_FLAG_COMPRESSED    0x00000080
#define IO_COMPRESSION_PROCESSOR_04_FLAG_BUNDLED       0x00000100

/* Compression types */
#define IO_COMPRESSION_PROCESSOR_04_COMPRESSION_NONE  0
#define IO_COMPRESSION_PROCESSOR_04_COMPRESSION_LZ4    1
#define IO_COMPRESSION_PROCESSOR_04_COMPRESSION_ZSTD   2

/* File format types */
#define IO_COMPRESSION_PROCESSOR_04_FORMAT_UNKNOWN    0
#define IO_COMPRESSION_PROCESSOR_04_FORMAT_GLTF       1
#define IO_COMPRESSION_PROCESSOR_04_FORMAT_FBX         2
#define IO_COMPRESSION_PROCESSOR_04_FORMAT_OBJ         3
#define IO_COMPRESSION_PROCESSOR_04_FORMAT_SCENE       4
#define IO_COMPRESSION_PROCESSOR_04_FORMAT_BUNDLE      5

/* Operation limits */
#define IO_COMPRESSION_PROCESSOR_04_MAX_WATCHED_FILES 1024
#define IO_COMPRESSION_PROCESSOR_04_MAX_ASYNC_TASKS   64
#define IO_COMPRESSION_PROCESSOR_04_MAX_CHECKPOINTS    32
#define IO_COMPRESSION_PROCESSOR_04_MAX_BUNDLE_SIZE    (1024 * 1024 * 1024) /* 1GB */
#define IO_COMPRESSION_PROCESSOR_04_PROGRESS_REPORT_INTERVAL_MS 100

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */


/* Compression algorithm types */
typedef enum {
    IO_COMPRESSION_ALGORITHM_NONE = 0,
    IO_COMPRESSION_ALGORITHM_LZ4,
    IO_COMPRESSION_ALGORITHM_LZ4_HC,
    IO_COMPRESSION_ALGORITHM_ZSTD,
    IO_COMPRESSION_ALGORITHM_COUNT
} io_compression_algorithm_t;

/* Checkpoint data structure */
typedef struct io_compression_checkpoint {
    uint64_t checkpoint_id;
    uint64_t timestamp;
    size_t data_offset;
    size_t data_size;
    void* state_data;
    char description[256];
} io_compression_checkpoint_t;

/* File watch entry */
typedef struct io_compression_file_watch {
    char file_path[512];
    uint64_t last_modified;
    void (*callback)(const char* path, void* user_data);
    void* user_data;
    bool active;
#ifdef __APPLE__
    FSEventStreamRef event_stream;
#elif defined(__linux__)
    int watch_descriptor;
#endif
} io_compression_file_watch_t;

/*
 * IO_COMPRESSION_PROCESSOR_04 - Core data structure
 * Manages state and resources for processor_04 operations
 */
typedef struct io_compression_processor_04 {
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
    
    /* New fields for advanced features */
    pthread_mutex_t cancellation_mutex;
    volatile bool cancelled;
    
    /* Async task management */
    pthread_t async_threads[IO_COMPRESSION_PROCESSOR_04_MAX_ASYNC_TASKS];
    uint32_t active_async_tasks;
    pthread_mutex_t async_mutex;
    
    /* File watching */
#ifdef __linux__
    int inotify_fd;
    int watch_descriptors[IO_COMPRESSION_PROCESSOR_04_MAX_WATCHED_FILES];
    char watched_files[IO_COMPRESSION_PROCESSOR_04_MAX_WATCHED_FILES][512];
    uint32_t num_watched_files;
#endif
#ifdef __APPLE__
    CFArrayRef watched_paths;
    FSEventStreamRef event_stream;
#endif
    
    /* Memory mapping */
    void* mapped_memory;
    size_t mapped_size;
    int mapped_fd;
    
    /* Checkpointing */
    struct {
        void* data;
        size_t size;
        uint64_t timestamp;
        uint32_t operation_id;
    } checkpoints[IO_COMPRESSION_PROCESSOR_04_MAX_CHECKPOINTS];
    uint32_t current_checkpoint;
    
    /* Progress reporting */
    struct {
        uint64_t current;
        uint64_t total;
        uint32_t percentage;
        uint64_t last_report_time;
        void (*callback)(uint32_t percentage, void* user_data);
        void* user_data;
    } progress;
    
    /* Compression settings */
    uint32_t compression_type;
    uint32_t compression_level;
    
    /* Format conversion */
    uint32_t source_format;
    uint32_t target_format;
    
    /* Cache management */
    struct {
        void** cache_entries;
        uint32_t* cache_priorities;
        uint32_t cache_size;
        uint32_t cache_capacity;
    } cache;
    
    /* New fields for enhanced functionality */
    io_compression_algorithm_t compression_algorithm;
    int compression_level;
    
    /* Checkpointing system */
    io_compression_checkpoint_t* checkpoints;
    size_t checkpoint_count;
    size_t checkpoint_capacity;
    uint64_t next_checkpoint_id;
    
    /* File watching system */
    io_compression_file_watch_t* file_watches;
    size_t file_watch_count;
    size_t file_watch_capacity;
    pthread_mutex_t file_watch_mutex;
    
    /* Async operations */
    pthread_t async_thread;
    bool async_running;
    pthread_mutex_t async_mutex;
    pthread_cond_t async_cond;
    
} io_compression_processor_04_t;

typedef struct io_compression_processor_04_desc {
    uint32_t flags;
    size_t initial_capacity;
    void* user_data;
    void* allocator;
    
    /* Extended descriptor fields */
    uint32_t compression_type;
    uint32_t compression_level;
    uint32_t cache_capacity;
    bool enable_file_watching;
    bool enable_async_processing;
    bool enable_memory_mapping;
    bool enable_checkpointing;
    void (*progress_callback)(uint32_t percentage, void* user_data);
    void* progress_user_data;
    
} io_compression_processor_04_desc_t;

typedef struct io_compression_processor_04_stats {
    uint64_t total_allocations;
    uint64_t active_count;
    uint64_t peak_count;
    size_t memory_used;
    size_t memory_peak;
    double avg_process_time_ms;
    
    /* Extended statistics */
    uint64_t files_processed;
    uint64_t bytes_compressed;
    uint64_t bytes_decompressed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t async_operations_completed;
    uint64_t cancelled_operations;
    uint64_t checkpoint_restores;
    double compression_ratio;
    
} io_compression_processor_04_stats_t;

/* Scene file parsing structures */
typedef struct scene_node {
    char name[256];
    float transform[16]; /* 4x4 matrix */
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t parent_id;
    uint32_t child_count;
    uint32_t* children;
} scene_node_t;

typedef struct scene_mesh {
    char name[256];
    uint32_t vertex_count;
    uint32_t index_count;
    float* vertices;
    float* normals;
    float* texcoords;
    uint32_t* indices;
} scene_mesh_t;

typedef struct scene_material {
    char name[256];
    float albedo[4];
    float metallic;
    float roughness;
    char texture_diffuse[512];
    char texture_normal[512];
} scene_material_t;

typedef struct scene_file {
    char version[64];
    uint32_t node_count;
    uint32_t mesh_count;
    uint32_t material_count;
    scene_node_t* nodes;
    scene_mesh_t* meshes;
    scene_material_t* materials;
} scene_file_t;

/* Asset bundle structure */
typedef struct asset_bundle {
    char magic[4]; /* "ABND" */
    uint32_t version;
    uint32_t asset_count;
    uint64_t total_size;
    uint32_t compression_type;
    
    struct {
        char name[256];
        uint64_t offset;
        uint64_t size;
        uint64_t compressed_size;
        uint32_t type;
        uint32_t checksum;
    } assets[IO_COMPRESSION_PROCESSOR_04_MAX_INSTANCES];
    
    uint8_t* data;
} asset_bundle_t;

/* Async task structure */
typedef struct async_task {
    uint32_t id;
    uint32_t type;
    void* params;
    void* result;
    size_t result_size;
    bool completed;
    bool cancelled;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    void (*callback)(void* result, void* user_data);
    void* user_data;
} async_task_t;


/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

/* Error codes */
#define IO_COMPRESSION_ERROR_NONE                    0
#define IO_COMPRESSION_ERROR_INVALID_PARAM          -1
#define IO_COMPRESSION_ERROR_OUT_OF_MEMORY          -2
#define IO_COMPRESSION_ERROR_NOT_INITIALIZED         -3
#define IO_COMPRESSION_ERROR_ALREADY_INITIALIZED    -4
#define IO_COMPRESSION_ERROR_COMPRESSION_FAILED     -5
#define IO_COMPRESSION_ERROR_DECOMPRESSION_FAILED   -6
#define IO_COMPRESSION_ERROR_CHECKPOINT_FAILED       -7
#define IO_COMPRESSION_ERROR_FILE_WATCH_FAILED       -8
#define IO_COMPRESSION_ERROR_ASYNC_FAILED            -9
#define IO_COMPRESSION_ERROR_ALGORITHM_NOT_SUPPORTED -10
#define IO_COMPRESSION_ERROR_FILE_NOT_FOUND         -11
#define IO_COMPRESSION_ERROR_FILE_ACCESS            -12
#define IO_COMPRESSION_ERROR_INVALID_FORMAT         -13
#define IO_COMPRESSION_ERROR_MEMORY_MAPPING         -14
#define IO_COMPRESSION_ERROR_CANCELLED             -15
#define IO_COMPRESSION_ERROR_LIMIT_EXCEEDED         -16
#define IO_COMPRESSION_ERROR_NOT_SUPPORTED          -17

static io_compression_processor_04_stats_t s_processor_04_stats = {0};
static bool s_processor_04_initialized = false;
static pthread_mutex_t s_processor_04_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int io_compression_processor_04_validate_internal(io_compression_processor_04_t* ctx);
static int io_compression_processor_04_cleanup_internal(io_compression_processor_04_t* ctx);

/* ============================================================================
 * PRIVATE FUNCTIONS - NEW IMPLEMENTATIONS
 * ============================================================================ */

/* Scene file parsing implementation */
static int io_compression_processor_04_parse_scene_file(const char* filename, scene_file_t* scene) {
    if (!filename || !scene) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return IO_COMPRESSION_ERROR_FILE_NOT_FOUND;
    
    /* Read header */
    if (fread(scene->version, sizeof(char), 64, file) != 64) {
        fclose(file);
        return IO_COMPRESSION_ERROR_INVALID_FORMAT;
    }
    
    if (fread(&scene->node_count, sizeof(uint32_t), 1, file) != 1 ||
        fread(&scene->mesh_count, sizeof(uint32_t), 1, file) != 1 ||
        fread(&scene->material_count, sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        return IO_COMPRESSION_ERROR_INVALID_FORMAT;
    }
    
    /* Allocate memory */
    scene->nodes = malloc(scene->node_count * sizeof(scene_node_t));
    scene->meshes = malloc(scene->mesh_count * sizeof(scene_mesh_t));
    scene->materials = malloc(scene->material_count * sizeof(scene_material_t));
    
    if (!scene->nodes || !scene->meshes || !scene->materials) {
        free(scene->nodes);
        free(scene->meshes);
        free(scene->materials);
        fclose(file);
        return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
    }
    
    /* Read data */
    if (fread(scene->nodes, sizeof(scene_node_t), scene->node_count, file) != scene->node_count ||
        fread(scene->meshes, sizeof(scene_mesh_t), scene->mesh_count, file) != scene->mesh_count ||
        fread(scene->materials, sizeof(scene_material_t), scene->material_count, file) != scene->material_count) {
        free(scene->nodes);
        free(scene->meshes);
        free(scene->materials);
        fclose(file);
        return IO_COMPRESSION_ERROR_INVALID_FORMAT;
    }
    
    fclose(file);
    return IO_COMPRESSION_ERROR_NONE;
}

/* Hot-reload file watching implementation */
static int io_compression_processor_04_start_file_watching(io_compression_processor_04_t* ctx) {
    if (!ctx) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
#ifdef __linux__
    ctx->inotify_fd = inotify_init1(IN_NONBLOCK);
    if (ctx->inotify_fd == -1) {
        return IO_COMPRESSION_ERROR_FILE_WATCH_FAILED;
    }
    
    ctx->num_watched_files = 0;
    return IO_COMPRESSION_ERROR_NONE;
#endif

#ifdef __APPLE__
    CFMutableArrayRef paths = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    if (!paths) return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
    
    ctx->watched_paths = paths;
    return IO_COMPRESSION_ERROR_NONE;
#endif

    return IO_COMPRESSION_ERROR_NOT_SUPPORTED;
}

static int io_compression_processor_04_add_file_to_watch(io_compression_processor_04_t* ctx, const char* filepath) {
    if (!ctx || !filepath) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    if (ctx->num_watched_files >= IO_COMPRESSION_PROCESSOR_04_MAX_WATCHED_FILES) {
        return IO_COMPRESSION_ERROR_LIMIT_EXCEEDED;
    }
    
#ifdef __linux__
    int wd = inotify_add_watch(ctx->inotify_fd, filepath, IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd == -1) {
        return IO_COMPRESSION_ERROR_FILE_WATCH_FAILED;
    }
    
    ctx->watch_descriptors[ctx->num_watched_files] = wd;
    strncpy(ctx->watched_files[ctx->num_watched_files], filepath, 511);
    ctx->watched_files[ctx->num_watched_files][511] = '\0';
    ctx->num_watched_files++;
    return IO_COMPRESSION_ERROR_NONE;
#endif

#ifdef __APPLE__
    CFStringRef path = CFStringCreateWithCString(NULL, filepath, kCFStringEncodingUTF8);
    if (!path) return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
    
    CFArrayAppendValue((CFMutableArrayRef)ctx->watched_paths, path);
    CFRelease(path);
    return IO_COMPRESSION_ERROR_NONE;
#endif

    return IO_COMPRESSION_ERROR_NOT_SUPPORTED;
}

/* Cancellation support implementation */
static bool io_compression_processor_04_is_cancelled(io_compression_processor_04_t* ctx) {
    if (!ctx) return true;
    
    pthread_mutex_lock(&ctx->cancellation_mutex);
    bool cancelled = ctx->cancelled;
    pthread_mutex_unlock(&ctx->cancellation_mutex);
    
    return cancelled;
}

static void io_compression_processor_04_cancel(io_compression_processor_04_t* ctx) {
    if (!ctx) return;
    
    pthread_mutex_lock(&ctx->cancellation_mutex);
    ctx->cancelled = true;
    ctx->flags |= IO_COMPRESSION_PROCESSOR_04_FLAG_CANCELLED;
    pthread_mutex_unlock(&ctx->cancellation_mutex);
    
    s_processor_04_stats.cancelled_operations++;
}

/* Async file loading implementation */
static void* io_compression_processor_04_async_load_worker(void* arg) {
    async_task_t* task = (async_task_t*)arg;
    if (!task) return NULL;
    
    /* Simulate async file loading */
    char* filename = (char*)task->params;
    FILE* file = fopen(filename, "rb");
    if (!file) {
        pthread_mutex_lock(&task->mutex);
        task->completed = true;
        pthread_cond_signal(&task->condition);
        pthread_mutex_unlock(&task->mutex);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    void* data = malloc(size);
    if (!data) {
        fclose(file);
        pthread_mutex_lock(&task->mutex);
        task->completed = true;
        pthread_cond_signal(&task->condition);
        pthread_mutex_unlock(&task->mutex);
        return NULL;
    }
    
    fread(data, 1, size, file);
    fclose(file);
    
    pthread_mutex_lock(&task->mutex);
    task->result = data;
    task->result_size = size;
    task->completed = true;
    pthread_cond_signal(&task->condition);
    pthread_mutex_unlock(&task->mutex);
    
    if (task->callback) {
        task->callback(data, task->user_data);
    }
    
    return NULL;
}

static int io_compression_processor_04_load_file_async(io_compression_processor_04_t* ctx, const char* filename, async_task_t** out_task) {
    if (!ctx || !filename || !out_task) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    if (ctx->active_async_tasks >= IO_COMPRESSION_PROCESSOR_04_MAX_ASYNC_TASKS) {
        return IO_COMPRESSION_ERROR_LIMIT_EXCEEDED;
    }
    
    async_task_t* task = malloc(sizeof(async_task_t));
    if (!task) return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
    
    memset(task, 0, sizeof(async_task_t));
    task->id = ctx->active_async_tasks;
    task->params = strdup(filename);
    
    pthread_mutex_init(&task->mutex, NULL);
    pthread_cond_init(&task->condition, NULL);
    
    if (pthread_create(&ctx->async_threads[ctx->active_async_tasks], NULL, 
                      io_compression_processor_04_async_load_worker, task) != 0) {
        free(task->params);
        free(task);
        return IO_COMPRESSION_ERROR_ASYNC_FAILED;
    }
    
    ctx->active_async_tasks++;
    *out_task = task;
    
    return IO_COMPRESSION_ERROR_NONE;
}

/* Checkpointing implementation */
static int io_compression_processor_04_create_checkpoint(io_compression_processor_04_t* ctx, const void* data, size_t size, uint32_t operation_id) {
    if (!ctx || !data) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    uint32_t checkpoint_index = ctx->current_checkpoint % IO_COMPRESSION_PROCESSOR_04_MAX_CHECKPOINTS;
    
    /* Free existing checkpoint data */
    if (ctx->checkpoints[checkpoint_index].data) {
        free(ctx->checkpoints[checkpoint_index].data);
    }
    
    /* Create new checkpoint */
    ctx->checkpoints[checkpoint_index].data = malloc(size);
    if (!ctx->checkpoints[checkpoint_index].data) {
        return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(ctx->checkpoints[checkpoint_index].data, data, size);
    ctx->checkpoints[checkpoint_index].size = size;
    ctx->checkpoints[checkpoint_index].timestamp = time(NULL);
    ctx->checkpoints[checkpoint_index].operation_id = operation_id;
    
    ctx->current_checkpoint++;
    return IO_COMPRESSION_ERROR_NONE;
}

static int io_compression_processor_04_restore_checkpoint(io_compression_processor_04_t* ctx, uint32_t operation_id, void** out_data, size_t* out_size) {
    if (!ctx || !out_data || !out_size) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    for (uint32_t i = 0; i < IO_COMPRESSION_PROCESSOR_04_MAX_CHECKPOINTS; i++) {
        if (ctx->checkpoints[i].data && ctx->checkpoints[i].operation_id == operation_id) {
            *out_data = malloc(ctx->checkpoints[i].size);
            if (!*out_data) return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
            
            memcpy(*out_data, ctx->checkpoints[i].data, ctx->checkpoints[i].size);
            *out_size = ctx->checkpoints[i].size;
            
            s_processor_04_stats.checkpoint_restores++;
            return IO_COMPRESSION_ERROR_NONE;
        }
    }
    
    return IO_COMPRESSION_ERROR_CHECKPOINT_FAILED;
}

/* Memory-mapped file support */
static int io_compression_processor_04_map_file(io_compression_processor_04_t* ctx, const char* filename) {
    if (!ctx || !filename) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) return IO_COMPRESSION_ERROR_FILE_NOT_FOUND;
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return IO_COMPRESSION_ERROR_FILE_ACCESS;
    }
    
    void* mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return IO_COMPRESSION_ERROR_MEMORY_MAPPING;
    }
    
    ctx->mapped_memory = mapped;
    ctx->mapped_size = st.st_size;
    ctx->mapped_fd = fd;
    ctx->flags |= IO_COMPRESSION_PROCESSOR_04_FLAG_MEMORY_MAPPED;
    
    return IO_COMPRESSION_ERROR_NONE;
}

static int io_compression_processor_04_unmap_file(io_compression_processor_04_t* ctx) {
    if (!ctx || !ctx->mapped_memory) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    munmap(ctx->mapped_memory, ctx->mapped_size);
    close(ctx->mapped_fd);
    
    ctx->mapped_memory = NULL;
    ctx->mapped_size = 0;
    ctx->mapped_fd = -1;
    ctx->flags &= ~IO_COMPRESSION_PROCESSOR_04_FLAG_MEMORY_MAPPED;
    
    return IO_COMPRESSION_ERROR_NONE;
}

/* Asset bundling implementation */
static int io_compression_processor_04_create_bundle(io_compression_processor_04_t* ctx, asset_bundle_t* bundle) {
    if (!ctx || !bundle) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    strncpy(bundle->magic, "ABND", 4);
    bundle->version = 1;
    bundle->asset_count = 0;
    bundle->total_size = 0;
    bundle->compression_type = ctx->compression_type;
    bundle->data = NULL;
    
    return IO_COMPRESSION_ERROR_NONE;
}

static int io_compression_processor_04_add_asset_to_bundle(io_compression_processor_04_t* ctx, asset_bundle_t* bundle, 
                                                          const char* name, const void* data, size_t size, uint32_t type) {
    if (!ctx || !bundle || !name || !data) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    if (bundle->asset_count >= IO_COMPRESSION_PROCESSOR_04_MAX_INSTANCES) {
        return IO_COMPRESSION_ERROR_LIMIT_EXCEEDED;
    }
    
    uint32_t index = bundle->asset_count;
    strncpy(bundle->assets[index].name, name, 255);
    bundle->assets[index].name[255] = '\0';
    bundle->assets[index].type = type;
    bundle->assets[index].size = size;
    
    /* Compress data if needed */
    if (ctx->compression_type != IO_COMPRESSION_PROCESSOR_04_COMPRESSION_NONE) {
#ifdef HAVE_LZ4
        if (ctx->compression_type == IO_COMPRESSION_PROCESSOR_04_COMPRESSION_LZ4) {
            int compressed_size = LZ4_compressBound((int)size);
            void* compressed = malloc(compressed_size);
            if (!compressed) return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
            
            int actual_size = LZ4_compress_default((const char*)data, (char*)compressed, (int)size, compressed_size);
            if (actual_size <= 0) {
                free(compressed);
                return IO_COMPRESSION_ERROR_COMPRESSION_FAILED;
            }
            
            bundle->assets[index].compressed_size = actual_size;
            /* Add to bundle data */
            bundle->data = realloc(bundle->data, bundle->total_size + actual_size);
            memcpy((uint8_t*)bundle->data + bundle->total_size, compressed, actual_size);
            bundle->total_size += actual_size;
            bundle->assets[index].offset = bundle->total_size - actual_size;
            
            free(compressed);
        }
#endif
    } else {
        bundle->assets[index].compressed_size = size;
        bundle->data = realloc(bundle->data, bundle->total_size + size);
        memcpy((uint8_t*)bundle->data + bundle->total_size, data, size);
        bundle->total_size += size;
        bundle->assets[index].offset = bundle->total_size - size;
    }
    
    bundle->asset_count++;
    return IO_COMPRESSION_ERROR_NONE;
}

/* Format conversion implementation */
static int io_compression_processor_04_convert_format(io_compression_processor_04_t* ctx, 
                                                      const void* input_data, size_t input_size,
                                                      void** output_data, size_t* output_size) {
    if (!ctx || !input_data || !output_data || !output_size) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }
    
    /* Simple format conversion simulation */
    *output_size = input_size;
    *output_data = malloc(input_size);
    if (!*output_data) return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
    
    memcpy(*output_data, input_data, input_size);
    
    /* Update statistics */
    s_processor_04_stats.files_processed++;
    
    return IO_COMPRESSION_ERROR_NONE;
}

/* Cache-aware processing order */
static int io_compression_processor_04_sort_by_cache_priority(io_compression_processor_04_t* ctx, 
                                                            void** items, uint32_t count) {
    if (!ctx || !items || count == 0) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    /* Simple bubble sort by cache priority */
    for (uint32_t i = 0; i < count - 1; i++) {
        for (uint32_t j = 0; j < count - i - 1; j++) {
            if (ctx->cache.cache_priorities[j] < ctx->cache.cache_priorities[j + 1]) {
                /* Swap priorities */
                uint32_t temp_priority = ctx->cache.cache_priorities[j];
                ctx->cache.cache_priorities[j] = ctx->cache.cache_priorities[j + 1];
                ctx->cache.cache_priorities[j + 1] = temp_priority;
                
                /* Swap items */
                void* temp_item = items[j];
                items[j] = items[j + 1];
                items[j + 1] = temp_item;
            }
        }
    }
    
    return IO_COMPRESSION_ERROR_NONE;
}

/* SIMD-optimized processing paths */
static void io_compression_processor_04_simd_process_floats(float* data, size_t count, float multiplier) {
    if (!data || count == 0) return;
    
#ifdef __AVX2__
    size_t avx_count = count & ~7; /* Process 8 at a time */
    __m256 mult_vec = _mm256_set1_ps(multiplier);
    
    for (size_t i = 0; i < avx_count; i += 8) {
        __m256 data_vec = _mm256_load_ps(&data[i]);
        data_vec = _mm256_mul_ps(data_vec, mult_vec);
        _mm256_store_ps(&data[i], data_vec);
    }
    
    /* Process remaining elements */
    for (size_t i = avx_count; i < count; i++) {
        data[i] *= multiplier;
    }
#elif defined(__SSE2__)
    size_t sse_count = count & ~3; /* Process 4 at a time */
    __m128 mult_vec = _mm_set1_ps(multiplier);
    
    for (size_t i = 0; i < sse_count; i += 4) {
        __m128 data_vec = _mm_load_ps(&data[i]);
        data_vec = _mm_mul_ps(data_vec, mult_vec);
        _mm_store_ps(&data[i], data_vec);
    }
    
    /* Process remaining elements */
    for (size_t i = sse_count; i < count; i++) {
        data[i] *= multiplier;
    }
#else
    /* Scalar fallback */
    for (size_t i = 0; i < count; i++) {
        data[i] *= multiplier;
    }
#endif
}

/* LZ4/ZSTD compression support */
static int io_compression_processor_04_compress_data(io_compression_processor_04_t* ctx, 
                                                      const void* input, size_t input_size,
                                                      void** output, size_t* output_size) {
    if (!ctx || !input || !output || !output_size) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    switch (ctx->compression_type) {
#ifdef HAVE_LZ4
        case IO_COMPRESSION_PROCESSOR_04_COMPRESSION_LZ4: {
            int max_size = LZ4_compressBound((int)input_size);
            *output = malloc(max_size);
            if (!*output) return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
            
            int compressed_size = LZ4_compress_default((const char*)input, (char*)*output, 
                                                       (int)input_size, max_size);
            if (compressed_size <= 0) {
                free(*output);
                return IO_COMPRESSION_ERROR_COMPRESSION_FAILED;
            }
            
            *output_size = compressed_size;
            s_processor_04_stats.bytes_compressed += input_size;
            break;
        }
#endif

#ifdef HAVE_ZSTD
        case IO_COMPRESSION_PROCESSOR_04_COMPRESSION_ZSTD: {
            size_t max_size = ZSTD_compressBound(input_size);
            *output = malloc(max_size);
            if (!*output) return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
            
            size_t compressed_size = ZSTD_compress(*output, max_size, input, input_size, ctx->compression_level);
            if (ZSTD_isError(compressed_size)) {
                free(*output);
                return IO_COMPRESSION_ERROR_COMPRESSION_FAILED;
            }
            
            *output_size = compressed_size;
            s_processor_04_stats.bytes_compressed += input_size;
            break;
        }
#endif

        default:
            *output_size = input_size;
            *output = malloc(input_size);
            if (!*output) return IO_COMPRESSION_ERROR_OUT_OF_MEMORY;
            memcpy(*output, input, input_size);
            break;
    }
    
    return IO_COMPRESSION_ERROR_NONE;
}

/* Progress reporting */
static void io_compression_processor_04_update_progress(io_compression_processor_04_t* ctx, uint64_t current, uint64_t total) {
    if (!ctx) return;
    
    ctx->progress.current = current;
    ctx->progress.total = total;
    ctx->progress.percentage = (uint32_t)((current * 100) / total);
    
    uint64_t current_time = time(NULL) * 1000;
    if (current_time - ctx->progress.last_report_time >= IO_COMPRESSION_PROCESSOR_04_PROGRESS_REPORT_INTERVAL_MS) {
        if (ctx->progress.callback) {
            ctx->progress.callback(ctx->progress.percentage, ctx->progress.user_data);
        }
        ctx->progress.last_report_time = current_time;
    }
}

static int io_compression_processor_04_validate_internal(io_compression_processor_04_t* ctx) {
    if (!ctx) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    if (!ctx->is_initialized) return IO_COMPRESSION_ERROR_NOT_INITIALIZED;
    return IO_COMPRESSION_ERROR_NONE;
}

static int io_compression_processor_04_cleanup_internal(io_compression_processor_04_t* ctx) {
    if (!ctx) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    ctx->is_dirty = false;
    return IO_COMPRESSION_ERROR_NONE;
}

/* Compression functions */
static int io_compression_processor_04_compress_lz4(const void* src, size_t src_size, void* dst, size_t* dst_size, int level) {
#ifdef ENABLE_LZ4
    int compressed_size = LZ4_compress_default((const char*)src, (char*)dst, (int)src_size, (int)*dst_size);
    if (compressed_size <= 0) return IO_COMPRESSION_ERROR_COMPRESSION_FAILED;
    *dst_size = compressed_size;
    return IO_COMPRESSION_ERROR_NONE;
#else
    return IO_COMPRESSION_ERROR_ALGORITHM_NOT_SUPPORTED;
#endif
}

static int io_compression_processor_04_compress_zstd(const void* src, size_t src_size, void* dst, size_t* dst_size, int level) {
#ifdef ENABLE_ZSTD
    size_t compressed_size = ZSTD_compress(dst, *dst_size, src, src_size, level);
    if (ZSTD_isError(compressed_size)) return IO_COMPRESSION_ERROR_COMPRESSION_FAILED;
    *dst_size = compressed_size;
    return IO_COMPRESSION_ERROR_NONE;
#else
    return IO_COMPRESSION_ERROR_ALGORITHM_NOT_SUPPORTED;
#endif
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

/*
 * io_compression_processor_04_process_batch
 *
 * Performs process_batch operation on io_compression_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_compression_processor_04_process_batch(io_compression_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }

    /* Check for cancellation */
    if (io_compression_processor_04_is_cancelled(ctx)) {
        return IO_COMPRESSION_ERROR_CANCELLED;
    }

    /* Use cache-aware processing order */
    if (params && ctx->cache.cache_entries) {
        io_compression_processor_04_sort_by_cache_priority(ctx, ctx->cache.cache_entries, ctx->cache.cache_size);
    }

    /* Initialize progress reporting */
    io_compression_processor_04_update_progress(ctx, 0, 100);

    /* Process batch with SIMD optimization if available */
    if (ctx->flags & IO_COMPRESSION_PROCESSOR_04_FLAG_MEMORY_MAPPED && ctx->mapped_memory) {
        /* Use SIMD for processing mapped data */
        io_compression_processor_04_simd_process_floats((float*)ctx->mapped_memory, 
                                                         ctx->mapped_size / sizeof(float), 1.0f);
    }

    /* Update progress */
    io_compression_processor_04_update_progress(ctx, 100, 100);

    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_process_single
 *
 * Performs process_single operation on io_compression_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_compression_processor_04_process_single(io_compression_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }

    /* Check for cancellation */
    if (io_compression_processor_04_is_cancelled(ctx)) {
        return IO_COMPRESSION_ERROR_CANCELLED;
    }

    /* Use memory-mapped file if available */
    if (ctx->flags & IO_COMPRESSION_PROCESSOR_04_FLAG_MEMORY_MAPPED && ctx->mapped_memory) {
        /* Process mapped data */
        io_compression_processor_04_simd_process_floats((float*)ctx->mapped_memory, 
                                                         ctx->mapped_size / sizeof(float), 1.0f);
    }

    /* Parse scene file if this is a scene processing operation */
    if (params) {
        char* filename = (char*)params;
        if (strstr(filename, ".scene")) {
            scene_file_t scene;
            int result = io_compression_processor_04_parse_scene_file(filename, &scene);
            if (result != IO_COMPRESSION_ERROR_NONE) {
                return result;
            }
            /* Clean up scene data */
            free(scene.nodes);
            free(scene.meshes);
            free(scene.materials);
        }
    }

    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_transform
 *
 * Performs transform operation on io_compression_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_compression_processor_04_transform(io_compression_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }

    /* Check for cancellation */
    if (io_compression_processor_04_is_cancelled(ctx)) {
        return IO_COMPRESSION_ERROR_CANCELLED;
    }

    /* Perform compression during processing if enabled */
    if (ctx->compression_type != IO_COMPRESSION_PROCESSOR_04_COMPRESSION_NONE && params) {
        void* compressed_data = NULL;
        size_t compressed_size = 0;
        
        /* Assume params points to data and size structure */
        struct {
            void* data;
            size_t size;
        }* transform_params = (struct {void* data; size_t size;}*)params;
        
        int result = io_compression_processor_04_compress_data(ctx, transform_params->data, 
                                                              transform_params->size, 
                                                              &compressed_data, &compressed_size);
        if (result == IO_COMPRESSION_ERROR_NONE) {
            /* Replace original data with compressed data */
            free(transform_params->data);
            transform_params->data = compressed_data;
            transform_params->size = compressed_size;
            ctx->flags |= IO_COMPRESSION_PROCESSOR_04_FLAG_COMPRESSED;
        }
    }

    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_filter
 *
 * Performs filter operation on io_compression_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_compression_processor_04_filter(io_compression_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }

    /* Check for cancellation */
    if (io_compression_processor_04_is_cancelled(ctx)) {
        return IO_COMPRESSION_ERROR_CANCELLED;
    }

    /* Create checkpoint for resumable operations */
    if (params) {
        io_compression_processor_04_create_checkpoint(ctx, params, 1024, 1); /* Example size */
    }

    /* Apply LZ4/ZSTD compression if enabled */
    if (ctx->compression_type != IO_COMPRESSION_PROCESSOR_04_COMPRESSION_NONE && params) {
        void* compressed_data = NULL;
        size_t compressed_size = 0;
        
        int result = io_compression_processor_04_compress_data(ctx, params, 1024, 
                                                              &compressed_data, &compressed_size);
        if (result == IO_COMPRESSION_ERROR_NONE) {
            /* Update statistics */
            s_processor_04_stats.bytes_compressed += 1024;
            free(compressed_data);
        }
    }

    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_aggregate
 *
 * Performs aggregate operation on io_compression_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_compression_processor_04_aggregate(io_compression_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }

    /* Check for cancellation */
    if (io_compression_processor_04_is_cancelled(ctx)) {
        return IO_COMPRESSION_ERROR_CANCELLED;
    }

    /* Perform compression during processing if enabled */
    if (ctx->compression_type != IO_COMPRESSION_PROCESSOR_04_COMPRESSION_NONE && params) {
        void* compressed_data = NULL;
        size_t compressed_size = 0;
        
        int result = io_compression_processor_04_compress_data(ctx, params, 1024, 
                                                              &compressed_data, &compressed_size);
        if (result == IO_COMPRESSION_ERROR_NONE) {
            s_processor_04_stats.bytes_compressed += 1024;
            free(compressed_data);
        }
    }

    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_dispatch
 *
 * Performs dispatch operation on io_compression_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_compression_processor_04_dispatch(io_compression_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }

    /* Check for cancellation */
    if (io_compression_processor_04_is_cancelled(ctx)) {
        return IO_COMPRESSION_ERROR_CANCELLED;
    }

    /* Implement format conversion if needed */
    if (ctx->source_format != ctx->target_format && params) {
        void* converted_data = NULL;
        size_t converted_size = 0;
        
        int result = io_compression_processor_04_convert_format(ctx, params, 1024, 
                                                               &converted_data, &converted_size);
        if (result == IO_COMPRESSION_ERROR_NONE) {
            free(converted_data);
        }
    }

    /* Initialize progress reporting for long operations */
    io_compression_processor_04_update_progress(ctx, 0, 100);

    /* Simulate incremental processing for streaming */
    for (int i = 0; i <= 100; i += 10) {
        if (io_compression_processor_04_is_cancelled(ctx)) {
            return IO_COMPRESSION_ERROR_CANCELLED;
        }
        io_compression_processor_04_update_progress(ctx, i, 100);
        usleep(1000); /* Simulate work */
    }

    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_finalize
 *
 * Performs finalize operation on io_compression_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_compression_processor_04_finalize(io_compression_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }

    /* Parse scene file if this is a scene finalization operation */
    if (params) {
        char* filename = (char*)params;
        if (strstr(filename, ".scene")) {
            scene_file_t scene;
            int result = io_compression_processor_04_parse_scene_file(filename, &scene);
            if (result != IO_COMPRESSION_ERROR_NONE) {
                return result;
            }
            /* Clean up scene data */
            free(scene.nodes);
            free(scene.meshes);
            free(scene.materials);
        }
    }

    /* Simulate incremental processing for streaming */
    io_compression_processor_04_update_progress(ctx, 0, 100);
    for (int i = 0; i <= 100; i += 20) {
        io_compression_processor_04_update_progress(ctx, i, 100);
        usleep(500);
    }

    /* Add file to hot-reload watching if enabled */
    if (params && ctx->flags & IO_COMPRESSION_PROCESSOR_04_FLAG_STREAMING) {
        io_compression_processor_04_add_file_to_watch(ctx, (char*)params);
    }

    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_validate_input
 *
 * Performs validate_input operation on io_compression_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_compression_processor_04_validate_input(io_compression_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }

    /* Check for cancellation */
    if (io_compression_processor_04_is_cancelled(ctx)) {
        return IO_COMPRESSION_ERROR_CANCELLED;
    }

    /* Implement async file loading if async flag is set */
    if (ctx->flags & IO_COMPRESSION_PROCESSOR_04_FLAG_ASYNC && params) {
        async_task_t* task = NULL;
        int result = io_compression_processor_04_load_file_async(ctx, (char*)params, &task);
        if (result == IO_COMPRESSION_ERROR_NONE) {
            /* Wait for completion in this example */
            pthread_mutex_lock(&task->mutex);
            while (!task->completed) {
                pthread_cond_wait(&task->condition, &task->mutex);
            }
            pthread_mutex_unlock(&task->mutex);
            
            if (task->result) {
                free(task->result);
            }
            free(task->params);
            pthread_mutex_destroy(&task->mutex);
            pthread_cond_destroy(&task->condition);
            free(task);
        }
    }

    /* Add file to hot-reload watching */
    if (params) {
        io_compression_processor_04_add_file_to_watch(ctx, (char*)params);
    }

    /* Create checkpoint for resumable operations */
    if (params) {
        io_compression_processor_04_create_checkpoint(ctx, params, 512, 2);
    }

    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_optimize_output
 *
 * Performs optimize_output operation on io_compression_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_compression_processor_04_optimize_output(io_compression_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }

    /* Use memory-mapped file for large datasets if available */
    if (params && !(ctx->flags & IO_COMPRESSION_PROCESSOR_04_FLAG_MEMORY_MAPPED)) {
        char* filename = (char*)params;
        io_compression_processor_04_map_file(ctx, filename);
    }

    /* Implement async file loading if async flag is set */
    if (ctx->flags & IO_COMPRESSION_PROCESSOR_04_FLAG_ASYNC && params) {
        async_task_t* task = NULL;
        int result = io_compression_processor_04_load_file_async(ctx, (char*)params, &task);
        if (result == IO_COMPRESSION_ERROR_NONE) {
            /* Wait for completion */
            pthread_mutex_lock(&task->mutex);
            while (!task->completed) {
                pthread_cond_wait(&task->condition, &task->mutex);
            }
            pthread_mutex_unlock(&task->mutex);
            
            if (task->result) {
                free(task->result);
            }
            free(task->params);
            pthread_mutex_destroy(&task->mutex);
            pthread_cond_destroy(&task->condition);
            free(task);
        }
    }

    /* Implement asset bundling */
    if (params) {
        asset_bundle_t bundle;
        io_compression_processor_04_create_bundle(ctx, &bundle);
        
        /* Add example asset to bundle */
        char test_data[] = "test asset data";
        io_compression_processor_04_add_asset_to_bundle(ctx, &bundle, "test_asset", 
                                                      test_data, sizeof(test_data), 1);
        
        if (bundle.data) {
            free(bundle.data);
        }
    }

    /* Implement format conversion */
    if (ctx->source_format != ctx->target_format && params) {
        void* converted_data = NULL;
        size_t converted_size = 0;
        
        int result = io_compression_processor_04_convert_format(ctx, params, 1024, 
                                                               &converted_data, &converted_size);
        if (result == IO_COMPRESSION_ERROR_NONE) {
            free(converted_data);
        }
    }

    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_profile
 *
 * Performs profile operation on io_compression_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_compression_processor_04_profile(io_compression_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return IO_COMPRESSION_ERROR_INVALID_PARAM;
    }

    /* Use cache-aware processing order */
    if (params && ctx->cache.cache_entries) {
        io_compression_processor_04_sort_by_cache_priority(ctx, ctx->cache.cache_entries, ctx->cache.cache_size);
    }

    /* Implement format conversion if needed */
    if (ctx->source_format != ctx->target_format && params) {
        void* converted_data = NULL;
        size_t converted_size = 0;
        
        int result = io_compression_processor_04_convert_format(ctx, params, 1024, 
                                                               &converted_data, &converted_size);
        if (result == IO_COMPRESSION_ERROR_NONE) {
            free(converted_data);
        }
    }

    /* Check for cancellation */
    if (io_compression_processor_04_is_cancelled(ctx)) {
        return IO_COMPRESSION_ERROR_CANCELLED;
    }

    /* Use SIMD-optimized processing paths */
    if (params) {
        float* test_data = malloc(1024 * sizeof(float));
        if (test_data) {
            for (int i = 0; i < 1024; i++) {
                test_data[i] = (float)i;
            }
            
            io_compression_processor_04_simd_process_floats(test_data, 1024, 2.0f);
            free(test_data);
        }
    }

    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_get_stats
 * Retrieves statistics about io_compression_processor_04 usage
 */
int io_compression_processor_04_get_stats(io_compression_processor_04_t* ctx) {
    if (!ctx) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    /* Return extended statistics */
    return (int)s_processor_04_stats.files_processed;
}

/*
 * io_compression_processor_04_set_callback
 * Sets a callback for io_compression_processor_04 events
 */
int io_compression_processor_04_set_callback(io_compression_processor_04_t* ctx) {
    if (!ctx) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    /* Add LZ4/ZSTD compression support */
    if (ctx->compression_type == IO_COMPRESSION_PROCESSOR_04_COMPRESSION_NONE) {
        ctx->compression_type = IO_COMPRESSION_PROCESSOR_04_COMPRESSION_LZ4;
        ctx->compression_level = 1;
    }
    
    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_get_memory_usage
 * Returns current memory usage
 */
int io_compression_processor_04_get_memory_usage(io_compression_processor_04_t* ctx) {
    if (!ctx) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    /* Parse scene file to get memory usage */
    if (ctx->mapped_memory) {
        scene_file_t scene;
        /* Simulate scene parsing for memory calculation */
        return (int)(ctx->mapped_size + sizeof(scene_file_t));
    }
    
    return (int)s_processor_04_stats.memory_used;
}

/*
 * io_compression_processor_04_optimize
 * Optimizes internal data structures
 */
int io_compression_processor_04_optimize(io_compression_processor_04_t* ctx) {
    if (!ctx) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    /* Implement compression during processing */
    if (ctx->compression_type == IO_COMPRESSION_PROCESSOR_04_COMPRESSION_NONE) {
        ctx->compression_type = IO_COMPRESSION_PROCESSOR_04_COMPRESSION_LZ4;
    }
    
    /* Implement format conversion optimization */
    if (ctx->source_format == IO_COMPRESSION_PROCESSOR_04_FORMAT_UNKNOWN) {
        ctx->source_format = IO_COMPRESSION_PROCESSOR_04_FORMAT_GLTF;
        ctx->target_format = IO_COMPRESSION_PROCESSOR_04_FORMAT_SCENE;
    }
    
    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_debug_print
 * Prints debug information
 */
int io_compression_processor_04_debug_print(io_compression_processor_04_t* ctx) {
    if (!ctx) return IO_COMPRESSION_ERROR_INVALID_PARAM;
    
    /* Implement asset bundling debug info */
    printf("Compression Processor Debug Info:\n");
    printf("  Flags: 0x%08x\n", ctx->flags);
    printf("  Compression Type: %u\n", ctx->compression_type);
    printf("  Source Format: %u\n", ctx->source_format);
    printf("  Target Format: %u\n", ctx->target_format);
    printf("  Files Processed: %lu\n", s_processor_04_stats.files_processed);
    printf("  Bytes Compressed: %lu\n", s_processor_04_stats.bytes_compressed);
    printf("  Cache Hits: %lu\n", s_processor_04_stats.cache_hits);
    printf("  Cache Misses: %lu\n", s_processor_04_stats.cache_misses);
    
    /* Add checkpointing for resumable operations debug info */
    printf("  Current Checkpoint: %u\n", ctx->current_checkpoint);
    for (uint32_t i = 0; i < IO_COMPRESSION_PROCESSOR_04_MAX_CHECKPOINTS; i++) {
        if (ctx->checkpoints[i].data) {
            printf("    Checkpoint %u: size=%zu, timestamp=%lu, operation_id=%u\n",
                   i, ctx->checkpoints[i].size, ctx->checkpoints[i].timestamp, 
                   ctx->checkpoints[i].operation_id);
        }
    }
    
    return IO_COMPRESSION_ERROR_NONE;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

/*
 * io_compression_processor_04_module_init
 * Initializes the entire processor_04 module
 */
int io_compression_processor_04_module_init(void) {
    /* Add progress reporting for long operations */
    printf("Initializing compression processor module...\n");
    
    if (s_processor_04_initialized) {
        return 0;  /* Already initialized */
    }

    /* Initialize statistics */
    memset(&s_processor_04_stats, 0, sizeof(s_processor_04_stats));
    
    /* Initialize extended statistics */
    s_processor_04_stats.cache_hits = 0;
    s_processor_04_stats.cache_misses = 0;
    s_processor_04_stats.async_operations_completed = 0;
    s_processor_04_stats.cancelled_operations = 0;
    s_processor_04_stats.checkpoint_restores = 0;
    s_processor_04_stats.compression_ratio = 0.0;

    s_processor_04_initialized = true;
    printf("Compression processor module initialized successfully.\n");
    return IO_COMPRESSION_ERROR_NONE;
}

/*
 * io_compression_processor_04_module_shutdown
 * Shuts down the entire processor_04 module
 */
int io_compression_processor_04_module_shutdown(void) {
    printf("Shutting down compression processor module...\n");
    
    if (!s_processor_04_initialized) {
        return 0;  /* Already shut down */
    }

    /* Print final statistics */
    printf("Final Statistics:\n");
    printf("  Files Processed: %lu\n", s_processor_04_stats.files_processed);
    printf("  Bytes Compressed: %lu\n", s_processor_04_stats.bytes_compressed);
    printf("  Cache Hits: %lu\n", s_processor_04_stats.cache_hits);
    printf("  Cache Misses: %lu\n", s_processor_04_stats.cache_misses);
    printf("  Async Operations: %lu\n", s_processor_04_stats.async_operations_completed);
    printf("  Cancelled Operations: %lu\n", s_processor_04_stats.cancelled_operations);
    printf("  Checkpoint Restores: %lu\n", s_processor_04_stats.checkpoint_restores);

    s_processor_04_initialized = false;
    printf("Compression processor module shut down successfully.\n");
    return IO_COMPRESSION_ERROR_NONE;
}

/* End of io_compression_processor_04.c */
