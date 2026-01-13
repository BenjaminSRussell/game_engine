/*
 * morph_weights.c
 * Morph weight blending
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement skeletal animation
 * TODO: Add animation blending
 * TODO: Implement IK solvers
 * TODO: Add morph target support
 * TODO: Implement GPU skinning
 * TODO: Add animation compression
 * TODO: Implement state machine
 * TODO: Add procedural animation
 * TODO: Implement ragdoll physics
 * TODO: Add animation retargeting
 * TODO: Implement morph weights initialization
 * TODO: Add morph weights cleanup/shutdown
 * TODO: Implement morph weights validation
 * TODO: Add morph weights error handling
 * TODO: Implement morph weights serialization
 * TODO: Add morph weights debug output
 * TODO: Implement morph weights unit tests
 * TODO: Add morph weights performance counters
 * TODO: Implement morph weights hot-reload
 * TODO: Add morph weights thread safety
 * TODO: Implement morph weights memory pooling
 * TODO: Add morph weights caching layer
 * TODO: Implement morph weights async operations
 * TODO: Add morph weights GPU integration
 * TODO: Implement morph weights SIMD optimization
 * TODO: Add morph weights batch processing
 * TODO: Implement morph weights streaming support
 * TODO: Add morph weights LOD support
 * TODO: Implement morph weights culling integration
 * TODO: Add morph weights render graph node
 */

#include "character/animation/morph_targets/morph_weights.h"
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
#include <math.h>
#include <errno.h>
#include <immintrin.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_MORPH_WEIGHTS_MAX_COUNT 4096
#define ANIMATION_MORPH_WEIGHTS_DEFAULT_CAPACITY 256
#define ANIMATION_MORPH_WEIGHTS_ALIGNMENT 16
#define ANIMATION_MORPH_WEIGHTS_CACHE_SIZE 1024
#define ANIMATION_MORPH_WEIGHTS_MAX_ASYNC_OPERATIONS 64
#define ANIMATION_MORPH_WEIGHTS_MEMORY_POOL_SIZE (16 * 1024 * 1024) // 16MB
#define ANIMATION_MORPH_WEIGHTS_MAGIC_NUMBER 0x4D4F5250 // 'MORP'
#define ANIMATION_MORPH_WEIGHTS_SERIALIZATION_VERSION 1
#define ANIMATION_MORPH_WEIGHTS_SIMD_WIDTH 16
#define ANIMATION_MORPH_WEIGHTS_BATCH_SIZE 32
#define ANIMATION_MORPH_WEIGHTS_LOD_LEVELS 4

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Error codes */
typedef enum {
    ANIMATION_MORPH_WEIGHTS_ERROR_NONE = 0,
    ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_HANDLE = -1,
    ANIMATION_MORPH_WEIGHTS_ERROR_NOT_INITIALIZED = -2,
    ANIMATION_MORPH_WEIGHTS_ERROR_OUT_OF_MEMORY = -3,
    ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_PARAMETER = -4,
    ANIMATION_MORPH_WEIGHTS_ERROR_SERIALIZATION_FAILED = -5,
    ANIMATION_MORPH_WEIGHTS_ERROR_GPU_OPERATION_FAILED = -6,
    ANIMATION_MORPH_WEIGHTS_ERROR_ASYNC_OPERATION_FAILED = -7
} animation_morph_weights_error_t;

/* Performance counters */
typedef struct {
    uint64_t total_operations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t async_operations;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t batch_operations;
    uint64_t streaming_operations;
    uint64_t lod_transitions;
    uint64_t culling_operations;
    double total_time_ms;
    double peak_memory_mb;
} animation_morph_weights_performance_t;

/* Cache entry */
typedef struct {
    uint32_t id;
    void* data;
    size_t size;
    uint64_t last_access;
    uint32_t access_count;
    bool valid;
} animation_morph_weights_cache_entry_t;

/* Async operation */
typedef struct {
    uint32_t id;
    animation_morph_weights_handle_t handle;
    enum {
        MORPH_ASYNC_OPERATION_UPDATE,
        MORPH_ASYNC_OPERATION_PROCESS,
        MORPH_ASYNC_OPERATION_STREAM
    } type;
    void* data;
    size_t size;
    bool completed;
    pthread_t thread;
} animation_morph_weights_async_op_t;

/* LOD level */
typedef struct {
    float distance_threshold;
    uint32_t quality_factor;
    bool enabled;
} animation_morph_weights_lod_level_t;

/* Render graph node */
typedef struct {
    uint32_t node_id;
    animation_morph_weights_handle_t handle;
    uint32_t dependency_count;
    uint32_t* dependencies;
    bool enabled;
} animation_morph_weights_render_node_t;

/* Streaming data */
typedef struct {
    uint32_t chunk_size;
    uint32_t current_chunk;
    uint32_t total_chunks;
    bool streaming_active;
    void* stream_buffer;
} animation_morph_weights_stream_data_t;

/* SIMD data */
typedef struct {
    __m128 simd_registers[4];
    bool simd_enabled;
    uint32_t alignment_size;
} animation_morph_weights_simd_data_t;

/* Batch context */
typedef struct {
    void* batch_items[ANIMATION_MORPH_WEIGHTS_BATCH_SIZE];
    size_t batch_sizes[ANIMATION_MORPH_WEIGHTS_BATCH_SIZE];
    uint32_t batch_count;
    bool batch_processing_active;
} animation_morph_weights_batch_context_t;

/* Performance counters */
typedef struct {
    uint64_t total_operations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t async_operations;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t batch_operations;
    uint64_t streaming_operations;
    uint64_t lod_transitions;
    uint64_t culling_operations;
    double total_time_ms;
    double peak_memory_mb;
} animation_morph_weights_performance_counters_t;

/* Memory pool */
typedef struct {
    void* pool_memory;
    size_t pool_size;
    size_t pool_used;
    uint32_t allocation_count;
    bool pool_initialized;
} animation_morph_weights_memory_pool_t;

typedef struct animation_morph_weights_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    /* Extended fields */
    float* weights;
    uint32_t weight_count;
    float* target_weights;
    float blend_factor;
    uint32_t blend_source_id;
    uint32_t blend_target_id;
    uint64_t last_update_time;
    uint32_t update_count;
    float lod_distance;
    uint32_t lod_level;
    bool visible;
    float bounding_radius;
    uint32_t render_graph_node_id;
    bool render_graph_dirty;
    uint32_t current_lod;
    bool gpu_resident;
    void* gpu_buffer;
    uint64_t last_gpu_update;
} animation_morph_weights_internal_t;

typedef struct animation_morph_weights_memory_pool {
    void* memory;
    size_t size;
    size_t used;
    void** free_blocks;
    uint32_t free_count;
    pthread_mutex_t mutex;
} animation_morph_weights_memory_pool_t;

typedef struct animation_morph_weights_cache_entry {
    uint32_t id;
    void* data;
    size_t data_size;
    uint64_t last_access;
    bool valid;
} animation_morph_weights_cache_entry_t;

typedef struct animation_morph_weights_async_operation {
    uint32_t operation_id;
    uint32_t morph_id;
    enum {
        MORPH_ASYNC_UPDATE,
        MORPH_ASYNC_BLEND,
        MORPH_ASYNC_SERIALIZE,
        MORPH_ASYNC_DESERIALIZE
    } operation_type;
    void* input_data;
    size_t input_size;
    void* output_data;
    size_t output_size;
    bool completed;
    bool in_progress;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} animation_morph_weights_async_operation_t;

typedef struct animation_morph_weights_performance_counters {
    uint64_t total_updates;
    uint64_t total_blends;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t memory_allocations;
    uint64_t memory_deallocations;
    uint64_t async_operations_completed;
    uint64_t gpu_transfers;
    double total_update_time;
    double total_blend_time;
    double peak_memory_usage;
} animation_morph_weights_performance_counters_t;

typedef struct animation_morph_weights_simd_data {
    float* aligned_weights;
    float* aligned_targets;
    __m128 blend_vectors[4];
    bool simd_enabled;
} animation_morph_weights_simd_data_t;

typedef struct animation_morph_weights_batch_context {
    uint32_t* morph_ids;
    uint32_t count;
    float* batch_weights;
    size_t batch_size;
    bool parallel_processing;
} animation_morph_weights_batch_context_t;

typedef struct animation_morph_weights_stream_data {
    FILE* stream_file;
    size_t stream_position;
    size_t stream_size;
    bool streaming_active;
    uint32_t chunk_size;
} animation_morph_weights_stream_data_t;

typedef struct animation_morph_weights_context {
    animation_morph_weights_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Thread safety */
    pthread_mutex_t global_mutex;
    pthread_rwlock_t cache_rwlock;
    
    /* Memory management */
    animation_morph_weights_memory_pool_t memory_pool;
    size_t current_memory_usage;
    size_t peak_memory_usage;
    
    /* Caching layer */
    animation_morph_weights_cache_entry_t cache[ANIMATION_MORPH_WEIGHTS_CACHE_SIZE];
    uint32_t cache_index;
    
    /* Async operations */
    animation_morph_weights_async_op_t async_ops[ANIMATION_MORPH_WEIGHTS_MAX_ASYNC_OPERATIONS];
    uint32_t async_count;
    pthread_mutex_t async_mutex;
    
    /* Performance counters */
    animation_morph_weights_performance_counters_t performance;
    
    /* Hot-reload support */
    int inotify_fd;
    int inotify_wd;
    pthread_t file_watcher_thread;
    bool hot_reload_enabled;
    
    /* GPU integration */
    void* gpu_context;
    bool gpu_available;
    
    /* SIMD optimization */
    animation_morph_weights_simd_data_t simd_data;
    
    /* Batch processing */
    animation_morph_weights_batch_context_t batch_context;
    
    /* Streaming support */
    animation_morph_weights_stream_data_t stream_data;
    
    /* Error handling */
    uint32_t last_error_code;
    char last_error_message[256];
} animation_morph_weights_context_t;

static animation_morph_weights_context_t g_morph_weights_ctx = {0};

/* Helper functions */
static void* animation_morph_weights_file_watcher_thread(void* arg) {
    (void)arg;
    char buffer[4096];
    
    while (g_morph_weights_ctx.hot_reload_enabled) {
        ssize_t len = read(g_morph_weights_ctx.inotify_fd, buffer, sizeof(buffer));
        if (len > 0) {
            /* Process file change events */
            pthread_mutex_lock(&g_morph_weights_ctx.mutex);
            
            /* Mark all items as dirty for reload */
            for (uint32_t i = 0; i < g_morph_weights_ctx.count; i++) {
                g_morph_weights_ctx.items[i].dirty = true;
            }
            
            pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
        }
        usleep(100000); /* 100ms */
    }
    
    return NULL;
}

static void* animation_morph_weights_async_worker_thread(void* arg) {
    animation_morph_weights_async_op_t* op = (animation_morph_weights_async_op_t*)arg;
    
    switch (op->type) {
        case MORPH_ASYNC_OPERATION_UPDATE:
            /* Simulate async update work */
            usleep(1000); /* 1ms */
            break;
        case MORPH_ASYNC_OPERATION_PROCESS:
            /* Simulate async processing work */
            usleep(5000); /* 5ms */
            break;
        case MORPH_ASYNC_OPERATION_STREAM:
            /* Simulate async streaming work */
            usleep(10000); /* 10ms */
            break;
    }
    
    op->completed = true;
    return NULL;
}

static bool animation_morph_weights_cache_lookup(uint32_t id, void** out_data, size_t* out_size) {
    pthread_rwlock_rdlock(&g_morph_weights_ctx.cache_lock);
    
    for (uint32_t i = 0; i < ANIMATION_MORPH_WEIGHTS_CACHE_SIZE; i++) {
        animation_morph_weights_cache_entry_t* entry = &g_morph_weights_ctx.cache[i];
        if (entry->valid && entry->id == id) {
            entry->last_access = time(NULL);
            entry->access_count++;
            *out_data = entry->data;
            *out_size = entry->size;
            
            g_morph_weights_ctx.performance.cache_hits++;
            pthread_rwlock_unlock(&g_morph_weights_ctx.cache_lock);
            return true;
        }
    }
    
    g_morph_weights_ctx.performance.cache_misses++;
    pthread_rwlock_unlock(&g_morph_weights_ctx.cache_lock);
    return false;
}

static void animation_morph_weights_cache_store(uint32_t id, const void* data, size_t size) {
    pthread_rwlock_wrlock(&g_morph_weights_ctx.cache_lock);
    
    /* Find LRU entry to replace */
    uint32_t lru_index = 0;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < ANIMATION_MORPH_WEIGHTS_CACHE_SIZE; i++) {
        animation_morph_weights_cache_entry_t* entry = &g_morph_weights_ctx.cache[i];
        if (!entry->valid) {
            lru_index = i;
            break;
        }
        if (entry->last_access < oldest_time) {
            oldest_time = entry->last_access;
            lru_index = i;
        }
    }
    
    animation_morph_weights_cache_entry_t* entry = &g_morph_weights_ctx.cache[lru_index];
    
    /* Clean up old entry */
    if (entry->valid && entry->data) {
        free(entry->data);
    }
    
    /* Store new entry */
    entry->id = id;
    entry->data = malloc(size);
    if (entry->data) {
        memcpy(entry->data, data, size);
        entry->size = size;
        entry->last_access = time(NULL);
        entry->access_count = 1;
        entry->valid = true;
    }
    
    pthread_rwlock_unlock(&g_morph_weights_ctx.cache_lock);
}

static void animation_morph_weights_simd_process(float* weights, uint32_t count) {
    if (!g_morph_weights_ctx.simd_available || count < 4) {
        return;
    }
    
    uint32_t simd_count = (count / 4) * 4;
    
    for (uint32_t i = 0; i < simd_count; i += 4) {
        __m128 vec = _mm_load_ps(&weights[i]);
        /* Apply SIMD operations - example: clamp to [0,1] */
        __m128 zero = _mm_setzero_ps();
        __m128 one = _mm_set1_ps(1.0f);
        vec = _mm_max_ps(vec, zero);
        vec = _mm_min_ps(vec, one);
        _mm_store_ps(&weights[i], vec);
    }
    
    g_morph_weights_ctx.performance.simd_operations++;
}

static uint32_t animation_morph_weights_calculate_lod(float distance) {
    for (uint32_t i = 0; i < g_morph_weights_ctx.current_lod_count; i++) {
        if (distance <= g_morph_weights_ctx.lod_levels[i].distance_threshold) {
            return i;
        }
    }
    return g_morph_weights_ctx.current_lod_count - 1;
}

static bool animation_morph_weights_is_culled(float distance) {
    if (!g_morph_weights_ctx.culling_enabled) {
        return false;
    }
    
    g_morph_weights_ctx.performance.culling_operations++;
    return distance > g_morph_weights_ctx.culling_distance;
}

static int animation_morph_weights_serialize_to_buffer(const animation_morph_weights_internal_t* item, 
                                                      void* buffer, size_t buffer_size, size_t* out_size) {
    if (!item || !buffer || !out_size) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_PARAMETER;
    }
    
    size_t required_size = sizeof(uint32_t) * 4 + sizeof(float) * item->weight_count + item->data_size;
    if (buffer_size < required_size) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_PARAMETER;
    }
    
    uint8_t* ptr = (uint8_t*)buffer;
    
    /* Write magic number and version */
    *(uint32_t*)ptr = ANIMATION_MORPH_WEIGHTS_MAGIC_NUMBER;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = ANIMATION_MORPH_WEIGHTS_VERSION;
    ptr += sizeof(uint32_t);
    
    /* Write item data */
    *(uint32_t*)ptr = item->id;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->flags;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->weight_count;
    ptr += sizeof(uint32_t);
    
    /* Write weights */
    if (item->weights && item->weight_count > 0) {
        memcpy(ptr, item->weights, sizeof(float) * item->weight_count);
        ptr += sizeof(float) * item->weight_count;
    }
    
    /* Write custom data */
    if (item->data && item->data_size > 0) {
        memcpy(ptr, item->data, item->data_size);
        ptr += item->data_size;
    }
    
    *out_size = required_size;
    return ANIMATION_MORPH_WEIGHTS_ERROR_NONE;
}

static int animation_morph_weights_deserialize_from_buffer(const void* buffer, size_t buffer_size,
                                                          animation_morph_weights_internal_t* out_item) {
    if (!buffer || buffer_size < sizeof(uint32_t) * 4 || !out_item) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_PARAMETER;
    }
    
    const uint8_t* ptr = (const uint8_t*)buffer;
    
    /* Read and verify magic number */
    uint32_t magic = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    if (magic != ANIMATION_MORPH_WEIGHTS_MAGIC_NUMBER) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_SERIALIZATION_FAILED;
    }
    
    /* Read version */
    uint32_t version = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    if (version != ANIMATION_MORPH_WEIGHTS_VERSION) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_SERIALIZATION_FAILED;
    }
    
    /* Read item data */
    out_item->id = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    out_item->flags = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    out_item->weight_count = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    
    /* Read weights */
    if (out_item->weight_count > 0) {
        size_t weights_size = sizeof(float) * out_item->weight_count;
        if (buffer_size < (ptr - (const uint8_t*)buffer) + weights_size) {
            return ANIMATION_MORPH_WEIGHTS_ERROR_SERIALIZATION_FAILED;
        }
        
        out_item->weights = malloc(weights_size);
        if (out_item->weights) {
            memcpy(out_item->weights, ptr, weights_size);
        }
        ptr += weights_size;
    }
    
    /* Read custom data */
    size_t remaining_size = buffer_size - (ptr - (const uint8_t*)buffer);
    if (remaining_size > 0) {
        out_item->data = malloc(remaining_size);
        if (out_item->data) {
            memcpy(out_item->data, ptr, remaining_size);
            out_item->data_size = remaining_size;
        }
    }
    
    out_item->initialized = true;
    out_item->dirty = false;
    
    return ANIMATION_MORPH_WEIGHTS_ERROR_NONE;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool animation_morph_weights_validate(const animation_morph_weights_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->weight_count == 0) return false;
    if (!item->weights && item->weight_count > 0) return false;
    
    /* Validate weight ranges */
    for (uint32_t i = 0; i < item->weight_count; i++) {
        if (item->weights && !isfinite(item->weights[i])) {
            return false;
        }
    }
    
    return true;
}

static void animation_morph_weights_cleanup_internal(animation_morph_weights_internal_t* item) {
    if (!item) return;
    
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    if (item->weights) {
        free(item->weights);
        item->weights = NULL;
    }
    
    if (item->target_weights) {
        free(item->target_weights);
        item->target_weights = NULL;
    }
    
    item->data_size = 0;
    item->initialized = false;
    item->dirty = false;
    item->weight_count = 0;
    item->current_lod = 0;
    item->visible = false;
    item->gpu_resident = false;
    
    pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_morph_weights_init(void) {
    if (g_morph_weights_ctx.initialized) {
        return 0; /* Already initialized */
    }
    
    /* Initialize mutexes */
    if (pthread_mutex_init(&g_morph_weights_ctx.mutex, NULL) != 0) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_OUT_OF_MEMORY;
    }
    
    if (pthread_rwlock_init(&g_morph_weights_ctx.cache_lock, NULL) != 0) {
        pthread_mutex_destroy(&g_morph_weights_ctx.mutex);
        return ANIMATION_MORPH_WEIGHTS_ERROR_OUT_OF_MEMORY;
    }
    
    if (pthread_mutex_init(&g_morph_weights_ctx.async_mutex, NULL) != 0) {
        pthread_mutex_destroy(&g_morph_weights_ctx.mutex);
        pthread_rwlock_destroy(&g_morph_weights_ctx.cache_lock);
        return ANIMATION_MORPH_WEIGHTS_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize main array */
    g_morph_weights_ctx.capacity = ANIMATION_MORPH_WEIGHTS_DEFAULT_CAPACITY;
    g_morph_weights_ctx.items = calloc(g_morph_weights_ctx.capacity, sizeof(animation_morph_weights_internal_t));
    if (!g_morph_weights_ctx.items) {
        pthread_mutex_destroy(&g_morph_weights_ctx.mutex);
        pthread_rwlock_destroy(&g_morph_weights_ctx.cache_lock);
        pthread_mutex_destroy(&g_morph_weights_ctx.async_mutex);
        return ANIMATION_MORPH_WEIGHTS_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize cache */
    memset(g_morph_weights_ctx.cache, 0, sizeof(g_morph_weights_ctx.cache));
    
    /* Initialize performance counters */
    memset(&g_morph_weights_ctx.performance, 0, sizeof(g_morph_weights_ctx.performance));
    
    /* Initialize LOD levels */
    g_morph_weights_ctx.current_lod_count = ANIMATION_MORPH_WEIGHTS_LOD_LEVELS;
    for (uint32_t i = 0; i < ANIMATION_MORPH_WEIGHTS_LOD_LEVELS; i++) {
        g_morph_weights_ctx.lod_levels[i].distance_threshold = (float)(i + 1) * 10.0f;
        g_morph_weights_ctx.lod_levels[i].quality_factor = (ANIMATION_MORPH_WEIGHTS_LOD_LEVELS - i) * 25;
        g_morph_weights_ctx.lod_levels[i].enabled = true;
    }
    
    /* Initialize hot-reload */
    g_morph_weights_ctx.hot_reload_enabled = true;
    g_morph_weights_ctx.inotify_fd = inotify_init();
    if (g_morph_weights_ctx.inotify_fd >= 0) {
        g_morph_weights_ctx.inotify_wd = inotify_add_watch(g_morph_weights_ctx.inotify_fd, 
                                                           ".", IN_MODIFY | IN_CREATE);
        if (g_morph_weights_ctx.inotify_wd >= 0) {
            pthread_create(&g_morph_weights_ctx.file_watcher_thread, NULL, 
                          animation_morph_weights_file_watcher_thread, NULL);
        }
    }
    
    /* Initialize GPU integration */
    g_morph_weights_ctx.gpu_available = true; /* Would check for GPU availability */
    g_morph_weights_ctx.gpu_context = NULL; /* Would initialize GPU context */
    
    /* Initialize SIMD */
    g_morph_weights_ctx.simd_available = true; /* Would check for SIMD support */
    
    /* Initialize streaming */
    g_morph_weights_ctx.streaming_enabled = true;
    g_morph_weights_ctx.streaming_chunk_size = 1024;
    
    /* Initialize culling */
    g_morph_weights_ctx.culling_enabled = true;
    g_morph_weights_ctx.culling_distance = 100.0f;
    
    /* Initialize render graph */
    g_morph_weights_ctx.render_node_capacity = 64;
    g_morph_weights_ctx.render_nodes = calloc(g_morph_weights_ctx.render_node_capacity, 
                                              sizeof(animation_morph_weights_render_node_t));
    
    g_morph_weights_ctx.count = 0;
    g_morph_weights_ctx.initialized = true;
    
    return ANIMATION_MORPH_WEIGHTS_ERROR_NONE;
}

void animation_morph_weights_shutdown(void) {
    if (!g_morph_weights_ctx.initialized) {
        return;
    }
    
    /* Stop hot-reload */
    g_morph_weights_ctx.hot_reload_enabled = false;
    if (g_morph_weights_ctx.inotify_fd >= 0) {
        pthread_join(g_morph_weights_ctx.file_watcher_thread, NULL);
        inotify_rm_watch(g_morph_weights_ctx.inotify_fd, g_morph_weights_ctx.inotify_wd);
        close(g_morph_weights_ctx.inotify_fd);
    }
    
    /* Wait for async operations to complete */
    pthread_mutex_lock(&g_morph_weights_ctx.async_mutex);
    for (uint32_t i = 0; i < g_morph_weights_ctx.async_count; i++) {
        if (!g_morph_weights_ctx.async_ops[i].completed) {
            pthread_join(g_morph_weights_ctx.async_ops[i].thread, NULL);
        }
        if (g_morph_weights_ctx.async_ops[i].data) {
            free(g_morph_weights_ctx.async_ops[i].data);
        }
    }
    pthread_mutex_unlock(&g_morph_weights_ctx.async_mutex);
    
    /* Cleanup all items */
    for (uint32_t i = 0; i < g_morph_weights_ctx.count; i++) {
        animation_morph_weights_cleanup_internal(&g_morph_weights_ctx.items[i]);
    }
    
    /* Cleanup cache */
    for (uint32_t i = 0; i < ANIMATION_MORPH_WEIGHTS_CACHE_SIZE; i++) {
        if (g_morph_weights_ctx.cache[i].valid && g_morph_weights_ctx.cache[i].data) {
            free(g_morph_weights_ctx.cache[i].data);
        }
    }
    
    /* Cleanup render graph */
    for (uint32_t i = 0; i < g_morph_weights_ctx.render_node_count; i++) {
        if (g_morph_weights_ctx.render_nodes[i].dependencies) {
            free(g_morph_weights_ctx.render_nodes[i].dependencies);
        }
    }
    
    /* Free allocated memory */
    free(g_morph_weights_ctx.items);
    free(g_morph_weights_ctx.render_nodes);
    
    /* Destroy mutexes */
    pthread_mutex_destroy(&g_morph_weights_ctx.mutex);
    pthread_rwlock_destroy(&g_morph_weights_ctx.cache_lock);
    pthread_mutex_destroy(&g_morph_weights_ctx.async_mutex);
    
    /* Reset context */
    memset(&g_morph_weights_ctx, 0, sizeof(g_morph_weights_ctx));
}

int animation_morph_weights_create(animation_morph_weights_handle_t* out_handle, const animation_morph_weights_desc_t* desc) {
    if (!out_handle || !desc) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_PARAMETER;
    }

    if (!g_morph_weights_ctx.initialized) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_NOT_INITIALIZED;
    }

    pthread_mutex_lock(&g_morph_weights_ctx.mutex);
    
    if (g_morph_weights_ctx.count >= g_morph_weights_ctx.capacity) {
        pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
        return ANIMATION_MORPH_WEIGHTS_ERROR_OUT_OF_MEMORY;
    }

    uint32_t index = g_morph_weights_ctx.count++;
    animation_morph_weights_internal_t* item = &g_morph_weights_ctx.items[index];

    memset(item, 0, sizeof(animation_morph_weights_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->blend_factor = 1.0f;
    item->visible = true;
    item->lod_level = 0;
    item->bounding_radius = 1.0f;
    item->render_graph_dirty = true;

    g_morph_weights_ctx.performance.memory_allocations++;
    
    if (!item->weights) {
        g_morph_weights_ctx.count--;
        pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
        return ANIMATION_MORPH_WEIGHTS_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize custom data */
    if (desc->user_data) {
        item->data_size = 256; /* Default data size */
        item->data = malloc(item->data_size);
        if (item->data) {
            memcpy(item->data, desc->user_data, 
                   (item->data_size < 256) ? item->data_size : 256);
        }
    }
    
    g_morph_weights_ctx.performance.total_operations++;
    
    pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
    
    out_handle->id = index;
    return ANIMATION_MORPH_WEIGHTS_ERROR_NONE;
}

void animation_morph_weights_destroy(animation_morph_weights_handle_t handle) {
    if (!g_morph_weights_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_morph_weights_ctx.mutex);
    
    if (handle.id >= g_morph_weights_ctx.count) {
        pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
        return;
    }
    
    animation_morph_weights_cleanup_internal(&g_morph_weights_ctx.items[handle.id]);
    
    /* Remove from cache */
    pthread_rwlock_wrlock(&g_morph_weights_ctx.cache_lock);
    for (uint32_t i = 0; i < ANIMATION_MORPH_WEIGHTS_CACHE_SIZE; i++) {
        if (g_morph_weights_ctx.cache[i].valid && 
            g_morph_weights_ctx.cache[i].id == handle.id) {
            if (g_morph_weights_ctx.cache[i].data) {
                free(g_morph_weights_ctx.cache[i].data);
            }
            memset(&g_morph_weights_ctx.cache[i], 0, 
                   sizeof(animation_morph_weights_cache_entry_t));
            break;
        }
    }
    pthread_rwlock_unlock(&g_morph_weights_ctx.cache_lock);
    
    g_morph_weights_ctx.performance.total_operations++;
    
    pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
}

int animation_morph_weights_update(animation_morph_weights_handle_t handle, const void* data, size_t size) {
    if (!g_morph_weights_ctx.initialized) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_NOT_INITIALIZED;
    }
    
    if (!data || size == 0) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&g_morph_weights_ctx.mutex);
    
    if (handle.id >= g_morph_weights_ctx.count) {
        pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
        return ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_weights_internal_t* item = &g_morph_weights_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
        return ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_HANDLE;
    }
    
    /* Update weights if data contains float array */
    if (size >= sizeof(float)) {
        uint32_t weight_count = (uint32_t)(size / sizeof(float));
        if (weight_count > 0 && weight_count <= 1024) {
            if (item->weight_count != weight_count) {
                /* Reallocate weights array */
                float* new_weights = realloc(item->weights, weight_count * sizeof(float));
                if (new_weights) {
                    item->weights = new_weights;
                    item->weight_count = weight_count;
                }
            }
            
            if (item->weights) {
                memcpy(item->weights, data, weight_count * sizeof(float));
                
                /* Apply SIMD processing if available */
                animation_morph_weights_simd_process(item->weights, item->weight_count);
                
                /* Store in cache */
                animation_morph_weights_cache_store(handle.id, item->weights, 
                                                  item->weight_count * sizeof(float));
            }
        }
    }
    
    /* Update custom data */
    if (item->data_size < size) {
        void* new_data = realloc(item->data, size);
        if (new_data) {
            item->data = new_data;
            item->data_size = size;
        }
    }
    
    if (item->data) {
        memcpy(item->data, data, size);
    }
    
    item->dirty = true;
    item->frame_updated++;
    
    /* GPU integration */
    if (g_morph_weights_ctx.gpu_available && item->gpu_resident) {
        /* Update GPU buffer */
        g_morph_weights_ctx.performance.gpu_operations++;
    }
    
    g_morph_weights_ctx.performance.total_operations++;
    
    pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
    
    return ANIMATION_MORPH_WEIGHTS_ERROR_NONE;
}

bool animation_morph_weights_is_valid(animation_morph_weights_handle_t handle) {
    if (!g_morph_weights_ctx.initialized) {
        return false;
    }
    
    if (handle.id >= g_morph_weights_ctx.count) {
        return false;
    }
    
    return animation_morph_weights_validate(&g_morph_weights_ctx.items[handle.id]);
}

int animation_morph_weights_get_info(animation_morph_weights_handle_t handle, animation_morph_weights_info_t* out_info) {
    if (!g_morph_weights_ctx.initialized) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_NOT_INITIALIZED;
    }
    
    if (!out_info) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_PARAMETER;
    }
    
    if (handle.id >= g_morph_weights_ctx.count) {
        return ANIMATION_MORPH_WEIGHTS_ERROR_INVALID_HANDLE;
    }
    
    const animation_morph_weights_internal_t* item = &g_morph_weights_ctx.items[handle.id];
    
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    return ANIMATION_MORPH_WEIGHTS_ERROR_NONE;
}

void animation_morph_weights_mark_dirty(animation_morph_weights_handle_t handle) {
    if (!g_morph_weights_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_morph_weights_ctx.mutex);
    
    if (handle.id < g_morph_weights_ctx.count) {
        g_morph_weights_ctx.items[handle.id].dirty = true;
        
        /* Update LOD based on distance (simulated) */
        float distance = 50.0f; /* Would get actual distance from camera */
        uint32_t new_lod = animation_morph_weights_calculate_lod(distance);
        if (new_lod != g_morph_weights_ctx.items[handle.id].current_lod) {
            g_morph_weights_ctx.items[handle.id].current_lod = new_lod;
            g_morph_weights_ctx.performance.lod_transitions++;
        }
        
        /* Check culling */
        g_morph_weights_ctx.items[handle.id].visible = 
            !animation_morph_weights_is_culled(distance);
    }
    
    pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
}

int animation_morph_weights_process_pending(void) {
    if (!g_morph_weights_ctx.initialized) {
        return 0;
    }
    
    int processed = 0;
    clock_t start_time = clock();
    
    pthread_mutex_lock(&g_morph_weights_ctx.mutex);
    
    /* Batch processing */
    uint32_t batch_items[ANIMATION_MORPH_WEIGHTS_BATCH_SIZE];
    uint32_t batch_count = 0;
    
    for (uint32_t i = 0; i < g_morph_weights_ctx.count; i++) {
        animation_morph_weights_internal_t* item = &g_morph_weights_ctx.items[i];
        if (item->initialized && item->dirty && item->visible) {
            batch_items[batch_count++] = i;
            
            if (batch_count >= ANIMATION_MORPH_WEIGHTS_BATCH_SIZE) {
                /* Process batch */
                for (uint32_t j = 0; j < batch_count; j++) {
                    animation_morph_weights_internal_t* batch_item = 
                        &g_morph_weights_ctx.items[batch_items[j]];
                    
                    /* Simulate processing work */
                    if (batch_item->weights) {
                        /* Apply LOD quality factor */
                        uint32_t quality_factor = 
                            g_morph_weights_ctx.lod_levels[batch_item->current_lod].quality_factor;
                        uint32_t process_count = (batch_item->weight_count * quality_factor) / 100;
                        
                        for (uint32_t k = 0; k < process_count; k++) {
                            batch_item->weights[k] = fmaxf(0.0f, fminf(1.0f, batch_item->weights[k]));
                        }
                    }
                    
                    batch_item->dirty = false;
                    batch_item->frame_updated++;
                    processed++;
                }
                
                g_morph_weights_ctx.performance.batch_operations++;
                batch_count = 0;
            }
        }
    }
    
    /* Process remaining items in batch */
    for (uint32_t j = 0; j < batch_count; j++) {
        animation_morph_weights_internal_t* batch_item = 
            &g_morph_weights_ctx.items[batch_items[j]];
        batch_item->dirty = false;
        batch_item->frame_updated++;
        processed++;
    }
    
    if (batch_count > 0) {
        g_morph_weights_ctx.performance.batch_operations++;
    }
    
    pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
    
    /* Update performance metrics */
    clock_t end_time = clock();
    double elapsed_ms = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;
    g_morph_weights_ctx.performance.total_time_ms += elapsed_ms;
    
    return processed;
}

uint32_t animation_morph_weights_get_count(void) {
    if (!g_morph_weights_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_morph_weights_ctx.mutex);
    uint32_t count = g_morph_weights_ctx.count;
    pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
    
    return count;
}

size_t animation_morph_weights_get_memory_usage(void) {
    if (!g_morph_weights_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_morph_weights_ctx.mutex);
    
    size_t total = sizeof(g_morph_weights_ctx);
    total += g_morph_weights_ctx.capacity * sizeof(animation_morph_weights_internal_t);
    
    /* Add item data sizes */
    for (uint32_t i = 0; i < g_morph_weights_ctx.count; i++) {
        const animation_morph_weights_internal_t* item = &g_morph_weights_ctx.items[i];
        total += item->data_size;
        if (item->weights) {
            total += item->weight_count * sizeof(float);
        }
    }
    
    /* Add cache memory usage */
    for (uint32_t i = 0; i < ANIMATION_MORPH_WEIGHTS_CACHE_SIZE; i++) {
        if (g_morph_weights_ctx.cache[i].valid && g_morph_weights_ctx.cache[i].data) {
            total += g_morph_weights_ctx.cache[i].size;
        }
    }
    
    /* Add async operation memory */
    total += g_morph_weights_ctx.async_count * sizeof(animation_morph_weights_async_op_t);
    
    /* Add render graph memory */
    total += g_morph_weights_ctx.render_node_capacity * 
              sizeof(animation_morph_weights_render_node_t);
    
    pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
    
    /* Update peak memory usage */
    double memory_mb = total / (1024.0 * 1024.0);
    if (memory_mb > g_morph_weights_ctx.performance.peak_memory_mb) {
        g_morph_weights_ctx.performance.peak_memory_mb = memory_mb;
    }
    
    return total;
}

void animation_morph_weights_debug_print(void) {
    if (!g_morph_weights_ctx.initialized) {
        printf("Morph weights system not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_morph_weights_ctx.mutex);
    
    printf("=== Morph Weights System Debug Info ===\n");
    printf("Total items: %u / %u\n", g_morph_weights_ctx.count, g_morph_weights_ctx.capacity);
    printf("GPU available: %s\n", g_morph_weights_ctx.gpu_available ? "Yes" : "No");
    printf("SIMD available: %s\n", g_morph_weights_ctx.simd_available ? "Yes" : "No");
    printf("Hot-reload enabled: %s\n", g_morph_weights_ctx.hot_reload_enabled ? "Yes" : "No");
    printf("Streaming enabled: %s\n", g_morph_weights_ctx.streaming_enabled ? "Yes" : "No");
    printf("Culling enabled: %s\n", g_morph_weights_ctx.culling_enabled ? "Yes" : "No");
    printf("Culling distance: %.2f\n", g_morph_weights_ctx.culling_distance);
    
    printf("\n=== Performance Counters ===\n");
    printf("Total operations: %llu\n", (unsigned long long)g_morph_weights_ctx.performance.total_operations);
    printf("Cache hits: %llu\n", (unsigned long long)g_morph_weights_ctx.performance.cache_hits);
    printf("Cache misses: %llu\n", (unsigned long long)g_morph_weights_ctx.performance.cache_misses);
    printf("Async operations: %llu\n", (unsigned long long)g_morph_weights_ctx.performance.async_operations);
    printf("GPU operations: %llu\n", (unsigned long long)g_morph_weights_ctx.performance.gpu_operations);
    printf("SIMD operations: %llu\n", (unsigned long long)g_morph_weights_ctx.performance.simd_operations);
    printf("Batch operations: %llu\n", (unsigned long long)g_morph_weights_ctx.performance.batch_operations);
    printf("Streaming operations: %llu\n", (unsigned long long)g_morph_weights_ctx.performance.streaming_operations);
    printf("LOD transitions: %llu\n", (unsigned long long)g_morph_weights_ctx.performance.lod_transitions);
    printf("Culling operations: %llu\n", (unsigned long long)g_morph_weights_ctx.performance.culling_operations);
    printf("Total time: %.2f ms\n", g_morph_weights_ctx.performance.total_time_ms);
    printf("Peak memory: %.2f MB\n", g_morph_weights_ctx.performance.peak_memory_mb);
    
    printf("\n=== LOD Levels ===\n");
    for (uint32_t i = 0; i < g_morph_weights_ctx.current_lod_count; i++) {
        printf("LOD %u: distance=%.1f, quality=%u%%, enabled=%s\n", 
               i, 
               g_morph_weights_ctx.lod_levels[i].distance_threshold,
               g_morph_weights_ctx.lod_levels[i].quality_factor,
               g_morph_weights_ctx.lod_levels[i].enabled ? "Yes" : "No");
    }
    
    printf("\n=== Item Details ===\n");
    for (uint32_t i = 0; i < g_morph_weights_ctx.count; i++) {
        const animation_morph_weights_internal_t* item = &g_morph_weights_ctx.items[i];
        printf("Item %u: initialized=%s, dirty=%s, weights=%u, lod=%u, visible=%s, gpu_resident=%s\n",
               item->id,
               item->initialized ? "Yes" : "No",
               item->dirty ? "Yes" : "No",
               item->weight_count,
               item->current_lod,
               item->visible ? "Yes" : "No",
               item->gpu_resident ? "Yes" : "No");
        
        if (item->weights && item->weight_count > 0) {
            printf("  Weights: ");
            uint32_t print_count = (item->weight_count > 8) ? 8 : item->weight_count;
            for (uint32_t j = 0; j < print_count; j++) {
                printf("%.3f ", item->weights[j]);
            }
            if (item->weight_count > 8) {
                printf("...");
            }
            printf("\n");
        }
    }
    
    printf("\n=== Cache Status ===\n");
    uint32_t valid_cache_entries = 0;
    for (uint32_t i = 0; i < ANIMATION_MORPH_WEIGHTS_CACHE_SIZE; i++) {
        if (g_morph_weights_ctx.cache[i].valid) {
            valid_cache_entries++;
        }
    }
    printf("Valid cache entries: %u / %u\n", valid_cache_entries, ANIMATION_MORPH_WEIGHTS_CACHE_SIZE);
    
    printf("\n=== Render Graph ===\n");
    printf("Render nodes: %u / %u\n", g_morph_weights_ctx.render_node_count, 
           g_morph_weights_ctx.render_node_capacity);
    
    printf("=== End Debug Info ===\n");
    
    pthread_mutex_unlock(&g_morph_weights_ctx.mutex);
}

/* End of morph_weights.c */
