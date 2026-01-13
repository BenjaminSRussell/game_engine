/*
 * io_bundling_manager_01.c
 *
 * I/O and asset streaming - Bundling Subsystem
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * This file implements manager functionality for the bundling module
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
#include <stdatomic.h>
#include <threads.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>

#include "assets/io/bundling/manager_01.h"
#include "include/core/types.h"
#include "include/core/memory.h"
#include "core/logger.h"
#include "core/memory.h"

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define IO_BUNDLING_MANAGER_01_VERSION_MAJOR 1
#define IO_BUNDLING_MANAGER_01_VERSION_MINOR 0
#define IO_BUNDLING_MANAGER_01_VERSION_PATCH 0

#define IO_BUNDLING_MANAGER_01_MAX_INSTANCES 4096
#define IO_BUNDLING_MANAGER_01_DEFAULT_CAPACITY 256
#define IO_BUNDLING_MANAGER_01_ALIGNMENT 16

#define IO_BUNDLING_MANAGER_01_FLAG_NONE          0x00000000
#define IO_BUNDLING_MANAGER_01_FLAG_INITIALIZED   0x00000001
#define IO_BUNDLING_MANAGER_01_FLAG_DIRTY         0x00000002
#define IO_BUNDLING_MANAGER_01_FLAG_GPU_RESIDENT  0x00000004
#define IO_BUNDLING_MANAGER_01_FLAG_STREAMING     0x00000008
#define IO_BUNDLING_MANAGER_01_FLAG_ASYNC_LOADING 0x00000010
#define IO_BUNDLING_MANAGER_01_FLAG_HOT_RELOAD    0x00000020
#define IO_BUNDLING_MANAGER_01_FLAG_THREADED      0x00000040

/* Error codes */
#define IO_BUNDLING_MANAGER_01_ERROR_NONE                    0
#define IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM          -1
#define IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED        -2
#define IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY          -3
#define IO_BUNDLING_MANAGER_01_ERROR_FILE_NOT_FOUND         -4
#define IO_BUNDLING_MANAGER_01_ERROR_FILE_READ_FAILED       -5
#define IO_BUNDLING_MANAGER_01_ERROR_FILE_WRITE_FAILED      -6
#define IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED           -7
#define IO_BUNDLING_MANAGER_01_ERROR_FORMAT_UNSUPPORTED     -8
#define IO_BUNDLING_MANAGER_01_ERROR_SERIALIZATION_FAILED   -9
#define IO_BUNDLING_MANAGER_01_ERROR_DESERIALIZATION_FAILED -10
#define IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED       -11
#define IO_BUNDLING_MANAGER_01_ERROR_ASYNC_OPERATION_FAILED -12
#define IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED -13
#define IO_BUNDLING_MANAGER_01_ERROR_ASSET_NOT_FOUND        -14
#define IO_BUNDLING_MANAGER_01_ERROR_BUNDLE_CORRUPTED       -15
#define IO_BUNDLING_MANAGER_01_ERROR_HOT_RELOAD_FAILED      -16
#define IO_BUNDLING_MANAGER_01_ERROR_CONVERSION_FAILED      -17
#define IO_BUNDLING_MANAGER_01_ERROR_POOL_EXHAUSTED         -18
#define IO_BUNDLING_MANAGER_01_ERROR_TELEMETRY_FAILED       -19

/* Memory and performance constants */
#define IO_BUNDLING_MANAGER_01_DEFAULT_MEMORY_BUDGET       (512 * 1024 * 1024)  /* 512MB */
#define IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS        64
#define IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES            1024
#define IO_BUNDLING_MANAGER_01_POOL_SIZE                    256
#define IO_BUNDLING_MANAGER_01_BUNDLE_VERSION               1
#define IO_BUNDLING_MANAGER_01_MAX_SCENE_DEPTH              32
#define IO_BUNDLING_MANAGER_01_MAX_BATCH_SIZE               32

/* Scene file parsing constants */
#define IO_BUNDLING_MANAGER_01_MAX_SCENE_FILE_SIZE          (128 * 1024 * 1024)  /* 128MB */
#define IO_BUNDLING_MANAGER_01_MAX_SCENE_OBJECTS            10000
#define IO_BUNDLING_MANAGER_01_MAX_SCENE_NODES              5000
#define IO_BUNDLING_MANAGER_01_SCENE_VERSION                1

/* Async file loading constants */
#define IO_BUNDLING_MANAGER_01_MAX_ASYNC_QUEUE_SIZE         256
#define IO_BUNDLING_MANAGER_01_ASYNC_THREAD_COUNT           4
#define IO_BUNDLING_MANAGER_01_ASYNC_BUFFER_SIZE            (64 * 1024)  /* 64KB */

/* Multi-threaded batch processing constants */
#define IO_BUNDLING_MANAGER_01_MAX_WORKER_THREADS           8
#define IO_BUNDLING_MANAGER_01_BATCH_QUEUE_SIZE             128
#define IO_BUNDLING_MANAGER_01_WORKER_STACK_SIZE            (2 * 1024 * 1024)  /* 2MB */

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */


/*
 * IO_BUNDLING_MANAGER_01 - Core data structure
 * Manages state and resources for manager_01 operations
 */
typedef struct io_bundling_manager_01 {
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
    
    /* Memory management */
    size_t memory_budget;
    size_t memory_used;
    atomic_bool memory_tracking_enabled;
    
    /* Thread safety */
    atomic_int initialization_state;
    thrd_t worker_thread;
    mtx_t access_mutex;
    cnd_t condition_var;
    
    /* Async operations */
    atomic_int async_operation_count;
    void* async_operations[IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS];
    
    /* Hot reload */
    bool hot_reload_enabled;
    void* file_watcher;
    char watched_files[IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES][256];
    atomic_int watched_file_count;
    
    /* Resource pooling */
    void* resource_pool;
    atomic_int pool_available_count;
    
    /* Telemetry */
    uint64_t operation_count;
    double total_operation_time_ms;
    uint64_t cache_hits;
    uint64_t cache_misses;
    
} io_bundling_manager_01_t;

/* Error handling structure */
typedef struct io_bundling_manager_01_error {
    int code;
    char message[256];
    char file[128];
    int line;
    uint64_t timestamp;
} io_bundling_manager_01_error_t;

/* Serialization structure */
typedef struct io_bundling_manager_01_serialization_header {
    uint32_t magic;
    uint32_t version;
    uint32_t data_size;
    uint32_t checksum;
    uint64_t timestamp;
} io_bundling_manager_01_serialization_header_t;

/* Async operation structure */
typedef struct io_bundling_manager_01_async_op {
    uint32_t id;
    int type;
    void* data;
    void (*callback)(void* result, int error_code);
    atomic_bool completed;
    atomic_bool cancelled;
    uint64_t start_time;
} io_bundling_manager_01_async_op_t;

/* Memory budget tracking */
typedef struct io_bundling_manager_01_memory_tracker {
    size_t total_budget;
    size_t current_usage;
    size_t peak_usage;
    uint32_t allocation_count;
    uint32_t eviction_count;
    bool auto_eviction_enabled;
    float eviction_threshold;
} io_bundling_manager_01_memory_tracker_t;

/* File watcher structure */
typedef struct io_bundling_manager_01_file_watcher {
    char path[512];
    uint64_t last_modified;
    bool (*callback)(const char* path, void* user_data);
    void* user_data;
    bool active;
} io_bundling_manager_01_file_watcher_t;

/* Bundle structure */
typedef struct io_bundling_manager_01_bundle {
    uint32_t id;
    char name[256];
    void* data;
    size_t data_size;
    uint32_t asset_count;
    uint64_t creation_time;
    uint64_t last_modified;
    bool compressed;
} io_bundling_manager_01_bundle_t;

/* Scene parser structure */
typedef struct io_bundling_manager_01_scene {
    uint32_t id;
    char name[256];
    void* nodes;
    uint32_t node_count;
    uint32_t depth;
    bool parsed;
} io_bundling_manager_01_scene_t;

/* Format converter structure */
typedef struct io_bundling_manager_01_converter {
    int source_format;
    int target_format;
    int (*convert_func)(const void* input, size_t input_size, void** output, size_t* output_size);
} io_bundling_manager_01_converter_t;

/* Telemetry structure */
typedef struct io_bundling_manager_01_telemetry {
    uint64_t total_operations;
    double avg_operation_time_ms;
    double min_operation_time_ms;
    double max_operation_time_ms;
    uint64_t memory_allocations;
    uint64_t memory_deallocations;
    uint64_t cache_operations;
    uint64_t async_operations;
    uint64_t error_count;
} io_bundling_manager_01_telemetry_t;

/* Resource pool structure */
typedef struct io_bundling_manager_01_resource_pool {
    void* resources[IO_BUNDLING_MANAGER_01_POOL_SIZE];
    size_t resource_sizes[IO_BUNDLING_MANAGER_01_POOL_SIZE];
    atomic_bool available[IO_BUNDLING_MANAGER_01_POOL_SIZE];
    atomic_int available_count;
    size_t total_allocated;
} io_bundling_manager_01_resource_pool_t;

typedef struct io_bundling_manager_01_desc {
    uint32_t flags;
    size_t initial_capacity;
    void* user_data;
    void* allocator;
} io_bundling_manager_01_desc_t;

typedef struct io_bundling_manager_01_stats {
    uint64_t total_allocations;
    uint64_t active_count;
    uint64_t peak_count;
    size_t memory_used;
    size_t memory_peak;
    double avg_process_time_ms;
} io_bundling_manager_01_stats_t;

typedef struct io_bundling_manager_01_scene_data {
    uint32_t node_count;
    uint32_t mesh_count;
    uint32_t material_count;
    uint32_t texture_count;
    void* nodes;
    void* meshes;
    void* materials;
    void* textures;
} io_bundling_manager_01_scene_data_t;

/* Scene file parsing structures */
typedef struct io_bundling_manager_01_scene_node {
    uint32_t id;
    char name[256];
    float transform[16];  /* 4x4 matrix */
    uint32_t parent_id;
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t child_count;
    uint32_t* children;
} io_bundling_manager_01_scene_node_t;

typedef struct io_bundling_manager_01_scene_mesh {
    uint32_t id;
    char name[256];
    uint32_t vertex_count;
    uint32_t index_count;
    void* vertices;
    void* indices;
    uint32_t material_id;
} io_bundling_manager_01_scene_mesh_t;

typedef struct io_bundling_manager_01_scene_material {
    uint32_t id;
    char name[256];
    float base_color[4];
    float metallic;
    float roughness;
    uint32_t base_texture_id;
    uint32_t normal_texture_id;
    uint32_t metallic_roughness_texture_id;
} io_bundling_manager_01_scene_material_t;

typedef struct io_bundling_manager_01_scene_texture {
    uint32_t id;
    char name[256];
    char file_path[512];
    uint32_t width;
    uint32_t height;
    uint32_t format;
    void* data;
} io_bundling_manager_01_scene_texture_t;

typedef struct io_bundling_manager_01_scene {
    uint32_t node_count;
    uint32_t mesh_count;
    uint32_t material_count;
    uint32_t texture_count;
    io_bundling_manager_01_scene_node_t* nodes;
    io_bundling_manager_01_scene_mesh_t* meshes;
    io_bundling_manager_01_scene_material_t* materials;
    io_bundling_manager_01_scene_texture_t* textures;
} io_bundling_manager_01_scene_t;

/* Scene file parsing structures */
typedef struct io_bundling_manager_01_scene_node {
    uint32_t id;
    char name[256];
    float transform[16];  /* 4x4 matrix */
    uint32_t parent_id;
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t child_count;
    uint32_t* children;
} io_bundling_manager_01_scene_node_t;

typedef struct io_bundling_manager_01_scene_object {
    uint32_t id;
    char name[256];
    char type[64];
    uint32_t node_id;
    size_t data_size;
    void* data;
} io_bundling_manager_01_scene_object_t;

typedef struct io_bundling_manager_01_scene {
    uint32_t version;
    char name[256];
    uint32_t node_count;
    uint32_t object_count;
    io_bundling_manager_01_scene_node_t* nodes;
    io_bundling_manager_01_scene_object_t* objects;
} io_bundling_manager_01_scene_t;

/* Async file loading structures */
typedef enum {
    IO_BUNDLING_MANAGER_01_ASYNC_OP_NONE,
    IO_BUNDLING_MANAGER_01_ASYNC_OP_LOAD,
    IO_BUNDLING_MANAGER_01_ASYNC_OP_SAVE,
    IO_BUNDLING_MANAGER_01_ASYNC_OP_PARSE
} io_bundling_manager_01_async_op_type_t;

typedef struct io_bundling_manager_01_async_operation {
    uint32_t id;
    io_bundling_manager_01_async_op_type_t type;
    char file_path[512];
    void* buffer;
    size_t buffer_size;
    size_t processed_bytes;
    bool is_completed;
    int error_code;
    void (*callback)(struct io_bundling_manager_01_async_operation*);
    void* user_data;
} io_bundling_manager_01_async_operation_t;

/* Multi-threaded batch processing structures */
typedef struct io_bundling_manager_01_batch_task {
    uint32_t id;
    void (*task_func)(void*);
    void* task_data;
    size_t task_data_size;
    bool is_completed;
    int error_code;
} io_bundling_manager_01_batch_task_t;

typedef struct io_bundling_manager_01_batch_queue {
    io_bundling_manager_01_batch_task_t* tasks;
    uint32_t capacity;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    thrd_mutex_t mutex;
    thrd_cond_t condition;
    bool is_shutdown;
} io_bundling_manager_01_batch_queue_t;


/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static io_bundling_manager_01_stats_t s_manager_01_stats = {0};
static bool s_manager_01_initialized = false;
static uint32_t s_manager_01_next_id = 1;

/* Scene file parsing globals */
static io_bundling_manager_01_scene_t* s_active_scene = NULL;
static char s_current_scene_file[512] = {0};

/* Async file loading globals */
static io_bundling_manager_01_async_operation_t* s_async_operations[IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS] = {0};
static thrd_t s_async_threads[IO_BUNDLING_MANAGER_01_ASYNC_THREAD_COUNT];
static atomic_uint s_next_async_id = ATOMIC_VAR_INIT(1);
static bool s_async_system_running = false;

/* Multi-threaded batch processing globals */
static io_bundling_manager_01_batch_queue_t s_batch_queue = {0};
static thrd_t s_worker_threads[IO_BUNDLING_MANAGER_01_MAX_WORKER_THREADS];
static atomic_uint s_next_task_id = ATOMIC_VAR_INIT(1);
static bool s_batch_system_running = false;
static atomic_bool s_manager_01_thread_safe_init = ATOMIC_VAR_INIT(false);
static io_bundling_manager_01_memory_tracker_t s_memory_tracker = {0};
static io_bundling_manager_01_telemetry_t s_telemetry = {0};
static io_bundling_manager_01_resource_pool_t s_resource_pool = {0};
static io_bundling_manager_01_file_watcher_t s_file_watchers[IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES] = {0};
static io_bundling_manager_01_converter_t s_converters[32] = {0};
static atomic_int s_converter_count = ATOMIC_VAR_INIT(0);
static io_bundling_manager_01_error_t s_last_error = {0};
static mtx_t s_global_mutex;
static cnd_t s_global_condition;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int io_bundling_manager_01_validate_internal(io_bundling_manager_01_t* ctx);
static int io_bundling_manager_01_cleanup_internal(io_bundling_manager_01_t* ctx);
static int io_bundling_manager_01_parse_scene_file(const char* file_path, io_bundling_manager_01_scene_data_t* scene_data);
static int io_bundling_manager_01_serialize_state(io_bundling_manager_01_t* ctx, const char* file_path);
static void io_bundling_manager_01_shutdown_batch_system(void);
static int io_bundling_manager_01_init_async_system(void);
static void io_bundling_manager_01_update_memory_stats(void);

/* Scene file parsing forward declarations */
static int io_bundling_manager_01_parse_scene_json(const char* json_data, size_t size, io_bundling_manager_01_scene_t* scene);
static int io_bundling_manager_01_parse_scene_binary(const void* data, size_t size, io_bundling_manager_01_scene_t* scene);
static int io_bundling_manager_01_validate_scene_structure(io_bundling_manager_01_scene_t* scene);
static void io_bundling_manager_01_cleanup_scene(io_bundling_manager_01_scene_t* scene);

/* Async file loading forward declarations */
static int io_bundling_manager_01_init_async_system(void);
static void io_bundling_manager_01_shutdown_async_system(void);
static int io_bundling_manager_01_enqueue_async_operation(io_bundling_manager_01_async_operation_t* op);
static io_bundling_manager_01_async_operation_t* io_bundling_manager_01_dequeue_async_operation(void);
static int io_bundling_manager_01_async_worker_thread(void* arg);

/* Multi-threaded batch processing forward declarations */
static int io_bundling_manager_01_init_batch_system(void);
static void io_bundling_manager_01_shutdown_batch_system(void);
static int io_bundling_manager_01_enqueue_batch_task(io_bundling_manager_01_batch_task_t* task);
static io_bundling_manager_01_batch_task_t* io_bundling_manager_01_dequeue_batch_task(void);
static int io_bundling_manager_01_worker_thread(void* arg);

/* Error handling functions */
static void io_bundling_manager_01_set_error(int code, const char* message, const char* file, int line);
static const char* io_bundling_manager_01_get_error_string(int error_code);

/* Serialization functions */
static int io_bundling_manager_01_serialize_data(const void* data, size_t size, void** output, size_t* output_size);
static int io_bundling_manager_01_deserialize_data(const void* data, size_t size, void** output, size_t* output_size);
static uint32_t io_bundling_manager_01_calculate_checksum(const void* data, size_t size);

/* Thread safety functions */
static int io_bundling_manager_01_thread_safe_init(io_bundling_manager_01_t* ctx);
static void io_bundling_manager_01_memory_barrier(void);
static int io_bundling_manager_01_worker_thread_func(void* arg);

/* Memory management functions */
static int io_bundling_manager_01_track_allocation(size_t size);
static int io_bundling_manager_01_track_deallocation(size_t size);
static int io_bundling_manager_01_evict_if_needed(size_t required_size);
static void io_bundling_manager_01_update_memory_stats(void);

/* Async operation functions */
static uint32_t io_bundling_manager_01_start_async_operation(io_bundling_manager_01_t* ctx, int type, void* data, void (*callback)(void*, int));
static int io_bundling_manager_01_wait_async_operation(uint32_t op_id);
static void io_bundling_manager_01_process_async_operations(io_bundling_manager_01_t* ctx);

/* Hot reload functions */
static int io_bundling_manager_01_start_file_watcher(const char* path, bool (*callback)(const char*, void*), void* user_data);
static int io_bundling_manager_01_stop_file_watcher(const char* path);
static void io_bundling_manager_01_process_file_changes(void);

/* Asset bundling functions */
static int io_bundling_manager_01_create_bundle(const char* name, void* assets, size_t asset_count, io_bundling_manager_01_bundle_t** bundle);
static int io_bundling_manager_01_load_bundle(const char* path, io_bundling_manager_01_bundle_t** bundle);
static int io_bundling_manager_01_save_bundle(const io_bundling_manager_01_bundle_t* bundle, const char* path);

/* Scene parsing functions */
static int io_bundling_manager_01_parse_scene_file(const char* path, io_bundling_manager_01_scene_t** scene);
static int io_bundling_manager_01_validate_scene_structure(const io_bundling_manager_01_scene_t* scene);

/* Format conversion functions */
static int io_bundling_manager_01_register_converter(int source_format, int target_format, int (*convert_func)(const void*, size_t, void**, size_t*));
static int io_bundling_manager_01_convert_format(const void* input, size_t input_size, int source_format, int target_format, void** output, size_t* output_size);

/* Telemetry functions */
static void io_bundling_manager_01_record_operation_start(uint64_t operation_id);
static void io_bundling_manager_01_record_operation_end(uint64_t operation_id);
static void io_bundling_manager_01_update_telemetry_stats(void);

/* Resource pooling functions */
static void* io_bundling_manager_01_pool_allocate(size_t size);
static void io_bundling_manager_01_pool_deallocate(void* resource);
static int io_bundling_manager_01_pool_init(void);
static void io_bundling_manager_01_pool_cleanup(void);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

/* Error handling implementation */
static void io_bundling_manager_01_set_error(int code, const char* message, const char* file, int line) {
    s_last_error.code = code;
    strncpy(s_last_error.message, message ? message : "Unknown error", sizeof(s_last_error.message) - 1);
    strncpy(s_last_error.file, file ? file : "Unknown file", sizeof(s_last_error.file) - 1);
    s_last_error.line = line;
    s_last_error.timestamp = time(NULL);
    
    s_telemetry.error_count++;
    
    // Log the error
    // LOG_ERROR("[%s:%d] Error %d: %s", file, line, code, message);
}

static const char* io_bundling_manager_01_get_error_string(int error_code) {
    switch (error_code) {
        case IO_BUNDLING_MANAGER_01_ERROR_NONE: return "No error";
        case IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM: return "Invalid parameter";
        case IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED: return "Not initialized";
        case IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case IO_BUNDLING_MANAGER_01_ERROR_FILE_NOT_FOUND: return "File not found";
        case IO_BUNDLING_MANAGER_01_ERROR_FILE_READ_FAILED: return "File read failed";
        case IO_BUNDLING_MANAGER_01_ERROR_FILE_WRITE_FAILED: return "File write failed";
        case IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED: return "Parse failed";
        case IO_BUNDLING_MANAGER_01_ERROR_FORMAT_UNSUPPORTED: return "Format unsupported";
        case IO_BUNDLING_MANAGER_01_ERROR_SERIALIZATION_FAILED: return "Serialization failed";
        case IO_BUNDLING_MANAGER_01_ERROR_DESERIALIZATION_FAILED: return "Deserialization failed";
        case IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED: return "Threading failed";
        case IO_BUNDLING_MANAGER_01_ERROR_ASYNC_OPERATION_FAILED: return "Async operation failed";
        case IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED: return "Memory budget exceeded";
        case IO_BUNDLING_MANAGER_01_ERROR_ASSET_NOT_FOUND: return "Asset not found";
        case IO_BUNDLING_MANAGER_01_ERROR_BUNDLE_CORRUPTED: return "Bundle corrupted";
        case IO_BUNDLING_MANAGER_01_ERROR_HOT_RELOAD_FAILED: return "Hot reload failed";
        case IO_BUNDLING_MANAGER_01_ERROR_CONVERSION_FAILED: return "Conversion failed";
        case IO_BUNDLING_MANAGER_01_ERROR_POOL_EXHAUSTED: return "Resource pool exhausted";
        case IO_BUNDLING_MANAGER_01_ERROR_TELEMETRY_FAILED: return "Telemetry failed";
        default: return "Unknown error";
    }
}

/* Serialization implementation */
static uint32_t io_bundling_manager_01_calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ bytes[i];
    }
    return checksum;
}

static int io_bundling_manager_01_serialize_data(const void* data, size_t size, void** output, size_t* output_size) {
    if (!data || !output || !output_size) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid parameters for serialization", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    io_bundling_manager_01_serialization_header_t header = {
        .magic = 0x42494E44,  // "BIND"
        .version = IO_BUNDLING_MANAGER_01_BUNDLE_VERSION,
        .data_size = (uint32_t)size,
        .checksum = io_bundling_manager_01_calculate_checksum(data, size),
        .timestamp = time(NULL)
    };
    
    *output_size = sizeof(header) + size;
    *output = malloc(*output_size);
    if (!*output) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY, 
                                        "Failed to allocate memory for serialization", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(*output, &header, sizeof(header));
    memcpy((uint8_t*)*output + sizeof(header), data, size);
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_deserialize_data(const void* data, size_t size, void** output, size_t* output_size) {
    if (!data || size < sizeof(io_bundling_manager_01_serialization_header_t) || !output || !output_size) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid parameters for deserialization", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    const io_bundling_manager_01_serialization_header_t* header = 
        (const io_bundling_manager_01_serialization_header_t*)data;
    
    if (header->magic != 0x42494E44) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_BUNDLE_CORRUPTED, 
                                        "Invalid magic number in serialized data", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_BUNDLE_CORRUPTED;
    }
    
    if (header->version != IO_BUNDLING_MANAGER_01_BUNDLE_VERSION) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_FORMAT_UNSUPPORTED, 
                                        "Unsupported serialization version", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_FORMAT_UNSUPPORTED;
    }
    
    if (size < sizeof(*header) + header->data_size) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_DESERIALIZATION_FAILED, 
                                        "Insufficient data size for deserialization", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_DESERIALIZATION_FAILED;
    }
    
    const uint8_t* payload = (const uint8_t*)data + sizeof(*header);
    uint32_t calculated_checksum = io_bundling_manager_01_calculate_checksum(payload, header->data_size);
    
    if (calculated_checksum != header->checksum) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_BUNDLE_CORRUPTED, 
                                        "Checksum mismatch in serialized data", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_BUNDLE_CORRUPTED;
    }
    
    *output_size = header->data_size;
    *output = malloc(*output_size);
    if (!*output) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY, 
                                        "Failed to allocate memory for deserialization", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(*output, payload, *output_size);
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Thread safety implementation */
static void io_bundling_manager_01_memory_barrier(void) {
    atomic_thread_fence(memory_order_seq_cst);
}

static int io_bundling_manager_01_thread_safe_init(io_bundling_manager_01_t* ctx) {
    if (!ctx) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid context for thread-safe init", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    /* Use double-checked locking pattern */
    if (atomic_load(&ctx->initialization_state) == 1) {
        return IO_BUNDLING_MANAGER_01_ERROR_NONE;  /* Already initialized */
    }
    
    if (mtx_lock(&ctx->access_mutex) != thrd_success) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED, 
                                        "Failed to acquire mutex for initialization", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
    }
    
    /* Check again after acquiring lock */
    if (atomic_load(&ctx->initialization_state) == 1) {
        mtx_unlock(&ctx->access_mutex);
        return IO_BUNDLING_MANAGER_01_ERROR_NONE;
    }
    
    /* Initialize memory tracking */
    ctx->memory_budget = IO_BUNDLING_MANAGER_01_DEFAULT_MEMORY_BUDGET;
    ctx->memory_used = 0;
    atomic_store(&ctx->memory_tracking_enabled, true);
    
    /* Initialize thread synchronization */
    if (cnd_init(&ctx->condition_var) != thrd_success) {
        mtx_unlock(&ctx->access_mutex);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED, 
                                        "Failed to initialize condition variable", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
    }
    
    /* Initialize async operations */
    atomic_store(&ctx->async_operation_count, 0);
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        ctx->async_operations[i] = NULL;
    }
    
    /* Initialize hot reload */
    ctx->hot_reload_enabled = false;
    ctx->file_watcher = NULL;
    atomic_store(&ctx->watched_file_count, 0);
    
    /* Initialize resource pool */
    ctx->resource_pool = &s_resource_pool;
    atomic_store(&ctx->pool_available_count, IO_BUNDLING_MANAGER_01_POOL_SIZE);
    
    /* Initialize telemetry */
    ctx->operation_count = 0;
    ctx->total_operation_time_ms = 0.0;
    ctx->cache_hits = 0;
    ctx->cache_misses = 0;
    
    /* Memory barrier to ensure all initialization is visible */
    io_bundling_manager_01_memory_barrier();
    
    /* Mark as initialized */
    atomic_store(&ctx->initialization_state, 1);
    ctx->is_initialized = true;
    
    mtx_unlock(&ctx->access_mutex);
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_worker_thread_func(void* arg) {
    io_bundling_manager_01_t* ctx = (io_bundling_manager_01_t*)arg;
    if (!ctx) return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    
    while (atomic_load(&ctx->initialization_state) == 1) {
        /* Process async operations */
        io_bundling_manager_01_process_async_operations(ctx);
        
        /* Process file changes for hot reload */
        if (ctx->hot_reload_enabled) {
            io_bundling_manager_01_process_file_changes();
        }
        
        /* Update telemetry */
        io_bundling_manager_01_update_telemetry_stats();
        
        /* Sleep for a short time */
        thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 1000000}, NULL);  /* 1ms */
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Memory management implementation */
static int io_bundling_manager_01_track_allocation(size_t size) {
    if (s_memory_tracker.current_usage + size > s_memory_tracker.total_budget) {
        if (s_memory_tracker.auto_eviction_enabled) {
            if (io_bundling_manager_01_evict_if_needed(size) != IO_BUNDLING_MANAGER_01_ERROR_NONE) {
                io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED, 
                                                "Memory budget exceeded and eviction failed", __FILE__, __LINE__);
                return IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED;
            }
        } else {
            io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED, 
                                            "Memory budget exceeded", __FILE__, __LINE__);
            return IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED;
        }
    }
    
    s_memory_tracker.current_usage += size;
    s_memory_tracker.allocation_count++;
    if (s_memory_tracker.current_usage > s_memory_tracker.peak_usage) {
        s_memory_tracker.peak_usage = s_memory_tracker.current_usage;
    }
    
    s_telemetry.memory_allocations++;
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_track_deallocation(size_t size) {
    if (s_memory_tracker.current_usage >= size) {
        s_memory_tracker.current_usage -= size;
    } else {
        s_memory_tracker.current_usage = 0;
    }
    
    s_telemetry.memory_deallocations++;
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_evict_if_needed(size_t required_size) {
    /* Simple LRU eviction - in a real implementation this would be more sophisticated */
    size_t evicted = 0;
    uint32_t evicted_count = 0;
    
    /* Evict until we have enough space or reach eviction threshold */
    while (s_memory_tracker.current_usage + required_size > s_memory_tracker.total_budget * s_memory_tracker.eviction_threshold && 
           evicted_count < 10) {
        /* Placeholder eviction logic */
        size_t eviction_size = required_size / 2;  /* Evict half of required size each iteration */
        if (s_memory_tracker.current_usage >= eviction_size) {
            s_memory_tracker.current_usage -= eviction_size;
            evicted += eviction_size;
            evicted_count++;
        } else {
            break;
        }
    }
    
    s_memory_tracker.eviction_count += evicted_count;
    
    return (evicted > 0) ? IO_BUNDLING_MANAGER_01_ERROR_NONE : IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED;
}

static void io_bundling_manager_01_update_memory_stats(void) {
    /* Update global memory statistics */
    s_manager_01_stats.memory_used = s_memory_tracker.current_usage;
    s_manager_01_stats.memory_peak = s_memory_tracker.peak_usage;
}

/* Async operations implementation */
static uint32_t io_bundling_manager_01_start_async_operation(io_bundling_manager_01_t* ctx, int type, void* data, void (*callback)(void*, int)) {
    if (!ctx) return 0;
    
    if (atomic_load(&ctx->async_operation_count) >= IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_ASYNC_OPERATION_FAILED, 
                                        "Too many concurrent async operations", __FILE__, __LINE__);
        return 0;
    }
    
    /* Find free slot */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        if (ctx->async_operations[i] == NULL) {
            io_bundling_manager_01_async_op_t* op = malloc(sizeof(io_bundling_manager_01_async_op_t));
            if (!op) {
                io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY, 
                                                "Failed to allocate async operation", __FILE__, __LINE__);
                return 0;
            }
            
            op->id = (uint32_t)time(NULL) + i;  /* Simple ID generation */
            op->type = type;
            op->data = data;
            op->callback = callback;
            atomic_store(&op->completed, false);
            atomic_store(&op->cancelled, false);
            op->start_time = time(NULL);
            
            ctx->async_operations[i] = op;
            atomic_fetch_add(&ctx->async_operation_count, 1);
            
            s_telemetry.async_operations++;
            
            return op->id;
        }
    }
    
    return 0;
}

static int io_bundling_manager_01_wait_async_operation(uint32_t op_id) {
    /* In a real implementation, this would wait for the specific operation */
    /* For now, just simulate a wait */
    thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 10000000}, NULL);  /* 10ms */
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static void io_bundling_manager_01_process_async_operations(io_bundling_manager_01_t* ctx) {
    if (!ctx) return;
    
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        io_bundling_manager_01_async_op_t* op = (io_bundling_manager_01_async_op_t*)ctx->async_operations[i];
        if (op && atomic_load(&op->completed)) {
            /* Call callback if provided */
            if (op->callback) {
                op->callback(op->data, IO_BUNDLING_MANAGER_01_ERROR_NONE);
            }
            
            /* Clean up operation */
            free(op);
            ctx->async_operations[i] = NULL;
            atomic_fetch_sub(&ctx->async_operation_count, 1);
        }
    }
}

/* Resource pooling implementation */
static int io_bundling_manager_01_pool_init(void) {
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_POOL_SIZE; i++) {
        s_resource_pool.resources[i] = NULL;
        s_resource_pool.resource_sizes[i] = 0;
        atomic_store(&s_resource_pool.available[i], true);
    }
    atomic_store(&s_resource_pool.available_count, IO_BUNDLING_MANAGER_01_POOL_SIZE);
    s_resource_pool.total_allocated = 0;
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static void io_bundling_manager_01_pool_cleanup(void) {
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_POOL_SIZE; i++) {
        if (s_resource_pool.resources[i]) {
            free(s_resource_pool.resources[i]);
            s_resource_pool.resources[i] = NULL;
        }
    }
    s_resource_pool.total_allocated = 0;
    atomic_store(&s_resource_pool.available_count, 0);
}

static void* io_bundling_manager_01_pool_allocate(size_t size) {
    /* Find available slot with sufficient size or allocate new */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_POOL_SIZE; i++) {
        if (atomic_load(&s_resource_pool.available[i])) {
            if (s_resource_pool.resources[i] && s_resource_pool.resource_sizes[i] >= size) {
                atomic_store(&s_resource_pool.available[i], false);
                atomic_fetch_sub(&s_resource_pool.available_count, 1);
                return s_resource_pool.resources[i];
            } else if (!s_resource_pool.resources[i]) {
                /* Allocate new resource */
                void* resource = malloc(size);
                if (resource) {
                    s_resource_pool.resources[i] = resource;
                    s_resource_pool.resource_sizes[i] = size;
                    s_resource_pool.total_allocated += size;
                    atomic_store(&s_resource_pool.available[i], false);
                    atomic_fetch_sub(&s_resource_pool.available_count, 1);
                    return resource;
                }
            }
        }
    }
    
    /* Pool exhausted, allocate directly */
    io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_POOL_EXHAUSTED, 
                                    "Resource pool exhausted, allocating directly", __FILE__, __LINE__);
    return malloc(size);
}

static void io_bundling_manager_01_pool_deallocate(void* resource) {
    if (!resource) return;
    
    /* Find the resource in the pool */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_POOL_SIZE; i++) {
        if (s_resource_pool.resources[i] == resource) {
            atomic_store(&s_resource_pool.available[i], true);
            atomic_fetch_add(&s_resource_pool.available_count, 1);
            return;
        }
    }
    
/* Hot reload implementation */
static int io_bundling_manager_01_start_file_watcher(const char* path, bool (*callback)(const char*, void*), void* user_data) {
    if (!path || !callback) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid parameters for file watcher", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    int watched_count = atomic_load(&s_file_watchers[0].active ? 1 : 0);  /* Simplified */
    if (watched_count >= IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_HOT_RELOAD_FAILED, 
                                        "Too many watched files", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_HOT_RELOAD_FAILED;
    }
    
    /* Find free slot */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES; i++) {
        if (!s_file_watchers[i].active) {
            strncpy(s_file_watchers[i].path, path, sizeof(s_file_watchers[i].path) - 1);
            s_file_watchers[i].callback = callback;
            s_file_watchers[i].user_data = user_data;
            s_file_watchers[i].last_modified = time(NULL);
            s_file_watchers[i].active = true;
            
            return IO_BUNDLING_MANAGER_01_ERROR_NONE;
        }
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_HOT_RELOAD_FAILED;
}

static int io_bundling_manager_01_stop_file_watcher(const char* path) {
    if (!path) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid path for file watcher", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES; i++) {
        if (s_file_watchers[i].active && strcmp(s_file_watchers[i].path, path) == 0) {
            s_file_watchers[i].active = false;
            s_file_watchers[i].callback = NULL;
            s_file_watchers[i].user_data = NULL;
            
            return IO_BUNDLING_MANAGER_01_ERROR_NONE;
        }
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_FILE_NOT_FOUND;
}

static void io_bundling_manager_01_process_file_changes(void) {
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES; i++) {
        if (s_file_watchers[i].active && s_file_watchers[i].callback) {
            /* In a real implementation, this would check file modification times */
            /* For now, simulate periodic checking */
            static uint64_t last_check = 0;
            uint64_t current_time = time(NULL);
            
            if (current_time - last_check > 5) {  /* Check every 5 seconds */
                /* Simulate file change */
                bool changed = (current_time % 10 == 0);  /* Simulate occasional changes */
                
                if (changed) {
                    s_file_watchers[i].callback(s_file_watchers[i].path, s_file_watchers[i].user_data);
                    s_file_watchers[i].last_modified = current_time;
                }
                
                last_check = current_time;
            }
        }
    }
}

/* Asset bundling implementation */
static int io_bundling_manager_01_create_bundle(const char* name, void* assets, size_t asset_count, io_bundling_manager_01_bundle_t** bundle) {
    if (!name || !bundle) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid parameters for bundle creation", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    io_bundling_manager_01_bundle_t* new_bundle = malloc(sizeof(io_bundling_manager_01_bundle_t));
    if (!new_bundle) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY, 
                                        "Failed to allocate bundle", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    new_bundle->id = (uint32_t)time(NULL);
    strncpy(new_bundle->name, name, sizeof(new_bundle->name) - 1);
    new_bundle->data = assets;
    new_bundle->data_size = asset_count * 1024;  /* Simplified size calculation */
    new_bundle->asset_count = (uint32_t)asset_count;
    new_bundle->creation_time = time(NULL);
    new_bundle->last_modified = new_bundle->creation_time;
    new_bundle->compressed = false;
    
    *bundle = new_bundle;
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_load_bundle(const char* path, io_bundling_manager_01_bundle_t** bundle) {
    if (!path || !bundle) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid parameters for bundle loading", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    /* Simulate file loading */
    FILE* file = fopen(path, "rb");
    if (!file) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_FILE_NOT_FOUND, 
                                        "Bundle file not found", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_FILE_NOT_FOUND;
    }
    
    /* Read header */
    io_bundling_manager_01_serialization_header_t header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_FILE_READ_FAILED, 
                                        "Failed to read bundle header", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_FILE_READ_FAILED;
    }
    
    if (header.magic != 0x42494E44) {
        fclose(file);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_BUNDLE_CORRUPTED, 
                                        "Invalid bundle magic number", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_BUNDLE_CORRUPTED;
    }
    
    /* Read bundle data */
    void* bundle_data = malloc(header.data_size);
    if (!bundle_data) {
        fclose(file);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY, 
                                        "Failed to allocate bundle data", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    if (fread(bundle_data, header.data_size, 1, file) != 1) {
        free(bundle_data);
        fclose(file);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_FILE_READ_FAILED, 
                                        "Failed to read bundle data", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_FILE_READ_FAILED;
    }
    
    fclose(file);
    
    /* Create bundle structure */
    io_bundling_manager_01_bundle_t* new_bundle = malloc(sizeof(io_bundling_manager_01_bundle_t));
    if (!new_bundle) {
        free(bundle_data);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY, 
                                        "Failed to allocate bundle structure", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    new_bundle->id = header.version;  /* Use version as ID for loaded bundles */
    strncpy(new_bundle->name, path, sizeof(new_bundle->name) - 1);
    new_bundle->data = bundle_data;
    new_bundle->data_size = header.data_size;
    new_bundle->asset_count = 1;  /* Simplified */
    new_bundle->creation_time = header.timestamp;
    new_bundle->last_modified = header.timestamp;
    new_bundle->compressed = false;
    
    *bundle = new_bundle;
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_save_bundle(const io_bundling_manager_01_bundle_t* bundle, const char* path) {
    if (!bundle || !path) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid parameters for bundle saving", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    /* Serialize bundle data */
    void* serialized_data = NULL;
    size_t serialized_size = 0;
    
    int result = io_bundling_manager_01_serialize_data(bundle->data, bundle->data_size, 
                                                       &serialized_data, &serialized_size);
    if (result != IO_BUNDLING_MANAGER_01_ERROR_NONE) {
        return result;
    }
    
    /* Write to file */
    FILE* file = fopen(path, "wb");
    if (!file) {
        free(serialized_data);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_FILE_WRITE_FAILED, 
                                        "Failed to create bundle file", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_FILE_WRITE_FAILED;
    }
    
    if (fwrite(serialized_data, serialized_size, 1, file) != 1) {
        fclose(file);
        free(serialized_data);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_FILE_WRITE_FAILED, 
                                        "Failed to write bundle data", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_FILE_WRITE_FAILED;
    }
    
    fclose(file);
    free(serialized_data);
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Scene parsing implementation */
static int io_bundling_manager_01_parse_scene_file(const char* path, io_bundling_manager_01_scene_t** scene) {
    if (!path || !scene) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid parameters for scene parsing", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    /* Simulate scene file parsing */
    FILE* file = fopen(path, "r");
    if (!file) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_FILE_NOT_FOUND, 
                                        "Scene file not found", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_FILE_NOT_FOUND;
    }
    
    /* Read file content */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* file_content = malloc(file_size + 1);
    if (!file_content) {
        fclose(file);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY, 
                                        "Failed to allocate scene file buffer", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    if (fread(file_content, file_size, 1, file) != 1) {
        free(file_content);
        fclose(file);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_FILE_READ_FAILED, 
                                        "Failed to read scene file", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_FILE_READ_FAILED;
    }
    
    file_content[file_size] = '\0';
    fclose(file);
    
    /* Create scene structure */
    io_bundling_manager_01_scene_t* new_scene = malloc(sizeof(io_bundling_manager_01_scene_t));
    if (!new_scene) {
        free(file_content);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY, 
                                        "Failed to allocate scene structure", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    new_scene->id = (uint32_t)time(NULL);
    strncpy(new_scene->name, path, sizeof(new_scene->name) - 1);
    new_scene->nodes = file_content;  /* Store file content as nodes data */
    new_scene->node_count = 1;  /* Simplified */
    new_scene->depth = 1;
    new_scene->parsed = true;
    
    *scene = new_scene;
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_validate_scene_structure(const io_bundling_manager_01_scene_t* scene) {
    if (!scene) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid scene for validation", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    if (!scene->parsed) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED, 
                                        "Scene not parsed", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    
    if (scene->depth > IO_BUNDLING_MANAGER_01_MAX_SCENE_DEPTH) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED, 
                                        "Scene depth exceeds maximum", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    
    if (scene->node_count == 0) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED, 
                                        "Scene has no nodes", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Format conversion implementation */
static int io_bundling_manager_01_register_converter(int source_format, int target_format, 
                                                    int (*convert_func)(const void*, size_t, void**, size_t*)) {
    if (!convert_func) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid converter function", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    int converter_count = atomic_load(&s_converter_count);
    if (converter_count >= 32) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_CONVERSION_FAILED, 
                                        "Too many registered converters", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_CONVERSION_FAILED;
    }
    
    s_converters[converter_count].source_format = source_format;
    s_converters[converter_count].target_format = target_format;
    s_converters[converter_count].convert_func = convert_func;
    
    atomic_fetch_add(&s_converter_count, 1);
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_convert_format(const void* input, size_t input_size, 
                                                int source_format, int target_format, 
                                                void** output, size_t* output_size) {
    if (!input || !output || !output_size) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid parameters for format conversion", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    /* Find appropriate converter */
    int converter_count = atomic_load(&s_converter_count);
    for (int i = 0; i < converter_count; i++) {
        if (s_converters[i].source_format == source_format && 
            s_converters[i].target_format == target_format) {
            
            return s_converters[i].convert_func(input, input_size, output, output_size);
        }
    }
    
/* Telemetry implementation */
static void io_bundling_manager_01_record_operation_start(uint64_t operation_id) {
    (void)operation_id;  /* Placeholder for operation-specific tracking */
    s_telemetry.total_operations++;
}

static void io_bundling_manager_01_record_operation_end(uint64_t operation_id) {
    (void)operation_id;  /* Placeholder for operation-specific tracking */
    /* Update timing statistics would go here */
}

static void io_bundling_manager_01_update_telemetry_stats(void) {
    /* Calculate running averages and update statistics */
    if (s_telemetry.total_operations > 0) {
        s_telemetry.avg_operation_time_ms = s_telemetry.total_operations > 0 ? 
            (s_telemetry.avg_operation_time_ms + 1.0) / 2.0 : 0.0;  /* Simplified */
    }
    
    /* Update cache hit ratio */
    uint64_t total_cache_ops = s_telemetry.cache_hits + s_telemetry.cache_misses;
    if (total_cache_ops > 0) {
        /* Cache hit ratio calculation would go here */
    }
}

static int io_bundling_manager_01_validate_internal(io_bundling_manager_01_t* ctx) {
    if (!ctx) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid context for validation", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->is_initialized) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED, 
                                        "Context not initialized", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED;
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_cleanup_internal(io_bundling_manager_01_t* ctx) {
    if (!ctx) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid context for cleanup", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    /* Stop worker thread */
    if (ctx->flags & IO_BUNDLING_MANAGER_01_FLAG_THREADED) {
        atomic_store(&ctx->initialization_state, 0);
        thrd_join(ctx->worker_thread, NULL);
    }
    
    /* Clean up async operations */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        if (ctx->async_operations[i]) {
            free(ctx->async_operations[i]);
            ctx->async_operations[i] = NULL;
        }
    }
    
    /* Clean up file watchers */
    if (ctx->hot_reload_enabled) {
        for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES; i++) {
            if (s_file_watchers[i].active) {
                s_file_watchers[i].active = false;
                s_file_watchers[i].callback = NULL;
                s_file_watchers[i].user_data = NULL;
            }
        }
    }
    
    /* Clean up synchronization objects */
    cnd_destroy(&ctx->condition_var);
    mtx_destroy(&ctx->access_mutex);
    
    /* Clean up internal data */
    if (ctx->internal_data) {
        free(ctx->internal_data);
        ctx->internal_data = NULL;
    }
    
    ctx->is_dirty = false;
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* ============================================================================
 * SCENE FILE PARSING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_manager_01_parse_scene_file(const char* file_path, io_bundling_manager_01_scene_data_t* scene_data) {
    if (!file_path || !scene_data) return -1;
    
    // Check file extension to determine format
    const char* ext = strrchr(file_path, '.');
    if (!ext) return -2;
    
    ext++; // Skip the dot
    
    // Initialize scene data
    memset(scene_data, 0, sizeof(io_bundling_manager_01_scene_data_t));
    
    if (strcasecmp(ext, "gltf") == 0 || strcasecmp(ext, "glb") == 0) {
        // Parse glTF scene file
        FILE* file = fopen(file_path, "rb");
        if (!file) return -3;
        
        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        if (file_size <= 0) {
            fclose(file);
            return -4;
        }
        
        // Read file content
        char* buffer = malloc(file_size + 1);
        if (!buffer) {
            fclose(file);
            return -5;
        }
        
        size_t read_size = fread(buffer, 1, file_size, file);
        fclose(file);
        
        if (read_size != (size_t)file_size) {
            free(buffer);
            return -6;
        }
        
        buffer[read_size] = '\0';
        
        // Parse JSON-based glTF
        io_bundling_manager_01_scene_t scene = {0};
        int result = io_bundling_manager_01_parse_scene_json(buffer, read_size, &scene);
        
        if (result == 0) {
            // Extract scene statistics
            scene_data->node_count = scene.node_count;
            scene_data->mesh_count = scene.mesh_count;
            scene_data->material_count = scene.material_count;
            scene_data->texture_count = scene.texture_count;
            
            // Store pointers to scene data (in a real implementation, these would be properly allocated)
            scene_data->nodes = scene.nodes;
            scene_data->meshes = scene.meshes;
            scene_data->materials = scene.materials;
            scene_data->textures = scene.textures;
        }
        
        io_bundling_manager_01_cleanup_scene(&scene);
        free(buffer);
        
        return result;
        
    } else if (strcasecmp(ext, "fbx") == 0) {
        // Parse FBX scene file
        // Placeholder implementation
        scene_data->node_count = 15;
        scene_data->mesh_count = 8;
        scene_data->material_count = 5;
        scene_data->texture_count = 12;
        
        // Allocate placeholder data
        scene_data->nodes = malloc(sizeof(io_bundling_manager_01_scene_node_t) * scene_data->node_count);
        scene_data->meshes = malloc(sizeof(io_bundling_manager_01_scene_mesh_t) * scene_data->mesh_count);
        scene_data->materials = malloc(sizeof(io_bundling_manager_01_scene_material_t) * scene_data->material_count);
        scene_data->textures = malloc(sizeof(io_bundling_manager_01_scene_texture_t) * scene_data->texture_count);
        
        return 0;
        
    } else {
        // Unsupported format
        return -7;
    }
}

static int io_bundling_manager_01_parse_scene_json(const char* json_data, size_t size, io_bundling_manager_01_scene_t* scene) {
    if (!json_data || !scene) return -1;
    
    // Placeholder JSON parsing implementation
    // In a real implementation, this would use a JSON library like cJSON or jsmn
    
    // Initialize scene structure
    memset(scene, 0, sizeof(io_bundling_manager_01_scene_t));
    
    // Simulate parsing scene from JSON
    scene->node_count = 10;
    scene->mesh_count = 5;
    scene->material_count = 3;
    scene->texture_count = 8;
    
    // Allocate arrays for scene data
    scene->nodes = malloc(sizeof(io_bundling_manager_01_scene_node_t) * scene->node_count);
    scene->meshes = malloc(sizeof(io_bundling_manager_01_scene_mesh_t) * scene->mesh_count);
    scene->materials = malloc(sizeof(io_bundling_manager_01_scene_material_t) * scene->material_count);
    scene->textures = malloc(sizeof(io_bundling_manager_01_scene_texture_t) * scene->texture_count);
    
    if (!scene->nodes || !scene->meshes || !scene->materials || !scene->textures) {
        io_bundling_manager_01_cleanup_scene(scene);
        return -2;
    }
    
    // Initialize scene data with placeholder values
    for (uint32_t i = 0; i < scene->node_count; i++) {
        io_bundling_manager_01_scene_node_t* node = &scene->nodes[i];
        node->id = i;
        snprintf(node->name, sizeof(node->name), "Node_%u", i);
        // Initialize identity matrix
        memset(node->transform, 0, sizeof(node->transform));
        node->transform[0] = node->transform[5] = node->transform[10] = node->transform[15] = 1.0f;
        node->parent_id = (i > 0) ? i - 1 : 0;
        node->mesh_id = i % scene->mesh_count;
        node->material_id = i % scene->material_count;
        node->child_count = 0;
        node->children = NULL;
    }
    
    return 0;
}

static int io_bundling_manager_01_parse_scene_binary(const void* data, size_t size, io_bundling_manager_01_scene_t* scene) {
    if (!data || !scene) return -1;
    
    // Placeholder binary parsing implementation
    // In a real implementation, this would parse binary glTF or custom format
    
    memset(scene, 0, sizeof(io_bundling_manager_01_scene_t));
    
    // Simulate parsing binary scene data
    scene->node_count = 8;
    scene->mesh_count = 4;
    scene->material_count = 2;
    scene->texture_count = 6;
    
    // Allocate and initialize scene data
    scene->nodes = malloc(sizeof(io_bundling_manager_01_scene_node_t) * scene->node_count);
    scene->meshes = malloc(sizeof(io_bundling_manager_01_scene_mesh_t) * scene->mesh_count);
    scene->materials = malloc(sizeof(io_bundling_manager_01_scene_material_t) * scene->material_count);
    scene->textures = malloc(sizeof(io_bundling_manager_01_scene_texture_t) * scene->texture_count);
    
    if (!scene->nodes || !scene->meshes || !scene->materials || !scene->textures) {
        io_bundling_manager_01_cleanup_scene(scene);
        return -2;
    }
    
    // Initialize with binary data
    for (uint32_t i = 0; i < scene->node_count; i++) {
        io_bundling_manager_01_scene_node_t* node = &scene->nodes[i];
        node->id = i;
        snprintf(node->name, sizeof(node->name), "BinaryNode_%u", i);
        // Copy transform from binary data (placeholder)
        memset(node->transform, 0, sizeof(node->transform));
        node->transform[0] = node->transform[5] = node->transform[10] = node->transform[15] = 1.0f;
        node->parent_id = 0;
        node->mesh_id = i % scene->mesh_count;
        node->material_id = i % scene->material_count;
        node->child_count = 0;
        node->children = NULL;
    }
    
    return 0;
}

static int io_bundling_manager_01_validate_scene_structure(io_bundling_manager_01_scene_t* scene) {
    if (!scene) return -1;
    
    // Validate scene structure
    if (scene->node_count == 0) return -2;
    if (scene->mesh_count == 0) return -3;
    if (scene->material_count == 0) return -4;
    
    // Validate node hierarchy
    for (uint32_t i = 0; i < scene->node_count; i++) {
        io_bundling_manager_01_scene_node_t* node = &scene->nodes[i];
        if (node->parent_id >= scene->node_count && node->parent_id != 0) return -5;
        if (node->mesh_id >= scene->mesh_count && node->mesh_id != 0xFFFFFFFF) return -6;
        if (node->material_id >= scene->material_count && node->material_id != 0xFFFFFFFF) return -7;
    }
    
    return 0;
}

static void io_bundling_manager_01_cleanup_scene(io_bundling_manager_01_scene_t* scene) {
    if (!scene) return;
    
    // Free scene data
    if (scene->nodes) {
        for (uint32_t i = 0; i < scene->node_count; i++) {
            if (scene->nodes[i].children) {
                free(scene->nodes[i].children);
            }
        }
        free(scene->nodes);
        scene->nodes = NULL;
    }
    
    if (scene->meshes) {
        free(scene->meshes);
        scene->meshes = NULL;
    }
    
    if (scene->materials) {
        free(scene->materials);
        scene->materials = NULL;
    }
    
    if (scene->textures) {
        free(scene->textures);
        scene->textures = NULL;
    }
    
    scene->node_count = 0;
    scene->mesh_count = 0;
    scene->material_count = 0;
    scene->texture_count = 0;
}

/* ============================================================================
 * SERIALIZATION AND BATCH PROCESSING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_manager_01_serialize_state(io_bundling_manager_01_t* ctx, const char* file_path) {
    if (!ctx || !file_path) return -1;
    
    // Create binary header
    typedef struct {
        uint32_t magic;
        uint32_t version;
        uint64_t timestamp;
        uint32_t data_size;
        uint32_t checksum;
    } state_header_t;
    
    state_header_t header = {0};
    header.magic = 0x4D414E47; // "MANG"
    header.version = 1;
    header.timestamp = (uint64_t)time(NULL);
    header.data_size = sizeof(io_bundling_manager_01_t);
    
    // Calculate checksum
    const uint8_t* ctx_bytes = (const uint8_t*)ctx;
    uint32_t checksum = 0;
    for (size_t i = 0; i < sizeof(io_bundling_manager_01_t); i++) {
        checksum = (checksum << 1) | (checksum >> 31);
        checksum += ctx_bytes[i];
    }
    header.checksum = checksum;
    
    // Write to file
    FILE* file = fopen(file_path, "wb");
    if (!file) return -2;
    
    if (fwrite(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -3;
    }
    
    if (fwrite(ctx, sizeof(io_bundling_manager_01_t), 1, file) != 1) {
        fclose(file);
        return -4;
    }
    
    fclose(file);
    printf("Manager state serialized to %s\n", file_path);
    return 0;
}

static void io_bundling_manager_01_shutdown_batch_system(void) {
    // Placeholder for batch system shutdown
    // In a real implementation, this would:
    // - Signal all worker threads to shutdown
    // - Wait for all pending tasks to complete
    // - Clean up thread pools and queues
    // - Release batch processing resources
    printf("Batch processing system shutdown\n");
}

static int io_bundling_manager_01_init_async_system(void) {
    // Initialize async file loading system
    // In a real implementation, this would:
    // - Create worker thread pool
    // - Initialize async operation queues
    // - Set up file I/O completion ports
    // - Start background file monitoring threads
    
    printf("Initializing async file loading system\n");
    
    // Initialize async operations array
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        s_async_operations[i] = NULL;
    }
    
    // Start async worker threads
    s_async_system_running = true;
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_ASYNC_THREAD_COUNT; i++) {
        if (thrd_create(&s_async_threads[i], io_bundling_manager_01_async_worker_thread, NULL) != thrd_success) {
            printf("Failed to create async worker thread %d\n", i);
            return -1;
        }
    }
    
    return 0;
}

static void io_bundling_manager_01_update_memory_stats(void) {
    // Update global memory statistics
    s_manager_01_stats.memory_used = s_memory_tracker.current_usage;
    s_manager_01_stats.memory_peak = s_memory_tracker.peak_usage;
    
    // Calculate average processing time
    if (s_telemetry.total_operations > 0) {
        s_manager_01_stats.avg_process_time_ms = 
            s_telemetry.total_operation_time_ms / s_telemetry.total_operations;
    }
}

/* ============================================================================
 * SCENE FILE PARSING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_manager_01_parse_scene_json(const char* json_data, size_t size, io_bundling_manager_01_scene_t* scene) {
    if (!json_data || !scene || size == 0) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    /* Simple JSON parsing implementation */
    /* In a real implementation, this would use a proper JSON parser */
    
    /* Parse scene header */
    scene->version = IO_BUNDLING_MANAGER_01_SCENE_VERSION;
    strncpy(scene->name, "Parsed Scene", sizeof(scene->name) - 1);
    
    /* Parse nodes */
    scene->node_count = 1; /* Simple placeholder */
    scene->nodes = calloc(scene->node_count, sizeof(io_bundling_manager_01_scene_node_t));
    if (!scene->nodes) {
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize default node */
    scene->nodes[0].id = 1;
    strncpy(scene->nodes[0].name, "RootNode", sizeof(scene->nodes[0].name) - 1);
    scene->nodes[0].parent_id = 0;
    scene->nodes[0].mesh_id = 0;
    scene->nodes[0].material_id = 0;
    scene->nodes[0].child_count = 0;
    scene->nodes[0].children = NULL;
    
    /* Identity matrix */
    memset(scene->nodes[0].transform, 0, sizeof(scene->nodes[0].transform));
    scene->nodes[0].transform[0] = 1.0f;
    scene->nodes[0].transform[5] = 1.0f;
    scene->nodes[0].transform[10] = 1.0f;
    scene->nodes[0].transform[15] = 1.0f;
    
    /* Parse objects */
    scene->object_count = 0;
    scene->objects = NULL;
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_parse_scene_binary(const void* data, size_t size, io_bundling_manager_01_scene_t* scene) {
    if (!data || !scene || size < sizeof(uint32_t)) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    const uint8_t* ptr = (const uint8_t*)data;
    size_t offset = 0;
    
    /* Read version */
    if (offset + sizeof(uint32_t) > size) return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    scene->version = *(const uint32_t*)(ptr + offset);
    offset += sizeof(uint32_t);
    
    /* Validate version */
    if (scene->version != IO_BUNDLING_MANAGER_01_SCENE_VERSION) {
        return IO_BUNDLING_MANAGER_01_ERROR_FORMAT_UNSUPPORTED;
    }
    
    /* Read scene name */
    if (offset + sizeof(uint32_t) > size) return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    uint32_t name_len = *(const uint32_t*)(ptr + offset);
    offset += sizeof(uint32_t);
    
    if (offset + name_len > size) return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    memcpy(scene->name, ptr + offset, name_len);
    scene->name[name_len] = '\0';
    offset += name_len;
    
    /* Read node count */
    if (offset + sizeof(uint32_t) > size) return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    scene->node_count = *(const uint32_t*)(ptr + offset);
    offset += sizeof(uint32_t);
    
    if (scene->node_count > IO_BUNDLING_MANAGER_01_MAX_SCENE_NODES) {
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    
    /* Read nodes */
    if (scene->node_count > 0) {
        size_t nodes_size = scene->node_count * sizeof(io_bundling_manager_01_scene_node_t);
        if (offset + nodes_size > size) return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
        
        scene->nodes = malloc(nodes_size);
        if (!scene->nodes) return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
        
        memcpy(scene->nodes, ptr + offset, nodes_size);
        offset += nodes_size;
        
        /* Fix up child pointers */
        for (uint32_t i = 0; i < scene->node_count; i++) {
            if (scene->nodes[i].child_count > 0) {
                size_t children_size = scene->nodes[i].child_count * sizeof(uint32_t);
                if (offset + children_size > size) {
                    free(scene->nodes);
                    scene->nodes = NULL;
                    return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
                }
                
                scene->nodes[i].children = malloc(children_size);
                if (!scene->nodes[i].children) {
                    free(scene->nodes);
                    scene->nodes = NULL;
                    return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
                }
                
                memcpy(scene->nodes[i].children, ptr + offset, children_size);
                offset += children_size;
            }
        }
    }
    
    /* Read object count */
    if (offset + sizeof(uint32_t) > size) return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    scene->object_count = *(const uint32_t*)(ptr + offset);
    offset += sizeof(uint32_t);
    
    if (scene->object_count > IO_BUNDLING_MANAGER_01_MAX_SCENE_OBJECTS) {
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    
    /* Read objects */
    if (scene->object_count > 0) {
        size_t objects_size = scene->object_count * sizeof(io_bundling_manager_01_scene_object_t);
        if (offset + objects_size > size) return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
        
        scene->objects = malloc(objects_size);
        if (!scene->objects) return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
        
        memcpy(scene->objects, ptr + offset, objects_size);
        offset += objects_size;
        
        /* Fix up object data pointers */
        for (uint32_t i = 0; i < scene->object_count; i++) {
            if (scene->objects[i].data_size > 0) {
                if (offset + scene->objects[i].data_size > size) {
                    free(scene->objects);
                    scene->objects = NULL;
                    return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
                }
                
                scene->objects[i].data = malloc(scene->objects[i].data_size);
                if (!scene->objects[i].data) {
                    free(scene->objects);
                    scene->objects = NULL;
                    return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
                }
                
                memcpy(scene->objects[i].data, ptr + offset, scene->objects[i].data_size);
                offset += scene->objects[i].data_size;
            }
        }
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static int io_bundling_manager_01_validate_scene_structure(io_bundling_manager_01_scene_t* scene) {
    if (!scene) return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    
    /* Validate version */
    if (scene->version != IO_BUNDLING_MANAGER_01_SCENE_VERSION) {
        return IO_BUNDLING_MANAGER_01_ERROR_FORMAT_UNSUPPORTED;
    }
    
    /* Validate node count */
    if (scene->node_count > IO_BUNDLING_MANAGER_01_MAX_SCENE_NODES) {
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    
    /* Validate object count */
    if (scene->object_count > IO_BUNDLING_MANAGER_01_MAX_SCENE_OBJECTS) {
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    
    /* Validate node hierarchy */
    for (uint32_t i = 0; i < scene->node_count; i++) {
        io_bundling_manager_01_scene_node_t* node = &scene->nodes[i];
        
        /* Check parent reference */
        if (node->parent_id != 0) {
            bool parent_found = false;
            for (uint32_t j = 0; j < scene->node_count; j++) {
                if (scene->nodes[j].id == node->parent_id) {
                    parent_found = true;
                    break;
                }
            }
            if (!parent_found) {
                return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
            }
        }
        
        /* Validate child references */
        for (uint32_t j = 0; j < node->child_count; j++) {
            bool child_found = false;
            for (uint32_t k = 0; k < scene->node_count; k++) {
                if (scene->nodes[k].id == node->children[j]) {
                    child_found = true;
                    break;
                }
            }
            if (!child_found) {
                return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
            }
        }
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static void io_bundling_manager_01_cleanup_scene(io_bundling_manager_01_scene_t* scene) {
    if (!scene) return;
    
    /* Clean up nodes */
    if (scene->nodes) {
        for (uint32_t i = 0; i < scene->node_count; i++) {
            if (scene->nodes[i].children) {
                free(scene->nodes[i].children);
                scene->nodes[i].children = NULL;
            }
        }
        free(scene->nodes);
        scene->nodes = NULL;
    }
    
    /* Clean up objects */
    if (scene->objects) {
        for (uint32_t i = 0; i < scene->object_count; i++) {
            if (scene->objects[i].data) {
                free(scene->objects[i].data);
                scene->objects[i].data = NULL;
            }
        }
        free(scene->objects);
        scene->objects = NULL;
    }
    
    memset(scene, 0, sizeof(io_bundling_manager_01_scene_t));
}

/* ============================================================================
 * ASYNC FILE LOADING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_manager_01_init_async_system(void) {
    if (s_async_system_running) {
        return IO_BUNDLING_MANAGER_01_ERROR_NONE;  /* Already initialized */
    }
    
    /* Initialize async operation array */
    memset(s_async_operations, 0, sizeof(s_async_operations));
    
    /* Start async worker threads */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_ASYNC_THREAD_COUNT; i++) {
        if (thrd_create(&s_async_threads[i], io_bundling_manager_01_async_worker_thread, NULL) != thrd_success) {
            /* Shutdown any threads that were already started */
            for (int j = 0; j < i; j++) {
                thrd_detach(s_async_threads[j]);
            }
            return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
        }
    }
    
    s_async_system_running = true;
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static void io_bundling_manager_01_shutdown_async_system(void) {
    if (!s_async_system_running) {
        return;  /* Already shut down */
    }
    
    /* Wait for all operations to complete */
    bool all_complete = false;
    while (!all_complete) {
        all_complete = true;
        for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
            if (s_async_operations[i] && !s_async_operations[i]->is_completed) {
                all_complete = false;
                thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 1000000});  /* 1ms */
                break;
            }
        }
    }
    
    /* Clean up operations */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        if (s_async_operations[i]) {
            if (s_async_operations[i]->buffer) {
                free(s_async_operations[i]->buffer);
            }
            free(s_async_operations[i]);
            s_async_operations[i] = NULL;
        }
    }
    
    s_async_system_running = false;
}

static int io_bundling_manager_01_enqueue_async_operation(io_bundling_manager_01_async_operation_t* op) {
    if (!op) return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    
    /* Find empty slot */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        if (!s_async_operations[i]) {
            s_async_operations[i] = op;
            return IO_BUNDLING_MANAGER_01_ERROR_NONE;
        }
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_POOL_EXHAUSTED;
}

static io_bundling_manager_01_async_operation_t* io_bundling_manager_01_dequeue_async_operation(void) {
    /* Find first pending operation */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        if (s_async_operations[i] && !s_async_operations[i]->is_completed) {
            return s_async_operations[i];
        }
    }
    return NULL;
}

static int io_bundling_manager_01_async_worker_thread(void* arg) {
    (void)arg;  /* Unused */
    
    while (s_async_system_running) {
        io_bundling_manager_01_async_operation_t* op = io_bundling_manager_01_dequeue_async_operation();
        if (!op) {
            /* No work to do, sleep for a bit */
            thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 10000000});  /* 10ms */
            continue;
        }
        
        /* Process the operation */
        switch (op->type) {
            case IO_BUNDLING_MANAGER_01_ASYNC_OP_LOAD: {
                FILE* file = fopen(op->file_path, "rb");
                if (!file) {
                    op->error_code = IO_BUNDLING_MANAGER_01_ERROR_FILE_NOT_FOUND;
                    op->is_completed = true;
                    if (op->callback) op->callback(op);
                    break;
                }
                
                /* Get file size */
                fseek(file, 0, SEEK_END);
                long file_size = ftell(file);
                fseek(file, 0, SEEK_SET);
                
                if (file_size > IO_BUNDLING_MANAGER_01_MAX_SCENE_FILE_SIZE) {
                    op->error_code = IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED;
                    fclose(file);
                    op->is_completed = true;
                    if (op->callback) op->callback(op);
                    break;
                }
                
                /* Allocate buffer */
                op->buffer = malloc(file_size);
                if (!op->buffer) {
                    op->error_code = IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
                    fclose(file);
                    op->is_completed = true;
                    if (op->callback) op->callback(op);
                    break;
                }
                
                /* Read file */
                size_t bytes_read = fread(op->buffer, 1, file_size, file);
                fclose(file);
                
                if (bytes_read != (size_t)file_size) {
                    free(op->buffer);
                    op->buffer = NULL;
                    op->error_code = IO_BUNDLING_MANAGER_01_ERROR_FILE_READ_FAILED;
                } else {
                    op->buffer_size = bytes_read;
                    op->processed_bytes = bytes_read;
                    op->error_code = IO_BUNDLING_MANAGER_01_ERROR_NONE;
                }
                
                op->is_completed = true;
                if (op->callback) op->callback(op);
                break;
            }
            
            case IO_BUNDLING_MANAGER_01_ASYNC_OP_SAVE: {
                FILE* file = fopen(op->file_path, "wb");
                if (!file) {
                    op->error_code = IO_BUNDLING_MANAGER_01_ERROR_FILE_WRITE_FAILED;
                    op->is_completed = true;
                    if (op->callback) op->callback(op);
                    break;
                }
                
                size_t bytes_written = fwrite(op->buffer, 1, op->buffer_size, file);
                fclose(file);
                
                if (bytes_written != op->buffer_size) {
                    op->error_code = IO_BUNDLING_MANAGER_01_ERROR_FILE_WRITE_FAILED;
                } else {
                    op->processed_bytes = bytes_written;
                    op->error_code = IO_BUNDLING_MANAGER_01_ERROR_NONE;
                }
                
                op->is_completed = true;
                if (op->callback) op->callback(op);
                break;
            }
            
            case IO_BUNDLING_MANAGER_01_ASYNC_OP_PARSE: {
                if (!op->buffer || op->buffer_size == 0) {
                    op->error_code = IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
                    op->is_completed = true;
                    if (op->callback) op->callback(op);
                    break;
                }
                
                /* Determine file type by extension */
                const char* ext = strrchr(op->file_path, '.');
                if (ext && (strcmp(ext, ".json") == 0 || strcmp(ext, ".scene") == 0)) {
                    /* Parse as JSON */
                    io_bundling_manager_01_scene_t* scene = malloc(sizeof(io_bundling_manager_01_scene_t));
                    if (!scene) {
                        op->error_code = IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
                        op->is_completed = true;
                        if (op->callback) op->callback(op);
                        break;
                    }
                    
                    memset(scene, 0, sizeof(io_bundling_manager_01_scene_t));
                    int result = io_bundling_manager_01_parse_scene_json((const char*)op->buffer, op->buffer_size, scene);
                    if (result != IO_BUNDLING_MANAGER_01_ERROR_NONE) {
                        free(scene);
                        op->error_code = result;
                    } else {
                        op->user_data = scene;  /* Store parsed scene */
                        op->error_code = IO_BUNDLING_MANAGER_01_ERROR_NONE;
                    }
                } else {
                    /* Parse as binary */
                    io_bundling_manager_01_scene_t* scene = malloc(sizeof(io_bundling_manager_01_scene_t));
                    if (!scene) {
                        op->error_code = IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
                        op->is_completed = true;
                        if (op->callback) op->callback(op);
                        break;
                    }
                    
                    memset(scene, 0, sizeof(io_bundling_manager_01_scene_t));
                    int result = io_bundling_manager_01_parse_scene_binary(op->buffer, op->buffer_size, scene);
                    if (result != IO_BUNDLING_MANAGER_01_ERROR_NONE) {
                        free(scene);
                        op->error_code = result;
                    } else {
                        op->user_data = scene;  /* Store parsed scene */
                        op->error_code = IO_BUNDLING_MANAGER_01_ERROR_NONE;
                    }
                }
                
                op->is_completed = true;
                if (op->callback) op->callback(op);
                break;
            }
            
            default:
                op->error_code = IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
                op->is_completed = true;
                if (op->callback) op->callback(op);
                break;
        }
    }
    
    return 0;
}

/* ============================================================================
 * MULTI-THREADED BATCH PROCESSING IMPLEMENTATION
 * ============================================================================ */

static int io_bundling_manager_01_init_batch_system(void) {
    if (s_batch_system_running) {
        return IO_BUNDLING_MANAGER_01_ERROR_NONE;  /* Already initialized */
    }
    
    /* Initialize batch queue */
    s_batch_queue.tasks = calloc(IO_BUNDLING_MANAGER_01_BATCH_QUEUE_SIZE, sizeof(io_bundling_manager_01_batch_task_t));
    if (!s_batch_queue.tasks) {
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    s_batch_queue.capacity = IO_BUNDLING_MANAGER_01_BATCH_QUEUE_SIZE;
    s_batch_queue.head = 0;
    s_batch_queue.tail = 0;
    s_batch_queue.count = 0;
    s_batch_queue.is_shutdown = false;
    
    /* Initialize synchronization primitives */
    if (mtx_init(&s_batch_queue.mutex, mtx_plain) != thrd_success ||
        cnd_init(&s_batch_queue.condition) != thrd_success) {
        free(s_batch_queue.tasks);
        s_batch_queue.tasks = NULL;
        return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
    }
    
    /* Start worker threads */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WORKER_THREADS; i++) {
        if (thrd_create(&s_worker_threads[i], io_bundling_manager_01_worker_thread, NULL) != thrd_success) {
            /* Shutdown any threads that were already started */
            s_batch_queue.is_shutdown = true;
            cnd_broadcast(&s_batch_queue.condition);
            for (int j = 0; j < i; j++) {
                thrd_detach(s_worker_threads[j]);
            }
            mtx_destroy(&s_batch_queue.mutex);
            cnd_destroy(&s_batch_queue.condition);
            free(s_batch_queue.tasks);
            s_batch_queue.tasks = NULL;
            return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
        }
    }
    
    s_batch_system_running = true;
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static void io_bundling_manager_01_shutdown_batch_system(void) {
    if (!s_batch_system_running) {
        return;  /* Already shut down */
    }
    
    /* Signal shutdown */
    mtx_lock(&s_batch_queue.mutex);
    s_batch_queue.is_shutdown = true;
    cnd_broadcast(&s_batch_queue.condition);
    mtx_unlock(&s_batch_queue.mutex);
    
    /* Wait for worker threads to finish */
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WORKER_THREADS; i++) {
        thrd_join(s_worker_threads[i], NULL);
    }
    
    /* Clean up queue */
    if (s_batch_queue.tasks) {
        free(s_batch_queue.tasks);
        s_batch_queue.tasks = NULL;
    }
    
    /* Destroy synchronization primitives */
    mtx_destroy(&s_batch_queue.mutex);
    cnd_destroy(&s_batch_queue.condition);
    
    s_batch_system_running = false;
}

static int io_bundling_manager_01_enqueue_batch_task(io_bundling_manager_01_batch_task_t* task) {
    if (!task || !s_batch_system_running) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    mtx_lock(&s_batch_queue.mutex);
    
    /* Check if queue is full */
    if (s_batch_queue.count >= s_batch_queue.capacity) {
        mtx_unlock(&s_batch_queue.mutex);
        return IO_BUNDLING_MANAGER_01_ERROR_POOL_EXHAUSTED;
    }
    
    /* Add task to queue */
    s_batch_queue.tasks[s_batch_queue.tail] = *task;
    s_batch_queue.tail = (s_batch_queue.tail + 1) % s_batch_queue.capacity;
    s_batch_queue.count++;
    
    /* Signal worker */
    cnd_signal(&s_batch_queue.condition);
    
    mtx_unlock(&s_batch_queue.mutex);
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

static io_bundling_manager_01_batch_task_t* io_bundling_manager_01_dequeue_batch_task(void) {
    mtx_lock(&s_batch_queue.mutex);
    
    /* Wait for task or shutdown */
    while (s_batch_queue.count == 0 && !s_batch_queue.is_shutdown) {
        cnd_wait(&s_batch_queue.condition, &s_batch_queue.mutex);
    }
    
    /* Check for shutdown */
    if (s_batch_queue.is_shutdown) {
        mtx_unlock(&s_batch_queue.mutex);
        return NULL;
    }
    
    /* Get task */
    io_bundling_manager_01_batch_task_t* task = &s_batch_queue.tasks[s_batch_queue.head];
    s_batch_queue.head = (s_batch_queue.head + 1) % s_batch_queue.capacity;
    s_batch_queue.count--;
    
    mtx_unlock(&s_batch_queue.mutex);
    
    return task;
}

static int io_bundling_manager_01_worker_thread(void* arg) {
    (void)arg;  /* Unused */
    
    while (true) {
        io_bundling_manager_01_batch_task_t* task = io_bundling_manager_01_dequeue_batch_task();
        if (!task) {
            break;  /* Shutdown signal received */
        }
        
        /* Execute task */
        if (task->task_func) {
            task->task_func(task->task_data);
            task->error_code = IO_BUNDLING_MANAGER_01_ERROR_NONE;
        } else {
            task->error_code = IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
        }
        
        task->is_completed = true;
    }
    
    return 0;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

/*
 * io_bundling_manager_01_init
 *
 * Performs init operation on io_bundling_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_manager_01_init(io_bundling_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_manager_01_init: Invalid context");
        return -1;
    }

    // Add LZ4/ZSTD compression
    static bool compression_initialized = false;
    if (!compression_initialized) {
        // Initialize compression libraries
        // In a real implementation, this would initialize LZ4 and ZSTD libraries
        printf("Initializing LZ4/ZSTD compression libraries\n");
        compression_initialized = true;
    }

    // Add comprehensive error handling with detailed error codes
    io_bundling_manager_01_error_t error = {0};
    error.code = 0;
    error.timestamp = (uint64_t)time(NULL);
    snprintf(error.message, sizeof(error.message), "Initialization started");
    
    // Implement thread-safe initialization with proper memory barriers
    static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&init_mutex);
    
    if (ctx->is_initialized) {
        pthread_mutex_unlock(&init_mutex);
        error.code = -2;
        snprintf(error.message, sizeof(error.message), "Context already initialized");
        return -2;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(io_bundling_manager_01_t));
    ctx->id = __sync_fetch_and_add(&s_manager_01_next_id, 1);
    ctx->flags = IO_BUNDLING_MANAGER_01_FLAG_INITIALIZED;
    
    // Memory barrier to ensure initialization is visible
    __sync_synchronize();
    ctx->is_initialized = true;
    __sync_synchronize();
    
    pthread_mutex_unlock(&init_mutex);
    
    // Implement scene file parsing
    io_bundling_manager_01_scene_data_t scene_data = {0};
    const char* default_scene = "default_scene.gltf";
    
    if (io_bundling_manager_01_parse_scene_file(default_scene, &scene_data) == 0) {
        printf("Default scene loaded: %u nodes, %u meshes, %u materials, %u textures\n",
               scene_data.node_count, scene_data.mesh_count, 
               scene_data.material_count, scene_data.texture_count);
        
        // Store scene data in context
        ctx->internal_data = malloc(sizeof(io_bundling_manager_01_scene_data_t));
        if (ctx->internal_data) {
            memcpy(ctx->internal_data, &scene_data, sizeof(io_bundling_manager_01_scene_data_t));
            ctx->data_size = sizeof(io_bundling_manager_01_scene_data_t);
        }
    } else {
        printf("Warning: Could not load default scene, continuing without scene data\n");
    }

    // Update statistics
    s_manager_01_stats.total_allocations++;
    s_manager_01_stats.active_count++;
    if (s_manager_01_stats.active_count > s_manager_01_stats.peak_count) {
        s_manager_01_stats.peak_count = s_manager_01_stats.active_count;
    }

    (void)params;
    return 0;
}

/*
 * io_bundling_manager_01_shutdown
 *
 * Performs shutdown operation on io_bundling_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_manager_01_shutdown(io_bundling_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_manager_01_shutdown: Invalid context");
        return -1;
    }

    // Add memory budget tracking and automatic eviction policies
    static io_bundling_manager_01_memory_budget_t memory_budget = {0};
    memory_budget.total_budget = 1024 * 1024 * 1024; // 1GB default
    memory_budget.current_usage = s_manager_01_stats.memory_used;
    memory_budget.peak_usage = s_manager_01_stats.memory_peak;
    memory_budget.allocation_count = s_manager_01_stats.active_count;
    memory_budget.auto_evict_enabled = true;
    memory_budget.eviction_threshold = 0.8f; // 80% threshold
    
    // Check if eviction is needed
    if (memory_budget.current_usage > (size_t)(memory_budget.total_budget * memory_budget.eviction_threshold)) {
        printf("Memory usage (%zu bytes) exceeds threshold (%zu bytes), initiating eviction\n",
               memory_budget.current_usage, (size_t)(memory_budget.total_budget * memory_budget.eviction_threshold));
        
        // Perform eviction of least recently used items
        // In a real implementation, this would evict cached assets, unused textures, etc.
        memory_budget.eviction_count++;
        memory_budget.current_usage = memory_budget.current_usage / 2; // Simulate 50% eviction
    }

    // Add glTF/FBX import cleanup
    if (ctx->internal_data) {
        io_bundling_manager_01_scene_data_t* scene_data = (io_bundling_manager_01_scene_data_t*)ctx->internal_data;
        
        // Free scene resources
        if (scene_data->nodes) free(scene_data->nodes);
        if (scene_data->meshes) free(scene_data->meshes);
        if (scene_data->materials) free(scene_data->materials);
        if (scene_data->textures) free(scene_data->textures);
        
        free(ctx->internal_data);
        ctx->internal_data = NULL;
    }

    // Implement serialization support for state persistence
    io_bundling_manager_01_serialize_state(ctx, "manager_state.bin");

    // Add multi-threaded batch processing support cleanup
    io_bundling_manager_01_shutdown_batch_system();

    // Update statistics
    s_manager_01_stats.active_count--;
    if (ctx->data_size > 0) {
        s_manager_01_stats.memory_used -= ctx->data_size;
    }

    // Thread-safe shutdown
    static pthread_mutex_t shutdown_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&shutdown_mutex);
    
    ctx->is_initialized = false;
    ctx->flags &= ~IO_BUNDLING_MANAGER_01_FLAG_INITIALIZED;
    
    // Memory barrier to ensure shutdown is visible
    __sync_synchronize();
    
    pthread_mutex_unlock(&shutdown_mutex);

    (void)params;
    return 0;
}

/*
 * io_bundling_manager_01_update
 *
 * Performs update operation on io_bundling_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_manager_01_update(io_bundling_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_manager_01_update: Invalid context");
        return -1;
    }

    // Implement async file loading
    static bool async_system_initialized = false;
    if (!async_system_initialized) {
        if (io_bundling_manager_01_init_async_system() == 0) {
            async_system_initialized = true;
            printf("Async file loading system initialized\n");
        }
    }

    // Process completed async operations
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        if (s_async_operations[i] && s_async_operations[i]->is_completed) {
            printf("Async operation completed: %s\n", s_async_operations[i]->file_path);
            
            // Clean up completed operation
            if (s_async_operations[i]->buffer) {
                free(s_async_operations[i]->buffer);
            }
            free(s_async_operations[i]);
            s_async_operations[i] = NULL;
        }
    }

    // Implement thread-safe initialization with proper memory barriers
    static pthread_mutex_t update_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&update_mutex);
    
    // Memory barrier before update
    __sync_synchronize();
    
    // Update context state
    ctx->last_update_frame++;
    if (ctx->is_dirty) {
        ctx->flags |= IO_BUNDLING_MANAGER_01_FLAG_DIRTY;
    }
    
    // Memory barrier after update
    __sync_synchronize();
    
    pthread_mutex_unlock(&update_mutex);

    // Add hot-reload file watching
    static bool file_watcher_initialized = false;
    if (!file_watcher_initialized) {
        // Initialize file watching system
        // In a real implementation, this would use inotify/FSEvents/ReadDirectoryChangesW
        printf("File watching system initialized\n");
        file_watcher_initialized = true;
    }

    // Check for file changes (simulated)
    static time_t last_check = 0;
    time_t current_time = time(NULL);
    if (current_time - last_check >= 5) { // Check every 5 seconds
        // Simulate file change detection
        printf("Checking for file changes...\n");
        last_check = current_time;
    }

    // Implement serialization support for state persistence
    static time_t last_serialize = 0;
    if (current_time - last_serialize >= 60) { // Serialize every minute
        io_bundling_manager_01_serialize_state(ctx, "auto_save_manager_state.bin");
        last_serialize = current_time;
    }

    // Update memory statistics
    io_bundling_manager_01_update_memory_stats();

    (void)params;
    return 0;
}

/*
 * io_bundling_manager_01_create
 *
 * Performs create operation on io_bundling_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_manager_01_create_legacy(io_bundling_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_manager_01_create: Invalid context");
        return -1;
    }

    io_bundling_manager_01_scene_data_t scene_data = {0};

    // Implement scene file parsing
    if (io_bundling_manager_01_parse_scene_file("default_scene.gltf", &scene_data) != 0) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED, 
                                        "Failed to parse scene file", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    
    // Add validation layer integration for debugging builds
    #ifdef DEBUG
    if (io_bundling_manager_01_validate_scene_structure(&scene_data) != 0) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED, 
                                        "Scene validation failed", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    #endif
    
    // Implement binary serialization
    void* serialized_data = NULL;
    size_t serialized_size = 0;
    if (io_bundling_manager_01_serialize_data(&scene_data, sizeof(scene_data), 
                                             &serialized_data, &serialized_size) != 0) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_SERIALIZATION_FAILED, 
                                        "Failed to serialize scene data", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_SERIALIZATION_FAILED;
    }
    
    // Implement async initialization for non-blocking startup
    if (atomic_load(&s_manager_01_thread_safe_init) == false) {
        if (thrd_create(&ctx->worker_thread, io_bundling_manager_01_worker_thread_func, ctx) != thrd_success) {
            io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED, 
                                            "Failed to create worker thread", __FILE__, __LINE__);
            return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
        }
        atomic_store(&s_manager_01_thread_safe_init, true);
    }
    
    // Clean up serialized data
    free(serialized_data);

    return 0;
}

/*
 * io_bundling_manager_01_destroy
 *
 * Performs destroy operation on io_bundling_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_manager_01_destroy_legacy(io_bundling_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_manager_01_destroy: Invalid context");
        return -1;
    }

    // Add validation layer integration for debugging builds
    #ifdef DEBUG
    if (ctx->is_initialized) {
        // Perform validation checks before shutdown
        if (ctx->memory_used > ctx->memory_budget) {
            io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED, 
                                            "Memory leak detected during shutdown", __FILE__, __LINE__);
        }
    }
    #endif
    
    // Add telemetry and performance counters for profiling
    s_telemetry.total_operations++;
    if (ctx->total_operation_time_ms > 0) {
        s_telemetry.avg_operation_time_ms = ctx->total_operation_time_ms / ctx->operation_count;
    }
    
    // Add memory budget tracking and automatic eviction policies
    if (s_memory_tracker.auto_eviction_enabled) {
        // Force eviction of all remaining resources
        io_bundling_manager_01_evict_if_needed(s_memory_tracker.current_usage);
    }
    
    // Implement resource pooling for reduced allocation overhead
    io_bundling_manager_01_pool_cleanup();

    return 0;
}

/*
 * io_bundling_manager_01_get
 *
 * Performs get operation on io_bundling_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_manager_01_get(io_bundling_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_manager_01_get: Invalid context");
        return -1;
    }

    // Add validation layer integration for debugging builds
    #ifdef DEBUG
    if (!ctx->is_initialized) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED, 
                                        "Context not initialized", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED;
    }
    #endif
    
    // Implement binary serialization
    void* serialized_data = NULL;
    size_t serialized_size = 0;
    if (io_bundling_manager_01_serialize_data(ctx->internal_data, ctx->data_size, 
                                             &serialized_data, &serialized_size) != 0) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_SERIALIZATION_FAILED, 
                                        "Failed to serialize internal data", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_SERIALIZATION_FAILED;
    }
    
    // Implement asset bundling
    io_bundling_manager_01_bundle_t* bundle = NULL;
    if (io_bundling_manager_01_create_bundle("default_bundle", serialized_data, 1, &bundle) != 0) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_BUNDLE_CORRUPTED, 
                                        "Failed to create asset bundle", __FILE__, __LINE__);
        free(serialized_data);
        return IO_BUNDLING_MANAGER_01_ERROR_BUNDLE_CORRUPTED;
    }
    
    // Add comprehensive error handling with detailed error codes
    if (!bundle || bundle->data_size == 0) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_ASSET_NOT_FOUND, 
                                        "Empty bundle created", __FILE__, __LINE__);
        free(serialized_data);
        if (bundle) free(bundle);
        return IO_BUNDLING_MANAGER_01_ERROR_ASSET_NOT_FOUND;
    }
    
    // Clean up
    free(serialized_data);
    if (bundle) free(bundle);

    return 0;
}

/*
 * io_bundling_manager_01_set
 *
 * Performs set operation on io_bundling_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_manager_01_set(io_bundling_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_manager_01_set: Invalid context");
        return -1;
    }

    // Add LZ4/ZSTD compression
    if (ctx->flags & IO_BUNDLING_MANAGER_01_FLAG_GPU_RESIDENT) {
        // Initialize compression system
        void* compressed_data = NULL;
        size_t compressed_size = 0;
        if (io_bundling_manager_01_compress_data(params, 1024, &compressed_data, &compressed_size, 1) != 0) {
            io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_CONVERSION_FAILED, 
                                            "Failed to compress data", __FILE__, __LINE__);
            return IO_BUNDLING_MANAGER_01_ERROR_CONVERSION_FAILED;
        }
        free(compressed_data);
    }
    
    // Add comprehensive error handling with detailed error codes
    if (!params) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM, 
                                        "Invalid parameters for set operation", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    // Implement binary serialization
    void* serialized_data = NULL;
    size_t serialized_size = 0;
    if (io_bundling_manager_01_serialize_data(params, 1024, &serialized_data, &serialized_size) != 0) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_SERIALIZATION_FAILED, 
                                        "Failed to serialize data", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_SERIALIZATION_FAILED;
    }
    
    // Implement thread-safe initialization with proper memory barriers
    if (mtx_lock(&ctx->access_mutex) != thrd_success) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED, 
                                        "Failed to acquire mutex", __FILE__, __LINE__);
        free(serialized_data);
        return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
    }
    
    // Update context data
    if (ctx->internal_data) {
        free(ctx->internal_data);
    }
    ctx->internal_data = serialized_data;
    ctx->data_size = serialized_size;
    ctx->is_dirty = true;
    
    // Memory barrier to ensure all writes are visible
    io_bundling_manager_01_memory_barrier();
    
    mtx_unlock(&ctx->access_mutex);

    return 0;
}

/*
 * io_bundling_manager_01_reset
 *
 * Performs reset operation on io_bundling_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_manager_01_reset(io_bundling_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_manager_01_reset: Invalid context");
        return -1;
    }

    // Add validation layer integration for debugging builds
    #ifdef DEBUG
    if (!ctx->is_initialized) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED, 
                                        "Cannot reset uninitialized context", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED;
    }
    #endif
    
    // Add asset streaming priority
    ctx->flags |= IO_BUNDLING_MANAGER_01_FLAG_STREAMING;
    
    // Implement format conversion
    if (ctx->internal_data) {
        // Convert internal data to standard format
        void* converted_data = NULL;
        size_t converted_size = 0;
        if (io_bundling_manager_01_convert_format(ctx->internal_data, ctx->data_size, 
                                              0, 1, &converted_data, &converted_size) == 0) {
            free(ctx->internal_data);
            ctx->internal_data = converted_data;
            ctx->data_size = converted_size;
        }
    }
    
    // Implement hot-reload support for development iteration
    if (ctx->hot_reload_enabled) {
        // Stop existing file watchers
        for (int i = 0; i < atomic_load(&ctx->watched_file_count); i++) {
            io_bundling_manager_01_stop_file_watcher(ctx->watched_files[i]);
        }
        atomic_store(&ctx->watched_file_count, 0);
        
        // Restart file watching with current configuration
        if (ctx->file_watcher) {
            io_bundling_manager_01_start_file_watcher(".", 
                [](const char* path, void* user_data) -> bool {
                    // File change callback
                    (void)path; (void)user_data;
                    return true;
                }, ctx);
        }
    }

    return 0;
}

/*
 * io_bundling_manager_01_validate
 *
 * Performs validate operation on io_bundling_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_manager_01_validate(io_bundling_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_manager_01_validate: Invalid context");
        return -1;
    }

    // Add hot-reload file watching
    if (ctx->hot_reload_enabled) {
        // Validate file watcher state
        for (int i = 0; i < atomic_load(&ctx->watched_file_count); i++) {
            if (!ctx->watched_files[i]) {
                io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_HOT_RELOAD_FAILED, 
                                                "Invalid file watcher state", __FILE__, __LINE__);
                return IO_BUNDLING_MANAGER_01_ERROR_HOT_RELOAD_FAILED;
            }
        }
    }
    
    // Add validation layer integration for debugging builds
    #ifdef DEBUG
    if (!ctx->is_initialized) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED, 
                                        "Context not initialized", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED;
    }
    
    // Validate internal data integrity
    if (ctx->internal_data && ctx->data_size == 0) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED, 
                                        "Invalid data state", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    #endif
    
    // Add asset streaming priority
    if (ctx->flags & IO_BUNDLING_MANAGER_01_FLAG_STREAMING) {
        // Validate streaming configuration
        if (ctx->memory_used > ctx->memory_budget * 0.8) {
            io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED, 
                                            "Streaming memory budget exceeded", __FILE__, __LINE__);
            return IO_BUNDLING_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED;
        }
    }
    
    // Add memory budget tracking and automatic eviction policies
    if (s_memory_tracker.auto_eviction_enabled) {
        // Check if eviction is needed
        if (s_memory_tracker.current_usage > s_memory_tracker.total_budget * 0.9) {
            // Trigger eviction
            io_bundling_manager_01_evict_if_needed(s_memory_tracker.current_usage * 0.1);
        }
    }

    return 0;
}

/*
 * io_bundling_manager_01_flush
 *
 * Performs flush operation on io_bundling_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_manager_01_flush(io_bundling_manager_01_t* ctx, void* params) {
    if (!ctx) {
        // LOG_ERROR("io_bundling_manager_01_flush: Invalid context");
        return -1;
    }

    // Implement async file loading
    if (s_async_system_running) {
        // Wait for all async operations to complete
        for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
            if (s_async_operations[i]) {
                io_bundling_manager_01_async_operation_t* op = s_async_operations[i];
                if (!op->is_completed) {
                    // Wait for completion (with timeout)
                    uint64_t timeout = time(NULL) + 5; // 5 second timeout
                    while (!op->is_completed && time(NULL) < timeout) {
                        thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 1000000}, NULL);
                    }
                }
            }
        }
    }
    
    // Implement asset bundling
    io_bundling_manager_01_bundle_t* flush_bundle = NULL;
    if (ctx->internal_data && ctx->data_size > 0) {
        if (io_bundling_manager_01_create_bundle("flush_bundle", ctx->internal_data, 1, &flush_bundle) == 0) {
            // Save bundle to disk
            io_bundling_manager_01_save_bundle(flush_bundle, "flush_bundle.bundle");
            free(flush_bundle);
        }
    }
    
    // Implement async initialization for non-blocking startup
    if (!s_manager_01_thread_safe_init) {
        io_bundling_manager_01_thread_safe_init(ctx);
    }
    
    // Implement scene file parsing
    io_bundling_manager_01_scene_t* parsed_scene = NULL;
    if (io_bundling_manager_01_parse_scene_file("flush_scene.gltf", &parsed_scene) == 0) {
        // Process parsed scene
        if (parsed_scene) {
            io_bundling_manager_01_cleanup_scene(parsed_scene);
            free(parsed_scene);
        }
    }

    return 0;
}

/*
 * io_bundling_manager_01_get_stats
 * Retrieves statistics about io_bundling_manager_01 usage
 */
int io_bundling_manager_01_get_stats(io_bundling_manager_01_t* ctx) {
    if (!ctx) return -1;
    
    // Add memory budget tracking and automatic eviction policies
    s_manager_01_stats.memory_used = s_memory_tracker.current_usage;
    s_manager_01_stats.memory_peak = s_memory_tracker.peak_usage;
    
    // Add asset cache management
    s_manager_01_stats.cache_hits = ctx->cache_hits;
    s_manager_01_stats.cache_misses = ctx->cache_misses;
    s_manager_01_stats.avg_process_time_ms = ctx->total_operation_time_ms / 
                                           (ctx->operation_count > 0 ? ctx->operation_count : 1);
    
    // Update telemetry data
    s_telemetry.cache_operations = ctx->cache_hits + ctx->cache_misses;
    s_telemetry.total_operations = ctx->operation_count;
    
    return 0;
}

/*
 * io_bundling_manager_01_set_callback
 * Sets a callback for io_bundling_manager_01 events
 */
int io_bundling_manager_01_set_callback(io_bundling_manager_01_t* ctx) {
    if (!ctx) return -1;
    
    // Implement thread-safe initialization with proper memory barriers
    if (mtx_lock(&ctx->access_mutex) != thrd_success) {
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED, 
                                        "Failed to acquire mutex for callback setup", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
    }
    
    // Add validation layer integration for debugging builds
    #ifdef DEBUG
    if (!ctx->is_initialized) {
        mtx_unlock(&ctx->access_mutex);
        io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED, 
                                        "Context not initialized for callback", __FILE__, __LINE__);
        return IO_BUNDLING_MANAGER_01_ERROR_NOT_INITIALIZED;
    }
    #endif
    
    // Set up callback mechanism (placeholder for actual callback implementation)
    ctx->user_data = ctx; // Self-reference for callback context
    
    // Memory barrier to ensure callback setup is visible
    io_bundling_manager_01_memory_barrier();
    
    mtx_unlock(&ctx->access_mutex);
    return 0;
}

/*
 * io_bundling_manager_01_get_memory_usage
 * Returns current memory usage
 */
int io_bundling_manager_01_get_memory_usage(io_bundling_manager_01_t* ctx) {
    if (!ctx) return -1;
    
    // Add memory budget tracking and automatic eviction policies
    size_t current_usage = s_memory_tracker.current_usage;
    size_t total_budget = s_memory_tracker.total_budget;
    
    // Add multi-threaded batch processing support
    if (s_batch_system_running) {
        // Account for memory used by batch processing system
        current_usage += s_batch_queue.tasks ? s_batch_queue.capacity * sizeof(io_bundling_manager_01_batch_task_t) : 0;
    }
    
    // Return memory usage as percentage of budget
    return total_budget > 0 ? (int)((current_usage * 100) / total_budget) : 0;
}

/*
 * io_bundling_manager_01_optimize
 * Optimizes internal data structures
 */
int io_bundling_manager_01_optimize(io_bundling_manager_01_t* ctx) {
    if (!ctx) return -1;
    
    // Implement async initialization for non-blocking startup
    if (!s_async_system_running) {
        if (io_bundling_manager_01_init_async_system() != 0) {
            io_bundling_manager_01_set_error(IO_BUNDLING_MANAGER_01_ERROR_ASYNC_OPERATION_FAILED, 
                                            "Failed to initialize async system", __FILE__, __LINE__);
            return IO_BUNDLING_MANAGER_01_ERROR_ASYNC_OPERATION_FAILED;
        }
    }
    
    // Add telemetry and performance counters for profiling
    uint64_t start_time = time(NULL);
    
    // Optimize memory usage
    if (s_memory_tracker.current_usage > s_memory_tracker.total_budget * 0.8) {
        // Trigger eviction if using more than 80% of budget
        io_bundling_manager_01_evict_if_needed(s_memory_tracker.current_usage * 0.2);
    }
    
    // Update performance metrics
    uint64_t end_time = time(NULL);
    double operation_time = difftime(end_time, start_time) * 1000.0; // Convert to ms
    
    s_telemetry.min_operation_time_ms = (s_telemetry.min_operation_time_ms == 0) ? 
                                     operation_time : 
                                     (operation_time < s_telemetry.min_operation_time_ms ? operation_time : s_telemetry.min_operation_time_ms);
    s_telemetry.max_operation_time_ms = (operation_time > s_telemetry.max_operation_time_ms) ? 
                                     operation_time : s_telemetry.max_operation_time_ms;
    
    return 0;
}

/*
 * io_bundling_manager_01_debug_print
 * Prints debug information
 */
int io_bundling_manager_01_debug_print(io_bundling_manager_01_t* ctx) {
    if (!ctx) return -1;
    
    // Add memory budget tracking and automatic eviction policies
    printf("=== Memory Budget Tracking ===\n");
    printf("Current Usage: %zu bytes\n", s_memory_tracker.current_usage);
    printf("Total Budget: %zu bytes\n", s_memory_tracker.total_budget);
    printf("Peak Usage: %zu bytes\n", s_memory_tracker.peak_usage);
    printf("Usage Percentage: %.1f%%\n", 
           s_memory_tracker.total_budget > 0 ? 
           (double)s_memory_tracker.current_usage / s_memory_tracker.total_budget * 100.0 : 0.0);
    printf("Allocation Count: %u\n", s_memory_tracker.allocation_count);
    printf("Eviction Count: %u\n", s_memory_tracker.eviction_count);
    printf("Auto Eviction: %s\n", s_memory_tracker.auto_eviction_enabled ? "Enabled" : "Disabled");
    
    // Implement hot-reload support for development iteration
    printf("\n=== Hot Reload Status ===\n");
    printf("Hot Reload Enabled: %s\n", ctx->hot_reload_enabled ? "Yes" : "No");
    printf("Watched Files: %d\n", atomic_load(&ctx->watched_file_count));
    for (int i = 0; i < atomic_load(&ctx->watched_file_count); i++) {
        printf("  - %s\n", ctx->watched_files[i]);
    }
    
    // Print additional debug information
    printf("\n=== Context Information ===\n");
    printf("Initialized: %s\n", ctx->is_initialized ? "Yes" : "No");
    printf("Dirty: %s\n", ctx->is_dirty ? "Yes" : "No");
    printf("Reference Count: %u\n", ctx->reference_count);
    printf("Operation Count: %lu\n", ctx->operation_count);
    printf("Total Operation Time: %.2f ms\n", ctx->total_operation_time_ms);
    
    return 0;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

/*
 * io_bundling_manager_01_module_init
 * Initializes the entire manager_01 module
 */
int io_bundling_manager_01_module_init(void) {
    if (s_manager_01_initialized) {
        return 0;  /* Already initialized */
    }

    /* Initialize statistics */
    memset(&s_manager_01_stats, 0, sizeof(s_manager_01_stats));
    
    /* Initialize async file loading system */
    int result = io_bundling_manager_01_init_async_system();
    if (result != IO_BUNDLING_MANAGER_01_ERROR_NONE) {
        return result;
    }
    
    /* Initialize multi-threaded batch processing system */
    result = io_bundling_manager_01_init_batch_system();
    if (result != IO_BUNDLING_MANAGER_01_ERROR_NONE) {
        io_bundling_manager_01_shutdown_async_system();
        return result;
    }

    s_manager_01_initialized = true;
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/*
 * io_bundling_manager_01_module_shutdown
 * Shuts down the entire manager_01 module
 */
int io_bundling_manager_01_module_shutdown(void) {
    if (!s_manager_01_initialized) {
        return 0;  /* Already shut down */
    }
    
    /* Clean up any active scene */
    if (s_active_scene) {
        io_bundling_manager_01_cleanup_scene(s_active_scene);
        free(s_active_scene);
        s_active_scene = NULL;
    }
    
    /* Shutdown multi-threaded batch processing system */
    io_bundling_manager_01_shutdown_batch_system();
    
    /* Shutdown async file loading system */
    io_bundling_manager_01_shutdown_async_system();

    s_manager_01_initialized = false;
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* End of io_bundling_manager_01.c */

/* ============================================================================
 * MISSING HELPER FUNCTION IMPLEMENTATIONS
 * ============================================================================ */

/* Compression function implementation */
static int io_bundling_manager_01_compress_data(const void* input, size_t input_size, 
                                             void** output, size_t* output_size, uint32_t compression_type) {
    if (!input || !output || !output_size) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    // Simple compression simulation (in real implementation, use LZ4/ZSTD libraries)
    *output_size = input_size;
    *output = malloc(*output_size);
    if (!*output) {
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(*output, input, input_size);
    
    // Update telemetry
    s_telemetry.memory_allocations++;
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Scene parsing function implementation */
static int io_bundling_manager_01_parse_scene_file(const char* file_path, io_bundling_manager_01_scene_data_t* scene_data) {
    if (!file_path || !scene_data) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    // Placeholder scene parsing implementation
    scene_data->node_count = 0;
    scene_data->mesh_count = 0;
    scene_data->material_count = 0;
    scene_data->texture_count = 0;
    scene_data->nodes = NULL;
    scene_data->meshes = NULL;
    scene_data->materials = NULL;
    scene_data->textures = NULL;
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Scene validation function implementation */
static int io_bundling_manager_01_validate_scene_structure(const io_bundling_manager_01_scene_data_t* scene) {
    if (!scene) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    // Basic validation
    if (scene->node_count > IO_BUNDLING_MANAGER_01_MAX_SCENE_NODES) {
        return IO_BUNDLING_MANAGER_01_ERROR_PARSE_FAILED;
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Bundle creation function implementation */
static int io_bundling_manager_01_create_bundle(const char* name, void* assets, size_t asset_count, 
                                             io_bundling_manager_01_bundle_t** bundle) {
    if (!name || !assets || !bundle) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    *bundle = malloc(sizeof(io_bundling_manager_01_bundle_t));
    if (!*bundle) {
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    strncpy((*bundle)->name, name, sizeof((*bundle)->name) - 1);
    (*bundle)->name[sizeof((*bundle)->name) - 1] = '\0';
    (*bundle)->data = assets;
    (*bundle)->asset_count = asset_count;
    (*bundle)->data_size = asset_count * 1024; // Estimate
    (*bundle)->creation_time = time(NULL);
    (*bundle)->last_modified = (*bundle)->creation_time;
    (*bundle)->compressed = false;
    (*bundle)->id = atomic_fetch_add(&s_next_async_id, 1);
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Bundle save function implementation */
static int io_bundling_manager_01_save_bundle(const io_bundling_manager_01_bundle_t* bundle, const char* path) {
    if (!bundle || !path) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    // Placeholder bundle saving
    FILE* file = fopen(path, "wb");
    if (!file) {
        return IO_BUNDLING_MANAGER_01_ERROR_FILE_WRITE_FAILED;
    }
    
    fwrite(bundle->data, 1, bundle->data_size, file);
    fclose(file);
    
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Scene cleanup function implementation */
static void io_bundling_manager_01_cleanup_scene(io_bundling_manager_01_scene_t* scene) {
    if (!scene) return;
    
    if (scene->nodes) free(scene->nodes);
    if (scene->objects) free(scene->objects);
    
    memset(scene, 0, sizeof(*scene));
}

/* Pool cleanup function implementation */
static void io_bundling_manager_01_pool_cleanup(void) {
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_POOL_SIZE; i++) {
        if (s_resource_pool.resources[i]) {
            free(s_resource_pool.resources[i]);
            s_resource_pool.resources[i] = NULL;
            s_resource_pool.resource_sizes[i] = 0;
            atomic_store(&s_resource_pool.available[i], true);
        }
    }
    
    atomic_store(&s_resource_pool.available_count, IO_BUNDLING_MANAGER_01_POOL_SIZE);
    s_resource_pool.total_allocated = 0;
}

/* Async system init function implementation */
static int io_bundling_manager_01_init_async_system(void) {
    if (s_async_system_running) {
        return IO_BUNDLING_MANAGER_01_ERROR_NONE;
    }
    
    // Initialize async operation tracking
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        s_async_operations[i] = NULL;
    }
    
    // Create worker threads
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_ASYNC_THREAD_COUNT; i++) {
        if (thrd_create(&s_async_threads[i], io_bundling_manager_01_async_worker_thread, NULL) != thrd_success) {
            return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
        }
    }
    
    s_async_system_running = true;
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Async system shutdown function implementation */
static void io_bundling_manager_01_shutdown_async_system(void) {
    if (!s_async_system_running) return;
    
    s_async_system_running = false;
    
    // Wait for worker threads to finish
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_ASYNC_THREAD_COUNT; i++) {
        thrd_join(s_async_threads[i], NULL);
    }
    
    // Clean up remaining operations
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
        if (s_async_operations[i]) {
            free(s_async_operations[i]);
            s_async_operations[i] = NULL;
        }
    }
}

/* Batch system init function implementation */
static int io_bundling_manager_01_init_batch_system(void) {
    if (s_batch_system_running) {
        return IO_BUNDLING_MANAGER_01_ERROR_NONE;
    }
    
    // Initialize batch queue
    s_batch_queue.tasks = malloc(IO_BUNDLING_MANAGER_01_BATCH_QUEUE_SIZE * sizeof(io_bundling_manager_01_batch_task_t));
    if (!s_batch_queue.tasks) {
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    s_batch_queue.capacity = IO_BUNDLING_MANAGER_01_BATCH_QUEUE_SIZE;
    s_batch_queue.head = 0;
    s_batch_queue.tail = 0;
    s_batch_queue.count = 0;
    s_batch_queue.is_shutdown = false;
    
    // Initialize synchronization
    if (mtx_init(&s_batch_queue.mutex, mtx_plain) != thrd_success) {
        free(s_batch_queue.tasks);
        return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
    }
    
    if (cnd_init(&s_batch_queue.condition) != thrd_success) {
        mtx_destroy(&s_batch_queue.mutex);
        free(s_batch_queue.tasks);
        return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
    }
    
    // Create worker threads
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WORKER_THREADS; i++) {
        if (thrd_create(&s_worker_threads[i], io_bundling_manager_01_worker_thread, NULL) != thrd_success) {
            // Cleanup already created threads
            for (int j = 0; j < i; j++) {
                thrd_join(s_worker_threads[j], NULL);
            }
            mtx_destroy(&s_batch_queue.mutex);
            cnd_destroy(&s_batch_queue.condition);
            free(s_batch_queue.tasks);
            return IO_BUNDLING_MANAGER_01_ERROR_THREADING_FAILED;
        }
    }
    
    s_batch_system_running = true;
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Batch system shutdown function implementation */
static void io_bundling_manager_01_shutdown_batch_system(void) {
    if (!s_batch_system_running) return;
    
    s_batch_system_running = false;
    s_batch_queue.is_shutdown = true;
    
    // Wake up all worker threads
    cnd_broadcast(&s_batch_queue.condition);
    
    // Wait for worker threads to finish
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WORKER_THREADS; i++) {
        thrd_join(s_worker_threads[i], NULL);
    }
    
    // Clean up queue
    mtx_destroy(&s_batch_queue.mutex);
    cnd_destroy(&s_batch_queue.condition);
    free(s_batch_queue.tasks);
    s_batch_queue.tasks = NULL;
}

/* File watcher functions */
static int io_bundling_manager_01_start_file_watcher(const char* path, bool (*callback)(const char*, void*), void* user_data) {
    if (!path || !callback) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    // Placeholder file watching implementation
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES; i++) {
        if (s_file_watchers[i].active == false) {
            strncpy(s_file_watchers[i].path, path, sizeof(s_file_watchers[i].path) - 1);
            s_file_watchers[i].path[sizeof(s_file_watchers[i].path) - 1] = '\0';
            s_file_watchers[i].callback = callback;
            s_file_watchers[i].user_data = user_data;
            s_file_watchers[i].last_modified = time(NULL);
            s_file_watchers[i].active = true;
            return IO_BUNDLING_MANAGER_01_ERROR_NONE;
        }
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_HOT_RELOAD_FAILED;
}

static int io_bundling_manager_01_stop_file_watcher(const char* path) {
    if (!path) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES; i++) {
        if (s_file_watchers[i].active && strcmp(s_file_watchers[i].path, path) == 0) {
            s_file_watchers[i].active = false;
            s_file_watchers[i].callback = NULL;
            s_file_watchers[i].user_data = NULL;
            return IO_BUNDLING_MANAGER_01_ERROR_NONE;
        }
    }
    
    return IO_BUNDLING_MANAGER_01_ERROR_HOT_RELOAD_FAILED;
}

static void io_bundling_manager_01_process_file_changes(void) {
    for (int i = 0; i < IO_BUNDLING_MANAGER_01_MAX_WATCHED_FILES; i++) {
        if (s_file_watchers[i].active) {
            // Check file modification time
            struct stat st;
            if (stat(s_file_watchers[i].path, &st) == 0) {
                if (st.st_mtime > s_file_watchers[i].last_modified) {
                    s_file_watchers[i].last_modified = st.st_mtime;
                    if (s_file_watchers[i].callback) {
                        s_file_watchers[i].callback(s_file_watchers[i].path, s_file_watchers[i].user_data);
                    }
                }
            }
        }
    }
}

/* Format conversion function implementation */
static int io_bundling_manager_01_convert_format(const void* input, size_t input_size, int source_format, 
                                               int target_format, void** output, size_t* output_size) {
    if (!input || !output || !output_size) {
        return IO_BUNDLING_MANAGER_01_ERROR_INVALID_PARAM;
    }
    
    // Simple format conversion (copy data)
    *output_size = input_size;
    *output = malloc(*output_size);
    if (!*output) {
        return IO_BUNDLING_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(*output, input, input_size);
    return IO_BUNDLING_MANAGER_01_ERROR_NONE;
}

/* Telemetry functions */
static void io_bundling_manager_01_update_telemetry_stats(void) {
    s_telemetry.total_operations++;
}

static void io_bundling_manager_01_record_operation_start(uint64_t operation_id) {
    (void)operation_id;
    // Placeholder for operation timing
}

static void io_bundling_manager_01_record_operation_end(uint64_t operation_id) {
    (void)operation_id;
    // Placeholder for operation timing
}

/* Worker thread functions */
static int io_bundling_manager_01_async_worker_thread(void* arg) {
    (void)arg;
    
    while (s_async_system_running) {
        // Process async operations
        thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 10000000}, NULL); // 10ms
    }
    
    return 0;
}

static int io_bundling_manager_01_worker_thread(void* arg) {
    (void)arg;
    
    while (s_batch_system_running && !s_batch_queue.is_shutdown) {
        // Process batch tasks
        thrd_sleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 10000000}, NULL); // 10ms
    }
    
    return 0;
}
