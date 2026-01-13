/*
 * sampler_cache.c
 * Sampler state caching
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
 * TODO: Implement sampler cache initialization
 * TODO: Add sampler cache cleanup/shutdown
 * TODO: Implement sampler cache validation
 * TODO: Add sampler cache error handling
 * TODO: Implement sampler cache serialization
 * TODO: Add sampler cache debug output
 * TODO: Implement sampler cache unit tests
 * TODO: Add sampler cache performance counters
 * TODO: Implement sampler cache hot-reload
 * TODO: Add sampler cache thread safety
 * TODO: Implement sampler cache memory pooling
 * TODO: Add sampler cache caching layer
 * TODO: Implement sampler cache async operations
 * TODO: Add sampler cache GPU integration
 * TODO: Implement sampler cache SIMD optimization
 * TODO: Add sampler cache batch processing
 * TODO: Implement sampler cache streaming support
 * TODO: Add sampler cache LOD support
 * TODO: Implement sampler cache culling integration
 * TODO: Add sampler cache render graph node
 */

#include "assets/textures/sampling/sampler_cache.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>
#include <immintrin.h>
#include <errno.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXTURE_SAMPLER_CACHE_MAX_COUNT 4096
#define TEXTURE_SAMPLER_CACHE_DEFAULT_CAPACITY 256
#define TEXTURE_SAMPLER_CACHE_ALIGNMENT 16
#define TEXTURE_SAMPLER_CACHE_MAGIC_NUMBER 0x53414D50  // "SAMP"
#define TEXTURE_SAMPLER_CACHE_VERSION 1
#define TEXTURE_SAMPLER_CACHE_MAX_WATCHES 64
#define TEXTURE_SAMPLER_CACHE_SIZE 512
#define TEXTURE_SAMPLER_CACHE_WORKER_THREADS 2
#define TEXTURE_SAMPLER_CACHE_MAX_LOD 8
#define TEXTURE_SAMPLER_CACHE_MAX_ANISOTROPY 16
#define TEXTURE_SAMPLER_CACHE_MAGIC 0x53414D50  // "SAMP"
#define TEXTURE_SAMPLER_CACHE_VERSION 1
#define TEXTURE_SAMPLER_CACHE_MAGIC 0x53414D50  // "SAMP"
#define TEXTURE_SAMPLER_CACHE_VERSION 1
#define TEXTURE_SAMPLER_CACHE_MAX_WATCHES 128
#define TEXTURE_SAMPLER_CACHE_ASYNC_THREADS 4
#define TEXTURE_SAMPLER_CACHE_BATCH_SIZE 64

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_sampler_cache_stats {
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t total_requests;
    uint32_t evictions;
    uint64_t total_memory_allocated;
    uint64_t peak_memory_usage;
    double avg_lookup_time_ms;
    uint32_t hot_reload_count;
    uint32_t gpu_operations;
    uint32_t simd_operations;
    uint32_t batch_operations;
} texture_sampler_cache_stats_t;

typedef struct texture_sampler_cache_batch {
    uint32_t* item_ids;
    uint32_t count;
    uint32_t capacity;
} texture_sampler_cache_batch_t;

typedef struct texture_sampler_cache_lod {
    uint32_t level;
    float min_distance;
    float max_distance;
    uint32_t texture_size;
} texture_sampler_cache_lod_t;

typedef struct texture_virtual_tile {
    uint32_t x, y, level;
    uint32_t tile_id;
    bool resident;
    uint64_t last_access;
} texture_virtual_tile_t;

typedef enum {
    TEXTURE_COMPRESSION_NONE = 0,
    TEXTURE_COMPRESSION_BC1,
    TEXTURE_COMPRESSION_BC3,
    TEXTURE_COMPRESSION_BC5,
    TEXTURE_COMPRESSION_BC7,
    TEXTURE_COMPRESSION_ASTC_4x4,
    TEXTURE_COMPRESSION_ASTC_6x6,
    TEXTURE_COMPRESSION_ASTC_8x8
} texture_compression_format_t;

typedef struct texture_mipmap_level {
    uint32_t width, height;
    size_t data_size;
    void* data;
    texture_compression_format_t compression;
} texture_mipmap_level_t;

typedef struct texture_bindless_handle {
    uint64_t gpu_handle;
    uint32_t array_index;
    bool bound;
} texture_bindless_handle_t;

typedef struct texture_array {
    uint32_t layer_count;
    uint32_t width, height;
    texture_bindless_handle_t* handles;
    void* array_data;
} texture_array_t;

typedef struct texture_feedback_data {
    uint32_t read_count;
    uint32_t write_count;
    float avg_mipmap_level;
    float anisotropy_ratio;
    bool frequently_accessed;
} texture_feedback_data_t;

typedef struct texture_lod_info {
    uint32_t min_level;
    uint32_t max_level;
    float lod_bias;
    bool anisotropic_enabled;
    float max_anisotropy;
} texture_lod_info_t;

typedef struct texture_culling_data {
    bool culled;
    float distance_to_camera;
    uint32_t screen_coverage;
    uint64_t last_cull_frame;
} texture_culling_data_t;

typedef struct texture_render_graph_node {
    uint32_t node_id;
    uint32_t dependency_count;
    uint32_t* dependencies;
    bool processed;
    uint64_t frame_processed;
} texture_render_graph_node_t;

typedef struct texture_async_operation {
    uint32_t operation_id;
    enum {
        TEXTURE_ASYNC_LOAD,
        TEXTURE_ASYNC_SAVE,
        TEXTURE_ASYNC_COMPRESS,
        TEXTURE_ASYNC_GENERATE_MIPMAPS
    } type;
    void* data;
    size_t data_size;
    bool completed;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
} texture_async_operation_t;

typedef struct texture_gpu_context {
    void* device;
    void* command_queue;
    void* memory_pool;
    bool initialized;
} texture_gpu_context_t;

typedef struct texture_simd_context {
    __m128i* simd_buffers;
    size_t buffer_count;
    bool avx2_supported;
    bool sse4_supported;
} texture_simd_context_t;

typedef struct texture_batch_operation {
    uint32_t operation_count;
    texture_sampler_cache_handle_t* handles;
    void** data_buffers;
    size_t* data_sizes;
} texture_batch_operation_t;

typedef struct texture_file_watch {
    int watch_descriptor;
    char file_path[256];
    uint64_t last_modified;
    texture_sampler_cache_handle_t associated_handle;
} texture_file_watch_t;

typedef struct texture_serialization_header {
    uint32_t magic;
    uint32_t version;
    uint32_t item_count;
    uint64_t timestamp;
    uint32_t checksum;
} texture_serialization_header_t;

typedef struct texture_cache_entry {
    texture_sampler_cache_handle_t handle;
    void* cached_data;
    size_t cached_size;
    uint64_t last_access;
    uint32_t access_count;
    bool dirty;
} texture_cache_entry_t;

typedef enum {
    TEXTURE_SAMPLER_CACHE_ERROR_NONE = 0,
    TEXTURE_SAMPLER_CACHE_ERROR_INVALID_PARAM = -1,
    TEXTURE_SAMPLER_CACHE_ERROR_NOT_INITIALIZED = -2,
    TEXTURE_SAMPLER_CACHE_ERROR_OUT_OF_MEMORY = -3,
    TEXTURE_SAMPLER_CACHE_ERROR_CAPACITY_EXCEEDED = -4,
    TEXTURE_SAMPLER_CACHE_ERROR_INVALID_HANDLE = -5,
    TEXTURE_SAMPLER_CACHE_ERROR_SERIALIZATION_FAILED = -6,
    TEXTURE_SAMPLER_CACHE_ERROR_ASYNC_FAILED = -7,
    TEXTURE_SAMPLER_CACHE_ERROR_GPU_INTEGRATION_FAILED = -8
} texture_sampler_cache_error_t;

typedef struct texture_sampler_cache_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    uint64_t last_access_time;
    uint32_t access_count;
    texture_sampler_cache_lod_t lod;
    bool gpu_resident;
    void* gpu_handle;
    
    /* Advanced features */
    texture_virtual_tile_t* virtual_tiles;
    uint32_t virtual_tile_count;
    texture_compression_format_t compression_format;
    texture_mipmap_level_t* mipmaps;
    uint32_t mipmap_count;
    texture_bindless_handle_t bindless_handle;
    texture_array_t* texture_array;
    texture_feedback_data_t feedback_data;
    texture_lod_info_t lod_info;
    texture_culling_data_t culling_data;
    texture_render_graph_node_t render_node;
    texture_cache_entry_t cache_entry;
} texture_sampler_cache_internal_t;

typedef struct texture_sampler_cache_context {
    texture_sampler_cache_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Thread safety */
    pthread_mutex_t global_mutex;
    pthread_rwlock_t cache_lock;
    
    /* Performance counters */
    texture_sampler_cache_stats_t stats;
    
    /* Hot-reload system */
    int inotify_fd;
    texture_file_watch_t file_watches[TEXTURE_SAMPLER_CACHE_MAX_WATCHES];
    uint32_t watch_count;
    pthread_t hot_reload_thread;
    bool hot_reload_running;
    
    /* Async operations */
    texture_async_operation_t async_operations[TEXTURE_SAMPLER_CACHE_ASYNC_THREADS];
    pthread_t async_threads[TEXTURE_SAMPLER_CACHE_ASYNC_THREADS];
    pthread_mutex_t async_mutex;
    uint32_t async_operation_count;
    
    /* GPU integration */
    texture_gpu_context_t gpu_context;
    
    /* SIMD optimization */
    texture_simd_context_t simd_context;
    
    /* Batch processing */
    texture_batch_operation_t current_batch;
    pthread_mutex_t batch_mutex;
    
    /* Caching layer */
    texture_cache_entry_t* cache_entries;
    uint32_t cache_capacity;
    uint32_t cache_count;
    size_t cache_memory_budget;
    size_t cache_memory_used;
    
    /* Render graph */
    texture_render_graph_node_t* render_nodes;
    uint32_t render_node_count;
    
    bool simd_optimization_enabled;
    bool batch_processing_enabled;
    bool culling_integration_enabled;
    bool render_graph_enabled;
} texture_sampler_cache_context_t;

static texture_sampler_cache_context_t g_sampler_cache_ctx = {0};

/* ============================================================================
 * HELPER FUNCTIONS - Advanced Features
 * ============================================================================ */

/* Virtual texturing implementation */
static int texture_sampler_cache_init_virtual_tiles(texture_sampler_cache_internal_t* item, uint32_t tile_count) {
    if (!item || tile_count == 0) return -1;
    
    item->virtual_tiles = calloc(tile_count, sizeof(texture_virtual_tile_t));
    if (!item->virtual_tiles) return -2;
    
    item->virtual_tile_count = tile_count;
    
    /* Initialize virtual tiles */
    for (uint32_t i = 0; i < tile_count; i++) {
        item->virtual_tiles[i].tile_id = i;
        item->virtual_tiles[i].resident = false;
        item->virtual_tiles[i].last_access = texture_sampler_cache_get_time_ms();
    }
    
    return 0;
}

/* BC/ASTC compression implementation */
static int texture_sampler_cache_compress_data(const void* src_data, size_t src_size, 
                                               texture_compression_format_t format,
                                               void** out_data, size_t* out_size) {
    if (!src_data || src_size == 0 || !out_data || !out_size) return -1;
    
    /* Simulate compression - in real implementation would use GPU compression */
    size_t compressed_size = src_size / 2; /* Assume 50% compression ratio */
    void* compressed_data = malloc(compressed_size);
    if (!compressed_data) return -2;
    
    /* Simple compression simulation */
    const uint8_t* src = (const uint8_t*)src_data;
    uint8_t* dst = (uint8_t*)compressed_data;
    
    for (size_t i = 0; i < compressed_size; i++) {
        dst[i] = src[i * 2] ^ src[i * 2 + 1]; /* Simple compression */
    }
    
    *out_data = compressed_data;
    *out_size = compressed_size;
    return 0;
}

/* Mipmap generation implementation */
static int texture_sampler_cache_generate_mipmaps(texture_sampler_cache_internal_t* item, 
                                                   uint32_t base_width, uint32_t base_height) {
    if (!item || base_width == 0 || base_height == 0) return -1;
    
    /* Calculate number of mipmap levels */
    uint32_t max_dim = base_width > base_height ? base_width : base_height;
    uint32_t levels = 0;
    while (max_dim > 0) {
        levels++;
        max_dim >>= 1;
    }
    
    item->mipmaps = calloc(levels, sizeof(texture_mipmap_level_t));
    if (!item->mipmaps) return -2;
    
    item->mipmap_count = levels;
    
    /* Generate mipmap levels */
    uint32_t width = base_width;
    uint32_t height = base_height;
    
    for (uint32_t i = 0; i < levels; i++) {
        item->mipmaps[i].width = width;
        item->mipmaps[i].height = height;
        item->mipmaps[i].compression = item->compression_format;
        
        size_t level_size = width * height * 4; /* Assume RGBA8 */
        item->mipmaps[i].data = malloc(level_size);
        if (!item->mipmaps[i].data) return -3;
        
        /* Simple mipmap generation - in real implementation would use proper filtering */
        if (i == 0 && item->data) {
            memcpy(item->mipmaps[i].data, item->data, level_size);
        } else if (i > 0) {
            /* Downsample from previous level */
            const uint8_t* prev = (const uint8_t*)item->mipmaps[i-1].data;
            uint8_t* curr = (uint8_t*)item->mipmaps[i].data;
            
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    uint32_t src_x = x * 2;
                    uint32_t src_y = y * 2;
                    uint32_t src_width = item->mipmaps[i-1].width;
                    
                    /* Simple 2x2 box filter */
                    for (int c = 0; c < 4; c++) {
                        uint32_t sum = 0;
                        uint32_t count = 0;
                        
                        for (int dy = 0; dy < 2; dy++) {
                            for (int dx = 0; dx < 2; dx++) {
                                uint32_t sx = src_x + dx;
                                uint32_t sy = src_y + dy;
                                if (sx < item->mipmaps[i-1].width && sy < item->mipmaps[i-1].height) {
                                    sum += prev[(sy * src_width + sx) * 4 + c];
                                    count++;
                                }
                            }
                        }
                        
                        curr[(y * width + x) * 4 + c] = count > 0 ? (uint8_t)(sum / count) : 0;
                    }
                }
            }
        }
        
        item->mipmaps[i].data_size = level_size;
        
        width = width > 1 ? width >> 1 : 1;
        height = height > 1 ? height >> 1 : 1;
    }
    
    return 0;
}

/* Bindless texture implementation */
static int texture_sampler_cache_init_bindless(texture_sampler_cache_internal_t* item) {
    if (!item) return -1;
    
    /* Simulate bindless texture handle creation */
    item->bindless_handle.gpu_handle = (uint64_t)(item->id + 1) << 32; /* Simulated GPU handle */
    item->bindless_handle.array_index = item->id;
    item->bindless_handle.bound = true;
    
    return 0;
}

/* Texture array implementation */
static int texture_sampler_cache_init_texture_array(texture_sampler_cache_internal_t* item, uint32_t layer_count) {
    if (!item || layer_count == 0) return -1;
    
    item->texture_array = malloc(sizeof(texture_array_t));
    if (!item->texture_array) return -2;
    
    item->texture_array->layer_count = layer_count;
    item->texture_array->width = 256; /* Default size */
    item->texture_array->height = 256;
    
    item->texture_array->handles = calloc(layer_count, sizeof(texture_bindless_handle_t));
    if (!item->texture_array->handles) {
        free(item->texture_array);
        item->texture_array = NULL;
        return -3;
    }
    
    size_t array_size = layer_count * 256 * 256 * 4; /* RGBA8 */
    item->texture_array->array_data = malloc(array_size);
    if (!item->texture_array->array_data) {
        free(item->texture_array->handles);
        free(item->texture_array);
        item->texture_array = NULL;
        return -4;
    }
    
    /* Initialize array handles */
    for (uint32_t i = 0; i < layer_count; i++) {
        item->texture_array->handles[i].gpu_handle = ((uint64_t)(item->id + 1) << 32) | i;
        item->texture_array->handles[i].array_index = i;
        item->texture_array->handles[i].bound = true;
    }
    
    return 0;
}

/* Feedback analysis implementation */
static void texture_sampler_cache_update_feedback(texture_sampler_cache_internal_t* item) {
    if (!item) return;
    
    uint64_t current_time = texture_sampler_cache_get_time_ms();
    uint64_t time_diff = current_time - item->feedback_data.last_access;
    
    /* Update access patterns */
    if (time_diff < 1000) { /* Accessed within last second */
        item->feedback_data.frequently_accessed = true;
        item->feedback_data.read_count++;
    }
    
    /* Calculate average mipmap level based on distance */
    item->feedback_data.avg_mipmap_level = item->lod_info.lod_bias;
    
    /* Calculate anisotropy ratio */
    if (item->lod_info.anisotropic_enabled) {
        item->feedback_data.anisotropy_ratio = item->lod_info.max_anisotropy / 16.0f;
    } else {
        item->feedback_data.anisotropy_ratio = 1.0f;
    }
    
    item->feedback_data.last_access = current_time;
}

/* LOD support implementation */
static void texture_sampler_cache_update_lod(texture_sampler_cache_internal_t* item, float distance) {
    if (!item) return;
    
    /* Calculate LOD based on distance */
    if (distance < item->lod_info.min_distance) {
        item->lod_info.lod_bias = 0.0f;
    } else if (distance > item->lod_info.max_distance) {
        item->lod_info.lod_bias = (float)item->lod_info.max_level;
    } else {
        float normalized = (distance - item->lod_info.min_distance) / 
                         (item->lod_info.max_distance - item->lod_info.min_distance);
        item->lod_info.lod_bias = normalized * item->lod_info.max_level;
    }
}

/* Culling integration implementation */
static void texture_sampler_cache_update_culling(texture_sampler_cache_internal_t* item, 
                                                float distance, uint32_t screen_coverage) {
    if (!item) return;
    
    item->culling_data.distance_to_camera = distance;
    item->culling_data.screen_coverage = screen_coverage;
    item->culling_data.last_cull_frame = g_sampler_cache_ctx.stats.total_requests;
    
    /* Simple culling logic */
    item->culling_data.culled = (screen_coverage < 1) || (distance > item->lod_info.max_distance * 2.0f);
}

/* Render graph node implementation */
static int texture_sampler_cache_add_render_node(texture_sampler_cache_internal_t* item) {
    if (!item) return -1;
    
    /* Expand render nodes array if needed */
    if (g_sampler_cache_ctx.render_node_count >= g_sampler_cache_ctx.capacity) {
        return -2; /* Capacity exceeded */
    }
    
    item->render_node.node_id = g_sampler_cache_ctx.render_node_count++;
    item->render_node.dependency_count = 0;
    item->render_node.dependencies = NULL;
    item->render_node.processed = false;
    item->render_node.frame_processed = 0;
    
    return 0;
}

/* SIMD optimization implementation */
static int texture_sampler_cache_simd_process(const void* src, void* dst, size_t size) {
    if (!src || !dst || size == 0) return -1;
    
    if (!g_sampler_cache_ctx.simd_context.avx2_supported && 
        !g_sampler_cache_ctx.simd_context.sse4_supported) {
        /* Fallback to non-SIMD processing */
        memcpy(dst, src, size);
        return 0;
    }
    
    size_t simd_size = size & ~15; /* Process in 16-byte chunks */
    
    if (g_sampler_cache_ctx.simd_context.avx2_supported && simd_size >= 32) {
        /* AVX2 implementation */
        for (size_t i = 0; i < simd_size; i += 32) {
            __m256i data = _mm256_loadu_si256((const __m256i*)((const uint8_t*)src + i));
            _mm256_storeu_si256((__m256i*)((uint8_t*)dst + i), data);
        }
    } else if (g_sampler_cache_ctx.simd_context.sse4_supported) {
        /* SSE4 implementation */
        for (size_t i = 0; i < simd_size; i += 16) {
            __m128i data = _mm_loadu_si128((const __m128i*)((const uint8_t*)src + i));
            _mm_storeu_si128((__m128i*)((uint8_t*)dst + i), data);
        }
    }
    
    /* Handle remaining bytes */
    if (simd_size < size) {
        memcpy((uint8_t*)dst + simd_size, (const uint8_t*)src + simd_size, size - simd_size);
    }
    
    g_sampler_cache_ctx.stats.simd_operations++;
    return 0;
}

/* Hot-reload thread function */
static void* texture_sampler_cache_hot_reload_thread(void* arg) {
    (void)arg; /* Unused */
    
    char buffer[4096];
    while (g_sampler_cache_ctx.hot_reload_running) {
        /* Wait for file system events */
        ssize_t length = read(g_sampler_cache_ctx.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            size_t i = 0;
            while (i < length) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                
                /* Find associated watch */
                for (uint32_t w = 0; w < g_sampler_cache_ctx.watch_count; w++) {
                    if (g_sampler_cache_ctx.file_watches[w].watch_descriptor == event->wd) {
                        /* File changed - trigger reload */
                        texture_sampler_cache_handle_t handle = 
                            g_sampler_cache_ctx.file_watches[w].associated_handle;
                        
                        if (handle.id < g_sampler_cache_ctx.count) {
                            g_sampler_cache_ctx.items[handle.id].dirty = true;
                            g_sampler_cache_ctx.stats.hot_reload_count++;
                        }
                        break;
                    }
                }
                
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        
        usleep(100000); /* 100ms delay */
    }
    
    return NULL;
}

/* Async operation thread function */
static void* texture_sampler_cache_async_thread(void* arg) {
    uint32_t thread_id = *(uint32_t*)arg;
    
    while (g_sampler_cache_ctx.initialized) {
        pthread_mutex_lock(&g_sampler_cache_ctx.async_mutex);
        
        /* Find pending async operation */
        texture_async_operation_t* operation = NULL;
        for (uint32_t i = 0; i < TEXTURE_SAMPLER_CACHE_ASYNC_THREADS; i++) {
            if (!g_sampler_cache_ctx.async_operations[i].completed) {
                operation = &g_sampler_cache_ctx.async_operations[i];
                break;
            }
        }
        
        pthread_mutex_unlock(&g_sampler_cache_ctx.async_mutex);
        
        if (operation) {
            /* Process async operation */
            switch (operation->type) {
                case TEXTURE_ASYNC_LOAD:
                    /* Simulate async load */
                    usleep(10000); /* 10ms simulated load time */
                    break;
                case TEXTURE_ASYNC_SAVE:
                    /* Simulate async save */
                    usleep(5000); /* 5ms simulated save time */
                    break;
                case TEXTURE_ASYNC_COMPRESS:
                    /* Simulate async compression */
                    usleep(20000); /* 20ms simulated compression time */
                    break;
                case TEXTURE_ASYNC_GENERATE_MIPMAPS:
                    /* Simulate async mipmap generation */
                    usleep(15000); /* 15ms simulated mipmap generation */
                    break;
            }
            
            /* Mark operation as completed */
            pthread_mutex_lock(&operation->completion_mutex);
            operation->completed = true;
            pthread_cond_signal(&operation->completion_cond);
            pthread_mutex_unlock(&operation->completion_mutex);
            
            g_sampler_cache_ctx.stats.async_operations++;
        } else {
            usleep(1000); /* 1ms delay when no work */
        }
    }
    
    return NULL;
}

static uint64_t texture_sampler_cache_get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

static uint32_t texture_sampler_cache_calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = checksum * 31 + bytes[i];
    }
    return checksum;
}

static int texture_sampler_cache_serialize_item(const texture_sampler_cache_internal_t* item, void** out_data, size_t* out_size) {
    if (!item || !out_data || !out_size) return -1;
    
    size_t total_size = sizeof(uint32_t) * 4 + sizeof(uint64_t) * 3 + sizeof(bool) * 3 + item->data_size;
    void* data = malloc(total_size);
    if (!data) return -2;
    
    uint8_t* ptr = (uint8_t*)data;
    *(uint32_t*)ptr = TEXTURE_SAMPLER_CACHE_MAGIC; ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = TEXTURE_SAMPLER_CACHE_VERSION; ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->id; ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->flags; ptr += sizeof(uint32_t);
    *(uint64_t*)ptr = item->data_size; ptr += sizeof(uint64_t);
    *(uint64_t*)ptr = item->frame_updated; ptr += sizeof(uint64_t);
    *(uint64_t*)ptr = item->last_access_time; ptr += sizeof(uint64_t);
    *(bool*)ptr = item->initialized; ptr += sizeof(bool);
    *(bool*)ptr = item->dirty; ptr += sizeof(bool);
    *(bool*)ptr = item->gpu_resident; ptr += sizeof(bool);
    *(uint32_t*)ptr = item->access_count; ptr += sizeof(uint32_t);
    
    if (item->data && item->data_size > 0) {
        memcpy(ptr, item->data, item->data_size);
    }
    
    *out_data = data;
    *out_size = total_size;
    return 0;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool texture_sampler_cache_validate(const texture_sampler_cache_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->id >= TEXTURE_SAMPLER_CACHE_MAX_COUNT) return false;
    if (item->data_size > 0 && !item->data) return false;
    return true;
}

static void texture_sampler_cache_cleanup_internal(texture_sampler_cache_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    if (item->gpu_handle && g_sampler_cache_ctx.gpu_integration_enabled) {
        // GPU cleanup would go here
        item->gpu_handle = NULL;
    }
    item->initialized = false;
    item->dirty = false;
    item->gpu_resident = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int texture_sampler_cache_init(void) {
    if (g_sampler_cache_ctx.initialized) {
        return 0; // Already initialized
    }

    /* Initialize thread safety */
    if (pthread_mutex_init(&g_sampler_cache_ctx.global_mutex, NULL) != 0) return -1;
    if (pthread_rwlock_init(&g_sampler_cache_ctx.cache_lock, NULL) != 0) return -2;
    if (pthread_mutex_init(&g_sampler_cache_ctx.async_mutex, NULL) != 0) return -3;
    if (pthread_mutex_init(&g_sampler_cache_ctx.batch_mutex, NULL) != 0) return -4;

    /* Initialize main cache */
    g_sampler_cache_ctx.capacity = TEXTURE_SAMPLER_CACHE_DEFAULT_CAPACITY;
    g_sampler_cache_ctx.items = calloc(g_sampler_cache_ctx.capacity, sizeof(texture_sampler_cache_internal_t));
    if (!g_sampler_cache_ctx.items) return -5;

    /* Initialize caching layer */
    g_sampler_cache_ctx.cache_capacity = TEXTURE_SAMPLER_CACHE_DEFAULT_CAPACITY;
    g_sampler_cache_ctx.cache_entries = calloc(g_sampler_cache_ctx.cache_capacity, sizeof(texture_cache_entry_t));
    g_sampler_cache_ctx.cache_memory_budget = 512 * 1024 * 1024; // 512MB
    g_sampler_cache_ctx.cache_memory_used = 0;

    /* Initialize hot-reload system */
    g_sampler_cache_ctx.inotify_fd = inotify_init();
    if (g_sampler_cache_ctx.inotify_fd >= 0) {
        g_sampler_cache_ctx.hot_reload_running = true;
        pthread_create(&g_sampler_cache_ctx.hot_reload_thread, NULL, texture_sampler_cache_hot_reload_thread, NULL);
    }

    /* Initialize async operations */
    for (uint32_t i = 0; i < TEXTURE_SAMPLER_CACHE_ASYNC_THREADS; i++) {
        pthread_mutex_init(&g_sampler_cache_ctx.async_operations[i].completion_mutex, NULL);
        pthread_cond_init(&g_sampler_cache_ctx.async_operations[i].completion_cond, NULL);
        g_sampler_cache_ctx.async_operations[i].completed = true;
        
        uint32_t thread_id = i;
        pthread_create(&g_sampler_cache_ctx.async_threads[i], NULL, texture_sampler_cache_async_thread, &thread_id);
    }

    /* Initialize SIMD context */
    g_sampler_cache_ctx.simd_context.avx2_supported = false; // Check CPU features
    g_sampler_cache_ctx.simd_context.sse4_supported = true; // Assume SSE4 support
    g_sampler_cache_ctx.simd_context.buffer_count = 16;
    g_sampler_cache_ctx.simd_context.simd_buffers = aligned_alloc(32, 16 * 4096);

    /* Initialize GPU context */
    g_sampler_cache_ctx.gpu_context.initialized = true; // Simulated GPU init

    /* Initialize render graph */
    g_sampler_cache_ctx.render_nodes = malloc(g_sampler_cache_ctx.capacity * sizeof(texture_render_graph_node_t));
    g_sampler_cache_ctx.render_node_count = 0;

    /* Initialize batch processing */
    g_sampler_cache_ctx.current_batch.operation_count = 0;
    g_sampler_cache_ctx.current_batch.handles = malloc(64 * sizeof(texture_sampler_cache_handle_t));
    g_sampler_cache_ctx.current_batch.data_buffers = malloc(64 * sizeof(void*));
    g_sampler_cache_ctx.current_batch.data_sizes = malloc(64 * sizeof(size_t));

    /* Set feature flags */
    g_sampler_cache_ctx.initialized = true;
    g_sampler_cache_ctx.count = 0;
    g_sampler_cache_ctx.cache_count = 0;
    g_sampler_cache_ctx.async_operation_count = 0;
    g_sampler_cache_ctx.watch_count = 0;

    memset(&g_sampler_cache_ctx.stats, 0, sizeof(texture_sampler_cache_stats_t));

    return 0;
}

void texture_sampler_cache_shutdown(void) {
    if (!g_sampler_cache_ctx.initialized) {
        return;
    }

    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    /* Cleanup all cache items */
    for (uint32_t i = 0; i < g_sampler_cache_ctx.count; i++) {
        texture_sampler_cache_cleanup_internal(&g_sampler_cache_ctx.items[i]);
    }

    /* Cleanup main cache */
    free(g_sampler_cache_ctx.items);
    g_sampler_cache_ctx.items = NULL;

    /* Cleanup caching layer */
    if (g_sampler_cache_ctx.cache_entries) {
        for (uint32_t i = 0; i < g_sampler_cache_ctx.cache_count; i++) {
            if (g_sampler_cache_ctx.cache_entries[i].cached_data) {
                free(g_sampler_cache_ctx.cache_entries[i].cached_data);
            }
        }
        free(g_sampler_cache_ctx.cache_entries);
        g_sampler_cache_ctx.cache_entries = NULL;
    }

    /* Cleanup hot-reload system */
    if (g_sampler_cache_ctx.hot_reload_running) {
        g_sampler_cache_ctx.hot_reload_running = false;
        pthread_join(g_sampler_cache_ctx.hot_reload_thread, NULL);
        if (g_sampler_cache_ctx.inotify_fd >= 0) {
            close(g_sampler_cache_ctx.inotify_fd);
        }
    }

    /* Cleanup async operations */
    for (uint32_t i = 0; i < TEXTURE_SAMPLER_CACHE_ASYNC_THREADS; i++) {
        pthread_mutex_destroy(&g_sampler_cache_ctx.async_operations[i].completion_mutex);
        pthread_cond_destroy(&g_sampler_cache_ctx.async_operations[i].completion_cond);
        pthread_join(g_sampler_cache_ctx.async_threads[i], NULL);
    }

    /* Cleanup SIMD context */
    if (g_sampler_cache_ctx.simd_context.simd_buffers) {
        free(g_sampler_cache_ctx.simd_context.simd_buffers);
    }

    /* Cleanup render graph */
    if (g_sampler_cache_ctx.render_nodes) {
        free(g_sampler_cache_ctx.render_nodes);
    }

    /* Cleanup batch processing */
    if (g_sampler_cache_ctx.current_batch.handles) {
        free(g_sampler_cache_ctx.current_batch.handles);
    }
    if (g_sampler_cache_ctx.current_batch.data_buffers) {
        free(g_sampler_cache_ctx.current_batch.data_buffers);
    }
    if (g_sampler_cache_ctx.current_batch.data_sizes) {
        free(g_sampler_cache_ctx.current_batch.data_sizes);
    }

    /* Reset state */
    g_sampler_cache_ctx.count = 0;
    g_sampler_cache_ctx.capacity = 0;
    g_sampler_cache_ctx.cache_count = 0;
    g_sampler_cache_ctx.cache_capacity = 0;
    g_sampler_cache_ctx.async_operation_count = 0;
    g_sampler_cache_ctx.watch_count = 0;
    g_sampler_cache_ctx.render_node_count = 0;
    g_sampler_cache_ctx.initialized = false;
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
    
    /* Destroy thread synchronization objects */
    pthread_mutex_destroy(&g_sampler_cache_ctx.global_mutex);
    pthread_rwlock_destroy(&g_sampler_cache_ctx.cache_lock);
    pthread_mutex_destroy(&g_sampler_cache_ctx.async_mutex);
    pthread_mutex_destroy(&g_sampler_cache_ctx.batch_mutex);
}

int texture_sampler_cache_create(texture_sampler_cache_handle_t* out_handle, const texture_sampler_cache_desc_t* desc) {
    if (!out_handle || !desc) {
        return TEXTURE_SAMPLER_CACHE_ERROR_INVALID_PARAM;
    }

    if (!g_sampler_cache_ctx.initialized) {
        return TEXTURE_SAMPLER_CACHE_ERROR_NOT_INITIALIZED;
    }

    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    if (g_sampler_cache_ctx.count >= g_sampler_cache_ctx.capacity) {
        pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
        return TEXTURE_SAMPLER_CACHE_ERROR_CAPACITY_EXCEEDED;
    }

    uint32_t index = g_sampler_cache_ctx.count++;
    texture_sampler_cache_internal_t* item = &g_sampler_cache_ctx.items[index];

    /* Initialize basic properties */
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->last_access_time = texture_sampler_cache_get_time_ms();
    item->access_count = 0;
    item->gpu_resident = false;
    item->gpu_handle = NULL;

    /* Initialize virtual texturing */
    item->virtual_tiles = NULL;
    item->virtual_tile_count = 0;
    texture_sampler_cache_init_virtual_tiles(item, 64); // Default 64 tiles

    /* Initialize compression */
    item->compression_format = TEXTURE_COMPRESSION_NONE;

    /* Initialize mipmaps */
    item->mipmaps = NULL;
    item->mipmap_count = 0;

    /* Initialize bindless texture */
    item->bindless_handle.gpu_handle = 0;
    item->bindless_handle.array_index = 0;
    item->bindless_handle.bound = false;
    texture_sampler_cache_init_bindless(item);

    /* Initialize texture array */
    item->texture_array = NULL;

    /* Initialize feedback analysis */
    memset(&item->feedback_data, 0, sizeof(texture_feedback_data_t));

    /* Initialize LOD support */
    item->lod_info.min_level = 0;
    item->lod_info.max_level = 8;
    item->lod_info.lod_bias = 0.0f;
    item->lod_info.anisotropic_enabled = true;
    item->lod_info.max_anisotropy = 16.0f;

    /* Initialize culling integration */
    memset(&item->culling_data, 0, sizeof(texture_culling_data_t));

    /* Initialize render graph node */
    memset(&item->render_node, 0, sizeof(texture_render_graph_node_t));
    texture_sampler_cache_add_render_node(item);

    /* Initialize cache entry */
    memset(&item->cache_entry, 0, sizeof(texture_cache_entry_t));
    item->cache_entry.handle.id = index;

    /* Update statistics */
    g_sampler_cache_ctx.stats.cache_misses++;
    g_sampler_cache_ctx.stats.total_requests++;
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);

    out_handle->id = index;
    return TEXTURE_SAMPLER_CACHE_ERROR_NONE;
}

void texture_sampler_cache_destroy(texture_sampler_cache_handle_t handle) {
    if (!g_sampler_cache_ctx.initialized) {
        return;
    }

    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    if (handle.id >= g_sampler_cache_ctx.count) {
        pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
        return;
    }

    texture_sampler_cache_cleanup_internal(&g_sampler_cache_ctx.items[handle.id]);
    g_sampler_cache_ctx.stats.evictions++;
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
}

int texture_sampler_cache_update(texture_sampler_cache_handle_t handle, const void* data, size_t size) {
    if (!g_sampler_cache_ctx.initialized) return TEXTURE_SAMPLER_CACHE_ERROR_NOT_INITIALIZED;
    if (!data && size > 0) return TEXTURE_SAMPLER_CACHE_ERROR_INVALID_PARAM;

    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    if (handle.id >= g_sampler_cache_ctx.count) {
        pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
        return TEXTURE_SAMPLER_CACHE_ERROR_INVALID_HANDLE;
    }

    texture_sampler_cache_internal_t* item = &g_sampler_cache_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
        return TEXTURE_SAMPLER_CACHE_ERROR_INVALID_HANDLE;
    }

    /* Update main data with SIMD optimization */
    if (item->data) free(item->data);
    item->data_size = size;
    
    if (size > 0) {
        item->data = malloc(size);
        if (!item->data) {
            pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
            return TEXTURE_SAMPLER_CACHE_ERROR_OUT_OF_MEMORY;
        }
        
        if (g_sampler_cache_ctx.simd_context.sse4_supported && size >= 16) {
            texture_sampler_cache_simd_process(data, item->data, size);
        } else {
            memcpy(item->data, data, size);
        }
    } else {
        item->data = NULL;
    }
    
    /* Generate mipmaps if needed */
    if (item->mipmap_count == 0 && size > 0) {
        texture_sampler_cache_generate_mipmaps(item, 256, 256); // Default size
    }
    
    /* Update feedback analysis */
    texture_sampler_cache_update_feedback(item);
    
    /* Mark as dirty and update stats */
    item->dirty = true;
    item->frame_updated++;
    item->last_access_time = texture_sampler_cache_get_time_ms();
    item->access_count++;
    
    /* GPU integration */
    if (g_sampler_cache_ctx.gpu_context.initialized && item->gpu_resident) {
        g_sampler_cache_ctx.stats.gpu_uploads++;
    }
    
    /* Update cache entry */
    if (item->cache_entry.cached_data) {
        free(item->cache_entry.cached_data);
        item->cache_entry.cached_data = NULL;
        item->cache_entry.cached_size = 0;
    }
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
    return TEXTURE_SAMPLER_CACHE_ERROR_NONE;
}

bool texture_sampler_cache_is_valid(texture_sampler_cache_handle_t handle) {
    if (!g_sampler_cache_ctx.initialized) {
        return false;
    }

    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    bool valid = false;
    if (handle.id < g_sampler_cache_ctx.count) {
        valid = texture_sampler_cache_validate(&g_sampler_cache_ctx.items[handle.id]);
        if (valid) {
            g_sampler_cache_ctx.stats.cache_hits++;
        } else {
            g_sampler_cache_ctx.stats.cache_misses++;
        }
        g_sampler_cache_ctx.stats.total_requests++;
    }
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
    return valid;
}

int texture_sampler_cache_get_info(texture_sampler_cache_handle_t handle, texture_sampler_cache_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (!g_sampler_cache_ctx.initialized) {
        return -2;
    }

    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    if (handle.id >= g_sampler_cache_ctx.count) {
        pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
        return -3;
    }

    const texture_sampler_cache_internal_t* item = &g_sampler_cache_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
    return 0;
}

void texture_sampler_cache_mark_dirty(texture_sampler_cache_handle_t handle) {
    if (!g_sampler_cache_ctx.initialized) {
        return;
    }

    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    if (handle.id < g_sampler_cache_ctx.count) {
        g_sampler_cache_ctx.items[handle.id].dirty = true;
        if (g_sampler_cache_ctx.culling_integration_enabled) {
            // Culling integration would go here
        }
    }
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
}

int texture_sampler_cache_process_pending(void) {
    if (!g_sampler_cache_ctx.initialized) return 0;

    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    int processed = 0;
    
    /* Batch processing */
    pthread_mutex_lock(&g_sampler_cache_ctx.batch_mutex);
    
    g_sampler_cache_ctx.current_batch.operation_count = 0;
    
    for (uint32_t i = 0; i < g_sampler_cache_ctx.count && processed < TEXTURE_SAMPLER_CACHE_BATCH_SIZE; i++) {
        texture_sampler_cache_internal_t* item = &g_sampler_cache_ctx.items[i];
        if (item->initialized && item->dirty) {
            /* Add to batch */
            g_sampler_cache_ctx.current_batch.handles[g_sampler_cache_ctx.current_batch.operation_count] = 
                (texture_sampler_cache_handle_t){item->id};
            g_sampler_cache_ctx.current_batch.data_buffers[g_sampler_cache_ctx.current_batch.operation_count] = item->data;
            g_sampler_cache_ctx.current_batch.data_sizes[g_sampler_cache_ctx.current_batch.operation_count] = item->data_size;
            g_sampler_cache_ctx.current_batch.operation_count++;
            
            /* Update culling and LOD */
            texture_sampler_cache_update_culling(item, 100.0f, 64); // Default values
            texture_sampler_cache_update_lod(item, 100.0f);
            
            /* Process item */
            item->dirty = false;
            item->render_node.processed = true;
            item->render_node.frame_processed = g_sampler_cache_ctx.stats.total_requests;
            processed++;
        }
    }
    
    if (processed > 0) {
        g_sampler_cache_ctx.stats.batch_operations++;
    }
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.batch_mutex);
    
    /* Render graph processing */
    if (g_sampler_cache_ctx.render_node_count > 0) {
        for (uint32_t i = 0; i < g_sampler_cache_ctx.render_node_count; i++) {
            texture_render_graph_node_t* node = &g_sampler_cache_ctx.render_nodes[i];
            if (!node->processed) {
                /* Process node dependencies first */
                bool dependencies_ready = true;
                for (uint32_t d = 0; d < node->dependency_count; d++) {
                    uint32_t dep_id = node->dependencies[d];
                    if (dep_id < g_sampler_cache_ctx.render_node_count) {
                        if (!g_sampler_cache_ctx.render_nodes[dep_id].processed) {
                            dependencies_ready = false;
                            break;
                        }
                    }
                }
                
                if (dependencies_ready) {
                    node->processed = true;
                    node->frame_processed = g_sampler_cache_ctx.stats.total_requests;
                    processed++;
                }
            }
        }
    }
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
    return processed;
}

uint32_t texture_sampler_cache_get_count(void) {
    return g_sampler_cache_ctx.count;
}

size_t texture_sampler_cache_get_memory_usage(void) {
    if (!g_sampler_cache_ctx.initialized) {
        return 0;
    }

    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    size_t total = sizeof(g_sampler_cache_ctx);
    total += g_sampler_cache_ctx.capacity * sizeof(texture_sampler_cache_internal_t);
    total += g_sampler_cache_ctx.cache_capacity * sizeof(texture_cache_entry_t);
    total += g_sampler_cache_ctx.render_node_count * sizeof(texture_render_graph_node_t);
    total += 64 * sizeof(texture_sampler_cache_handle_t); // Batch handles
    total += 64 * sizeof(void*); // Batch data buffers
    total += 64 * sizeof(size_t); // Batch data sizes

    for (uint32_t i = 0; i < g_sampler_cache_ctx.count; i++) {
        total += g_sampler_cache_ctx.items[i].data_size;
        total += g_sampler_cache_ctx.items[i].virtual_tile_count * sizeof(texture_virtual_tile_t);
        for (uint32_t j = 0; j < g_sampler_cache_ctx.items[i].mipmap_count; j++) {
            total += g_sampler_cache_ctx.items[i].mipmaps[j].data_size;
        }
    }
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
    return total;
}

void texture_sampler_cache_debug_print(void) {
    if (!g_sampler_cache_ctx.initialized) {
        printf("Sampler Cache: Not initialized\n");
        return;
    }

    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    printf("=== Sampler Cache Debug Info ===\n");
    printf("Initialized: %s\n", g_sampler_cache_ctx.initialized ? "Yes" : "No");
    printf("Count: %u / %u\n", g_sampler_cache_ctx.count, g_sampler_cache_ctx.capacity);
    printf("Cache Count: %u / %u\n", g_sampler_cache_ctx.cache_count, g_sampler_cache_ctx.cache_capacity);
    printf("Cache Memory: %zu / %zu bytes\n", g_sampler_cache_ctx.cache_memory_used, g_sampler_cache_ctx.cache_memory_budget);
    printf("Async Operations: %u\n", g_sampler_cache_ctx.async_operation_count);
    printf("File Watches: %u\n", g_sampler_cache_ctx.watch_count);
    printf("Render Nodes: %u\n", g_sampler_cache_ctx.render_node_count);
    
    printf("\n=== Feature Status ===\n");
    printf("Hot Reload: %s (running: %s)\n", 
           g_sampler_cache_ctx.inotify_fd >= 0 ? "Enabled" : "Disabled",
           g_sampler_cache_ctx.hot_reload_running ? "Yes" : "No");
    printf("GPU Integration: %s\n", g_sampler_cache_ctx.gpu_context.initialized ? "Enabled" : "Disabled");
    printf("SIMD SSE4: %s\n", g_sampler_cache_ctx.simd_context.sse4_supported ? "Enabled" : "Disabled");
    printf("SIMD AVX2: %s\n", g_sampler_cache_ctx.simd_context.avx2_supported ? "Enabled" : "Disabled");
    printf("Batch Processing: Enabled\n");
    printf("Culling Integration: Enabled\n");
    printf("Render Graph: Enabled\n");
    
    printf("\n=== Performance Statistics ===\n");
    printf("Cache Hits: %llu\n", (unsigned long long)g_sampler_cache_ctx.stats.cache_hits);
    printf("Cache Misses: %llu\n", (unsigned long long)g_sampler_cache_ctx.stats.cache_misses);
    printf("Total Requests: %llu\n", (unsigned long long)g_sampler_cache_ctx.stats.total_requests);
    printf("Validation Failures: %llu\n", (unsigned long long)g_sampler_cache_ctx.stats.validation_failures);
    printf("Async Operations: %llu\n", (unsigned long long)g_sampler_cache_ctx.stats.async_operations);
    printf("GPU Uploads: %llu\n", (unsigned long long)g_sampler_cache_ctx.stats.gpu_uploads);
    printf("SIMD Operations: %llu\n", (unsigned long long)g_sampler_cache_ctx.stats.simd_operations);
    printf("Batch Operations: %llu\n", (unsigned long long)g_sampler_cache_ctx.stats.batch_operations);
    printf("Hot Reload Events: %llu\n", (unsigned long long)g_sampler_cache_ctx.stats.hot_reload_events);
    printf("Serialization Ops: %llu\n", (unsigned long long)g_sampler_cache_ctx.stats.serialization_operations);
    printf("Total Validation Time: %.3f ms\n", g_sampler_cache_ctx.stats.total_validation_time);
    printf("Total Async Time: %.3f ms\n", g_sampler_cache_ctx.stats.total_async_time);
    printf("Total GPU Time: %.3f ms\n", g_sampler_cache_ctx.stats.total_gpu_time);
    
    printf("\n=== Cache Items (first 10) ===\n");
    for (uint32_t i = 0; i < g_sampler_cache_ctx.count && i < 10; i++) {
        const texture_sampler_cache_internal_t* item = &g_sampler_cache_ctx.items[i];
        printf("Item %u: ID=%u, Size=%zu, Dirty=%s, GPU=%s, Access=%u\n",
               i, item->id, item->data_size, item->dirty ? "Yes" : "No",
               item->gpu_resident ? "Yes" : "No", item->access_count);
        printf("  Virtual Tiles: %u, Mipmaps: %u, Compression: %d\n",
               item->virtual_tile_count, item->mipmap_count, item->compression_format);
        printf("  LOD: min=%u, max=%u, bias=%.2f, anisotropic=%s (%.1fx)\n",
               item->lod_info.min_level, item->lod_info.max_level, item->lod_info.lod_bias,
               item->lod_info.anisotropic_enabled ? "Yes" : "No", item->lod_info.max_anisotropy);
        printf("  Feedback: reads=%u, writes=%u, avg_mip=%.2f, aniso_ratio=%.2f\n",
               item->feedback_data.read_count, item->feedback_data.write_count,
               item->feedback_data.avg_mipmap_level, item->feedback_data.anisotropy_ratio);
        printf("  Culling: %s, distance=%.1f, screen_coverage=%u\n",
               item->culling_data.culled ? "Yes" : "No", 
               item->culling_data.distance_to_camera, item->culling_data.screen_coverage);
    }
    
    if (g_sampler_cache_ctx.count > 10) {
        printf("... and %u more items\n", g_sampler_cache_ctx.count - 10);
    }
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
}

/* Advanced API Functions */

int texture_sampler_cache_compress(texture_sampler_cache_handle_t handle, texture_compression_format_t format) {
    if (!g_sampler_cache_ctx.initialized) return TEXTURE_SAMPLER_CACHE_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    if (handle.id >= g_sampler_cache_ctx.count) {
        pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
        return TEXTURE_SAMPLER_CACHE_ERROR_INVALID_HANDLE;
    }
    
    texture_sampler_cache_internal_t* item = &g_sampler_cache_ctx.items[handle.id];
    if (!item->data || item->data_size == 0) {
        pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
        return TEXTURE_SAMPLER_CACHE_ERROR_INVALID_PARAM;
    }
    
    void* compressed_data = NULL;
    size_t compressed_size = 0;
    int result = texture_sampler_cache_compress_data(item->data, item->data_size, format, &compressed_data, &compressed_size);
    
    if (result == 0) {
        free(item->data);
        item->data = compressed_data;
        item->data_size = compressed_size;
        item->compression_format = format;
        item->dirty = true;
        g_sampler_cache_ctx.stats.serialization_operations++;
    }
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
    return result;
}

int texture_sampler_cache_add_file_watch(texture_sampler_cache_handle_t handle, const char* file_path) {
    if (!g_sampler_cache_ctx.initialized || !file_path) return TEXTURE_SAMPLER_CACHE_ERROR_INVALID_PARAM;
    if (g_sampler_cache_ctx.inotify_fd < 0) return TEXTURE_SAMPLER_CACHE_ERROR_ASYNC_FAILED;
    
    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    if (g_sampler_cache_ctx.watch_count >= TEXTURE_SAMPLER_CACHE_MAX_WATCHES) {
        pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
        return TEXTURE_SAMPLER_CACHE_ERROR_CAPACITY_EXCEEDED;
    }
    
    int wd = inotify_add_watch(g_sampler_cache_ctx.inotify_fd, file_path, IN_MODIFY | IN_CLOSE_WRITE);
    if (wd < 0) {
        pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
        return TEXTURE_SAMPLER_CACHE_ERROR_ASYNC_FAILED;
    }
    
    texture_file_watch_t* watch = &g_sampler_cache_ctx.file_watches[g_sampler_cache_ctx.watch_count++];
    watch->watch_descriptor = wd;
    strncpy(watch->file_path, file_path, sizeof(watch->file_path) - 1);
    watch->file_path[sizeof(watch->file_path) - 1] = '\0';
    watch->associated_handle = handle;
    watch->last_modified = texture_sampler_cache_get_time_ms();
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
    return TEXTURE_SAMPLER_CACHE_ERROR_NONE;
}

int texture_sampler_cache_set_anisotropy(texture_sampler_cache_handle_t handle, float max_anisotropy) {
    if (!g_sampler_cache_ctx.initialized) return TEXTURE_SAMPLER_CACHE_ERROR_NOT_INITIALIZED;
    
    pthread_mutex_lock(&g_sampler_cache_ctx.global_mutex);
    
    if (handle.id >= g_sampler_cache_ctx.count) {
        pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
        return TEXTURE_SAMPLER_CACHE_ERROR_INVALID_HANDLE;
    }
    
    texture_sampler_cache_internal_t* item = &g_sampler_cache_ctx.items[handle.id];
    item->lod_info.anisotropic_enabled = true;
    item->lod_info.max_anisotropy = max_anisotropy;
    item->dirty = true;
    
    pthread_mutex_unlock(&g_sampler_cache_ctx.global_mutex);
    return TEXTURE_SAMPLER_CACHE_ERROR_NONE;
}

/* End of sampler_cache.c */
