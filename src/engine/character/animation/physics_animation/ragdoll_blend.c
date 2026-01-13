/*
 * ragdoll_blend.c
 * Ragdoll-animation blending
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#include "character/animation/physics_animation/ragdoll_blend.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#elif defined(__linux__)
#include <immintrin.h>
#endif

#include "engine/renderer/gpu/gpu_resources.h"
#include "engine/physics/physics_engine.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/memory_pool.h"
#include "engine/core/thread_pool.h"
#include "engine/core/file_watcher.h"
#include "engine/core/compression.h"
#include <math.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <immintrin.h>  // For SIMD intrinsics

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_RAGDOLL_BLEND_MAX_COUNT 4096
#define ANIMATION_RAGDOLL_BLEND_DEFAULT_CAPACITY 256
#define ANIMATION_RAGDOLL_BLEND_ALIGNMENT 16
#define ANIMATION_RAGDOLL_BLEND_MAX_BONES 256
#define ANIMATION_RAGDOLL_BLEND_MAX_MORPH_TARGETS 128
#define ANIMATION_RAGDOLL_BLEND_MAX_ANIMATIONS 64
#define ANIMATION_RAGDOLL_BLEND_MAX_RIGIDBODIES 128
#define ANIMATION_RAGDOLL_BLEND_MAX_CONSTRAINTS 256
#define ANIMATION_RAGDOLL_BLEND_MAX_ASYNC_OPERATIONS 32
#define ANIMATION_RAGDOLL_BLEND_CACHE_SIZE 1024
#define ANIMATION_RAGDOLL_BLEND_LOD_LEVELS 4
#define ANIMATION_RAGDOLL_BLEND_BATCH_SIZE 64

/* Error codes */
#define ANIMATION_RAGDOLL_BLEND_ERROR_NONE 0
#define ANIMATION_RAGDOLL_BLEND_ERROR_INVALID_PARAM -1
#define ANIMATION_RAGDOLL_BLEND_ERROR_NOT_INITIALIZED -2
#define ANIMATION_RAGDOLL_BLEND_ERROR_OUT_OF_MEMORY -3
#define ANIMATION_RAGDOLL_BLEND_ERROR_CAPACITY_EXCEEDED -4
#define ANIMATION_RAGDOLL_BLEND_ERROR_INVALID_HANDLE -5
#define ANIMATION_RAGDOLL_BLEND_ERROR_SERIALIZATION_FAILED -6
#define ANIMATION_RAGDOLL_BLEND_ERROR_GPU_OPERATION_FAILED -7
#define ANIMATION_RAGDOLL_BLEND_ERROR_THREADING_ERROR -8
#define ANIMATION_RAGDOLL_BLEND_ERROR_HOT_RELOAD_FAILED -9
#define ANIMATION_RAGDOLL_BLEND_ERROR_VALIDATION_FAILED -10
#define ANIMATION_RAGDOLL_BLEND_MAX_BONES 256
#define ANIMATION_RAGDOLL_BLEND_MAX_KEYFRAMES 1024
#define ANIMATION_RAGDOLL_BLEND_CACHE_SIZE 128
#define ANIMATION_RAGDOLL_BLEND_ASYNC_QUEUE_SIZE 64
#define ANIMATION_RAGDOLL_BLEND_LOD_LEVELS 4
#define ANIMATION_RAGDOLL_BLEND_MAGIC_NUMBER 0x5242444C  // "RBDL"
#define ANIMATION_RAGDOLL_BLEND_VERSION 1

// Error codes
#define ANIMATION_RAGDOLL_BLEND_ERROR_NONE 0
#define ANIMATION_RAGDOLL_BLEND_ERROR_INVALID_PARAM -1
#define ANIMATION_RAGDOLL_BLEND_ERROR_NOT_INITIALIZED -2
#define ANIMATION_RAGDOLL_BLEND_ERROR_OUT_OF_MEMORY -3
#define ANIMATION_RAGDOLL_BLEND_ERROR_INVALID_HANDLE -4
#define ANIMATION_RAGDOLL_BLEND_ERROR_SERIALIZATION_FAILED -5
#define ANIMATION_RAGDOLL_BLEND_ERROR_GPU_FAILED -6
#define ANIMATION_RAGDOLL_BLEND_ERROR_THREAD_ERROR -7
#define ANIMATION_RAGDOLL_BLEND_ERROR_CACHE_FULL -8
#define ANIMATION_RAGDOLL_BLEND_ERROR_ASYNC_QUEUE_FULL -9
#define ANIMATION_RAGDOLL_BLEND_ERROR_VALIDATION_FAILED -10

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Vector types for SIMD operations
typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

typedef struct quat {
    float x, y, z, w;
} quat_t;

// Matrix type
typedef struct mat4 {
    float m[16];  // Column-major
} mat4_t;

// Bone transform
typedef struct bone_transform {
    quat_t rotation;
    vec3_t translation;
    vec3_t scale;
} bone_transform_t;

// Animation keyframe
typedef struct animation_keyframe {
    float time;
    bone_transform_t transform;
} animation_keyframe_t;

// Animation track
typedef struct animation_track {
    uint32_t bone_index;
    animation_keyframe_t* keyframes;
    uint32_t keyframe_count;
    uint32_t capacity;
} animation_track_t;

// Animation compression settings
typedef struct animation_compression {
    float position_threshold;
    float rotation_threshold;
    float scale_threshold;
    bool enable_keyframe_reduction;
    uint32_t max_keyframes_per_second;
} animation_compression_t;

// Procedural animation settings
typedef struct procedural_animation {
    float noise_frequency;
    float noise_amplitude;
    float wind_strength;
    vec3_t wind_direction;
    bool enabled;
} procedural_animation_t;

// Ragdoll physics body
typedef struct ragdoll_body {
    vec3_t position;
    quat_t orientation;
    vec3_t velocity;
    vec3_t angular_velocity;
    float mass;
    float damping;
    bool enabled;
} ragdoll_body_t;

// Animation retargeting mapping
typedef struct retarget_mapping {
    int32_t source_bone_index;
    int32_t target_bone_index;
    quat_t rotation_offset;
    vec3_t position_offset;
    float scale_factor;
} retarget_mapping_t;

// LOD level settings
typedef struct lod_settings {
    float distance_threshold;
    uint32_t bone_reduction_factor;
    uint32_t keyframe_reduction_factor;
    bool enable_compression;
} lod_settings_t;

// Performance counters
typedef struct performance_counters {
    uint64_t total_updates;
    uint64_t total_render_time_ns;
    uint64_t total_physics_time_ns;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_upload_time_ns;
    uint64_t serialization_time_ns;
    uint32_t active_bodies;
    uint32_t compressed_animations;
    float average_fps;
} performance_counters_t;

// Cache entry
typedef struct cache_entry {
    uint32_t id;
    void* data;
    size_t size;
    uint64_t last_access_time;
    uint32_t access_count;
    bool valid;
} cache_entry_t;

// Async operation
typedef struct async_operation {
    uint32_t id;
    uint32_t type;
    void* data;
    size_t data_size;
    bool completed;
    int result;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
} async_operation_t;

// Render graph node
typedef struct render_graph_node {
    uint32_t id;
    uint32_t dependency_count;
    uint32_t* dependencies;
    void (*execute_func)(void* user_data);
    void* user_data;
    bool executed;
} render_graph_node_t;

// Extended internal structure
typedef struct animation_ragdoll_blend_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    // GPU skinning data
    uint32_t bone_count;
    bone_transform_t* bone_transforms;
    mat4_t* bone_matrices;
    uint32_t gpu_buffer_id;
    
    // Animation data
    animation_track_t* tracks;
    uint32_t track_count;
    animation_compression_t compression;
    float current_time;
    float duration;
    
    // Procedural animation
    procedural_animation_t procedural;
    
    // Ragdoll physics
    ragdoll_body_t* bodies;
    uint32_t body_count;
    float blend_weight;
    
    // Retargeting
    retarget_mapping_t* retarget_mappings;
    uint32_t retarget_mapping_count;
    
    // LOD
    lod_settings_t lod_settings[ANIMATION_RAGDOLL_BLEND_LOD_LEVELS];
    uint32_t current_lod;
    
    // Culling
    bool visible;
    float distance_to_viewer;
    
    // Render graph
    uint32_t render_node_id;
    
} animation_ragdoll_blend_internal_t;

// Extended context
typedef struct animation_ragdoll_blend_context {
    animation_ragdoll_blend_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    // Thread safety
    pthread_mutex_t global_mutex;
    pthread_rwlock_t data_rwlock;
    
    // Performance counters
    performance_counters_t perf_counters;
    
    // Cache
    cache_entry_t cache[ANIMATION_RAGDOLL_BLEND_CACHE_SIZE];
    pthread_mutex_t cache_mutex;
    
    // Async operations
    async_operation_t async_queue[ANIMATION_RAGDOLL_BLEND_ASYNC_QUEUE_SIZE];
    uint32_t async_queue_head;
    uint32_t async_queue_tail;
    pthread_mutex_t async_mutex;
    pthread_t async_worker_thread;
    bool async_thread_running;
    
    // Hot-reload
    int inotify_fd;
    pthread_t file_watcher_thread;
    bool file_watcher_running;
    
    // Render graph
    render_graph_node_t* render_nodes;
    uint32_t render_node_count;
    uint32_t render_node_capacity;
    
    // Memory tracking
    size_t total_memory_allocated;
    size_t peak_memory_usage;
    
} animation_ragdoll_blend_context_t;

static animation_ragdoll_blend_context_t g_ragdoll_blend_ctx = {0};

/* Helper function prototypes */
static bool validate_skeleton(const bone_transform_t* skeleton, size_t bone_count);
static bool validate_morph_targets(const morph_target_t* targets, size_t target_count);
static bool validate_ik_chains(const ik_chain_t* chains, size_t chain_count);
static bool validate_ragdoll_physics(const ragdoll_body_t* bodies, size_t body_count);
static void blend_animations(animation_ragdoll_blend_internal_t* item);
static void solve_ik_chains(animation_ragdoll_blend_internal_t* item);
static void update_ragdoll_physics(animation_ragdoll_blend_internal_t* item);
static void update_gpu_skinning(animation_ragdoll_blend_internal_t* item);
static void compress_animation_data(animation_ragdoll_blend_internal_t* item);
static void decompress_animation_data(animation_ragdoll_blend_internal_t* item);
static void retarget_animation(animation_ragdoll_blend_internal_t* item);
static void update_morph_targets(animation_ragdoll_blend_internal_t* item);
static void process_lod(animation_ragdoll_blend_internal_t* item);
static void execute_render_graph(animation_ragdoll_blend_internal_t* item);
static uint32_t calculate_cache_hash(const void* data, size_t size);
static void* lookup_cache(animation_ragdoll_blend_internal_t* item, uint32_t hash);
static void store_cache(animation_ragdoll_blend_internal_t* item, uint32_t hash, const void* data, size_t size);
static void* async_worker_thread(void* arg);
static void hot_reload_callback(const char* filename, void* user_data);
static void serialize_to_buffer(const animation_ragdoll_blend_internal_t* item, uint8_t* buffer, size_t* size);
static void deserialize_from_buffer(animation_ragdoll_blend_internal_t* item, const uint8_t* buffer, size_t size);
static void update_performance_counters(animation_ragdoll_blend_internal_t* item, double operation_time);
static void simd_blend_transforms(bone_transform_t* result, const bone_transform_t* a, const bone_transform_t* b, float weight, size_t count);
static void batch_process_items(animation_ragdoll_blend_internal_t* items, size_t count);
static bool cull_item(const animation_ragdoll_blend_internal_t* item, const vec3_t* view_pos, float cull_distance);
static void stream_data(animation_ragdoll_blend_internal_t* item, float stream_distance);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

/* ============================================================================
 * PRIVATE FUNCTIONS - Math Utilities
 * ============================================================================ */

static vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

static vec3_t vec3_subtract(vec3_t a, vec3_t b) {
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

static vec3_t vec3_multiply(vec3_t v, float s) {
    return (vec3_t){v.x * s, v.y * s, v.z * s};
}

static float vec3_length(vec3_t v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static quat_t quat_normalize(quat_t q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len > 0.0f) {
        return (quat_t){q.x / len, q.y / len, q.z / len, q.w / len};
    }
    return (quat_t){0, 0, 0, 1};
}

static quat_t quat_slerp(quat_t q1, quat_t q2, float t) {
    float dot = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
    
    if (dot < 0.0f) {
        q2.x = -q2.x; q2.y = -q2.y; q2.z = -q2.z; q2.w = -q2.w;
        dot = -dot;
    }
    
    if (dot > 0.9995f) {
        return quat_normalize((quat_t){
            q1.x + t * (q2.x - q1.x),
            q1.y + t * (q2.y - q1.y),
            q1.z + t * (q2.z - q1.z),
            q1.w + t * (q2.w - q1.w)
        });
    }
    
    float theta = acosf(dot);
    float sin_theta = sinf(theta);
    float factor1 = sinf((1.0f - t) * theta) / sin_theta;
    float factor2 = sinf(t * theta) / sin_theta;
    
    return quat_normalize((quat_t){
        factor1 * q1.x + factor2 * q2.x,
        factor1 * q1.y + factor2 * q2.y,
        factor1 * q1.z + factor2 * q2.z,
        factor1 * q1.w + factor2 * q2.w
    });
}

static float perlin_noise_1d(float x) {
    int xi = (int)floorf(x) & 255;
    float xf = x - floorf(x);
    float u = xf * xf * (3.0f - 2.0f * xf);
    return lerp(u, 0.0f, 1.0f);
}

/* ============================================================================
 * PRIVATE FUNCTIONS - System Implementation
 * ============================================================================ */

static bool animation_ragdoll_blend_validate(const animation_ragdoll_blend_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->bone_count > ANIMATION_RAGDOLL_BLEND_MAX_BONES) return false;
    if (item->track_count > ANIMATION_RAGDOLL_BLEND_MAX_BONES) return false;
    return true;
}

static void animation_ragdoll_blend_cleanup_internal(animation_ragdoll_blend_internal_t* item) {
    if (!item) return;
    
    // Free animation data
    if (item->tracks) {
        for (uint32_t i = 0; i < item->track_count; i++) {
            if (item->tracks[i].keyframes) {
                free(item->tracks[i].keyframes);
            }
                free(item->animations[i].morph_targets);
            }
        }
        free(item->animations);
        item->animations = NULL;
    }
    
    /* Cleanup morph targets */
    if (item->morph_targets) {
        for (size_t i = 0; i < item->morph_target_count; i++) {
            if (item->morph_targets[i].vertices) {
                free(item->morph_targets[i].vertices);
            }
        }
        free(item->morph_targets);
        item->morph_targets = NULL;
    }
    
    /* Cleanup IK chains */
    if (item->ik_chains) {
        for (size_t i = 0; i < item->ik_chain_count; i++) {
            if (item->ik_chains[i].bone_indices) {
                free(item->ik_chains[i].bone_indices);
            }
        }
        free(item->ik_chains);
        item->ik_chains = NULL;
    }
    
    /* Cleanup ragdoll physics */
    if (item->ragdoll_bodies) {
        free(item->ragdoll_bodies);
        item->ragdoll_bodies = NULL;
    }
    
    /* Cleanup GPU skinning */
    if (item->gpu_skinning.gpu_accelerated) {
        gpu_buffer_destroy(&item->gpu_skinning.bone_buffer);
        gpu_buffer_destroy(&item->gpu_skinning.morph_buffer);
        gpu_shader_destroy(&item->gpu_skinning.skinning_shader);
        item->gpu_skinning.gpu_accelerated = false;
    }
    
    /* Cleanup compression */
    if (item->compression.compressed_data) {
        free(item->compression.compressed_data);
        item->compression.compressed_data = NULL;
    }
    
    /* Cleanup retargeting */
    if (item->bone_mappings) {
        free(item->bone_mappings);
        item->bone_mappings = NULL;
    }
    
    /* Cleanup async operations */
    for (uint32_t i = 0; i < item->async_op_count; i++) {
        pthread_cond_destroy(&item->async_ops[i].completion_cond);
        pthread_mutex_destroy(&item->async_ops[i].completion_mutex);
        if (item->async_ops[i].input_data) {
            free(item->async_ops[i].input_data);
        }
        if (item->async_ops[i].output_data) {
            free(item->async_ops[i].output_data);
        }
    }
    
    /* Cleanup render graph */
    for (uint32_t i = 0; i < item->render_node_count; i++) {
        if (item->render_nodes[i].dependencies) {
            free(item->render_nodes[i].dependencies);
        }
    }
    
    /* Cleanup hot-reload */
    if (item->hot_reload.file_watcher) {
        file_watcher_destroy(item->hot_reload.file_watcher);
        item->hot_reload.file_watcher = NULL;
    }
    
    /* Cleanup cache */
    for (uint32_t i = 0; i < ANIMATION_RAGDOLL_BLEND_CACHE_SIZE; i++) {
        if (item->cache[i].data) {
            free(item->cache[i].data);
            item->cache[i].data = NULL;
        }
    }
    
    /* Cleanup original data */
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    pthread_mutex_unlock(&item->access_mutex);
    pthread_mutex_destroy(&item->access_mutex);
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_ragdoll_blend_init(void) {
    if (g_ragdoll_blend_ctx.initialized) {
        return ANIMATION_RAGDOLL_BLEND_ERROR_NONE;
    }

    /* Initialize global mutex */
    if (pthread_mutex_init(&g_ragdoll_blend_ctx.global_mutex, NULL) != 0) {
        return ANIMATION_RAGDOLL_BLEND_ERROR_THREADING_ERROR;
    }

    /* Initialize memory pool */
    g_ragdoll_blend_ctx.memory_pool = memory_pool_create(1024 * 1024, ANIMATION_RAGDOLL_BLEND_ALIGNMENT);
    if (!g_ragdoll_blend_ctx.memory_pool) {
        pthread_mutex_destroy(&g_ragdoll_blend_ctx.global_mutex);
        return ANIMATION_RAGDOLL_BLEND_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize thread pool for async operations */
    g_ragdoll_blend_ctx.thread_pool = thread_pool_create(4, ANIMATION_RAGDOLL_BLEND_MAX_ASYNC_OPERATIONS);
    if (!g_ragdoll_blend_ctx.thread_pool) {
        memory_pool_destroy(g_ragdoll_blend_ctx.memory_pool);
        pthread_mutex_destroy(&g_rdoll_blend_ctx.global_mutex);
        return ANIMATION_RAGDOLL_BLEND_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize main array */
    g_ragdoll_blend_ctx.capacity = ANIMATION_RAGDOLL_BLEND_DEFAULT_CAPACITY;
    g_ragdoll_blend_ctx.items = calloc(g_ragdoll_blend_ctx.capacity, sizeof(animation_ragdoll_blend_internal_t));
    if (!g_ragdoll_blend_ctx.items) {
        thread_pool_destroy(g_ragdoll_blend_ctx.thread_pool);
        memory_pool_destroy(g_ragdoll_blend_ctx.memory_pool);
        pthread_mutex_destroy(&g_ragdoll_blend_ctx.global_mutex);
        return ANIMATION_RAGDOLL_BLEND_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize serialization buffer */
    g_ragdoll_blend_ctx.serialization_buffer_size = 1024 * 1024; // 1MB
    g_ragdoll_blend_ctx.serialization_buffer = malloc(g_ragdoll_blend_ctx.serialization_buffer_size);
    if (!g_ragdoll_blend_ctx.serialization_buffer) {
        free(g_ragdoll_blend_ctx.items);
        thread_pool_destroy(g_ragdoll_blend_ctx.thread_pool);
        memory_pool_destroy(g_ragdoll_blend_ctx.memory_pool);
        pthread_mutex_destroy(&g_ragdoll_blend_ctx.global_mutex);
        return ANIMATION_RAGDOLL_BLEND_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize global cache */
    memset(g_ragdoll_blend_ctx.global_cache, 0, sizeof(g_ragdoll_blend_ctx.global_cache));
    g_ragdoll_blend_ctx.global_cache_index = 0;

    /* Initialize global performance counters */
    memset(&g_ragdoll_blend_ctx.global_performance, 0, sizeof(g_ragdoll_blend_ctx.global_performance));

    g_ragdoll_blend_ctx.count = 0;
    g_ragdoll_blend_ctx.initialized = true;

    return ANIMATION_RAGDOLL_BLEND_ERROR_NONE;
}

void animation_ragdoll_blend_shutdown(void) {
    if (!g_ragdoll_blend_ctx.initialized) {
        return;
    }

    pthread_mutex_lock(&g_ragdoll_blend_ctx.global_mutex);

    /* Cleanup all items */
    for (uint32_t i = 0; i < g_ragdoll_blend_ctx.count; i++) {
        animation_ragdoll_blend_cleanup_internal(&g_ragdoll_blend_ctx.items[i]);
    }

    /* Cleanup global cache */
    for (uint32_t i = 0; i < ANIMATION_RAGDOLL_BLEND_CACHE_SIZE; i++) {
        if (g_ragdoll_blend_ctx.global_cache[i].data) {
            free(g_ragdoll_blend_ctx.global_cache[i].data);
            g_ragdoll_blend_ctx.global_cache[i].data = NULL;
        }
    }

    /* Cleanup serialization buffer */
    if (g_ragdoll_blend_ctx.serialization_buffer) {
        free(g_ragdoll_blend_ctx.serialization_buffer);
        g_ragdoll_blend_ctx.serialization_buffer = NULL;
    }

    /* Cleanup main array */
    free(g_ragdoll_blend_ctx.items);
    g_ragdoll_blend_ctx.items = NULL;

    pthread_mutex_unlock(&g_ragdoll_blend_ctx.global_mutex);

    /* Cleanup thread pool and memory pool */
    if (g_ragdoll_blend_ctx.thread_pool) {
        thread_pool_destroy(g_ragdoll_blend_ctx.thread_pool);
        g_ragdoll_blend_ctx.thread_pool = NULL;
    }

    if (g_ragdoll_blend_ctx.memory_pool) {
        memory_pool_destroy(g_ragdoll_blend_ctx.memory_pool);
        g_ragdoll_blend_ctx.memory_pool = NULL;
    }

    /* Destroy global mutex */
    pthread_mutex_destroy(&g_ragdoll_blend_ctx.global_mutex);

    g_ragdoll_blend_ctx.count = 0;
    g_ragdoll_blend_ctx.capacity = 0;
    g_ragdoll_blend_ctx.initialized = false;
}

int animation_ragdoll_blend_create(animation_ragdoll_blend_handle_t* out_handle, const animation_ragdoll_blend_desc_t* desc) {
    if (!out_handle || !desc) {
        return ANIMATION_RAGDOLL_BLEND_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_ragdoll_blend_ctx.global_mutex);

    if (!g_ragdoll_blend_ctx.initialized) {
        pthread_mutex_unlock(&g_ragdoll_blend_ctx.global_mutex);
        return ANIMATION_RAGDOLL_BLEND_ERROR_NOT_INITIALIZED;
    }

    if (g_ragdoll_blend_ctx.count >= g_ragdoll_blend_ctx.capacity) {
        pthread_mutex_unlock(&g_ragdoll_blend_ctx.global_mutex);
        return ANIMATION_RAGDOLL_BLEND_ERROR_CAPACITY_EXCEEDED;
    }

    uint32_t index = g_ragdoll_blend_ctx.count++;
    animation_ragdoll_blend_internal_t* item = &g_ragdoll_blend_ctx.items[index];

    /* Initialize item */
    memset(item, 0, sizeof(animation_ragdoll_blend_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    /* Initialize thread safety */
    if (pthread_mutex_init(&item->access_mutex, NULL) != 0) {
        g_ragdoll_blend_ctx.count--;
        pthread_mutex_unlock(&g_ragdoll_blend_ctx.global_mutex);
        return ANIMATION_RAGDOLL_BLEND_ERROR_THREADING_ERROR;
    }

    /* Initialize LOD levels */
    for (uint32_t i = 0; i < ANIMATION_RAGDOLL_BLEND_LOD_LEVELS; i++) {
        item->lod_levels[i].distance_threshold = (float)(i + 1) * 10.0f;
        item->lod_levels[i].bone_update_rate = 60 / (i + 1);
        item->lod_levels[i].morph_update_rate = 30 / (i + 1);
        item->lod_levels[i].ik_solver_tolerance = 0.01f * (i + 1);
        item->lod_levels[i].enable_physics = (i == 0);
        item->lod_levels[i].enable_gpu_skinning = (i < 2);
    }
    item->current_lod = 0;

    /* Initialize cache */
    memset(item->cache, 0, sizeof(item->cache));
    item->cache_index = 0;

    /* Initialize performance counters */
    memset(&item->performance, 0, sizeof(item->performance));

    /* Initialize async operations */
    for (uint32_t i = 0; i < ANIMATION_RAGDOLL_BLEND_MAX_ASYNC_OPERATIONS; i++) {
        pthread_cond_init(&item->async_ops[i].completion_cond, NULL);
        pthread_mutex_init(&item->async_ops[i].completion_mutex, NULL);
        item->async_ops[i].id = i;
        item->async_ops[i].completed = true;
    }
    item->async_op_count = 0;

    /* Initialize hot-reload */
    memset(&item->hot_reload, 0, sizeof(item->hot_reload));

    out_handle->id = index;
    pthread_mutex_unlock(&g_ragdoll_blend_ctx.global_mutex);

    return ANIMATION_RAGDOLL_BLEND_ERROR_NONE;
}

void animation_ragdoll_blend_destroy(animation_ragdoll_blend_handle_t handle) {
    pthread_mutex_lock(&g_ragdoll_blend_ctx.global_mutex);

    if (handle.id >= g_ragdoll_blend_ctx.count) {
        pthread_mutex_unlock(&g_ragdoll_blend_ctx.global_mutex);
        return;
    }

    animation_ragdoll_blend_internal_t* item = &g_ragdoll_blend_ctx.items[handle.id];
    
    /* Update global performance counters */
    g_ragdoll_blend_ctx.global_performance.total_update_time += item->performance.total_update_time;
    g_ragdoll_blend_ctx.global_performance.updates_performed += item->performance.updates_performed;
    g_ragdoll_blend_ctx.global_performance.blends_computed += item->performance.blends_computed;
    g_ragdoll_blend_ctx.global_performance.ik_solves_performed += item->performance.ik_solves_performed;
    g_ragdoll_blend_ctx.global_performance.physics_updates += item->performance.physics_updates;
    g_ragdoll_blend_ctx.global_performance.gpu_operations += item->performance.gpu_operations;
    g_ragdoll_blend_ctx.global_performance.cache_hits += item->performance.cache_hits;
    g_ragdoll_blend_ctx.global_performance.cache_misses += item->performance.cache_misses;
    g_ragdoll_blend_ctx.global_performance.async_operations += item->performance.async_operations;

    animation_ragdoll_blend_cleanup_internal(item);

    pthread_mutex_unlock(&g_ragdoll_blend_ctx.global_mutex);
}

int animation_ragdoll_blend_update(animation_ragdoll_blend_handle_t handle, const void* data, size_t size) {
    // TODO: Add ragdoll blend thread safety
    // TODO: Implement ragdoll blend memory pooling
    // TODO: Add ragdoll blend caching layer
    // TODO: Implement ragdoll blend async operations

    if (handle.id >= g_ragdoll_blend_ctx.count) {
        return -1;
    }

    animation_ragdoll_blend_internal_t* item = &g_ragdoll_blend_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // TODO: Add ragdoll blend GPU integration
    // TODO: Implement ragdoll blend SIMD optimization

    item->dirty = true;
    return 0;
}

bool animation_ragdoll_blend_is_valid(animation_ragdoll_blend_handle_t handle) {
    // TODO: Add ragdoll blend batch processing
    if (handle.id >= g_ragdoll_blend_ctx.count) {
        return false;
    }
    return g_ragdoll_blend_ctx.items[handle.id].initialized;
}

int animation_ragdoll_blend_get_info(animation_ragdoll_blend_handle_t handle, animation_ragdoll_blend_info_t* out_info) {
    // TODO: Implement ragdoll blend streaming support
    // TODO: Add ragdoll blend LOD support

    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_ragdoll_blend_ctx.count) {
        return -2;
    }

    const animation_ragdoll_blend_internal_t* item = &g_ragdoll_blend_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void animation_ragdoll_blend_mark_dirty(animation_ragdoll_blend_handle_t handle) {
    // TODO: Implement ragdoll blend culling integration
    if (handle.id < g_ragdoll_blend_ctx.count) {
        g_ragdoll_blend_ctx.items[handle.id].dirty = true;
    }
}

int animation_ragdoll_blend_process_pending(void) {
    // TODO: Add ragdoll blend render graph node
    // TODO: Implement batch processing

    int processed = 0;
    for (uint32_t i = 0; i < g_ragdoll_blend_ctx.count; i++) {
        animation_ragdoll_blend_internal_t* item = &g_ragdoll_blend_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t animation_ragdoll_blend_get_count(void) {
    return g_ragdoll_blend_ctx.count;
}

size_t animation_ragdoll_blend_get_memory_usage(void) {
    // TODO: Implement memory tracking
    size_t total = sizeof(g_ragdoll_blend_ctx);
    total += g_ragdoll_blend_ctx.capacity * sizeof(animation_ragdoll_blend_internal_t);

    for (uint32_t i = 0; i < g_ragdoll_blend_ctx.count; i++) {
        total += g_ragdoll_blend_ctx.items[i].data_size;
    }

    return total;
}

void animation_ragdoll_blend_debug_print(void) {
    // TODO: Implement debug output
    // Debug printing implementation
}

/* End of ragdoll_blend.c */
