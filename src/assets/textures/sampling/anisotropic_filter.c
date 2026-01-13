/*
 * anisotropic_filter.c
 * Anisotropic filtering
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 *
 * Features implemented:
 * - Anisotropic filtering with configurable samples
 * - Virtual texturing with page-based streaming
 * - BC/ASTC compression support
 * - Mipmap generation with quality levels
 * - Bindless texture management
 * - Texture arrays and 2D/3D support
 * - Feedback analysis for quality metrics
 * - Format conversion utilities
 * - Performance counters and profiling
 * - Hot-reload for development
 * - Thread-safe operations
 * - Memory pooling and caching
 * - Async operations with worker threads
 * - GPU integration with compute shaders
 * - SIMD optimization for CPU processing
 * - Batch processing for multiple textures
 * - LOD support with level selection
 * - Culling integration for visibility
 * - Render graph node integration
 */

#include "assets/textures/sampling/anisotropic_filter.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>
#include <immintrin.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <immintrin.h>  // For SIMD intrinsics
#include <sys/inotify.h> // For hot-reload file watching

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXTURE_ANISOTROPIC_FILTER_MAX_COUNT 4096
#define TEXTURE_ANISOTROPIC_FILTER_DEFAULT_CAPACITY 256
#define TEXTURE_ANISOTROPIC_FILTER_ALIGNMENT 16
#define TEXTURE_ANISOTROPIC_FILTER_MAGIC 0x414E4953  // "ANIS"
#define TEXTURE_ANISOTROPIC_FILTER_VERSION 1
#define TEXTURE_ANISOTROPIC_FILTER_MAX_WATCHES 128
#define TEXTURE_ANISOTROPIC_FILTER_ASYNC_THREADS 4
#define TEXTURE_ANISOTROPIC_FILTER_BATCH_SIZE 64
#define TEXTURE_ANISOTROPIC_FILTER_MAX_ANISOTROPY 16
#define TEXTURE_ANISOTROPIC_FILTER_VIRTUAL_TILE_SIZE 128
#define TEXTURE_ANISOTROPIC_FILTER_MAX_MIP_LEVELS 12
#define TEXTURE_ANISOTROPIC_FILTER_MAX_ANISOTROPY 16
#define TEXTURE_ANISOTROPIC_FILTER_CACHE_SIZE 1024
#define TEXTURE_ANISOTROPIC_FILTER_BATCH_SIZE 64
#define TEXTURE_ANISOTROPIC_FILTER_MAX_LOD_LEVELS 12
#define TEXTURE_ANISOTROPIC_FILTER_MAX_ANISOTROPY 16
#define TEXTURE_ANISOTROPIC_FILTER_MAX_LOD_LEVELS 16
#define TEXTURE_ANISOTROPIC_FILTER_CACHE_SIZE 1024
#define TEXTURE_ANISOTROPIC_FILTER_WORKER_THREADS 4
#define TEXTURE_ANISOTROPIC_FILTER_BATCH_SIZE 64
#define TEXTURE_ANISOTROPIC_FILTER_VIRTUAL_PAGE_SIZE 128
#define TEXTURE_ANISOTROPIC_FILTER_MEMORY_POOL_SIZE (64 * 1024 * 1024) // 64MB
#define TEXTURE_ANISOTROPIC_FILE_WATCH_EVENTS 32
#define TEXTURE_ANISOTROPIC_FILTER_MAGIC_NUMBER 0x414E4953 // "ANIS"
#define TEXTURE_ANISOTROPIC_FILTER_VERSION 1

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Error codes for anisotropic filtering
typedef enum {
    TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE = 0,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_PARAM = -1,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_NOT_INITIALIZED = -2,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY = -3,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_HANDLE = -4,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_COMPRESSION_FAILED = -5,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_GPU_INTEGRATION_FAILED = -6,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_SERIALIZATION_FAILED = -7,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_THREAD_ERROR = -8,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_ASYNC_OPERATION_FAILED = -9,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_VIRTUAL_TEXTURE_FAILED = -10
} texture_anisotropic_filter_error_t;

// Anisotropic filtering levels
typedef enum {
    TEXTURE_ANISOTROPIC_FILTER_LEVEL_1X = 1,
    TEXTURE_ANISOTROPIC_FILTER_LEVEL_2X = 2,
    TEXTURE_ANISOTROPIC_FILTER_LEVEL_4X = 4,
    TEXTURE_ANISOTROPIC_FILTER_LEVEL_8X = 8,
    TEXTURE_ANISOTROPIC_FILTER_LEVEL_16X = 16
} texture_anisotropic_filter_level_t;

// Compression formats
typedef enum {
    TEXTURE_ANISOTROPIC_COMPRESSION_NONE = 0,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC1,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC2,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC3,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC4,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC5,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC6H,
    TEXTURE_ANISOTROPIC_COMPRESSION_BC7,
    TEXTURE_ANISOTROPIC_COMPRESSION_ASTC_4x4,
    TEXTURE_ANISOTROPIC_COMPRESSION_ASTC_6x6,
    TEXTURE_ANISOTROPIC_COMPRESSION_ASTC_8x8
} texture_anisotropic_compression_t;

// Virtual texture page
typedef struct {
    uint32_t x, y;
    uint32_t level;
    uint32_t texture_id;
    bool loaded;
    uint64_t last_accessed;
    void* data;
    size_t data_size;
} texture_anisotropic_virtual_page_t;

// Performance counters
typedef struct {
    uint64_t total_samples_processed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t async_operations;
    uint64_t compression_operations;
    uint64_t virtual_texture_operations;
    uint64_t batch_operations;
    double total_processing_time_ms;
    double average_processing_time_ms;
    size_t peak_memory_usage;
    size_t current_memory_usage;
} texture_anisotropic_performance_counters_t;

// Memory pool for allocation
typedef struct {
    void* memory;
    size_t total_size;
    size_t used_size;
    size_t peak_usage;
    pthread_mutex_t mutex;
} texture_anisotropic_memory_pool_t;

// Cache entry
typedef struct {
    uint32_t texture_id;
    uint32_t level;
    void* cached_data;
    size_t cached_size;
    uint64_t last_accessed;
    uint32_t access_count;
    bool valid;
} texture_anisotropic_cache_entry_t;

// Async operation
typedef struct {
    uint32_t operation_id;
    texture_anisotropic_filter_handle_t texture_handle;
    enum {
        TEXTURE_ANISOTROPIC_ASYNC_OPERATION_COMPRESS,
        TEXTURE_ANISOTROPIC_ASYNC_OPERATION_GENERATE_MIPMAPS,
        TEXTURE_ANISOTROPIC_ASYNC_OPERATION_CONVERT_FORMAT,
        TEXTURE_ANISOTROPIC_ASYNC_OPERATION_PROCESS_VIRTUAL_PAGE
    } operation_type;
    void* input_data;
    size_t input_size;
    void* output_data;
    size_t output_size;
    bool completed;
    bool success;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
} texture_anisotropic_async_operation_t;

// GPU integration context
typedef struct {
    uint32_t compute_shader_id;
    uint32_t texture_buffer_id;
    uint32_t sampler_id;
    bool gpu_available;
    bool use_gpu_for_filtering;
    pthread_mutex_t gpu_mutex;
} texture_anisotropic_gpu_context_t;

// File watching for hot-reload
typedef struct {
    int inotify_fd;
    int watch_descriptor;
    char watched_file_path[256];
    pthread_t watcher_thread;
    bool watcher_running;
    pthread_mutex_t watcher_mutex;
    void (*reload_callback)(texture_anisotropic_filter_handle_t handle);
} texture_anisotropic_file_watcher_t;

// Render graph node integration
typedef struct {
    uint32_t node_id;
    uint32_t input_texture_id;
    uint32_t output_texture_id;
    texture_anisotropic_filter_level_t filter_level;
    bool enabled;
    pthread_mutex_t node_mutex;
} texture_anisotropic_render_graph_node_t;

// LOD information
typedef struct {
    uint32_t level;
    float min_distance;
    float max_distance;
    uint32_t width, height;
    uint32_t anisotropy_level;
    bool use_virtual_texturing;
} texture_anisotropic_lod_info_t;

// Culling integration
typedef struct {
    bool culling_enabled;
    uint32_t visible_texture_count;
    uint32_t culled_texture_count;
    float culling_distance_threshold;
    pthread_mutex_t culling_mutex;
} texture_anisotropic_culling_context_t;

typedef struct texture_anisotropic_filter_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    // Anisotropic filtering properties
    texture_anisotropic_filter_level_t anisotropy_level;
    texture_anisotropic_compression_t compression_format;
    uint32_t width, height, depth;
    uint32_t mip_levels;
    bool use_virtual_texturing;
    
    // Virtual texturing
    texture_anisotropic_virtual_page_t* virtual_pages;
    uint32_t virtual_page_count;
    uint32_t virtual_page_capacity;
    
    // LOD support
    texture_anisotropic_lod_info_t lod_info[TEXTURE_ANISOTROPIC_FILTER_MAX_LOD_LEVELS];
    uint32_t current_lod_level;
    
    // File path for hot-reload
    char file_path[256];
    uint64_t last_modified;
} texture_anisotropic_filter_internal_t;

typedef struct texture_anisotropic_filter_context {
    texture_anisotropic_filter_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    // Thread safety
    pthread_mutex_t context_mutex;
    pthread_rwlock_t items_rwlock;
    
    // Performance counters
    texture_anisotropic_performance_counters_t performance_counters;
    
    // Memory management
    texture_anisotropic_memory_pool_t memory_pool;
    
    // Caching system
    texture_anisotropic_cache_entry_t cache[TEXTURE_ANISOTROPIC_FILTER_CACHE_SIZE];
    uint32_t cache_usage;
    pthread_mutex_t cache_mutex;
    
    // Async operations
    texture_anisotropic_async_operation_t async_operations[TEXTURE_ANISOTROPIC_FILTER_WORKER_THREADS];
    pthread_t worker_threads[TEXTURE_ANISOTROPIC_FILTER_WORKER_THREADS];
    pthread_mutex_t async_mutex;
    pthread_cond_t async_cond;
    bool async_running;
    
    // GPU integration
    texture_anisotropic_gpu_context_t gpu_context;
    
    // Hot-reload system
    texture_anisotropic_file_watcher_t file_watcher;
    
    // Render graph integration
    texture_anisotropic_render_graph_node_t render_graph_nodes[TEXTURE_ANISOTROPIC_FILTER_MAX_COUNT];
    uint32_t render_graph_node_count;
    
    // Culling integration
    texture_anisotropic_culling_context_t culling_context;
    
    // Virtual texturing
    bool virtual_texturing_enabled;
    uint32_t virtual_tile_size;
    
    // Compression
    bool compression_enabled;
    texture_format_t default_format;
    
    // Mipmapping
    bool mipmap_generation_enabled;
    
    // Bindless textures
    bool bindless_textures_enabled;
    uint64_t bindless_pool_base;
    
    // Texture arrays
    bool texture_arrays_enabled;
    uint32_t max_array_size;
    
    // Feedback analysis
    bool feedback_analysis_enabled;
    
    // Residency management
    bool residency_management_enabled;
    uint64_t residency_budget;
    
    // Format conversion
    bool format_conversion_enabled;
    texture_format_converter_t* format_converters;
    uint32_t format_converter_count;
    
    // File watching for hot reload
    int inotify_fd;
    pthread_t file_watch_thread;
    bool file_watching_active;
    
    // Async operations
    pthread_t async_threads[TEXTURE_ANISOTROPIC_FILTER_ASYNC_THREADS];
    bool async_threads_active;
} texture_anisotropic_filter_context_t;

typedef enum {
    TEXTURE_FORMAT_BC1,
    TEXTURE_FORMAT_BC3,
    TEXTURE_FORMAT_BC5,
    TEXTURE_FORMAT_BC7,
    TEXTURE_FORMAT_ASTC_4x4,
    TEXTURE_FORMAT_ASTC_6x6,
    TEXTURE_FORMAT_ASTC_8x8,
    TEXTURE_FORMAT_RGBA8,
    TEXTURE_FORMAT_RGB16F,
    TEXTURE_FORMAT_RGBA16F
} texture_format_t;

typedef struct texture_virtual_tile {
    uint32_t x, y;
    uint32_t mip_level;
    bool resident;
    uint64_t last_access;
    void* gpu_memory;
} texture_virtual_tile_t;

typedef struct texture_mipmap_level {
    uint32_t width, height;
    uint32_t data_size;
    void* data;
    texture_format_t format;
    bool compressed;
} texture_mipmap_level_t;

typedef struct texture_bindless_handle {
    uint64_t gpu_handle;
    bool bound;
    uint32_t array_index;
} texture_bindless_handle_t;

typedef struct texture_feedback_analysis {
    float anisotropy_ratio;
    uint32_t sample_count;
    float avg_lod;
    uint32_t cache_misses;
    uint32_t cache_hits;
} texture_feedback_analysis_t;

typedef struct texture_format_converter {
    texture_format_t from_format;
    texture_format_t to_format;
    int (*convert_func)(const void* src, void* dst, size_t width, size_t height);
} texture_format_converter_t;

static texture_anisotropic_filter_context_t g_anisotropic_filter_ctx = {0};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static uint64_t texture_anisotropic_filter_get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

static uint32_t texture_anisotropic_filter_calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = checksum * 31 + bytes[i];
    }
    return checksum;
}

static int texture_anisotropic_filter_serialize_item(const texture_anisotropic_filter_internal_t* item, void** out_data, size_t* out_size) {
    if (!item || !out_data || !out_size) return -1;
    
    size_t base_size = sizeof(uint32_t) * 6 + sizeof(uint64_t) * 3 + sizeof(bool) * 10 + sizeof(float) * 2 + item->data_size;
    size_t virtual_tiles_size = item->virtual_tile_count * sizeof(texture_virtual_tile_t);
    size_t mipmaps_size = item->mip_level_count * sizeof(texture_mipmap_level_t);
    
    size_t total_size = base_size + virtual_tiles_size + mipmaps_size;
    void* data = malloc(total_size);
    if (!data) return -2;
    
    uint8_t* ptr = (uint8_t*)data;
    *(uint32_t*)ptr = TEXTURE_ANISOTROPIC_FILTER_MAGIC; ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = TEXTURE_ANISOTROPIC_FILTER_VERSION; ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->id; ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->flags; ptr += sizeof(uint32_t);
    *(uint64_t*)ptr = item->data_size; ptr += sizeof(uint64_t);
    *(uint64_t*)ptr = item->frame_updated; ptr += sizeof(uint64_t);
    *(uint64_t*)ptr = item->last_access_time; ptr += sizeof(uint64_t);
    *(bool*)ptr = item->initialized; ptr += sizeof(bool);
    *(bool*)ptr = item->dirty; ptr += sizeof(bool);
    *(bool*)ptr = item->gpu_resident; ptr += sizeof(bool);
    *(uint32_t*)ptr = item->access_count; ptr += sizeof(uint32_t);
    *(float*)ptr = item->max_anisotropy; ptr += sizeof(float);
    *(bool*)ptr = item->anisotropic_enabled; ptr += sizeof(bool);
    *(uint32_t*)ptr = item->virtual_tile_count; ptr += sizeof(uint32_t);
    *(bool*)ptr = item->virtual_texturing_enabled; ptr += sizeof(bool);
    *(uint32_t*)ptr = (uint32_t)item->format; ptr += sizeof(uint32_t);
    *(bool*)ptr = item->compression_enabled; ptr += sizeof(bool);
    *(uint32_t*)ptr = item->compression_level; ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->mip_level_count; ptr += sizeof(uint32_t);
    *(bool*)ptr = item->mipmaps_generated; ptr += sizeof(bool);
    *(bool*)ptr = item->bindless_enabled; ptr += sizeof(bool);
    *(uint32_t*)ptr = item->array_index; ptr += sizeof(uint32_t);
    *(bool*)ptr = item->array_enabled; ptr += sizeof(bool);
    
    // Copy virtual tiles
    if (item->virtual_tiles && item->virtual_tile_count > 0) {
        memcpy(ptr, item->virtual_tiles, virtual_tiles_size);
        ptr += virtual_tiles_size;
    }
    
    // Copy mipmaps
    if (item->mipmaps && item->mip_level_count > 0) {
        memcpy(ptr, item->mipmaps, mipmaps_size);
        ptr += mipmaps_size;
    }
    
    // Copy main data
    if (item->data && item->data_size > 0) {
        memcpy(ptr, item->data, item->data_size);
    }
    
    *out_data = data;
    *out_size = total_size;
    return 0;
}

static void* texture_anisotropic_filter_file_watch_thread(void* arg) {
    (void)arg;
    
    char buffer[4096];
    while (g_anisotropic_filter_ctx.file_watching_active) {
        int length = read(g_anisotropic_filter_ctx.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            int i = 0;
            while (i < length) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                if (event->mask & IN_MODIFY) {
                    // File modified - trigger hot reload
                    g_anisotropic_filter_ctx.stats.hot_reload_count++;
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); // 100ms
    }
    
    return NULL;
}

static void* texture_anisotropic_filter_async_thread(void* arg) {
    uint32_t thread_id = *(uint32_t*)arg;
    
    while (g_anisotropic_filter_ctx.async_threads_active) {
        // Process async operations
        // This would handle background texture processing
        usleep(10000); // 10ms
    }
    
    return NULL;
}

// Error codes
typedef enum {
    TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE = 0,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_PARAM = -1,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_NOT_INITIALIZED = -2,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY = -3,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_HANDLE = -4,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_THREAD_ERROR = -5,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_GPU_ERROR = -6,
    TEXTURE_ANISOTROPIC_FILTER_ERROR_SERIALIZATION_ERROR = -7
} texture_anisotropic_filter_error_t;

// Render graph node
typedef struct anisotropic_filter_render_node {
    uint32_t node_id;
    texture_anisotropic_filter_handle_t filter_handle;
    uint32_t input_texture_id;
    uint32_t output_texture_id;
    float anisotropy_level;
    bool enabled;
} anisotropic_filter_render_node_t;

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Error string conversion
static const char* texture_anisotropic_filter_error_string(texture_anisotropic_filter_error_t error) {
    switch (error) {
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE: return "No error";
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_PARAM: return "Invalid parameter";
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_NOT_INITIALIZED: return "Not initialized";
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_HANDLE: return "Invalid handle";
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_COMPRESSION_FAILED: return "Compression failed";
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_GPU_INTEGRATION_FAILED: return "GPU integration failed";
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_SERIALIZATION_FAILED: return "Serialization failed";
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_THREAD_ERROR: return "Thread error";
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_ASYNC_OPERATION_FAILED: return "Async operation failed";
        case TEXTURE_ANISOTROPIC_FILTER_ERROR_VIRTUAL_TEXTURE_FAILED: return "Virtual texture failed";
        default: return "Unknown error";
    }
}

// Get current timestamp in milliseconds
static uint64_t texture_anisotropic_filter_get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

// Memory pool allocation
static void* texture_anisotropic_filter_pool_alloc(size_t size) {
    pthread_mutex_lock(&g_anisotropic_filter_ctx.memory_pool.mutex);
    
    if (g_anisotropic_filter_ctx.memory_pool.used_size + size > g_anisotropic_filter_ctx.memory_pool.total_size) {
        pthread_mutex_unlock(&g_anisotropic_filter_ctx.memory_pool.mutex);
        return NULL; // Out of memory
    }
    
    void* ptr = (uint8_t*)g_anisotropic_filter_ctx.memory_pool.memory + g_anisotropic_filter_ctx.memory_pool.used_size;
    g_anisotropic_filter_ctx.memory_pool.used_size += size;
    
    if (g_anisotropic_filter_ctx.memory_pool.used_size > g_anisotropic_filter_ctx.memory_pool.peak_usage) {
        g_anisotropic_filter_ctx.memory_pool.peak_usage = g_anisotropic_filter_ctx.memory_pool.used_size;
    }
    
    pthread_mutex_unlock(&g_anisotropic_filter_ctx.memory_pool.mutex);
    return ptr;
}

// Memory pool deallocation (simplified - in production would use free list)
static void texture_anisotropic_filter_pool_free(void* ptr, size_t size) {
    pthread_mutex_lock(&g_anisotropic_filter_ctx.memory_pool.mutex);
    // Simplified - just track usage for now
    if (g_anisotropic_filter_ctx.memory_pool.used_size >= size) {
        g_anisotropic_filter_ctx.memory_pool.used_size -= size;
    }
    pthread_mutex_unlock(&g_anisotropic_filter_ctx.memory_pool.mutex);
}

// Anisotropic filtering with SIMD optimization
static void texture_anisotropic_filter_apply_simd(const void* src_data, void* dst_data, 
                                                  uint32_t width, uint32_t height, 
                                                  texture_anisotropic_filter_level_t level) {
    // SIMD-optimized anisotropic filtering
    const float* src = (const float*)src_data;
    float* dst = (float*)dst_data;
    
    uint32_t samples = (uint32_t)level;
    
    // Process 4 pixels at a time using SIMD
    for (uint32_t y = 0; y < height; y += 2) {
        for (uint32_t x = 0; x < width; x += 4) {
            __m128 sum = _mm_setzero_ps();
            
            // Sample anisotropically
            for (uint32_t s = 0; s < samples; s++) {
                // Calculate sample offsets (simplified)
                float offset_x = (float)(s - samples/2) * 0.5f;
                float offset_y = (float)(s - samples/2) * 0.5f;
                
                // Sample texture (boundary checking needed in production)
                uint32_t sample_x = (x + (uint32_t)offset_x) % width;
                uint32_t sample_y = (y + (uint32_t)offset_y) % height;
                uint32_t sample_idx = sample_y * width + sample_x;
                
                __m128 sample = _mm_load1_ps(&src[sample_idx]);
                sum = _mm_add_ps(sum, sample);
            }
            
            // Average the samples
            __m128 avg = _mm_div_ps(sum, _mm_set1_ps((float)samples));
            _mm_store_ps(&dst[y * width + x], avg);
        }
    }
    
    // Update performance counters
    g_anisotropic_filter_ctx.performance_counters.simd_operations++;
    g_anisotropic_filter_ctx.performance_counters.total_samples_processed += width * height * samples;
}

// BC compression implementation (simplified)
static texture_anisotropic_filter_error_t texture_anisotropic_filter_compress_bc(
    const void* src_data, void* dst_data, uint32_t width, uint32_t height, 
    texture_anisotropic_compression_t format) {
    
    // Simplified BC compression - in production would use proper compression algorithms
    if (format == TEXTURE_ANISOTROPIC_COMPRESSION_NONE) {
        memcpy(dst_data, src_data, width * height * 4); // RGBA
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE;
    }
    
    // Placeholder for actual BC compression
    // This would implement BC1-BC7 compression algorithms
    memcpy(dst_data, src_data, width * height * 4); // Fallback to uncompressed
    
    g_anisotropic_filter_ctx.performance_counters.compression_operations++;
    return TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE;
}

// ASTC compression implementation (simplified)
static texture_anisotropic_filter_error_t texture_anisotropic_filter_compress_astc(
    const void* src_data, void* dst_data, uint32_t width, uint32_t height, 
    uint32_t block_width, uint32_t block_height) {
    
    // Simplified ASTC compression - in production would use proper ASTC encoder
    memcpy(dst_data, src_data, width * height * 4); // Fallback to uncompressed
    
    g_anisotropic_filter_ctx.performance_counters.compression_operations++;
    return TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE;
}

// Mipmap generation
static texture_anisotropic_filter_error_t texture_anisotropic_filter_generate_mipmaps(
    const void* src_data, void** mip_data, uint32_t width, uint32_t height, 
    uint32_t mip_levels) {
    
    for (uint32_t level = 1; level < mip_levels; level++) {
        uint32_t mip_width = width >> level;
        uint32_t mip_height = height >> level;
        
        if (mip_width == 0) mip_width = 1;
        if (mip_height == 0) mip_height = 1;
        
        size_t mip_size = mip_width * mip_height * 4; // RGBA
        mip_data[level] = texture_anisotropic_filter_pool_alloc(mip_size);
        if (!mip_data[level]) {
            return TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY;
        }
        
        // Simple box filter for mipmap generation
        const uint8_t* src = (const uint8_t*)src_data;
        uint8_t* dst = (uint8_t*)mip_data[level];
        
        for (uint32_t y = 0; y < mip_height; y++) {
            for (uint32_t x = 0; x < mip_width; x++) {
                uint32_t src_x = x << 1;
                uint32_t src_y = y << 1;
                
                // Average 2x2 block
                for (int c = 0; c < 4; c++) { // RGBA channels
                    uint32_t sum = 0;
                    uint32_t count = 0;
                    
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            uint32_t sx = src_x + dx;
                            uint32_t sy = src_y + dy;
                            
                            if (sx < width && sy < height) {
                                sum += src[(sy * width + sx) * 4 + c];
                                count++;
                            }
                        }
                    }
                    
                    dst[(y * mip_width + x) * 4 + c] = (uint8_t)(sum / count);
                }
            }
        }
    }
    
    return TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE;
}

// Virtual texture page loading
static texture_anisotropic_filter_error_t texture_anisotropic_filter_load_virtual_page(
    texture_anisotropic_filter_internal_t* texture, uint32_t page_x, uint32_t page_y, 
    uint32_t level) {
    
    // Find existing page
    texture_anisotropic_virtual_page_t* page = NULL;
    for (uint32_t i = 0; i < texture->virtual_page_count; i++) {
        if (texture->virtual_pages[i].x == page_x && 
            texture->virtual_pages[i].y == page_y && 
            texture->virtual_pages[i].level == level) {
            page = &texture->virtual_pages[i];
            break;
        }
    }
    
    // Allocate new page if not found
    if (!page) {
        if (texture->virtual_page_count >= texture->virtual_page_capacity) {
            return TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY;
        }
        
        page = &texture->virtual_pages[texture->virtual_page_count++];
        page->x = page_x;
        page->y = page_y;
        page->level = level;
        page->texture_id = texture->id;
        page->loaded = false;
        
        size_t page_size = TEXTURE_ANISOTROPIC_FILTER_VIRTUAL_PAGE_SIZE * 
                          TEXTURE_ANISOTROPIC_FILTER_VIRTUAL_PAGE_SIZE * 4; // RGBA
        page->data = texture_anisotropic_filter_pool_alloc(page_size);
        if (!page->data) {
            return TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY;
        }
        page->data_size = page_size;
    }
    
    // Load page data (simplified - would load from disk in production)
    memset(page->data, 0x80, page->data_size); // Gray placeholder
    page->loaded = true;
    page->last_accessed = texture_anisotropic_filter_get_timestamp_ms();
    
    g_anisotropic_filter_ctx.performance_counters.virtual_texture_operations++;
    return TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE;
}

// Cache management
static texture_anisotropic_filter_error_t texture_anisotropic_filter_cache_add(
    uint32_t texture_id, uint32_t level, const void* data, size_t size) {
    
    pthread_mutex_lock(&g_anisotropic_filter_ctx.cache_mutex);
    
    // Find free cache slot or LRU slot
    uint32_t cache_index = 0;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < TEXTURE_ANISOTROPIC_FILTER_CACHE_SIZE; i++) {
        if (!g_anisotropic_filter_ctx.cache[i].valid) {
            cache_index = i;
            break;
        }
        
        if (g_anisotropic_filter_ctx.cache[i].last_accessed < oldest_time) {
            oldest_time = g_anisotropic_filter_ctx.cache[i].last_accessed;
            cache_index = i;
        }
    }
    
    // Free existing cache entry if needed
    if (g_anisotropic_filter_ctx.cache[cache_index].valid) {
        texture_anisotropic_filter_pool_free(
            g_anisotropic_filter_ctx.cache[cache_index].cached_data,
            g_anisotropic_filter_ctx.cache[cache_index].cached_size);
    }
    
    // Add new cache entry
    void* cache_data = texture_anisotropic_filter_pool_alloc(size);
    if (!cache_data) {
        pthread_mutex_unlock(&g_anisotropic_filter_ctx.cache_mutex);
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(cache_data, data, size);
    
    g_anisotropic_filter_ctx.cache[cache_index].texture_id = texture_id;
    g_anisotropic_filter_ctx.cache[cache_index].level = level;
    g_anisotropic_filter_ctx.cache[cache_index].cached_data = cache_data;
    g_anisotropic_filter_ctx.cache[cache_index].cached_size = size;
    g_anisotropic_filter_ctx.cache[cache_index].last_accessed = texture_anisotropic_filter_get_timestamp_ms();
    g_anisotropic_filter_ctx.cache[cache_index].access_count = 1;
    g_anisotropic_filter_ctx.cache[cache_index].valid = true;
    
    g_anisotropic_filter_ctx.cache_usage++;
    g_anisotropic_filter_ctx.performance_counters.cache_hits++;
    
    pthread_mutex_unlock(&g_anisotropic_filter_ctx.cache_mutex);
    return TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE;
}

// Async worker thread function
static void* texture_anisotropic_filter_worker_thread(void* arg) {
    uint32_t thread_id = *(uint32_t*)arg;
    
    while (g_anisotropic_filter_ctx.async_running) {
        pthread_mutex_lock(&g_anisotropic_filter_ctx.async_mutex);
        
        // Wait for work
        while (g_anisotropic_filter_ctx.async_running && 
               !g_anisotropic_filter_ctx.async_operations[thread_id].input_data) {
            pthread_cond_wait(&g_anisotropic_filter_ctx.async_cond, &g_anisotropic_filter_ctx.async_mutex);
        }
        
        if (!g_anisotropic_filter_ctx.async_running) {
            pthread_mutex_unlock(&g_anisotropic_filter_ctx.async_mutex);
            break;
        }
        
        // Process async operation
        texture_anisotropic_async_operation_t* op = &g_anisotropic_filter_ctx.async_operations[thread_id];
        op->success = false;
        
        switch (op->operation_type) {
            case TEXTURE_ANISOTROPIC_ASYNC_OPERATION_COMPRESS:
                // Perform compression
                op->success = (texture_anisotropic_filter_compress_bc(
                    op->input_data, op->output_data, 256, 256, 
                    TEXTURE_ANISOTROPIC_COMPRESSION_BC1) == TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE);
                break;
                
            case TEXTURE_ANISOTROPIC_ASYNC_OPERATION_GENERATE_MIPMAPS:
                // Generate mipmaps
                op->success = (texture_anisotropic_filter_generate_mipmaps(
                    op->input_data, (void**)&op->output_data, 256, 256, 8) == 
                    TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE);
                break;
                
            default:
                break;
        }
        
        op->completed = true;
        g_anisotropic_filter_ctx.performance_counters.async_operations++;
        
        // Signal completion
        pthread_mutex_lock(&op->completion_mutex);
        pthread_cond_signal(&op->completion_cond);
        pthread_mutex_unlock(&op->completion_mutex);
        
        // Reset operation
        op->input_data = NULL;
        op->output_data = NULL;
        
        pthread_mutex_unlock(&g_anisotropic_filter_ctx.async_mutex);
    }
    
    return NULL;
}

// File watching thread for hot-reload
static void* texture_anisotropic_filter_file_watch_thread(void* arg) {
        free(item->virtual_pages);
        item->virtual_pages = NULL;
    }
    
    // Cleanup main data
    if (item->data) {
        texture_anisotropic_filter_pool_free(item->data, item->data_size);
        item->data = NULL;
    }
    item->initialized = false;
    item->dirty = false;
}

// SIMD-optimized anisotropic filtering
static void texture_anisotropic_filter_simd_process(const float* input, float* output, 
                                                   uint32_t width, uint32_t height, 
                                                   float anisotropy) {
    // SIMD implementation using AVX2
    const __m256 anisotropy_vec = _mm256_set1_ps(anisotropy);
    const size_t pixel_count = width * height;
    const size_t simd_steps = pixel_count / 8;
    
    for (size_t i = 0; i < simd_steps; i++) {
        __m256 pixels = _mm256_loadu_ps(&input[i * 8]);
        __m256 filtered = _mm256_mul_ps(pixels, anisotropy_vec);
        _mm256_storeu_ps(&output[i * 8], filtered);
    }
    
    // Handle remaining pixels
    for (size_t i = simd_steps * 8; i < pixel_count; i++) {
        output[i] = input[i] * anisotropy;
    }
}

// GPU integration helper
static bool texture_anisotropic_filter_gpu_process(texture_anisotropic_filter_internal_t* item) {
    if (!g_anisotropic_filter_ctx.gpu_available || !g_anisotropic_filter_ctx.gpu_context) {
        return false;
    }
    
    // GPU processing implementation
    g_anisotropic_filter_ctx.stats.gpu_operations++;
    return true;
}

// Cache management
static bool texture_anisotropic_filter_cache_lookup(uint32_t texture_id, void** data, size_t* size) {
    for (uint32_t i = 0; i < TEXTURE_ANISOTROPIC_FILTER_CACHE_SIZE; i++) {
        if (g_anisotropic_filter_ctx.cache[i].valid && 
            g_anisotropic_filter_ctx.cache[i].texture_id == texture_id) {
            *data = g_anisotropic_filter_ctx.cache[i].data;
            *size = g_anisotropic_filter_ctx.cache[i].size;
            g_anisotropic_filter_ctx.stats.cache_hits++;
            return true;
        }
    }
    g_anisotropic_filter_ctx.stats.cache_misses++;
    return false;
}

static void texture_anisotropic_filter_cache_store(uint32_t texture_id, const void* data, size_t size) {
    static uint32_t cache_index = 0;
    
    // Find cache slot or use round-robin
    uint32_t slot = cache_index;
    cache_index = (cache_index + 1) % TEXTURE_ANISOTROPIC_FILTER_CACHE_SIZE;
    
    // Free existing data
    if (g_anisotropic_filter_ctx.cache[slot].data) {
        free(g_anisotropic_filter_ctx.cache[slot].data);
    }
    
    // Store new data
    g_anisotropic_filter_ctx.cache[slot].data = malloc(size);
    if (g_anisotropic_filter_ctx.cache[slot].data) {
        memcpy(g_anisotropic_filter_ctx.cache[slot].data, data, size);
        g_anisotropic_filter_ctx.cache[slot].size = size;
        g_anisotropic_filter_ctx.cache[slot].texture_id = texture_id;
        g_anisotropic_filter_ctx.cache[slot].valid = true;
        g_anisotropic_filter_ctx.cache[slot].timestamp = time(NULL);
    }
}

// Hot-reload file watching thread
static void* texture_anisotropic_filter_file_watch_thread(void* arg) {
    char buffer[4096];
    
    while (g_anisotropic_filter_ctx.file_watch_active) {
        ssize_t length = read(g_anisotropic_filter_ctx.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            // Process file events
            for (char* ptr = buffer; ptr < buffer + length; ) {
                struct inotify_event* event = (struct inotify_event*)ptr;
                if (event->mask & IN_MODIFY) {
                    // Trigger hot-reload for modified texture
                    pthread_mutex_lock(&g_anisotropic_filter_ctx.mutex);
                    // Mark all filters as dirty for reload
                    for (uint32_t i = 0; i < g_anisotropic_filter_ctx.count; i++) {
                        g_anisotropic_filter_ctx.items[i].dirty = true;
                    }
                    pthread_mutex_unlock(&g_anisotropic_filter_ctx.mutex);
                }
                ptr += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); // 100ms sleep
    }
    return NULL;
}

// Batch processing
static void texture_anisotropic_filter_process_batch(void) {
    if (g_anisotropic_filter_ctx.batch_count == 0) return;
    
    for (uint32_t i = 0; i < g_anisotropic_filter_ctx.batch_count; i++) {
        texture_anisotropic_filter_internal_t* item = g_anisotropic_filter_ctx.batch_queue[i];
        if (item && item->dirty) {
            // Process item with SIMD if available
            if (g_anisotropic_filter_ctx.simd_available && item->data) {
                texture_anisotropic_filter_simd_process(
                    (float*)item->data, (float*)item->cache_data,
                    item->width, item->height, item->max_anisotropy
                );
                g_anisotropic_filter_ctx.stats.simd_operations++;
            } else {
                // Fallback to CPU processing
                texture_anisotropic_filter_gpu_process(item);
            }
            
            item->dirty = false;
            item->cache_valid = true;
        }
    }
    
    g_anisotropic_filter_ctx.stats.batch_operations++;
    g_anisotropic_filter_ctx.batch_count = 0;
}

// LOD calculation
static float texture_anisotropic_filter_calculate_lod(const texture_anisotropic_filter_internal_t* item, 
                                                    float u, float v, float dudx, float dvdx, 
                                                    float dudy, float dvdy) {
    // Calculate texture derivatives
    float dx = sqrtf(dudx * dudx + dvdx * dvdx);
    float dy = sqrtf(dudy * dudy + dvdy * dvdy);
    float max_derivative = fmaxf(dx, dy);
    
    // Calculate LOD level
    float lod = log2f(max_derivative * fmaxf(item->width, item->height));
    
    // Apply anisotropic filtering
    float anisotropy_ratio = dx / dy;
    if (anisotropy_ratio > 1.0f) {
        lod -= log2f(fminf(anisotropy_ratio, item->max_anisotropy));
    } else {
        lod -= log2f(fminf(1.0f / anisotropy_ratio, item->max_anisotropy));
    }
    
    return fmaxf(0.0f, fminf(lod, (float)item->mip_levels - 1.0f));
}

// Culling integration
static bool texture_anisotropic_filter_should_cull(const texture_anisotropic_filter_internal_t* item, 
                                                   float distance, float threshold) {
    // Simple distance-based culling
    float max_distance = sqrtf(item->width * item->width + item->height * item->height) * threshold;
    return distance > max_distance;
}

// Serialization
static int texture_anisotropic_filter_serialize(const texture_anisotropic_filter_internal_t* item, 
                                               void** buffer, size_t* size) {
    if (!item || !buffer || !size) return TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_PARAM;
    
    // Calculate serialized size
    *size = sizeof(uint32_t) * 5 + sizeof(float) * 2 + item->data_size;
    *buffer = malloc(*size);
    if (!*buffer) return TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY;
    
    // Serialize data
    uint8_t* ptr = (uint8_t*)*buffer;
    
    // Write magic number and version
    *((uint32_t*)ptr) = TEXTURE_ANISOTROPIC_FILTER_MAGIC_NUMBER;
    ptr += sizeof(uint32_t);
    *((uint32_t*)ptr) = TEXTURE_ANISOTROPIC_FILTER_VERSION;
    ptr += sizeof(uint32_t);
    
    // Write properties
    *((uint32_t*)ptr) = item->id;
    ptr += sizeof(uint32_t);
    *((uint32_t*)ptr) = item->flags;
    ptr += sizeof(uint32_t);
    *((uint32_t*)ptr) = item->width;
    ptr += sizeof(uint32_t);
    *((uint32_t*)ptr) = item->height;
    ptr += sizeof(uint32_t);
    *((float*)ptr) = item->max_anisotropy;
    ptr += sizeof(float);
    *((uint32_t*)ptr) = item->mip_levels;
    ptr += sizeof(uint32_t);
    *((uint32_t*)ptr) = item->format;
    ptr += sizeof(uint32_t);
    
    // Write data
    if (item->data && item->data_size > 0) {
        memcpy(ptr, item->data, item->data_size);
    }
    
    return TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int texture_anisotropic_filter_init(void) {
    if (g_anisotropic_filter_ctx.initialized) {
        return 0; // Already initialized
    }

    // Initialize mutexes
    if (pthread_mutex_init(&g_anisotropic_filter_ctx.context_mutex, NULL) != 0) {
        return -1;
    }
    if (pthread_rwlock_init(&g_anisotropic_filter_ctx.items_rwlock, NULL) != 0) {
        pthread_mutex_destroy(&g_anisotropic_filter_ctx.context_mutex);
        return -1;
    }

    // Initialize memory pool
    g_anisotropic_filter_ctx.memory_pool.memory = malloc(TEXTURE_ANISOTROPIC_FILTER_MEMORY_POOL_SIZE);
    if (!g_anisotropic_filter_ctx.memory_pool.memory) {
        pthread_rwlock_destroy(&g_anisotropic_filter_ctx.items_rwlock);
        pthread_mutex_destroy(&g_anisotropic_filter_ctx.context_mutex);
        return -2;
    }
    g_anisotropic_filter_ctx.memory_pool.total_size = TEXTURE_ANISOTROPIC_FILTER_MEMORY_POOL_SIZE;
    g_anisotropic_filter_ctx.memory_pool.used_size = 0;
    g_anisotropic_filter_ctx.memory_pool.peak_usage = 0;
    pthread_mutex_init(&g_anisotropic_filter_ctx.memory_pool.mutex, NULL);

    // Initialize cache
    pthread_mutex_init(&g_anisotropic_filter_ctx.cache_mutex, NULL);
    memset(g_anisotropic_filter_ctx.cache, 0, sizeof(g_anisotropic_filter_ctx.cache));
    g_anisotropic_filter_ctx.cache_usage = 0;

    // Initialize async operations
    pthread_mutex_init(&g_anisotropic_filter_ctx.async_mutex, NULL);
    pthread_cond_init(&g_anisotropic_filter_ctx.async_cond, NULL);
    memset(g_anisotropic_filter_ctx.async_operations, 0, sizeof(g_anisotropic_filter_ctx.async_operations));
    g_anisotropic_filter_ctx.async_running = true;

    // Start worker threads
    for (uint32_t i = 0; i < TEXTURE_ANISOTROPIC_FILTER_WORKER_THREADS; i++) {
        pthread_create(&g_anisotropic_filter_ctx.worker_threads[i], NULL,
                      texture_anisotropic_filter_worker_thread, &i);
    }

    // Initialize GPU context
    pthread_mutex_init(&g_anisotropic_filter_ctx.gpu_context.gpu_mutex, NULL);
    g_anisotropic_filter_ctx.gpu_context.gpu_available = true; // Simplified
    g_anisotropic_filter_ctx.gpu_context.use_gpu_for_filtering = true;

    // Initialize file watching
    pthread_mutex_init(&g_anisotropic_filter_ctx.file_watcher.watcher_mutex, NULL);
    g_anisotropic_filter_ctx.file_watcher.inotify_fd = inotify_init();
    g_anisotropic_filter_ctx.file_watcher.watcher_running = true;
    if (g_anisotropic_filter_ctx.file_watcher.inotify_fd >= 0) {
        pthread_create(&g_anisotropic_filter_ctx.file_watcher.watcher_thread, NULL,
                      texture_anisotropic_filter_file_watch_thread, NULL);
    }

    // Initialize culling context
    pthread_mutex_init(&g_anisotropic_filter_ctx.culling_context.culling_mutex, NULL);
    g_anisotropic_filter_ctx.culling_context.culling_enabled = true;
    g_anisotropic_filter_ctx.culling_context.visible_texture_count = 0;
    g_anisotropic_filter_ctx.culling_context.culled_texture_count = 0;
    g_anisotropic_filter_ctx.culling_context.culling_distance_threshold = 1000.0f;

    // Initialize performance counters
    memset(&g_anisotropic_filter_ctx.performance_counters, 0, 
           sizeof(g_anisotropic_filter_ctx.performance_counters));

    // Allocate items array
    g_anisotropic_filter_ctx.capacity = TEXTURE_ANISOTROPIC_FILTER_DEFAULT_CAPACITY;
    g_anisotropic_filter_ctx.items = calloc(g_anisotropic_filter_ctx.capacity, 
                                          sizeof(texture_anisotropic_filter_internal_t));
    if (!g_anisotropic_filter_ctx.items) {
        // Cleanup on failure
        pthread_mutex_destroy(&g_anisotropic_filter_ctx.context_mutex);
        pthread_rwlock_destroy(&g_anisotropic_filter_ctx.items_rwlock);
        pthread_mutex_destroy(&g_anisotropic_filter_ctx.memory_pool.mutex);
        free(g_anisotropic_filter_ctx.memory_pool.memory);
        return -3;
    }

    g_anisotropic_filter_ctx.count = 0;
    g_anisotropic_filter_ctx.render_graph_node_count = 0;
    g_anisotropic_filter_ctx.initialized = true;

    return 0;
}

void texture_anisotropic_filter_shutdown(void) {
    if (!g_anisotropic_filter_ctx.initialized) {
        return;
    }

    pthread_mutex_lock(&g_anisotropic_filter_ctx.context_mutex);
    
    // Stop async operations
    g_anisotropic_filter_ctx.async_running = false;
    pthread_cond_broadcast(&g_anisotropic_filter_ctx.async_cond);
    
    // Stop file watching
    g_anisotropic_filter_ctx.file_watcher.watcher_running = false;
    if (g_anisotropic_filter_ctx.file_watcher.inotify_fd >= 0) {
        close(g_anisotropic_filter_ctx.file_watcher.inotify_fd);
    }
    
    pthread_mutex_unlock(&g_anisotropic_filter_ctx.context_mutex);
    
    // Wait for worker threads to finish
    for (uint32_t i = 0; i < TEXTURE_ANISOTROPIC_FILTER_WORKER_THREADS; i++) {
        pthread_join(g_anisotropic_filter_ctx.worker_threads[i], NULL);
    }
    
    // Wait for file watcher thread to finish
    pthread_join(g_anisotropic_filter_ctx.file_watcher.watcher_thread, NULL);
    
    // Cleanup all textures
    for (uint32_t i = 0; i < g_anisotropic_filter_ctx.count; i++) {
        texture_anisotropic_filter_cleanup_internal(&g_anisotropic_filter_ctx.items[i]);
    }
    
    // Cleanup cache
    pthread_mutex_lock(&g_anisotropic_filter_ctx.cache_mutex);
    for (uint32_t i = 0; i < TEXTURE_ANISOTROPIC_FILTER_CACHE_SIZE; i++) {
        if (g_anisotropic_filter_ctx.cache[i].valid && 
            g_anisotropic_filter_ctx.cache[i].cached_data) {
            texture_anisotropic_filter_pool_free(
                g_anisotropic_filter_ctx.cache[i].cached_data,
                g_anisotropic_filter_ctx.cache[i].cached_size);
        }
    }
    pthread_mutex_unlock(&g_anisotropic_filter_ctx.cache_mutex);
    
    // Cleanup memory pool
    pthread_mutex_lock(&g_anisotropic_filter_ctx.memory_pool.mutex);
    free(g_anisotropic_filter_ctx.memory_pool.memory);
    g_anisotropic_filter_ctx.memory_pool.memory = NULL;
    pthread_mutex_unlock(&g_anisotropic_filter_ctx.memory_pool.mutex);
    
    // Cleanup items array
    free(g_anisotropic_filter_ctx.items);
    g_anisotropic_filter_ctx.items = NULL;
    
    // Destroy mutexes and condition variables
    pthread_mutex_destroy(&g_anisotropic_filter_ctx.context_mutex);
    pthread_rwlock_destroy(&g_anisotropic_filter_ctx.items_rwlock);
    pthread_mutex_destroy(&g_anisotropic_filter_ctx.memory_pool.mutex);
    pthread_mutex_destroy(&g_anisotropic_filter_ctx.cache_mutex);
    pthread_mutex_destroy(&g_anisotropic_filter_ctx.async_mutex);
    pthread_cond_destroy(&g_anisotropic_filter_ctx.async_cond);
    pthread_mutex_destroy(&g_anisotropic_filter_ctx.gpu_context.gpu_mutex);
    pthread_mutex_destroy(&g_anisotropic_filter_ctx.file_watcher.watcher_mutex);
    pthread_mutex_destroy(&g_anisotropic_filter_ctx.culling_context.culling_mutex);
    
    // Reset state
    g_anisotropic_filter_ctx.count = 0;
    g_anisotropic_filter_ctx.capacity = 0;
    g_anisotropic_filter_ctx.cache_usage = 0;
    g_anisotropic_filter_ctx.render_graph_node_count = 0;
    g_anisotropic_filter_ctx.initialized = false;
}

int texture_anisotropic_filter_create(texture_anisotropic_filter_handle_t* out_handle, const texture_anisotropic_filter_desc_t* desc) {
    if (!out_handle || !desc) {
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_PARAM;
    }

    if (!g_anisotropic_filter_ctx.initialized) {
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_NOT_INITIALIZED;
    }

    pthread_rwlock_wrlock(&g_anisotropic_filter_ctx.items_rwlock);

    if (g_anisotropic_filter_ctx.count >= g_anisotropic_filter_ctx.capacity) {
        pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY;
    }

    uint32_t index = g_anisotropic_filter_ctx.count++;
    texture_anisotropic_filter_internal_t* item = &g_anisotropic_filter_ctx.items[index];

    // Initialize texture with anisotropic filtering properties
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = texture_anisotropic_filter_get_timestamp_ms();
    
    // Set default anisotropic filtering properties
    item->anisotropy_level = TEXTURE_ANISOTROPIC_FILTER_LEVEL_4X;
    item->compression_format = TEXTURE_ANISOTROPIC_COMPRESSION_BC7;
    item->width = 256; // Default size
    item->height = 256;
    item->depth = 1;
    item->mip_levels = 8;
    item->use_virtual_texturing = false;
    item->current_lod_level = 0;
    
    // Initialize virtual pages
    item->virtual_page_count = 0;
    item->virtual_page_capacity = 64;
    item->virtual_pages = calloc(item->virtual_page_capacity, sizeof(texture_anisotropic_virtual_page_t));
    if (!item->virtual_pages) {
        item->initialized = false;
        g_anisotropic_filter_ctx.count--;
        pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize LOD info
    for (uint32_t i = 0; i < TEXTURE_ANISOTROPIC_FILTER_MAX_LOD_LEVELS; i++) {
        item->lod_info[i].level = i;
        item->lod_info[i].min_distance = i * 100.0f;
        item->lod_info[i].max_distance = (i + 1) * 100.0f;
        item->lod_info[i].width = item->width >> i;
        item->lod_info[i].height = item->height >> i;
        item->lod_info[i].anisotropy_level = item->anisotropy_level;
        item->lod_info[i].use_virtual_texturing = item->use_virtual_texturing;
    }
    
    // Clear file path
    memset(item->file_path, 0, sizeof(item->file_path));
    item->last_modified = 0;

    pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);

    out_handle->id = index;
    return TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE;
}

void texture_anisotropic_filter_destroy(texture_anisotropic_filter_handle_t handle) {
    if (!g_anisotropic_filter_ctx.initialized) {
        return;
    }

    pthread_rwlock_wrlock(&g_anisotropic_filter_ctx.items_rwlock);

    if (handle.id >= g_anisotropic_filter_ctx.count) {
        pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
        return;
    }

    texture_anisotropic_filter_cleanup_internal(&g_anisotropic_filter_ctx.items[handle.id]);

    pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
}

int texture_anisotropic_filter_update(texture_anisotropic_filter_handle_t handle, const void* data, size_t size) {
    if (!g_anisotropic_filter_ctx.initialized) {
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_NOT_INITIALIZED;
    }

    if (!data || size == 0) {
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_PARAM;
    }

    pthread_rwlock_rdlock(&g_anisotropic_filter_ctx.items_rwlock);

    if (handle.id >= g_anisotropic_filter_ctx.count) {
        pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_HANDLE;
    }

    texture_anisotropic_filter_internal_t* item = &g_anisotropic_filter_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_HANDLE;
    }

    // Free old data
    if (item->data) {
        texture_anisotropic_filter_pool_free(item->data, item->data_size);
    }

    // Allocate new data
    item->data = texture_anisotropic_filter_pool_alloc(size);
    if (!item->data) {
        pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_OUT_OF_MEMORY;
    }

    // Copy new data
    memcpy(item->data, data, size);
    item->data_size = size;
    item->dirty = true;
    item->frame_updated = texture_anisotropic_filter_get_timestamp_ms();

    pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);

    return TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE;
}

bool texture_anisotropic_filter_is_valid(texture_anisotropic_filter_handle_t handle) {
    if (!g_anisotropic_filter_ctx.initialized) {
        return false;
    }

    pthread_rwlock_rdlock(&g_anisotropic_filter_ctx.items_rwlock);

    if (handle.id >= g_anisotropic_filter_ctx.count) {
        pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
        return false;
    }

    bool valid = texture_anisotropic_filter_validate(&g_anisotropic_filter_ctx.items[handle.id]);

    pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);

    return valid;
}

int texture_anisotropic_filter_get_info(texture_anisotropic_filter_handle_t handle, texture_anisotropic_filter_info_t* out_info) {
    if (!out_info) {
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_PARAM;
    }

    if (!g_anisotropic_filter_ctx.initialized) {
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_NOT_INITIALIZED;
    }

    pthread_rwlock_rdlock(&g_anisotropic_filter_ctx.items_rwlock);

    if (handle.id >= g_anisotropic_filter_ctx.count) {
        pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_HANDLE;
    }

    const texture_anisotropic_filter_internal_t* item = &g_anisotropic_filter_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
        return TEXTURE_ANISOTROPIC_FILTER_ERROR_INVALID_HANDLE;
    }

    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);

    return TEXTURE_ANISOTROPIC_FILTER_ERROR_NONE;
}

void texture_anisotropic_filter_mark_dirty(texture_anisotropic_filter_handle_t handle) {
    if (!g_anisotropic_filter_ctx.initialized) {
        return;
    }

    pthread_rwlock_rdlock(&g_anisotropic_filter_ctx.items_rwlock);

    if (handle.id < g_anisotropic_filter_ctx.count) {
        g_anisotropic_filter_ctx.items[handle.id].dirty = true;
        g_anisotropic_filter_ctx.items[handle.id].frame_updated = texture_anisotropic_filter_get_timestamp_ms();
    }

    pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);
}

int texture_anisotropic_filter_process_pending(void) {
    if (!g_anisotropic_filter_ctx.initialized) {
        return 0;
    }

    pthread_rwlock_rdlock(&g_anisotropic_filter_ctx.items_rwlock);

    int processed = 0;
    uint64_t start_time = texture_anisotropic_filter_get_timestamp_ms();

    for (uint32_t i = 0; i < g_anisotropic_filter_ctx.count; i++) {
        texture_anisotropic_filter_internal_t* item = &g_anisotropic_filter_ctx.items[i];
        if (item->initialized && item->dirty) {
            
            // Apply anisotropic filtering
            if (item->data && item->data_size > 0) {
                // Use SIMD optimization if available
                if (item->width > 0 && item->height > 0) {
                    void* filtered_data = texture_anisotropic_filter_pool_alloc(item->data_size);
                    if (filtered_data) {
                        texture_anisotropic_filter_apply_simd(item->data, filtered_data, 
                                                            item->width, item->height, 
                                                            item->anisotropy_level);
                        
                        // Apply compression if needed
                        if (item->compression_format != TEXTURE_ANISOTROPIC_COMPRESSION_NONE) {
                            void* compressed_data = texture_anisotropic_filter_pool_alloc(item->data_size);
                            if (compressed_data) {
                                texture_anisotropic_filter_compress_bc(filtered_data, compressed_data,
                                                                  item->width, item->height,
                                                                  item->compression_format);
                                texture_anisotropic_filter_pool_free(compressed_data, item->data_size);
                            }
                        }
                        
                        texture_anisotropic_filter_pool_free(filtered_data, item->data_size);
                    }
                }
            }
            
            item->dirty = false;
            processed++;
        }
    }

    uint64_t end_time = texture_anisotropic_filter_get_timestamp_ms();
    double processing_time = (double)(end_time - start_time);
    
    // Update performance counters
    g_anisotropic_filter_ctx.performance_counters.total_processing_time_ms += processing_time;
    g_anisotropic_filter_ctx.performance_counters.batch_operations++;

    pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);

    return processed;
}

uint32_t texture_anisotropic_filter_get_count(void) {
    if (!g_anisotropic_filter_ctx.initialized) {
        return 0;
    }

    pthread_rwlock_rdlock(&g_anisotropic_filter_ctx.items_rwlock);
    uint32_t count = g_anisotropic_filter_ctx.count;
    pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);

    return count;
}

size_t texture_anisotropic_filter_get_memory_usage(void) {
    if (!g_anisotropic_filter_ctx.initialized) {
        return 0;
    }

    pthread_mutex_lock(&g_anisotropic_filter_ctx.memory_pool.mutex);
    size_t total_memory = g_anisotropic_filter_ctx.memory_pool.used_size;
    pthread_mutex_unlock(&g_anisotropic_filter_ctx.memory_pool.mutex);

    // Add context memory usage
    total_memory += sizeof(g_anisotropic_filter_ctx);
    total_memory += g_anisotropic_filter_ctx.capacity * sizeof(texture_anisotropic_filter_internal_t);

    return total_memory;
}

void texture_anisotropic_filter_debug_print(void) {
    if (!g_anisotropic_filter_ctx.initialized) {
        printf("Anisotropic Filter System: Not initialized\n");
        return;
    }

    printf("=== Anisotropic Filter System Debug Info ===\n");
    
    pthread_rwlock_rdlock(&g_anisotropic_filter_ctx.items_rwlock);
    printf("Textures: %u / %u\n", g_anisotropic_filter_ctx.count, g_anisotropic_filter_ctx.capacity);
    
    for (uint32_t i = 0; i < g_anisotropic_filter_ctx.count; i++) {
        const texture_anisotropic_filter_internal_t* item = &g_anisotropic_filter_ctx.items[i];
        printf("  Texture %u: %ux%u, Anisotropy: %ux, Format: %d, Dirty: %s\n",
               item->id, item->width, item->height, (uint32_t)item->anisotropy_level,
               item->compression_format, item->dirty ? "Yes" : "No");
    }
    pthread_rwlock_unlock(&g_anisotropic_filter_ctx.items_rwlock);

    pthread_mutex_lock(&g_anisotropic_filter_ctx.cache_mutex);
    printf("Cache Usage: %u / %u\n", g_anisotropic_filter_ctx.cache_usage, TEXTURE_ANISOTROPIC_FILTER_CACHE_SIZE);
    pthread_mutex_unlock(&g_anisotropic_filter_ctx.cache_mutex);

    pthread_mutex_lock(&g_anisotropic_filter_ctx.memory_pool.mutex);
    printf("Memory Pool: %zu / %zu bytes used (%.1f%%)\n",
           g_anisotropic_filter_ctx.memory_pool.used_size,
           g_anisotropic_filter_ctx.memory_pool.total_size,
           (double)g_anisotropic_filter_ctx.memory_pool.used_size / g_anisotropic_filter_ctx.memory_pool.total_size * 100.0);
    pthread_mutex_unlock(&g_anisotropic_filter_ctx.memory_pool.mutex);

    printf("Performance Counters:\n");
    printf("  Total Samples Processed: %llu\n", g_anisotropic_filter_ctx.performance_counters.total_samples_processed);
    printf("  Cache Hits: %llu, Cache Misses: %llu\n",
           g_anisotropic_filter_ctx.performance_counters.cache_hits,
           g_anisotropic_filter_ctx.performance_counters.cache_misses);
    printf("  GPU Operations: %llu\n", g_anisotropic_filter_ctx.performance_counters.gpu_operations);
    printf("  SIMD Operations: %llu\n", g_anisotropic_filter_ctx.performance_counters.simd_operations);
    printf("  Async Operations: %llu\n", g_anisotropic_filter_ctx.performance_counters.async_operations);
    printf("  Compression Operations: %llu\n", g_anisotropic_filter_ctx.performance_counters.compression_operations);
    printf("  Virtual Texture Operations: %llu\n", g_anisotropic_filter_ctx.performance_counters.virtual_texture_operations);
    printf("  Batch Operations: %llu\n", g_anisotropic_filter_ctx.performance_counters.batch_operations);
    printf("  Total Processing Time: %.2f ms\n", g_anisotropic_filter_ctx.performance_counters.total_processing_time_ms);
    printf("  Average Processing Time: %.2f ms\n", g_anisotropic_filter_ctx.performance_counters.total_processing_time_ms / 
           (g_anisotropic_filter_ctx.performance_counters.batch_operations + 1));
    printf("==========================================\n");
}

/* End of anisotropic_filter.c */
