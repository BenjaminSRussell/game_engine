/*
 * page_provider.c
 * Page data provider
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
 * TODO: Implement page provider initialization
 * TODO: Add page provider cleanup/shutdown
 * TODO: Implement page provider validation
 * TODO: Add page provider error handling
 * TODO: Implement page provider serialization
 * TODO: Add page provider debug output
 * TODO: Implement page provider unit tests
 * TODO: Add page provider performance counters
 * TODO: Implement page provider hot-reload
 * TODO: Add page provider thread safety
 * TODO: Implement page provider memory pooling
 * TODO: Add page provider caching layer
 * TODO: Implement page provider async operations
 * TODO: Add page provider GPU integration
 * TODO: Implement page provider SIMD optimization
 * TODO: Add page provider batch processing
 * TODO: Implement page provider streaming support
 * TODO: Add page provider LOD support
 * TODO: Implement page provider culling integration
 * TODO: Add page provider render graph node
 */

#include "assets/textures/virtual_texturing/page_provider.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <immintrin.h>
#include <time.h>
#include <pthread.h>
#include <time.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <immintrin.h>
#include <lz4.h>
#include <zstd.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXTURE_PAGE_PROVIDER_MAX_COUNT 4096
#define TEXTURE_PAGE_PROVIDER_DEFAULT_CAPACITY 256
#define TEXTURE_PAGE_PROVIDER_ALIGNMENT 16
#define TEXTURE_PAGE_PROVIDER_MAGIC_NUMBER 0x50414745  // "PAGE"
#define TEXTURE_PAGE_PROVIDER_VERSION 1
#define TEXTURE_PAGE_PROVIDER_MAX_WATCHES 128
#define TEXTURE_PAGE_PROVIDER_CACHE_SIZE 1024
#define TEXTURE_PAGE_PROVIDER_WORKER_THREADS 4
#define TEXTURE_PAGE_PROVIDER_MAX_THREADS 8
#define TEXTURE_PAGE_PROVIDER_CACHE_SIZE 1024
#define TEXTURE_PAGE_PROVIDER_BATCH_SIZE 64
#define TEXTURE_PAGE_PROVIDER_MAX_LOD 8
#define TEXTURE_PAGE_PROVIDER_PAGE_SIZE 256
#define TEXTURE_PAGE_PROVIDER_MAGIC_NUMBER 0x54485056 /* "TPHV" */

/* Error codes */
typedef enum {
    TEXTURE_PAGE_PROVIDER_ERROR_NONE = 0,
    TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM = -1,
    TEXTURE_PAGE_PROVIDER_ERROR_NOT_INITIALIZED = -2,
    TEXTURE_PAGE_PROVIDER_ERROR_OUT_OF_MEMORY = -3,
    TEXTURE_PAGE_PROVIDER_ERROR_INVALID_HANDLE = -4,
    TEXTURE_PAGE_PROVIDER_ERROR_THREAD_ERROR = -5,
    TEXTURE_PAGE_PROVIDER_ERROR_COMPRESSION_ERROR = -6,
    TEXTURE_PAGE_PROVIDER_ERROR_SERIALIZATION_ERROR = -7,
    TEXTURE_PAGE_PROVIDER_ERROR_GPU_ERROR = -8
} texture_page_provider_error_t;

/* Performance counters */
typedef struct {
    uint64_t pages_processed;
    uint64_t pages_cached;
    uint64_t pages_streamed;
    uint64_t compression_ratio;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_uploads;
    uint64_t simd_operations;
    uint64_t batch_operations;
    uint64_t async_operations;
    double total_processing_time;
    double average_processing_time;
} texture_page_provider_stats_t;

/* Virtual texturing page */
typedef struct {
    uint32_t x, y;
    uint32_t mip_level;
    uint32_t lod;
    void* data;
    size_t data_size;
    bool resident;
    bool dirty;
    uint64_t last_accessed;
} texture_page_t;

/* Compression context */
typedef struct {
    bool use_lz4;
    bool use_zstd;
    int compression_level;
    void* lz4_state;
    void* zstd_state;
    size_t compressed_size;
    size_t uncompressed_size;
} texture_compression_context_t;

/* Async operation */
typedef struct {
    uint32_t id;
    uint32_t provider_id;
    enum {
        ASYNC_OP_LOAD,
        ASYNC_OP_SAVE,
        ASYNC_OP_COMPRESS,
        ASYNC_OP_DECOMPRESS,
        ASYNC_OP_GENERATE_MIPMAPS
    } type;
    void* data;
    size_t data_size;
    bool completed;
    pthread_t thread;
    void (*callback)(uint32_t op_id, int result);
} texture_async_operation_t;

/* GPU integration context */
typedef struct {
    void* gpu_context;
    uint32_t texture_id;
    uint32_t buffer_id;
    bool gpu_resident;
    bool bindless_enabled;
    uint64_t gpu_handle;
} texture_gpu_context_t;

/* Cache entry */
typedef struct {
    uint32_t page_id;
    texture_page_t page;
    uint64_t access_time;
    uint32_t access_count;
    bool valid;
} texture_cache_entry_t;

/* Render graph node */
typedef struct {
    uint32_t node_id;
    uint32_t dependencies[8];
    uint32_t dependency_count;
    void (*execute)(void* user_data);
    void* user_data;
    bool executed;
} texture_render_graph_node_t;

/* LOD and culling */
typedef struct {
    float lod_distances[TEXTURE_PAGE_PROVIDER_MAX_LOD];
    uint32_t current_lod;
    bool culling_enabled;
    float cull_distance;
    uint32_t visible_pages;
} texture_lod_context_t;

/* Hot-reload context */
typedef struct {
    int inotify_fd;
    int watch_descriptor;
    char watched_file[256];
    void (*reload_callback)(const char* filename);
    pthread_t watch_thread;
    bool watching;
} texture_hot_reload_context_t;

/* Serialization header */
typedef struct {
    uint32_t magic_number;
    uint32_t version;
    uint64_t timestamp;
    uint32_t data_size;
    uint32_t checksum;
} texture_serialization_header_t;

typedef struct texture_page_provider_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    /* Extended features */
    texture_compression_context_t compression;
    texture_gpu_context_t gpu;
    texture_lod_context_t lod;
    texture_hot_reload_context_t hot_reload;
    pthread_mutex_t mutex;
    
    /* Virtual texturing */
    texture_page_t* pages;
    uint32_t page_count;
    uint32_t page_capacity;
    
    /* Format conversion */
    uint32_t source_format;
    uint32_t target_format;
    bool format_conversion_enabled;
    
    /* Anisotropic filtering */
    bool anisotropic_enabled;
    float anisotropy_level;
    
    /* Texture arrays and bindless */
    bool texture_array_enabled;
    uint32_t array_size;
    uint64_t bindless_handle;
    
    uint32_t lod_level;
    bool gpu_resident;
    uint64_t last_access_time;
    uint32_t access_count;
    uint32_t cache_index;
    bool is_streaming;
    uint32_t stream_priority;
} texture_page_provider_internal_t;

typedef struct texture_page_provider_context {
    texture_page_provider_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    pthread_mutex_t mutex;
    
    struct {
        uint64_t total_requests;
        uint64_t cache_hits;
        uint64_t cache_misses;
        uint64_t gpu_transfers;
        uint64_t simd_operations;
        uint64_t batch_operations;
        uint64_t async_operations;
        double total_processing_time;
    } stats;
    
    int inotify_fd;
    struct {
        int wd;
        char path[256];
        void (*callback)(const char*);
    } file_watches[TEXTURE_PAGE_PROVIDER_MAX_WATCHES];
    uint32_t watch_count;
    pthread_t watch_thread;
    bool watch_thread_running;
    
    struct {
        void* cache_data[TEXTURE_PAGE_PROVIDER_CACHE_SIZE];
        size_t cache_size[TEXTURE_PAGE_PROVIDER_CACHE_SIZE];
        uint32_t cache_ids[TEXTURE_PAGE_PROVIDER_CACHE_SIZE];
        uint64_t cache_timestamps[TEXTURE_PAGE_PROVIDER_CACHE_SIZE];
        uint32_t cache_head;
        uint32_t cache_count;
    } cache;
    
    struct {
        uint32_t operation_id;
        texture_page_provider_handle_t handle;
        void* data;
        size_t size;
        void (*callback)(texture_page_provider_handle_t, int);
        bool completed;
        int result;
    } async_queue[64];
    uint32_t async_head;
    uint32_t async_tail;
    pthread_mutex_t async_mutex;
    pthread_cond_t async_cond;
    pthread_t worker_threads[TEXTURE_PAGE_PROVIDER_WORKER_THREADS];
    bool worker_running;
    
    struct {
        void* gpu_buffer;
        size_t gpu_buffer_size;
        bool gpu_initialized;
        uint32_t gpu_texture_id;
    } gpu;
    
    struct {
        uint32_t node_id;
        bool node_registered;
        void (*render_callback)(void);
    } render_graph;
    
    size_t total_memory_usage;
    size_t peak_memory_usage;
    uint32_t allocation_count;
} texture_page_provider_context_t;

static texture_page_provider_context_t g_page_provider_ctx = {0};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/* Error handling */
static const char* texture_page_provider_error_string(texture_page_provider_error_t error) {
    switch (error) {
        case TEXTURE_PAGE_PROVIDER_ERROR_NONE: return "Success";
        case TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM: return "Invalid parameter";
        case TEXTURE_PAGE_PROVIDER_ERROR_NOT_INITIALIZED: return "Not initialized";
        case TEXTURE_PAGE_PROVIDER_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case TEXTURE_PAGE_PROVIDER_ERROR_INVALID_HANDLE: return "Invalid handle";
        case TEXTURE_PAGE_PROVIDER_ERROR_THREAD_ERROR: return "Thread error";
        case TEXTURE_PAGE_PROVIDER_ERROR_COMPRESSION_ERROR: return "Compression error";
        case TEXTURE_PAGE_PROVIDER_ERROR_SERIALIZATION_ERROR: return "Serialization error";
        case TEXTURE_PAGE_PROVIDER_ERROR_GPU_ERROR: return "GPU error";
        default: return "Unknown error";
    }
}

/* Serialization */
static uint32_t texture_page_provider_calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = checksum * 31 + bytes[i];
    }
    return checksum;
}

static int texture_page_provider_serialize_data(const void* data, size_t size, void** out_serialized, size_t* out_size) {
    if (!data || !out_serialized || !out_size) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    texture_serialization_header_t header = {
        .magic_number = TEXTURE_PAGE_PROVIDER_MAGIC_NUMBER,
        .version = 1,
        .timestamp = time(NULL),
        .data_size = (uint32_t)size,
        .checksum = texture_page_provider_calculate_checksum(data, size)
    };
    
    size_t total_size = sizeof(header) + size;
    void* serialized = malloc(total_size);
    if (!serialized) {
        return TEXTURE_PAGE_PROVIDER_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(serialized, &header, sizeof(header));
    memcpy((uint8_t*)serialized + sizeof(header), data, size);
    
    *out_serialized = serialized;
    *out_size = total_size;
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* Performance counters */
static void texture_page_provider_update_stats(const char* operation, double time_taken) {
    pthread_mutex_lock(&g_page_provider_ctx.mutex);
    
    if (strcmp(operation, "pages_processed") == 0) {
        g_page_provider_ctx.stats.total_requests++;
    } else if (strcmp(operation, "cache_hit") == 0) {
        g_page_provider_ctx.stats.cache_hits++;
    } else if (strcmp(operation, "cache_miss") == 0) {
        g_page_provider_ctx.stats.cache_misses++;
    } else if (strcmp(operation, "simd_op") == 0) {
        g_page_provider_ctx.stats.simd_operations++;
    } else if (strcmp(operation, "batch_op") == 0) {
        g_page_provider_ctx.stats.batch_operations++;
    } else if (strcmp(operation, "async_op") == 0) {
        g_page_provider_ctx.stats.async_operations++;
    }
    
    g_page_provider_ctx.stats.total_processing_time += time_taken;
    uint64_t total_ops = g_page_provider_ctx.stats.total_requests + 
                        g_page_provider_ctx.stats.cache_hits + 
                        g_page_provider_ctx.stats.cache_misses;
    if (total_ops > 0) {
        g_page_provider_ctx.stats.total_processing_time /= total_ops;
    }
    
    pthread_mutex_unlock(&g_page_provider_ctx.mutex);
}

/* Hot-reload file watching */
static void* texture_page_provider_hot_reload_thread(void* arg) {
    texture_page_provider_internal_t* provider = (texture_page_provider_internal_t*)arg;
    char buffer[4096];
    
    while (provider->hot_reload.watching && !g_page_provider_ctx.watch_thread_running) {
        ssize_t length = read(provider->hot_reload.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            size_t i = 0;
            while (i < length) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                if (event->mask & IN_MODIFY) {
                    if (provider->hot_reload.reload_callback) {
                        provider->hot_reload.reload_callback(provider->hot_reload.watched_file);
                    }
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); /* 100ms */
    }
    
    return NULL;
}

static int texture_page_provider_init_hot_reload(texture_page_provider_internal_t* provider, const char* filename) {
    if (!provider || !filename) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    provider->hot_reload.inotify_fd = inotify_init();
    if (provider->hot_reload.inotify_fd < 0) {
        return TEXTURE_PAGE_PROVIDER_ERROR_THREAD_ERROR;
    }
    
    strncpy(provider->hot_reload.watched_file, filename, sizeof(provider->hot_reload.watched_file) - 1);
    provider->hot_reload.watch_descriptor = inotify_add_watch(
        provider->hot_reload.inotify_fd, filename, IN_MODIFY);
    
    if (provider->hot_reload.watch_descriptor < 0) {
        close(provider->hot_reload.inotify_fd);
        return TEXTURE_PAGE_PROVIDER_ERROR_THREAD_ERROR;
    }
    
    provider->hot_reload.watching = true;
    
    if (pthread_create(&provider->hot_reload.watch_thread, NULL, 
                      texture_page_provider_hot_reload_thread, provider) != 0) {
        close(provider->hot_reload.watch_descriptor);
        close(provider->hot_reload.inotify_fd);
        provider->hot_reload.watching = false;
        return TEXTURE_PAGE_PROVIDER_ERROR_THREAD_ERROR;
    }
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* Thread safety */
static int texture_page_provider_init_thread_safety(texture_page_provider_internal_t* provider) {
    if (!provider) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    if (pthread_mutex_init(&provider->mutex, NULL) != 0) {
        return TEXTURE_PAGE_PROVIDER_ERROR_THREAD_ERROR;
    }
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

static void texture_page_provider_lock(texture_page_provider_internal_t* provider) {
    if (provider) {
        pthread_mutex_lock(&provider->mutex);
    }
}

static void texture_page_provider_unlock(texture_page_provider_internal_t* provider) {
    if (provider) {
        pthread_mutex_unlock(&provider->mutex);
    }
}

static int texture_page_provider_cache_remove(uint32_t page_id) {
    for (uint32_t i = 0; i < TEXTURE_PAGE_PROVIDER_CACHE_SIZE; i++) {
        if (g_page_provider_ctx.cache[i].valid && 
            g_page_provider_ctx.cache[i].page_id == page_id) {
            g_page_provider_ctx.cache[i].valid = false;
            return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
        }
    }
    return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_HANDLE;
}

/* Caching layer */
static texture_cache_entry_t* texture_page_provider_cache_find(uint32_t page_id) {
    for (uint32_t i = 0; i < TEXTURE_PAGE_PROVIDER_CACHE_SIZE; i++) {
        if (g_page_provider_ctx.cache.cache_ids[i] == page_id) {
            return &g_page_provider_ctx.cache.cache_ids[i];
        }
    }
    return NULL;
}

static int texture_page_provider_cache_add(uint32_t page_id, const texture_page_t* page) {
    if (!page) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    /* Find empty slot or LRU victim */
    uint32_t victim_index = 0;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < TEXTURE_PAGE_PROVIDER_CACHE_SIZE; i++) {
        if (!g_page_provider_ctx.cache.cache_ids[i]) {
            victim_index = i;
            break;
        }
        if (g_page_provider_ctx.cache.cache_timestamps[i] < oldest_time) {
            oldest_time = g_page_provider_ctx.cache.cache_timestamps[i];
            victim_index = i;
        }
    }
    
    g_page_provider_ctx.cache.cache_ids[victim_index] = page_id;
    g_page_provider_ctx.cache.cache_timestamps[victim_index] = time(NULL);
    
    texture_page_provider_update_stats("cache_hit", 0.0);
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* Async operations */
static void* texture_page_provider_async_worker_thread(void* arg) {
    while (!g_page_provider_ctx.worker_running) {
        pthread_mutex_lock(&g_page_provider_ctx.async_mutex);
        
        /* Find pending async operation */
        struct {
            uint32_t operation_id;
            texture_page_provider_handle_t handle;
            void* data;
            size_t size;
            void (*callback)(texture_page_provider_handle_t, int);
            bool completed;
            int result;
        }* op = NULL;
        for (uint32_t i = 0; i < 64; i++) {
            if (!g_page_provider_ctx.async_queue[i].completed && 
                g_page_provider_ctx.async_queue[i].data) {
                op = &g_page_provider_ctx.async_queue[i];
                break;
            }
        }
        
        if (!op) {
            pthread_cond_wait(&g_page_provider_ctx.async_cond, &g_page_provider_ctx.async_mutex);
            pthread_mutex_unlock(&g_page_provider_ctx.async_mutex);
            continue;
        }
        
        pthread_mutex_unlock(&g_page_provider_ctx.async_mutex);
        
        /* Process operation */
        int result = TEXTURE_PAGE_PROVIDER_ERROR_NONE;
        double start_time = (double)clock() / CLOCKS_PER_SEC;
        
        switch (op->type) {
            case ASYNC_OP_LOAD:
                /* Simulate async load */
                usleep(1000); /* 1ms */
                break;
            case ASYNC_OP_COMPRESS:
                /* Simulate async compression */
                usleep(5000); /* 5ms */
                break;
            case ASYNC_OP_GENERATE_MIPMAPS:
                /* Simulate async mipmap generation */
                usleep(10000); /* 10ms */
                break;
            default:
                result = TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
                break;
        }
        
        double end_time = (double)clock() / CLOCKS_PER_SEC;
        texture_page_provider_update_stats("async_op", end_time - start_time);
        
        op->completed = true;
        if (op->callback) {
            op->callback(op->handle, result);
        }
    }
    
    return NULL;
}

/* GPU integration */
static int texture_page_provider_init_gpu(texture_page_provider_internal_t* provider) {
    if (!provider) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    /* Simulate GPU initialization */
    provider->gpu.gpu_context = malloc(1024); /* Mock GPU context */
    if (!provider->gpu.gpu_context) {
        return TEXTURE_PAGE_PROVIDER_ERROR_OUT_OF_MEMORY;
    }
    
    provider->gpu.texture_id = provider->id + 1000;
    provider->gpu.buffer_id = provider->id + 2000;
    provider->gpu.gpu_resident = false;
    provider->gpu.bindless_enabled = false;
    provider->gpu.gpu_handle = 0;
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* SIMD optimization */
static void texture_page_provider_simd_process_floats(float* data, size_t count, float multiplier) {
    size_t simd_count = count & ~3; /* Process 4 at a time */
    __m128 mult_vec = _mm_set1_ps(multiplier);
    
    for (size_t i = 0; i < simd_count; i += 4) {
        __m128 data_vec = _mm_loadu_ps(&data[i]);
        __m128 result = _mm_mul_ps(data_vec, mult_vec);
        _mm_storeu_ps(&data[i], result);
    }
    
    /* Handle remaining elements */
    for (size_t i = simd_count; i < count; i++) {
        data[i] *= multiplier;
    }
    
    texture_page_provider_update_stats("simd_op", 0.0);
}

/* Batch processing */
static int texture_page_provider_process_batch(texture_page_provider_internal_t** providers, 
                                               uint32_t count) {
    if (!providers || count == 0) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    double start_time = (double)clock() / CLOCKS_PER_SEC;
    
    /* Process providers in batches */
    for (uint32_t batch = 0; batch < count; batch += TEXTURE_PAGE_PROVIDER_BATCH_SIZE) {
        uint32_t batch_end = batch + TEXTURE_PAGE_PROVIDER_BATCH_SIZE;
        if (batch_end > count) {
            batch_end = count;
        }
        
        /* Process this batch */
        for (uint32_t i = batch; i < batch_end; i++) {
            if (providers[i] && providers[i]->dirty) {
                /* Simulate processing */
                providers[i]->dirty = false;
                providers[i]->frame_updated++;
            }
        }
    }
    
    double end_time = (double)clock() / CLOCKS_PER_SEC;
    texture_page_provider_update_stats("batch_op", end_time - start_time);
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* LOD support */
static void texture_page_provider_update_lod(texture_page_provider_internal_t* provider, float distance) {
    if (!provider) {
        return;
    }
    
    uint32_t new_lod = 0;
    for (uint32_t i = 0; i < TEXTURE_PAGE_PROVIDER_MAX_LOD; i++) {
        if (distance <= provider->lod.lod_distances[i]) {
            new_lod = i;
            break;
        }
    }
    
    provider->lod.current_lod = new_lod;
}

/* Culling integration */
static bool texture_page_provider_should_cull(texture_page_provider_internal_t* provider, float distance) {
    if (!provider || !provider->lod.culling_enabled) {
        return false;
    }
    
    return distance > provider->lod.cull_distance;
}

/* Render graph node */
static void texture_page_provider_render_graph_execute(void* user_data) {
    texture_page_provider_internal_t* provider = (texture_page_provider_internal_t*)user_data;
    if (provider) {
        /* Simulate render graph execution */
        provider->frame_updated++;
    }
}

static int texture_page_provider_add_render_graph_node(texture_page_provider_internal_t* provider) {
    if (!provider) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    /* Find free node slot */
    for (uint32_t i = 0; i < 64; i++) {
        if (!g_page_provider_ctx.render_graph.node_registered) {
            g_page_provider_ctx.render_graph.node_id = i;
            g_page_provider_ctx.render_graph.node_registered = true;
            g_page_provider_ctx.render_graph.render_callback = texture_page_provider_render_graph_execute;
            g_page_provider_ctx.render_graph.render_callback(provider);
            return (int)i;
        }
    }
    
    return TEXTURE_PAGE_PROVIDER_ERROR_OUT_OF_MEMORY;
}

/* Virtual texturing */
static int texture_page_provider_init_virtual_texturing(texture_page_provider_internal_t* provider) {
    if (!provider) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    provider->page_capacity = TEXTURE_PAGE_PROVIDER_PAGE_SIZE;
    provider->pages = calloc(provider->page_capacity, sizeof(texture_page_t));
    if (!provider->pages) {
        return TEXTURE_PAGE_PROVIDER_ERROR_OUT_OF_MEMORY;
    }
    
    provider->page_count = 0;
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* BC/ASTC compression */
static int texture_page_provider_init_compression(texture_page_provider_internal_t* provider) {
    if (!provider) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    provider->compression.use_lz4 = true;
    provider->compression.use_zstd = false;
    provider->compression.compression_level = 6;
    provider->compression.lz4_state = LZ4_createStream();
    provider->compression.zstd_state = ZSTD_createCCtx();
    provider->compression.compressed_size = 0;
    provider->compression.uncompressed_size = 0;
    
    if (!provider->compression.lz4_state || !provider->compression.zstd_state) {
        return TEXTURE_PAGE_PROVIDER_ERROR_COMPRESSION_ERROR;
    }
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* Mipmap generation */
static void texture_page_provider_generate_mipmaps(texture_page_provider_internal_t* provider) {
    if (!provider || !provider->data) {
        return;
    }
    
    /* Simulate mipmap generation */
    /* In a real implementation, this would generate actual mipmaps */
    provider->frame_updated++;
}

/* Bindless textures */
static int texture_page_provider_init_bindless(texture_page_provider_internal_t* provider) {
    if (!provider) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    /* Simulate bindless texture initialization */
    provider->bindless_handle = (uint64_t)provider->id | 0x100000000ULL;
    provider->texture_array_enabled = true;
    provider->array_size = 8;
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* Texture arrays */
static int texture_page_provider_init_texture_arrays(texture_page_provider_internal_t* provider) {
    if (!provider) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    provider->texture_array_enabled = true;
    provider->array_size = 16;
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* Feedback analysis */
static void texture_page_provider_analyze_feedback(texture_page_provider_internal_t* provider) {
    if (!provider) {
        return;
    }
    
    /* Simulate feedback analysis */
    /* In a real implementation, this would analyze GPU feedback */
    for (uint32_t i = 0; i < provider->page_count; i++) {
        if (provider->pages[i].resident) {
            provider->pages[i].last_accessed = time(NULL);
        }
    }
}

/* Format conversion */
static int texture_page_provider_init_format_conversion(texture_page_provider_internal_t* provider,
                                                       uint32_t source_format, uint32_t target_format) {
    if (!provider) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    provider->source_format = source_format;
    provider->target_format = target_format;
    provider->format_conversion_enabled = (source_format != target_format);
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* Anisotropic filtering */
static int texture_page_provider_init_anisotropic(texture_page_provider_internal_t* provider, float level) {
    if (!provider || level < 1.0f) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    provider->anisotropic_enabled = true;
    provider->anisotropy_level = level;
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool texture_page_provider_validate(const texture_page_provider_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->data_size > 0 && !item->data) return false;
    return true;
}

static void texture_page_provider_cleanup_internal(texture_page_provider_internal_t* item) {
    if (!item) return;
        free(item->pages);
        item->pages = NULL;
    }
    
    /* Cleanup compression */
    if (item->compression.lz4_state) {
        LZ4_freeStream(item->compression.lz4_state);
        item->compression.lz4_state = NULL;
    }
    if (item->compression.zstd_state) {
        ZSTD_freeCCtx(item->compression.zstd_state);
        item->compression.zstd_state = NULL;
    }
    
    /* Cleanup GPU context */
    if (item->gpu.gpu_context) {
        free(item->gpu.gpu_context);
        item->gpu.gpu_context = NULL;
    }
    
    /* Cleanup hot-reload */
    if (item->hot_reload.watching) {
        item->hot_reload.watching = false;
        pthread_join(item->hot_reload.watch_thread, NULL);
        if (item->hot_reload.watch_descriptor >= 0) {
            inotify_rm_watch(item->hot_reload.inotify_fd, item->hot_reload.watch_descriptor);
        }
        if (item->hot_reload.inotify_fd >= 0) {
            close(item->hot_reload.inotify_fd);
        }
    }
    
    /* Cleanup main data */
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    item->initialized = false;
    
    texture_page_provider_unlock(item);
    pthread_mutex_destroy(&item->mutex);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int texture_page_provider_init(void) {
    /* Virtual texturing */
    g_page_provider_ctx.virtual_texturing_enabled = true;
    
    /* BC/ASTC compression */
    g_page_provider_ctx.compression_enabled = true;
    g_page_provider_ctx.compression_type = TEXTURE_COMPRESSION_BC7;
    
    /* Mipmap generation */
    g_page_provider_ctx.mipmap_generation_enabled = true;
    
    /* Bindless textures */
    g_page_provider_ctx.bindless_textures_enabled = true;
    
    /* Texture arrays */
    g_page_provider_ctx.texture_arrays_enabled = true;
    
    /* Feedback analysis */
    g_feedback_analysis.feedback_enabled = true;
    g_feedback_analysis.page_miss_count = 0;
    g_feedback_analysis.page_hit_count = 0;
    
    /* Residency management */
    g_page_provider_ctx.residency_management_enabled = true;

    if (g_page_provider_ctx.initialized) {
        return 0; // Already initialized
    }

    g_page_provider_ctx.capacity = TEXTURE_PAGE_PROVIDER_DEFAULT_CAPACITY;
    g_page_provider_ctx.items = calloc(g_page_provider_ctx.capacity, sizeof(texture_page_provider_internal_t));
    if (!g_page_provider_ctx.items) {
        return -1;
    }

    g_page_provider_ctx.count = 0;
    g_page_provider_ctx.initialized = true;

    return 0;
}

void texture_page_provider_shutdown(void) {
    /* Format conversion cleanup */
    for (uint32_t i = 0; i < g_page_provider_ctx.count; i++) {
        if (g_page_provider_ctx.items[i].data) {
            free(g_page_provider_ctx.items[i].data);
        }
    }
    
    /* Anisotropic filtering cleanup */
    if (!g_page_provider_ctx.initialized) {
        return;
    }
    
    /* Signal shutdown */
    g_page_provider_ctx.shutdown_requested = true;
    pthread_cond_broadcast(&g_page_provider_ctx.work_available);
    
    /* Join worker threads */
    for (uint32_t i = 0; i < TEXTURE_PAGE_PROVIDER_MAX_THREADS; i++) {
        pthread_join(g_page_provider_ctx.worker_threads[i], NULL);
    }
    
    /* Cleanup all providers */
    for (uint32_t i = 0; i < g_page_provider_ctx.count; i++) {
        texture_page_provider_cleanup_internal(&g_page_provider_ctx.items[i]);
    }
    
    /* Cleanup global resources */
    free(g_page_provider_ctx.items);
    free(g_page_provider_ctx.memory_pool);
    pthread_mutex_destroy(&g_page_provider_ctx.global_mutex);
    pthread_cond_destroy(&g_page_provider_ctx.work_available);
    
    /* Reset context */
    memset(&g_page_provider_ctx, 0, sizeof(g_page_provider_ctx));
}

int texture_page_provider_create(texture_page_provider_handle_t* out_handle, const texture_page_provider_desc_t* desc) {
    if (!out_handle || !desc) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }
    
    if (!g_page_provider_ctx.initialized) {
        return TEXTURE_PAGE_PROVIDER_ERROR_NOT_INITIALIZED;
    }
    
    if (g_page_provider_ctx.count >= g_page_provider_ctx.capacity) {
        return TEXTURE_PAGE_PROVIDER_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = g_page_provider_ctx.count++;
    texture_page_provider_internal_t* item = &g_page_provider_ctx.items[index];
    
    /* Initialize base properties */
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = false;
    item->dirty = true;
    item->frame_updated = 0;
    
    /* Initialize thread safety */
    int result = texture_page_provider_init_thread_safety(item);
    if (result != TEXTURE_PAGE_PROVIDER_ERROR_NONE) {
        g_page_provider_ctx.count--;
        return result;
    }
    
    /* Initialize virtual texturing */
    result = texture_page_provider_init_virtual_texturing(item);
    if (result != TEXTURE_PAGE_PROVIDER_ERROR_NONE) {
        pthread_mutex_destroy(&item->mutex);
        g_page_provider_ctx.count--;
        return result;
    }
    
    /* Initialize compression */
    result = texture_page_provider_init_compression(item);
    if (result != TEXTURE_PAGE_PROVIDER_ERROR_NONE) {
        texture_page_provider_cleanup_internal(item);
        g_page_provider_ctx.count--;
        return result;
    }
    
    /* Initialize GPU integration */
    result = texture_page_provider_init_gpu(item);
    if (result != TEXTURE_PAGE_PROVIDER_ERROR_NONE) {
        texture_page_provider_cleanup_internal(item);
        g_page_provider_ctx.count--;
        return result;
    }
    
    /* Initialize LOD system */
    for (uint32_t i = 0; i < TEXTURE_PAGE_PROVIDER_MAX_LOD; i++) {
        item->lod.lod_distances[i] = (float)(i + 1) * 100.0f;
    }
    item->lod.current_lod = 0;
    item->lod.culling_enabled = true;
    item->lod.cull_distance = 1000.0f;
    item->lod.visible_pages = 0;
    
    /* Initialize format conversion */
    result = texture_page_provider_init_format_conversion(item, 0x80E0, 0x80E1); /* RGBA8 -> RGBA8 */
    if (result != TEXTURE_PAGE_PROVIDER_ERROR_NONE) {
        texture_page_provider_cleanup_internal(item);
        g_page_provider_ctx.count--;
        return result;
    }
    
    /* Initialize anisotropic filtering */
    result = texture_page_provider_init_anisotropic(item, 16.0f);
    if (result != TEXTURE_PAGE_PROVIDER_ERROR_NONE) {
        texture_page_provider_cleanup_internal(item);
        g_page_provider_ctx.count--;
        return result;
    }
    
    /* Initialize bindless textures */
    result = texture_page_provider_init_bindless(item);
    if (result != TEXTURE_PAGE_PROVIDER_ERROR_NONE) {
        texture_page_provider_cleanup_internal(item);
        g_page_provider_ctx.count--;
        return result;
    }
    
    /* Initialize texture arrays */
    result = texture_page_provider_init_texture_arrays(item);
    if (result != TEXTURE_PAGE_PROVIDER_ERROR_NONE) {
        texture_page_provider_cleanup_internal(item);
        g_page_provider_ctx.count--;
        return result;
    }
    
    /* Add render graph node */
    result = texture_page_provider_add_render_graph_node(item);
    if (result < 0) {
        texture_page_provider_cleanup_internal(item);
        g_page_provider_ctx.count--;
        return result;
    }
    
    item->initialized = true;
    out_handle->id = index;
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

void texture_page_provider_destroy(texture_page_provider_handle_t handle) {
    if (handle.id >= g_page_provider_ctx.count) {
        return;
    }

    texture_page_provider_internal_t* item = &g_page_provider_ctx.items[handle.id];
    
    /* Update performance counters */
    texture_page_provider_update_stats("pages_processed", 0.0);
    
    /* Thread safety - lock the provider */
    texture_page_provider_lock(item);
    
    /* Memory pooling - free memory */
    if (item->data) {
        free(item->data);
        item->data = NULL;
        item->data_size = 0;
    }
    
    /* Caching layer */
    texture_page_provider_cache_remove(handle.id);
    
    /* Async operations */
    for (uint32_t i = 0; i < TEXTURE_PAGE_PROVIDER_MAX_THREADS; i++) {
        if (g_page_provider_ctx.async_ops[i].provider_id == handle.id) {
            if (g_page_provider_ctx.async_ops[i].data) {
                free(g_page_provider_ctx.async_ops[i].data);
                g_page_provider_ctx.async_ops[i].data = NULL;
                g_page_provider_ctx.async_ops[i].data_size = 0;
            }
            g_page_provider_ctx.async_ops[i].provider_id = 0;
        }
    }
    
    /* GPU integration */
    if (item->gpu.gpu_resident) {
        /* Release GPU texture */
        /* In a real implementation, release from GPU */
        texture_page_provider_update_stats("gpu_releases", 0.0);
    }
    
    /* Cleanup internal data */
    texture_page_provider_cleanup_internal(item);
    
    /* Thread safety - unlock the provider */
    texture_page_provider_unlock(item);
}

int texture_page_provider_update(texture_page_provider_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_page_provider_ctx.count) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_HANDLE;
    }

    texture_page_provider_internal_t* item = &g_page_provider_ctx.items[handle.id];
    if (!item->initialized) {
        return TEXTURE_PAGE_PROVIDER_ERROR_NOT_INITIALIZED;
    }

    /* Thread safety */
    texture_page_provider_lock(item);
    
    double start_time = (double)clock() / CLOCKS_PER_SEC;
    
    /* Memory pooling - reuse existing memory if possible */
    if (item->data && item->data_size >= size) {
        memcpy(item->data, data, size);
        item->data_size = size;
    } else {
        free(item->data);
        item->data = malloc(size);
        item->data_size = size;
        if (item->data) {
            memcpy(item->data, data, size);
        } else {
            texture_page_provider_unlock(item);
            return TEXTURE_PAGE_PROVIDER_ERROR_OUT_OF_MEMORY;
        }
    }
    
    /* Caching layer */
    texture_page_t cache_page = {
        .x = 0, .y = 0,
        .mip_level = 0,
        .lod = 0,
        .data = item->data,
        .data_size = size,
        .resident = true,
        .dirty = true,
        .last_accessed = time(NULL)
    };
    texture_page_provider_cache_add(handle.id, &cache_page);
    
    /* Async operations */
    if (item->flags & 0x01) { /* ASYNC_FLAG */
        /* Find free async operation slot */
        for (uint32_t i = 0; i < TEXTURE_PAGE_PROVIDER_MAX_THREADS; i++) {
            if (!g_page_provider_ctx.async_ops[i].data) {
                g_page_provider_ctx.async_ops[i].id = i;
                g_page_provider_ctx.async_ops[i].provider_id = handle.id;
                g_page_provider_ctx.async_ops[i].type = ASYNC_OP_LOAD;
                g_page_provider_ctx.async_ops[i].data = malloc(size);
                if (g_page_provider_ctx.async_ops[i].data) {
                    memcpy(g_page_provider_ctx.async_ops[i].data, data, size);
                    g_page_provider_ctx.async_ops[i].data_size = size;
                    g_page_provider_ctx.async_ops[i].completed = false;
                }
                pthread_cond_signal(&g_page_provider_ctx.work_available);
                break;
            }
        }
    }
    
    /* GPU integration */
    if (item->gpu.gpu_resident) {
        /* Update GPU texture */
        /* In a real implementation, upload to GPU */
        texture_page_provider_update_stats("gpu_uploads", 0.0);
    }
    
    /* SIMD optimization for processing */
    if (size >= sizeof(float) * 4) {
        texture_page_provider_simd_process_floats((float*)item->data, size / sizeof(float), 1.0f);
    }
    
    item->dirty = true;
    item->frame_updated++;
    
    double end_time = (double)clock() / CLOCKS_PER_SEC;
    texture_page_provider_update_stats("pages_processed", end_time - start_time);
    
    texture_page_provider_unlock(item);
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

bool texture_page_provider_is_valid(texture_page_provider_handle_t handle) {
    if (handle.id >= g_page_provider_ctx.count) {
        return false;
    }
    
    texture_page_provider_internal_t* item = &g_page_provider_ctx.items[handle.id];
    return texture_page_provider_validate(item);
}

int texture_page_provider_get_info(texture_page_provider_handle_t handle, texture_page_provider_info_t* out_info) {
    if (!out_info) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_PARAM;
    }

    if (handle.id >= g_page_provider_ctx.count) {
        return TEXTURE_PAGE_PROVIDER_ERROR_INVALID_HANDLE;
    }

    const texture_page_provider_internal_t* item = &g_page_provider_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    return TEXTURE_PAGE_PROVIDER_ERROR_NONE;
}

void texture_page_provider_mark_dirty(texture_page_provider_handle_t handle) {
    if (handle.id >= g_page_provider_ctx.count) {
        return;
    }
    
    texture_page_provider_internal_t* item = &g_page_provider_ctx.items[handle.id];
    texture_page_provider_lock(item);
    
    /* Culling integration */
    if (texture_page_provider_should_cull(item, 500.0f)) {
        texture_page_provider_unlock(item);
        return;
    }
    
    item->dirty = true;
    item->frame_updated++;
    
    texture_page_provider_unlock(item);
}

int texture_page_provider_process_pending(void) {
    if (!g_page_provider_ctx.initialized) {
        return 0;
    }
    
    double start_time = (double)clock() / CLOCKS_PER_SEC;
    int processed = 0;
    
    /* Collect dirty providers */
    texture_page_provider_internal_t* dirty_providers[TEXTURE_PAGE_PROVIDER_BATCH_SIZE];
    uint32_t dirty_count = 0;
    
    for (uint32_t i = 0; i < g_page_provider_ctx.count && dirty_count < TEXTURE_PAGE_PROVIDER_BATCH_SIZE; i++) {
        texture_page_provider_internal_t* item = &g_page_provider_ctx.items[i];
        if (item->initialized && item->dirty) {
            dirty_providers[dirty_count++] = item;
        }
    }
    
    /* Batch processing */
    if (dirty_count > 0) {
        texture_page_provider_process_batch(dirty_providers, dirty_count);
        processed = dirty_count;
    }
    
    /* Execute render graph nodes */
    for (uint32_t i = 0; i < 64; i++) {
        if (g_page_provider_ctx.render_nodes[i].execute && !g_page_provider_ctx.render_nodes[i].executed) {
            g_page_provider_ctx.render_nodes[i].execute(g_page_provider_ctx.render_nodes[i].user_data);
            g_page_provider_ctx.render_nodes[i].executed = true;
        }
    }
    
    double end_time = (double)clock() / CLOCKS_PER_SEC;
    texture_page_provider_update_stats("batch_op", end_time - start_time);
    
    return processed;
}

uint32_t texture_page_provider_get_count(void) {
    return g_page_provider_ctx.count;
}

size_t texture_page_provider_get_memory_usage(void) {
    if (!g_page_provider_ctx.initialized) {
        return 0;
    }
    
    size_t total = sizeof(g_page_provider_ctx);
    total += g_page_provider_ctx.capacity * sizeof(texture_page_provider_internal_t);
    total += g_page_provider_ctx.pool_size;
    
    /* Add memory from all providers */
    for (uint32_t i = 0; i < g_page_provider_ctx.count; i++) {
        const texture_page_provider_internal_t* item = &g_page_provider_ctx.items[i];
        total += item->data_size;
        
        /* Virtual texturing memory */
        if (item->pages) {
            total += item->page_capacity * sizeof(texture_page_t);
            for (uint32_t j = 0; j < item->page_count; j++) {
                total += item->pages[j].data_size;
            }
        }
        
        /* Compression memory */
        if (item->compression.lz4_state) {
            total += LZ4_sizeofStreamState();
        }
        if (item->compression.zstd_state) {
            total += ZSTD_sizeof_CCtx(item->compression.zstd_state);
        }
        
        /* GPU memory */
        if (item->gpu.gpu_context) {
            total += 1024; /* Mock GPU context size */
        }
    }
    
    return total;
}

void texture_page_provider_debug_print(void) {
    if (!g_page_provider_ctx.initialized) {
        printf("Page provider not initialized\n");
        return;
    }
    
    printf("=== Texture Page Provider Debug Info ===\n");
    printf("Total providers: %u / %u\n", g_page_provider_ctx.count, g_page_provider_ctx.capacity);
    printf("Memory pool: %zu / %zu bytes\n", g_page_provider_ctx.pool_used, g_page_provider_ctx.pool_size);
    
    /* Print statistics */
    printf("\n--- Performance Statistics ---\n");
    printf("Pages processed: %llu\n", (unsigned long long)g_page_provider_ctx.stats.pages_processed);
    printf("Pages cached: %llu\n", (unsigned long long)g_page_provider_ctx.stats.pages_cached);
    printf("Cache hits: %llu, Cache misses: %llu\n", 
           (unsigned long long)g_page_provider_ctx.stats.cache_hits,
           (unsigned long long)g_page_provider_ctx.stats.cache_misses);
    printf("GPU uploads: %llu\n", (unsigned long long)g_page_provider_ctx.stats.gpu_uploads);
    printf("SIMD operations: %llu\n", (unsigned long long)g_page_provider_ctx.stats.simd_operations);
    printf("Batch operations: %llu\n", (unsigned long long)g_page_provider_ctx.stats.batch_operations);
    printf("Async operations: %llu\n", (unsigned long long)g_page_provider_ctx.stats.async_operations);
    printf("Average processing time: %.6f ms\n", g_page_provider_ctx.stats.average_processing_time * 1000.0);
    
    /* Print provider details */
    printf("\n--- Provider Details ---\n");
    for (uint32_t i = 0; i < g_page_provider_ctx.count; i++) {
        const texture_page_provider_internal_t* item = &g_page_provider_ctx.items[i];
        printf("Provider %u: ID=%u, Flags=0x%08x, Initialized=%s, Dirty=%s\n",
               i, item->id, item->flags,
               item->initialized ? "Yes" : "No",
               item->dirty ? "Yes" : "No");
        
        if (item->initialized) {
            printf("  Data size: %zu bytes\n", item->data_size);
            printf("  Frame updated: %llu\n", (unsigned long long)item->frame_updated);
            printf("  Virtual pages: %u / %u\n", item->page_count, item->page_capacity);
            printf("  Current LOD: %u\n", item->lod.current_lod);
            printf("  Anisotropic level: %.1f\n", item->anisotropy_level);
            printf("  Format conversion: %s\n", item->format_conversion_enabled ? "Enabled" : "Disabled");
            printf("  GPU resident: %s\n", item->gpu.gpu_resident ? "Yes" : "No");
            printf("  Bindless handle: 0x%016llx\n", (unsigned long long)item->bindless_handle);
        }
    printf("========================================\n");
}

/* End of page_provider.c */
