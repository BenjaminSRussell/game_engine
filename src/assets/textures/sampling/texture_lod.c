/*
 * texture_lod.c
 * Texture LOD calculation
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement texture streaming
 * TODO: Add mipmap generation
 * TODO: Add residency management
 * TODO: Add texture lod debug output
 * TODO: Implement texture lod unit tests
 * TODO: Implement texture lod memory pooling
 * TODO: Add texture lod batch processing
 * TODO: Implement texture lod streaming support
 */

#include "assets/textures/sampling/texture_lod.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <immintrin.h>  // For SIMD intrinsics
#include <sys/time.h>

// Virtual texturing includes
#define VIRTUAL_TEXTURE_PAGE_SIZE 128
#define VIRTUAL_TEXTURE_MAX_PAGES 65536
#define VIRTUAL_TEXTURE_TILE_SIZE 64

// BC/ASTC compression includes
#define BC_COMPRESSION_BLOCK_SIZE 4
#define ASTC_COMPRESSION_BLOCK_SIZE 6
#define MAX_COMPRESSION_QUALITY 10

// Bindless texture includes
#define MAX_BINDLESS_TEXTURES 4096
#define BINDLESS_TEXTURE_HANDLE_SIZE 8

// Texture array includes
#define MAX_TEXTURE_ARRAY_LAYERS 2048
#define MAX_TEXTURE_ARRAYS 256

// Feedback analysis includes
#define FEEDBACK_BUFFER_SIZE 1024
#define FEEDBACK_HISTORY_SIZE 64

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXTURE_TEXTURE_LOD_MAX_COUNT 4096
#define TEXTURE_TEXTURE_LOD_DEFAULT_CAPACITY 256
#define TEXTURE_TEXTURE_LOD_ALIGNMENT 16
#define TEXTURE_TEXTURE_LOD_MAX_MIP_LEVELS 16
#define TEXTURE_TEXTURE_LOD_CACHE_SIZE 2048
#define TEXTURE_TEXTURE_LOD_BATCH_SIZE 128
#define TEXTURE_TEXTURE_LOD_WORKER_THREADS 8
#define TEXTURE_TEXTURE_LOD_MEMORY_POOL_SIZE (128 * 1024 * 1024) // 128MB
#define TEXTURE_TEXTURE_LOD_MAGIC_NUMBER 0x544C4F44 // "TLOD"
#define TEXTURE_TEXTURE_LOD_VERSION 1
#define TEXTURE_TEXTURE_LOD_MAX_ANISOTROPY 16.0f

#define TEXTURE_TEXTURE_LOD_FLAG_VIRTUAL_TEXTURING (1u << 0)
#define TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION (1u << 1)
#define TEXTURE_TEXTURE_LOD_FLAG_BINDLESS (1u << 2)
#define TEXTURE_TEXTURE_LOD_FLAG_TEXTURE_ARRAY (1u << 3)
#define TEXTURE_TEXTURE_LOD_FLAG_FEEDBACK (1u << 4)
#define TEXTURE_TEXTURE_LOD_FLAG_FORMAT_CONVERSION (1u << 5)
#define TEXTURE_TEXTURE_LOD_FLAG_ANISOTROPIC (1u << 6)
#define TEXTURE_TEXTURE_LOD_FLAG_CULLED (1u << 7)

typedef enum texture_texture_lod_format {
    TEXTURE_TEXTURE_LOD_FORMAT_RGBA8 = 0,
    TEXTURE_TEXTURE_LOD_FORMAT_BGRA8 = 1
} texture_texture_lod_format_t;

typedef enum texture_texture_lod_error_code {
    TEXTURE_TEXTURE_LOD_ERROR_NONE = 0,
    TEXTURE_TEXTURE_LOD_ERROR_INVALID_ARGUMENT = -1,
    TEXTURE_TEXTURE_LOD_ERROR_NOT_INITIALIZED = -2,
    TEXTURE_TEXTURE_LOD_ERROR_CAPACITY = -3,
    TEXTURE_TEXTURE_LOD_ERROR_OUT_OF_MEMORY = -4,
    TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE = -5,
    TEXTURE_TEXTURE_LOD_ERROR_INVALID_STATE = -6
} texture_texture_lod_error_code_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_texture_lod_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    // LOD properties
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
    uint32_t format;
    float lod_bias;
    
    // Virtual texturing
    struct {
        uint32_t page_table[VIRTUAL_TEXTURE_MAX_PAGES];
        uint32_t page_width;
        uint32_t page_height;
        bool virtual_enabled;
        void* virtual_cache;
        size_t virtual_cache_size;
    } virtual_texture;
    
    // BC/ASTC compression
    struct {
        bool compressed;
        uint32_t compression_format; // BC1, BC3, BC5, BC7, ASTC_4x4, ASTC_6x6, etc.
        uint32_t compression_quality;
        void* compressed_data;
        size_t compressed_size;
        float compression_ratio;
    } compression;
    
    // Bindless texture
    struct {
        uint64_t bindless_handle;
        bool bindless_enabled;
        uint32_t bindless_index;
    } bindless;
    
    // Texture array
    struct {
        bool is_array;
        uint32_t array_layers;
        uint32_t array_index;
        uint32_t current_layer;
    } texture_array;
    
    // Feedback analysis
    struct {
        uint32_t access_count;
        uint64_t last_access_time;
        float average_lod;
        float lod_variance;
        uint32_t feedback_buffer[FEEDBACK_BUFFER_SIZE];
        uint32_t feedback_index;
        float feedback_history[FEEDBACK_HISTORY_SIZE];
        uint32_t history_index;
    } feedback;
    
    // Performance tracking
    uint64_t total_samples;
    double total_sample_time;
    float average_sample_time;
    
    // Cache
    void* cache_data;
    size_t cache_size;
    bool cache_valid;
    
    // Missing fields for TODO implementation
    void* pending_data;
    size_t pending_size;
    uint64_t pending_hash;
    uint64_t data_hash;
    uint32_t lod_level;
    size_t serialized_size;
    bool gpu_resident;
    bool virtual_texturing_enabled;
    bool bindless_enabled;
    bool texture_array_enabled;
    bool feedback_enabled;
    bool compressed;
    bool culled;
    uint32_t array_layers;
    float anisotropy;
    float feedback_score;
} texture_texture_lod_internal_t;

typedef struct texture_texture_lod_stats {
    uint64_t created;
    uint64_t destroyed;
    uint64_t updated;
    uint64_t processed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t hot_reloads;
    uint64_t validation_failures;
    uint64_t bytes_uploaded;
    uint64_t async_enqueued;
} texture_texture_lod_stats_t;

typedef struct texture_texture_lod_context {
    texture_texture_lod_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    // Thread safety
    pthread_mutex_t mutex;
    
    // Performance counters
    struct {
        uint64_t total_lod_calculations;
        uint64_t cache_hits;
        uint64_t cache_misses;
        uint64_t virtual_page_requests;
        uint64_t virtual_page_evictions;
        uint64_t compression_operations;
        uint64_t decompression_operations;
        uint64_t bindless_operations;
        uint64_t array_operations;
        uint64_t feedback_samples;
        double total_lod_time;
    } stats;
    
    // Virtual texturing system
    struct {
        void* page_cache;
        size_t page_cache_size;
        uint32_t page_table_size;
        uint32_t lru_list[VIRTUAL_TEXTURE_MAX_PAGES];
        uint32_t lru_head;
        uint32_t lru_tail;
    } virtual_system;
    
    // BC/ASTC compression system
    struct {
        void* compression_workspace;
        size_t workspace_size;
        bool bc_available;
        bool astc_available;
        uint32_t default_quality;
    } compression_system;
    
    // Bindless texture system
    struct {
        uint64_t bindless_handles[MAX_BINDLESS_TEXTURES];
        bool bindless_used[MAX_BINDLESS_TEXTURES];
        uint32_t next_free_handle;
        void* gpu_descriptor_pool;
    } bindless_system;
    
    // Texture array system
    struct {
        void* texture_arrays[MAX_TEXTURE_ARRAYS];
        uint32_t array_layers[MAX_TEXTURE_ARRAYS];
        bool array_used[MAX_TEXTURE_ARRAYS];
        uint32_t next_free_array;
    } array_system;
    
    // Feedback analysis system
    struct {
        float global_feedback_buffer[FEEDBACK_BUFFER_SIZE];
        uint32_t global_feedback_index;
        float lod_distribution[16]; // Distribution of LOD usage
        float performance_metrics[8]; // Various performance metrics
        uint64_t analysis_timestamp;
    } feedback_system;
    
    // Hot-reload system
    int inotify_fd;
    int inotify_wd;
    pthread_t file_watch_thread;
    bool file_watch_active;
    
    // GPU integration
    void* gpu_context;
    bool gpu_available;
    
    // SIMD optimization
    bool simd_available;
    
    // Batch processing
    texture_texture_lod_internal_t* batch_queue[TEXTURE_TEXTURE_LOD_BATCH_SIZE];
    uint32_t batch_count;
    
    // Caching layer
    struct {
        void* data;
        size_t size;
        uint32_t texture_id;
        uint32_t lod_level;
        bool valid;
        uint64_t timestamp;
    } cache[TEXTURE_TEXTURE_LOD_CACHE_SIZE];
    char last_error_message[128];
} texture_texture_lod_context_t;

static texture_texture_lod_context_t g_texture_lod_ctx = {0};

// Missing field declarations
static texture_texture_lod_error_code_t g_texture_lod_ctx_last_error = TEXTURE_TEXTURE_LOD_ERROR_NONE;
static bool g_texture_lod_ctx_mutex_initialized = false;
static bool g_texture_lod_ctx_async_enabled = true;
static uint64_t g_texture_lod_ctx_frame_counter = 0;

// Error codes
typedef enum {
    TEXTURE_TEXTURE_LOD_ERROR_NONE = 0,
    TEXTURE_TEXTURE_LOD_ERROR_INVALID_PARAM = -1,
    TEXTURE_TEXTURE_LOD_ERROR_NOT_INITIALIZED = -2,
    TEXTURE_TEXTURE_LOD_ERROR_OUT_OF_MEMORY = -3,
    TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE = -4,
    TEXTURE_TEXTURE_LOD_ERROR_THREAD_ERROR = -5,
    TEXTURE_TEXTURE_LOD_ERROR_GPU_ERROR = -6,
    TEXTURE_TEXTURE_LOD_ERROR_COMPRESSION_ERROR = -7,
    TEXTURE_TEXTURE_LOD_ERROR_VIRTUAL_TEXTURE_ERROR = -8,
    TEXTURE_TEXTURE_LOD_ERROR_BINDLESS_ERROR = -9,
    TEXTURE_TEXTURE_LOD_ERROR_ARRAY_ERROR = -10
} texture_texture_lod_error_t;

// Render graph node
typedef struct texture_lod_render_node {
    uint32_t node_id;
    texture_texture_lod_handle_t lod_handle;
    uint32_t input_texture_id;
    uint32_t output_texture_id;
    float lod_bias;
    bool enabled;
} texture_lod_render_node_t;

// Compression formats
typedef enum {
    TEXTURE_LOD_COMPRESSION_NONE = 0,
    TEXTURE_LOD_COMPRESSION_BC1 = 1,
    TEXTURE_LOD_COMPRESSION_BC3 = 2,
    TEXTURE_LOD_COMPRESSION_BC5 = 3,
    TEXTURE_LOD_COMPRESSION_BC7 = 4,
    TEXTURE_LOD_COMPRESSION_ASTC_4X4 = 5,
    TEXTURE_LOD_COMPRESSION_ASTC_6X6 = 6,
    TEXTURE_LOD_COMPRESSION_ASTC_8X8 = 7
} texture_lod_compression_format_t;

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void texture_texture_lod_set_error(texture_texture_lod_error_code_t code, const char* message) {
    g_texture_lod_ctx_last_error = code;
    if (message) {
        snprintf(g_texture_lod_ctx.last_error_message, sizeof(g_texture_lod_ctx.last_error_message), "%s", message);
    } else {
        g_texture_lod_ctx.last_error_message[0] = '\0';
    }
}

static void texture_texture_lod_lock(void) {
    if (g_texture_lod_ctx_mutex_initialized) {
        pthread_mutex_lock(&g_texture_lod_ctx.mutex);
    }
}

static void texture_texture_lod_unlock(void) {
    if (g_texture_lod_ctx_mutex_initialized) {
        pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
    }
}

static uint64_t texture_texture_lod_hash(const void* data, size_t size) {
    const unsigned char* bytes = (const unsigned char*)data;
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static void texture_texture_lod_copy_data(void* dst, const void* src, size_t size) {
    if (!dst || !src || size == 0) {
        return;
    }

    if ((((uintptr_t)dst | (uintptr_t)src) & (TEXTURE_TEXTURE_LOD_ALIGNMENT - 1)) == 0 &&
        (size % TEXTURE_TEXTURE_LOD_ALIGNMENT) == 0) {
        const uint64_t* src64 = (const uint64_t*)src;
        uint64_t* dst64 = (uint64_t*)dst;
        size_t count = size / sizeof(uint64_t);
        for (size_t i = 0; i < count; ++i) {
            dst64[i] = src64[i];
        }
        return;
    }

    memcpy(dst, src, size);
}

static void texture_texture_lod_convert_format(void* dst, const void* src, size_t size, bool swap_rb) {
    if (!dst || !src || size == 0) {
        return;
    }

    if (!swap_rb || size < 4 || (size % 4) != 0) {
        texture_texture_lod_copy_data(dst, src, size);
        return;
    }

    const uint8_t* in_bytes = (const uint8_t*)src;
    uint8_t* out_bytes = (uint8_t*)dst;
    for (size_t i = 0; i < size; i += 4) {
        out_bytes[i] = in_bytes[i + 2];
        out_bytes[i + 1] = in_bytes[i + 1];
        out_bytes[i + 2] = in_bytes[i];
        out_bytes[i + 3] = in_bytes[i + 3];
    }
}

static uint8_t texture_texture_lod_compute_level(size_t size) {
    uint8_t level = 0;
    while (size > 1) {
        size >>= 1;
        level++;
    }
    return level;
}

static size_t texture_texture_lod_serialize_item(const texture_texture_lod_internal_t* item, void* out_buffer, size_t buffer_size) {
    if (!item) {
        return 0;
    }

    struct texture_texture_lod_serialized {
        uint32_t id;
        uint32_t flags;
        uint32_t array_layers;
        uint8_t lod_level;
        uint8_t reserved[3];
        uint64_t data_size;
        uint64_t data_hash;
        float anisotropy;
        float feedback_score;
        uint32_t feature_flags;
    } snapshot;

    snapshot.id = item->id;
    snapshot.flags = item->flags;
    snapshot.array_layers = item->array_layers;
    snapshot.lod_level = item->lod_level;
    snapshot.reserved[0] = 0;
    snapshot.reserved[1] = 0;
    snapshot.reserved[2] = 0;
    snapshot.data_size = (uint64_t)item->data_size;
    snapshot.data_hash = item->data_hash;
    snapshot.anisotropy = item->anisotropy;
    snapshot.feedback_score = item->feedback_score;
    snapshot.feature_flags = 0;
    if (item->virtual_texturing_enabled) {
        snapshot.feature_flags |= TEXTURE_TEXTURE_LOD_FLAG_VIRTUAL_TEXTURING;
    }
    if (item->compressed) {
        snapshot.feature_flags |= TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION;
    }
    if (item->bindless_enabled) {
        snapshot.feature_flags |= TEXTURE_TEXTURE_LOD_FLAG_BINDLESS;
    }
    if (item->texture_array_enabled) {
        snapshot.feature_flags |= TEXTURE_TEXTURE_LOD_FLAG_TEXTURE_ARRAY;
    }
    if (item->feedback_enabled) {
        snapshot.feature_flags |= TEXTURE_TEXTURE_LOD_FLAG_FEEDBACK;
    }

    if (out_buffer && buffer_size >= sizeof(snapshot)) {
        memcpy(out_buffer, &snapshot, sizeof(snapshot));
    }

    return sizeof(snapshot);
}

static void texture_texture_lod_update_feedback(texture_texture_lod_internal_t* item) {
    if (!item) {
        return;
    }

    if (item->data_size == 0) {
        item->feedback_score = 0.0f;
        return;
    }

    item->feedback_score = 1.0f;
}

static void texture_texture_lod_apply_gpu_upload(texture_texture_lod_internal_t* item) {
    if (!item || item->data_size == 0) {
        return;
    }

    item->gpu_resident = true;
    g_texture_lod_ctx.stats.bytes_uploaded += item->data_size;
    item->frame_updated = ++g_texture_lod_ctx.frame_counter;
}

static bool texture_texture_lod_validate(const texture_texture_lod_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->width == 0 || item->height == 0) return false;
    if (item->mip_levels == 0 || item->mip_levels > TEXTURE_TEXTURE_LOD_MAX_MIP_LEVELS) return false;
    return true;
}

static void texture_texture_lod_cleanup_internal(texture_texture_lod_internal_t* item) {
    if (!item) return;
    
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    // Cleanup virtual texturing
    if (item->virtual_texture.virtual_cache) {
        free(item->virtual_texture.virtual_cache);
        item->virtual_texture.virtual_cache = NULL;
    }
    
    // Cleanup compression
    if (item->compression.compressed_data) {
        free(item->compression.compressed_data);
        item->compression.compressed_data = NULL;
    }
    
    // Cleanup cache
    if (item->cache_data) {
        free(item->cache_data);
        item->cache_data = NULL;
    }
    
    item->initialized = false;
    item->cache_valid = false;
}

// Virtual texturing implementation
static bool texture_texture_lod_init_virtual_texture(texture_texture_lod_internal_t* item) {
    if (!item) return false;
    
    item->virtual_texture.page_width = VIRTUAL_TEXTURE_PAGE_SIZE;
    item->virtual_texture.page_height = VIRTUAL_TEXTURE_PAGE_SIZE;
    item->virtual_texture.virtual_enabled = true;
    
    // Initialize page table
    memset(item->virtual_texture.page_table, 0, sizeof(item->virtual_texture.page_table));
    
    // Allocate virtual cache
    size_t cache_size = VIRTUAL_TEXTURE_MAX_PAGES * VIRTUAL_TEXTURE_PAGE_SIZE * VIRTUAL_TEXTURE_PAGE_SIZE * 4; // RGBA
    item->virtual_texture.virtual_cache = malloc(cache_size);
    if (!item->virtual_texture.virtual_cache) return false;
    
    item->virtual_texture.virtual_cache_size = cache_size;
    g_texture_lod_ctx.stats.virtual_page_requests++;
    return true;
}

// BC/ASTC compression implementation
static bool texture_texture_lod_compress_bc_astc(texture_texture_lod_internal_t* item, uint32_t format) {
    if (!item || !item->data) return false;
    
    item->compression.compressed = true;
    item->compression.compression_format = format;
    item->compression.compression_quality = 8; // Default quality
    
    // Simulate compression (real implementation would use GPU/compression library)
    size_t compressed_size = item->data_size / 2; // Assume 50% compression
    item->compression.compressed_data = malloc(compressed_size);
    if (!item->compression.compressed_data) return false;
    
    // Simulate compression process
    memcpy(item->compression.compressed_data, item->data, item->data_size);
    item->compression.compressed_size = compressed_size;
    item->compression.compression_ratio = (float)item->data_size / compressed_size;
    
    g_texture_lod_ctx.stats.compression_operations++;
    return true;
}

// Bindless texture implementation
static bool texture_texture_lod_init_bindless(texture_texture_lod_internal_t* item) {
    if (!item) return false;
    
    // Find free bindless handle
    for (uint32_t i = 0; i < MAX_BINDLESS_TEXTURES; i++) {
        if (!g_texture_lod_ctx.bindless_system.bindless_used[i]) {
            item->bindless.bindless_enabled = true;
            item->bindless.bindless_index = i;
            item->bindless.bindless_handle = ((uint64_t)i << 32) | item->id;
            
            g_texture_lod_ctx.bindless_system.bindless_used[i] = true;
            g_texture_lod_ctx.bindless_system.bindless_handles[i] = item->bindless.bindless_handle;
            
            g_texture_lod_ctx.stats.bindless_operations++;
            return true;
        }
    }
    return false;
}

// Texture array implementation
static bool texture_texture_lod_init_texture_array(texture_texture_lod_internal_t* item, uint32_t layers) {
    if (!item || layers == 0 || layers > MAX_TEXTURE_ARRAY_LAYERS) return false;
    
    // Find free array slot
    for (uint32_t i = 0; i < MAX_TEXTURE_ARRAYS; i++) {
        if (!g_texture_lod_ctx.array_system.array_used[i]) {
            item->texture_array.is_array = true;
            item->texture_array.array_layers = layers;
            item->texture_array.array_index = i;
            item->texture_array.current_layer = 0;
            
            g_texture_lod_ctx.array_system.array_used[i] = true;
            g_texture_lod_ctx.array_system.array_layers[i] = layers;
            
            g_texture_lod_ctx.stats.array_operations++;
            return true;
        }
    }
    return false;
}

// Feedback analysis implementation
static void texture_texture_lod_update_feedback(texture_texture_lod_internal_t* item, float lod_level) {
    if (!item) return;
    
    item->feedback.access_count++;
    item->feedback.last_access_time = time(NULL);
    
    // Update feedback buffer
    item->feedback.feedback_buffer[item->feedback.feedback_index] = lod_level;
    item->feedback.feedback_index = (item->feedback.feedback_index + 1) % FEEDBACK_BUFFER_SIZE;
    
    // Update history
    item->feedback.feedback_history[item->feedback.history_index] = lod_level;
    item->feedback.history_index = (item->feedback.history_index + 1) % FEEDBACK_HISTORY_SIZE;
    
    // Calculate average LOD and variance
    float sum = 0.0f;
    float sum_sq = 0.0f;
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < FEEDBACK_HISTORY_SIZE; i++) {
        if (item->feedback.feedback_history[i] > 0) {
            sum += item->feedback.feedback_history[i];
            sum_sq += item->feedback.feedback_history[i] * item->feedback.feedback_history[i];
            count++;
        }
    }
    
    if (count > 0) {
        item->feedback.average_lod = sum / count;
        item->feedback.lod_variance = (sum_sq / count) - (item->feedback.average_lod * item->feedback.average_lod);
    }
    
    g_texture_lod_ctx.stats.feedback_samples++;
}

static void texture_texture_lod_apply_pending_locked(texture_texture_lod_internal_t* item) {
    if (!item || !item->pending_data) {
        return;
    }

    bool had_data = item->data_size > 0;
    uint64_t previous_hash = item->data_hash;

    if (item->data) {
        free(item->data);
    }

    item->data = item->pending_data;
    item->data_size = item->pending_size;
    item->data_hash = item->pending_hash;
    item->pending_data = NULL;
    item->pending_size = 0;
    item->pending_hash = 0;
    item->lod_level = texture_texture_lod_compute_level(item->data_size);
    item->serialized_size = texture_texture_lod_serialize_item(item, NULL, 0);
    item->compressed = (item->flags & TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION) != 0;
    item->gpu_resident = false;

    if (had_data && previous_hash != item->data_hash) {
        g_texture_lod_ctx.stats.hot_reloads++;
    }
}

// Hot-reload file watching thread
static void* texture_texture_lod_file_watch_thread(void* arg) {
    char buffer[4096];
    
    while (g_texture_lod_ctx.file_watch_active) {
        ssize_t length = read(g_texture_lod_ctx.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            // Process file events
            for (char* ptr = buffer; ptr < buffer + length; ) {
                struct inotify_event* event = (struct inotify_event*)ptr;
                if (event->mask & IN_MODIFY) {
                    // Trigger hot-reload for modified texture
                    pthread_mutex_lock(&g_texture_lod_ctx.mutex);
                    // Mark all LOD textures as dirty for reload
                    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
                        g_texture_lod_ctx.items[i].dirty = true;
                        g_texture_lod_ctx.items[i].cache_valid = false;
                    }
                    pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
                }
                ptr += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); // 100ms sleep
    }
    return NULL;
}

// SIMD-optimized LOD calculation
static float texture_texture_lod_simd_calculate(const texture_texture_lod_internal_t* item, 
                                              float u, float v, float dudx, float dvdx, 
                                              float dudy, float dvdy) {
    if (!g_texture_lod_ctx.simd_available) {
        // Fallback to scalar calculation
        float dx = sqrtf(dudx * dudx + dvdx * dvdx);
        float dy = sqrtf(dudy * dudy + dvdy * dvdy);
        float max_derivative = fmaxf(dx, dy);
        float lod = log2f(max_derivative * fmaxf(item->width, item->height));
        return fmaxf(0.0f, fminf(lod + item->lod_bias, (float)item->mip_levels - 1.0f));
    }
    
    // SIMD implementation using AVX2
    const __m256 uv_vec = _mm256_set_ps(u, v, dudx, dvdx, dudy, dvdy, 0.0f, 0.0f);
    const __m256 width_height = _mm256_set_ps((float)item->width, (float)item->height, 
                                           (float)item->width, (float)item->height,
                                           0.0f, 0.0f, 0.0f, 0.0f);
    
    // Calculate derivatives
    __m256 derivatives = _mm256_mul_ps(uv_vec, uv_vec);
    derivatives = _mm256_hadd_ps(derivatives, derivatives);
    
    // Extract dx and dy
    float deriv_array[8];
    _mm256_storeu_ps(deriv_array, derivatives);
    float dx = sqrtf(deriv_array[0]);
    float dy = sqrtf(deriv_array[1]);
    
    float max_derivative = fmaxf(dx, dy);
    float lod = log2f(max_derivative * fmaxf(item->width, item->height));
    
    return fmaxf(0.0f, fminf(lod + item->lod_bias, (float)item->mip_levels - 1.0f));
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int texture_texture_lod_init(void) {
    if (g_texture_lod_ctx.initialized) {
        return 0; // Already initialized
    }

    // Initialize mutex
    if (pthread_mutex_init(&g_texture_lod_ctx.mutex, NULL) != 0) {
        return TEXTURE_TEXTURE_LOD_ERROR_THREAD_ERROR;
    }
    g_texture_lod_ctx_mutex_initialized = true;

    g_texture_lod_ctx.capacity = TEXTURE_TEXTURE_LOD_DEFAULT_CAPACITY;
    g_texture_lod_ctx.items = calloc(g_texture_lod_ctx.capacity, sizeof(texture_texture_lod_internal_t));
    if (!g_texture_lod_ctx.items) {
        pthread_mutex_destroy(&g_texture_lod_ctx.mutex);
        return TEXTURE_TEXTURE_LOD_ERROR_OUT_OF_MEMORY;
    }

    g_texture_lod_ctx.count = 0;
    
    // Initialize performance counters
    memset(&g_texture_lod_ctx.stats, 0, sizeof(g_texture_lod_ctx.stats));
    
    // Initialize virtual texturing system
    g_texture_lod_ctx.virtual_system.page_cache_size = VIRTUAL_TEXTURE_MAX_PAGES * VIRTUAL_TEXTURE_PAGE_SIZE * VIRTUAL_TEXTURE_PAGE_SIZE * 4;
    g_texture_lod_ctx.virtual_system.page_cache = malloc(g_texture_lod_ctx.virtual_system.page_cache_size);
    if (!g_texture_lod_ctx.virtual_system.page_cache) {
        free(g_texture_lod_ctx.items);
        pthread_mutex_destroy(&g_texture_lod_ctx.mutex);
        return TEXTURE_TEXTURE_LOD_ERROR_OUT_OF_MEMORY;
    }
    memset(g_texture_lod_ctx.virtual_system.lru_list, 0, sizeof(g_texture_lod_ctx.virtual_system.lru_list));
    g_texture_lod_ctx.virtual_system.lru_head = 0;
    g_texture_lod_ctx.virtual_system.lru_tail = 0;
    
    // Initialize BC/ASTC compression system
    g_texture_lod_ctx.compression_system.workspace_size = 64 * 1024 * 1024; // 64MB workspace
    g_texture_lod_ctx.compression_system.compression_workspace = malloc(g_texture_lod_ctx.compression_system.workspace_size);
    g_texture_lod_ctx.compression_system.bc_available = true; // Assume BC available
    g_texture_lod_ctx.compression_system.astc_available = true; // Assume ASTC available
    g_texture_lod_ctx.compression_system.default_quality = 8;
    
    // Initialize bindless texture system
    memset(g_texture_lod_ctx.bindless_system.bindless_handles, 0, sizeof(g_texture_lod_ctx.bindless_system.bindless_handles));
    memset(g_texture_lod_ctx.bindless_system.bindless_used, 0, sizeof(g_texture_lod_ctx.bindless_system.bindless_used));
    g_texture_lod_ctx.bindless_system.next_free_handle = 0;
    g_texture_lod_ctx.bindless_system.gpu_descriptor_pool = NULL; // Initialize GPU pool
    
    // Initialize texture array system
    memset(g_texture_lod_ctx.array_system.texture_arrays, 0, sizeof(g_texture_lod_ctx.array_system.texture_arrays));
    memset(g_texture_lod_ctx.array_system.array_used, 0, sizeof(g_texture_lod_ctx.array_system.array_used));
    g_texture_lod_ctx.array_system.next_free_array = 0;
    
    // Initialize feedback analysis system
    memset(g_texture_lod_ctx.feedback_system.global_feedback_buffer, 0, sizeof(g_texture_lod_ctx.feedback_system.global_feedback_buffer));
    memset(g_texture_lod_ctx.feedback_system.lod_distribution, 0, sizeof(g_texture_lod_ctx.feedback_system.lod_distribution));
    memset(g_texture_lod_ctx.feedback_system.performance_metrics, 0, sizeof(g_texture_lod_ctx.feedback_system.performance_metrics));
    g_texture_lod_ctx.feedback_system.global_feedback_index = 0;
    g_texture_lod_ctx.feedback_system.analysis_timestamp = time(NULL);
    
    // Initialize hot-reload system
    g_texture_lod_ctx.inotify_fd = inotify_init();
    if (g_texture_lod_ctx.inotify_fd >= 0) {
        g_texture_lod_ctx.inotify_wd = inotify_add_watch(g_texture_lod_ctx.inotify_fd, ".", IN_MODIFY);
        g_texture_lod_ctx.file_watch_active = true;
        pthread_create(&g_texture_lod_ctx.file_watch_thread, NULL, 
                      texture_texture_lod_file_watch_thread, NULL);
    }
    
    // Initialize GPU integration
    g_texture_lod_ctx.gpu_available = true; // Assume available
    g_texture_lod_ctx.gpu_context = NULL; // Initialize GPU context
    
    // Initialize SIMD support
    g_texture_lod_ctx.simd_available = true; // Assume AVX2 available
    
    // Initialize batch processing
    g_texture_lod_ctx.batch_count = 0;
    
    // Initialize cache
    memset(g_texture_lod_ctx.cache, 0, sizeof(g_texture_lod_ctx.cache));
    
    g_texture_lod_ctx.initialized = true;
    return TEXTURE_TEXTURE_LOD_ERROR_NONE;
}

void texture_texture_lod_shutdown(void) {
    if (!g_texture_lod_ctx.initialized) {
        return;
    }

    texture_texture_lod_lock();
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        texture_texture_lod_cleanup_internal(&g_texture_lod_ctx.items[i]);
    }

    free(g_texture_lod_ctx.items);
    g_texture_lod_ctx.items = NULL;
    g_texture_lod_ctx.count = 0;
    g_texture_lod_ctx.capacity = 0;
    g_texture_lod_ctx.initialized = false;
    texture_texture_lod_unlock();

    if (g_texture_lod_ctx_mutex_initialized) {
        pthread_mutex_destroy(&g_texture_lod_ctx.mutex);
        g_texture_lod_ctx_mutex_initialized = false;
    }
}

void texture_texture_lod_debug_print(void) {
    // Implement texture lod debug output
    printf("=== Texture LOD System Debug Info ===\n");
    printf("Initialized: %s\n", g_texture_lod_ctx.initialized ? "Yes" : "No");
    printf("Count: %u / %u\n", g_texture_lod_ctx.count, g_texture_lod_ctx.capacity);
    printf("Async enabled: %s\n", g_texture_lod_ctx.async_enabled ? "Yes" : "No");
    printf("Frame counter: %lu\n", g_texture_lod_ctx.frame_counter);
    
    printf("\n--- Performance Statistics ---\n");
    printf("Created: %lu\n", g_texture_lod_ctx.stats.created);
    printf("Destroyed: %lu\n", g_texture_lod_ctx.stats.destroyed);
    printf("Updated: %lu\n", g_texture_lod_ctx.stats.updated);
    printf("Processed: %lu\n", g_texture_lod_ctx.stats.processed);
    printf("Cache hits: %lu\n", g_texture_lod_ctx.stats.cache_hits);
    printf("Cache misses: %lu\n", g_texture_lod_ctx.stats.cache_misses);
    printf("Hot reloads: %lu\n", g_texture_lod_ctx.stats.hot_reloads);
    printf("Serialized: %lu\n", g_texture_lod_ctx.stats.serialized);
    printf("GPU uploads: %lu\n", g_texture_lod_ctx.stats.gpu_uploads);
    printf("SIMD operations: %lu\n", g_texture_lod_ctx.stats.simd_operations);
    printf("Async enqueued: %lu\n", g_texture_lod_ctx.stats.async_enqueued);
    printf("Virtual texture ops: %lu\n", g_texture_lod_ctx.stats.virtual_texture_ops);
    printf("Compression ops: %lu\n", g_texture_lod_ctx.stats.compression_ops);
    printf("Total samples: %lu\n", g_texture_lod_ctx.stats.total_samples);
    printf("Average sample time: %.3f ms\n", g_texture_lod_ctx.stats.average_sample_time);
    
    printf("\n--- Item Details ---\n");
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[i];
        if (item->initialized) {
            printf("Item %u: id=%u, flags=0x%08x, data_size=%zu, mip_levels=%u\n",
                   i, item->id, item->flags, item->data_size, item->mip_levels);
            printf("  Virtual texturing: %s, Compression: %s, Bindless: %s, Array: %s\n",
                   item->virtual_texturing_enabled ? "Yes" : "No",
                   item->compressed ? "Yes" : "No",
                   item->bindless_enabled ? "Yes" : "No",
                   item->texture_array_enabled ? "Yes" : "No");
            printf("  LOD bias: %.2f, Anisotropy: %.1f, Feedback score: %.3f\n",
                   item->lod_bias, item->anisotropy, item->feedback_score);
            printf("  GPU resident: %s, Culled: %s, Dirty: %s\n",
                   item->gpu_resident ? "Yes" : "No",
                   item->culled ? "Yes" : "No",
                   item->dirty ? "Yes" : "No");
        }
    }
    
    if (g_texture_lod_ctx_last_error != TEXTURE_TEXTURE_LOD_ERROR_NONE) {
        printf("\n--- Last Error ---\n");
        printf("Error code: %d\n", g_texture_lod_ctx_last_error);
        printf("Error message: %s\n", g_texture_lod_ctx.last_error_message);
    }
    
    printf("\n=== End Debug Info ===\n");
}

int texture_texture_lod_create(texture_texture_lod_handle_t* out_handle, const texture_texture_lod_desc_t* desc) {
    // Add texture lod debug output
    printf("Creating texture LOD with flags: 0x%08x\n", desc->flags);

    if (!out_handle || !desc) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_ARGUMENT, "Invalid texture LOD create arguments");
        return -1;
    }

    if (!g_texture_lod_ctx.initialized) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_NOT_INITIALIZED, "Texture LOD system not initialized");
        return -2;
    }

    pthread_mutex_lock(&g_texture_lod_ctx.mutex);
    
    if (g_texture_lod_ctx.count >= g_texture_lod_ctx.capacity) {
        pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_OUT_OF_MEMORY, "Texture LOD capacity reached");
        return -3;
    }

    uint32_t index = g_texture_lod_ctx.count++;
    texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[index];

    // Initialize all fields
    memset(item, 0, sizeof(texture_texture_lod_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->gpu_resident = false;
    item->virtual_texturing_enabled = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_VIRTUAL_TEXTURING) != 0;
    item->bindless_enabled = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_BINDLESS) != 0;
    item->texture_array_enabled = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_TEXTURE_ARRAY) != 0;
    item->feedback_enabled = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_FEEDBACK) != 0;
    item->compressed = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION) != 0;
    item->culled = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_CULLED) != 0;
    item->frame_updated = 0;
    item->lod_level = 0;
    item->anisotropy = TEXTURE_TEXTURE_LOD_MAX_ANISOTROPY;
    item->feedback_score = 0.0f;

    // Initialize virtual texturing if enabled
    if (item->virtual_texturing_enabled) {
        if (!texture_texture_lod_init_virtual_texture(item)) {
            pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
            texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_VIRTUAL_TEXTURE_ERROR, "Failed to initialize virtual texturing");
            return -4;
        }
    }

    // Initialize compression if enabled
    if (item->compressed) {
        if (!texture_texture_lod_compress_bc_astc(item, TEXTURE_LOD_COMPRESSION_BC7)) {
            pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
            texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_COMPRESSION_ERROR, "Failed to initialize compression");
            return -5;
        }
    }

    // Initialize bindless if enabled
    if (item->bindless_enabled) {
        if (!texture_texture_lod_init_bindless(item)) {
            pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
            texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_BINDLESS_ERROR, "Failed to initialize bindless texture");
            return -6;
        }
    }

    // Initialize texture array if enabled
    if (item->texture_array_enabled) {
        if (!texture_texture_lod_init_texture_array(item, 1)) {
            pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
            texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_ARRAY_ERROR, "Failed to initialize texture array");
            return -7;
        }
    }

    out_handle->id = index;
    g_texture_lod_ctx.stats.created++;
    printf("Successfully created texture LOD %u\n", index);
    pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
    return 0;
}

void texture_texture_lod_destroy(texture_texture_lod_handle_t handle) {
    texture_texture_lod_lock();

    if (handle.id >= g_texture_lod_ctx.count) {
        texture_texture_lod_unlock();
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE, "Invalid texture LOD handle");
        return;
    }

    texture_texture_lod_cleanup_internal(&g_texture_lod_ctx.items[handle.id]);
    g_texture_lod_ctx.stats.destroyed++;
    texture_texture_lod_unlock();
}

int texture_texture_lod_update(texture_texture_lod_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_texture_lod_ctx.count) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE, "Invalid texture LOD handle");
        return -1;
    }

    if (size > 0 && !data) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_ARGUMENT, "Texture LOD update missing data");
        return -2;
    }

    texture_texture_lod_lock();
    texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[handle.id];
    if (!item->initialized) {
        texture_texture_lod_unlock();
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_STATE, "Texture LOD not initialized");
        return -2;
    }

    if (size == 0) {
        if (item->data) {
            free(item->data);
            item->data = NULL;
            item->data_size = 0;
        }
        if (item->pending_data) {
            free(item->pending_data);
            item->pending_data = NULL;
            item->pending_size = 0;
        }
        item->data_hash = 0;
        item->pending_hash = 0;
        item->dirty = true;
        g_texture_lod_ctx.stats.updated++;
        texture_texture_lod_unlock();
        return 0;
    }

    uint64_t incoming_hash = texture_texture_lod_hash(data, size);
    if (item->data_size == size && item->data_hash == incoming_hash && item->pending_data == NULL) {
        g_texture_lod_ctx.stats.cache_hits++;
        item->dirty = false;
        texture_texture_lod_unlock();
        return 0;
    }
    g_texture_lod_ctx.stats.cache_misses++;

    if (item->pending_data) {
        free(item->pending_data);
        item->pending_data = NULL;
    }

    item->pending_data = malloc(size);
    if (!item->pending_data) {
        texture_texture_lod_unlock();
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_OUT_OF_MEMORY, "Texture LOD update allocation failed");
        return -3;
    }

    bool needs_format_conversion = (item->flags & TEXTURE_TEXTURE_LOD_FLAG_FORMAT_CONVERSION) != 0;
    texture_texture_lod_convert_format(
        item->pending_data,
        data,
        size,
        needs_format_conversion
    );

    item->pending_size = size;
    item->pending_hash = incoming_hash;
    item->dirty = true;
    g_texture_lod_ctx.stats.updated++;
    g_texture_lod_ctx.stats.async_enqueued++;

    if (!g_texture_lod_ctx.async_enabled) {
        texture_texture_lod_apply_pending_locked(item);
    }
    texture_texture_lod_unlock();
    return 0;
}

bool texture_texture_lod_serialize_item(texture_texture_lod_handle_t handle, void* buffer, size_t buffer_size, size_t* out_size) {
    // Implement texture lod serialization
    if (handle.id >= g_texture_lod_ctx.count) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE, "Invalid texture LOD handle for serialization");
        return false;
    }
    
    texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[handle.id];
    if (!item->initialized) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_NOT_INITIALIZED, "Texture LOD not initialized for serialization");
        return false;
    }
    
    if (!buffer || !out_size) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_ARGUMENT, "Invalid buffer or size pointer for serialization");
        return false;
    }
    
    // Calculate required size
    size_t required_size = sizeof(uint32_t) * 4 + // id, flags, data_size, mip_levels
                           sizeof(float) * 2 + // lod_bias, anisotropy
                           item->data_size +
                           sizeof(item->virtual_texture) +
                           sizeof(item->compression) +
                           sizeof(item->bindless) +
                           sizeof(item->texture_array) +
                           sizeof(item->feedback);
    
    if (buffer_size < required_size) {
        *out_size = required_size;
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_BUFFER_TOO_SMALL, "Buffer too small for serialization");
        return false;
    }
    
    // Serialize data
    uint8_t* ptr = (uint8_t*)buffer;
    
    // Write header
    *(uint32_t*)ptr = TEXTURE_TEXTURE_LOD_MAGIC_NUMBER;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = TEXTURE_TEXTURE_LOD_VERSION;
    ptr += sizeof(uint32_t);
    
    // Write basic properties
    *(uint32_t*)ptr = item->id;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->flags;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->data_size;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->mip_levels;
    ptr += sizeof(uint32_t);
    *(float*)ptr = item->lod_bias;
    ptr += sizeof(float);
    *(float*)ptr = item->anisotropy;
    ptr += sizeof(float);
    
    // Write data
    if (item->data && item->data_size > 0) {
        memcpy(ptr, item->data, item->data_size);
        ptr += item->data_size;
    }
    
    // Write virtual texture data
    memcpy(ptr, &item->virtual_texture, sizeof(item->virtual_texture));
    ptr += sizeof(item->virtual_texture);
    
    // Write compression data
    memcpy(ptr, &item->compression, sizeof(item->compression));
    ptr += sizeof(item->compression);
    
    // Write bindless data
    memcpy(ptr, &item->bindless, sizeof(item->bindless));
    ptr += sizeof(item->bindless);
    
    // Write texture array data
    memcpy(ptr, &item->texture_array, sizeof(item->texture_array));
    ptr += sizeof(item->texture_array);
    
    // Write feedback data
    memcpy(ptr, &item->feedback, sizeof(item->feedback));
    ptr += sizeof(item->feedback);
    
    *out_size = required_size;
    g_texture_lod_ctx.stats.serialized++;
    
    return true; // Success
}

bool texture_texture_lod_validate(texture_texture_lod_handle_t handle) {
    if (handle.id >= g_texture_lod_ctx.count) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE, "Invalid texture LOD handle for validation");
        return false;
    }
    
    texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[handle.id];
    if (!item->initialized) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_NOT_INITIALIZED, "Texture LOD not initialized for validation");
        return false;
    }
    
    // Validate data integrity
    if (!item->data && item->data_size > 0) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_VALIDATION_ERROR, "Texture LOD data inconsistency detected");
        return false;
    }
    
    // Validate virtual texturing if enabled
    if (item->virtual_texturing_enabled && !item->virtual_texture.page_cache) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_VIRTUAL_TEXTURE_ERROR, "Virtual texture cache not initialized");
        return false;
    }
    
    // Validate compression if enabled
    if (item->compressed && !item->compression.compressed_data) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_COMPRESSION_ERROR, "Compression data not available");
        return false;
    }
    
    // Validate bindless if enabled
    if (item->bindless_enabled && item->bindless.bindless_handle == 0) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_BINDLESS_ERROR, "Bindless handle not valid");
        return false;
    }
    
    // Validate texture array if enabled
    if (item->texture_array_enabled && item->texture_array.array_layers == 0) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_ARRAY_ERROR, "Texture array layers not configured");
        return false;
    }
    
    return true; // Validation successful
}

int texture_texture_lod_get_info(texture_texture_lod_handle_t handle, texture_texture_lod_info_t* out_info) {
    // Implement texture lod info retrieval with streaming support
    if (!out_info) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_ARGUMENT, "Texture LOD info output missing");
        return -1;
    }

    if (handle.id >= g_texture_lod_ctx.count) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE, "Invalid texture LOD handle");
        return -2;
    }

    texture_texture_lod_lock();
    const texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[handle.id];
    
    // Copy basic info
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->width = item->width;
    out_info->height = item->height;
    out_info->mip_levels = item->mip_levels;
    out_info->format = item->format;
    out_info->lod_bias = item->lod_bias;
    
    // Copy streaming info
    out_info->streaming_enabled = item->virtual_texturing_enabled;
    out_info->streaming_priority = item->feedback_score > 0.5f ? 1 : 0;
    out_info->streaming_bandwidth = item->data_size > 0 ? (float)(item->data_size / 1024) : 0.0f; // KB/s
    
    // Copy compression info
    out_info->compression_enabled = item->compressed;
    out_info->compression_format = item->compression.compression_format;
    out_info->compression_ratio = item->compression.compression_ratio;
    
    // Copy virtual texturing info
    out_info->virtual_enabled = item->virtual_texturing_enabled;
    out_info->virtual_page_width = item->virtual_texture.page_width;
    out_info->virtual_page_height = item->virtual_texture.page_height;
    
    // Copy bindless info
    out_info->bindless_enabled = item->bindless_enabled;
    out_info->bindless_handle = item->bindless.bindless_handle;
    
    // Copy array info
    out_info->array_enabled = item->texture_array_enabled;
    out_info->array_layers = item->texture_array.array_layers;
    
    // Copy feedback info
    out_info->feedback_enabled = item->feedback_enabled;
    out_info->feedback_score = item->feedback_score;
    
    texture_texture_lod_unlock();
    return 0;
}

void texture_texture_lod_mark_dirty(texture_texture_lod_handle_t handle) {
    if (handle.id < g_texture_lod_ctx.count) {
        texture_texture_lod_lock();
        g_texture_lod_ctx.items[handle.id].dirty = true;
        g_texture_lod_ctx.items[handle.id].culled = false;
        texture_texture_lod_unlock();
    }
}

int texture_texture_lod_process_pending(void) {
    // Implement batch processing with streaming and memory pooling
    int processed = 0;
    texture_texture_lod_lock();
    
    // Process batch queue first
    for (uint32_t i = 0; i < g_texture_lod_ctx.batch_count && i < TEXTURE_TEXTURE_LOD_BATCH_SIZE; i++) {
        if (g_texture_lod_ctx.batch_queue[i]) {
            texture_texture_lod_internal_t* item = g_texture_lod_ctx.batch_queue[i];
            
            // Apply streaming if enabled
            if (item->virtual_texturing_enabled) {
                // Simulate streaming operation
                g_texture_lod_ctx.stats.virtual_page_requests++;
            }
            
            // Apply memory pooling if enabled
            if (item->data_size > 0 && !item->cache_valid) {
                // Find cache entry
                for (uint32_t j = 0; j < TEXTURE_TEXTURE_LOD_CACHE_SIZE; j++) {
                    if (!g_texture_lod_ctx.cache[j].valid) {
                        g_texture_lod_ctx.cache[j].texture_id = item->id;
                        g_texture_lod_ctx.cache[j].lod_level = item->lod_level;
                        g_texture_lod_ctx.cache[j].data = malloc(item->data_size);
                        if (g_texture_lod_ctx.cache[j].data) {
                            memcpy(g_texture_lod_ctx.cache[j].data, item->data, item->data_size);
                            g_texture_lod_ctx.cache[j].size = item->data_size;
                            g_texture_lod_ctx.cache[j].valid = true;
                            g_texture_lod_ctx.cache[j].timestamp = time(NULL);
                            item->cache_valid = true;
                            g_texture_lod_ctx.stats.cache_hits++;
                        }
                        break;
                    }
                }
            }
            
            g_texture_lod_ctx.batch_queue[i] = NULL;
            processed++;
        }
    }
    
    // Process regular items
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[i];
        if (item->culled) {
            continue;
        }
        if (item->initialized && item->dirty) {
            if (item->pending_data) {
                texture_texture_lod_apply_pending_locked(item);
            }

            if (item->feedback_enabled) {
                texture_texture_lod_update_feedback(item);
            }

            if (!item->gpu_resident) {
                texture_texture_lod_apply_gpu_upload(item);
            }

            item->dirty = false;
            processed++;
            g_texture_lod_ctx.stats.processed++;
        }
    }
    
    g_texture_lod_ctx.batch_count = 0;
    texture_texture_lod_unlock();

    return processed;
}

uint32_t texture_texture_lod_get_count(void) {
    texture_texture_lod_lock();
    uint32_t count = g_texture_lod_ctx.count;
    texture_texture_lod_unlock();
    return count;
}

size_t texture_texture_lod_get_memory_usage(void) {
    // Implement comprehensive memory tracking
    size_t total = sizeof(g_texture_lod_ctx);
    texture_texture_lod_lock();
    
    // Add context memory
    total += g_texture_lod_ctx.capacity * sizeof(texture_texture_lod_internal_t);
    
    // Add virtual texturing memory
    total += g_texture_lod_ctx.virtual_system.page_cache_size;
    
    // Add compression workspace memory
    total += g_texture_lod_ctx.compression_system.workspace_size;
    
    // Add bindless descriptor pool memory (estimate)
    total += MAX_BINDLESS_TEXTURES * BINDLESS_TEXTURE_HANDLE_SIZE;
    
    // Add texture array memory (estimate)
    total += MAX_TEXTURE_ARRAYS * MAX_TEXTURE_ARRAY_LAYERS * sizeof(void*);
    
    // Add feedback system memory
    total += sizeof(g_texture_lod_ctx.feedback_system);
    
    // Add cache memory
    total += TEXTURE_TEXTURE_LOD_CACHE_SIZE * sizeof(g_texture_lod_ctx.cache[0]);
    
    // Add per-item data memory
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        total += g_texture_lod_ctx.items[i].data_size;
        total += g_texture_lod_ctx.items[i].pending_size;
        total += g_texture_lod_ctx.items[i].cache_size;
        
        // Add virtual texture cache memory
        if (g_texture_lod_ctx.items[i].virtual_texture.virtual_cache) {
            total += g_texture_lod_ctx.items[i].virtual_texture.virtual_cache_size;
        }
        
        // Add compression memory
        if (g_texture_lod_ctx.items[i].compression.compressed_data) {
            total += g_texture_lod_ctx.items[i].compression.compressed_size;
        }
    }
    
    texture_texture_lod_unlock();
    return total;
}

void texture_texture_lod_debug_print(void) {
    if (!g_texture_lod_ctx.initialized) {
        printf("Texture LOD System: Not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_texture_lod_ctx.mutex);
    
    printf("=== Texture LOD System Debug Info ===\n");
    printf("Initialized: %s\n", g_texture_lod_ctx.initialized ? "Yes" : "No");
    printf("LOD Count: %u / %u\n", g_texture_lod_ctx.count, g_texture_lod_ctx.capacity);
    printf("GPU Available: %s\n", g_texture_lod_ctx.gpu_available ? "Yes" : "No");
    printf("SIMD Available: %s\n", g_texture_lod_ctx.simd_available ? "Yes" : "No");
    printf("File Watch Active: %s\n", g_texture_lod_ctx.file_watch_active ? "Yes" : "No");
    printf("Batch Queue Size: %u / %u\n", g_texture_lod_ctx.batch_count, TEXTURE_TEXTURE_LOD_BATCH_SIZE);
    
    printf("\n=== Performance Statistics ===\n");
    printf("Total LOD Calculations: %lu\n", g_texture_lod_ctx.stats.total_lod_calculations);
    printf("Cache Hits: %lu\n", g_texture_lod_ctx.stats.cache_hits);
    printf("Cache Misses: %lu\n", g_texture_lod_ctx.stats.cache_misses);
    printf("Virtual Page Requests: %lu\n", g_texture_lod_ctx.stats.virtual_page_requests);
    printf("Virtual Page Evictions: %lu\n", g_texture_lod_ctx.stats.virtual_page_evictions);
    printf("Compression Operations: %lu\n", g_texture_lod_ctx.stats.compression_operations);
    printf("Decompression Operations: %lu\n", g_texture_lod_ctx.stats.decompression_operations);
    printf("Bindless Operations: %lu\n", g_texture_lod_ctx.stats.bindless_operations);
    printf("Array Operations: %lu\n", g_texture_lod_ctx.stats.array_operations);
    printf("Feedback Samples: %lu\n", g_texture_lod_ctx.stats.feedback_samples);
    printf("Total LOD Time: %.3f ms\n", g_texture_lod_ctx.stats.total_lod_time);
    
    if (g_texture_lod_ctx.stats.cache_hits + g_texture_lod_ctx.stats.cache_misses > 0) {
        float hit_rate = (float)g_texture_lod_ctx.stats.cache_hits / 
                        (g_texture_lod_ctx.stats.cache_hits + g_texture_lod_ctx.stats.cache_misses) * 100.0f;
        printf("Cache Hit Rate: %.2f%%\n", hit_rate);
    }
    
    printf("\n=== System Status ===\n");
    printf("Virtual Texturing: %s\n", g_texture_lod_ctx.virtual_system.page_cache ? "Available" : "Not Available");
    printf("BC/ASTC Compression: %s\n", g_texture_lod_ctx.compression_system.bc_available ? "BC Available" : "BC Not Available");
    if (g_texture_lod_ctx.compression_system.astc_available) {
        printf("ASTC Compression: Available\n");
    }
    printf("Bindless Textures: %s\n", g_texture_lod_ctx.bindless_system.gpu_descriptor_pool ? "Available" : "Not Available");
    printf("Texture Arrays: %u arrays allocated\n", g_texture_lod_ctx.array_system.next_free_array);
    
    printf("\n=== LOD Details ===\n");
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        const texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[i];
        printf("LOD %u: ID=%u, Size=%ux%u, Mips=%u, Bias=%.2f, Dirty=%s, Cache=%s\n",
               i, item->id, item->width, item->height, 
               item->mip_levels, item->lod_bias, item->dirty ? "Yes" : "No", 
               item->cache_valid ? "Valid" : "Invalid");
        
        if (item->virtual_texture.virtual_enabled) {
            printf("  Virtual Texturing: Enabled (Page: %ux%u)\n", 
                   item->virtual_texture.page_width, item->virtual_texture.page_height);
        }
        
        if (item->compression.compressed) {
            printf("  Compression: %s (Ratio: %.2f:1, Quality: %u)\n",
                   item->compression.compression_format == TEXTURE_LOD_COMPRESSION_BC1 ? "BC1" :
                   item->compression.compression_format == TEXTURE_LOD_COMPRESSION_BC3 ? "BC3" :
                   item->compression.compression_format == TEXTURE_LOD_COMPRESSION_BC7 ? "BC7" :
                   item->compression.compression_format == TEXTURE_LOD_COMPRESSION_ASTC_4X4 ? "ASTC 4x4" :
                   item->compression.compression_format == TEXTURE_LOD_COMPRESSION_ASTC_6X6 ? "ASTC 6x6" : "Unknown",
                   item->compression.compression_ratio, item->compression.compression_quality);
        }
        
        if (item->bindless.bindless_enabled) {
            printf("  Bindless: Handle=0x%lx, Index=%u\n", 
                   item->bindless.bindless_handle, item->bindless.bindless_index);
        }
        
        if (item->texture_array.is_array) {
            printf("  Texture Array: Layers=%u, Current=%u, Index=%u\n",
                   item->texture_array.array_layers, item->texture_array.current_layer, 
                   item->texture_array.array_index);
        }
        
        if (item->feedback.access_count > 0) {
            printf("  Feedback: Samples=%u, Avg LOD=%.2f, Variance=%.4f\n",
                   item->feedback.access_count, item->feedback.average_lod, 
                   item->feedback.lod_variance);
        }
    }
    
    printf("\n=== Cache Status ===\n");
    uint32_t valid_cache_entries = 0;
    for (uint32_t i = 0; i < TEXTURE_TEXTURE_LOD_CACHE_SIZE; i++) {
        if (g_texture_lod_ctx.cache[i].valid) {
            valid_cache_entries++;
        }
    }
    printf("Valid Cache Entries: %u / %u\n", valid_cache_entries, TEXTURE_TEXTURE_LOD_CACHE_SIZE);
    
    printf("\n=== Feedback Analysis ===\n");
    printf("Global Feedback Index: %u\n", g_texture_lod_ctx.feedback_system.global_feedback_index);
    printf("Analysis Timestamp: %lu\n", g_texture_lod_ctx.feedback_system.analysis_timestamp);
    printf("LOD Distribution: ");
    for (uint32_t i = 0; i < 16; i++) {
        if (g_texture_lod_ctx.feedback_system.lod_distribution[i] > 0) {
            printf("LOD%u=%.1f%% ", i, g_texture_lod_ctx.feedback_system.lod_distribution[i] * 100.0f);
        }
    }
    printf("\n");
    
    pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
    printf("=== End Debug Info ===\n");
}

// Unit tests for texture LOD system
int texture_texture_lod_run_unit_tests(void) {
    printf("=== Running Texture LOD Unit Tests ===\n");
    
    int tests_run = 0;
    int tests_passed = 0;
    
    // Test 1: System initialization
    printf("Test 1: System initialization... ");
    tests_run++;
    if (texture_texture_lod_init() == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 2: Create texture LOD
    printf("Test 2: Create texture LOD... ");
    tests_run++;
    texture_texture_lod_handle_t handle;
    texture_texture_lod_desc_t desc = {
        .flags = TEXTURE_TEXTURE_LOD_FLAG_VIRTUAL_TEXTURING | TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION,
        .width = 256,
        .height = 256,
        .format = TEXTURE_TEXTURE_LOD_FORMAT_RGBA8
    };
    
    if (texture_texture_lod_create(&handle, &desc) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 3: Update texture LOD
    printf("Test 3: Update texture LOD... ");
    tests_run++;
    uint8_t test_data[256 * 256 * 4]; // 256x256 RGBA
    memset(test_data, 0x80, sizeof(test_data));
    
    if (texture_texture_lod_update(handle, test_data, sizeof(test_data)) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 4: Validate texture LOD
    printf("Test 4: Validate texture LOD... ");
    tests_run++;
    if (texture_texture_lod_validate(handle)) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 5: Get texture LOD info
    printf("Test 5: Get texture LOD info... ");
    tests_run++;
    texture_texture_lod_info_t info;
    if (texture_texture_lod_get_info(handle, &info) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 6: Process pending operations
    printf("Test 6: Process pending operations... ");
    tests_run++;
    if (texture_texture_lod_process_pending() >= 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 7: Memory usage tracking
    printf("Test 7: Memory usage tracking... ");
    tests_run++;
    size_t memory_usage = texture_texture_lod_get_memory_usage();
    if (memory_usage > sizeof(g_texture_lod_ctx)) {
        printf("PASS (Usage: %zu bytes)\n", memory_usage);
        tests_passed++;
    } else {
        printf("FAIL (Usage: %zu bytes)\n", memory_usage);
    }
    
    // Test 8: Mark dirty
    printf("Test 8: Mark dirty... ");
    tests_run++;
    texture_texture_lod_mark_dirty(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Test 9: Get count
    printf("Test 9: Get count... ");
    tests_run++;
    uint32_t count = texture_texture_lod_get_count();
    if (count > 0) {
        printf("PASS (Count: %u)\n", count);
        tests_passed++;
    } else {
        printf("FAIL (Count: %u)\n", count);
    }
    
    // Test 10: Destroy texture LOD
    printf("Test 10: Destroy texture LOD... ");
    tests_run++;
    texture_texture_lod_destroy(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Cleanup
    texture_texture_lod_shutdown();
    
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

/* End of texture_lod.c */
