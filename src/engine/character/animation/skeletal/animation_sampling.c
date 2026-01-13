/*
 * animation_sampling.c
 * Keyframe sampling
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
 * TODO: Implement animation sampling initialization
 * TODO: Add animation sampling cleanup/shutdown
 * TODO: Implement animation sampling validation
 * TODO: Add animation sampling error handling
 * TODO: Implement animation sampling serialization
 * TODO: Add animation sampling debug output
 * TODO: Implement animation sampling unit tests
 * TODO: Add animation sampling performance counters
 * TODO: Implement animation sampling hot-reload
 * TODO: Add animation sampling thread safety
 * TODO: Implement animation sampling memory pooling
 * TODO: Add animation sampling caching layer
 * TODO: Implement animation sampling async operations
 * TODO: Add animation sampling GPU integration
 * TODO: Implement animation sampling SIMD optimization
 * TODO: Add animation sampling batch processing
 * TODO: Implement animation sampling streaming support
 * TODO: Add animation sampling LOD support
 * TODO: Implement animation sampling culling integration
 * TODO: Add animation sampling render graph node
 */

#include "character/animation/skeletal/animation_sampling.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <immintrin.h>
#include <time.h>

/* SIMD and GPU includes */
#ifdef __APPLE__
#include <Metal/Metal.h>
#include <QuartzCore/QuartzCore.h>
#else
#include <vulkan/vulkan.h>
#endif
#include <math.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

/* SIMD Headers */
#if defined(__SSE2__)
#include <emmintrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#endif

/* Compression Headers */
#ifdef USE_LZ4
#include <lz4.h>
#endif
#ifdef USE_ZSTD
#include <zstd.h>
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_ANIMATION_SAMPLING_MAX_COUNT 4096
#define ANIMATION_ANIMATION_SAMPLING_DEFAULT_CAPACITY 256
#define ANIMATION_ANIMATION_SAMPLING_ALIGNMENT 16
#define ANIMATION_MAX_BONES 256
#define ANIMATION_MAX_MORPH_TARGETS 64
#define ANIMATION_MAX_RIGIDBODIES 128
#define ANIMATION_CACHE_SIZE 1024
#define ANIMATION_MAX_ASYNC_OPERATIONS 64
#define ANIMATION_LOD_LEVELS 4
#define ANIMATION_RENDER_GRAPH_NODES 32
#define ANIMATION_SAMPLING_MAX_BONES 256
#define ANIMATION_SAMPLING_MAX_CHANNELS 1024
#define ANIMATION_SAMPLING_CACHE_SIZE 512
#define ANIMATION_SAMPLING_MAX_ASYNC_OPERATIONS 64
#define ANIMATION_SAMPLING_LOD_LEVELS 4
#define ANIMATION_SAMPLING_MAX_RAGDOLL_BONES 64
#define ANIMATION_SAMPLING_MAX_IK_CHAINS 16
#define ANIMATION_SAMPLING_MAX_PROCEDURAL_LAYERS 8
#define ANIMATION_SAMPLING_MAX_RETARGET_MAPPINGS 128
#define ANIMATION_SAMPLING_MAX_COMPRESSION_RATIO 0.1f
#define ANIMATION_SAMPLING_THREAD_STACK_SIZE (1024 * 1024)

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Animation Compression Types */
typedef enum {
    ANIMATION_COMPRESSION_NONE = 0,
    ANIMATION_COMPRESSION_QUANTIZATION,
    ANIMATION_COMPRESSION_KEYFRAME_REDUCTION,
    ANIMATION_COMPRESSION_CURVE_FITTING,
    ANIMATION_COMPRESSION_WAVELET
} animation_compression_type_t;

/* Procedural Animation Types */
typedef enum {
    PROCEDURAL_TYPE_NONE = 0,
    PROCEDURAL_TYPE_WALK_CYCLE,
    PROCEDURAL_TYPE_BREATHING,
    PROCEDURAL_TYPE_HEAD_TRACK,
    PROCEDURAL_TYPE_FOOT_IK,
    PROCEDURAL_TYPE_BALANCE
} procedural_animation_type_t;

/* Ragdoll Physics Types */
typedef enum {
    RAGDOLL_STATE_ANIMATED = 0,
    RAGDOLL_STATE_PHYSICS,
    RAGDOLL_STATE_BLENDING
} ragdoll_state_t;

/* Animation Retargeting Types */
typedef enum {
    RETARGETING_MODE_NONE = 0,
    RETARGETING_MODE_HUMANOID,
    RETARGETING_MODE_GENERIC
} retargeting_mode_t;

/* SIMD Vector Types */
typedef union {
    float f[4];
    uint32_t u[4];
#if defined(__SSE2__)
    __m128 m128;
#elif defined(__ARM_NEON)
    float32x4_t neon;
#endif
} simd_vector_t;

/* Bone Transform */
typedef struct {
    simd_vector_t translation;
    simd_vector_t rotation;     // Quaternion
    simd_vector_t scale;
} bone_transform_t;

/* Animation Channel */
typedef struct {
    uint32_t bone_index;
    uint32_t keyframe_count;
    float* keyframe_times;
    bone_transform_t* keyframes;
    animation_compression_type_t compression_type;
    void* compressed_data;
    size_t compressed_size;
} animation_channel_t;

/* Procedural Animation Layer */
typedef struct {
    procedural_animation_type_t type;
    float weight;
    float frequency;
    float amplitude;
    float phase;
    uint32_t target_bone;
    bool enabled;
} procedural_layer_t;

/* Ragdoll Bone */
typedef struct {
    uint32_t bone_index;
    float mass;
    float friction;
    float restitution;
    simd_vector_t velocity;
    simd_vector_t angular_velocity;
    ragdoll_state_t state;
    float blend_weight;
} ragdoll_bone_t;

/* Retargeting Mapping */
typedef struct {
    uint32_t source_bone;
    uint32_t target_bone;
    simd_vector_t offset;
    simd_vector_t scale;
    bool active;
} retargeting_mapping_t;

/* Performance Counters */
typedef struct {
    uint64_t total_samples;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t compression_operations;
    uint64_t decompression_operations;
    uint64_t procedural_updates;
    uint64_t ragdoll_updates;
    uint64_t retargeting_operations;
    uint64_t simd_operations;
    uint64_t gpu_transfers;
    double total_sample_time;
    double compression_time;
    double decompression_time;
} animation_performance_counters_t;

/* Async Operation */
typedef struct {
    uint32_t id;
    animation_animation_sampling_handle_t handle;
    enum {
        ASYNC_OP_LOAD,
        ASYNC_OP_SAVE,
        ASYNC_OP_COMPRESS,
        ASYNC_OP_DECOMPRESS
    } type;
    bool completed;
    bool success;
    void* data;
    size_t data_size;
    pthread_t thread;
    pthread_mutex_t mutex;
} async_operation_t;

/* Cache Entry */
typedef struct {
    uint32_t bone_index;
    float time;
    bone_transform_t transform;
    uint64_t access_time;
    uint32_t access_count;
    bool dirty;
} cache_entry_t;

/* LOD Level */
typedef struct {
    float distance_threshold;
    uint32_t update_frequency;
    float compression_ratio;
    bool enable_procedural;
    bool enable_ragdoll;
} lod_level_t;

/* Render Graph Node */
typedef struct {
    uint32_t node_id;
    animation_animation_sampling_handle_t handle;
    uint32_t dependencies[8];
    uint32_t dependency_count;
    bool executed;
    uint64_t execution_time;
} render_graph_node_t;

/* Error Codes */
typedef enum {
    ANIMATION_SAMPLING_ERROR_NONE = 0,
    ANIMATION_SAMPLING_ERROR_INVALID_PARAM,
    ANIMATION_SAMPLING_ERROR_NOT_INITIALIZED,
    ANIMATION_SAMPLING_ERROR_OUT_OF_MEMORY,
    ANIMATION_SAMPLING_ERROR_INVALID_HANDLE,
    ANIMATION_SAMPLING_ERROR_COMPRESSION_FAILED,
    ANIMATION_SAMPLING_ERROR_DECOMPRESSION_FAILED,
    ANIMATION_SAMPLING_ERROR_THREAD_ERROR,
    ANIMATION_SAMPLING_ERROR_FILE_ERROR,
    ANIMATION_SAMPLING_ERROR_GPU_ERROR
} animation_sampling_error_t;

/* Extended Internal Structure */
typedef struct animation_bone_transform {
    float position[3];
    float rotation[4];  // Quaternion
    float scale[3];
    float matrix[16];   // 4x4 transform matrix
} animation_bone_transform_t;

typedef struct animation_keyframe {
    float time;
    animation_bone_transform_t transform;
} animation_keyframe_t;

typedef struct animation_bone {
    char name[64];
    int32_t parent_index;
    animation_bone_transform_t bind_pose;
    animation_bone_transform_t inverse_bind_pose;
    uint32_t flags;
} animation_bone_t;

typedef struct animation_skeleton {
    animation_bone_t bones[ANIMATION_MAX_BONES];
    uint32_t bone_count;
    char name[64];
} animation_skeleton_t;

typedef struct animation_clip {
    char name[64];
    float duration;
    float fps;
    uint32_t frame_count;
    animation_keyframe_t* keyframes;
    uint32_t keyframe_count;
    bool loop;
    uint32_t flags;
} animation_clip_t;

typedef struct animation_gpu_skinning_data {
    void* bone_buffer;
    void* vertex_buffer;
    void* index_buffer;
    void* shader;
    uint32_t bone_count;
    uint32_t vertex_count;
    bool gpu_accelerated;
#ifdef __APPLE__
    id<MTLBuffer> metal_bone_buffer;
    id<MTLRenderPipelineState> metal_pipeline;
#else
    VkBuffer vk_bone_buffer;
    VkPipeline vk_pipeline;
#endif
} animation_gpu_skinning_data_t;

typedef struct animation_compression_settings {
    float position_tolerance;
    float rotation_tolerance;
    float scale_tolerance;
    uint32_t compression_level;
    bool use_keyframe_reduction;
    bool use_quantization;
} animation_compression_settings_t;

typedef struct animation_compressed_clip {
    uint8_t* compressed_data;
    size_t compressed_size;
    animation_compression_settings_t settings;
    float compression_ratio;
} animation_compressed_clip_t;

typedef struct animation_procedural_layer {
    float amplitude;
    float frequency;
    float phase;
    uint32_t bone_mask[ANIMATION_MAX_BONES / 32];
    uint32_t mask_size;
    float (*generator)(float time, void* user_data);
    void* user_data;
} animation_procedural_layer_t;

typedef struct animation_procedural_system {
    animation_procedural_layer_t layers[16];
    uint32_t layer_count;
    float global_time;
    bool enabled;
} animation_procedural_system_t;

typedef struct animation_ragdoll_body {
    float position[3];
    float rotation[4];
    float velocity[3];
    float angular_velocity[3];
    float mass;
    float radius;
    int32_t bone_index;
    bool active;
} animation_ragdoll_body_t;

typedef struct animation_ragdoll_constraint {
    int32_t body_a;
    int32_t body_b;
    float min_angle[3];
    float max_angle[3];
    float stiffness;
    float damping;
} animation_ragdoll_constraint_t;

typedef struct animation_ragdoll_system {
    animation_ragdoll_body_t bodies[ANIMATION_MAX_RIGIDBODIES];
    animation_ragdoll_constraint_t constraints[ANIMATION_MAX_RIGIDBODIES * 2];
    uint32_t body_count;
    uint32_t constraint_count;
    float gravity[3];
    float time_step;
    bool enabled;
} animation_ragdoll_system_t;

typedef struct animation_retarget_mapping {
    int32_t source_bone;
    int32_t target_bone;
    float scale_offset[3];
    float rotation_offset[4];
    float position_offset[3];
} animation_retarget_mapping_t;

typedef struct animation_retarget_profile {
    animation_retarget_mapping_t mappings[ANIMATION_MAX_BONES];
    uint32_t mapping_count;
    char source_skeleton[64];
    char target_skeleton[64];
} animation_retarget_profile_t;

typedef struct animation_morph_target {
    char name[64];
    float* vertex_offsets;
    uint32_t vertex_count;
    float weight;
    bool active;
} animation_morph_target_t;

typedef struct animation_morph_system {
    animation_morph_target_t targets[ANIMATION_MAX_MORPH_TARGETS];
    uint32_t target_count;
    float* base_vertices;
    uint32_t vertex_count;
    float* blended_vertices;
} animation_morph_system_t;

typedef struct animation_cache_entry {
    uint64_t hash;
    void* data;
    size_t size;
    uint64_t last_access;
    uint32_t access_count;
    bool valid;
} animation_cache_entry_t;

typedef struct animation_async_operation {
    uint32_t id;
    uint32_t type;
    void* input_data;
    void* output_data;
    size_t input_size;
    size_t output_size;
    bool completed;
    bool cancelled;
    pthread_t thread;
    void (*callback)(uint32_t id, void* result, void* user_data);
    void* user_data;
} animation_async_operation_t;

typedef struct animation_performance_counters {
    uint64_t samples_processed;
    uint64_t gpu_operations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t async_operations;
    double total_sample_time;
    double total_gpu_time;
    double total_cache_time;
} animation_performance_counters_t;

typedef struct animation_thread_context {
    pthread_mutex_t global_mutex;
    pthread_mutex_t cache_mutex;
    pthread_mutex_t async_mutex;
    pthread_rwlock_t data_rwlock;
    bool thread_safe;
} animation_thread_context_t;

typedef struct animation_hot_reload_context {
    int inotify_fd;
    int watch_descriptor;
    pthread_t watcher_thread;
    bool active;
    void (*reload_callback)(const char* filename, void* user_data);
    void* user_data;
} animation_hot_reload_context_t;

typedef struct animation_render_graph_node {
    uint32_t node_id;
    uint32_t dependencies[8];
    uint32_t dependency_count;
    void (*execute)(void* input, void* output, void* user_data);
    void* user_data;
    bool enabled;
} animation_render_graph_node_t;

typedef struct animation_lod_level {
    float distance;
    uint32_t bone_reduction;
    uint32_t keyframe_reduction;
    float update_rate;
    bool use_procedural;
} animation_lod_level_t;

typedef struct animation_lod_system {
    animation_lod_level_t levels[ANIMATION_LOD_LEVELS];
    uint32_t level_count;
    uint32_t current_level;
    float viewer_distance;
} animation_lod_system_t;

typedef struct animation_animation_sampling_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    /* Core animation data */
    animation_skeleton_t skeleton;
    animation_clip_t* clips;
    uint32_t clip_count;
    
    /* GPU skinning */
    animation_gpu_skinning_data_t gpu_skinning;
    
    /* Compression */
    animation_compressed_clip_t* compressed_clips;
    uint32_t compressed_clip_count;
    animation_compression_settings_t compression_settings;
    
    /* Procedural animation */
    animation_procedural_system_t procedural_system;
    
    /* Ragdoll physics */
    animation_ragdoll_system_t ragdoll_system;
    
    /* Retargeting */
    animation_retarget_profile_t retarget_profile;
    
    /* Morph targets */
    animation_morph_system_t morph_system;
    
    /* Caching */
    animation_cache_entry_t cache[ANIMATION_CACHE_SIZE];
    uint32_t cache_size;
    
    /* Async operations */
    animation_async_operation_t async_ops[ANIMATION_MAX_ASYNC_OPERATIONS];
    uint32_t async_count;
    
    /* Performance */
    animation_performance_counters_t performance;
    
    /* LOD */
    animation_lod_system_t lod_system;
    
    /* Render graph */
    animation_render_graph_node_t render_nodes[ANIMATION_RENDER_GRAPH_NODES];
    uint32_t render_node_count;
    
} animation_animation_sampling_internal_t;

typedef struct animation_animation_sampling_context {
    animation_animation_sampling_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Global systems */
    animation_thread_context_t thread_context;
    animation_hot_reload_context_t hot_reload;
    animation_performance_counters_t global_performance;
    /* Async Operations */
    async_operation_t async_operations[ANIMATION_SAMPLING_MAX_ASYNC_OPERATIONS];
    uint32_t async_operation_count;
    pthread_mutex_t async_mutex;
    
    /* Hot-Reload */
    int inotify_fd;
    pthread_t file_watcher_thread;
    bool file_watcher_running;
    
    /* Render Graph */
    render_graph_node_t render_nodes[ANIMATION_ANIMATION_SAMPLING_MAX_COUNT];
    uint32_t render_node_count;
    
    /* Memory Tracking */
    size_t total_allocated;
    size_t peak_allocated;
    uint32_t allocation_count;
    
    /* Batch Processing */
    animation_animation_sampling_handle_t batch_queue[ANIMATION_ANIMATION_SAMPLING_MAX_COUNT];
    uint32_t batch_size;
    bool batch_processing;
    
    /* Culling Integration */
    bool culling_enabled;
    float culling_distance;
    uint32_t culled_count;
    
} animation_animation_sampling_context_t;

static animation_animation_sampling_context_t g_animation_sampling_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ============================================================================ */

/* GPU Skinning */
static int animation_gpu_skinning_init(animation_gpu_skinning_data_t* gpu_data, const animation_skeleton_t* skeleton);
static void animation_gpu_skinning_shutdown(animation_gpu_skinning_data_t* gpu_data);
static int animation_gpu_skinning_update_bones(animation_gpu_skinning_data_t* gpu_data, const animation_bone_transform_t* transforms, uint32_t bone_count);

/* Animation Compression */
static int animation_compress_clip(const animation_clip_t* clip, animation_compressed_clip_t* compressed, const animation_compression_settings_t* settings);
static int animation_decompress_clip(const animation_compressed_clip_t* compressed, animation_clip_t* clip);
static float animation_calculate_compression_ratio(const animation_clip_t* original, const animation_compressed_clip_t* compressed);

/* Procedural Animation */
static float animation_sine_generator(float time, void* user_data);
static float animation_noise_generator(float time, void* user_data);
static void animation_procedural_update(animation_procedural_system_t* procedural, float delta_time);
static void animation_procedural_blend(animation_bone_transform_t* output, const animation_procedural_layer_t* layer, float time);

/* Ragdoll Physics */
static int animation_ragdoll_init(animation_ragdoll_system_t* ragdoll, const animation_skeleton_t* skeleton);
static void animation_ragdoll_shutdown(animation_ragdoll_system_t* ragdoll);
static void animation_ragdoll_update(animation_ragdoll_system_t* ragdoll, float delta_time);
static void animation_ragdoll_apply_forces(animation_ragdoll_system_t* ragdoll);
static void animation_ragdoll_solve_constraints(animation_ragdoll_system_t* ragdoll);

/* Animation Retargeting */
static int animation_retarget_init(animation_retarget_profile_t* profile, const animation_skeleton_t* source, const animation_skeleton_t* target);
static void animation_retarget_transform(const animation_retarget_profile_t* profile, const animation_bone_transform_t* source, animation_bone_transform_t* target);

/* Morph Targets */
static int animation_morph_init(animation_morph_system_t* morph, uint32_t vertex_count);
static void animation_morph_shutdown(animation_morph_system_t* morph);
static void animation_morph_update(animation_morph_system_t* morph);
static void animation_morph_apply_target(animation_morph_system_t* morph, uint32_t target_index, float weight);

/* Caching System */
static uint64_t animation_calculate_hash(const void* data, size_t size);
static animation_cache_entry_t* animation_cache_find(uint64_t hash);
static void animation_cache_insert(uint64_t hash, const void* data, size_t size);
static void animation_cache_cleanup(void);

/* Async Operations */
static uint32_t animation_async_start_operation(uint32_t type, void* input, size_t input_size, void (*callback)(uint32_t, void*, void*), void* user_data);
static void* animation_async_worker_thread(void* arg);
static void animation_async_complete_operation(uint32_t id, void* result);

/* SIMD Operations */
static void animation_simd_transform_blend(animation_bone_transform_t* result, const animation_bone_transform_t* a, const animation_bone_transform_t* b, float weight);
static void animation_simd_matrix_multiply(float* result, const float* a, const float* b);
static void animation_simd_quaternion_slerp(float* result, const float* a, const float* b, float t);

/* Thread Safety */
static int animation_thread_context_init(animation_thread_context_t* ctx);
static void animation_thread_context_shutdown(animation_thread_context_t* ctx);
static void animation_thread_lock_read(void);
static void animation_thread_lock_write(void);
static void animation_thread_unlock(void);

/* Hot-Reload */
static int animation_hot_reload_init(animation_hot_reload_context_t* ctx);
static void animation_hot_reload_shutdown(animation_hot_reload_context_t* ctx);
static void* animation_hot_reload_watcher_thread(void* arg);

/* Render Graph */
static uint32_t animation_render_graph_add_node(const animation_render_graph_node_t* node);
static void animation_render_graph_execute(void);
static void animation_render_graph_topological_sort(uint32_t* sorted_nodes, uint32_t* count);

/* LOD System */
static void animation_lod_update(animation_lod_system_t* lod, float viewer_distance);
static uint32_t animation_lod_get_current_level(const animation_lod_system_t* lod);
static void animation_lod_apply_level(animation_lod_system_t* lod, uint32_t level);

/* Validation and Error Handling */
static bool animation_validate_skeleton(const animation_skeleton_t* skeleton);
static bool animation_validate_clip(const animation_clip_t* clip);
static const char* animation_get_error_string(int error_code);

/* Serialization */
static int animation_serialize_to_memory(const void* data, size_t size, uint8_t** out_buffer, size_t* out_size);
static int animation_deserialize_from_memory(const uint8_t* buffer, size_t size, void** out_data, size_t* out_size);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool animation_animation_sampling_validate(const animation_animation_sampling_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    
    /* Validate skeleton */
    if (!animation_validate_skeleton(&item->skeleton)) {
        return false;
    }
    
    /* Validate clips */
    for (uint32_t i = 0; i < item->clip_count; i++) {
        if (!animation_validate_clip(&item->clips[i])) {
            return false;
        }
    }
    
    return true;
}

static void animation_animation_sampling_cleanup_internal(animation_animation_sampling_internal_t* item) {
    if (!item) return;
    
    /* Free core data */
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    if (item->clips) {
        for (uint32_t i = 0; i < item->clip_count; i++) {
            if (item->clips[i].keyframes) {
                free(item->clips[i].keyframes);
            }
        }
        free(item->clips);
        item->clips = NULL;
    }
    
    /* Cleanup GPU skinning */
    animation_gpu_skinning_shutdown(&item->gpu_skinning);
    
    /* Cleanup compressed clips */
    if (item->compressed_clips) {
        for (uint32_t i = 0; i < item->compressed_clip_count; i++) {
            if (item->compressed_clips[i].compressed_data) {
                free(item->compressed_clips[i].compressed_data);
            }
        }
        free(item->compressed_clips);
        item->compressed_clips = NULL;
    }
    
    /* Cleanup morph system */
    animation_morph_shutdown(&item->morph_system);
    
    /* Cleanup ragdoll system */
    animation_ragdoll_shutdown(&item->ragdoll_system);
    
    /* Cleanup cache */
    for (uint32_t i = 0; i < item->cache_size; i++) {
        if (item->cache[i].data) {
            free(item->cache[i].data);
        }
    }
    
    /* Cancel async operations */
    for (uint32_t i = 0; i < item->async_count; i++) {
        item->async_ops[i].cancelled = true;
        if (item->async_ops[i].thread) {
            pthread_join(item->async_ops[i].thread, NULL);
        }
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_animation_sampling_init(void) {
    // TODO: Implement GPU skinning
    // TODO: Add animation compression
    // TODO: Implement state machine
    // TODO: Add procedural animation

    if (g_animation_sampling_ctx.initialized) {
        return 0; // Already initialized
    }

    g_animation_sampling_ctx.capacity = ANIMATION_ANIMATION_SAMPLING_DEFAULT_CAPACITY;
    g_animation_sampling_ctx.items = calloc(g_animation_sampling_ctx.capacity, sizeof(animation_animation_sampling_internal_t));
    if (!g_animation_sampling_ctx.items) {
        return -1;
    }

    g_animation_sampling_ctx.count = 0;
    g_animation_sampling_ctx.initialized = true;

    return 0;
}

void animation_animation_sampling_shutdown(void) {
    // TODO: Implement ragdoll physics
    // TODO: Add animation retargeting
    // TODO: Implement animation sampling initialization
    // TODO: Add animation sampling cleanup/shutdown

    if (!g_animation_sampling_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_animation_sampling_ctx.count; i++) {
        animation_animation_sampling_cleanup_internal(&g_animation_sampling_ctx.items[i]);
    }

    free(g_animation_sampling_ctx.items);
    g_animation_sampling_ctx.items = NULL;
    g_animation_sampling_ctx.count = 0;
    g_animation_sampling_ctx.capacity = 0;
    g_animation_sampling_ctx.initialized = false;
}

int animation_animation_sampling_create(animation_animation_sampling_handle_t* out_handle, const animation_animation_sampling_desc_t* desc) {
    // TODO: Implement animation sampling validation
    // TODO: Add animation sampling error handling
    // TODO: Implement animation sampling serialization
    // TODO: Add animation sampling debug output

    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_animation_sampling_ctx.initialized) {
        return -2;
    }

    if (g_animation_sampling_ctx.count >= g_animation_sampling_ctx.capacity) {
        // TODO: Implement animation sampling unit tests
        return -3;
    }

    uint32_t index = g_animation_sampling_ctx.count++;
    animation_animation_sampling_internal_t* item = &g_animation_sampling_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void animation_animation_sampling_destroy(animation_animation_sampling_handle_t handle) {
    // TODO: Add animation sampling performance counters
    // TODO: Implement animation sampling hot-reload

    if (handle.id >= g_animation_sampling_ctx.count) {
        return;
    }

    animation_animation_sampling_cleanup_internal(&g_animation_sampling_ctx.items[handle.id]);
}

int animation_animation_sampling_update(animation_animation_sampling_handle_t handle, const void* data, size_t size) {
    // TODO: Add animation sampling thread safety
    // TODO: Implement animation sampling memory pooling
    // TODO: Add animation sampling caching layer
    // TODO: Implement animation sampling async operations

    if (handle.id >= g_animation_sampling_ctx.count) {
        return -1;
    }

    animation_animation_sampling_internal_t* item = &g_animation_sampling_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // TODO: Add animation sampling GPU integration
    // TODO: Implement animation sampling SIMD optimization

    item->dirty = true;
    return 0;
}

bool animation_animation_sampling_is_valid(animation_animation_sampling_handle_t handle) {
    // TODO: Add animation sampling batch processing
    if (handle.id >= g_animation_sampling_ctx.count) {
        return false;
    }
    return g_animation_sampling_ctx.items[handle.id].initialized;
}

int animation_animation_sampling_get_info(animation_animation_sampling_handle_t handle, animation_animation_sampling_info_t* out_info) {
    // TODO: Implement animation sampling streaming support
    // TODO: Add animation sampling LOD support

    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_animation_sampling_ctx.count) {
        return -2;
    }

    const animation_animation_sampling_internal_t* item = &g_animation_sampling_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void animation_animation_sampling_mark_dirty(animation_animation_sampling_handle_t handle) {
    // TODO: Implement animation sampling culling integration
    if (handle.id < g_animation_sampling_ctx.count) {
        g_animation_sampling_ctx.items[handle.id].dirty = true;
    }
}

int animation_animation_sampling_process_pending(void) {
    // TODO: Add animation sampling render graph node
    // TODO: Implement batch processing

    int processed = 0;
    for (uint32_t i = 0; i < g_animation_sampling_ctx.count; i++) {
        animation_animation_sampling_internal_t* item = &g_animation_sampling_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t animation_animation_sampling_get_count(void) {
    return g_animation_sampling_ctx.count;
}

size_t animation_animation_sampling_get_memory_usage(void) {
    // TODO: Implement memory tracking
    size_t total = sizeof(g_animation_sampling_ctx);
    total += g_animation_sampling_ctx.capacity * sizeof(animation_animation_sampling_internal_t);

    for (uint32_t i = 0; i < g_animation_sampling_ctx.count; i++) {
        total += g_animation_sampling_ctx.items[i].data_size;
    }

    return total;
}

void animation_animation_sampling_debug_print(void) {
    // TODO: Implement debug output
    // Debug printing implementation
}

/* End of animation_sampling.c */
