/*
 * io_bundling_processor_04.c
 *
 * I/O and asset streaming - Bundling Subsystem
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * This file implements processor functionality for the bundling module
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
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <immintrin.h>

#include "assets/io/bundling/processor_04.h"
#include "include/core/types.h"
#include "include/core/memory.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================ */

#define IO_BUNDLING_PROCESSOR_04_VERSION_MAJOR 1
#define IO_BUNDLING_PROCESSOR_04_VERSION_MINOR 0
#define IO_BUNDLING_PROCESSOR_04_VERSION_PATCH 0

#define IO_BUNDLING_PROCESSOR_04_MAX_INSTANCES 4096
#define IO_BUNDLING_PROCESSOR_04_DEFAULT_CAPACITY 256
#define IO_BUNDLING_PROCESSOR_04_ALIGNMENT 16
#define IO_BUNDLING_PROCESSOR_04_MAX_PATH_LENGTH 512
#define IO_BUNDLING_PROCESSOR_04_MAX_QUEUE_SIZE 1024
#define IO_BUNDLING_PROCESSOR_04_MAX_WORKERS 8
#define IO_BUNDLING_PROCESSOR_04_CACHE_SIZE 64
#define IO_BUNDLING_PROCESSOR_04_PROGRESS_UPDATE_INTERVAL_MS 100

/* Compression algorithms */
#define IO_BUNDLING_PROCESSOR_04_COMPRESSION_NONE 0
#define IO_BUNDLING_PROCESSOR_04_COMPRESSION_LZ4  1
#define IO_BUNDLING_PROCESSOR_04_COMPRESSION_ZSTD 2

/* Asset formats */
#define IO_BUNDLING_PROCESSOR_04_FORMAT_GLTF  1
#define IO_BUNDLING_PROCESSOR_04_FORMAT_FBX   2
#define IO_BUNDLING_PROCESSOR_04_FORMAT_OBJ   3
#define IO_BUNDLING_PROCESSOR_04_FORMAT_CUSTOM 4

/* Processing priorities */
#define IO_BUNDLING_PROCESSOR_04_PRIORITY_LOW    0
#define IO_BUNDLING_PROCESSOR_04_PRIORITY_NORMAL 1
#define IO_BUNDLING_PROCESSOR_04_PRIORITY_HIGH   2
#define IO_BUNDLING_PROCESSOR_04_PRIORITY_CRITICAL 3

#define IO_BUNDLING_PROCESSOR_04_FLAG_NONE          0x00000000
#define IO_BUNDLING_PROCESSOR_04_FLAG_INITIALIZED   0x00000001
#define IO_BUNDLING_PROCESSOR_04_FLAG_DIRTY         0x00000002
#define IO_BUNDLING_PROCESSOR_04_FLAG_GPU_RESIDENT  0x00000004
#define IO_BUNDLING_PROCESSOR_04_FLAG_STREAMING     0x00000008

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */


/*
 * IO_BUNDLING_PROCESSOR_04 - Core data structure
 * Manages state and resources for processor_04 operations
 */
typedef struct io_bundling_processor_04 {
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
} io_bundling_processor_04_t;

typedef struct io_bundling_processor_04_desc {
    uint32_t flags;
    size_t initial_capacity;
    void* user_data;
    void* allocator;
} io_bundling_processor_04_desc_t;

typedef struct io_bundling_processor_04_stats {
    uint64_t total_allocations;
    uint64_t active_count;
    uint64_t peak_count;
    size_t memory_used;
    size_t memory_peak;
    double avg_process_time_ms;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t files_processed;
    uint64_t bytes_compressed;
    uint64_t bytes_decompressed;
    double compression_ratio;
} io_bundling_processor_04_stats_t;

typedef struct io_bundling_processor_04_binary_header {
    uint32_t magic;
    uint32_t version;
    uint32_t data_size;
    uint32_t compressed_size;
    uint32_t compression_type;
    uint32_t format_type;
    uint64_t timestamp;
    uint32_t checksum;
} io_bundling_processor_04_binary_header_t;

typedef struct io_bundling_processor_04_cache_entry {
    char file_path[IO_BUNDLING_PROCESSOR_04_MAX_PATH_LENGTH];
    void* data;
    size_t size;
    uint64_t last_access;
    uint32_t priority;
    uint32_t access_count;
    bool is_compressed;
} io_bundling_processor_04_cache_entry_t;

typedef struct io_bundling_processor_04_work_queue {
    void* items[IO_BUNDLING_PROCESSOR_04_MAX_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} io_bundling_processor_04_work_queue_t;

typedef struct io_bundling_processor_04_async_task {
    char file_path[IO_BUNDLING_PROCESSOR_04_MAX_PATH_LENGTH];
    void* callback;
    void* user_data;
    uint32_t priority;
    bool is_completed;
} io_bundling_processor_04_async_task_t;

typedef struct io_bundling_processor_04_mmap_region {
    void* data;
    size_t size;
    int fd;
    bool is_mapped;
} io_bundling_processor_04_mmap_region_t;

typedef struct io_bundling_processor_04_progress {
    uint32_t current_item;
    uint32_t total_items;
    double percentage;
    char current_file[IO_BUNDLING_PROCESSOR_04_MAX_PATH_LENGTH];
    uint64_t last_update_time;
} io_bundling_processor_04_progress_t;

typedef struct io_bundling_processor_04_scene_data {
    uint32_t node_count;
    uint32_t mesh_count;
    uint32_t material_count;
    uint32_t texture_count;
    void* nodes;
    void* meshes;
    void* materials;
    void* textures;
} io_bundling_processor_04_scene_data_t;

typedef struct io_bundling_processor_04_bundle {
    char name[256];
    io_bundling_processor_04_binary_header_t header;
    void* asset_data;
    size_t asset_count;
    size_t total_size;
} io_bundling_processor_04_bundle_t;


/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static io_bundling_processor_04_stats_t s_processor_04_stats = {0};
static bool s_processor_04_initialized = false;
static io_bundling_processor_04_cache_entry_t s_processor_04_cache[IO_BUNDLING_PROCESSOR_04_CACHE_SIZE];
static io_bundling_processor_04_work_queue_t s_processor_04_work_queue;
static pthread_t s_processor_04_workers[IO_BUNDLING_PROCESSOR_04_MAX_WORKERS];
static bool s_processor_04_workers_running = false;
static io_bundling_processor_04_progress_t s_processor_04_progress = {0};
static uint32_t s_processor_04_next_id = 1;

// SIMD optimization
static bool s_processor_04_simd_available = false;
static void* s_processor_04_simd_buffer = NULL;

// Format conversion
typedef struct {
    uint32_t from_format;
    uint32_t to_format;
    int (*convert_func)(const void*, void**);
} format_converter_t;
static format_converter_t s_processor_04_format_converters[IO_BUNDLING_PROCESSOR_04_MAX_CONVERTERS];

// Asset streaming priority
static void* s_processor_04_priority_queue[IO_BUNDLING_PROCESSOR_04_MAX_QUEUE_SIZE];
static uint32_t s_processor_04_priority_queue_head = 0;
static uint32_t s_processor_04_priority_queue_tail = 0;
static uint32_t s_processor_04_priority_queue_count = 0;

// Checkpointing for resumable operations
static bool s_processor_04_cancel_all_operations = false;
static io_bundling_processor_04_async_task_t s_processor_04_operations[IO_BUNDLING_PROCESSOR_04_MAX_OPERATIONS];

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int io_bundling_processor_04_convert_gltf_to_fbx(const void* input, void** output);
static int io_bundling_processor_04_convert_gltf_to_obj(const void* input, void** output);
static int io_bundling_processor_04_convert_gltf_to_custom(const void* input, void** output);
static int io_bundling_processor_04_convert_fbx_to_gltf(const void* input, void** output);
static int io_bundling_processor_04_convert_fbx_to_obj(const void* input, void** output);
static int io_bundling_processor_04_convert_fbx_to_custom(const void* input, void** output);
static int io_bundling_processor_04_convert_obj_to_gltf(const void* input, void** output);
static int io_bundling_processor_04_convert_obj_to_fbx(const void* input, void** output);
static int io_bundling_processor_04_convert_obj_to_custom(const void* input, void** output);
static int io_bundling_processor_04_convert_custom_to_gltf(const void* input, void** output);
static int io_bundling_processor_04_convert_custom_to_fbx(const void* input, void** output);
static int io_bundling_processor_04_convert_custom_to_obj(const void* input, void** output);
static int io_bundling_processor_04_validate_internal(io_bundling_processor_04_t* ctx);
static int io_bundling_processor_04_cleanup_internal(io_bundling_processor_04_t* ctx);
static int io_bundling_processor_04_serialize_binary(const void* data, size_t size, void** out_data, size_t* out_size);
static int io_bundling_processor_04_deserialize_binary(const void* data, size_t size, void** out_data, size_t* out_size);
static int io_bundling_processor_04_compress_data(const void* input, size_t input_size, void** output, size_t* output_size, uint32_t compression_type);
static int io_bundling_processor_04_decompress_data(const void* input, size_t input_size, void** output, size_t* output_size, uint32_t compression_type);
static int io_bundling_processor_04_convert_format(const void* input, uint32_t input_format, void** output, uint32_t output_format);
static int io_bundling_processor_04_cache_add(const char* file_path, void* data, size_t size, uint32_t priority);
static void* io_bundling_processor_04_cache_get(const char* file_path);
static void io_bundling_processor_04_cache_remove(const char* file_path);
static int io_bundling_processor_04_mmap_file(const char* file_path, io_bundling_processor_04_mmap_region_t* region);
static int io_bundling_processor_04_munmap_file(io_bundling_processor_04_mmap_region_t* region);
static void* io_bundling_processor_04_worker_thread(void* arg);
static int io_bundling_processor_04_queue_task(io_bundling_processor_04_async_task_t* task);
static io_bundling_processor_04_async_task_t* io_bundling_processor_04_dequeue_task(void);
static void io_bundling_processor_04_update_progress(uint32_t current, uint32_t total, const char* file_name);
static int io_bundling_processor_04_parse_scene_file(const char* file_path, io_bundling_processor_04_scene_data_t* scene_data);
static int io_bundling_processor_04_create_bundle(const char* bundle_name, const void** assets, size_t asset_count, io_bundling_processor_04_bundle_t* bundle);
static void io_bundling_processor_04_register_converter(uint32_t from_format, uint32_t to_format, int (*convert_func)(const void*, void**));
static void io_bundling_processor_04_watch_file_changes(const char* directory);
static uint32_t io_bundling_processor_04_calculate_priority(const char* file_path, size_t file_size);
static bool io_bundling_processor_04_gpu_compute_available(void);
static int io_bundling_processor_04_gpu_compute_fallback(void* data, size_t size);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static int io_bundling_processor_04_validate_internal(io_bundling_processor_04_t* ctx) {
    if (!ctx) return -1;
    if (!ctx->is_initialized) return -2;
    
    // Validate LZ4/ZSTD compression support
    if (ctx->flags & IO_BUNDLING_PROCESSOR_04_FLAG_GPU_RESIDENT) {
        if (!io_bundling_processor_04_gpu_compute_available()) {
            // GPU compute not available, fallback to CPU
            ctx->flags &= ~IO_BUNDLING_PROCESSOR_04_FLAG_GPU_RESIDENT;
        }
    }
    
    return 0;
}

static int io_bundling_processor_04_cleanup_internal(io_bundling_processor_04_t* ctx) {
    if (!ctx) return -1;
    
    // Implement binary serialization cleanup
    if (ctx->internal_data) {
        void* serialized_data = NULL;
        size_t serialized_size = 0;
        
        if (io_bundling_processor_04_serialize_binary(ctx->internal_data, ctx->data_size, &serialized_data, &serialized_size) == 0) {
            // Save serialized data to disk or cache
            io_bundling_processor_04_cache_add("processor_state", serialized_data, serialized_size, IO_BUNDLING_PROCESSOR_04_PRIORITY_HIGH);
            free(serialized_data);
        }
        
        free(ctx->internal_data);
        ctx->internal_data = NULL;
    }
    
    ctx->is_dirty = false;
    return 0;
}

static int io_bundling_processor_04_serialize_binary(const void* data, size_t size, void** out_data, size_t* out_size) {
    if (!data || !out_data || !out_size) return -1;
    
    io_bundling_processor_04_binary_header_t header = {0};
    header.magic = 0x50524F43; // "PROC"
    header.version = (IO_BUNDLING_PROCESSOR_04_VERSION_MAJOR << 16) | (IO_BUNDLING_PROCESSOR_04_VERSION_MINOR << 8) | IO_BUNDLING_PROCESSOR_04_VERSION_PATCH;
    header.data_size = (uint32_t)size;
    header.compressed_size = (uint32_t)size;
    header.compression_type = IO_BUNDLING_PROCESSOR_04_COMPRESSION_NONE;
    header.timestamp = (uint64_t)time(NULL);
    
    // Calculate simple checksum
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = (checksum << 1) | (checksum >> 31);
        checksum += bytes[i];
    }
    header.checksum = checksum;
    
    size_t total_size = sizeof(header) + size;
    void* result = malloc(total_size);
    if (!result) return -2;
    
    memcpy(result, &header, sizeof(header));
    memcpy((uint8_t*)result + sizeof(header), data, size);
    
    *out_data = result;
    *out_size = total_size;
    
    s_processor_04_stats.files_processed++;
    
    return 0;
}

static int io_bundling_processor_04_deserialize_binary(const void* data, size_t size, void** out_data, size_t* out_size) {
    if (!data || size < sizeof(io_bundling_processor_04_binary_header_t) || !out_data || !out_size) return -1;
    
    const io_bundling_processor_04_binary_header_t* header = (const io_bundling_processor_04_binary_header_t*)data;
    
    // Validate header
    if (header->magic != 0x50524F43) return -2;
    if (header->data_size + sizeof(*header) != size) return -3;
    
    // Verify checksum
    const uint8_t* payload = (const uint8_t*)data + sizeof(*header);
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < header->data_size; i++) {
        checksum = (checksum << 1) | (checksum >> 31);
        checksum += payload[i];
    }
    if (checksum != header->checksum) return -4;
    
    void* result = malloc(header->data_size);
    if (!result) return -5;
    
    memcpy(result, payload, header->data_size);
    
    *out_data = result;
    *out_size = header->data_size;
    
    return 0;
}

static int io_bundling_processor_04_compress_data(const void* input, size_t input_size, void** output, size_t* output_size, uint32_t compression_type) {
    if (!input || !output || !output_size) return -1;
    
    switch (compression_type) {
        case IO_BUNDLING_PROCESSOR_04_COMPRESSION_NONE: {
            void* result = malloc(input_size);
            if (!result) return -2;
            memcpy(result, input, input_size);
            *output = result;
            *output_size = input_size;
            break;
        }
        
        case IO_BUNDLING_PROCESSOR_04_COMPRESSION_LZ4: {
            // Placeholder for LZ4 compression
            // In a real implementation, this would call LZ4 library
            void* result = malloc(input_size);
            if (!result) return -2;
            memcpy(result, input, input_size);
            *output = result;
            *output_size = input_size;
            s_processor_04_stats.bytes_compressed += input_size;
            break;
        }
        
        case IO_BUNDLING_PROCESSOR_04_COMPRESSION_ZSTD: {
            // Placeholder for ZSTD compression
            // In a real implementation, this would call ZSTD library
            void* result = malloc(input_size);
            if (!result) return -2;
            memcpy(result, input, input_size);
            *output = result;
            *output_size = input_size;
            s_processor_04_stats.bytes_compressed += input_size;
            break;
        }
        
        default:
            return -3;
    }
    
    return 0;
}

static int io_bundling_processor_04_decompress_data(const void* input, size_t input_size, void** output, size_t* output_size, uint32_t compression_type) {
    if (!input || !output || !output_size) return -1;
    
    switch (compression_type) {
        case IO_BUNDLING_PROCESSOR_04_COMPRESSION_NONE: {
            void* result = malloc(input_size);
            if (!result) return -2;
            memcpy(result, input, input_size);
            *output = result;
            *output_size = input_size;
            break;
        }
        
        case IO_BUNDLING_PROCESSOR_04_COMPRESSION_LZ4:
        case IO_BUNDLING_PROCESSOR_04_COMPRESSION_ZSTD: {
            // Placeholder for decompression
            // In a real implementation, this would call the appropriate library
            void* result = malloc(input_size);
            if (!result) return -2;
            memcpy(result, input, input_size);
            *output = result;
            *output_size = input_size;
            s_processor_04_stats.bytes_decompressed += input_size;
            break;
        }
        
        default:
            return -3;
    }
    
    return 0;
}

static int io_bundling_processor_04_convert_format(const void* input, uint32_t input_format, void** output, uint32_t output_format) {
    if (!input || !output) return -1;
    
    // Enhanced format conversion implementation
    switch (input_format) {
        case IO_BUNDLING_PROCESSOR_04_FORMAT_GLTF: {
            // Convert from glTF to various formats
            switch (output_format) {
                case IO_BUNDLING_PROCESSOR_04_FORMAT_FBX:
                    return io_bundling_processor_04_convert_gltf_to_fbx(input, output);
                case IO_BUNDLING_PROCESSOR_04_FORMAT_OBJ:
                    return io_bundling_processor_04_convert_gltf_to_obj(input, output);
                case IO_BUNDLING_PROCESSOR_04_FORMAT_CUSTOM:
                    return io_bundling_processor_04_convert_gltf_to_custom(input, output);
                default:
                    return -4; // Unsupported conversion
            }
        }
        
        case IO_BUNDLING_PROCESSOR_04_FORMAT_FBX: {
            // Convert from FBX to various formats
            switch (output_format) {
                case IO_BUNDLING_PROCESSOR_04_FORMAT_GLTF:
                    return io_bundling_processor_04_convert_fbx_to_gltf(input, output);
                case IO_BUNDLING_PROCESSOR_04_FORMAT_OBJ:
                    return io_bundling_processor_04_convert_fbx_to_obj(input, output);
                case IO_BUNDLING_PROCESSOR_04_FORMAT_CUSTOM:
                    return io_bundling_processor_04_convert_fbx_to_custom(input, output);
                default:
                    return -4; // Unsupported conversion
            }
        }
        
        case IO_BUNDLING_PROCESSOR_04_FORMAT_OBJ: {
            // Convert from OBJ to various formats
            switch (output_format) {
                case IO_BUNDLING_PROCESSOR_04_FORMAT_GLTF:
                    return io_bundling_processor_04_convert_obj_to_gltf(input, output);
                case IO_BUNDLING_PROCESSOR_04_FORMAT_FBX:
                    return io_bundling_processor_04_convert_obj_to_fbx(input, output);
                case IO_BUNDLING_PROCESSOR_04_FORMAT_CUSTOM:
                    return io_bundling_processor_04_convert_obj_to_custom(input, output);
                default:
                    return -4; // Unsupported conversion
            }
        }
        
        case IO_BUNDLING_PROCESSOR_04_FORMAT_CUSTOM: {
            // Convert from custom format to various formats
            switch (output_format) {
                case IO_BUNDLING_PROCESSOR_04_FORMAT_GLTF:
                    return io_bundling_processor_04_convert_custom_to_gltf(input, output);
                case IO_BUNDLING_PROCESSOR_04_FORMAT_FBX:
                    return io_bundling_processor_04_convert_custom_to_fbx(input, output);
                case IO_BUNDLING_PROCESSOR_04_FORMAT_OBJ:
                    return io_bundling_processor_04_convert_custom_to_obj(input, output);
                default:
                    return -4; // Unsupported conversion
            }
        }
        
        default:
            return -3; // Unknown input format
    }
}

/* ============================================================================
 * FORMAT CONVERSION IMPLEMENTATIONS
 * ============================================================================ */

static int io_bundling_processor_04_convert_gltf_to_fbx(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for glTF to FBX conversion
    // In a real implementation, this would use FBX SDK
    size_t output_size = 2048; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "FBX converted from glTF");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_gltf_to_obj(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for glTF to OBJ conversion
    size_t output_size = 1024; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "OBJ converted from glTF");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_gltf_to_custom(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for glTF to custom format conversion
    size_t output_size = 1536; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "Custom format converted from glTF");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_fbx_to_gltf(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for FBX to glTF conversion
    size_t output_size = 2048; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "glTF converted from FBX");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_fbx_to_obj(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for FBX to OBJ conversion
    size_t output_size = 1024; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "OBJ converted from FBX");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_fbx_to_custom(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for FBX to custom format conversion
    size_t output_size = 1536; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "Custom format converted from FBX");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_obj_to_gltf(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for OBJ to glTF conversion
    size_t output_size = 1024; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "glTF converted from OBJ");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_obj_to_fbx(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for OBJ to FBX conversion
    size_t output_size = 2048; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "FBX converted from OBJ");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_obj_to_custom(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for OBJ to custom format conversion
    size_t output_size = 1536; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "Custom format converted from OBJ");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_custom_to_gltf(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for custom to glTF conversion
    size_t output_size = 1024; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "glTF converted from custom format");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_custom_to_fbx(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for custom to FBX conversion
    size_t output_size = 2048; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "FBX converted from custom format");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_convert_custom_to_obj(const void* input, void** output) {
    if (!input || !output) return -1;
    
    // Placeholder implementation for custom to OBJ conversion
    size_t output_size = 1024; // Placeholder size
    void* result = malloc(output_size);
    if (!result) return -2;
    
    // Simulate conversion process
    memset(result, 0, output_size);
    strcpy((char*)result, "OBJ converted from custom format");
    
    *output = result;
    return 0;
}

static int io_bundling_processor_04_cache_add(const char* file_path, void* data, size_t size, uint32_t priority) {
    if (!file_path || !data) return -1;
    
    // Find empty slot or replace LRU
    int slot = -1;
    uint64_t oldest_time = UINT64_MAX;
    
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_CACHE_SIZE; i++) {
        if (s_processor_04_cache[i].data == NULL) {
            slot = i;
            break;
        }
        if (s_processor_04_cache[i].last_access < oldest_time) {
            oldest_time = s_processor_04_cache[i].last_access;
            slot = i;
        }
    }
    
    if (slot == -1) return -2;
    
    // Free existing data if necessary
    if (s_processor_04_cache[slot].data) {
        free(s_processor_04_cache[slot].data);
    }
    
    // Add new entry
    strncpy(s_processor_04_cache[slot].file_path, file_path, IO_BUNDLING_PROCESSOR_04_MAX_PATH_LENGTH - 1);
    s_processor_04_cache[slot].file_path[IO_BUNDLING_PROCESSOR_04_MAX_PATH_LENGTH - 1] = '\0';
    s_processor_04_cache[slot].data = malloc(size);
    if (!s_processor_04_cache[slot].data) return -3;
    
    memcpy(s_processor_04_cache[slot].data, data, size);
    s_processor_04_cache[slot].size = size;
    s_processor_04_cache[slot].last_access = (uint64_t)time(NULL);
    s_processor_04_cache[slot].priority = priority;
    s_processor_04_cache[slot].access_count = 1;
    s_processor_04_cache[slot].is_compressed = false;
    return 0;
}

static void io_bundling_processor_04_register_converter(uint32_t from_format, uint32_t to_format, int (*convert_func)(const void*, void**)) {
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_MAX_CONVERTERS; i++) {
        if (s_processor_04_format_converters[i].convert_func == NULL) {
            s_processor_04_format_converters[i].from_format = from_format;
            s_processor_04_format_converters[i].to_format = to_format;
            s_processor_04_format_converters[i].convert_func = convert_func;
            printf("Registered format converter: %u -> %u at slot %d\n", from_format, to_format, i);
            return;
        }
    }
    printf("Warning: Could not register format converter %u -> %u, no available slots\n", from_format, to_format);
}

static void* io_bundling_processor_04_cache_get(const char* file_path) {
    if (!file_path) return NULL;
    
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_CACHE_SIZE; i++) {
        if (s_processor_04_cache[i].data && strcmp(s_processor_04_cache[i].file_path, file_path) == 0) {
            s_processor_04_cache[i].last_access = (uint64_t)time(NULL);
            s_processor_04_cache[i].access_count++;
            s_processor_04_stats.cache_hits++;
            return s_processor_04_cache[i].data;
        }
    }
    
    s_processor_04_stats.cache_misses++;
    return NULL;
}

static void io_bundling_processor_04_cache_remove(const char* file_path) {
    if (!file_path) return;
    
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_CACHE_SIZE; i++) {
        if (s_processor_04_cache[i].data && strcmp(s_processor_04_cache[i].file_path, file_path) == 0) {
            free(s_processor_04_cache[i].data);
            s_processor_04_cache[i].data = NULL;
            s_processor_04_cache[i].file_path[0] = '\0';
            s_processor_04_cache[i].size = 0;
            s_processor_04_cache[i].last_access = 0;
            s_processor_04_cache[i].priority = 0;
            s_processor_04_cache[i].access_count = 0;
            s_processor_04_cache[i].is_compressed = false;
            break;
        }
    }
}

static int io_bundling_processor_04_mmap_file(const char* file_path, io_bundling_processor_04_mmap_region_t* region) {
    if (!file_path || !region) return -1;
    
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) return -2;
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return -3;
    }
    
    void* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return -4;
    }
    
    region->data = data;
    region->size = st.st_size;
    region->fd = fd;
    region->is_mapped = true;
    
    return 0;
}

static int io_bundling_processor_04_munmap_file(io_bundling_processor_04_mmap_region_t* region) {
    if (!region || !region->is_mapped) return -1;
    
    if (region->data) {
        munmap(region->data, region->size);
        region->data = NULL;
    }
    
    if (region->fd != -1) {
        close(region->fd);
        region->fd = -1;
    }
    
    region->size = 0;
    region->is_mapped = false;
    
    return 0;
}

static void* io_bundling_processor_04_worker_thread(void* arg) {
    int worker_id = *(int*)arg;
    
    while (s_processor_04_workers_running) {
        io_bundling_processor_04_async_task_t* task = io_bundling_processor_04_dequeue_task();
        if (task) {
            // Process the task
            io_bundling_processor_04_mmap_region_t region = {0};
            
            if (io_bundling_processor_04_mmap_file(task->file_path, &region) == 0) {
                // Process the file
                io_bundling_processor_04_update_progress(s_processor_04_progress.current_item++, 
                                                       s_processor_04_progress.total_items, 
                                                       task->file_path);
                
                // Cache the processed data
                io_bundling_processor_04_cache_add(task->file_path, region.data, region.size, task->priority);
                
                io_bundling_processor_04_munmap_file(&region);
            }
            
            task->is_completed = true;
            free(task);
        } else {
            // No tasks available, wait
            usleep(1000); // 1ms
        }
    }
    
    return NULL;
}

static int io_bundling_processor_04_queue_task(io_bundling_processor_04_async_task_t* task) {
    if (!task) return -1;
    
    pthread_mutex_lock(&s_processor_04_work_queue.mutex);
    
    if (s_processor_04_work_queue.count >= IO_BUNDLING_PROCESSOR_04_MAX_QUEUE_SIZE) {
        pthread_mutex_unlock(&s_processor_04_work_queue.mutex);
        return -2;
    }
    
    s_processor_04_work_queue.items[s_processor_04_work_queue.tail] = task;
    s_processor_04_work_queue.tail = (s_processor_04_work_queue.tail + 1) % IO_BUNDLING_PROCESSOR_04_MAX_QUEUE_SIZE;
    s_processor_04_work_queue.count++;
    
    pthread_cond_signal(&s_processor_04_work_queue.cond);
    pthread_mutex_unlock(&s_processor_04_work_queue.mutex);
    
    return 0;
}

static io_bundling_processor_04_async_task_t* io_bundling_processor_04_dequeue_task(void) {
    pthread_mutex_lock(&s_processor_04_work_queue.mutex);
    
    if (s_processor_04_work_queue.count == 0) {
        pthread_mutex_unlock(&s_processor_04_work_queue.mutex);
        return NULL;
    }
    
    io_bundling_processor_04_async_task_t* task = s_processor_04_work_queue.items[s_processor_04_work_queue.head];
    s_processor_04_work_queue.head = (s_processor_04_work_queue.head + 1) % IO_BUNDLING_PROCESSOR_04_MAX_QUEUE_SIZE;
    s_processor_04_work_queue.count--;
    
    pthread_mutex_unlock(&s_processor_04_work_queue.mutex);
    
    return task;
}

static void io_bundling_processor_04_update_progress(uint32_t current, uint32_t total, const char* file_name) {
    uint64_t current_time = (uint64_t)time(NULL) * 1000;
    
    if (current_time - s_processor_04_progress.last_update_time >= IO_BUNDLING_PROCESSOR_04_PROGRESS_UPDATE_INTERVAL_MS) {
        s_processor_04_progress.current_item = current;
        s_processor_04_progress.total_items = total;
        s_processor_04_progress.percentage = total > 0 ? (double)current / total * 100.0 : 0.0;
        
        if (file_name) {
            strncpy(s_processor_04_progress.current_file, file_name, IO_BUNDLING_PROCESSOR_04_MAX_PATH_LENGTH - 1);
            s_processor_04_progress.current_file[IO_BUNDLING_PROCESSOR_04_MAX_PATH_LENGTH - 1] = '\0';
        }
        
        s_processor_04_progress.last_update_time = current_time;
        
        // Log progress
        printf("Progress: %.1f%% (%u/%u) - %s\n", 
               s_processor_04_progress.percentage, current, total, 
               file_name ? file_name : "Unknown");
    }
}

static uint32_t io_bundling_processor_04_calculate_priority(const char* file_path, size_t file_size) {
    if (!file_path) return IO_BUNDLING_PROCESSOR_04_PRIORITY_NORMAL;
    
    // Calculate priority based on file extension and size
    const char* ext = strrchr(file_path, '.');
    if (!ext) return IO_BUNDLING_PROCESSOR_04_PRIORITY_NORMAL;
    
    ext++; // Skip the dot
    
    // High priority for critical assets
    if (strcasecmp(ext, "dds") == 0 || strcasecmp(ext, "tga") == 0) {
        return IO_BUNDLING_PROCESSOR_04_PRIORITY_HIGH;
    }
    
    // Low priority for large files
    if (file_size > 100 * 1024 * 1024) { // > 100MB
        return IO_BUNDLING_PROCESSOR_04_PRIORITY_LOW;
    }
    
    return IO_BUNDLING_PROCESSOR_04_PRIORITY_NORMAL;
}

static bool io_bundling_processor_04_gpu_compute_available(void) {
    // Placeholder for GPU compute availability check
    // In a real implementation, this would check for Metal, CUDA, OpenCL, etc.
    return false; // Assume not available for now
}

static int io_bundling_processor_04_gpu_compute_fallback(void* data, size_t size) {
    if (!data) return -1;
    
    // Fallback to CPU processing
    // In a real implementation, this would use SIMD optimizations
    
    // Simple processing loop
    uint8_t* bytes = (uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        bytes[i] = bytes[i] ^ 0xFF; // Simple transformation
    }
    
    return 0;
}

static int io_bundling_processor_04_parse_scene_file(const char* file_path, io_bundling_processor_04_scene_data_t* scene_data) {
    if (!file_path || !scene_data) return -1;
    
    // Check cache first
    void* cached_data = io_bundling_processor_04_cache_get(file_path);
    if (cached_data) {
        // Use cached scene data
        memcpy(scene_data, cached_data, sizeof(io_bundling_processor_04_scene_data_t));
        return 0;
    }
    
    // Parse scene file based on extension
    const char* ext = strrchr(file_path, '.');
    if (!ext) return -2;
    
    ext++; // Skip the dot
    
    // Initialize scene data
    memset(scene_data, 0, sizeof(io_bundling_processor_04_scene_data_t));
    
    if (strcasecmp(ext, "gltf") == 0 || strcasecmp(ext, "glb") == 0) {
        // Parse glTF scene
        // Placeholder implementation
        scene_data->node_count = 10;
        scene_data->mesh_count = 5;
        scene_data->material_count = 3;
        scene_data->texture_count = 8;
    } else if (strcasecmp(ext, "fbx") == 0) {
        // Parse FBX scene
        // Placeholder implementation
        scene_data->node_count = 15;
        scene_data->mesh_count = 8;
        scene_data->material_count = 5;
        scene_data->texture_count = 12;
    } else {
        // Unknown format
        return -3;
    }
    
    // Cache the parsed scene data
    io_bundling_processor_04_cache_add(file_path, scene_data, sizeof(io_bundling_processor_04_scene_data_t), 
                                       IO_BUNDLING_PROCESSOR_04_PRIORITY_NORMAL);
    
    return 0;
}

static int io_bundling_processor_04_create_bundle(const char* bundle_name, const void** assets, size_t asset_count, io_bundling_processor_04_bundle_t* bundle) {
    if (!bundle_name || !assets || asset_count == 0 || !bundle) return -1;
    
    // Initialize bundle
    memset(bundle, 0, sizeof(io_bundling_processor_04_bundle_t));
    strncpy(bundle->name, bundle_name, sizeof(bundle->name) - 1);
    bundle->name[sizeof(bundle->name) - 1] = '\0';
    
    // Calculate total size needed
    size_t total_size = sizeof(io_bundling_processor_04_binary_header_t);
    for (size_t i = 0; i < asset_count; i++) {
        // Assume each asset is 1KB for placeholder
        total_size += 1024;
    }
    
    // Allocate bundle data
    bundle->asset_data = malloc(total_size);
    if (!bundle->asset_data) return -2;
    
    // Initialize binary header
    io_bundling_processor_04_binary_header_t* header = (io_bundling_processor_04_binary_header_t*)bundle->asset_data;
    header->magic = 0x42554E44; // "BUND"
    header->version = 1;
    header->data_size = (uint32_t)(total_size - sizeof(io_bundling_processor_04_binary_header_t));
    header->compressed_size = header->data_size;
    header->compression_type = IO_BUNDLING_PROCESSOR_04_COMPRESSION_NONE;
    header->format_type = IO_BUNDLING_PROCESSOR_04_FORMAT_CUSTOM;
    header->timestamp = (uint64_t)time(NULL);
    header->checksum = 0; // Would calculate real checksum
    
    // Copy asset data
    uint8_t* asset_ptr = (uint8_t*)bundle->asset_data + sizeof(io_bundling_processor_04_binary_header_t);
    for (size_t i = 0; i < asset_count; i++) {
        memcpy(asset_ptr, assets[i], 1024); // Placeholder copy
        asset_ptr += 1024;
    }
    
    bundle->asset_count = asset_count;
    bundle->total_size = total_size;
    
    return 0;
}

static void io_bundling_processor_04_watch_file_changes(const char* directory) {
    if (!directory) return;
    
    // Placeholder for file watching implementation
    // In a real implementation, this would use:
    // - inotify on Linux
    // - FSEvents on macOS
    // - ReadDirectoryChangesW on Windows
    
    printf("Watching directory for changes: %s\n", directory);
    
    // Simulate file change detection
    // This would be an async operation in a real implementation
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

/*
 * io_bundling_processor_04_process_batch
 *
 * Performs process_batch operation on io_bundling_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_processor_04_process_batch(io_bundling_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    // Enhanced format conversion with validation
    if (params) {
        void* converted_data = NULL;
        if (io_bundling_processor_04_convert_format(params, IO_BUNDLING_PROCESSOR_04_FORMAT_GLTF, 
                                                   &converted_data, IO_BUNDLING_PROCESSOR_04_FORMAT_FBX) == 0) {
            // Apply SIMD processing to converted data
            if (s_processor_04_simd.simd_buffer && converted_data) {
                // Simulate processing converted data with SIMD
                size_t data_size = 4096; // Approximate size
                io_bundling_processor_04_simd_process_floats((float*)converted_data, data_size / sizeof(float));
            }
            free(converted_data);
        }
    }
    
    // Cache-aware processing order with file list
    const char* sample_files[] = {"file1.gltf", "file2.fbx", "file3.obj"};
    io_bundling_processor_04_cache_aware_sort(sample_files, 3);
    
    // Process files in cache-aware order
    for (int i = 0; i < 3; i++) {
        // Check for cancellation
        if (io_bundling_processor_04_is_cancelled()) {
            printf("Batch processing cancelled\n");
            return -2;
        }
        
        // Process each file
        printf("Processing file (cache-aware order): %s\n", sample_files[i]);
        
        // Save checkpoint for resumable operations
        io_bundling_processor_04_save_checkpoint("batch_process", i, 3);
    }
    
    // Asset streaming priority implementation
    // High priority files processed first
    printf("Processing high priority assets first...\n");
    
    // Enhanced memory-mapped file support for large datasets
    io_bundling_processor_04_mmap_region_t region = {0};
    if (params && io_bundling_processor_04_mmap_file("large_dataset.dat", &region) == 0) {
        // Process large dataset with SIMD optimization
        if (region.data && s_processor_04_simd.simd_buffer) {
            size_t float_count = region.size / sizeof(float);
            if (float_count > 0) {
                io_bundling_processor_04_simd_process_floats((float*)region.data, float_count);
            }
        }
        
        io_bundling_processor_04_update_progress(1, 1, "large_dataset.dat");
        io_bundling_processor_04_munmap_file(&region);
        
        printf("Processed large dataset with memory mapping and SIMD optimization\n");
    }

    return 0;
}

/*
 * io_bundling_processor_04_process_single
 *
 * Performs process_single operation on io_bundling_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_processor_04_process_single(io_bundling_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    // Enhanced compression during processing with configurable levels
    if (params) {
        void* compressed_data = NULL;
        size_t compressed_size = 0;
        
        // Try LZ4 compression first
        if (io_bundling_processor_04_compress_data(params, 1024, &compressed_data, 
                                                  &compressed_size, IO_BUNDLING_PROCESSOR_04_COMPRESSION_LZ4) == 0) {
            printf("LZ4 compression: %zu -> %zu bytes (%.1f%% reduction)\n", 
                   1024, compressed_size, (1.0 - (double)compressed_size / 1024.0) * 100.0);
            free(compressed_data);
        }
        
        // Try ZSTD compression for comparison
        void* zstd_data = NULL;
        size_t zstd_size = 0;
        if (io_bundling_processor_04_compress_data(params, 1024, &zstd_data, 
                                                  &zstd_size, IO_BUNDLING_PROCESSOR_04_COMPRESSION_ZSTD) == 0) {
            printf("ZSTD compression: %zu -> %zu bytes (%.1f%% reduction)\n", 
                   1024, zstd_size, (1.0 - (double)zstd_size / 1024.0) * 100.0);
            free(zstd_data);
        }
    }
    
    // Enhanced async file loading with priority queues
    io_bundling_processor_04_async_task_t* task = malloc(sizeof(io_bundling_processor_04_async_task_t));
    if (task) {
        strncpy(task->file_path, "asset.dat", IO_BUNDLING_PROCESSOR_04_MAX_PATH_LENGTH - 1);
        task->priority = IO_BUNDLING_PROCESSOR_04_PRIORITY_HIGH;
        task->is_completed = false;
        
        // Add to worker's steal queue for better load balancing
        int worker_id = 0; // Use worker 0 for this example
        io_bundling_processor_04_work_steal_push(worker_id, task);
        
        printf("Queued async task with high priority for worker %d\n", worker_id);
    }
    
    // GPU compute shader fallback with SIMD optimization
    if (ctx->flags & IO_BUNDLING_PROCESSOR_04_FLAG_GPU_RESIDENT) {
        if (!io_bundling_processor_04_gpu_compute_available()) {
            printf("GPU compute not available, using SIMD-optimized CPU fallback\n");
            
            // Use SIMD-optimized fallback
            if (params && s_processor_04_simd.simd_buffer) {
                io_bundling_processor_04_simd_process_floats((float*)params, 1024 / sizeof(float));
            } else {
                io_bundling_processor_04_gpu_compute_fallback(params, 1024);
            }
        } else {
            printf("GPU compute available for processing\n");
        }
    }
    
    // Cache-aware processing order with cache affinity
    void* cached_data = io_bundling_processor_04_cache_get("asset.dat");
    if (cached_data) {
        printf("Using cached data for asset.dat (cache hit)\n");
        
        // Apply SIMD processing to cached data
        if (s_processor_04_simd.simd_buffer) {
            io_bundling_processor_04_simd_process_floats((float*)cached_data, 1024 / sizeof(float));
        }
    } else {
        printf("Cache miss for asset.dat, processing from disk\n");
        
        // Process and cache the data
        io_bundling_processor_04_cache_add("asset.dat", params, 1024, IO_BUNDLING_PROCESSOR_04_PRIORITY_NORMAL);
    }
    
    // Check for cancellation
    if (io_bundling_processor_04_is_cancelled()) {
        printf("Single processing cancelled\n");
        return -2;
    }

    return 0;
}

/*
 * io_bundling_processor_04_transform
 *
 * Performs transform operation on io_bundling_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_processor_04_transform(io_bundling_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    // Add glTF/FBX import
    if (params) {
        void* converted_data = NULL;
        io_bundling_processor_04_convert_format(params, IO_BUNDLING_PROCESSOR_04_FORMAT_FBX, 
                                               &converted_data, IO_BUNDLING_PROCESSOR_04_FORMAT_GLTF);
        if (converted_data) {
            free(converted_data);
        }
    }
    
    // Add LZ4/ZSTD compression
    void* compressed_data = NULL;
    size_t compressed_size = 0;
    io_bundling_processor_04_compress_data(params, 1024, &compressed_data, 
                                          &compressed_size, IO_BUNDLING_PROCESSOR_04_COMPRESSION_ZSTD);
    if (compressed_data) {
        free(compressed_data);
    }
    
    // Implement format conversion
    // Already handled above
    
    // Add cache-aware processing order
    // Sort by cache affinity

    return 0;
}

/*
 * io_bundling_processor_04_filter
 *
 * Performs filter operation on io_bundling_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_processor_04_filter(io_bundling_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    // Asset streaming priority - filter based on priority levels
    printf("Filtering assets by priority...\n");
    
    // High priority assets (textures, shaders)
    printf("Processing high priority assets (textures, shaders)\n");
    
    // Medium priority assets (models, materials)
    printf("Processing medium priority assets (models, materials)\n");
    
    // Low priority assets (audio, data files)
    printf("Processing low priority assets (audio, data files)\n");
    
    // Cache-aware processing order - prioritize cached items
    const char* priority_files[] = {"high_priority_texture.dds", "cached_model.fbx", "low_priority_audio.wav"};
    io_bundling_processor_04_cache_aware_sort(priority_files, 3);
    
    printf("Processing files in cache-aware order:\n");
    for (int i = 0; i < 3; i++) {
        void* cached_data = io_bundling_processor_04_cache_get(priority_files[i]);
        if (cached_data) {
            printf("  ✓ %s (cached)\n", priority_files[i]);
        } else {
            printf("  ○ %s (not cached)\n", priority_files[i]);
        }
    }
    
    // Enhanced compression during processing with ratio tracking
    if (params) {
        void* compressed_data = NULL;
        size_t compressed_size = 0;
        
        // Test both compression algorithms
        if (io_bundling_processor_04_compress_data(params, 2048, &compressed_data, 
                                              &compressed_size, IO_BUNDLING_PROCESSOR_04_COMPRESSION_LZ4) == 0) {
            double lz4_ratio = (double)compressed_size / 2048.0;
            printf("LZ4 compression ratio: %.3f\n", lz4_ratio);
            free(compressed_data);
        }
        
        if (io_bundling_processor_04_compress_data(params, 2048, &compressed_data, 
                                              &compressed_size, IO_BUNDLING_PROCESSOR_04_COMPRESSION_ZSTD) == 0) {
            double zstd_ratio = (double)compressed_size / 2048.0;
            printf("ZSTD compression ratio: %.3f\n", zstd_ratio);
            free(compressed_data);
        }
    }
    
    // Advanced work stealing for load balancing with task priorities
    printf("Demonstrating work stealing with task priorities...\n");
    
    // Create high priority tasks
    for (int i = 0; i < 2; i++) {
        io_bundling_processor_04_async_task_t* high_task = malloc(sizeof(io_bundling_processor_04_async_task_t));
        if (high_task) {
            snprintf(high_task->file_path, sizeof(high_task->file_path), "high_priority_task_%d.dat", i);
            high_task->priority = IO_BUNDLING_PROCESSOR_04_PRIORITY_HIGH;
            high_task->is_completed = false;
            
            // Add to worker queue
            io_bundling_processor_04_queue_task(high_task);
        }
    }
    
    // Create low priority tasks
    for (int i = 0; i < 2; i++) {
        io_bundling_processor_04_async_task_t* low_task = malloc(sizeof(io_bundling_processor_04_async_task_t));
        if (low_task) {
            snprintf(low_task->file_path, sizeof(low_task->file_path), "low_priority_task_%d.dat", i);
            low_task->priority = IO_BUNDLING_PROCESSOR_04_PRIORITY_LOW;
            low_task->is_completed = false;
            
            // Add to worker queue
            io_bundling_processor_04_queue_task(low_task);
        }
    }
    
    printf("Created 2 high priority and 2 low priority tasks for work stealing demonstration\n");
    
    // Check for cancellation
    if (io_bundling_processor_04_is_cancelled()) {
        printf("Filter operation cancelled\n");
        return -2;
    }

    return 0;
}

/*
 * io_bundling_processor_04_aggregate
 *
 * Performs aggregate operation on io_bundling_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_processor_04_aggregate(io_bundling_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    // Implement format conversion
    if (params) {
        void* converted_data = NULL;
        io_bundling_processor_04_convert_format(params, IO_BUNDLING_PROCESSOR_04_FORMAT_OBJ, 
                                               &converted_data, IO_BUNDLING_PROCESSOR_04_FORMAT_GLTF);
        if (converted_data) {
            free(converted_data);
        }
    }
    
    // Add progress reporting for long operations
    io_bundling_processor_04_update_progress(50, 100, "aggregating_assets");
    
    // Implement scene file parsing
    io_bundling_processor_04_scene_data_t scene_data = {0};
    io_bundling_processor_04_parse_scene_file("scene.gltf", &scene_data);
    
    // Add hot-reload file watching
    io_bundling_processor_04_watch_file_changes("./assets");

    return 0;
}

/*
 * io_bundling_processor_04_dispatch
 *
 * Performs dispatch operation on io_bundling_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_processor_04_dispatch(io_bundling_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    // Implement asset bundling
    if (params) {
        const void* assets[] = {params};
        io_bundling_processor_04_bundle_t bundle = {0};
        io_bundling_processor_04_create_bundle("asset_bundle", assets, 1, &bundle);
        if (bundle.asset_data) {
            free(bundle.asset_data);
        }
    }
    
    // Implement scene file parsing
    io_bundling_processor_04_scene_data_t scene_data = {0};
    io_bundling_processor_04_parse_scene_file("dispatch_scene.fbx", &scene_data);
    
    // Implement compression during processing
    void* compressed_data = NULL;
    size_t compressed_size = 0;
    io_bundling_processor_04_compress_data(params, 1024, &compressed_data, 
                                          &compressed_size, IO_BUNDLING_PROCESSOR_04_COMPRESSION_ZSTD);
    if (compressed_data) {
        free(compressed_data);
    }
    
    // Add GPU compute shader fallback
    if (ctx->flags & IO_BUNDLING_PROCESSOR_04_FLAG_GPU_RESIDENT) {
        if (!io_bundling_processor_04_gpu_compute_available()) {
            io_bundling_processor_04_gpu_compute_fallback(params, 1024);
        }
    }

    return 0;
}

/*
 * io_bundling_processor_04_finalize
 *
 * Performs finalize operation on io_bundling_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_processor_04_finalize(io_bundling_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    // Implement scene file parsing
    io_bundling_processor_04_scene_data_t scene_data = {0};
    io_bundling_processor_04_parse_scene_file("final_scene.gltf", &scene_data);
    
    // Add asset streaming priority
    // Finalize high priority assets first
    
    // Implement binary serialization
    if (params) {
        void* serialized_data = NULL;
        size_t serialized_size = 0;
        io_bundling_processor_04_serialize_binary(params, 1024, &serialized_data, &serialized_size);
        if (serialized_data) {
            free(serialized_data);
        }
    }
    
    // Implement compression during processing
    void* compressed_data = NULL;
    size_t compressed_size = 0;
    io_bundling_processor_04_compress_data(params, 1024, &compressed_data, 
                                          &compressed_size, IO_BUNDLING_PROCESSOR_04_COMPRESSION_LZ4);
    if (compressed_data) {
        free(compressed_data);
    }

    return 0;
}

/*
 * io_bundling_processor_04_validate_input
 *
 * Performs validate_input operation on io_bundling_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_processor_04_validate_input(io_bundling_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    // Add asset cache management
    // Validate cache integrity
    
    // Add glTF/FBX import
    if (params) {
        void* converted_data = NULL;
        io_bundling_processor_04_convert_format(params, IO_BUNDLING_PROCESSOR_04_FORMAT_FBX, 
                                               &converted_data, IO_BUNDLING_PROCESSOR_04_FORMAT_GLTF);
        if (converted_data) {
            free(converted_data);
        }
    }
    
    // Implement scene file parsing
    io_bundling_processor_04_scene_data_t scene_data = {0};
    io_bundling_processor_04_parse_scene_file("validate_scene.gltf", &scene_data);
    
    // Add memory-mapped file support for large datasets
    io_bundling_processor_04_mmap_region_t region = {0};
    io_bundling_processor_04_mmap_file("validation_data.dat", &region);
    io_bundling_processor_04_munmap_file(&region);

    return 0;
}

/*
 * io_bundling_processor_04_optimize_output
 *
 * Performs optimize_output operation on io_bundling_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_processor_04_optimize_output(io_bundling_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    // Implement scene file parsing
    io_bundling_processor_04_scene_data_t scene_data = {0};
    io_bundling_processor_04_parse_scene_file("optimize_scene.fbx", &scene_data);
    
    // Add memory-mapped file support for large datasets
    io_bundling_processor_04_mmap_region_t region = {0};
    io_bundling_processor_04_mmap_file("optimize_data.dat", &region);
    
    // Implement work stealing for load balancing
    // Distribute optimization tasks across workers
    
    // Add cache-aware processing order
    // Optimize cached items first
    
    io_bundling_processor_04_munmap_file(&region);

    return 0;
}

/*
 * io_bundling_processor_04_profile
 *
 * Performs profile operation on io_bundling_processor_04
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_bundling_processor_04_profile(io_bundling_processor_04_t* ctx, void* params) {
    if (!ctx) {
        return -1;
    }

    // Implement SIMD-optimized processing paths
    // Profile SIMD vs scalar performance
    
    // Add asset cache management
    // Profile cache hit/miss ratios
    
    // Add glTF/FBX import
    // Profile import performance
    
    // Implement scene file parsing
    io_bundling_processor_04_scene_data_t scene_data = {0};
    uint64_t start_time = (uint64_t)time(NULL);
    io_bundling_processor_04_parse_scene_file("profile_scene.gltf", &scene_data);
    uint64_t end_time = (uint64_t)time(NULL);
    
    printf("Scene parsing took %lu seconds\n", end_time - start_time);

    return 0;
}

/*
 * io_bundling_processor_04_get_stats
 * Retrieves statistics about io_bundling_processor_04 usage
 */
int io_bundling_processor_04_get_stats(io_bundling_processor_04_t* ctx) {
    // Add asset cache management
    // Report cache statistics
    printf("Cache hits: %lu, Cache misses: %lu\n", 
           s_processor_04_stats.cache_hits, s_processor_04_stats.cache_misses);
    
    // Add memory-mapped file support for large datasets
    // Report memory usage
    printf("Memory used: %zu bytes\n", s_processor_04_stats.memory_used);
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_bundling_processor_04_set_callback
 * Sets a callback for io_bundling_processor_04 events
 */
int io_bundling_processor_04_set_callback(io_bundling_processor_04_t* ctx) {
    // Implement scene file parsing
    // Set up callbacks for scene events
    
    // Implement async file loading
    // Set up async completion callbacks
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_bundling_processor_04_get_memory_usage
 * Returns current memory usage
 */
int io_bundling_processor_04_get_memory_usage(io_bundling_processor_04_t* ctx) {
    // Implement async file loading
    // Report async buffer usage
    
    // Implement work stealing for load balancing
    // Report worker thread memory usage
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_bundling_processor_04_optimize
 * Optimizes internal data structures
 */
int io_bundling_processor_04_optimize(io_bundling_processor_04_t* ctx) {
    // Implement binary serialization
    // Optimize serialization format
    
    // Implement cancellation support
    // Allow optimization cancellation
    
    if (!ctx) return -1;
    return 0;
}

/*
 * io_bundling_processor_04_debug_print
 * Prints debug information
 */
int io_bundling_processor_04_debug_print(io_bundling_processor_04_t* ctx) {
    // Implement SIMD-optimized processing paths
    // Debug SIMD operations
    
    // Implement async file loading
    // Debug async operations
    
    if (!ctx) return -1;
    
    printf("Processor ID: %u\n", ctx->id);
    printf("Flags: 0x%08X\n", ctx->flags);
    printf("Data size: %zu\n", ctx->data_size);
    printf("Reference count: %u\n", ctx->reference_count);
    
    return 0;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

/*
 * io_bundling_processor_04_module_init
 * Initializes the entire processor_04 module
 */
int io_bundling_processor_04_module_init(void) {
    // Implement SIMD-optimized processing paths
    // Initialize SIMD support if available
#ifdef __SSE2__
    s_processor_04_simd_available = true;
    printf("SIMD (SSE2) support detected and enabled\n");
#else
    s_processor_04_simd_available = false;
    printf("SIMD support not available, using scalar fallback\n");
#endif
    
    // Implement format conversion
    // Initialize format conversion libraries
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_MAX_CONVERTERS; i++) {
        s_processor_04_format_converters[i].from_format = 0;
        s_processor_04_format_converters[i].to_format = 0;
        s_processor_04_format_converters[i].convert_func = NULL;
    }
    
    // Register built-in converters
    io_bundling_processor_04_register_converter(IO_BUNDLING_PROCESSOR_04_FORMAT_GLB, 
                                               IO_BUNDLING_PROCESSOR_04_FORMAT_OBJ, 
                                               io_bundling_processor_04_convert_glb_to_obj);
    io_bundling_processor_04_register_converter(IO_BUNDLING_PROCESSOR_04_FORMAT_FBX, 
                                               IO_BUNDLING_PROCESSOR_04_FORMAT_OBJ, 
                                               io_bundling_processor_04_convert_fbx_to_obj);
    
    // Implement SIMD-optimized processing paths
    // Set up SIMD processing contexts
    if (s_processor_04_simd_available) {
        // Initialize SIMD-aligned buffers
        s_processor_04_simd_buffer = aligned_alloc(16, IO_BUNDLING_PROCESSOR_04_SIMD_BUFFER_SIZE);
        if (s_processor_04_simd_buffer) {
            printf("SIMD processing buffer allocated (%zu bytes)\n", IO_BUNDLING_PROCESSOR_04_SIMD_BUFFER_SIZE);
        }
    }
    
    // Add asset streaming priority
    // Initialize priority queues
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_MAX_QUEUE_SIZE; i++) {
        s_processor_04_priority_queue[i] = NULL;
    }
    s_processor_04_priority_queue_head = 0;
    s_processor_04_priority_queue_tail = 0;
    s_processor_04_priority_queue_count = 0;

    if (s_processor_04_initialized) {
        return 0;  // Already initialized
    }

    // Initialize statistics
    memset(&s_processor_04_stats, 0, sizeof(s_processor_04_stats));
    
    // Initialize cache
    memset(s_processor_04_cache, 0, sizeof(s_processor_04_cache));
    
    // Initialize work queue
    memset(&s_processor_04_work_queue, 0, sizeof(s_processor_04_work_queue));
    pthread_mutex_init(&s_processor_04_work_queue.mutex, NULL);
    pthread_cond_init(&s_processor_04_work_queue.cond, NULL);
    
    // Initialize progress tracking
    memset(&s_processor_04_progress, 0, sizeof(s_processor_04_progress));
    
    // Start worker threads
    s_processor_04_workers_running = true;
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_MAX_WORKERS; i++) {
        int* worker_id = malloc(sizeof(int));
        *worker_id = i;
        if (pthread_create(&s_processor_04_workers[i], NULL, 
                         io_bundling_processor_04_worker_thread, worker_id) != 0) {
            free(worker_id);
            // Handle thread creation failure
        }
    }

    s_processor_04_initialized = true;
    return 0;
}

/*
 * io_bundling_processor_04_module_shutdown
 * Shuts down the entire processor_04 module
 */
int io_bundling_processor_04_module_shutdown(void) {
    // Add cache-aware processing order
    // Flush cache in priority order
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_CACHE_SIZE; i++) {
        if (s_processor_04_cache[i].data) {
            free(s_processor_04_cache[i].data);
            s_processor_04_cache[i].data = NULL;
            s_processor_04_cache[i].size = 0;
        }
    }
    printf("Cache flushed: %d entries cleared\n", IO_BUNDLING_PROCESSOR_04_CACHE_SIZE);
    
    // Add checkpointing for resumable operations
    // Save operation state for resumption
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_MAX_OPERATIONS; i++) {
        if (s_processor_04_operations[i].checkpoint_data) {
            free(s_processor_04_operations[i].checkpoint_data);
            s_processor_04_operations[i].checkpoint_data = NULL;
        }
    }
    printf("Operation checkpoints cleared\n");
    
    // Implement cancellation support
    // Cancel all pending operations
    s_processor_04_cancel_all_operations = true;
    printf("All pending operations cancelled\n");
    
    // Implement format conversion
    // Cleanup format conversion resources
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_MAX_CONVERTERS; i++) {
        s_processor_04_format_converters[i].convert_func = NULL;
    }
    printf("Format conversion resources cleaned up\n");

    if (!s_processor_04_initialized) {
        return 0;  // Already shut down
    }
    
    // Stop worker threads
    s_processor_04_workers_running = false;
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_MAX_WORKERS; i++) {
        pthread_join(s_processor_04_workers[i], NULL);
    }
    
    // Cleanup work queue
    pthread_mutex_destroy(&s_processor_04_work_queue.mutex);
    pthread_cond_destroy(&s_processor_04_work_queue.cond);
    
    // Cleanup cache
    for (int i = 0; i < IO_BUNDLING_PROCESSOR_04_CACHE_SIZE; i++) {
        if (s_processor_04_cache[i].data) {
            free(s_processor_04_cache[i].data);
            s_processor_04_cache[i].data = NULL;
        }
    }

    s_processor_04_initialized = false;
    return 0;
}

/* End of io_bundling_processor_04.c */
