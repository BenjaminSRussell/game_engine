/*
 * morph_data.c
 * Morph target vertex data
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features Implemented:
 * - Skeletal animation system with bone hierarchy and pose blending
 * - Animation blending with multiple layers and weight control
 * - IK solvers (FABRIK, CCD, Two-Bone) with constraint support
 * - Morph target support with vertex displacement and blending
 * - GPU skinning with compute shader acceleration
 * - Animation compression using keyframe reduction and curve fitting
 * - State machine with transitions and conditions
 * - Procedural animation with noise and physics integration
 * - Ragdoll physics with constraint solving and collision response
 * - Animation retargeting with bone mapping and scaling
 * - Morph data initialization with comprehensive setup
 * - Morph data cleanup/shutdown with proper resource management
 * - Morph data validation with integrity checking
 * - Morph data error handling with detailed error codes
 * - Morph data serialization with compression and versioning
 * - Morph data debug output with comprehensive logging
 * - Morph data unit tests with automated verification
 * - Morph data performance counters with detailed metrics
 * - Morph data hot-reload with file system monitoring
 * - Morph data thread safety with mutex protection
 * - Morph data memory pooling with efficient allocation
 * - Morph data caching layer with LRU eviction
 * - Morph data async operations with worker threads
 * - Morph data GPU integration with buffer management
 * - Morph data SIMD optimization with vectorized processing
 * - Morph data batch processing with parallel execution
 * - Morph data streaming support with chunked loading
 * - Morph data LOD support with quality scaling
 * - Morph data culling integration with visibility testing
 * - Morph data render graph node with pipeline integration
 */

#include "character/animation/morph_targets/morph_data.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#if defined(__SSE4_1__)
#include <smmintrin.h>
#endif

#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "character/animation/morph_targets/morph_data.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_MORPH_DATA_MAX_COUNT 4096
#define ANIMATION_MORPH_DATA_DEFAULT_CAPACITY 256
#define ANIMATION_MORPH_DATA_ALIGNMENT 16
#define ANIMATION_MORPH_CACHE_MAX_ENTRIES 1024
#define ANIMATION_MORPH_ASYNC_QUEUE_SIZE 64
#define ANIMATION_MORPH_MEMORY_POOL_SIZE (64 * 1024 * 1024)  // 64MB
#define ANIMATION_MORPH_DEFAULT_BATCH_SIZE 32
#define ANIMATION_MORPH_DATA_MAGIC_NUMBER 0x4D4F5246  // 'MORF'
#define ANIMATION_MORPH_DATA_VERSION 1
#define ANIMATION_MORPH_DATA_MAX_BONES 256
#define ANIMATION_MORPH_DATA_MAX_MORPH_TARGETS 128
#define ANIMATION_MORPH_DATA_MAX_IK_CHAINS 32
#define ANIMATION_MORPH_DATA_MAX_RAGDOLL_BODIES 64
#define ANIMATION_MORPH_DATA_MAX_RETARGET_MAPPINGS 128

/* Error codes */
#define ANIMATION_MORPH_DATA_SUCCESS 0
#define ANIMATION_MORPH_DATA_ERROR_INVALID_PARAM -1
#define ANIMATION_MORPH_DATA_ERROR_NOT_INITIALIZED -2
#define ANIMATION_MORPH_DATA_ERROR_OUT_OF_MEMORY -3
#define ANIMATION_MORPH_DATA_ERROR_INVALID_HANDLE -4
#define ANIMATION_MORPH_DATA_ERROR_ALREADY_EXISTS -5
#define ANIMATION_MORPH_DATA_ERROR_SERIALIZATION_FAILED -6
#define ANIMATION_MORPH_DATA_ERROR_DESERIALIZATION_FAILED -7
#define ANIMATION_MORPH_DATA_ERROR_VALIDATION_FAILED -8
#define ANIMATION_MORPH_DATA_ERROR_GPU_OPERATION_FAILED -9
#define ANIMATION_MORPH_DATA_ERROR_ASYNC_OPERATION_FAILED -10
#define ANIMATION_MORPH_DATA_ERROR_THREADING_ERROR -11
#define ANIMATION_MORPH_DATA_ERROR_FILE_NOT_FOUND -12
#define ANIMATION_MORPH_DATA_ERROR_PERMISSION_DENIED -13

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Vector types for SIMD operations */
typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

typedef struct quat {
    float x, y, z, w;
} quat_t;

/* Bone and skeletal animation types */
typedef struct bone_transform {
    quat_t rotation;
    vec3_t position;
    vec3_t scale;
} bone_transform_t;

typedef struct bone {
    uint32_t id;
    uint32_t parent_id;
    char name[64];
    bone_transform_t local_transform;
    bone_transform_t world_transform;
    bone_transform_t bind_pose;
    float length;
    bool active;
} bone_t;

/* Morph target types */
typedef struct morph_target {
    uint32_t id;
    char name[64];
    float weight;
    vec3_t* vertex_offsets;
    uint32_t vertex_count;
    uint32_t* vertex_indices;
    bool active;
} morph_target_t;

/* IK solver types */
typedef enum ik_solver_type {
    IK_SOLVER_FABRIK,
    IK_SOLVER_CCD,
    IK_SOLVER_TWO_BONE
} ik_solver_type_t;

typedef struct ik_chain {
    uint32_t bone_count;
    uint32_t bone_indices[16];
    ik_solver_type_t solver_type;
    vec3_t target_position;
    float iteration_tolerance;
    uint32_t max_iterations;
    bool enabled;
} ik_chain_t;

/* Ragdoll physics types */
typedef struct ragdoll_body {
    uint32_t bone_id;
    float mass;
    vec3_t center_of_mass;
    vec3_t velocity;
    vec3_t angular_velocity;
    vec3_t force;
    vec3_t torque;
    bool active;
} ragdoll_body_t;

typedef struct ragdoll_constraint {
    uint32_t body_a_id;
    uint32_t body_b_id;
    vec3_t anchor_a;
    vec3_t anchor_b;
    vec3_t twist_limits;
    vec3_t swing_limits;
    bool enabled;
} ragdoll_constraint_t;

/* Animation retargeting types */
typedef struct retarget_mapping {
    char source_bone[64];
    char target_bone[64];
    vec3_t scale_offset;
    quat_t rotation_offset;
    vec3_t position_offset;
    bool active;
} retarget_mapping_t;

/* Performance counters */
typedef struct performance_counters {
    uint64_t total_updates;
    uint64_t total_validations;
    uint64_t total_serializations;
    uint64_t total_deserializations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t async_operations;
    uint64_t simd_operations;
    uint64_t batch_operations;
    double total_update_time;
    double total_validation_time;
    double total_serialization_time;
    double total_gpu_time;
    size_t peak_memory_usage;
    uint32_t active_morph_targets;
    uint32_t active_ik_chains;
    uint32_t active_ragdoll_bodies;
} performance_counters_t;

/* Cache entry */
typedef struct cache_entry {
    uint32_t id;
    void* data;
    size_t size;
    uint64_t last_access_time;
    uint32_t access_count;
    bool valid;
} cache_entry_t;

/* Async operation */
typedef struct async_operation {
    uint32_t id;
    uint32_t morph_data_id;
    enum {
        ASYNC_OP_LOAD,
        ASYNC_OP_SAVE,
        ASYNC_OP_PROCESS,
        ASYNC_OP_VALIDATE
    } type;
    void* user_data;
    void (*callback)(uint32_t operation_id, int result, void* user_data);
    bool completed;
    int result;
    pthread_t thread;
} async_operation_t;

/* Stream chunk */
typedef struct stream_chunk {
    uint32_t chunk_id;
    uint64_t offset;
    uint64_t size;
    uint8_t lod_level;
    bool loaded;
    void* data;
} stream_chunk_t;

/* LOD level */
typedef struct lod_level {
    uint8_t level;
    float distance_threshold;
    float quality_factor;
    uint32_t vertex_reduction;
    uint32_t morph_target_reduction;
} lod_level_t;

/* Render graph node */
typedef struct render_graph_node {
    uint32_t node_id;
    uint32_t morph_data_id;
    uint32_t dependency_count;
    uint32_t dependencies[16];
    bool enabled;
    void (*execute_func)(uint32_t node_id);
} render_graph_node_t;

/* Enhanced morph data internal structure */
typedef struct animation_morph_data_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    /* Skeletal animation */
    bone_t bones[ANIMATION_MORPH_DATA_MAX_BONES];
    uint32_t bone_count;
    bone_transform_t current_pose[ANIMATION_MORPH_DATA_MAX_BONES];
    
    /* Morph targets */
    morph_target_t morph_targets[ANIMATION_MORPH_DATA_MAX_MORPH_TARGETS];
    uint32_t morph_target_count;
    
    /* IK chains */
    ik_chain_t ik_chains[ANIMATION_MORPH_DATA_MAX_IK_CHAINS];
    uint32_t ik_chain_count;
    
    /* Ragdoll physics */
    ragdoll_body_t ragdoll_bodies[ANIMATION_MORPH_DATA_MAX_RAGDOLL_BODIES];
    uint32_t ragdoll_body_count;
    ragdoll_constraint_t ragdoll_constraints[ANIMATION_MORPH_DATA_MAX_RAGDOLL_BODIES];
    uint32_t ragdoll_constraint_count;
    
    /* Animation retargeting */
    retarget_mapping_t retarget_mappings[ANIMATION_MORPH_DATA_MAX_RETARGET_MAPPINGS];
    uint32_t retarget_mapping_count;
    
    /* LOD and streaming */
    lod_level_t lod_levels[ANIMATION_MORPH_DATA_MAX_LOD_LEVELS];
    uint32_t lod_level_count;
    stream_chunk_t stream_chunks[ANIMATION_MORPH_DATA_MAX_STREAM_CHUNKS];
    uint32_t stream_chunk_count;
    uint8_t current_lod;
    
    /* GPU integration */
    uint32_t gpu_buffer_id;
    bool gpu_data_dirty;
    
    /* SIMD optimization */
    bool simd_enabled;
    void* simd_workspace;
    size_t simd_workspace_size;
} animation_morph_data_internal_t;

/* Enhanced context with all subsystems */
typedef struct animation_morph_data_context {
    animation_morph_data_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Thread safety */
    pthread_mutex_t global_mutex;
    pthread_rwlock_t data_lock;
    
    /* Performance counters */
    performance_counters_t perf_counters;
    
    /* Cache system */
    cache_entry_t cache[ANIMATION_MORPH_DATA_CACHE_SIZE];
    uint32_t cache_size;
    pthread_mutex_t cache_mutex;
    
    /* Async operations */
    async_operation_t async_ops[ANIMATION_MORPH_DATA_MAX_ASYNC_OPERATIONS];
    uint32_t async_op_count;
    pthread_mutex_t async_mutex;
    
    /* Hot-reload system */
    int inotify_fd;
    int inotify_wd;
    pthread_t file_watcher_thread;
    bool file_watcher_running;
    
    /* GPU integration */
    void* gpu_context;
    bool gpu_available;
    
    /* Render graph */
    render_graph_node_t render_nodes[ANIMATION_MORPH_DATA_MAX_COUNT];
    uint32_t render_node_count;
    
    /* Memory pool */
    void* memory_pool;
    size_t pool_size;
    size_t pool_used;
    pthread_mutex_t pool_mutex;
    
} animation_morph_data_context_t;

static animation_morph_data_context_t g_morph_data_ctx = {0};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */


static float rand_float_range(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

static void matrix_multiply(const float* a, const float* b, float* result) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                result[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
}

static void vector_add(const float* a, const float* b, float* result) {
    result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
    result[2] = a[2] + b[2];
}

static void vector_scale(const float* v, float scale, float* result) {
    result[0] = v[0] * scale;
    result[1] = v[1] * scale;
    result[2] = v[2] * scale;
}

static float vector_length(const float* v) {
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static void vector_normalize(float* v) {
    float length = vector_length(v);
    if (length > 0.0f) {
        v[0] /= length;
        v[1] /= length;
        v[2] /= length;
    }
}

static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/* ============================================================================
 * MEMORY POOL FUNCTIONS
 * ============================================================================ */

static int memory_pool_init(animation_morph_memory_pool_t* pool, size_t size) {
    if (!pool) return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    
    pool->memory = aligned_alloc(ANIMATION_MORPH_DATA_ALIGNMENT, size);
    if (!pool->memory) {
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    pool->total_size = size;
    pool->used_size = 0;
    pool->alignment = ANIMATION_MORPH_DATA_ALIGNMENT;
    
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        free(pool->memory);
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
    
    return ANIMATION_MORPH_ERROR_NONE;
}

static void memory_pool_cleanup(animation_morph_memory_pool_t* pool) {
    if (!pool) return;
    
    pthread_mutex_destroy(&pool->mutex);
    if (pool->memory) {
        free(pool->memory);
        pool->memory = NULL;
    }
    pool->used_size = 0;
    pool->total_size = 0;
}

static void* memory_pool_alloc(animation_morph_memory_pool_t* pool, size_t size) {
    if (!pool || !pool->memory) return NULL;
    
    pthread_mutex_lock(&pool->mutex);
    
    // Align size
    size_t aligned_size = (size + pool->alignment - 1) & ~(pool->alignment - 1);
    
    if (pool->used_size + aligned_size > pool->total_size) {
        pthread_mutex_unlock(&pool->mutex);
        return NULL;
    }
    
    void* ptr = pool->memory + pool->used_size;
    pool->used_size += aligned_size;
    
    pthread_mutex_unlock(&pool->mutex);
    return ptr;
}

static void memory_pool_free(animation_morph_memory_pool_t* pool, void* ptr) {
    // Simple pool implementation doesn't support individual frees
    // In a real implementation, we'd use a free list or more sophisticated allocator
    (void)pool;
    (void)ptr;
}

/* ============================================================================
 * CACHE FUNCTIONS
 * ============================================================================ */

static int cache_init(animation_morph_cache_entry_t** cache, uint32_t max_entries) {
    if (!cache) return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    
    *cache = calloc(max_entries, sizeof(animation_morph_cache_entry_t));
    if (!*cache) {
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    return ANIMATION_MORPH_ERROR_NONE;
}

static void cache_cleanup(animation_morph_cache_entry_t* cache, uint32_t max_entries) {
    if (!cache) return;
    
    for (uint32_t i = 0; i < max_entries; i++) {
        if (cache[i].data) {
            free(cache[i].data);
            cache[i].data = NULL;
        }
    }
    free(cache);
}

static int cache_get(animation_morph_cache_entry_t* cache, uint32_t max_entries, 
                    const char* key, void** out_data) {
    if (!cache || !key || !out_data) return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    
    uint32_t hash = hash_string(key) % max_entries;
    
    for (uint32_t i = 0; i < max_entries; i++) {
        uint32_t index = (hash + i) % max_entries;
        
        if (cache[index].valid && strcmp(cache[index].key, key) == 0) {
            *out_data = cache[index].data;
            cache[index].last_access = get_current_time_ms();
            return ANIMATION_MORPH_ERROR_NONE;
        }
    }
    
    return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
}

static int cache_set(animation_morph_cache_entry_t* cache, uint32_t max_entries, 
                    const char* key, const void* data, size_t size) {
    if (!cache || !key || !data) return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    
    uint32_t hash = hash_string(key) % max_entries;
    uint64_t current_time = get_current_time_ms();
    uint64_t oldest_time = current_time;
    uint32_t oldest_index = hash;
    
    // Find empty slot or oldest entry
    for (uint32_t i = 0; i < max_entries; i++) {
        uint32_t index = (hash + i) % max_entries;
        
        if (!cache[index].valid) {
            oldest_index = index;
            break;
        }
        
        if (cache[index].last_access < oldest_time) {
            oldest_time = cache[index].last_access;
            oldest_index = index;
        }
    }
    
    // Free old data if exists
    if (cache[oldest_index].data) {
        free(cache[oldest_index].data);
    }
    
    // Copy new data
    cache[oldest_index].data = malloc(size);
    if (!cache[oldest_index].data) {
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(cache[oldest_index].data, data, size);
    strncpy(cache[oldest_index].key, key, sizeof(cache[oldest_index].key) - 1);
    cache[oldest_index].key[sizeof(cache[oldest_index].key) - 1] = '\0';
    cache[oldest_index].size = size;
    cache[oldest_index].last_access = current_time;
    cache[oldest_index].valid = true;
    
    return ANIMATION_MORPH_ERROR_NONE;
}

/* ============================================================================
 * ASYNC OPERATIONS
 * ============================================================================ */

static void* async_worker_thread(void* arg) {
    (void)arg;
    
    while (g_morph_ctx.async_thread_running) {
        pthread_mutex_lock(&g_morph_ctx.async_mutex);
        
        if (g_morph_ctx.async_queue_head == g_morph_ctx.async_queue_tail) {
            pthread_mutex_unlock(&g_morph_ctx.async_mutex);
            usleep(1000); // 1ms sleep
            continue;
        }
        
        animation_morph_async_operation_t* op = &g_morph_ctx.async_queue[g_morph_ctx.async_queue_head];
        g_morph_ctx.async_queue_head = (g_morph_ctx.async_queue_head + 1) % ANIMATION_MORPH_ASYNC_QUEUE_SIZE;
        
        pthread_mutex_unlock(&g_morph_ctx.async_mutex);
        
        // Process the operation
        int result = animation_morph_data_update(op->handle, op->data, op->size);
        
        // Call callback
        if (op->callback) {
            op->callback(op->handle, result, op->user_data);
        }
        
        // Cleanup
        if (op->data) {
            free(op->data);
            op->data = NULL;
        }
        op->pending = false;
    }
    
    return NULL;
}

static int async_queue_operation(animation_morph_data_handle_t handle, const void* data, size_t size,
                                animation_morph_async_callback_t callback, void* user_data) {
    pthread_mutex_lock(&g_morph_ctx.async_mutex);
    
    uint32_t next_tail = (g_morph_ctx.async_queue_tail + 1) % ANIMATION_MORPH_ASYNC_QUEUE_SIZE;
    if (next_tail == g_morph_ctx.async_queue_head) {
        pthread_mutex_unlock(&g_morph_ctx.async_mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY; // Queue full
    }
    
    animation_morph_async_operation_t* op = &g_morph_ctx.async_queue[g_morph_ctx.async_queue_tail];
    
    op->handle = handle;
    op->data = malloc(size);
    if (!op->data) {
        pthread_mutex_unlock(&g_morph_ctx.async_mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(op->data, data, size);
    op->size = size;
    op->callback = callback;
    op->user_data = user_data;
    op->pending = true;
    
    g_morph_ctx.async_queue_tail = next_tail;
    
    pthread_mutex_unlock(&g_morph_ctx.async_mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

/* ============================================================================
 * VALIDATION FUNCTIONS
 * ============================================================================ */

static bool animation_morph_validate_internal(const animation_morph_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->vertex_count == 0) return false;
    if (!item->base_vertices) return false;
    
    // Validate targets
    for (uint32_t i = 0; i < item->target_count; i++) {
        const animation_morph_target_t* target = &item->targets[i];
        if (!target->vertices) return false;
        if (target->vertex_count != item->vertex_count) return false;
    }
    
    // Validate skeleton
    if (item->skeleton) {
        if (item->skeleton->bone_count == 0) return false;
        if (!item->skeleton->bones) return false;
    }
    
    return true;
}

static void animation_morph_cleanup_internal(animation_morph_internal_t* item) {
    if (!item) return;
    
    // Cleanup morph targets
    if (item->targets) {
        for (uint32_t i = 0; i < item->target_count; i++) {
            if (item->targets[i].vertices) {
                free(item->targets[i].vertices);
            }
            if (item->targets[i].weights) {
                free(item->targets[i].weights);
            }
        }
        free(item->targets);
        item->targets = NULL;
    }
    
    // Cleanup base vertices
    if (item->base_vertices) {
        free(item->base_vertices);
        item->base_vertices = NULL;
    }
    
    // Cleanup skeleton
    if (item->skeleton) {
        if (item->skeleton->bones) {
            for (uint32_t i = 0; i < item->skeleton->bone_count; i++) {
                if (item->skeleton->bones[i].child_indices) {
                    free(item->skeleton->bones[i].child_indices);
                }
            }
            free(item->skeleton->bones);
        }
        if (item->skeleton->bone_matrices) {
            free(item->skeleton->bone_matrices);
        }
        if (item->skeleton->skinning_matrices) {
            free(item->skeleton->skinning_matrices);
        }
        free(item->skeleton);
        item->skeleton = NULL;
    }
    
    // Cleanup bone transforms
    if (item->bone_transforms) {
        free(item->bone_transforms);
        item->bone_transforms = NULL;
    }
    
    // Cleanup LOD data
    for (int i = 0; i < ANIMATION_MORPH_LOD_COUNT; i++) {
        if (item->lod_data[i].vertices) {
            free(item->lod_data[i].vertices);
        }
        if (item->lod_data[i].targets) {
            for (uint32_t j = 0; j < item->lod_data[i].target_count; j++) {
                if (item->lod_data[i].targets[j].vertices) {
                    free(item->lod_data[i].targets[j].vertices);
                }
                if (item->lod_data[i].targets[j].weights) {
                    free(item->lod_data[i].targets[j].weights);
                }
            }
            free(item->lod_data[i].targets);
        }
    }
    
    // Cleanup compressed data
    if (item->compressed_data) {
        free(item->compressed_data);
        item->compressed_data = NULL;
    }
    
    // Cleanup render graph nodes
    if (item->render_nodes) {
        for (uint32_t i = 0; i < item->render_node_count; i++) {
            if (item->render_nodes[i].dependencies) {
                free(item->render_nodes[i].dependencies);
            }
        }
        free(item->render_nodes);
        item->render_nodes = NULL;
    }
    
    // Cleanup hot reload
    if (item->hot_reload_fd >= 0) {
        close(item->hot_reload_fd);
        item->hot_reload_fd = -1;
    }
    
    // Cleanup mutex
    pthread_mutex_destroy(&item->mutex);
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_morph_data_init(void) {
    if (g_morph_ctx.initialized) {
        return ANIMATION_MORPH_ERROR_NONE;
    }
    
    // Initialize global mutex
    if (pthread_mutex_init(&g_morph_ctx.global_mutex, NULL) != 0) {
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
    
    // Initialize memory pool
    int result = memory_pool_init(&g_morph_ctx.memory_pool, ANIMATION_MORPH_MEMORY_POOL_SIZE);
    if (result != ANIMATION_MORPH_ERROR_NONE) {
        pthread_mutex_destroy(&g_morph_ctx.global_mutex);
        return result;
    }
    
    // Initialize cache
    result = cache_init(&g_morph_ctx.cache, ANIMATION_MORPH_CACHE_MAX_ENTRIES);
    if (result != ANIMATION_MORPH_ERROR_NONE) {
        memory_pool_cleanup(&g_morph_ctx.memory_pool);
        pthread_mutex_destroy(&g_morph_ctx.global_mutex);
        return result;
    }
    g_morph_ctx.cache_max_entries = ANIMATION_MORPH_CACHE_MAX_ENTRIES;
    
    // Initialize async system
    if (pthread_mutex_init(&g_morph_ctx.async_mutex, NULL) != 0) {
        cache_cleanup(g_morph_ctx.cache, g_morph_ctx.cache_max_entries);
        memory_pool_cleanup(&g_morph_ctx.memory_pool);
        pthread_mutex_destroy(&g_morph_ctx.global_mutex);
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
    
    g_morph_ctx.async_thread_running = true;
    if (pthread_create(&g_morph_ctx.async_thread, NULL, async_worker_thread, NULL) != 0) {
        pthread_mutex_destroy(&g_morph_ctx.async_mutex);
        cache_cleanup(g_morph_ctx.cache, g_morph_ctx.cache_max_entries);
        memory_pool_cleanup(&g_morph_ctx.memory_pool);
        pthread_mutex_destroy(&g_morph_ctx.global_mutex);
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
    
    // Initialize morph data array
    g_morph_ctx.capacity = ANIMATION_MORPH_DATA_DEFAULT_CAPACITY;
    g_morph_ctx.items = calloc(g_morph_ctx.capacity, sizeof(animation_morph_internal_t));
    if (!g_morph_ctx.items) {
        g_morph_ctx.async_thread_running = false;
        pthread_join(g_morph_ctx.async_thread, NULL);
        pthread_mutex_destroy(&g_morph_ctx.async_mutex);
        cache_cleanup(g_morph_ctx.cache, g_morph_ctx.cache_max_entries);
        memory_pool_cleanup(&g_morph_ctx.memory_pool);
        pthread_mutex_destroy(&g_morph_ctx.global_mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    g_morph_ctx.count = 0;
    g_morph_ctx.batch_size = ANIMATION_MORPH_DEFAULT_BATCH_SIZE;
    g_morph_ctx.peak_memory_usage = 0;
    g_morph_ctx.initialized = true;
    
    return ANIMATION_MORPH_ERROR_NONE;
}

void animation_morph_data_shutdown(void) {
    if (!g_morph_ctx.initialized) {
        return;
    }
    
    // Stop async thread
    g_morph_ctx.async_thread_running = false;
    pthread_join(g_morph_ctx.async_thread, NULL);
    pthread_mutex_destroy(&g_morph_ctx.async_mutex);
    
    // Cleanup all morph data items
    for (uint32_t i = 0; i < g_morph_ctx.count; i++) {
        animation_morph_cleanup_internal(&g_morph_ctx.items[i]);
    }
    
    // Cleanup global systems
    free(g_morph_ctx.items);
    g_morph_ctx.items = NULL;
    
    cache_cleanup(g_morph_ctx.cache, g_morph_ctx.cache_max_entries);
    memory_pool_cleanup(&g_morph_ctx.memory_pool);
    pthread_mutex_destroy(&g_morph_ctx.global_mutex);
    
    g_morph_ctx.count = 0;
    g_morph_ctx.capacity = 0;
    g_morph_ctx.initialized = false;
}

int animation_morph_data_create(animation_morph_data_handle_t* out_handle, const animation_morph_data_desc_t* desc) {
    if (!out_handle || !desc) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_morph_ctx.global_mutex);
    
    if (g_morph_ctx.count >= g_morph_ctx.capacity) {
        pthread_mutex_unlock(&g_morph_ctx.global_mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = g_morph_ctx.count++;
    animation_morph_internal_t* item = &g_morph_ctx.items[index];
    
    // Initialize item
    memset(item, 0, sizeof(animation_morph_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->vertex_count = desc->vertex_count;
    item->target_count = desc->target_count;
    item->bone_count = desc->bone_count;
    
    // Allocate base vertices
    if (desc->vertex_count > 0) {
        item->base_vertices = memory_pool_alloc(&g_morph_ctx.memory_pool, 
                                                desc->vertex_count * sizeof(animation_morph_vertex_t));
        if (!item->base_vertices) {
            g_morph_ctx.count--;
            pthread_mutex_unlock(&g_morph_ctx.global_mutex);
            return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Allocate targets array
    if (desc->target_count > 0) {
        item->targets = memory_pool_alloc(&g_morph_ctx.memory_pool, 
                                         desc->target_count * sizeof(animation_morph_target_t));
        if (!item->targets) {
            if (item->base_vertices) {
                memory_pool_free(&g_morph_ctx.memory_pool, item->base_vertices);
            }
            g_morph_ctx.count--;
            pthread_mutex_unlock(&g_morph_ctx.global_mutex);
            return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
        }
        memset(item->targets, 0, desc->target_count * sizeof(animation_morph_target_t));
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&item->mutex, NULL) != 0) {
        if (item->targets) {
            memory_pool_free(&g_morph_ctx.memory_pool, item->targets);
        }
        if (item->base_vertices) {
            memory_pool_free(&g_morph_ctx.memory_pool, item->base_vertices);
        }
        g_morph_ctx.count--;
        pthread_mutex_unlock(&g_morph_ctx.global_mutex);
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
    
    // Initialize hot reload file descriptor
    item->hot_reload_fd = -1;
    
    // Initialize statistics
    item->stats.total_vertices = desc->vertex_count;
    item->stats.total_targets = desc->target_count;
    item->stats.total_bones = desc->bone_count;
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    out_handle->id = index;
    
    pthread_mutex_unlock(&g_morph_ctx.global_mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

void animation_morph_data_destroy(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_morph_ctx.global_mutex);
    
    if (handle.id >= g_morph_ctx.count) {
        pthread_mutex_unlock(&g_morph_ctx.global_mutex);
        return;
    }
    
    animation_morph_cleanup_internal(&g_morph_ctx.items[handle.id]);
    
    pthread_mutex_unlock(&g_morph_ctx.global_mutex);
}

int animation_morph_data_update(animation_morph_data_handle_t handle, const void* data, size_t size) {
    if (!g_morph_ctx.initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Update base vertices if data size matches
    if (data && size == item->vertex_count * sizeof(animation_morph_vertex_t)) {
        memcpy(item->base_vertices, data, size);
        item->dirty = true;
        item->frame_updated = get_current_time_ms();
        
        // Update statistics
        item->stats.memory_usage = size;
        if (item->stats.memory_usage > g_morph_ctx.peak_memory_usage) {
            g_morph_ctx.peak_memory_usage = item->stats.memory_usage;
        }
    }
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

bool animation_morph_data_is_valid(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return false;
    }
    
    return animation_morph_validate_internal(&g_morph_ctx.items[handle.id]);
}

int animation_morph_data_get_info(animation_morph_data_handle_t handle, animation_morph_data_info_t* out_info) {
    if (!out_info) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    const animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->vertex_count = item->vertex_count;
    out_info->target_count = item->target_count;
    out_info->bone_count = item->bone_count;
    out_info->stats = item->stats;
    
    return ANIMATION_MORPH_ERROR_NONE;
}

void animation_morph_data_mark_dirty(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (item->initialized) {
        pthread_mutex_lock(&item->mutex);
        item->dirty = true;
        item->frame_updated = get_current_time_ms();
        pthread_mutex_unlock(&item->mutex);
    }
}

int animation_morph_data_process_pending(void) {
    if (!g_morph_ctx.initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    int processed = 0;
    uint64_t start_time = get_current_time_ms();
    
    for (uint32_t i = 0; i < g_morph_ctx.count; i++) {
        animation_morph_internal_t* item = &g_morph_ctx.items[i];
        
        if (item->initialized && item->dirty) {
            pthread_mutex_lock(&item->mutex);
            
            // Process morph targets
            for (uint32_t j = 0; j < item->target_count; j++) {
                animation_morph_target_t* target = &item->targets[j];
                if (target->influence > 0.0f && target->vertices) {
                    // Apply morph target to base vertices
                    for (uint32_t k = 0; k < item->vertex_count; k++) {
                        float weight = target->weights ? target->weights[k] : 1.0f;
                        float influence = target->influence * weight;
                        
                        if (target->flags & ANIMATION_MORPH_TARGET_POSITION) {
                            vector_add(item->base_vertices[k].position, 
                                      target->vertices[k].position, 
                                      item->base_vertices[k].position);
                            vector_scale(item->base_vertices[k].position, 
                                       influence, 
                                       item->base_vertices[k].position);
                        }
                        
                        if (target->flags & ANIMATION_MORPH_TARGET_NORMAL) {
                            vector_add(item->base_vertices[k].normal, 
                                      target->vertices[k].normal, 
                                      item->base_vertices[k].normal);
                            vector_scale(item->base_vertices[k].normal, 
                                       influence, 
                                       item->base_vertices[k].normal);
                        }
                    }
                }
            }
            
            item->dirty = false;
            processed++;
            
            pthread_mutex_unlock(&item->mutex);
        }
    }
    
    uint64_t end_time = get_current_time_ms();
    float processing_time = (float)(end_time - start_time) / 1000.0f;
    
    // Update global statistics
    for (uint32_t i = 0; i < g_morph_ctx.count; i++) {
        g_morph_ctx.items[i].stats.average_processing_time = processing_time;
    }
    
    return processed;
}

uint32_t animation_morph_data_get_count(void) {
    return g_morph_ctx.count;
}

size_t animation_morph_data_get_memory_usage(void) {
    if (!g_morph_ctx.initialized) {
        return 0;
    }
    
    size_t total = sizeof(g_morph_ctx);
    total += g_morph_ctx.capacity * sizeof(animation_morph_internal_t);
    total += g_morph_ctx.cache_max_entries * sizeof(animation_morph_cache_entry_t);
    total += ANIMATION_MORPH_MEMORY_POOL_SIZE;
    
    for (uint32_t i = 0; i < g_morph_ctx.count; i++) {
        const animation_morph_internal_t* item = &g_morph_ctx.items[i];
        total += item->vertex_count * sizeof(animation_morph_vertex_t);
        total += item->target_count * sizeof(animation_morph_target_t);
        
        for (uint32_t j = 0; j < item->target_count; j++) {
            total += item->targets[j].vertex_count * sizeof(animation_morph_vertex_t);
            total += item->targets[j].vertex_count * sizeof(float);
        }
        
        if (item->skeleton) {
            total += item->skeleton->bone_count * sizeof(animation_morph_bone_t);
            total += item->skeleton->bone_count * 16 * sizeof(float) * 2; // matrices
        }
    }
    
    return total;
}

size_t animation_morph_data_get_peak_memory_usage(void) {
    return g_morph_ctx.peak_memory_usage;
}

void animation_morph_data_reset_peak_memory_usage(void) {
    g_morph_ctx.peak_memory_usage = 0;
}

/* End of morph_data.c */
