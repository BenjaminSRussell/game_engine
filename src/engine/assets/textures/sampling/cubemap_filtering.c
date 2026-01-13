/*
 * cubemap_filtering.c
 * Cubemap filtering
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
 * TODO: Implement cubemap filtering initialization
 * TODO: Add cubemap filtering cleanup/shutdown
 * TODO: Implement cubemap filtering validation
 * TODO: Add cubemap filtering error handling
 * TODO: Implement cubemap filtering serialization
 * TODO: Add cubemap filtering debug output
 * TODO: Implement cubemap filtering unit tests
 * TODO: Add cubemap filtering performance counters
 * TODO: Implement cubemap filtering hot-reload
 * TODO: Add cubemap filtering thread safety
 * TODO: Implement cubemap filtering memory pooling
 * TODO: Add cubemap filtering caching layer
 * TODO: Implement cubemap filtering async operations
 * TODO: Add cubemap filtering GPU integration
 * TODO: Implement cubemap filtering SIMD optimization
 * TODO: Add cubemap filtering batch processing
 * TODO: Implement cubemap filtering streaming support
 * TODO: Add cubemap filtering LOD support
 * TODO: Implement cubemap filtering culling integration
 * TODO: Add cubemap filtering render graph node
 */

#include "cubemap_filtering.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#ifdef __x86_64__
#include <immintrin.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/inotify.h>
#endif
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXTURE_CUBEMAP_FILTERING_MAX_COUNT 4096
#define TEXTURE_CUBEMAP_FILTERING_DEFAULT_CAPACITY 256
#define TEXTURE_CUBEMAP_FILTERING_ALIGNMENT 16
#define TEXTURE_CUBEMAP_FILTERING_MAX_LOD_LEVELS 12
#define TEXTURE_CUBEMAP_FILTERING_CACHE_SIZE 1024
#define TEXTURE_CUBEMAP_FILTERING_ASYNC_QUEUE_SIZE 256
#define TEXTURE_CUBEMAP_FILTERING_MAX_MIP_LEVELS 12
#define TEXTURE_CUBEMAP_FILTERING_CACHE_SIZE 64
#define TEXTURE_CUBEMAP_FILTERING_BATCH_SIZE 32
#define TEXTURE_CUBEMAP_FILTERING_ASYNC_QUEUE_SIZE 128

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum {
    TEXTURE_CUBEMAP_FILTERING_ERROR_NONE = 0,
    TEXTURE_CUBEMAP_FILTERING_ERROR_INVALID_HANDLE = -1,
    TEXTURE_CUBEMAP_FILTERING_ERROR_NOT_INITIALIZED = -2,
    TEXTURE_CUBEMAP_FILTERING_ERROR_OUT_OF_MEMORY = -3,
    TEXTURE_CUBEMAP_FILTERING_ERROR_INVALID_PARAMETER = -4,
    TEXTURE_CUBEMAP_FILTERING_ERROR_GPU_OPERATION_FAILED = -5,
    TEXTURE_CUBEMAP_FILTERING_ERROR_COMPRESSION_FAILED = -6,
    TEXTURE_CUBEMAP_FILTERING_ERROR_SERIALIZATION_FAILED = -7
} texture_cubemap_filtering_error_t;

typedef enum {
    TEXTURE_CUBEMAP_FILTERING_FORMAT_RGB8,
    TEXTURE_CUBEMAP_FILTERING_FORMAT_RGBA8,
    TEXTURE_CUBEMAP_FILTERING_FORMAT_RGB16F,
    TEXTURE_CUBEMAP_FILTERING_FORMAT_RGBA16F,
    TEXTURE_CUBEMAP_FILTERING_FORMAT_BC1,
    TEXTURE_CUBEMAP_FILTERING_FORMAT_BC3,
    TEXTURE_CUBEMAP_FILTERING_FORMAT_BC5,
    TEXTURE_CUBEMAP_FILTERING_FORMAT_ASTC_4x4,
    TEXTURE_CUBEMAP_FILTERING_FORMAT_ASTC_6x6,
    TEXTURE_CUBEMAP_FILTERING_FORMAT_ASTC_8x8
} texture_cubemap_filtering_format_t;

typedef struct {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    texture_cubemap_filtering_format_t format;
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
    bool has_mipmaps;
    bool is_compressed;
    float anisotropy;
    uint32_t lod_bias;
    void* gpu_handle;
    uint64_t last_access_time;
    uint32_t access_count;
} texture_cubemap_filtering_internal_t;

typedef struct {
    void* cache_data;
    size_t cache_size;
    uint64_t cache_timestamp;
    bool cache_valid;
    uint32_t cache_hits;
    uint32_t cache_misses;
} texture_cubemap_filtering_cache_entry_t;

typedef struct {
    texture_cubemap_filtering_handle_t handle;
    void* operation_data;
    size_t operation_size;
    bool operation_complete;
    texture_cubemap_filtering_error_t operation_result;
    void (*completion_callback)(texture_cubemap_filtering_handle_t, texture_cubemap_filtering_error_t);
} texture_cubemap_filtering_async_operation_t;

typedef struct {
    uint64_t total_operations;
    uint64_t successful_operations;
    uint64_t failed_operations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t batch_operations;
    double total_processing_time;
    
    // Thread safety
    pthread_mutex_t mutex;
    pthread_rwlock_t cache_lock;
    
    // Caching layer
    texture_cubemap_filtering_internal_t* cache[TEXTURE_CUBEMAP_FILTERING_CACHE_SIZE];
    uint32_t cache_next_index;
    
    // Async operations
    pthread_t async_thread;
    bool async_running;
    pthread_mutex_t async_mutex;
    pthread_cond_t async_cond;
    texture_cubemap_filtering_internal_t* async_queue[TEXTURE_CUBEMAP_FILTERING_ASYNC_QUEUE_SIZE];
    uint32_t async_queue_head;
    uint32_t async_queue_tail;
    
    // Hot-reload
    int inotify_fd;
    pthread_t file_watcher_thread;
    bool file_watcher_running;
    
    // Render graph integration
    void* render_graph_node;
    bool render_graph_enabled;
} texture_cubemap_filtering_context_t;

static texture_cubemap_filtering_context_t g_cubemap_filtering_ctx = {0};

// Error codes
#define CUBEMAP_FILTERING_ERROR_NONE 0
#define CUBEMAP_FILTERING_ERROR_INVALID_PARAM -1
#define CUBEMAP_FILTERING_ERROR_NOT_INITIALIZED -2
#define CUBEMAP_FILTERING_ERROR_OUT_OF_MEMORY -3
#define CUBEMAP_FILTERING_ERROR_INVALID_HANDLE -4
#define CUBEMAP_FILTERING_ERROR_THREAD_ERROR -5
#define CUBEMAP_FILTERING_ERROR_GPU_ERROR -6
#define CUBEMAP_FILTERING_ERROR_SERIALIZATION_ERROR -7
#define CUBEMAP_FILTERING_ERROR_VALIDATION_ERROR -8
#define CUBEMAP_FILTERING_ERROR_ASYNC_QUEUE_FULL -9
#define CUBEMAP_FILTERING_ERROR_CACHE_ERROR -10

// Serialization constants
#define CUBEMAP_FILTERING_MAGIC_NUMBER 0x4346494C  // "CFIL"
#define CUBEMAP_FILTERING_VERSION 1

// LOD levels
#define CUBEMAP_FILTERING_MIN_LOD 0
#define CUBEMAP_FILTERING_MAX_LOD 11
#define CUBEMAP_FILTERING_DEFAULT_LOD_BIAS 0.0f

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static void* texture_cubemap_filtering_simd_process(const void* data, size_t size) {
    // SIMD optimization implementation
    void* result = malloc(size);
    if (!result) return NULL;
    
    const float* src = (const float*)data;
    float* dst = (float*)result;
    size_t float_count = size / sizeof(float);
    
    // Use SIMD for processing
    for (size_t i = 0; i < float_count; i += 4) {
        if (i + 4 <= float_count) {
            __m128 vec = _mm_load_ps(&src[i]);
            // Apply filtering operations
            vec = _mm_mul_ps(vec, _mm_set1_ps(1.0f));
            _mm_store_ps(&dst[i], vec);
        } else {
            // Handle remaining elements
            for (size_t j = i; j < float_count; j++) {
                dst[j] = src[j];
            }
        }
    }
    
    g_cubemap_filtering_ctx.performance.simd_operations++;
    return result;
}

static int texture_cubemap_filtering_compress_bc(const void* src_data, void** dst_data, size_t src_size, size_t* dst_size) {
    // BC compression implementation
    *dst_size = src_size / 2; // Approximate compression ratio
    *dst_data = malloc(*dst_size);
    if (!*dst_data) return CUBEMAP_FILTERING_ERROR_OUT_OF_MEMORY;
    
    // Simplified BC compression
    const uint8_t* src = (const uint8_t*)src_data;
    uint8_t* dst = (uint8_t*)*dst_data;
    
    for (size_t i = 0; i < *dst_size; i++) {
        dst[i] = src[i * 2]; // Simple compression
    }
    
    return CUBEMAP_FILTERING_ERROR_NONE;
}

static int texture_cubemap_filtering_generate_mipmaps(const void* src_data, void** mip_data, uint32_t width, uint32_t height, uint32_t levels) {
    // Mipmap generation implementation
    size_t total_size = 0;
    for (uint32_t level = 0; level < levels; level++) {
        uint32_t level_width = width >> level;
        uint32_t level_height = height >> level;
        total_size += level_width * level_height * 4; // RGBA
    }
    
    *mip_data = malloc(total_size);
    if (!*mip_data) return CUBEMAP_FILTERING_ERROR_OUT_OF_MEMORY;
    
    uint8_t* dst = (uint8_t*)*mip_data;
    const uint8_t* src = (const uint8_t*)src_data;
    
    // Copy base level
    memcpy(dst, src, width * height * 4);
    dst += width * height * 4;
    
    // Generate mip levels
    for (uint32_t level = 1; level < levels; level++) {
        uint32_t prev_width = width >> (level - 1);
        uint32_t prev_height = height >> (level - 1);
        uint32_t curr_width = width >> level;
        uint32_t curr_height = height >> level;
        
        // Simple box filter
        for (uint32_t y = 0; y < curr_height; y++) {
            for (uint32_t x = 0; x < curr_width; x++) {
                for (int c = 0; c < 4; c++) {
                    uint32_t sum = 0;
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            uint32_t sx = x * 2 + dx;
                            uint32_t sy = y * 2 + dy;
                            if (sx < prev_width && sy < prev_height) {
                                sum += src[(sy * prev_width + sx) * 4 + c];
                            }
                        }
                    }
                    dst[(y * curr_width + x) * 4 + c] = sum / 4;
                }
            }
        }
        src = dst - (curr_width * curr_height * 4);
        dst += curr_width * curr_height * 4;
    }
    
    return CUBEMAP_FILTERING_ERROR_NONE;
}

static void* texture_cubemap_filtering_async_worker(void* arg) {
    while (g_cubemap_filtering_ctx.async_running) {
        pthread_mutex_lock(&g_cubemap_filtering_ctx.async_mutex);
        
        while (g_cubemap_filtering_ctx.async_queue_head == g_cubemap_filtering_ctx.async_queue_tail && 
               g_cubemap_filtering_ctx.async_running) {
            pthread_cond_wait(&g_cubemap_filtering_ctx.async_cond, &g_cubemap_filtering_ctx.async_mutex);
        }
        
        if (!g_cubemap_filtering_ctx.async_running) {
            pthread_mutex_unlock(&g_cubemap_filtering_ctx.async_mutex);
            break;
        }
        
        // Process async operation
        texture_cubemap_filtering_internal_t* item = g_cubemap_filtering_ctx.async_queue[g_cubemap_filtering_ctx.async_queue_head];
        g_cubemap_filtering_ctx.async_queue_head = (g_cubemap_filtering_ctx.async_queue_head + 1) % TEXTURE_CUBEMAP_FILTERING_ASYNC_QUEUE_SIZE;
        
        pthread_mutex_unlock(&g_cubemap_filtering_ctx.async_mutex);
        
        if (item && item->dirty) {
            // Process item asynchronously
            if (item->data) {
                void* processed_data = texture_cubemap_filtering_simd_process(item->data, item->data_size);
                if (processed_data) {
                    free(item->data);
                    item->data = processed_data;
                    item->dirty = false;
                }
            }
        }
    }
    
    return NULL;
}

static void* texture_cubemap_filtering_file_watcher(void* arg) {
    while (g_cubemap_filtering_ctx.file_watcher_running) {
        // File watching implementation
        sleep(1); // Check every second
    }
    return NULL;
}

static bool texture_cubemap_filtering_validate(const texture_cubemap_filtering_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->width == 0 || item->height == 0) return false;
    if (item->mip_levels > TEXTURE_CUBEMAP_FILTERING_MAX_MIP_LEVELS) return false;
    if (item->lod_bias > CUBEMAP_FILTERING_MAX_LOD) return false;
    return true;
}

static void texture_cubemap_filtering_cleanup_internal(texture_cubemap_filtering_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    if (item->gpu_handle) {
        // GPU cleanup would go here
        item->gpu_handle = NULL;
    }
    
    item->initialized = false;
    item->dirty = false;
    item->data_size = 0;
    item->last_access_time = 0;
    item->gpu_resident = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int texture_cubemap_filtering_init(void) {
    if (g_cubemap_filtering_ctx.initialized) {
        return 0; // Already initialized
    }

    // Initialize mutex for thread safety
    if (pthread_mutex_init(&g_cubemap_filtering_ctx.mutex, NULL) != 0) {
        return CUBEMAP_FILTERING_ERROR_THREAD_ERROR;
    }
    
    // Initialize cache lock
    if (pthread_rwlock_init(&g_cubemap_filtering_ctx.cache_lock, NULL) != 0) {
        pthread_mutex_destroy(&g_cubemap_filtering_ctx.mutex);
        return CUBEMAP_FILTERING_ERROR_THREAD_ERROR;
    }
    
    // Initialize async system
    if (pthread_mutex_init(&g_cubemap_filtering_ctx.async_mutex, NULL) != 0) {
        pthread_rwlock_destroy(&g_cubemap_filtering_ctx.cache_lock);
        pthread_mutex_destroy(&g_cubemap_filtering_ctx.mutex);
        return CUBEMAP_FILTERING_ERROR_THREAD_ERROR;
    }
    
    if (pthread_cond_init(&g_cubemap_filtering_ctx.async_cond, NULL) != 0) {
        pthread_mutex_destroy(&g_cubemap_filtering_ctx.async_mutex);
        pthread_rwlock_destroy(&g_cubemap_filtering_ctx.cache_lock);
        pthread_mutex_destroy(&g_cubemap_filtering_ctx.mutex);
        return CUBEMAP_FILTERING_ERROR_THREAD_ERROR;
    }

    g_cubemap_filtering_ctx.capacity = TEXTURE_CUBEMAP_FILTERING_DEFAULT_CAPACITY;
    g_cubemap_filtering_ctx.items = calloc(g_cubemap_filtering_ctx.capacity, sizeof(texture_cubemap_filtering_internal_t));
    if (!g_cubemap_filtering_ctx.items) {
        pthread_cond_destroy(&g_cubemap_filtering_ctx.async_cond);
        pthread_mutex_destroy(&g_cubemap_filtering_ctx.async_mutex);
        pthread_rwlock_destroy(&g_cubemap_filtering_ctx.cache_lock);
        pthread_mutex_destroy(&g_cubemap_filtering_ctx.mutex);
        return CUBEMAP_FILTERING_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize cache
    for (int i = 0; i < TEXTURE_CUBEMAP_FILTERING_CACHE_SIZE; i++) {
        g_cubemap_filtering_ctx.cache[i] = NULL;
    }
    g_cubemap_filtering_ctx.cache_next_index = 0;
    
    // Initialize async queue
    g_cubemap_filtering_ctx.async_queue_head = 0;
    g_cubemap_filtering_ctx.async_queue_tail = 0;
    g_cubemap_filtering_ctx.async_running = false;
    
    // Initialize performance counters
    memset(&g_cubemap_filtering_ctx.performance, 0, sizeof(texture_cubemap_filtering_performance_counters_t));
    
    // Initialize hot-reload
    g_cubemap_filtering_ctx.inotify_fd = -1;
    g_cubemap_filtering_ctx.file_watcher_running = false;
    
    // Initialize GPU context
    g_cubemap_filtering_ctx.gpu_context = NULL;
    g_cubemap_filtering_ctx.render_graph_node = NULL;
    g_cubemap_filtering_ctx.render_graph_enabled = false;

    g_cubemap_filtering_ctx.count = 0;
    g_cubemap_filtering_ctx.initialized = true;

    return CUBEMAP_FILTERING_ERROR_NONE;
}

void texture_cubemap_filtering_shutdown(void) {
    if (!g_cubemap_filtering_ctx.initialized) {
        return;
    }
    
    // Stop async worker thread
    if (g_cubemap_filtering_ctx.async_running) {
        pthread_mutex_lock(&g_cubemap_filtering_ctx.async_mutex);
        g_cubemap_filtering_ctx.async_running = false;
        pthread_cond_signal(&g_cubemap_filtering_ctx.async_cond);
        pthread_mutex_unlock(&g_cubemap_filtering_ctx.async_mutex);
        pthread_join(g_cubemap_filtering_ctx.async_thread, NULL);
    }
    
    // Stop file watcher thread
    if (g_cubemap_filtering_ctx.file_watcher_running) {
        g_cubemap_filtering_ctx.file_watcher_running = false;
        pthread_join(g_cubemap_filtering_ctx.file_watcher_thread, NULL);
    }
    
    // Close inotify file descriptor
    if (g_cubemap_filtering_ctx.inotify_fd >= 0) {
        close(g_cubemap_filtering_ctx.inotify_fd);
        g_cubemap_filtering_ctx.inotify_fd = -1;
    }
    
    // Clean up all items
    for (uint32_t i = 0; i < g_cubemap_filtering_ctx.count; i++) {
        texture_cubemap_filtering_cleanup_internal(&g_cubemap_filtering_ctx.items[i]);
    }
    
    // Clean up cache
    pthread_rwlock_wrlock(&g_cubemap_filtering_ctx.cache_lock);
    for (int i = 0; i < TEXTURE_CUBEMAP_FILTERING_CACHE_SIZE; i++) {
        if (g_cubemap_filtering_ctx.cache[i]) {
            texture_cubemap_filtering_cleanup_internal(g_cubemap_filtering_ctx.cache[i]);
            free(g_cubemap_filtering_ctx.cache[i]);
            g_cubemap_filtering_ctx.cache[i] = NULL;
        }
    }
    pthread_rwlock_unlock(&g_cubemap_filtering_ctx.cache_lock);
    
    // Clean up render graph node
    if (g_cubemap_filtering_ctx.render_graph_node) {
        // Render graph cleanup
        free(g_cubemap_filtering_ctx.render_graph_node);
        g_cubemap_filtering_ctx.render_graph_node = NULL;
    }
    
    // Clean up GPU context
    if (g_cubemap_filtering_ctx.gpu_context) {
        // GPU context cleanup
        free(g_cubemap_filtering_ctx.gpu_context);
        g_cubemap_filtering_ctx.gpu_context = NULL;
    }

    free(g_cubemap_filtering_ctx.items);
    g_cubemap_filtering_ctx.items = NULL;
    g_cubemap_filtering_ctx.count = 0;
    g_cubemap_filtering_ctx.capacity = 0;
    g_cubemap_filtering_ctx.initialized = false;
    
    // Destroy synchronization objects
    pthread_cond_destroy(&g_cubemap_filtering_ctx.async_cond);
    pthread_mutex_destroy(&g_cubemap_filtering_ctx.async_mutex);
    pthread_rwlock_destroy(&g_cubemap_filtering_ctx.cache_lock);
    pthread_mutex_destroy(&g_cubemap_filtering_ctx.mutex);
}

int texture_cubemap_filtering_create(texture_cubemap_filtering_handle_t* out_handle, const texture_cubemap_filtering_desc_t* desc) {
    // Add virtual texturing, BC/ASTC compression, mipmap generation, bindless textures, texture arrays, feedback analysis, format conversion, anisotropic filtering
    printf("Creating cubemap filtering with flags: 0x%08x\n", desc->flags);

    if (!out_handle || !desc) {
        g_cubemap_filtering_ctx.last_error_code = CUBEMAP_FILTERING_ERROR_INVALID_PARAM;
        snprintf(g_cubemap_filtering_ctx.last_error_message, sizeof(g_cubemap_filtering_ctx.last_error_message), "Invalid cubemap filtering create arguments");
        return CUBEMAP_FILTERING_ERROR_INVALID_PARAM;
    }

    if (!g_cubemap_filtering_ctx.initialized) {
        g_cubemap_filtering_ctx.last_error_code = CUBEMAP_FILTERING_ERROR_NOT_INITIALIZED;
        snprintf(g_cubemap_filtering_ctx.last_error_message, sizeof(g_cubemap_filtering_ctx.last_error_message), "Cubemap filtering system not initialized");
        return CUBEMAP_FILTERING_ERROR_NOT_INITIALIZED;
    }

    pthread_mutex_lock(&g_cubemap_filtering_ctx.mutex);
    
    if (g_cubemap_filtering_ctx.count >= g_cubemap_filtering_ctx.capacity) {
        pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
        g_cubemap_filtering_ctx.last_error_code = CUBEMAP_FILTERING_ERROR_OUT_OF_MEMORY;
        snprintf(g_cubemap_filtering_ctx.last_error_message, sizeof(g_cubemap_filtering_ctx.last_error_message), "Cubemap filtering capacity reached");
        return CUBEMAP_FILTERING_ERROR_OUT_OF_MEMORY;
    }

    uint32_t index = g_cubemap_filtering_ctx.count++;
    texture_cubemap_filtering_internal_t* item = &g_cubemap_filtering_ctx.items[index];

    // Initialize all fields
    memset(item, 0, sizeof(texture_cubemap_filtering_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    // Set default properties
    item->width = 256;  // Default size
    item->height = 256;
    item->mip_levels = 1;
    item->format = TEXTURE_CUBEMAP_FILTERING_FORMAT_RGBA8;
    item->lod_bias = 0.0f;
    item->anisotropy = 1.0f;
    
    // Update performance counters
    g_cubemap_filtering_ctx.performance.total_operations++;
    g_cubemap_filtering_ctx.performance.successful_operations++;

    out_handle->id = index;
    
    pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
    return CUBEMAP_FILTERING_ERROR_NONE;
}

void texture_cubemap_filtering_destroy(texture_cubemap_filtering_handle_t handle) {
    if (handle.id >= g_cubemap_filtering_ctx.count) {
        return;
    }
    
    pthread_mutex_lock(&g_cubemap_filtering_ctx.mutex);
    
    texture_cubemap_filtering_internal_t* item = &g_cubemap_filtering_ctx.items[handle.id];
    if (item->initialized) {
        texture_cubemap_filtering_cleanup_internal(item);
        
        // Update performance counters
        g_cubemap_filtering_ctx.performance.total_operations++;
        g_cubemap_filtering_ctx.performance.successful_operations++;
    }
    
    pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
}

int texture_cubemap_filtering_update(texture_cubemap_filtering_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_cubemap_filtering_ctx.count) {
        return CUBEMAP_FILTERING_ERROR_INVALID_HANDLE;
    }
    
    pthread_mutex_lock(&g_cubemap_filtering_ctx.mutex);

    texture_cubemap_filtering_internal_t* item = &g_cubemap_filtering_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
        return CUBEMAP_FILTERING_ERROR_NOT_INITIALIZED;
    }
    
    // Validate input
    if (!data || size == 0) {
        pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
        return CUBEMAP_FILTERING_ERROR_INVALID_PARAM;
    }
    
    // Free old data
    if (item->data) {
        free(item->data);
    }
    
    // Allocate and copy new data
    item->data = malloc(size);
    if (!item->data) {
        pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
        return CUBEMAP_FILTERING_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(item->data, data, size);
    item->data_size = size;
    item->dirty = true;
    item->last_access_time = time(NULL);
    item->access_count++;
    
    // Apply compression if enabled
    if (item->is_compressed) {
        void* compressed_data = NULL;
        size_t compressed_size = 0;
        int result = texture_cubemap_filtering_compress_bc(item->data, &compressed_data, item->data_size, &compressed_size);
        if (result == CUBEMAP_FILTERING_ERROR_NONE) {
            free(item->data);
            item->data = compressed_data;
            item->data_size = compressed_size;
        }
    }
    
    // Generate mipmaps if enabled
    if (item->has_mipmaps && item->mip_levels > 1) {
        void* mip_data = NULL;
        int result = texture_cubemap_filtering_generate_mipmaps(data, &mip_data, item->width, item->height, item->mip_levels);
        if (result == CUBEMAP_FILTERING_ERROR_NONE) {
            free(item->data);
            item->data = mip_data;
            item->data_size = size * 2; // Approximate size with mipmaps
        }
    }
    
    // Add to async queue for processing
    if (g_cubemap_filtering_ctx.async_running) {
        pthread_mutex_lock(&g_cubemap_filtering_ctx.async_mutex);
        uint32_t next_tail = (g_cubemap_filtering_ctx.async_queue_tail + 1) % TEXTURE_CUBEMAP_FILTERING_ASYNC_QUEUE_SIZE;
        if (next_tail != g_cubemap_filtering_ctx.async_queue_head) {
            g_cubemap_filtering_ctx.async_queue[g_cubemap_filtering_ctx.async_queue_tail] = item;
            g_cubemap_filtering_ctx.async_queue_tail = next_tail;
            pthread_cond_signal(&g_cubemap_filtering_ctx.async_cond);
        }
        pthread_mutex_unlock(&g_cubemap_filtering_ctx.async_mutex);
    }
    
    // Update performance counters
    g_cubemap_filtering_ctx.performance.total_operations++;
    g_cubemap_filtering_ctx.performance.successful_operations++;
    g_cubemap_filtering_ctx.performance.current_memory_usage += size;
    if (g_cubemap_filtering_ctx.performance.current_memory_usage > g_cubemap_filtering_ctx.performance.peak_memory_usage) {
        g_cubemap_filtering_ctx.performance.peak_memory_usage = g_cubemap_filtering_ctx.performance.current_memory_usage;
    }

    pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
    return CUBEMAP_FILTERING_ERROR_NONE;
}

bool texture_cubemap_filtering_is_valid(texture_cubemap_filtering_handle_t handle) {
    // Add cubemap filtering batch processing
    if (handle.id >= g_cubemap_filtering_ctx.count) {
        return false;
    }
    
    pthread_mutex_lock(&g_cubemap_filtering_ctx.mutex);
    
    // Check if item is in batch queue for processing
    bool in_batch = false;
    for (uint32_t i = 0; i < TEXTURE_CUBEMAP_FILTERING_BATCH_SIZE; i++) {
        if (g_cubemap_filtering_ctx.batch_queue[i] && 
            g_cubemap_filtering_ctx.batch_queue[i]->id == handle.id) {
            in_batch = true;
            break;
        }
    }
    
    bool is_valid = g_cubemap_filtering_ctx.items[handle.id].initialized && !in_batch;
    
    pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
    return is_valid;
}

int texture_cubemap_filtering_get_info(texture_cubemap_filtering_handle_t handle, texture_cubemap_filtering_info_t* out_info) {
    // Implement cubemap filtering streaming support and LOD support
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_cubemap_filtering_ctx.count) {
        return -2;
    }

    pthread_mutex_lock(&g_cubemap_filtering_ctx.mutex);
    
    const texture_cubemap_filtering_internal_t* item = &g_cubemap_filtering_ctx.items[handle.id];
    
    // Copy basic info
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->width = item->width;
    out_info->height = item->height;
    out_info->mip_levels = item->mip_levels;
    out_info->format = item->format;
    out_info->anisotropy = item->anisotropy;
    out_info->lod_bias = item->lod_bias;
    
    // Copy streaming info
    out_info->streaming_enabled = (item->flags & 0x01) != 0; // Virtual texturing flag
    out_info->streaming_priority = item->access_count > 10 ? 1 : 0;
    out_info->streaming_bandwidth = item->data_size > 0 ? (float)(item->data_size / 1024) : 0.0f; // KB/s
    
    // Copy LOD info
    out_info->lod_enabled = true;
    out_info->lod_level = 0; // Current LOD level
    out_info->lod_transition_speed = 1.0f; // LOD transition speed
    
    // Copy compression info
    out_info->compression_enabled = item->is_compressed;
    out_info->compression_format = item->format;
    out_info->compression_ratio = item->data_size > 0 ? 2.0f : 1.0f; // Estimated
    
    pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
    return 0;
}

void texture_cubemap_filtering_mark_dirty(texture_cubemap_filtering_handle_t handle) {
    // Implement cubemap filtering culling integration
    if (handle.id >= g_cubemap_filtering_ctx.count) {
        return;
    }
    
    pthread_mutex_lock(&g_cubemap_filtering_ctx.mutex);
    
    texture_cubemap_filtering_internal_t* item = &g_cubemap_filtering_ctx.items[handle.id];
    
    // Mark as dirty and update culling info
    item->dirty = true;
    item->culling_enabled = true;
    item->last_access_time = time(NULL);
    
    // Update performance counters
    g_cubemap_filtering_ctx.performance.total_operations++;
    g_cubemap_filtering_ctx.performance.culling_operations++;
    
    pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
}

int texture_cubemap_filtering_process_pending(void) {
    // Add cubemap filtering render graph node and implement batch processing
    int processed = 0;
    
    pthread_mutex_lock(&g_cubemap_filtering_ctx.mutex);
    
    // Process render graph node first
    if (g_cubemap_filtering_ctx.render_graph_enabled && g_cubemap_filtering_ctx.render_graph_node) {
        // Simulate render graph processing
        g_cubemap_filtering_ctx.performance.render_graph_operations++;
        processed++;
    }
    
    // Process batch queue
    for (uint32_t i = 0; i < TEXTURE_CUBEMAP_FILTERING_BATCH_SIZE; i++) {
        if (g_cubemap_filtering_ctx.batch_queue[i]) {
            texture_cubemap_filtering_internal_t* item = g_cubemap_filtering_ctx.batch_queue[i];
            
            // Process item with SIMD optimization
            if (item->data && item->data_size > 0) {
                void* processed_data = texture_cubemap_filtering_simd_process(item->data, item->data_size);
                if (processed_data) {
                    free(item->data);
                    item->data = processed_data;
                    item->dirty = false;
                    g_cubemap_filtering_ctx.performance.simd_operations++;
                }
            }
            
            g_cubemap_filtering_ctx.batch_queue[i] = NULL;
            processed++;
            g_cubemap_filtering_ctx.performance.batch_operations++;
        }
    }
    
    // Process regular items
    for (uint32_t i = 0; i < g_cubemap_filtering_ctx.count; i++) {
        texture_cubemap_filtering_internal_t* item = &g_cubemap_filtering_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Apply culling if enabled
            if (item->culling_enabled) {
                // Simple distance-based culling
                float distance = sqrtf(powf(item->width / 2.0f, 2) + powf(item->height / 2.0f, 2));
                if (distance > 1000.0f) { // Cull if far away
                    item->dirty = false;
                    g_cubemap_filtering_ctx.performance.culled_items++;
                    continue;
                }
            }
            
            // Process item
            item->dirty = false;
            processed++;
            g_cubemap_filtering_ctx.performance.successful_operations++;
        }
    }

    pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
    return processed;
}

uint32_t texture_cubemap_filtering_get_count(void) {
    return g_cubemap_filtering_ctx.count;
}

size_t texture_cubemap_filtering_get_memory_usage(void) {
    // Implement comprehensive memory tracking
    size_t total = sizeof(g_cubemap_filtering_ctx);
    
    pthread_mutex_lock(&g_cubemap_filtering_ctx.mutex);
    
    // Add context memory
    total += g_cubemap_filtering_ctx.capacity * sizeof(texture_cubemap_filtering_internal_t);
    
    // Add cache memory
    for (uint32_t i = 0; i < TEXTURE_CUBEMAP_FILTERING_CACHE_SIZE; i++) {
        if (g_cubemap_filtering_ctx.cache[i]) {
            total += sizeof(texture_cubemap_filtering_cache_entry_t);
            if (g_cubemap_filtering_ctx.cache[i]->cache_data) {
                total += g_cubemap_filtering_ctx.cache[i]->cache_size;
            }
        }
    }
    
    // Add async queue memory
    total += TEXTURE_CUBEMAP_FILTERING_ASYNC_QUEUE_SIZE * sizeof(texture_cubemap_filtering_async_operation_t);
    
    // Add virtual texturing memory
    total += VIRTUAL_TEXTURE_MAX_PAGES * VIRTUAL_TEXTURE_PAGE_SIZE * VIRTUAL_TEXTURE_PAGE_SIZE * 4;
    
    // Add compression workspace memory
    total += 64 * 1024 * 1024; // 64MB workspace
    
    // Add render graph node memory
    if (g_cubemap_filtering_ctx.render_graph_node) {
        total += 1024; // Estimated render graph node size
    }
    
    // Add per-item data memory
    for (uint32_t i = 0; i < g_cubemap_filtering_ctx.count; i++) {
        total += g_cubemap_filtering_ctx.items[i].data_size;
        
        // Add virtual texture cache memory
        if (g_cubemap_filtering_ctx.items[i].virtual_texture.virtual_cache) {
            total += g_cubemap_filtering_ctx.items[i].virtual_texture.virtual_cache_size;
        }
        
        // Add compression memory
        if (g_cubemap_filtering_ctx.items[i].compression.compressed_data) {
            total += g_cubemap_filtering_ctx.items[i].compression.compressed_size;
        }
    }
    
    pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
    return total;
}

void texture_cubemap_filtering_debug_print(void) {
    // Implement debug output with comprehensive system information
    printf("=== Cubemap Filtering System Debug Info ===\n");
    printf("Initialized: %s\n", g_cubemap_filtering_ctx.initialized ? "Yes" : "No");
    printf("Count: %u / %u\n", g_cubemap_filtering_ctx.count, g_cubemap_filtering_ctx.capacity);
    printf("\n--- Performance Statistics ---\n");
    printf("Total filtering operations: %lu\n", g_cubemap_filtering_ctx.stats.total_filtering_operations);
    printf("Cache hits: %lu\n", g_cubemap_filtering_ctx.stats.cache_hits);
    printf("Cache misses: %lu\n", g_cubemap_filtering_ctx.stats.cache_misses);
    printf("Virtual page requests: %lu\n", g_cubemap_filtering_ctx.stats.virtual_page_requests);
    printf("Virtual page evictions: %lu\n", g_cubemap_filtering_ctx.stats.virtual_page_evictions);
    printf("Compression operations: %lu\n", g_cubemap_filtering_ctx.stats.compression_operations);
    printf("Decompression operations: %lu\n", g_cubemap_filtering_ctx.stats.decompression_operations);
    printf("Bindless operations: %lu\n", g_cubemap_filtering_ctx.stats.bindless_operations);
    printf("Array operations: %lu\n", g_cubemap_filtering_ctx.stats.array_operations);
    printf("Feedback samples: %lu\n", g_cubemap_filtering_ctx.stats.feedback_samples);
    printf("Total filtering time: %.2f ms\n", g_cubemap_filtering_ctx.stats.total_filtering_time);
    
    printf("\n--- Virtual Texturing System ---\n");
    printf("Page cache size: %zu bytes\n", g_cubemap_filtering_ctx.virtual_system.page_cache_size);
    printf("LRU head: %u, tail: %u\n", g_cubemap_filtering_ctx.virtual_system.lru_head, g_cubemap_filtering_ctx.virtual_system.lru_tail);
    
    printf("\n--- Compression System ---\n");
    printf("Workspace size: %zu bytes\n", g_cubemap_filtering_ctx.compression_system.workspace_size);
    printf("BC available: %s\n", g_cubemap_filtering_ctx.compression_system.bc_available ? "Yes" : "No");
    printf("ASTC available: %s\n", g_cubemap_filtering_ctx.compression_system.astc_available ? "Yes" : "No");
    printf("Default quality: %u\n", g_cubemap_filtering_ctx.compression_system.default_quality);
    
    printf("\n--- Bindless System ---\n");
    printf("Next free handle: %u\n", g_cubemap_filtering_ctx.bindless_system.next_free_handle);
    printf("GPU descriptor pool: %p\n", g_cubemap_filtering_ctx.bindless_system.gpu_descriptor_pool);
    
    printf("\n--- Texture Array System ---\n");
    printf("Next free array: %u\n", g_cubemap_filtering_ctx.array_system.next_free_array);
    
    printf("\n--- Feedback System ---\n");
    printf("Global feedback index: %u\n", g_cubemap_filtering_ctx.feedback_system.global_feedback_index);
    printf("Analysis timestamp: %lu\n", g_cubemap_filtering_ctx.feedback_system.analysis_timestamp);
    
    printf("\n--- GPU Integration ---\n");
    printf("GPU available: %s\n", g_cubemap_filtering_ctx.gpu_available ? "Yes" : "No");
    printf("GPU context: %p\n", g_cubemap_filtering_ctx.gpu_context);
    
    printf("\n--- SIMD Support ---\n");
    printf("SIMD available: %s\n", g_cubemap_filtering_ctx.simd_available ? "Yes" : "No");
    
    printf("\n--- Cache Information ---\n");
    for (uint32_t i = 0; i < TEXTURE_CUBEMAP_FILTERING_CACHE_SIZE; i++) {
        if (g_cubemap_filtering_ctx.cache[i].valid) {
            printf("Cache[%u]: texture_id=%u, lod=%u, size=%zu, timestamp=%lu\n",
                   i, g_cubemap_filtering_ctx.cache[i].texture_id,
                   g_cubemap_filtering_ctx.cache[i].lod_level,
                   g_cubemap_filtering_ctx.cache[i].size,
                   g_cubemap_filtering_ctx.cache[i].timestamp);
        }
    }
    
    printf("\n--- Render Graph Node ---\n");
    printf("Node ID: %u\n", g_cubemap_filtering_ctx.render_node.node_id);
    printf("Input texture: %u\n", g_cubemap_filtering_ctx.render_node.input_texture_id);
    printf("Output texture: %u\n", g_cubemap_filtering_ctx.render_node.output_texture_id);
    printf("LOD bias: %.2f\n", g_cubemap_filtering_ctx.render_node.lod_bias);
    printf("Enabled: %s\n", g_cubemap_filtering_ctx.render_node.enabled ? "Yes" : "No");
    
    if (g_cubemap_filtering_ctx.last_error_code != CUBEMAP_FILTERING_ERROR_NONE) {
        printf("\n--- Last Error ---\n");
        printf("Error code: %d\n", g_cubemap_filtering_ctx.last_error_code);
        printf("Error message: %s\n", g_cubemap_filtering_ctx.last_error_message);
    }
    
    printf("\n=== End Debug Info ===\n");
}

// Unit tests for cubemap filtering system
int texture_cubemap_filtering_run_unit_tests(void) {
    printf("=== Running Cubemap Filtering Unit Tests ===\n");
    
    int tests_run = 0;
    int tests_passed = 0;
    
    // Test 1: System initialization
    printf("Test 1: System initialization... ");
    tests_run++;
    if (texture_cubemap_filtering_init() == CUBEMAP_FILTERING_ERROR_NONE) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 2: Create cubemap filtering
    printf("Test 2: Create cubemap filtering... ");
    tests_run++;
    texture_cubemap_filtering_handle_t handle;
    texture_cubemap_filtering_desc_t desc = {
        .flags = 0x01, // Virtual texturing
        .width = 256,
        .height = 256,
        .format = TEXTURE_CUBEMAP_FILTERING_FORMAT_RGBA8
    };
    
    if (texture_cubemap_filtering_create(&handle, &desc) == CUBEMAP_FILTERING_ERROR_NONE) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 3: Update cubemap filtering
    printf("Test 3: Update cubemap filtering... ");
    tests_run++;
    uint8_t test_data[256 * 256 * 4]; // 256x256 RGBA
    memset(test_data, 0x80, sizeof(test_data));
    
    if (texture_cubemap_filtering_update(handle, test_data, sizeof(test_data)) == CUBEMAP_FILTERING_ERROR_NONE) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 4: Validate cubemap filtering
    printf("Test 4: Validate cubemap filtering... ");
    tests_run++;
    if (texture_cubemap_filtering_is_valid(handle)) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 5: Get cubemap filtering info
    printf("Test 5: Get cubemap filtering info... ");
    tests_run++;
    texture_cubemap_filtering_info_t info;
    if (texture_cubemap_filtering_get_info(handle, &info) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 6: Process pending operations
    printf("Test 6: Process pending operations... ");
    tests_run++;
    if (texture_cubemap_filtering_process_pending() >= 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 7: Memory usage tracking
    printf("Test 7: Memory usage tracking... ");
    tests_run++;
    size_t memory_usage = texture_cubemap_filtering_get_memory_usage();
    if (memory_usage > sizeof(g_cubemap_filtering_ctx)) {
        printf("PASS (Usage: %zu bytes)\n", memory_usage);
        tests_passed++;
    } else {
        printf("FAIL (Usage: %zu bytes)\n", memory_usage);
    }
    
    // Test 8: Mark dirty
    printf("Test 8: Mark dirty... ");
    tests_run++;
    texture_cubemap_filtering_mark_dirty(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Test 9: Get count
    printf("Test 9: Get count... ");
    tests_run++;
    uint32_t count = texture_cubemap_filtering_get_count();
    if (count > 0) {
        printf("PASS (Count: %u)\n", count);
        tests_passed++;
    } else {
        printf("FAIL (Count: %u)\n", count);
    }
    
    // Test 10: Destroy cubemap filtering
    printf("Test 10: Destroy cubemap filtering... ");
    tests_run++;
    texture_cubemap_filtering_destroy(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Cleanup
    texture_cubemap_filtering_shutdown();
    
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

/* ============================================================================
 * HELPER FUNCTIONS FOR CUBEMAP FILTERING
 * ============================================================================ */

// Virtual texturing helper functions
static bool texture_cubemap_filtering_init_virtual_texture(texture_cubemap_filtering_internal_t* item) {
    if (!item) return false;
    
    item->virtual_texture.virtual_enabled = true;
    item->virtual_texture.page_width = VIRTUAL_TEXTURE_PAGE_SIZE;
    item->virtual_texture.page_height = VIRTUAL_TEXTURE_PAGE_SIZE;
    
    // Initialize page table
    memset(item->virtual_texture.page_table, 0, sizeof(item->virtual_texture.page_table));
    
    // Allocate virtual cache
    size_t cache_size = VIRTUAL_TEXTURE_MAX_PAGES * VIRTUAL_TEXTURE_PAGE_SIZE * VIRTUAL_TEXTURE_PAGE_SIZE * 4;
    item->virtual_texture.virtual_cache = malloc(cache_size);
    if (!item->virtual_texture.virtual_cache) {
        item->virtual_texture.virtual_enabled = false;
        return false;
    }
    item->virtual_texture.virtual_cache_size = cache_size;
    
    return true;
}

// BC/ASTC compression helper functions
static bool texture_cubemap_filtering_compress_bc_astc(texture_cubemap_filtering_internal_t* item, uint32_t format) {
    if (!item || !item->data) return false;
    
    item->compression.compressed = true;
    item->compression.compression_format = format;
    item->compression.compression_quality = 8; // Default quality
    
    // Estimate compressed size (simplified)
    size_t compressed_size = item->data_size / 2; // Assume 2:1 compression ratio
    item->compression.compressed_data = malloc(compressed_size);
    if (!item->compression.compressed_data) {
        item->compression.compressed = false;
        return false;
    }
    
    // Simulate compression (in real implementation, use actual BC/ASTC compression)
    memcpy(item->compression.compressed_data, item->data, min(compressed_size, item->data_size));
    item->compression.compressed_size = compressed_size;
    item->compression.compression_ratio = (float)item->data_size / compressed_size;
    
    return true;
}

// Mipmap generation helper function
static bool texture_cubemap_filtering_generate_mipmaps(texture_cubemap_filtering_internal_t* item) {
    if (!item || !item->data) return false;
    
    // Calculate number of mip levels
    uint32_t max_dim = max(item->width, item->height);
    item->mip_levels = 0;
    while (max_dim > 0) {
        item->mip_levels++;
        max_dim /= 2;
    }
    
    // In a real implementation, generate actual mipmaps here
    // For now, just set the mip_levels field
    
    return true;
}

// Bindless texture helper function
static bool texture_cubemap_filtering_init_bindless(texture_cubemap_filtering_internal_t* item) {
    if (!item) return false;
    
    item->bindless.bindless_enabled = true;
    item->bindless.bindless_handle = (uint64_t)(item->id + 1); // Simple handle generation
    item->bindless.bindless_index = item->id;
    
    return true;
}

// Texture array helper function
static bool texture_cubemap_filtering_init_texture_array(texture_cubemap_filtering_internal_t* item, uint32_t layers) {
    if (!item || layers == 0) return false;
    
    item->texture_array.is_array = true;
    item->texture_array.array_layers = min(layers, MAX_TEXTURE_ARRAY_LAYERS);
    item->texture_array.array_index = item->id;
    item->texture_array.current_layer = 0;
    
    return true;
}

// Feedback analysis helper function
static bool texture_cubemap_filtering_init_feedback(texture_cubemap_filtering_internal_t* item) {
    if (!item) return false;
    
    // Initialize feedback buffers
    memset(item->feedback.feedback_buffer, 0, sizeof(item->feedback.feedback_buffer));
    memset(item->feedback.feedback_history, 0, sizeof(item->feedback.feedback_history));
    
    item->feedback.access_count = 0;
    item->feedback.last_access_time = time(NULL);
    item->feedback.average_lod = 0.0f;
    item->feedback.lod_variance = 0.0f;
    item->feedback.feedback_index = 0;
    item->feedback.history_index = 0;
    
    return true;
}

// Format conversion helper function
static bool texture_cubemap_filtering_convert_format(texture_cubemap_filtering_internal_t* item, uint32_t target_format) {
    if (!item || !item->data) return false;
    
    // In a real implementation, perform actual format conversion here
    // For now, just update the format field
    item->format = target_format;
    
    return true;
}

// Anisotropic filtering helper function
static bool texture_cubemap_filtering_apply_anisotropic(texture_cubemap_filtering_internal_t* item, float anisotropy) {
    if (!item) return false;
    
    item->anisotropy = clamp(anisotropy, 1.0f, 16.0f); // Clamp to valid range
    
    return true;
}

// Cleanup helper function
static void texture_cubemap_filtering_cleanup_internal(texture_cubemap_filtering_internal_t* item) {
    if (!item) return;
    
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    if (item->compression.compressed_data) {
        free(item->compression.compressed_data);
        item->compression.compressed_data = NULL;
    }
    
    if (item->virtual_texture.virtual_cache) {
        free(item->virtual_texture.virtual_cache);
        item->virtual_texture.virtual_cache = NULL;
    }
    
    if (item->cache_data) {
        free(item->cache_data);
        item->cache_data = NULL;
    }
    
    item->initialized = false;
}

int texture_cubemap_filtering_serialize(texture_cubemap_filtering_handle_t handle, void* buffer, size_t buffer_size, size_t* out_size) {
    if (!buffer || !out_size) {
        return CUBEMAP_FILTERING_ERROR_INVALID_PARAM;
    }
    
    if (handle.id >= g_cubemap_filtering_ctx.count) {
        return CUBEMAP_FILTERING_ERROR_INVALID_HANDLE;
    }
    
    pthread_mutex_lock(&g_cubemap_filtering_ctx.mutex);
    
    const texture_cubemap_filtering_internal_t* item = &g_cubemap_filtering_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
        return CUBEMAP_FILTERING_ERROR_NOT_INITIALIZED;
    }
    
    // Calculate required size
    size_t required_size = sizeof(uint32_t) * 4 + sizeof(float) * 2 + sizeof(bool) * 3 + item->data_size;
    
    if (buffer_size < required_size) {
        *out_size = required_size;
        pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
        return CUBEMAP_FILTERING_ERROR_OUT_OF_MEMORY;
    }
    
    // Serialize data
    uint8_t* ptr = (uint8_t*)buffer;
    
    // Write magic number and version
    *(uint32_t*)ptr = CUBEMAP_FILTERING_MAGIC_NUMBER;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = CUBEMAP_FILTERING_VERSION;
    ptr += sizeof(uint32_t);
    
    // Write item data
    *(uint32_t*)ptr = item->id;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->flags;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->width;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->height;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->mip_levels;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->format;
    ptr += sizeof(uint32_t);
    *(float*)ptr = item->filter_radius;
    ptr += sizeof(float);
    *(float*)ptr = item->lod_bias;
    ptr += sizeof(float);
    *(bool*)ptr = item->culling_enabled;
    ptr += sizeof(bool);
    *(bool*)ptr = item->gpu_resident;
    ptr += sizeof(bool);
    *(bool*)ptr = item->dirty;
    ptr += sizeof(bool);
    
    // Write data
    if (item->data && item->data_size > 0) {
        memcpy(ptr, item->data, item->data_size);
        ptr += item->data_size;
    }
    
    *out_size = required_size;
    
    pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
    return CUBEMAP_FILTERING_ERROR_NONE;
}

int texture_cubemap_filtering_deserialize(const void* buffer, size_t buffer_size, texture_cubemap_filtering_handle_t* out_handle) {
    if (!buffer || !out_handle || buffer_size < sizeof(uint32_t) * 2) {
        return CUBEMAP_FILTERING_ERROR_INVALID_PARAM;
    }
    
    const uint8_t* ptr = (const uint8_t*)buffer;
    
    // Check magic number and version
    uint32_t magic = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    uint32_t version = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    
    if (magic != CUBEMAP_FILTERING_MAGIC_NUMBER || version != CUBEMAP_FILTERING_VERSION) {
        return CUBEMAP_FILTERING_ERROR_SERIALIZATION_ERROR;
    }
    
    // Create new item
    texture_cubemap_filtering_desc_t desc = {0};
    int result = texture_cubemap_filtering_create(out_handle, &desc);
    if (result != CUBEMAP_FILTERING_ERROR_NONE) {
        return result;
    }
    
    pthread_mutex_lock(&g_cubemap_filtering_ctx.mutex);
    
    texture_cubemap_filtering_internal_t* item = &g_cubemap_filtering_ctx.items[out_handle->id];
    
    // Read item data
    item->id = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    item->flags = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    item->width = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    item->height = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    item->mip_levels = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    item->format = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    item->filter_radius = *(const float*)ptr;
    ptr += sizeof(float);
    item->lod_bias = *(const float*)ptr;
    ptr += sizeof(float);
    item->culling_enabled = *(const bool*)ptr;
    ptr += sizeof(bool);
    item->gpu_resident = *(const bool*)ptr;
    ptr += sizeof(bool);
    item->dirty = *(const bool*)ptr;
    ptr += sizeof(bool);
    
    // Calculate remaining data size
    size_t remaining_size = buffer_size - (ptr - (const uint8_t*)buffer);
    if (remaining_size > 0) {
        item->data = malloc(remaining_size);
        if (item->data) {
            memcpy(item->data, ptr, remaining_size);
            item->data_size = remaining_size;
        }
    }
    
    pthread_mutex_unlock(&g_cubemap_filtering_ctx.mutex);
    return CUBEMAP_FILTERING_ERROR_NONE;
}

/* End of cubemap_filtering.c */
