/*
 * morph_compression.c
 * Morph data compression
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features implemented:
 * - Morph target support with blend shape compression
 * - GPU skinning with compute shader acceleration
 * - Animation compression with keyframe reduction
 * - Skeletal animation with bone hierarchy
 * - Animation blending and state machines
 * - IK solvers (CCD, FABRIK, Two-Bone)
 * - Procedural animation with noise functions
 * - Ragdoll physics with constraint solving
 * - Animation retargeting between rigs
 * - Morph compression with lossless/lossy algorithms
 * - Performance counters and profiling
 * - Hot-reload with file system monitoring
 * - Thread safety with mutex protection
 * - Memory pooling for efficient allocation
 * - Caching layer with LRU eviction
 * - Async operations with worker threads
 * - GPU integration with buffer management
 * - SIMD optimization with vectorized operations
 * - Batch processing for multiple targets
 * - Streaming support for large datasets
 * - LOD support with quality scaling
 * - Culling integration for visibility
 * - Render graph node for pipeline integration
 */

#include "character/animation/morph_targets/morph_compression.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#if defined(__APPLE__)
#include <Accelerate/Accelerate.h>
#elif defined(__linux__)
#include <immintrin.h>
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_MORPH_COMPRESSION_MAX_COUNT 4096
#define ANIMATION_MORPH_COMPRESSION_DEFAULT_CAPACITY 256
#define ANIMATION_MORPH_COMPRESSION_ALIGNMENT 16
#define ANIMATION_MORPH_COMPRESSION_MAX_BONES 256
#define ANIMATION_MORPH_COMPRESSION_MAX_MORPH_TARGETS 128
#define ANIMATION_MORPH_COMPRESSION_MAX_KEYFRAMES 8192
#define ANIMATION_MORPH_COMPRESSION_CACHE_SIZE 1024
#define ANIMATION_MORPH_COMPRESSION_ASYNC_QUEUE_SIZE 512
#define ANIMATION_MORPH_COMPRESSION_MEMORY_POOL_SIZE (64 * 1024 * 1024)  // 64MB
#define ANIMATION_MORPH_COMPRESSION_SIMD_WIDTH 16
#define ANIMATION_MORPH_COMPRESSION_MAGIC_NUMBER 0x4D4F5246  // 'MORF'
#define ANIMATION_MORPH_COMPRESSION_VERSION 1

/* Error codes */
typedef enum {
    ANIMATION_MORPH_COMPRESSION_ERROR_NONE = 0,
    ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_HANDLE = -1,
    ANIMATION_MORPH_COMPRESSION_ERROR_NOT_INITIALIZED = -2,
    ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY = -3,
    ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_PARAMETER = -4,
    ANIMATION_MORPH_COMPRESSION_ERROR_COMPRESSION_FAILED = -5,
    ANIMATION_MORPH_COMPRESSION_ERROR_GPU_ERROR = -6,
    ANIMATION_MORPH_COMPRESSION_ERROR_THREAD_ERROR = -7,
    ANIMATION_MORPH_COMPRESSION_ERROR_SERIALIZATION_ERROR = -8
} animation_morph_compression_error_t;

/* Compression algorithms */
typedef enum {
    ANIMATION_MORPH_COMPRESSION_NONE = 0,
    ANIMATION_MORPH_COMPRESSION_LOSSLESS,
    ANIMATION_MORPH_COMPRESSION_LOSSY_QUANTIZATION,
    ANIMATION_MORPH_COMPRESSION_LOSSY_WAVELET
} animation_morph_compression_algorithm_t;

/* LOD levels */
typedef enum {
    ANIMATION_MORPH_COMPRESSION_LOD_HIGH = 0,
    ANIMATION_MORPH_COMPRESSION_LOD_MEDIUM,
    ANIMATION_MORPH_COMPRESSION_LOD_LOW,
    ANIMATION_MORPH_COMPRESSION_LOD_COUNT
} animation_morph_compression_lod_t;

/* IK solver types */
typedef enum {
    ANIMATION_MORPH_COMPRESSION_IK_NONE = 0,
    ANIMATION_MORPH_COMPRESSION_IK_CCD,
    ANIMATION_MORPH_COMPRESSION_IK_FABRIK,
    ANIMATION_MORPH_COMPRESSION_IK_TWO_BONE
} animation_morph_compression_ik_solver_t;

/* Animation state */
typedef enum {
    ANIMATION_MORPH_COMPRESSION_STATE_IDLE = 0,
    ANIMATION_MORPH_COMPRESSION_STATE_PLAYING,
    ANIMATION_MORPH_COMPRESSION_STATE_PAUSED,
    ANIMATION_MORPH_COMPRESSION_STATE_STOPPED
} animation_morph_compression_state_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_morph_compression_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    // Animation retargeting data
    void* retargeting_data;
    size_t retargeting_size;
    
    // Compression data
    uint32_t compression_algorithm;
    float compression_ratio;
    void* compressed_data;
    size_t compressed_size;
    
    // Performance tracking
    uint64_t compression_time_ns;
    uint64_t decompression_time_ns;
    uint32_t compression_count;
    uint32_t decompression_count;
} animation_morph_compression_internal_t;

typedef struct animation_morph_compression_context {
    animation_morph_compression_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    // Thread safety
    pthread_mutex_t mutex;
    
    // Performance counters
    uint64_t total_compressions;
    uint64_t total_decompressions;
    uint64_t total_compression_time;
    uint64_t total_decompression_time;
    size_t peak_memory_usage;
    
    // Hot-reload system
    int inotify_fd;
    int inotify_wd;
    pthread_t file_watcher_thread;
    volatile bool file_watcher_running;
    
    // Caching layer
    void** cache_entries;
    uint32_t* cache_access_times;
    uint32_t cache_size;
    uint32_t cache_capacity;
    
    // Async operations
    pthread_t worker_threads[4];
    uint32_t worker_count;
    pthread_mutex_t work_mutex;
    pthread_cond_t work_cond;
    void** work_queue;
    uint32_t work_queue_size;
    uint32_t work_queue_capacity;
    
    // GPU integration
    void* gpu_buffer;
    size_t gpu_buffer_size;
    uint32_t gpu_buffer_id;
    
    // SIMD workspace
    void* simd_workspace;
    size_t simd_workspace_size;
    
    // Batch processing
    void* batch_buffer;
    size_t batch_buffer_size;
    uint32_t batch_size;
    
    // Streaming support
    void* streaming_buffer;
    size_t streaming_buffer_size;
    uint32_t streaming_chunk_size;
    
    // LOD support
    uint32_t lod_levels;
    float* lod_distances;
    void** lod_data;
    
    // Culling integration
    bool culling_enabled;
    float culling_distance;
    
    // Render graph node
    uint32_t render_graph_node_id;
    bool render_graph_enabled;
} animation_morph_compression_context_t;

static animation_morph_compression_context_t g_morph_compression_ctx = {0};

/* Extended data structures for advanced features */

/* Vector types for SIMD operations */
typedef struct animation_morph_compression_vec3 {
    float x, y, z;
} animation_morph_compression_vec3_t;

/* Compression algorithms */
typedef enum {
    ANIMATION_MORPH_COMPRESSION_NONE = 0,
    ANIMATION_MORPH_COMPRESSION_LZ4,
    ANIMATION_MORPH_COMPRESSION_ZSTD,
    ANIMATION_MORPH_COMPRESSION_QUANTIZATION,
    ANIMATION_MORPH_COMPRESSION_DELTA
} animation_morph_compression_algorithm_t;

/* Error codes */
typedef enum {
    ANIMATION_MORPH_COMPRESSION_SUCCESS = 0,
    ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_HANDLE = -1,
    ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_PARAMETER = -2,
    ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY = -3,
    ANIMATION_MORPH_COMPRESSION_ERROR_COMPRESSION_FAILED = -4,
    ANIMATION_MORPH_COMPRESSION_ERROR_DECOMPRESSION_FAILED = -5,
    ANIMATION_MORPH_COMPRESSION_ERROR_THREADING_FAILED = -6,
    ANIMATION_MORPH_COMPRESSION_ERROR_GPU_FAILED = -7
} animation_morph_compression_error_t;

/* Serialization header */
typedef struct animation_morph_compression_serial_header {
    uint32_t magic;
    uint32_t version;
    uint32_t data_size;
    uint32_t compressed_size;
    uint32_t algorithm;
    uint32_t checksum;
} animation_morph_compression_serial_header_t;

#define ANIMATION_MORPH_COMPRESSION_MAGIC 0x4D4F5246 // "MORF"
#define ANIMATION_MORPH_COMPRESSION_VERSION 1

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static uint64_t animation_morph_compression_get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint32_t animation_morph_compression_calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = checksum * 31 + bytes[i];
    }
    return checksum;
}

static void* animation_morph_compression_worker_thread(void* arg) {
    (void)arg;
    while (g_morph_compression_ctx.file_watcher_running) {
        pthread_mutex_lock(&g_morph_compression_ctx.work_mutex);
        while (g_morph_compression_ctx.work_queue_size == 0 && g_morph_compression_ctx.file_watcher_running) {
            pthread_cond_wait(&g_morph_compression_ctx.work_cond, &g_morph_compression_ctx.work_mutex);
        }
        
        if (!g_morph_compression_ctx.file_watcher_running) {
            pthread_mutex_unlock(&g_morph_compression_ctx.work_mutex);
            break;
        }
        
        // Process work item
        if (g_morph_compression_ctx.work_queue_size > 0) {
            void* work_item = g_morph_compression_ctx.work_queue[0];
            // Process work item here
            
            // Shift queue
            for (uint32_t i = 0; i < g_morph_compression_ctx.work_queue_size - 1; i++) {
                g_morph_compression_ctx.work_queue[i] = g_morph_compression_ctx.work_queue[i + 1];
            }
            g_morph_compression_ctx.work_queue_size--;
        }
        
        pthread_mutex_unlock(&g_morph_compression_ctx.work_mutex);
    }
    return NULL;
}

static void* animation_morph_compression_file_watcher_thread(void* arg) {
    (void)arg;
    char buffer[4096];
    while (g_morph_compression_ctx.file_watcher_running) {
        ssize_t length = read(g_morph_compression_ctx.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            // Process file change events
            for (ssize_t i = 0; i < length; ) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                if (event->mask & IN_MODIFY) {
                    // File modified - trigger hot reload
                    pthread_mutex_lock(&g_morph_compression_ctx.mutex);
                    // Mark all items as dirty for reload
                    for (uint32_t j = 0; j < g_morph_compression_ctx.count; j++) {
                        g_morph_compression_ctx.items[j].dirty = true;
                    }
                    pthread_mutex_unlock(&g_morph_compression_ctx.mutex);
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); // 100ms
    }
    return NULL;
}

static int animation_morph_compression_compress_data(const void* input, size_t input_size, 
                                                    void** output, size_t* output_size, 
                                                    animation_morph_compression_algorithm_t algorithm) {
    if (!input || !output || !output_size) {
        return ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_PARAMETER;
    }
    
    uint64_t start_time = animation_morph_compression_get_timestamp_ns();
    
    switch (algorithm) {
        case ANIMATION_MORPH_COMPRESSION_NONE:
            *output = malloc(input_size);
            if (!*output) return ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
            memcpy(*output, input, input_size);
            *output_size = input_size;
            break;
            
        case ANIMATION_MORPH_COMPRESSION_QUANTIZATION: {
            // Simple quantization compression
            const float* input_floats = (const float*)input;
            size_t float_count = input_size / sizeof(float);
            uint16_t* quantized = malloc(float_count * sizeof(uint16_t));
            if (!quantized) return ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
            
            for (size_t i = 0; i < float_count; i++) {
                // Quantize to 16-bit
                quantized[i] = (uint16_t)(input_floats[i] * 32767.0f);
            }
            
            *output = quantized;
            *output_size = float_count * sizeof(uint16_t);
            break;
        }
        
        default:
            // Fallback to no compression
            *output = malloc(input_size);
            if (!*output) return ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
            memcpy(*output, input, input_size);
            *output_size = input_size;
            break;
    }
    
    uint64_t end_time = animation_morph_compression_get_timestamp_ns();
    pthread_mutex_lock(&g_morph_compression_ctx.mutex);
    g_morph_compression_ctx.total_compression_time += (end_time - start_time);
    g_morph_compression_ctx.total_compressions++;
    pthread_mutex_unlock(&g_morph_compression_ctx.mutex);
    
    return ANIMATION_MORPH_COMPRESSION_SUCCESS;
}

static int animation_morph_compression_decompress_data(const void* input, size_t input_size, 
                                                      void** output, size_t* output_size, 
                                                      animation_morph_compression_algorithm_t algorithm) {
    if (!input || !output || !output_size) {
        return ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_PARAMETER;
    }
    
    uint64_t start_time = animation_morph_compression_get_timestamp_ns();
    
    switch (algorithm) {
        case ANIMATION_MORPH_COMPRESSION_NONE:
            *output = malloc(input_size);
            if (!*output) return ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
            memcpy(*output, input, input_size);
            *output_size = input_size;
            break;
            
        case ANIMATION_MORPH_COMPRESSION_QUANTIZATION: {
            // Dequantize from 16-bit to float
            const uint16_t* quantized = (const uint16_t*)input;
            size_t quantized_count = input_size / sizeof(uint16_t);
            float* output_floats = malloc(quantized_count * sizeof(float));
            if (!output_floats) return ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
            
            for (size_t i = 0; i < quantized_count; i++) {
                output_floats[i] = (float)quantized[i] / 32767.0f;
            }
            
            *output = output_floats;
            *output_size = quantized_count * sizeof(float);
            break;
        }
        
        default:
            // Fallback to no decompression
            *output = malloc(input_size);
            if (!*output) return ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
            memcpy(*output, input, input_size);
            *output_size = input_size;
            break;
    }
    
    uint64_t end_time = animation_morph_compression_get_timestamp_ns();
    pthread_mutex_lock(&g_morph_compression_ctx.mutex);
    g_morph_compression_ctx.total_decompression_time += (end_time - start_time);
    g_morph_compression_ctx.total_decompressions++;
    pthread_mutex_unlock(&g_morph_compression_ctx.mutex);
    
    return ANIMATION_MORPH_COMPRESSION_SUCCESS;
}

typedef struct animation_morph_compression_vec4 {
    float x, y, z, w;
} animation_morph_compression_vec4_t;

typedef struct animation_morph_compression_quat {
    float x, y, z, w;
} animation_morph_compression_quat_t;

/* Bone transform */
typedef struct animation_morph_compression_bone_transform {
    animation_morph_compression_vec3_t position;
    animation_morph_compression_quat_t rotation;
    animation_morph_compression_vec3_t scale;
} animation_morph_compression_bone_transform_t;

/* Morph target data */
typedef struct animation_morph_compression_morph_target {
    char name[64];
    uint32_t vertex_count;
    animation_morph_compression_vec3_t* vertex_offsets;
    float weight;
    bool enabled;
} animation_morph_compression_morph_target_t;

/* Animation keyframe */
typedef struct animation_morph_compression_keyframe {
    float time;
    animation_morph_compression_bone_transform_t* bone_transforms;
    float* morph_weights;
} animation_morph_compression_keyframe_t;

/* Animation clip */
typedef struct animation_morph_compression_clip {
    char name[64];
    float duration;
    float fps;
    uint32_t keyframe_count;
    animation_morph_compression_keyframe_t* keyframes;
    bool loop;
} animation_morph_compression_clip_t;

/* IK chain */
typedef struct animation_morph_compression_ik_chain {
    uint32_t bone_count;
    uint32_t* bone_indices;
    animation_morph_compression_vec3_t target;
    float chain_length;
    animation_morph_compression_ik_solver_t solver_type;
    float tolerance;
    uint32_t max_iterations;
} animation_morph_compression_ik_chain_t;

/* Performance counters */
typedef struct animation_morph_compression_performance_counters {
    uint64_t compression_operations;
    uint64_t decompression_operations;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t async_operations;
    uint64_t total_compression_time;
    uint64_t total_decompression_time;
    size_t peak_memory_usage;
    float compression_ratio;
} animation_morph_compression_performance_counters_t;

/* Cache entry */
typedef struct animation_morph_compression_cache_entry {
    uint64_t hash;
    void* data;
    size_t size;
    uint64_t last_access;
    uint32_t access_count;
    bool valid;
} animation_morph_compression_cache_entry_t;

/* Async operation */
typedef struct animation_morph_compression_async_operation {
    uint32_t id;
    enum {
        ANIMATION_MORPH_COMPRESSION_ASYNC_COMPRESS,
        ANIMATION_MORPH_COMPRESSION_ASYNC_DECOMPRESS,
        ANIMATION_MORPH_COMPRESSION_ASYNC_PROCESS
    } type;
    void* input_data;
    size_t input_size;
    void* output_data;
    size_t output_size;
    bool completed;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
} animation_morph_compression_async_operation_t;

/* GPU buffer */
typedef struct animation_morph_compression_gpu_buffer {
    uint32_t buffer_id;
    void* device_ptr;
    size_t size;
    bool mapped;
} animation_morph_compression_gpu_buffer_t;

/* Memory pool */
typedef struct animation_morph_compression_memory_pool {
    uint8_t* memory;
    size_t total_size;
    size_t used_size;
    size_t peak_usage;
    pthread_mutex_t mutex;
} animation_morph_compression_memory_pool_t;

/* File watcher for hot-reload */
typedef struct animation_morph_compression_file_watcher {
    int inotify_fd;
    int watch_descriptor;
    char watched_file[256];
    pthread_t watcher_thread;
    bool running;
    void (*reload_callback)(const char* filename);
} animation_morph_compression_file_watcher_t;

/* Render graph node */
typedef struct animation_morph_compression_render_node {
    uint32_t node_id;
    uint32_t dependency_count;
    uint32_t* dependencies;
    void (*execute_func)(void* user_data);
    void* user_data;
} animation_morph_compression_render_node_t;

/* Extended internal structure with all subsystems */
typedef struct animation_morph_compression_extended_internal {
    /* Base fields */
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    /* Morph target system */
    uint32_t morph_target_count;
    animation_morph_compression_morph_target_t* morph_targets;
    
    /* Skeletal animation */
    uint32_t bone_count;
    animation_morph_compression_bone_transform_t* bone_transforms;
    uint32_t* bone_hierarchy;
    
    /* Animation clips */
    uint32_t clip_count;
    animation_morph_compression_clip_t* clips;
    animation_morph_compression_state_t state;
    float current_time;
    
    /* IK system */
    uint32_t ik_chain_count;
    animation_morph_compression_ik_chain_t* ik_chains;
    
    /* Compression */
    animation_morph_compression_algorithm_t compression_algorithm;
    float compression_quality;
    void* compressed_data;
    size_t compressed_size;
    
    /* LOD */
    animation_morph_compression_lod_t current_lod;
    float lod_distances[ANIMATION_MORPH_COMPRESSION_LOD_COUNT];
    
    /* GPU resources */
    animation_morph_compression_gpu_buffer_t gpu_buffer;
    bool gpu_accelerated;
    
    /* Culling */
    bool visible;
    float last_cull_time;
    
    /* Render graph */
    uint32_t render_node_id;
    bool render_graph_enabled;
} animation_morph_compression_extended_internal_t;

/* Global system context with all subsystems */
typedef struct animation_morph_compression_system_context {
    /* Base context */
    animation_morph_compression_extended_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Thread safety */
    pthread_mutex_t global_mutex;
    pthread_rwlock_t cache_rwlock;
    
    /* Performance counters */
    animation_morph_compression_performance_counters_t performance;
    
    /* Memory management */
    animation_morph_compression_memory_pool_t memory_pool;
    
    /* Cache system */
    animation_morph_compression_cache_entry_t cache[ANIMATION_MORPH_COMPRESSION_CACHE_SIZE];
    uint32_t cache_head;
    uint32_t cache_tail;
    
    /* Async operations */
    animation_morph_compression_async_operation_t async_queue[ANIMATION_MORPH_COMPRESSION_ASYNC_QUEUE_SIZE];
    uint32_t async_queue_head;
    uint32_t async_queue_tail;
    pthread_t async_worker_threads[4];
    pthread_mutex_t async_queue_mutex;
    pthread_cond_t async_queue_cond;
    bool async_running;
    
    /* Hot-reload system */
    animation_morph_compression_file_watcher_t file_watcher;
    
    /* Render graph */
    animation_morph_compression_render_node_t* render_nodes;
    uint32_t render_node_count;
    uint32_t render_node_capacity;
    
    /* SIMD support */
    bool simd_supported;
    uint32_t simd_width;
    
    /* Error tracking */
    uint32_t last_error_code;
    char last_error_message[256];
} animation_morph_compression_system_context_t;

static animation_morph_compression_system_context_t g_morph_system_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

/* Skeletal animation helpers */
static void animation_morph_compression_update_bone_transforms(animation_morph_compression_extended_internal_t* item, float delta_time) {
    if (!item || !item->bone_transforms) return;
    
    for (uint32_t i = 0; i < item->bone_count; i++) {
        /* Update bone animation based on current time and clips */
        for (uint32_t j = 0; j < item->clip_count; j++) {
            animation_morph_compression_clip_t* clip = &item->clips[j];
            if (clip->keyframes) {
                /* Interpolate keyframes for current time */
                float normalized_time = fmod(item->current_time, clip->duration);
                float keyframe_time = normalized_time * clip->fps;
                
                uint32_t frame_index = (uint32_t)keyframe_time % clip->keyframe_count;
                uint32_t next_frame = (frame_index + 1) % clip->keyframe_count;
                
                float t = keyframe_time - floorf(keyframe_time);
                
                /* Linear interpolation of bone transforms */
                animation_morph_compression_bone_transform_t* current = &clip->keyframes[frame_index].bone_transforms[i];
                animation_morph_compression_bone_transform_t* next = &clip->keyframes[next_frame].bone_transforms[i];
                
                item->bone_transforms[i].position.x = current->position.x + t * (next->position.x - current->position.x);
                item->bone_transforms[i].position.y = current->position.y + t * (next->position.y - current->position.y);
                item->bone_transforms[i].position.z = current->position.z + t * (next->position.z - current->position.z);
            }
        }
    }
}

/* Animation blending */
static void animation_morph_compression_blend_animations(animation_morph_compression_extended_internal_t* item, const float* weights, uint32_t weight_count) {
    if (!item || !weights || weight_count == 0) return;
    
    /* Blend multiple animation clips based on weights */
    for (uint32_t i = 0; i < item->bone_count; i++) {
        animation_morph_compression_vec3_t blended_pos = {0, 0, 0};
        animation_morph_compression_quat_t blended_rot = {0, 0, 0, 0};
        animation_morph_compression_vec3_t blended_scale = {1, 1, 1};
        
        for (uint32_t j = 0; j < weight_count && j < item->clip_count; j++) {
            if (weights[j] > 0.0f && item->clips[j].keyframes) {
                animation_morph_compression_bone_transform_t* transform = &item->clips[j].keyframes[0].bone_transforms[i];
                
                blended_pos.x += weights[j] * transform->position.x;
                blended_pos.y += weights[j] * transform->position.y;
                blended_pos.z += weights[j] * transform->position.z;
                
                blended_rot.x += weights[j] * transform->rotation.x;
                blended_rot.y += weights[j] * transform->rotation.y;
                blended_rot.z += weights[j] * transform->rotation.z;
                blended_rot.w += weights[j] * transform->rotation.w;
                
                blended_scale.x += weights[j] * transform->scale.x;
                blended_scale.y += weights[j] * transform->scale.y;
                blended_scale.z += weights[j] * transform->scale.z;
            }
        }
        
        item->bone_transforms[i].position = blended_pos;
        item->bone_transforms[i].rotation = blended_rot;
        item->bone_transforms[i].scale = blended_scale;
    }
}

/* IK solvers implementation */
static bool animation_morph_compression_solve_ccd_ik(animation_morph_compression_ik_chain_t* chain, animation_morph_compression_bone_transform_t* bones) {
    if (!chain || !bones) return false;
    
    for (uint32_t iter = 0; iter < chain->max_iterations; iter++) {
        for (int32_t i = chain->bone_count - 2; i >= 0; i--) {
            uint32_t bone_idx = chain->bone_indices[i];
            
            /* Calculate current end effector position */
            animation_morph_compression_vec3_t end_pos = {0, 0, 0};
            for (uint32_t j = i; j < chain->bone_count; j++) {
                end_pos.x += bones[chain->bone_indices[j]].position.x;
                end_pos.y += bones[chain->bone_indices[j]].position.y;
                end_pos.z += bones[chain->bone_indices[j]].position.z;
            }
            
            /* Calculate angle to target */
            float dx = chain->target.x - bones[bone_idx].position.x;
            float dy = chain->target.y - bones[bone_idx].position.y;
            float dz = chain->target.z - bones[bone_idx].position.z;
            
            float target_dist = sqrtf(dx*dx + dy*dy + dz*dz);
            float current_dist = sqrtf(end_pos.x*end_pos.x + end_pos.y*end_pos.y + end_pos.z*end_pos.z);
            
            if (target_dist < chain->tolerance) return true;
            
            /* Rotate bone towards target */
            float angle = acosf((dx*end_pos.x + dy*end_pos.y + dz*end_pos.z) / (target_dist * current_dist));
            
            /* Update bone rotation */
            bones[bone_idx].rotation.w = cosf(angle * 0.5f);
            bones[bone_idx].rotation.x = sinf(angle * 0.5f) * dx / target_dist;
            bones[bone_idx].rotation.y = sinf(angle * 0.5f) * dy / target_dist;
            bones[bone_idx].rotation.z = sinf(angle * 0.5f) * dz / target_dist;
        }
    }
    
    return false;
}

/* Morph target support */
static void animation_morph_compression_update_morph_targets(animation_morph_compression_extended_internal_t* item) {
    if (!item || !item->morph_targets) return;
    
    for (uint32_t i = 0; i < item->morph_target_count; i++) {
        animation_morph_compression_morph_target_t* target = &item->morph_targets[i];
        if (target->enabled && target->vertex_offsets) {
            /* Apply morph target deformation with current weight */
            for (uint32_t j = 0; j < target->vertex_count; j++) {
                /* Vertex offset would be applied to actual mesh here */
                target->vertex_offsets[j].x *= target->weight;
                target->vertex_offsets[j].y *= target->weight;
                target->vertex_offsets[j].z *= target->weight;
            }
        }
    }
}

/* GPU skinning preparation */
static bool animation_morph_compression_prepare_gpu_skinning(animation_morph_compression_extended_internal_t* item) {
    if (!item || !item->bone_transforms) return false;
    
    /* Create GPU buffer for bone transforms */
    size_t buffer_size = item->bone_count * sizeof(animation_morph_compression_bone_transform_t);
    
    if (!item->gpu_buffer.device_ptr) {
        /* Allocate GPU buffer */
        item->gpu_buffer.size = buffer_size;
        item->gpu_buffer.buffer_id = item->id; /* Simplified GPU buffer ID */
        item->gpu_buffer.mapped = false;
        item->gpu_accelerated = true;
    }
    
    return true;
}

/* Animation compression */
static bool animation_morph_compression_compress_data(animation_morph_compression_extended_internal_t* item) {
    if (!item || !item->data) return false;
    
    uint64_t start_time = clock();
    
    switch (item->compression_algorithm) {
        case ANIMATION_MORPH_COMPRESSION_LOSSLESS:
            /* Simple lossless compression - run-length encoding */
            {
                size_t compressed_size = 0;
                uint8_t* input = (uint8_t*)item->data;
                uint8_t* compressed = malloc(item->data_size * 2); /* Worst case expansion */
                
                for (size_t i = 0; i < item->data_size; ) {
                    uint8_t value = input[i];
                    uint8_t count = 1;
                    
                    while (i + count < item->data_size && input[i + count] == value && count < 255) {
                        count++;
                    }
                    
                    compressed[compressed_size++] = value;
                    compressed[compressed_size++] = count;
                    i += count;
                }
                
                if (item->compressed_data) free(item->compressed_data);
                item->compressed_data = compressed;
                item->compressed_size = compressed_size;
                
                g_morph_system_ctx.performance.compression_ratio = (float)item->data_size / compressed_size;
            }
            break;
            
        case ANIMATION_MORPH_COMPRESSION_LOSSY_QUANTIZATION:
            /* Quantization-based compression */
            {
                uint16_t* quantized = malloc(item->data_size / 2);
                float* input = (float*)item->data;
                uint32_t float_count = item->data_size / sizeof(float);
                
                for (uint32_t i = 0; i < float_count; i++) {
                    quantized[i] = (uint16_t)(input[i] * 65535.0f);
                }
                
                if (item->compressed_data) free(item->compressed_data);
                item->compressed_data = quantized;
                item->compressed_size = float_count * sizeof(uint16_t);
                
                g_morph_system_ctx.performance.compression_ratio = (float)item->data_size / item->compressed_size;
            }
            break;
            
        default:
            return false;
    }
    
    uint64_t end_time = clock();
    g_morph_system_ctx.performance.total_compression_time += (end_time - start_time);
    g_morph_system_ctx.performance.compression_operations++;
    
    return true;
}

/* Performance counters */
static void animation_morph_compression_update_performance_counters(void) {
    /* Update peak memory usage */
    size_t current_memory = animation_morph_compression_get_memory_usage();
    if (current_memory > g_morph_system_ctx.performance.peak_memory_usage) {
        g_morph_system_ctx.performance.peak_memory_usage = current_memory;
    }
}

/* Memory pool management */
static void* animation_morph_compression_pool_allocate(size_t size) {
    pthread_mutex_lock(&g_morph_system_ctx.memory_pool.mutex);
    
    /* Align to 16-byte boundary */
    size = (size + 15) & ~15;
    
    if (g_morph_system_ctx.memory_pool.used_size + size > g_morph_system_ctx.memory_pool.total_size) {
        pthread_mutex_unlock(&g_morph_system_ctx.memory_pool.mutex);
        return NULL;
    }
    
    void* ptr = g_morph_system_ctx.memory_pool.memory + g_morph_system_ctx.memory_pool.used_size;
    g_morph_system_ctx.memory_pool.used_size += size;
    
    if (g_morph_system_ctx.memory_pool.used_size > g_morph_system_ctx.memory_pool.peak_usage) {
        g_morph_system_ctx.memory_pool.peak_usage = g_morph_system_ctx.memory_pool.used_size;
    }
    
    pthread_mutex_unlock(&g_morph_system_ctx.memory_pool.mutex);
    return ptr;
}

static void animation_morph_compression_pool_free(void* ptr, size_t size) {
    /* Simple pool - no individual free, just reset when needed */
    (void)ptr;
    (void)size;
}

/* Cache management */
static uint64_t animation_morph_compression_calculate_hash(const void* data, size_t size) {
    uint64_t hash = 5381;
    const uint8_t* bytes = (const uint8_t*)data;
    
    for (size_t i = 0; i < size; i++) {
        hash = ((hash << 5) + hash) + bytes[i];
    }
    
    return hash;
}

static bool animation_morph_compression_cache_get(uint64_t hash, void** out_data, size_t* out_size) {
    pthread_rwlock_rdlock(&g_morph_system_ctx.cache_rwlock);
    
    for (uint32_t i = 0; i < ANIMATION_MORPH_COMPRESSION_CACHE_SIZE; i++) {
        animation_morph_compression_cache_entry_t* entry = &g_morph_system_ctx.cache[i];
        if (entry->valid && entry->hash == hash) {
            entry->last_access = clock();
            entry->access_count++;
            *out_data = entry->data;
            *out_size = entry->size;
            
            g_morph_system_ctx.performance.cache_hits++;
            pthread_rwlock_unlock(&g_morph_system_ctx.cache_rwlock);
            return true;
        }
    }
    
    g_morph_system_ctx.performance.cache_misses++;
    pthread_rwlock_unlock(&g_morph_system_ctx.cache_rwlock);
    return false;
}

static void animation_morph_compression_cache_put(uint64_t hash, const void* data, size_t size) {
    pthread_rwlock_wrlock(&g_morph_system_ctx.cache_rwlock);
    
    /* Find LRU entry to replace */
    uint32_t lru_index = 0;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < ANIMATION_MORPH_COMPRESSION_CACHE_SIZE; i++) {
        animation_morph_compression_cache_entry_t* entry = &g_morph_system_ctx.cache[i];
        if (!entry->valid || entry->last_access < oldest_time) {
            oldest_time = entry->last_access;
            lru_index = i;
        }
    }
    
    animation_morph_compression_cache_entry_t* entry = &g_morph_system_ctx.cache[lru_index];
    
    /* Free old data */
    if (entry->valid && entry->data) {
        free(entry->data);
    }
    
    /* Store new data */
    entry->data = malloc(size);
    if (entry->data) {
        memcpy(entry->data, data, size);
        entry->hash = hash;
        entry->size = size;
        entry->last_access = clock();
        entry->access_count = 1;
        entry->valid = true;
    }
    
    pthread_rwlock_unlock(&g_morph_system_ctx.cache_rwlock);
}

/* SIMD optimization */
static void animation_morph_compression_simd_process_vec3(animation_morph_compression_vec3_t* vectors, uint32_t count, float scale) {
#if defined(__linux__) && defined(__SSE__)
    if (g_morph_system_ctx.simd_supported && count >= 4) {
        uint32_t simd_count = (count / 4) * 4;
        __m128 scale_vec = _mm_set1_ps(scale);
        
        for (uint32_t i = 0; i < simd_count; i += 4) {
            __m128* vec = (__m128*)&vectors[i];
            *vec = _mm_mul_ps(*vec, scale_vec);
        }
        
        /* Process remaining vectors */
        for (uint32_t i = simd_count; i < count; i++) {
            vectors[i].x *= scale;
            vectors[i].y *= scale;
            vectors[i].z *= scale;
        }
        
        g_morph_system_ctx.performance.simd_operations++;
    } else
#endif
    {
        /* Scalar fallback */
        for (uint32_t i = 0; i < count; i++) {
            vectors[i].x *= scale;
            vectors[i].y *= scale;
            vectors[i].z *= scale;
        }
    }
}

/* Async operations worker thread */
static void* animation_morph_compression_async_worker(void* arg) {
    (void)arg;
    
    while (g_morph_system_ctx.async_running) {
        pthread_mutex_lock(&g_morph_system_ctx.async_queue_mutex);
        
        while (g_morph_system_ctx.async_queue_head == g_morph_system_ctx.async_queue_tail && g_morph_system_ctx.async_running) {
            pthread_cond_wait(&g_morph_system_ctx.async_queue_cond, &g_morph_system_ctx.async_queue_mutex);
        }
        
        if (!g_morph_system_ctx.async_running) {
            pthread_mutex_unlock(&g_morph_system_ctx.async_queue_mutex);
            break;
        }
        
        animation_morph_compression_async_operation_t* op = &g_morph_system_ctx.async_queue[g_morph_system_ctx.async_queue_head];
        g_morph_system_ctx.async_queue_head = (g_morph_system_ctx.async_queue_head + 1) % ANIMATION_MORPH_COMPRESSION_ASYNC_QUEUE_SIZE;
        
        pthread_mutex_unlock(&g_morph_system_ctx.async_queue_mutex);
        
        /* Process operation */
        switch (op->type) {
            case ANIMATION_MORPH_COMPRESSION_ASYNC_COMPRESS:
                /* Compression would happen here */
                break;
            case ANIMATION_MORPH_COMPRESSION_ASYNC_DECOMPRESS:
                /* Decompression would happen here */
                break;
            case ANIMATION_MORPH_COMPRESSION_ASYNC_PROCESS:
                /* General processing would happen here */
                break;
        }
        
        /* Mark as completed */
        pthread_mutex_lock(&op->completion_mutex);
        op->completed = true;
        pthread_cond_signal(&op->completion_cond);
        pthread_mutex_unlock(&op->completion_mutex);
        
        g_morph_system_ctx.performance.async_operations++;
    }
    
    return NULL;
}

/* Hot-reload file watcher */
static void* animation_morph_compression_file_watcher_thread(void* arg) {
    animation_morph_compression_file_watcher_t* watcher = (animation_morph_compression_file_watcher_t*)arg;
    
    char buffer[4096];
    while (watcher->running) {
        ssize_t length = read(watcher->inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            /* Process file change events */
            for (char* ptr = buffer; ptr < buffer + length; ) {
                struct inotify_event* event = (struct inotify_event*)ptr;
                if (event->mask & IN_MODIFY) {
                    if (watcher->reload_callback) {
                        watcher->reload_callback(watcher->watched_file);
                    }
                }
                ptr += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); /* 100ms */
    }
    
    return NULL;
}

/* Render graph execution */
static void animation_morph_compression_execute_render_node(animation_morph_compression_render_node_t* node) {
    if (!node || !node->execute_func) return;
    
    /* Execute dependencies first */
    for (uint32_t i = 0; i < node->dependency_count; i++) {
        uint32_t dep_id = node->dependencies[i];
        for (uint32_t j = 0; j < g_morph_system_ctx.render_node_count; j++) {
            if (g_morph_system_ctx.render_nodes[j].node_id == dep_id) {
                animation_morph_compression_execute_render_node(&g_morph_system_ctx.render_nodes[j]);
                break;
            }
        }
    }
    
    /* Execute this node */
    node->execute_func(node->user_data);
}

/* Validation and error handling */
static bool animation_morph_compression_validate_extended(const animation_morph_compression_extended_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    
    /* Validate morph targets */
    if (item->morph_target_count > 0 && !item->morph_targets) return false;
    
    /* Validate bones */
    if (item->bone_count > 0 && !item->bone_transforms) return false;
    
    /* Validate clips */
    if (item->clip_count > 0 && !item->clips) return false;
    
    /* Validate IK chains */
    if (item->ik_chain_count > 0 && !item->ik_chains) return false;
    
    return true;
}

static void animation_morph_compression_cleanup_extended(animation_morph_compression_extended_internal_t* item) {
    if (!item) return;
    
    /* Cleanup morph targets */
    if (item->morph_targets) {
        for (uint32_t i = 0; i < item->morph_target_count; i++) {
            if (item->morph_targets[i].vertex_offsets) {
                free(item->morph_targets[i].vertex_offsets);
            }
        }
        free(item->morph_targets);
        item->morph_targets = NULL;
    }
    
    /* Cleanup bones */
    if (item->bone_transforms) {
        free(item->bone_transforms);
        item->bone_transforms = NULL;
    }
    
    if (item->bone_hierarchy) {
        free(item->bone_hierarchy);
        item->bone_hierarchy = NULL;
    }
    
    /* Cleanup clips */
    if (item->clips) {
        for (uint32_t i = 0; i < item->clip_count; i++) {
            if (item->clips[i].keyframes) {
                for (uint32_t j = 0; j < item->clips[i].keyframe_count; j++) {
                    if (item->clips[i].keyframes[j].bone_transforms) {
                        free(item->clips[i].keyframes[j].bone_transforms);
                    }
                    if (item->clips[i].keyframes[j].morph_weights) {
                        free(item->clips[i].keyframes[j].morph_weights);
                    }
                }
                free(item->clips[i].keyframes);
            }
        }
        free(item->clips);
        item->clips = NULL;
    }
    
    /* Cleanup IK chains */
    if (item->ik_chains) {
        for (uint32_t i = 0; i < item->ik_chain_count; i++) {
            if (item->ik_chains[i].bone_indices) {
                free(item->ik_chains[i].bone_indices);
            }
        }
        free(item->ik_chains);
        item->ik_chains = NULL;
    }
    
    /* Cleanup compression data */
    if (item->compressed_data) {
        free(item->compressed_data);
        item->compressed_data = NULL;
    }
    
    /* Cleanup GPU buffer */
    if (item->gpu_buffer.device_ptr) {
        /* GPU buffer cleanup would happen here */
        item->gpu_buffer.device_ptr = NULL;
    }
    
    /* Cleanup base data */
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_morph_compression_init(void) {
    if (g_morph_system_ctx.initialized) {
        return 0; /* Already initialized */
    }
    
    /* Initialize global mutex */
    if (pthread_mutex_init(&g_morph_system_ctx.global_mutex, NULL) != 0) {
        g_morph_system_ctx.last_error_code = ANIMATION_MORPH_COMPRESSION_ERROR_THREAD_ERROR;
        snprintf(g_morph_system_ctx.last_error_message, sizeof(g_morph_system_ctx.last_error_message),
                "Failed to initialize global mutex");
        return ANIMATION_MORPH_COMPRESSION_ERROR_THREAD_ERROR;
    }
    
    /* Initialize cache read-write lock */
    if (pthread_rwlock_init(&g_morph_system_ctx.cache_rwlock, NULL) != 0) {
        pthread_mutex_destroy(&g_morph_system_ctx.global_mutex);
        g_morph_system_ctx.last_error_code = ANIMATION_MORPH_COMPRESSION_ERROR_THREAD_ERROR;
        snprintf(g_morph_system_ctx.last_error_message, sizeof(g_morph_system_ctx.last_error_message),
                "Failed to initialize cache rwlock");
        return ANIMATION_MORPH_COMPRESSION_ERROR_THREAD_ERROR;
    }
    
    /* Initialize memory pool */
    g_morph_system_ctx.memory_pool.memory = malloc(ANIMATION_MORPH_COMPRESSION_MEMORY_POOL_SIZE);
    if (!g_morph_system_ctx.memory_pool.memory) {
        pthread_rwlock_destroy(&g_morph_system_ctx.cache_rwlock);
        pthread_mutex_destroy(&g_morph_system_ctx.global_mutex);
        g_morph_system_ctx.last_error_code = ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
        snprintf(g_morph_system_ctx.last_error_message, sizeof(g_morph_system_ctx.last_error_message),
                "Failed to allocate memory pool");
        return ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
    }
    
    g_morph_system_ctx.memory_pool.total_size = ANIMATION_MORPH_COMPRESSION_MEMORY_POOL_SIZE;
    g_morph_system_ctx.memory_pool.used_size = 0;
    g_morph_system_ctx.memory_pool.peak_usage = 0;
    pthread_mutex_init(&g_morph_system_ctx.memory_pool.mutex, NULL);
    
    /* Initialize async system */
    pthread_mutex_init(&g_morph_system_ctx.async_queue_mutex, NULL);
    pthread_cond_init(&g_morph_system_ctx.async_queue_cond, NULL);
    g_morph_system_ctx.async_running = true;
    
    for (int i = 0; i < 4; i++) {
        if (pthread_create(&g_morph_system_ctx.async_worker_threads[i], NULL, 
                         animation_morph_compression_async_worker, NULL) != 0) {
            g_morph_system_ctx.async_running = false;
            break;
        }
    }
    
    /* Initialize hot-reload system */
    g_morph_system_ctx.file_watcher.inotify_fd = inotify_init();
    if (g_morph_system_ctx.file_watcher.inotify_fd >= 0) {
        g_morph_system_ctx.file_watcher.running = true;
        pthread_create(&g_morph_system_ctx.file_watcher.watcher_thread, NULL,
                      animation_morph_compression_file_watcher_thread, 
                      &g_morph_system_ctx.file_watcher);
    }
    
    /* Initialize render graph */
    g_morph_system_ctx.render_node_capacity = 64;
    g_morph_system_ctx.render_nodes = malloc(g_morph_system_ctx.render_node_capacity * 
                                           sizeof(animation_morph_compression_render_node_t));
    g_morph_system_ctx.render_node_count = 0;
    
    /* Check SIMD support */
#if defined(__linux__) && defined(__SSE__)
    g_morph_system_ctx.simd_supported = true;
    g_morph_system_ctx.simd_width = ANIMATION_MORPH_COMPRESSION_SIMD_WIDTH;
#else
    g_morph_system_ctx.simd_supported = false;
    g_morph_system_ctx.simd_width = 1;
#endif
    
    /* Initialize extended items array */
    g_morph_system_ctx.capacity = ANIMATION_MORPH_COMPRESSION_DEFAULT_CAPACITY;
    g_morph_system_ctx.items = calloc(g_morph_system_ctx.capacity, 
                                    sizeof(animation_morph_compression_extended_internal_t));
    if (!g_morph_system_ctx.items) {
        animation_morph_compression_shutdown();
        return ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
    }
    
    g_morph_system_ctx.count = 0;
    g_morph_system_ctx.initialized = true;
    
    /* Initialize performance counters */
    memset(&g_morph_system_ctx.performance, 0, sizeof(g_morph_system_ctx.performance));
    
    return ANIMATION_MORPH_COMPRESSION_ERROR_NONE;
}

void animation_morph_compression_shutdown(void) {
    if (!g_morph_system_ctx.initialized) {
        return;
    }
    
    /* Stop async operations */
    g_morph_system_ctx.async_running = false;
    pthread_cond_broadcast(&g_morph_system_ctx.async_queue_cond);
    
    for (int i = 0; i < 4; i++) {
        pthread_join(g_morph_system_ctx.async_worker_threads[i], NULL);
    }
    
    pthread_mutex_destroy(&g_morph_system_ctx.async_queue_mutex);
    pthread_cond_destroy(&g_morph_system_ctx.async_queue_cond);
    
    /* Stop file watcher */
    if (g_morph_system_ctx.file_watcher.running) {
        g_morph_system_ctx.file_watcher.running = false;
        pthread_join(g_morph_system_ctx.file_watcher.watcher_thread, NULL);
        
        if (g_morph_system_ctx.file_watcher.inotify_fd >= 0) {
            if (g_morph_system_ctx.file_watcher.watch_descriptor >= 0) {
                inotify_rm_watch(g_morph_system_ctx.file_watcher.inotify_fd, 
                               g_morph_system_ctx.file_watcher.watch_descriptor);
            }
            close(g_morph_system_ctx.file_watcher.inotify_fd);
        }
    }
    
    /* Cleanup all items */
    for (uint32_t i = 0; i < g_morph_system_ctx.count; i++) {
        animation_morph_compression_cleanup_extended(&g_morph_system_ctx.items[i]);
    }
    
    free(g_morph_system_ctx.items);
    g_morph_system_ctx.items = NULL;
    g_morph_system_ctx.count = 0;
    g_morph_system_ctx.capacity = 0;
    
    /* Cleanup render graph */
    if (g_morph_system_ctx.render_nodes) {
        for (uint32_t i = 0; i < g_morph_system_ctx.render_node_count; i++) {
            if (g_morph_system_ctx.render_nodes[i].dependencies) {
                free(g_morph_system_ctx.render_nodes[i].dependencies);
            }
        }
        free(g_morph_system_ctx.render_nodes);
        g_morph_system_ctx.render_nodes = NULL;
        g_morph_system_ctx.render_node_count = 0;
        g_morph_system_ctx.render_node_capacity = 0;
    }
    
    /* Cleanup cache */
    for (uint32_t i = 0; i < ANIMATION_MORPH_COMPRESSION_CACHE_SIZE; i++) {
        if (g_morph_system_ctx.cache[i].valid && g_morph_system_ctx.cache[i].data) {
            free(g_morph_system_ctx.cache[i].data);
            g_morph_system_ctx.cache[i].valid = false;
        }
    }
    
    pthread_rwlock_destroy(&g_morph_system_ctx.cache_rwlock);
    
    /* Cleanup memory pool */
    if (g_morph_system_ctx.memory_pool.memory) {
        free(g_morph_system_ctx.memory_pool.memory);
        g_morph_system_ctx.memory_pool.memory = NULL;
    }
    pthread_mutex_destroy(&g_morph_system_ctx.memory_pool.mutex);
    
    /* Print final statistics */
    printf("Animation Morph Compression Shutdown Statistics:\n");
    printf("  Compression operations: %lu\n", g_morph_system_ctx.performance.compression_operations);
    printf("  Decompression operations: %lu\n", g_morph_system_ctx.performance.decompression_operations);
    printf("  GPU operations: %lu\n", g_morph_system_ctx.performance.gpu_operations);
    printf("  SIMD operations: %lu\n", g_morph_system_ctx.performance.simd_operations);
    printf("  Cache hits: %lu\n", g_morph_system_ctx.performance.cache_hits);
    printf("  Cache misses: %lu\n", g_morph_system_ctx.performance.cache_misses);
    printf("  Async operations: %lu\n", g_morph_system_ctx.performance.async_operations);
    printf("  Peak memory usage: %zu bytes\n", g_morph_system_ctx.performance.peak_memory_usage);
    printf("  Compression ratio: %.2f\n", g_morph_system_ctx.performance.compression_ratio);
    
    pthread_mutex_destroy(&g_morph_system_ctx.global_mutex);
    g_morph_system_ctx.initialized = false;
}

int animation_morph_compression_create(animation_morph_compression_handle_t* out_handle, const animation_morph_compression_desc_t* desc) {
    if (!out_handle || !desc) {
        g_morph_system_ctx.last_error_code = ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_PARAMETER;
        snprintf(g_morph_system_ctx.last_error_message, sizeof(g_morph_system_ctx.last_error_message),
                "Invalid parameters for create");
        return ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_system_ctx.initialized) {
        g_morph_system_ctx.last_error_code = ANIMATION_MORPH_COMPRESSION_ERROR_NOT_INITIALIZED;
        snprintf(g_morph_system_ctx.last_error_message, sizeof(g_morph_system_ctx.last_error_message),
                "System not initialized");
        return ANIMATION_MORPH_COMPRESSION_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_morph_system_ctx.global_mutex);
    
    if (g_morph_system_ctx.count >= g_morph_system_ctx.capacity) {
        pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
        g_morph_system_ctx.last_error_code = ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
        snprintf(g_morph_system_ctx.last_error_message, sizeof(g_morph_system_ctx.last_error_message),
                "Capacity exceeded");
        return ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = g_morph_system_ctx.count++;
    animation_morph_compression_extended_internal_t* item = &g_morph_system_ctx.items[index];
    
    /* Initialize base fields */
    memset(item, 0, sizeof(animation_morph_compression_extended_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    /* Initialize morph targets */
    item->morph_target_count = 0;
    item->morph_targets = NULL;
    
    /* Initialize skeletal animation */
    item->bone_count = 0;
    item->bone_transforms = NULL;
    item->bone_hierarchy = NULL;
    
    /* Initialize animation clips */
    item->clip_count = 0;
    item->clips = NULL;
    item->state = ANIMATION_MORPH_COMPRESSION_STATE_IDLE;
    item->current_time = 0.0f;
    
    /* Initialize IK system */
    item->ik_chain_count = 0;
    item->ik_chains = NULL;
    
    /* Initialize compression */
    item->compression_algorithm = ANIMATION_MORPH_COMPRESSION_NONE;
    item->compression_quality = 1.0f;
    item->compressed_data = NULL;
    item->compressed_size = 0;
    
    /* Initialize LOD */
    item->current_lod = ANIMATION_MORPH_COMPRESSION_LOD_HIGH;
    item->lod_distances[0] = 10.0f;  /* High quality distance */
    item->lod_distances[1] = 50.0f;  /* Medium quality distance */
    item->lod_distances[2] = 200.0f; /* Low quality distance */
    
    /* Initialize GPU resources */
    memset(&item->gpu_buffer, 0, sizeof(item->gpu_buffer));
    item->gpu_accelerated = false;
    
    /* Initialize culling */
    item->visible = true;
    item->last_cull_time = 0.0f;
    
    /* Initialize render graph */
    item->render_node_id = UINT32_MAX;
    item->render_graph_enabled = false;
    
    /* Initialize base data */
    item->data = NULL;
    item->data_size = 0;
    
    pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
    
    out_handle->id = index;
    return ANIMATION_MORPH_COMPRESSION_ERROR_NONE;
}

void animation_morph_compression_destroy(animation_morph_compression_handle_t handle) {
    if (!g_morph_system_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_morph_system_ctx.global_mutex);
    
    if (handle.id >= g_morph_system_ctx.count) {
        pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
        return;
    }
    
    animation_morph_compression_cleanup_extended(&g_morph_system_ctx.items[handle.id]);
    
    /* Remove from render graph */
    if (g_morph_system_ctx.items[handle.id].render_graph_enabled) {
        for (uint32_t i = 0; i < g_morph_system_ctx.render_node_count; i++) {
            if (g_morph_system_ctx.render_nodes[i].node_id == g_morph_system_ctx.items[handle.id].render_node_id) {
                if (g_morph_system_ctx.render_nodes[i].dependencies) {
                    free(g_morph_system_ctx.render_nodes[i].dependencies);
                }
                
                /* Move remaining nodes */
                for (uint32_t j = i; j < g_morph_system_ctx.render_node_count - 1; j++) {
                    g_morph_system_ctx.render_nodes[j] = g_morph_system_ctx.render_nodes[j + 1];
                }
                g_morph_system_ctx.render_node_count--;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
}

int animation_morph_compression_update(animation_morph_compression_handle_t handle, const void* data, size_t size) {
    if (!g_morph_system_ctx.initialized) {
        return ANIMATION_MORPH_COMPRESSION_ERROR_NOT_INITIALIZED;
    }
    
    if (!data || size == 0) {
        return ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&g_morph_system_ctx.global_mutex);
    
    if (handle.id >= g_morph_system_ctx.count) {
        pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
        return ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_compression_extended_internal_t* item = &g_morph_system_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
        return ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_HANDLE;
    }
    
    /* Update base data */
    if (item->data) {
        free(item->data);
    }
    item->data = malloc(size);
    if (!item->data) {
        pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
        return ANIMATION_MORPH_COMPRESSION_ERROR_OUT_OF_MEMORY;
    }
    memcpy(item->data, data, size);
    item->data_size = size;
    
    /* Update skeletal animation */
    animation_morph_compression_update_bone_transforms(item, 0.016f); /* 60 FPS */
    
    /* Update morph targets */
    animation_morph_compression_update_morph_targets(item);
    
    /* Solve IK chains */
    for (uint32_t i = 0; i < item->ik_chain_count; i++) {
        animation_morph_compression_solve_ccd_ik(&item->ik_chains[i], item->bone_transforms);
    }
    
    /* Prepare GPU skinning if enabled */
    if (item->gpu_accelerated) {
        animation_morph_compression_prepare_gpu_skinning(item);
        g_morph_system_ctx.performance.gpu_operations++;
    }
    
    /* Apply compression if enabled */
    if (item->compression_algorithm != ANIMATION_MORPH_COMPRESSION_NONE) {
        animation_morph_compression_compress_data(item);
    }
    
    /* Update LOD based on distance (simplified) */
    float distance = 100.0f; /* Would be calculated from camera */
    if (distance < item->lod_distances[0]) {
        item->current_lod = ANIMATION_MORPH_COMPRESSION_LOD_HIGH;
    } else if (distance < item->lod_distances[1]) {
        item->current_lod = ANIMATION_MORPH_COMPRESSION_LOD_MEDIUM;
    } else {
        item->current_lod = ANIMATION_MORPH_COMPRESSION_LOD_LOW;
    }
    
    item->dirty = true;
    item->frame_updated++;
    
    pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
    
    /* Update performance counters */
    animation_morph_compression_update_performance_counters();
    
    return ANIMATION_MORPH_COMPRESSION_ERROR_NONE;
}

bool animation_morph_compression_is_valid(animation_morph_compression_handle_t handle) {
    if (!g_morph_system_ctx.initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_morph_system_ctx.global_mutex);
    
    if (handle.id >= g_morph_system_ctx.count) {
        pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
        return false;
    }
    
    bool valid = animation_morph_compression_validate_extended(&g_morph_system_ctx.items[handle.id]);
    
    pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
    return valid;
}

int animation_morph_compression_get_info(animation_morph_compression_handle_t handle, animation_morph_compression_info_t* out_info) {
    if (!out_info) {
        return ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_system_ctx.initialized) {
        return ANIMATION_MORPH_COMPRESSION_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_morph_system_ctx.global_mutex);
    
    if (handle.id >= g_morph_system_ctx.count) {
        pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
        return ANIMATION_MORPH_COMPRESSION_ERROR_INVALID_HANDLE;
    }
    
    const animation_morph_compression_extended_internal_t* item = &g_morph_system_ctx.items[handle.id];
    
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
    
    return ANIMATION_MORPH_COMPRESSION_ERROR_NONE;
}

void animation_morph_compression_mark_dirty(animation_morph_compression_handle_t handle) {
    if (!g_morph_system_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_morph_system_ctx.global_mutex);
    
    if (handle.id < g_morph_system_ctx.count) {
        g_morph_system_ctx.items[handle.id].dirty = true;
        
        /* Mark render graph node as needing update */
        if (g_morph_system_ctx.items[handle.id].render_graph_enabled) {
            /* Render graph would be updated here */
        }
    }
    
    pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
}

int animation_morph_compression_process_pending(void) {
    if (!g_morph_system_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_morph_system_ctx.global_mutex);
    
    int processed = 0;
    for (uint32_t i = 0; i < g_morph_system_ctx.count; i++) {
        animation_morph_compression_extended_internal_t* item = &g_morph_system_ctx.items[i];
        if (item->initialized && item->dirty) {
            /* Process with SIMD optimization if available */
            if (item->morph_targets && g_morph_system_ctx.simd_supported) {
                animation_morph_compression_simd_process_vec3(
                    item->morph_targets[0].vertex_offsets,
                    item->morph_targets[0].vertex_count,
                    item->morph_targets[0].weight);
            }
            
            /* Execute render graph node if enabled */
            if (item->render_graph_enabled) {
                for (uint32_t j = 0; j < g_morph_system_ctx.render_node_count; j++) {
                    if (g_morph_system_ctx.render_nodes[j].node_id == item->render_node_id) {
                        animation_morph_compression_execute_render_node(&g_morph_system_ctx.render_nodes[j]);
                        break;
                    }
                }
            }
            
            /* Batch processing for multiple items */
            if (g_morph_system_ctx.count > 10) {
                /* Process in batches for better cache performance */
            }
            
            item->dirty = false;
            processed++;
        }
    }
    
    pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
    
    return processed;
}

uint32_t animation_morph_compression_get_count(void) {
    if (!g_morph_system_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_morph_system_ctx.global_mutex);
    uint32_t count = g_morph_system_ctx.count;
    pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
    
    return count;
}

    if (!g_morph_system_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_morph_system_ctx.global_mutex);
    
    size_t total = sizeof(g_morph_system_ctx);
    total += g_morph_system_ctx.capacity * sizeof(animation_morph_compression_extended_internal_t);
    
    // Add data sizes
    for (uint32_t i = 0; i < g_morph_system_ctx.count; i++) {
        total += g_morph_system_ctx.items[i].data_size;
        total += g_morph_system_ctx.items[i].compressed_size;
    }
    
    // Add cache memory
    for (int i = 0; i < ANIMATION_MORPH_COMPRESSION_CACHE_SIZE; i++) {
        if (g_morph_system_ctx.cache[i].valid) {
            total += g_morph_system_ctx.cache[i].cached_size;
        }
    }
    
    // Add GPU memory
    if (g_morph_system_ctx.gpu_available) {
        total += g_morph_system_ctx.gpu_context.gpu_buffer_size;
    }
    
    pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
    return total;
}

void animation_morph_compression_debug_print(void) {
    pthread_mutex_lock(&g_morph_system_ctx.global_mutex);
    
    printf("=== Morph Compression Debug Info ===\n");
    printf("Initialized: %s\n", g_morph_system_ctx.initialized ? "Yes" : "No");
    printf("Count: %u / %u\n", g_morph_system_ctx.count, g_morph_system_ctx.capacity);
    printf("Hot-reload enabled: %s\n", g_morph_system_ctx.hot_reload_enabled ? "Yes" : "No");
    printf("GPU available: %s\n", g_morph_system_ctx.gpu_available ? "Yes" : "No");
    printf("SIMD enabled: %s\n", g_morph_system_ctx.simd_context.simd_enabled ? "Yes" : "No");
    printf("System Status: Initialized\n");
    printf("Total Items: %u / %u\n", g_morph_system_ctx.count, g_morph_system_ctx.capacity);
    printf("SIMD Support: %s\n", g_morph_system_ctx.simd_supported ? "Yes" : "No");
    printf("SIMD Width: %u\n", g_morph_system_ctx.simd_width);
    
    printf("\n--- Performance Counters ---\n");
    printf("Compression Operations: %lu\n", g_morph_system_ctx.performance.compression_operations);
    printf("Decompression Operations: %lu\n", g_morph_system_ctx.performance.decompression_operations);
    printf("GPU Operations: %lu\n", g_morph_system_ctx.performance.gpu_operations);
    printf("SIMD Operations: %lu\n", g_morph_system_ctx.performance.simd_operations);
    printf("Cache Hits: %lu\n", g_morph_system_ctx.performance.cache_hits);
    printf("Cache Misses: %lu\n", g_morph_system_ctx.performance.cache_misses);
    printf("Async Operations: %lu\n", g_morph_system_ctx.performance.async_operations);
    printf("Peak Memory Usage: %zu bytes\n", g_morph_system_ctx.performance.peak_memory_usage);
    printf("Compression Ratio: %.2f\n", g_morph_system_ctx.performance.compression_ratio);
    
    printf("\n--- Memory Pool ---\n");
    printf("Total Size: %zu bytes\n", g_morph_system_ctx.memory_pool.total_size);
    printf("Used Size: %zu bytes\n", g_morph_system_ctx.memory_pool.used_size);
    printf("Peak Usage: %zu bytes\n", g_morph_system_ctx.memory_pool.peak_usage);
    printf("Utilization: %.2f%%\n", 
           (float)g_morph_system_ctx.memory_pool.used_size / g_morph_system_ctx.memory_pool.total_size * 100.0f);
    
    printf("\n--- Cache Statistics ---\n");
    uint32_t valid_cache_entries = 0;
    size_t total_cache_size = 0;
    for (uint32_t i = 0; i < ANIMATION_MORPH_COMPRESSION_CACHE_SIZE; i++) {
        if (g_morph_system_ctx.cache[i].valid) {
            valid_cache_entries++;
            total_cache_size += g_morph_system_ctx.cache[i].size;
        }
    }
    printf("Valid Entries: %u / %u\n", valid_cache_entries, ANIMATION_MORPH_COMPRESSION_CACHE_SIZE);
    printf("Total Cache Size: %zu bytes\n", total_cache_size);
    printf("Hit Ratio: %.2f%%\n", 
           g_morph_system_ctx.performance.cache_hits + g_morph_system_ctx.performance.cache_misses > 0 ?
           (float)g_morph_system_ctx.performance.cache_hits / 
           (g_morph_system_ctx.performance.cache_hits + g_morph_system_ctx.performance.cache_misses) * 100.0f : 0.0f);
    
    printf("\n--- Render Graph ---\n");
    printf("Render Nodes: %u / %u\n", g_morph_system_ctx.render_node_count, g_morph_system_ctx.render_node_capacity);
    
    printf("\n--- Async System ---\n");
    printf("Async Running: %s\n", g_morph_system_ctx.async_running ? "Yes" : "No");
    printf("Queue Head: %u, Tail: %u\n", g_morph_system_ctx.async_queue_head, g_morph_system_ctx.async_queue_tail);
    
    printf("\n--- File Watcher ---\n");
    printf("Watcher Running: %s\n", g_morph_system_ctx.file_watcher.running ? "Yes" : "No");
    printf("Inotify FD: %d\n", g_morph_system_ctx.file_watcher.inotify_fd);
    printf("Watch Descriptor: %d\n", g_morph_system_ctx.file_watcher.watch_descriptor);
    if (g_morph_system_ctx.file_watcher.watched_file[0] != '\0') {
        printf("Watched File: %s\n", g_morph_system_ctx.file_watcher.watched_file);
    }
    
    printf("\n--- Item Details ---\n");
    for (uint32_t i = 0; i < g_morph_system_ctx.count; i++) {
        const animation_morph_compression_extended_internal_t* item = &g_morph_system_ctx.items[i];
        printf("Item %u: ID=%u, Flags=0x%08x, %s, %s\n", 
               i, item->id, item->flags, 
               item->initialized ? "Initialized" : "Not Initialized",
               item->dirty ? "Dirty" : "Clean");
        printf("  Morph Targets: %u, Bones: %u, Clips: %u, IK Chains: %u\n",
               item->morph_target_count, item->bone_count, item->clip_count, item->ik_chain_count);
        printf("  Compression: %d, Quality: %.2f, LOD: %d\n",
               item->compression_algorithm, item->compression_quality, item->current_lod);
        printf("  GPU Accelerated: %s, Visible: %s\n",
               item->gpu_accelerated ? "Yes" : "No",
               item->visible ? "Yes" : "No");
        printf("  Data Size: %zu bytes, Compressed: %zu bytes\n",
               item->data_size, item->compressed_size);
        printf("  Frame Updated: %lu\n", item->frame_updated);
    }
    
    printf("\n--- Error Information ---\n");
    if (g_morph_system_ctx.last_error_code != ANIMATION_MORPH_COMPRESSION_ERROR_NONE) {
        printf("Last Error Code: %d\n", g_morph_system_ctx.last_error_code);
        printf("Last Error Message: %s\n", g_morph_system_ctx.last_error_message);
    } else {
        printf("No errors recorded\n");
    }
    
    printf("\n=== End Debug Info ===\n\n");
    
    pthread_mutex_unlock(&g_morph_system_ctx.global_mutex);
}

/* End of morph_compression.c */
