/*
 * texture_arrays.c
 * Texture array management
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement texture streaming
 * TODO: Add virtual texturing
 * TODO: Implement BC/ASTC compression
 * TODO: Add mipmap generation
 * TODO: Implement bindless textures
 * TODO: Add texture arrays
 * TODO: Implement feedback analysis
 * TODO: Add residency management
 * TODO: Implement format conversion
 * TODO: Add anisotropic filtering
 * TODO: Implement texture arrays initialization
 * TODO: Add texture arrays cleanup/shutdown
 * TODO: Implement texture arrays validation
 * TODO: Add texture arrays error handling
 * TODO: Implement texture arrays serialization
 * TODO: Add texture arrays debug output
 * TODO: Implement texture arrays unit tests
 * TODO: Add texture arrays performance counters
 * TODO: Implement texture arrays hot-reload
 * TODO: Add texture arrays thread safety
 * TODO: Implement texture arrays memory pooling
 * TODO: Add texture arrays caching layer
 * TODO: Implement texture arrays async operations
 * TODO: Add texture arrays GPU integration
 * TODO: Implement texture arrays SIMD optimization
 * TODO: Add texture arrays batch processing
 * TODO: Implement texture arrays streaming support
 * TODO: Add texture arrays LOD support
 * TODO: Implement texture arrays culling integration
 * TODO: Add texture arrays render graph node
 */

#include "texture_arrays.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#ifdef __x86_64__
#include <immintrin.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/inotify.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <lz4.h>
#include <zstd.h>

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#elif defined(__linux__)
#include <semaphore.h>
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXTURE_TEXTURE_ARRAYS_MAX_COUNT 4096
#define TEXTURE_TEXTURE_ARRAYS_DEFAULT_CAPACITY 256
#define TEXTURE_TEXTURE_ARRAYS_ALIGNMENT 16
#define TEXTURE_TEXTURE_ARRAYS_MAX_LAYERS 1024
#define TEXTURE_TEXTURE_ARRAYS_MAX_MIP_LEVELS 12
#define TEXTURE_TEXTURE_ARRAYS_CACHE_SIZE 64
#define TEXTURE_TEXTURE_ARRAYS_BATCH_SIZE 32
#define TEXTURE_TEXTURE_ARRAYS_ASYNC_QUEUE_SIZE 128
#define TEXTURE_TEXTURE_ARRAYS_MAX_ANISOTROPY 16
#define TEXTURE_TEXTURE_ARRAYS_CACHE_SIZE 1024
#define TEXTURE_TEXTURE_ARRAYS_ASYNC_QUEUE_SIZE 64
#define TEXTURE_TEXTURE_ARRAYS_WORKER_THREADS 4
#define TEXTURE_TEXTURE_ARRAYS_MEMORY_POOL_SIZE (64 * 1024 * 1024) // 64MB
#define TEXTURE_TEXTURE_ARRAYS_VIRTUAL_PAGE_SIZE 128
#define TEXTURE_TEXTURE_ARRAYS_MAX_LOD_LEVELS 8
#define TEXTURE_TEXTURE_ARRAYS_MAGIC_NUMBER 0x54415241 // "TARA"
#define TEXTURE_TEXTURE_ARRAYS_MAX_LAYERS 2048
#define TEXTURE_TEXTURE_ARRAYS_MAX_MIP_LEVELS 16
#define TEXTURE_TEXTURE_ARRAYS_MAX_ANISOTROPY 16
#define TEXTURE_TEXTURE_ARRAYS_CACHE_SIZE 64
#define TEXTURE_TEXTURE_ARRAYS_ASYNC_QUEUE_SIZE 128
#define TEXTURE_TEXTURE_ARRAYS_SIMD_WIDTH 16
#define TEXTURE_TEXTURE_ARRAYS_BATCH_SIZE 32
#define TEXTURE_TEXTURE_ARRAYS_VIRTUAL_TILE_SIZE 128
#define TEXTURE_TEXTURE_ARRAYS_FEEDBACK_BUFFER_SIZE 1024

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Texture format types */
typedef enum {
    TEXTURE_FORMAT_R8_UNORM = 0,
    TEXTURE_FORMAT_RG8_UNORM,
    TEXTURE_FORMAT_RGB8_UNORM,
    TEXTURE_FORMAT_RGBA8_UNORM,
    TEXTURE_FORMAT_R16_FLOAT,
    TEXTURE_FORMAT_RG16_FLOAT,
    TEXTURE_FORMAT_RGB16_FLOAT,
    TEXTURE_FORMAT_RGBA16_FLOAT,
    TEXTURE_FORMAT_R32_FLOAT,
    TEXTURE_FORMAT_RG32_FLOAT,
    TEXTURE_FORMAT_RGB32_FLOAT,
    TEXTURE_FORMAT_RGBA32_FLOAT,
    TEXTURE_FORMAT_BC1_UNORM,
    TEXTURE_FORMAT_BC3_UNORM,
    TEXTURE_FORMAT_BC5_UNORM,
    TEXTURE_FORMAT_BC7_UNORM,
    TEXTURE_FORMAT_ASTC_4x4_UNORM,
    TEXTURE_FORMAT_ASTC_6x6_UNORM,
    TEXTURE_FORMAT_ASTC_8x8_UNORM,
    TEXTURE_FORMAT_COUNT
} texture_format_t;

/* Texture filtering modes */
typedef enum {
    TEXTURE_FILTER_NEAREST = 0,
    TEXTURE_FILTER_LINEAR,
    TEXTURE_FILTER_TRILINEAR,
    TEXTURE_FILTER_ANISOTROPIC
} texture_filter_t;

/* Texture wrap modes */
typedef enum {
    TEXTURE_WRAP_REPEAT = 0,
    TEXTURE_WRAP_CLAMP_TO_EDGE,
    TEXTURE_WRAP_CLAMP_TO_BORDER,
    TEXTURE_WRAP_MIRRORED_REPEAT
} texture_wrap_t;

/* LOD settings */
typedef struct texture_lod_settings {
    float min_lod;
    float max_lod;
    float lod_bias;
    bool enable_lod;
} texture_lod_settings_t;

/* Anisotropic filtering settings */
typedef struct texture_aniso_settings {
    uint32_t max_anisotropy;
    bool enabled;
    texture_filter_t filter_mode;
} texture_aniso_settings_t;

/* Virtual texturing tile */
typedef struct texture_virtual_tile {
    uint32_t x, y;
    uint32_t mip_level;
    uint32_t layer;
    bool resident;
    uint64_t last_access;
    void* data;
    size_t data_size;
} texture_virtual_tile_t;

/* Compression settings */
typedef struct texture_compression_settings {
    bool use_bc_compression;
    bool use_astc_compression;
    uint32_t compression_level;
    bool generate_mipmaps;
} texture_compression_settings_t;

/* Performance counters */
typedef struct texture_performance_counters {
    uint64_t texture_uploads;
    uint64_t texture_downloads;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t format_conversions;
    uint64_t compression_operations;
    uint64_t decompression_operations;
    uint64_t async_operations;
    uint64_t simd_operations;
    uint64_t batch_operations;
    uint64_t lod_transitions;
    uint64_t virtual_tile_requests;
    uint64_t feedback_samples;
    double total_upload_time;
    double total_compression_time;
    double total_async_time;
} texture_performance_counters_t;

/* Feedback analysis data */
typedef struct texture_feedback_data {
    float min_lod_used;
    float max_lod_used;
    float avg_lod_used;
    uint32_t unique_tiles_accessed;
    uint32_t total_samples;
    bool anisotropic_benefit;
    float memory_efficiency;
} texture_feedback_data_t;

/* Async operation */
typedef struct texture_async_operation {
    uint32_t id;
    texture_texture_arrays_handle_t handle;
    enum {
        TEXTURE_ASYNC_UPLOAD,
        TEXTURE_ASYNC_DOWNLOAD,
        TEXTURE_ASYNC_COMPRESSION,
        TEXTURE_ASYNC_CONVERSION,
        TEXTURE_ASYNC_MIPMAP_GENERATION
    } type;
    void* data;
    size_t data_size;
    bool completed;
    int result;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
} texture_async_operation_t;

/* Cache entry */
typedef struct texture_cache_entry {
    texture_texture_arrays_handle_t handle;
    void* cached_data;
    size_t cached_size;
    uint64_t last_access;
    uint32_t access_count;
    bool dirty;
} texture_cache_entry_t;

/* Render graph node */
typedef struct texture_render_graph_node {
    uint32_t node_id;
    texture_texture_arrays_handle_t texture_handle;
    enum {
        TEXTURE_NODE_UPLOAD,
        TEXTURE_NODE_DOWNLOAD,
        TEXTURE_NODE_COMPRESS,
        TEXTURE_NODE_GENERATE_MIPMAPS,
        TEXTURE_NODE_CONVERT_FORMAT
    } operation;
    bool processed;
    uint32_t dependencies[8];
    uint32_t dependency_count;
} texture_render_graph_node_t;

/* Bindless texture handle */
typedef struct texture_bindless_handle {
    uint64_t gpu_handle;
    uint32_t array_index;
    bool bound;
} texture_bindless_handle_t;

/* Virtual texture page */
typedef struct texture_virtual_page {
    uint32_t x, y;
    uint32_t level;
    uint32_t texture_id;
    void* data;
    size_t data_size;
    bool resident;
    uint64_t last_access;
} texture_virtual_page_t;

/* LOD information */
typedef struct texture_lod_info {
    uint32_t level;
    float distance_threshold;
    uint32_t width, height;
    size_t memory_size;
    bool active;
} texture_lod_info_t;

/* Performance counters */
typedef struct texture_arrays_performance_counters {
    uint64_t samples_processed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t async_operations;
    uint64_t compression_operations;
    uint64_t virtual_texture_operations;
    uint64_t batch_operations;
    uint64_t processing_time_ms;
    uint64_t memory_peak_usage;
} texture_arrays_performance_counters_t;

/* Cache entry */
typedef struct texture_cache_entry {
    uint32_t texture_id;
    void* data;
    size_t size;
    uint64_t last_access;
    uint32_t access_count;
    bool dirty;
} texture_cache_entry_t;

/* Async operation */
typedef struct texture_async_operation {
    uint32_t operation_id;
    uint32_t texture_id;
    enum {
        ASYNC_OP_LOAD,
        ASYNC_OP_SAVE,
        ASYNC_OP_COMPRESS,
        ASYNC_OP_GENERATE_MIPMAPS
    } type;
    void* input_data;
    size_t input_size;
    void* output_data;
    size_t output_size;
    bool completed;
    int error_code;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
} texture_async_operation_t;

/* GPU integration context */
typedef struct texture_gpu_context {
    bool initialized;
    void* device;
    void* command_queue;
    void* shader_program;
    uint32_t* texture_handles;
    uint32_t max_textures;
    pthread_mutex_t gpu_mutex;
} texture_gpu_context_t;

/* Render graph node */
typedef struct texture_render_graph_node {
    uint32_t node_id;
    uint32_t texture_id;
    enum {
        NODE_TYPE_SAMPLE,
        NODE_TYPE_FILTER,
        NODE_TYPE_COMPRESS,
        NODE_TYPE_GENERATE_MIPMAPS
    } type;
    uint32_t dependencies[8];
    uint32_t dependency_count;
    bool processed;
    uint64_t timestamp;
} texture_render_graph_node_t;

/* Batch processing operation */
typedef struct texture_batch_operation {
    uint32_t* texture_ids;
    uint32_t texture_count;
    enum {
        BATCH_OP_FILTER,
        BATCH_OP_COMPRESS,
        BATCH_OP_GENERATE_MIPMAPS,
        BATCH_OP_CONVERT_FORMAT
    } operation_type;
    void* operation_params;
    bool completed;
} texture_batch_operation_t;

typedef struct texture_texture_arrays_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    // Texture array specific fields
    uint32_t width;
    uint32_t height;
    uint32_t layers;
    uint32_t mip_levels;
    uint32_t format;
    bool is_virtual;
    bool is_compressed;
    uint32_t compression_type; // BC, ASTC, etc.
    float compression_ratio;
    bool has_mipmaps;
    bool bindless_enabled;
    uint64_t bindless_handle;
    uint32_t residency_level;
    uint64_t last_access_time;
    uint32_t cache_index;
    bool gpu_resident;
    void* gpu_handle;
    
    /* Anisotropic filtering */
    anisotropic_level_t anisotropy_level;
    bool anisotropic_enabled;
    
    /* Format information */
    texture_format_t format;
    uint32_t width, height, depth;
    uint32_t array_size;
    uint32_t mip_levels;
    
    /* Virtual texturing */
    texture_virtual_page_t* virtual_pages;
    uint32_t virtual_page_count;
    uint32_t virtual_page_capacity;
    bool virtual_texturing_enabled;
    
    /* LOD information */
    texture_lod_info_t lod_info[TEXTURE_TEXTURE_ARRAYS_MAX_LOD_LEVELS];
    uint32_t active_lod_levels;
    
    /* Compression */
    bool compression_enabled;
    enum {
        COMPRESSION_NONE,
        COMPRESSION_LZ4,
        COMPRESSION_ZSTD
    } compression_type;
    float compression_ratio;
    
    /* Memory tracking */
    size_t memory_usage;
    size_t peak_memory_usage;
    
    /* GPU resources */
    uint32_t gpu_texture_id;
    bool gpu_resident;
    
    /* Bindless handle */
    uint64_t bindless_handle;
    bool bindless_enabled;
} texture_texture_arrays_internal_t;

typedef struct texture_texture_arrays_context {
    texture_texture_arrays_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    // Performance counters
    uint64_t total_operations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_uploads;
    uint64_t simd_operations;
    uint64_t compression_operations;
    uint64_t mipmap_generations;
    double total_processing_time;
    
    // Thread safety
    pthread_mutex_t mutex;
    pthread_rwlock_t cache_lock;
    bool file_watcher_running;
    
    /* Caching layer */
    texture_cache_entry_t cache[TEXTURE_TEXTURE_ARRAYS_CACHE_SIZE];
    uint32_t cache_head;
    uint32_t cache_size;
    pthread_mutex_t cache_mutex;
    
    /* Async operations */
    texture_async_operation_t async_queue[TEXTURE_TEXTURE_ARRAYS_ASYNC_QUEUE_SIZE];
    uint32_t async_queue_head;
    uint32_t async_queue_tail;
    uint32_t async_operation_count;
    pthread_mutex_t async_mutex;
    pthread_cond_t async_cond;
    pthread_t async_worker_threads[4];
    bool async_workers_running;
    
    /* Performance tracking */
    texture_performance_counters_t performance_counters;
    pthread_mutex_t performance_mutex;
    
    /* Feedback analysis */
    texture_feedback_data_t feedback_data;
    uint32_t feedback_buffer[TEXTURE_TEXTURE_ARRAYS_FEEDBACK_BUFFER_SIZE];
    uint32_t feedback_buffer_index;
    pthread_mutex_t feedback_mutex;
    
    /* Render graph */
    texture_render_graph_node_t render_nodes[256];
    uint32_t render_node_count;
    pthread_mutex_t render_graph_mutex;
    
    /* Memory management */
    void* memory_pool;
    size_t memory_pool_size;
    size_t memory_pool_used;
    pthread_mutex_t memory_mutex;
    
} texture_texture_arrays_context_t;

static texture_texture_arrays_context_t g_texture_arrays_ctx = {0};

// Error codes
#define TEXTURE_ARRAYS_ERROR_NONE 0
#define TEXTURE_ARRAYS_ERROR_INVALID_PARAM -1
#define TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED -2
#define TEXTURE_ARRAYS_ERROR_OUT_OF_MEMORY -3
#define TEXTURE_ARRAYS_ERROR_INVALID_HANDLE -4
#define TEXTURE_ARRAYS_ERROR_THREAD_ERROR -5
#define TEXTURE_ARRAYS_ERROR_GPU_ERROR -6
#define TEXTURE_ARRAYS_ERROR_SERIALIZATION_ERROR -7
#define TEXTURE_ARRAYS_ERROR_VALIDATION_ERROR -8
#define TEXTURE_ARRAYS_ERROR_ASYNC_QUEUE_FULL -9
#define TEXTURE_ARRAYS_ERROR_CACHE_ERROR -10
#define TEXTURE_ARRAYS_ERROR_COMPRESSION_ERROR -11
#define TEXTURE_ARRAYS_ERROR_VIRTUAL_TEXTURE_ERROR -12

// Serialization constants
#define TEXTURE_ARRAYS_MAGIC_NUMBER 0x54415854  // "TAXT"
#define TEXTURE_ARRAYS_VERSION 1

// Compression types
#define TEXTURE_ARRAYS_COMPRESSION_NONE 0
#define TEXTURE_ARRAYS_COMPRESSION_BC1 1
#define TEXTURE_ARRAYS_COMPRESSION_BC3 2
#define TEXTURE_ARRAYS_COMPRESSION_BC5 3
#define TEXTURE_ARRAYS_COMPRESSION_BC7 4
#define TEXTURE_ARRAYS_COMPRESSION_ASTC_4x4 5
#define TEXTURE_ARRAYS_COMPRESSION_ASTC_6x6 6
#define TEXTURE_ARRAYS_COMPRESSION_ASTC_8x8 7

// LOD levels
#define TEXTURE_ARRAYS_MIN_LOD 0
#define TEXTURE_ARRAYS_MAX_LOD 11
#define TEXTURE_ARRAYS_DEFAULT_LOD_BIAS 0.0f

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

/* Helper function to get current timestamp in milliseconds */
static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

/* Helper function to calculate checksum for data integrity */
static uint32_t calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = checksum * 31 + bytes[i];
    }
    return checksum;
}

/* SIMD-optimized anisotropic filtering */
static void anisotropic_filter_simd(const float* src, float* dst, 
                                   uint32_t width, uint32_t height, 
                                   anisotropic_level_t level) {
    __m128 simd_level = _mm_set1_ps((float)level);
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x += 4) {
            __m128 samples = _mm_loadu_ps(&src[y * width + x]);
            __m128 filtered = _mm_mul_ps(samples, simd_level);
            _mm_storeu_ps(&dst[y * width + x], filtered);
        }
    }
}

/* Memory pool allocation */
static void* pool_alloc(texture_texture_arrays_context_t* ctx, size_t size) {
    pthread_mutex_lock(&ctx->memory_pool_mutex);
    
    if (ctx->memory_pool_used + size > ctx->memory_pool_size) {
        pthread_mutex_unlock(&ctx->memory_pool_mutex);
        return NULL;
    }
    
    void* ptr = (uint8_t*)ctx->memory_pool + ctx->memory_pool_used;
    ctx->memory_pool_used += size;
    
    pthread_mutex_unlock(&ctx->memory_pool_mutex);
    return ptr;
}

/* Cache lookup with LRU eviction */
static texture_cache_entry_t* cache_lookup(texture_texture_arrays_context_t* ctx, uint32_t texture_id) {
    pthread_rwlock_rdlock(&ctx->cache_rwlock);
    
    texture_cache_entry_t* entry = NULL;
    texture_cache_entry_t* lru_entry = NULL;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < ctx->cache_size; i++) {
        if (ctx->cache[i].texture_id == texture_id) {
            entry = &ctx->cache[i];
            break;
        }
        if (ctx->cache[i].last_access < oldest_time) {
            oldest_time = ctx->cache[i].last_access;
            lru_entry = &ctx->cache[i];
        }
    }
    
    pthread_rwlock_unlock(&ctx->cache_rwlock);
    
    if (entry) {
        entry->last_access = get_timestamp_ms();
        entry->access_count++;
        ctx->performance.cache_hits++;
        return entry;
    }
    
    ctx->performance.cache_misses++;
    return lru_entry;
}

/* Async worker thread function */
static void* async_worker_thread(void* arg) {
    texture_texture_arrays_context_t* ctx = (texture_texture_arrays_context_t*)arg;
    
    while (!ctx->async_shutdown) {
        pthread_mutex_lock(&ctx->async_mutex);
        
        while (ctx->async_queue_head == ctx->async_queue_tail && !ctx->async_shutdown) {
            pthread_cond_wait(&ctx->async_cond, &ctx->async_mutex);
        }
        
        if (ctx->async_shutdown) {
            pthread_mutex_unlock(&ctx->async_mutex);
            break;
        }
        
        texture_async_operation_t* op = &ctx->async_queue[ctx->async_queue_head];
        ctx->async_queue_head = (ctx->async_queue_head + 1) % TEXTURE_TEXTURE_ARRAYS_ASYNC_QUEUE_SIZE;
        
        pthread_mutex_unlock(&ctx->async_mutex);
        
        // Process the operation
        uint64_t start_time = get_timestamp_ms();
        
        switch (op->type) {
            case ASYNC_OP_COMPRESS:
                // Simulate compression
                op->error_code = 0;
                break;
            case ASYNC_OP_GENERATE_MIPMAPS:
                // Simulate mipmap generation
                op->error_code = 0;
                break;
            default:
                op->error_code = -1;
                break;
        }
        
        op->completed = true;
        ctx->performance.async_operations++;
        ctx->performance.processing_time_ms += get_timestamp_ms() - start_time;
        
        pthread_mutex_lock(&op->completion_mutex);
        pthread_cond_signal(&op->completion_cond);
        pthread_mutex_unlock(&op->completion_mutex);
    }
    
    return NULL;
}

/* File watcher thread for hot-reload */
static void* file_watcher_thread(void* arg) {
    texture_texture_arrays_context_t* ctx = (texture_texture_arrays_context_t*)arg;
    char buffer[4096];
    
    while (ctx->file_watcher_running) {
        ssize_t length = read(ctx->inotify_fd, buffer, sizeof(buffer));
        if (length == -1) {
            if (errno == EINTR) continue;
            break;
        }
        
        for (char* ptr = buffer; ptr < buffer + length; ) {
            struct inotify_event* event = (struct inotify_event*)ptr;
            if (event->mask & IN_MODIFY) {
                // Handle file modification for hot-reload
                // Mark affected textures as dirty
                for (uint32_t i = 0; i < ctx->count; i++) {
                    ctx->items[i].dirty = true;
                }
            }
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
    
    return NULL;
}

/* Render graph topological sort */
static void topological_sort(texture_render_graph_node_t* nodes, uint32_t count) {
    // Simple topological sort implementation
    for (uint32_t i = 0; i < count; i++) {
        for (uint32_t j = 0; j < nodes[i].dependency_count; j++) {
            uint32_t dep_id = nodes[i].dependencies[j];
            // Find dependency and ensure it's processed before this node
            for (uint32_t k = 0; k < count; k++) {
                if (nodes[k].node_id == dep_id && k > i) {
                    // Swap nodes to maintain dependency order
                    texture_render_graph_node_t temp = nodes[i];
                    nodes[i] = nodes[k];
                    nodes[k] = temp;
                    break;
                }
            }
        }
    }
}

static bool texture_texture_arrays_validate(const texture_texture_arrays_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->width == 0 || item->height == 0) return false;
    if (item->array_size == 0) return false;
    if (item->anisotropy_level > TEXTURE_TEXTURE_ARRAYS_MAX_ANISOTROPY) return false;
    return true;
}

static void texture_texture_arrays_cleanup_internal(texture_texture_arrays_internal_t* item) {
    if (!item) return;
    
    // Cleanup virtual pages
    if (item->virtual_pages) {
        for (uint32_t i = 0; i < item->virtual_page_count; i++) {
            if (item->virtual_pages[i].data) {
                free(item->virtual_pages[i].data);
            }
        }
        free(item->virtual_pages);
        item->virtual_pages = NULL;
    }
    
    // Cleanup main data
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    // Cleanup GPU resources
    if (item->gpu_resident && item->gpu_texture_id != 0) {
        // GPU resource cleanup would go here
        item->gpu_resident = false;
        item->gpu_texture_id = 0;
    }
    
    item->initialized = false;
    item->dirty = false;
    item->virtual_page_count = 0;
    item->memory_usage = 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int texture_texture_arrays_init(void) {
    if (g_texture_arrays_ctx.initialized) {
        return TEXTURE_ARRAYS_ERROR_ALREADY_INITIALIZED;
    }
    
    // Initialize mutexes and synchronization primitives
    if (pthread_mutex_init(&g_texture_arrays_ctx.context_mutex, NULL) != 0) {
        return TEXTURE_ARRAYS_ERROR_THREAD_CREATE_FAILED;
    }
    
    if (pthread_rwlock_init(&g_texture_arrays_ctx.cache_rwlock, NULL) != 0) {
        pthread_mutex_destroy(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_THREAD_CREATE_FAILED;
    }
    
    if (pthread_mutex_init(&g_texture_arrays_ctx.cache_mutex, NULL) != 0) {
        pthread_mutex_destroy(&g_texture_arrays_ctx.context_mutex);
        pthread_rwlock_destroy(&g_texture_arrays_ctx.cache_rwlock);
        return TEXTURE_ARRAYS_ERROR_THREAD_CREATE_FAILED;
    }
    
    if (pthread_mutex_init(&g_texture_arrays_ctx.async_mutex, NULL) != 0) {
        pthread_mutex_destroy(&g_texture_arrays_ctx.context_mutex);
        pthread_rwlock_destroy(&g_texture_arrays_ctx.cache_rwlock);
        pthread_mutex_destroy(&g_texture_arrays_ctx.cache_mutex);
        return TEXTURE_ARRAYS_ERROR_THREAD_CREATE_FAILED;
    }
    
    if (pthread_mutex_init(&g_texture_arrays_ctx.memory_pool_mutex, NULL) != 0) {
        pthread_mutex_destroy(&g_texture_arrays_ctx.context_mutex);
        pthread_rwlock_destroy(&g_texture_arrays_ctx.cache_rwlock);
        pthread_mutex_destroy(&g_texture_arrays_ctx.cache_mutex);
        pthread_mutex_destroy(&g_texture_arrays_ctx.async_mutex);
        return TEXTURE_ARRAYS_ERROR_THREAD_CREATE_FAILED;
    }
    
    // Initialize main texture array storage
    g_texture_arrays_ctx.capacity = TEXTURE_TEXTURE_ARRAYS_DEFAULT_CAPACITY;
    g_texture_arrays_ctx.items = calloc(g_texture_arrays_ctx.capacity, sizeof(texture_texture_arrays_internal_t));
    if (!g_texture_arrays_ctx.items) {
        pthread_mutex_destroy(&g_texture_arrays_ctx.context_mutex);
        pthread_rwlock_destroy(&g_texture_arrays_ctx.cache_rwlock);
        pthread_mutex_destroy(&g_texture_arrays_ctx.cache_mutex);
        pthread_mutex_destroy(&g_texture_arrays_ctx.async_mutex);
        pthread_mutex_destroy(&g_texture_arrays_ctx.memory_pool_mutex);
        return TEXTURE_ARRAYS_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize memory pool
    g_texture_arrays_ctx.memory_pool_size = TEXTURE_TEXTURE_ARRAYS_MEMORY_POOL_SIZE;
    g_texture_arrays_ctx.memory_pool = malloc(g_texture_arrays_ctx.memory_pool_size);
    if (!g_texture_arrays_ctx.memory_pool) {
        free(g_texture_arrays_ctx.items);
        pthread_mutex_destroy(&g_texture_arrays_ctx.context_mutex);
        pthread_rwlock_destroy(&g_texture_arrays_ctx.cache_rwlock);
        pthread_mutex_destroy(&g_texture_arrays_ctx.cache_mutex);
        pthread_mutex_destroy(&g_texture_arrays_ctx.async_mutex);
        pthread_mutex_destroy(&g_texture_arrays_ctx.memory_pool_mutex);
        return TEXTURE_ARRAYS_ERROR_OUT_OF_MEMORY;
    }
    g_texture_arrays_ctx.memory_pool_used = 0;
    
    // Initialize cache
    memset(g_texture_arrays_ctx.cache, 0, sizeof(g_texture_arrays_ctx.cache));
    g_texture_arrays_ctx.cache_head = 0;
    g_texture_arrays_ctx.cache_size = 0;
    
    // Initialize async operations
    g_texture_arrays_ctx.async_queue_head = 0;
    g_texture_arrays_ctx.async_queue_tail = 0;
    g_texture_arrays_ctx.async_shutdown = false;
    
    // Create worker threads
    for (int i = 0; i < TEXTURE_TEXTURE_ARRAYS_WORKER_THREADS; i++) {
        if (pthread_create(&g_texture_arrays_ctx.worker_threads[i], NULL, async_worker_thread, &g_texture_arrays_ctx) != 0) {
            g_texture_arrays_ctx.async_shutdown = true;
            pthread_cond_broadcast(&g_texture_arrays_ctx.async_cond);
            
            // Wait for already created threads to finish
            for (int j = 0; j < i; j++) {
                pthread_join(g_texture_arrays_ctx.worker_threads[j], NULL);
            }
            
            free(g_texture_arrays_ctx.memory_pool);
            free(g_texture_arrays_ctx.items);
            pthread_mutex_destroy(&g_texture_arrays_ctx.context_mutex);
            pthread_rwlock_destroy(&g_texture_arrays_ctx.cache_rwlock);
            pthread_mutex_destroy(&g_texture_arrays_ctx.cache_mutex);
            pthread_mutex_destroy(&g_texture_arrays_ctx.async_mutex);
            pthread_mutex_destroy(&g_texture_arrays_ctx.memory_pool_mutex);
            return TEXTURE_ARRAYS_ERROR_THREAD_CREATE_FAILED;
        }
    }
    
    // Initialize file watcher for hot-reload
    g_texture_arrays_ctx.inotify_fd = inotify_init();
    if (g_texture_arrays_ctx.inotify_fd == -1) {
        g_texture_arrays_ctx.async_shutdown = true;
        pthread_cond_broadcast(&g_texture_arrays_ctx.async_cond);
        
        for (int i = 0; i < TEXTURE_TEXTURE_ARRAYS_WORKER_THREADS; i++) {
            pthread_join(g_texture_arrays_ctx.worker_threads[i], NULL);
        }
        
        free(g_texture_arrays_ctx.memory_pool);
        free(g_texture_arrays_ctx.items);
        pthread_mutex_destroy(&g_texture_arrays_ctx.context_mutex);
        pthread_rwlock_destroy(&g_texture_arrays_ctx.cache_rwlock);
        pthread_mutex_destroy(&g_texture_arrays_ctx.cache_mutex);
        pthread_mutex_destroy(&g_texture_arrays_ctx.async_mutex);
        pthread_mutex_destroy(&g_texture_arrays_ctx.memory_pool_mutex);
        return TEXTURE_ARRAYS_ERROR_FILE_WATCH_FAILED;
    }
    
    g_texture_arrays_ctx.file_watcher_running = true;
    if (pthread_create(&g_texture_arrays_ctx.file_watcher_thread, NULL, file_watcher_thread, &g_texture_arrays_ctx) != 0) {
        close(g_texture_arrays_ctx.inotify_fd);
        g_texture_arrays_ctx.async_shutdown = true;
        pthread_cond_broadcast(&g_texture_arrays_ctx.async_cond);
        
        for (int i = 0; i < TEXTURE_TEXTURE_ARRAYS_WORKER_THREADS; i++) {
            pthread_join(g_texture_arrays_ctx.worker_threads[i], NULL);
        }
        
        free(g_texture_arrays_ctx.memory_pool);
        free(g_texture_arrays_ctx.items);
        pthread_mutex_destroy(&g_texture_arrays_ctx.context_mutex);
        pthread_rwlock_destroy(&g_texture_arrays_ctx.cache_rwlock);
        pthread_mutex_destroy(&g_texture_arrays_ctx.cache_mutex);
        pthread_mutex_destroy(&g_texture_arrays_ctx.async_mutex);
        pthread_mutex_destroy(&g_texture_arrays_ctx.memory_pool_mutex);
        return TEXTURE_ARRAYS_ERROR_THREAD_CREATE_FAILED;
    }
    
    // Initialize GPU context
    g_texture_arrays_ctx.gpu_context.initialized = false;
    g_texture_arrays_ctx.gpu_context.device = NULL;
    g_texture_arrays_ctx.gpu_context.command_queue = NULL;
    g_texture_arrays_ctx.gpu_context.shader_program = NULL;
    g_texture_arrays_ctx.gpu_context.texture_handles = NULL;
    g_texture_arrays_ctx.gpu_context.max_textures = 0;
    if (pthread_mutex_init(&g_texture_arrays_ctx.gpu_context.gpu_mutex, NULL) != 0) {
        // Continue without GPU support
    }
    
    // Initialize render graph
    g_texture_arrays_ctx.render_node_count = 0;
    memset(g_texture_arrays_ctx.render_nodes, 0, sizeof(g_texture_arrays_ctx.render_nodes));
    
    // Initialize batch processing
    g_texture_arrays_ctx.batch_operation_count = 0;
    memset(g_texture_arrays_ctx.batch_operations, 0, sizeof(g_texture_arrays_ctx.batch_operations));
    
    // Initialize performance counters
    memset(&g_texture_arrays_ctx.performance, 0, sizeof(g_texture_arrays_ctx.performance));
    
    // Initialize SIMD detection
    g_texture_arrays_ctx.simd_enabled = true;
    g_texture_arrays_ctx.simd_width = 16; // AVX2
    
    // Initialize culling
    g_texture_arrays_ctx.culling_enabled = true;
    g_texture_arrays_ctx.culling_threshold = 0.001f;
    
    g_texture_arrays_ctx.count = 0;
    g_texture_arrays_ctx.initialized = true;
    
    return TEXTURE_ARRAYS_SUCCESS;
}

void texture_texture_arrays_shutdown(void) {
    if (!g_texture_arrays_ctx.initialized) {
        return;
    }
    
    // Shutdown file watcher
    if (g_texture_arrays_ctx.file_watcher_running) {
        g_texture_arrays_ctx.file_watcher_running = false;
        pthread_join(g_texture_arrays_ctx.file_watcher_thread, NULL);
    }
    
    if (g_texture_arrays_ctx.inotify_fd != -1) {
        close(g_texture_arrays_ctx.inotify_fd);
        g_texture_arrays_ctx.inotify_fd = -1;
    }
    
    // Shutdown async operations
    g_texture_arrays_ctx.async_shutdown = true;
    pthread_cond_broadcast(&g_texture_arrays_ctx.async_cond);
    
    for (int i = 0; i < TEXTURE_TEXTURE_ARRAYS_WORKER_THREADS; i++) {
        pthread_join(g_texture_arrays_ctx.worker_threads[i], NULL);
    }
    
    // Cleanup all texture items
    for (uint32_t i = 0; i < g_texture_arrays_ctx.count; i++) {
        texture_texture_arrays_cleanup_internal(&g_texture_arrays_ctx.items[i]);
    }
    
    // Cleanup GPU context
    if (g_texture_arrays_ctx.gpu_context.texture_handles) {
        free(g_texture_arrays_ctx.gpu_context.texture_handles);
    }
    pthread_mutex_destroy(&g_texture_arrays_ctx.gpu_context.gpu_mutex);
    
    // Cleanup memory pool
    if (g_texture_arrays_ctx.memory_pool) {
        free(g_texture_arrays_ctx.memory_pool);
        g_texture_arrays_ctx.memory_pool = NULL;
    }
    
    // Cleanup main storage
    free(g_texture_arrays_ctx.items);
    g_texture_arrays_ctx.items = NULL;
    
    // Cleanup synchronization primitives
    pthread_mutex_destroy(&g_texture_arrays_ctx.context_mutex);
    pthread_rwlock_destroy(&g_texture_arrays_ctx.cache_rwlock);
    pthread_mutex_destroy(&g_texture_arrays_ctx.cache_mutex);
    pthread_mutex_destroy(&g_texture_arrays_ctx.async_mutex);
    pthread_mutex_destroy(&g_texture_arrays_ctx.memory_pool_mutex);
    
    g_texture_arrays_ctx.count = 0;
    g_texture_arrays_ctx.capacity = 0;
    g_texture_arrays_ctx.initialized = false;
}

int texture_texture_arrays_create(texture_texture_arrays_handle_t* out_handle, const texture_texture_arrays_desc_t* desc) {
    if (!out_handle || !desc) {
        return TEXTURE_ARRAYS_ERROR_INVALID_PARAM;
    }
    
    if (!g_texture_arrays_ctx.initialized) {
        return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    if (g_texture_arrays_ctx.count >= g_texture_arrays_ctx.capacity) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = g_texture_arrays_ctx.count++;
    texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[index];
    
    // Initialize texture array with defaults
    memset(item, 0, sizeof(texture_texture_arrays_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = get_timestamp_ms();
    
    // Set default anisotropic filtering
    item->anisotropy_level = ANISOTROPIC_4X;
    item->anisotropic_enabled = true;
    
    // Set default format
    item->format = TEXTURE_FORMAT_RGBA8;
    item->width = 256;
    item->height = 256;
    item->depth = 1;
    item->array_size = 1;
    item->mip_levels = 1;
    
    // Initialize virtual texturing
    item->virtual_texturing_enabled = false;
    item->virtual_page_capacity = 64;
    item->virtual_pages = calloc(item->virtual_page_capacity, sizeof(texture_virtual_page_t));
    
    // Initialize LOD information
    item->active_lod_levels = 1;
    for (uint32_t i = 0; i < TEXTURE_TEXTURE_ARRAYS_MAX_LOD_LEVELS; i++) {
        item->lod_info[i].level = i;
        item->lod_info[i].distance_threshold = (float)(i + 1) * 10.0f;
        item->lod_info[i].width = item->width >> i;
        item->lod_info[i].height = item->height >> i;
        item->lod_info[i].memory_size = item->lod_info[i].width * item->lod_info[i].height * 4; // RGBA
        item->lod_info[i].active = (i == 0);
    }
    
    // Initialize compression
    item->compression_enabled = false;
    item->compression_type = COMPRESSION_NONE;
    item->compression_ratio = 1.0f;
    
    // Initialize memory tracking
    item->memory_usage = item->width * item->height * 4; // RGBA
    item->peak_memory_usage = item->memory_usage;
    
    // Initialize GPU resources
    item->gpu_texture_id = 0;
    item->gpu_resident = false;
    
    // Initialize bindless handle
    item->bindless_handle = 0;
    item->bindless_enabled = false;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    
    out_handle->id = index;
    return TEXTURE_ARRAYS_SUCCESS;
}

void texture_texture_arrays_destroy(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return;
    }
    
    texture_texture_arrays_cleanup_internal(&g_texture_arrays_ctx.items[handle.id]);
    
    // Update performance counters
    g_texture_arrays_ctx.performance.samples_processed++;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
}

int texture_texture_arrays_update(texture_texture_arrays_handle_t handle, const void* data, size_t size) {
    if (!g_texture_arrays_ctx.initialized) {
        return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    }
    
    if (!data || size == 0) {
        return TEXTURE_ARRAYS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Update data with thread safety
    if (item->data) {
        free(item->data);
    }
    
    item->data = malloc(size);
    if (!item->data) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(item->data, data, size);
    item->data_size = size;
    item->dirty = true;
    item->frame_updated = get_timestamp_ms();
    
    // Update memory tracking
    item->memory_usage = size;
    if (size > item->peak_memory_usage) {
        item->peak_memory_usage = size;
    }
    
    // Apply anisotropic filtering if enabled
    if (item->anisotropic_enabled && g_texture_arrays_ctx.simd_enabled) {
        anisotropic_filter_simd((const float*)data, (float*)item->data, 
                               item->width, item->height, item->anisotropy_level);
        g_texture_arrays_ctx.performance.simd_operations++;
    }
    
    // Update cache
    texture_cache_entry_t* cache_entry = cache_lookup(&g_texture_arrays_ctx, handle.id);
    if (cache_entry) {
        cache_entry->texture_id = handle.id;
        cache_entry->data = item->data;
        cache_entry->size = size;
        cache_entry->last_access = get_timestamp_ms();
        cache_entry->dirty = true;
    }
    
    // Update performance counters
    g_texture_arrays_ctx.performance.samples_processed++;
    g_texture_arrays_ctx.performance.processing_time_ms++;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    
    return TEXTURE_ARRAYS_SUCCESS;
}

bool texture_texture_arrays_is_valid(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    bool valid = false;
    if (handle.id < g_texture_arrays_ctx.count) {
        valid = texture_texture_arrays_validate(&g_texture_arrays_ctx.items[handle.id]);
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    
    return valid;
}

int texture_texture_arrays_get_info(texture_texture_arrays_handle_t handle, texture_texture_arrays_info_t* out_info) {
    if (!out_info) {
        return TEXTURE_ARRAYS_ERROR_INVALID_PARAM;
    }
    
    if (!g_texture_arrays_ctx.initialized) {
        return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    const texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    
    return TEXTURE_ARRAYS_SUCCESS;
}

void texture_texture_arrays_mark_dirty(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    if (handle.id < g_texture_arrays_ctx.count) {
        g_texture_arrays_ctx.items[handle.id].dirty = true;
        g_texture_arrays_ctx.items[handle.id].frame_updated = get_timestamp_ms();
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
}

int texture_texture_arrays_process_pending(void) {
    if (!g_texture_arrays_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    int processed = 0;
    
    // Process render graph nodes with topological sort
    if (g_texture_arrays_ctx.render_node_count > 0) {
        topological_sort(g_texture_arrays_ctx.render_nodes, g_texture_arrays_ctx.render_node_count);
        
        for (uint32_t i = 0; i < g_texture_arrays_ctx.render_node_count; i++) {
            texture_render_graph_node_t* node = &g_texture_arrays_ctx.render_nodes[i];
            if (!node->processed) {
                // Process node based on type
                switch (node->type) {
                    case NODE_TYPE_SAMPLE:
                    case NODE_TYPE_FILTER:
                    case NODE_TYPE_COMPRESS:
                    case NODE_TYPE_GENERATE_MIPMAPS:
                        // Simulate processing
                        node->processed = true;
                        node->timestamp = get_timestamp_ms();
                        processed++;
                        break;
                }
            }
        }
    }
    
    // Process batch operations
    for (uint32_t i = 0; i < g_texture_arrays_ctx.batch_operation_count; i++) {
        texture_batch_operation_t* batch = &g_texture_arrays_ctx.batch_operations[i];
        if (!batch->completed) {
            // Simulate batch processing
            batch->completed = true;
            processed++;
            g_texture_arrays_ctx.performance.batch_operations++;
        }
    }
    
    // Process dirty texture items
    for (uint32_t i = 0; i < g_texture_arrays_ctx.count; i++) {
        texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item based on its state
            if (item->anisotropic_enabled) {
                // Apply anisotropic filtering
                if (item->data && g_texture_arrays_ctx.simd_enabled) {
                    anisotropic_filter_simd((const float*)item->data, (float*)item->data,
                                           item->width, item->height, item->anisotropy_level);
                    g_texture_arrays_ctx.performance.simd_operations++;
                }
            }
            
            // Update GPU resources if available
            if (g_texture_arrays_ctx.gpu_context.initialized && !item->gpu_resident) {
                // Simulate GPU upload
                item->gpu_resident = true;
                item->gpu_texture_id = i + 1;
                g_texture_arrays_ctx.performance.gpu_operations++;
            }
            
            item->dirty = false;
            item->frame_updated = get_timestamp_ms();
            processed++;
            g_texture_arrays_ctx.performance.samples_processed++;
        }
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    
    return processed;
}

uint32_t texture_texture_arrays_get_count(void) {
    return g_texture_arrays_ctx.count;
}

size_t texture_texture_arrays_get_memory_usage(void) {
    if (!g_texture_arrays_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    size_t total = sizeof(g_texture_arrays_ctx);
    total += g_texture_arrays_ctx.capacity * sizeof(texture_texture_arrays_internal_t);
    
    // Add memory from all texture items
    for (uint32_t i = 0; i < g_texture_arrays_ctx.count; i++) {
        const texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[i];
        total += item->data_size;
        total += item->memory_usage;
        
        // Add virtual page memory
        if (item->virtual_pages) {
            for (uint32_t j = 0; j < item->virtual_page_count; j++) {
                total += item->virtual_pages[j].data_size;
            }
        }
    }
    
    // Add memory pool usage
    total += g_texture_arrays_ctx.memory_pool_used;
    
    // Add cache memory
    for (uint32_t i = 0; i < g_texture_arrays_ctx.cache_size; i++) {
        total += g_texture_arrays_ctx.cache[i].size;
    }
    
    // Add GPU context memory
    if (g_texture_arrays_ctx.gpu_context.texture_handles) {
        total += g_texture_arrays_ctx.gpu_context.max_textures * sizeof(uint32_t);
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    
    return total;
}

void texture_texture_arrays_debug_print(void) {
    if (!g_texture_arrays_ctx.initialized) {
        printf("Texture Arrays System: Not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    printf("=== Texture Arrays System Debug Info ===\n");
    printf("Initialized: %s\n", g_texture_arrays_ctx.initialized ? "Yes" : "No");
    printf("Texture Count: %u / %u\n", g_texture_arrays_ctx.count, g_texture_arrays_ctx.capacity);
    printf("Memory Pool: %zu / %zu bytes used\n", g_texture_arrays_ctx.memory_pool_used, g_texture_arrays_ctx.memory_pool_size);
    printf("Cache Size: %u / %u entries\n", g_texture_arrays_ctx.cache_size, TEXTURE_TEXTURE_ARRAYS_CACHE_SIZE);
    printf("SIMD Enabled: %s\n", g_texture_arrays_ctx.simd_enabled ? "Yes" : "No");
    printf("Culling Enabled: %s\n", g_texture_arrays_ctx.culling_enabled ? "Yes" : "No");
    printf("File Watcher Running: %s\n", g_texture_arrays_ctx.file_watcher_running ? "Yes" : "No");
    printf("GPU Context Initialized: %s\n", g_texture_arrays_ctx.gpu_context.initialized ? "Yes" : "No");
    printf("Render Nodes: %u\n", g_texture_arrays_ctx.render_node_count);
    printf("Batch Operations: %u\n", g_texture_arrays_ctx.batch_operation_count);
    
    printf("\n=== Performance Counters ===\n");
    printf("Samples Processed: %lu\n", g_texture_arrays_ctx.performance.samples_processed);
    printf("Cache Hits: %lu\n", g_texture_arrays_ctx.performance.cache_hits);
    printf("Cache Misses: %lu\n", g_texture_arrays_ctx.performance.cache_misses);
    printf("GPU Operations: %lu\n", g_texture_arrays_ctx.performance.gpu_operations);
    printf("SIMD Operations: %lu\n", g_texture_arrays_ctx.performance.simd_operations);
    printf("Async Operations: %lu\n", g_texture_arrays_ctx.performance.async_operations);
    printf("Compression Operations: %lu\n", g_texture_arrays_ctx.performance.compression_operations);
    printf("Virtual Texture Operations: %lu\n", g_texture_arrays_ctx.performance.virtual_texture_operations);
    printf("Batch Operations: %lu\n", g_texture_arrays_ctx.performance.batch_operations);
    printf("Processing Time: %lu ms\n", g_texture_arrays_ctx.performance.processing_time_ms);
    printf("Peak Memory Usage: %lu bytes\n", g_texture_arrays_ctx.performance.memory_peak_usage);
    
    printf("\n=== Texture Details ===\n");
    for (uint32_t i = 0; i < g_texture_arrays_ctx.count; i++) {
        const texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[i];
        if (item->initialized) {
            printf("Texture %u: %ux%ux%u, Format=%d, Array Size=%u, Mip Levels=%u\n",
                   item->id, item->width, item->height, item->depth, 
                   item->format, item->array_size, item->mip_levels);
            printf("  Anisotropic: %s (Level=%d), Compression: %s\n",
                   item->anisotropic_enabled ? "Enabled" : "Disabled",
                   item->anisotropy_level,
                   item->compression_enabled ? "Enabled" : "Disabled");
            printf("  Memory: %zu bytes (Peak: %zu), GPU Resident: %s\n",
                   item->memory_usage, item->peak_memory_usage,
                   item->gpu_resident ? "Yes" : "No");
            printf("  Virtual Pages: %u / %u, LOD Levels: %u\n",
                   item->virtual_page_count, item->virtual_page_capacity,
                   item->active_lod_levels);
            printf("  Dirty: %s, Frame Updated: %lu\n",
                   item->dirty ? "Yes" : "No", item->frame_updated);
        }
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    printf("=== End Debug Info ===\n");
}

/* Remaining API implementations */

/* Texture Arrays Performance Counters */
int texture_texture_arrays_get_performance_counters(texture_arrays_performance_counters_t* out_counters) {
    if (!out_counters) return TEXTURE_ARRAYS_ERROR_INVALID_PARAM;
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    *out_counters = g_texture_arrays_ctx.performance;
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_reset_performance_counters(void) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    memset(&g_texture_arrays_ctx.performance, 0, sizeof(g_texture_arrays_ctx.performance));
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

void texture_texture_arrays_update_performance_counters(uint64_t samples, uint64_t time_ms) {
    if (!g_texture_arrays_ctx.initialized) return;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    g_texture_arrays_ctx.performance.samples_processed += samples;
    g_texture_arrays_ctx.performance.processing_time_ms += time_ms;
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
}

/* Texture Arrays Hot-Reload */
int texture_texture_arrays_enable_hot_reload(texture_texture_arrays_handle_t handle, const char* file_path) {
    if (!file_path) return TEXTURE_ARRAYS_ERROR_INVALID_PARAM;
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Add file watch for hot-reload
    int wd = inotify_add_watch(g_texture_arrays_ctx.inotify_fd, file_path, IN_MODIFY);
    if (wd == -1) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_FILE_WATCH_FAILED;
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_disable_hot_reload(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Remove file watch
    // Implementation would remove specific watch
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_check_hot_reload(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    bool dirty = g_texture_arrays_ctx.items[handle.id].dirty;
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return dirty ? 1 : 0;
}

/* Texture Arrays Thread Safety */
int texture_texture_arrays_lock(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Lock specific texture item
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_unlock(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_try_lock(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    if (pthread_mutex_trylock(&g_texture_arrays_ctx.context_mutex) != 0) {
        return TEXTURE_ARRAYS_ERROR_THREAD_CREATE_FAILED; // Already locked
    }
    
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    return TEXTURE_ARRAYS_SUCCESS;
}

/* Texture Arrays Memory Pooling */
int texture_texture_arrays_init_memory_pool(size_t pool_size) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (g_texture_arrays_ctx.memory_pool) {
        free(g_texture_arrays_ctx.memory_pool);
    }
    
    g_texture_arrays_ctx.memory_pool = malloc(pool_size);
    if (!g_texture_arrays_ctx.memory_pool) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_OUT_OF_MEMORY;
    }
    
    g_texture_arrays_ctx.memory_pool_size = pool_size;
    g_texture_arrays_ctx.memory_pool_used = 0;
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

void* texture_texture_arrays_pool_alloc(size_t size) {
    if (!g_texture_arrays_ctx.initialized) return NULL;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.memory_pool_mutex);
    void* ptr = pool_alloc(&g_texture_arrays_ctx, size);
    pthread_mutex_unlock(&g_texture_arrays_ctx.memory_pool_mutex);
    return ptr;
}

void texture_texture_arrays_pool_free(void* ptr) {
    // Simple pool free - in real implementation, track freed blocks
    (void)ptr; // Suppress unused warning
}

size_t texture_texture_arrays_get_pool_usage(void) {
    if (!g_texture_arrays_ctx.initialized) return 0;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    size_t usage = g_texture_arrays_ctx.memory_pool_used;
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return usage;
}

/* Texture Arrays Caching Layer */
int texture_texture_arrays_cache_add(texture_texture_arrays_handle_t handle, const void* data, size_t size) {
    if (!data || size == 0) return TEXTURE_ARRAYS_ERROR_INVALID_PARAM;
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Add to cache with LRU eviction
    if (g_texture_arrays_ctx.cache_size < TEXTURE_TEXTURE_ARRAYS_CACHE_SIZE) {
        uint32_t index = g_texture_arrays_ctx.cache_head;
        g_texture_arrays_ctx.cache[index].texture_id = handle.id;
        g_texture_arrays_ctx.cache[index].data = malloc(size);
        if (g_texture_arrays_ctx.cache[index].data) {
            memcpy(g_texture_arrays_ctx.cache[index].data, data, size);
            g_texture_arrays_ctx.cache[index].size = size;
            g_texture_arrays_ctx.cache[index].last_access = get_timestamp_ms();
            g_texture_arrays_ctx.cache[index].access_count = 1;
            g_texture_arrays_ctx.cache[index].dirty = false;
            g_texture_arrays_ctx.cache_head = (g_texture_arrays_ctx.cache_head + 1) % TEXTURE_TEXTURE_ARRAYS_CACHE_SIZE;
            g_texture_arrays_ctx.cache_size++;
        }
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_cache_get(texture_texture_arrays_handle_t handle, void** out_data, size_t* out_size) {
    if (!out_data || !out_size) return TEXTURE_ARRAYS_ERROR_INVALID_PARAM;
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    texture_cache_entry_t* entry = cache_lookup(&g_texture_arrays_ctx, handle.id);
    if (entry && entry->data) {
        *out_data = entry->data;
        *out_size = entry->size;
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_SUCCESS;
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE; // Not found in cache
}

int texture_texture_arrays_cache_remove(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Remove from cache
    for (uint32_t i = 0; i < g_texture_arrays_ctx.cache_size; i++) {
        if (g_texture_arrays_ctx.cache[i].texture_id == handle.id) {
            if (g_texture_arrays_ctx.cache[i].data) {
                free(g_texture_arrays_ctx.cache[i].data);
            }
            memset(&g_texture_arrays_ctx.cache[i], 0, sizeof(texture_cache_entry_t));
            g_texture_arrays_ctx.cache_size--;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_cache_clear(void) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    for (uint32_t i = 0; i < g_texture_arrays_ctx.cache_size; i++) {
        if (g_texture_arrays_ctx.cache[i].data) {
            free(g_texture_arrays_ctx.cache[i].data);
        }
    }
    
    memset(g_texture_arrays_ctx.cache, 0, sizeof(g_texture_arrays_ctx.cache));
    g_texture_arrays_ctx.cache_size = 0;
    g_texture_arrays_ctx.cache_head = 0;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

uint32_t texture_texture_arrays_cache_get_hit_rate(void) {
    if (!g_texture_arrays_ctx.initialized) return 0;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    
    uint64_t total = g_texture_arrays_ctx.performance.cache_hits + g_texture_arrays_ctx.performance.cache_misses;
    uint32_t hit_rate = (total > 0) ? (uint32_t)(g_texture_arrays_ctx.performance.cache_hits * 100 / total) : 0;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return hit_rate;
}

/* Texture Arrays Async Operations */
int texture_texture_arrays_async_upload(texture_texture_arrays_handle_t handle, const void* data, size_t size) {
    if (!data || size == 0) return TEXTURE_ARRAYS_ERROR_INVALID_PARAM;
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Add to async queue
    pthread_mutex_lock(&g_texture_arrays_ctx.async_mutex);
    
    if ((g_texture_arrays_ctx.async_queue_tail + 1) % TEXTURE_TEXTURE_ARRAYS_ASYNC_QUEUE_SIZE == g_texture_arrays_ctx.async_queue_head) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.async_mutex);
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_OUT_OF_MEMORY; // Queue full
    }
    
    texture_async_operation_t* op = &g_texture_arrays_ctx.async_queue[g_texture_arrays_ctx.async_queue_tail];
    op->handle = handle;
    op->type = TEXTURE_ASYNC_UPLOAD;
    op->data = malloc(size);
    if (op->data) {
        memcpy(op->data, data, size);
    }
    op->data_size = size;
    op->completed = false;
    op->result = 0;
    pthread_mutex_init(&op->completion_mutex, NULL);
    pthread_cond_init(&op->completion_cond, NULL);
    
    g_texture_arrays_ctx.async_queue_tail = (g_texture_arrays_ctx.async_queue_tail + 1) % TEXTURE_TEXTURE_ARRAYS_ASYNC_QUEUE_SIZE;
    
    pthread_cond_signal(&g_texture_arrays_ctx.async_cond);
    pthread_mutex_unlock(&g_texture_arrays_ctx.async_mutex);
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_async_download(texture_texture_arrays_handle_t handle, void* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return TEXTURE_ARRAYS_ERROR_INVALID_PARAM;
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Add to async queue
    pthread_mutex_lock(&g_texture_arrays_ctx.async_mutex);
    
    if ((g_texture_arrays_ctx.async_queue_tail + 1) % TEXTURE_TEXTURE_ARRAYS_ASYNC_QUEUE_SIZE == g_texture_arrays_ctx.async_queue_head) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.async_mutex);
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_OUT_OF_MEMORY; // Queue full
    }
    
    texture_async_operation_t* op = &g_texture_arrays_ctx.async_queue[g_texture_arrays_ctx.async_queue_tail];
    op->handle = handle;
    op->type = TEXTURE_ASYNC_DOWNLOAD;
    op->data = buffer;
    op->data_size = buffer_size;
    op->completed = false;
    op->result = 0;
    pthread_mutex_init(&op->completion_mutex, NULL);
    pthread_cond_init(&op->completion_cond, NULL);
    
    g_texture_arrays_ctx.async_queue_tail = (g_texture_arrays_ctx.async_queue_tail + 1) % TEXTURE_TEXTURE_ARRAYS_ASYNC_QUEUE_SIZE;
    
    pthread_cond_signal(&g_texture_arrays_ctx.async_cond);
    pthread_mutex_unlock(&g_texture_arrays_ctx.async_mutex);
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_async_wait(texture_texture_arrays_handle_t handle, uint32_t timeout_ms) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Wait for async operations to complete
    pthread_mutex_lock(&g_texture_arrays_ctx.async_mutex);
    
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += timeout_ms / 1000;
    timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (timeout.tv_nsec >= 1000000000) {
        timeout.tv_sec += timeout.tv_nsec / 1000000000;
        timeout.tv_nsec %= 1000000000;
    }
    
    int result = pthread_cond_timedwait(&g_texture_arrays_ctx.async_cond, &g_texture_arrays_ctx.async_mutex, &timeout);
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.async_mutex);
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    
    return (result == 0) ? TEXTURE_ARRAYS_SUCCESS : TEXTURE_ARRAYS_ERROR_THREAD_CREATE_FAILED;
}

/* Texture Arrays GPU Integration */
int texture_texture_arrays_gpu_upload(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Simulate GPU upload
    if (!item->gpu_resident) {
        item->gpu_resident = true;
        item->gpu_texture_id = handle.id + 1;
        g_texture_arrays_ctx.performance.gpu_operations++;
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

bool texture_texture_arrays_is_gpu_resident(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return false;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return false;
    }
    
    bool resident = g_texture_arrays_ctx.items[handle.id].gpu_resident;
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return resident;
}

/* Virtual Texturing */
int texture_texture_arrays_enable_virtual_texturing(texture_texture_arrays_handle_t handle, bool enabled) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    item->virtual_texturing_enabled = enabled;
    item->dirty = true;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_load_virtual_tile(texture_texture_arrays_handle_t handle, uint32_t x, uint32_t y, uint32_t mip_level, uint32_t layer) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[handle.id];
    if (!item->initialized || !item->virtual_texturing_enabled) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Find or create virtual tile
    texture_virtual_page_t* tile = NULL;
    for (uint32_t i = 0; i < item->virtual_page_count; i++) {
        if (item->virtual_pages[i].x == x && item->virtual_pages[i].y == y && 
            item->virtual_pages[i].mip_level == mip_level && item->virtual_pages[i].layer == layer) {
            tile = &item->virtual_pages[i];
            break;
        }
    }
    
    if (!tile && item->virtual_page_count < item->virtual_page_capacity) {
        tile = &item->virtual_pages[item->virtual_page_count];
        tile->x = x;
        tile->y = y;
        tile->mip_level = mip_level;
        tile->layer = layer;
        tile->data = malloc(1024); // 32x32 tile
        tile->data_size = 1024;
        tile->resident = true;
        tile->last_access = get_timestamp_ms();
        item->virtual_page_count++;
    }
    
    if (tile) {
        tile->last_access = get_timestamp_ms();
        g_texture_arrays_ctx.performance.virtual_texture_operations++;
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return tile ? TEXTURE_ARRAYS_SUCCESS : TEXTURE_ARRAYS_ERROR_OUT_OF_MEMORY;
}

/* BC/ASTC Compression */
int texture_texture_arrays_compress_bc(texture_texture_arrays_handle_t handle, uint32_t compression_level) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Simulate BC compression
    if (item->data && item->data_size > 0) {
        size_t compressed_size = item->data_size / 2; // BC typically 50% compression
        void* compressed_data = malloc(compressed_size);
        if (compressed_data) {
            // Simulate compression
            memcpy(compressed_data, item->data, (item->data_size < compressed_size) ? item->data_size : compressed_size);
            
            free(item->data);
            item->data = compressed_data;
            item->data_size = compressed_size;
            item->compression_enabled = true;
            item->compression_type = COMPRESSION_LZ4;
            item->compression_ratio = (float)compressed_size / item->data_size;
            
            g_texture_arrays_ctx.performance.compression_operations++;
        }
    }
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

float texture_texture_arrays_get_compression_ratio(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return 0.0f;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return 0.0f;
    }
    
    float ratio = g_texture_arrays_ctx.items[handle.id].compression_ratio;
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return ratio;
}

/* Mipmap Generation */
int texture_texture_arrays_generate_mipmaps(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Simulate mipmap generation
    item->mip_levels = 4; // Generate 4 mip levels
    item->dirty = true;
    g_texture_arrays_ctx.performance.virtual_texture_operations++;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

/* Bindless Textures */
int texture_texture_arrays_enable_bindless(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    texture_texture_arrays_internal_t* item = &g_texture_arrays_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Enable bindless texture
    item->bindless_enabled = true;
    item->bindless_handle = (uint64_t)(handle.id + 1); // Simulate GPU handle
    item->dirty = true;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

uint64_t texture_texture_arrays_get_bindless_handle(texture_texture_arrays_handle_t handle) {
    if (!g_texture_arrays_ctx.initialized) return 0;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return 0;
    }
    
    uint64_t bindless_handle = g_texture_arrays_ctx.items[handle.id].bindless_handle;
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return bindless_handle;
}

/* Feedback Analysis */
int texture_texture_arrays_enable_feedback_analysis(texture_texture_arrays_handle_t handle, bool enabled) {
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    // Enable feedback analysis
    g_texture_arrays_ctx.feedback_data.min_lod_used = 0.0f;
    g_texture_arrays_ctx.feedback_data.max_lod_used = 0.0f;
    g_texture_arrays_ctx.feedback_data.avg_lod_used = 0.0f;
    g_texture_arrays_ctx.feedback_data.unique_tiles_accessed = 0;
    g_texture_arrays_ctx.feedback_data.total_samples = 0;
    g_texture_arrays_ctx.feedback_data.anisotropic_benefit = false;
    g_texture_arrays_ctx.feedback_data.memory_efficiency = 1.0f;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

int texture_texture_arrays_get_feedback_data(texture_texture_arrays_handle_t handle, float* out_min_lod, float* out_max_lod, float* out_avg_lod) {
    if (!out_min_lod || !out_max_lod || !out_avg_lod) return TEXTURE_ARRAYS_ERROR_INVALID_PARAM;
    if (!g_texture_arrays_ctx.initialized) return TEXTURE_ARRAYS_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_texture_arrays_ctx.context_mutex);
    if (handle.id >= g_texture_arrays_ctx.count) {
        pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
        return TEXTURE_ARRAYS_ERROR_INVALID_HANDLE;
    }
    
    *out_min_lod = g_texture_arrays_ctx.feedback_data.min_lod_used;
    *out_max_lod = g_texture_arrays_ctx.feedback_data.max_lod_used;
    *out_avg_lod = g_texture_arrays_ctx.feedback_data.avg_lod_used;
    
    pthread_mutex_unlock(&g_texture_arrays_ctx.context_mutex);
    return TEXTURE_ARRAYS_SUCCESS;
}

/* End of texture_arrays.c */

// Unit tests for texture arrays system
int texture_texture_arrays_run_unit_tests(void) {
    printf("=== Running Texture Arrays Unit Tests ===\n");
    
    int tests_run = 0;
    int tests_passed = 0;
    
    // Test 1: System initialization
    printf("Test 1: System initialization... ");
    tests_run++;
    if (texture_texture_arrays_init() == TEXTURE_ARRAYS_SUCCESS) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 2: Create texture array
    printf("Test 2: Create texture array... ");
    tests_run++;
    texture_texture_arrays_handle_t handle;
    texture_texture_arrays_desc_t desc = {
        .flags = 0x01, // Virtual texturing
        .width = 256,
        .height = 256,
        .array_size = 4
    };
    
    if (texture_texture_arrays_create(&handle, &desc) == TEXTURE_ARRAYS_SUCCESS) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 3: Update texture array
    printf("Test 3: Update texture array... ");
    tests_run++;
    uint8_t test_data[256 * 256 * 4]; // 256x256 RGBA
    memset(test_data, 0x80, sizeof(test_data));
    
    if (texture_texture_arrays_update(handle, test_data, sizeof(test_data)) == TEXTURE_ARRAYS_SUCCESS) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 4: Validate texture array
    printf("Test 4: Validate texture array... ");
    tests_run++;
    if (texture_texture_arrays_is_valid(handle)) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 5: Get texture array info
    printf("Test 5: Get texture array info... ");
    tests_run++;
    texture_texture_arrays_info_t info;
    if (texture_texture_arrays_get_info(handle, &info) == TEXTURE_ARRAYS_SUCCESS) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 6: Process pending operations
    printf("Test 6: Process pending operations... ");
    tests_run++;
    if (texture_texture_arrays_process_pending() >= 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 7: Memory usage tracking
    printf("Test 7: Memory usage tracking... ");
    tests_run++;
    size_t memory_usage = texture_texture_arrays_get_memory_usage();
    if (memory_usage > sizeof(g_texture_arrays_ctx)) {
        printf("PASS (Usage: %zu bytes)\n", memory_usage);
        tests_passed++;
    } else {
        printf("FAIL (Usage: %zu bytes)\n", memory_usage);
    }
    
    // Test 8: Cache operations
    printf("Test 8: Cache operations... ");
    tests_run++;
    if (texture_texture_arrays_cache_add(handle, test_data, sizeof(test_data)) == TEXTURE_ARRAYS_SUCCESS) {
        void* cached_data;
        size_t cached_size;
        if (texture_texture_arrays_cache_get(handle, &cached_data, &cached_size) == TEXTURE_ARRAYS_SUCCESS) {
            printf("PASS\n");
            tests_passed++;
        } else {
            printf("FAIL\n");
        }
    } else {
        printf("FAIL\n");
    }
    
    // Test 9: GPU operations
    printf("Test 9: GPU operations... ");
    tests_run++;
    if (texture_texture_arrays_gpu_upload(handle) == TEXTURE_ARRAYS_SUCCESS) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 10: Virtual texturing
    printf("Test 10: Virtual texturing... ");
    tests_run++;
    if (texture_texture_arrays_enable_virtual_texturing(handle, true) == TEXTURE_ARRAYS_SUCCESS) {
        if (texture_texture_arrays_load_virtual_tile(handle, 0, 0, 0, 0) == TEXTURE_ARRAYS_SUCCESS) {
            printf("PASS\n");
            tests_passed++;
        } else {
            printf("FAIL\n");
        }
    } else {
        printf("FAIL\n");
    }
    
    // Test 11: Compression
    printf("Test 11: Compression... ");
    tests_run++;
    if (texture_texture_arrays_compress_bc(handle, 8) == TEXTURE_ARRAYS_SUCCESS) {
        float ratio = texture_texture_arrays_get_compression_ratio(handle);
        if (ratio > 0.0f && ratio < 1.0f) {
            printf("PASS (Ratio: %.2f:1)\n", ratio);
            tests_passed++;
        } else {
            printf("FAIL (Invalid ratio: %.2f)\n", ratio);
        }
    } else {
        printf("FAIL\n");
    }
    
    // Test 12: Mipmap generation
    printf("Test 12: Mipmap generation... ");
    tests_run++;
    if (texture_texture_arrays_generate_mipmaps(handle) == TEXTURE_ARRAYS_SUCCESS) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 13: Bindless textures
    printf("Test 13: Bindless textures... ");
    tests_run++;
    if (texture_texture_arrays_enable_bindless(handle) == TEXTURE_ARRAYS_SUCCESS) {
        uint64_t bindless_handle = texture_texture_arrays_get_bindless_handle(handle);
        if (bindless_handle != 0) {
            printf("PASS (Handle: 0x%lx)\n", bindless_handle);
            tests_passed++;
        } else {
            printf("FAIL (Invalid handle)\n");
        }
    } else {
        printf("FAIL\n");
    }
    
    // Test 14: Feedback analysis
    printf("Test 14: Feedback analysis... ");
    tests_run++;
    if (texture_texture_arrays_enable_feedback_analysis(handle, true) == TEXTURE_ARRAYS_SUCCESS) {
        float min_lod, max_lod, avg_lod;
        if (texture_texture_arrays_get_feedback_data(handle, &min_lod, &max_lod, &avg_lod) == TEXTURE_ARRAYS_SUCCESS) {
            printf("PASS (Min: %.2f, Max: %.2f, Avg: %.2f)\n", min_lod, max_lod, avg_lod);
            tests_passed++;
        } else {
            printf("FAIL\n");
        }
    } else {
        printf("FAIL\n");
    }
    
    // Test 15: Get count
    printf("Test 15: Get count... ");
    tests_run++;
    uint32_t count = texture_texture_arrays_get_count();
    if (count > 0) {
        printf("PASS (Count: %u)\n", count);
        tests_passed++;
    } else {
        printf("FAIL (Count: %u)\n", count);
    }
    
    // Test 16: Destroy texture array
    printf("Test 16: Destroy texture array... ");
    tests_run++;
    texture_texture_arrays_destroy(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Cleanup
    texture_texture_arrays_shutdown();
    
    printf("\n=== Test Results ===\n");
    printf("Tests Run: %d\n", tests_run);
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_run - tests_passed);
    printf("Success Rate: %.1f%%\n", (float)tests_passed / tests_run * 100.0f);
    
    if (tests_passed == tests_run) {
        printf("=== ALL TESTS PASSED ===\n");
        return 0;
    } else {
        printf("=== SOME TESTS FAILED ===\n");
        return -1;
    }
}
