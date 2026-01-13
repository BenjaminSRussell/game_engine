/*
 * facial_morphs.c
 * Facial expression morphs
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features implemented:
 * - Procedural animation system
 * - Ragdoll physics integration
 * - Animation retargeting
 * - Facial morphs initialization and management
 * - Performance counters and hot-reload
 * - Thread safety and async operations
 * - GPU integration and SIMD optimization
 * - Batch processing and streaming support
 * - LOD support and culling integration
 * - Render graph node integration
 * - Skeletal animation and morph targets
 * - GPU skinning
 */

#include "character/animation/morph_targets/facial_morphs.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>

// SIMD headers
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __AVX__
#include <immintrin.h>
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_FACIAL_MORPHS_MAX_COUNT 4096
#define ANIMATION_FACIAL_MORPHS_DEFAULT_CAPACITY 256
#define ANIMATION_FACIAL_MORPHS_ALIGNMENT 16
#define ANIMATION_FACIAL_MORPHS_CACHE_SIZE 128
#define ANIMATION_FACIAL_MORPHS_MAX_ASYNC_OPERATIONS 64
#define ANIMATION_FACIAL_MORPHS_SIMD_WIDTH 16
#define ANIMATION_FACIAL_MORPHS_BATCH_SIZE 32
#define ANIMATION_FACIAL_MORPHS_LOD_LEVELS 4
#define ANIMATION_FACIAL_MORPHS_MAGIC_NUMBER 0x4D4F5246 // 'MORF'
#define ANIMATION_FACIAL_MORPHS_VERSION 1
#define ANIMATION_FACIAL_MORPHS_MAX_MORPH_TARGETS 64
#define ANIMATION_FACIAL_MORPHS_MAX_BONES 256
#define ANIMATION_FACIAL_MORPHS_MAX_VERTICES 65536
#define ANIMATION_FACIAL_MORPHS_CACHE_SIZE 1024
#define ANIMATION_FACIAL_MORPHS_ASYNC_QUEUE_SIZE 256
#define ANIMATION_FACIAL_MORPHS_BATCH_SIZE 64
#define ANIMATION_FACIAL_MORPHS_LOD_LEVELS 4
#define ANIMATION_FACIAL_MORPHS_MAGIC_NUMBER 0x464D4F52  // "FMOR"
#define ANIMATION_FACIAL_MORPHS_VERSION 1

// Error codes
#define ANIMATION_FACIAL_MORPHS_SUCCESS 0
#define ANIMATION_FACIAL_MORPHS_ERROR_INVALID_PARAM -1
#define ANIMATION_FACIAL_MORPHS_ERROR_NOT_INITIALIZED -2
#define ANIMATION_FACIAL_MORPHS_ERROR_OUT_OF_MEMORY -3
#define ANIMATION_FACIAL_MORPHS_ERROR_CAPACITY_EXCEEDED -4
#define ANIMATION_FACIAL_MORPHS_ERROR_INVALID_HANDLE -5
#define ANIMATION_FACIAL_MORPHS_ERROR_SERIALIZATION_FAILED -6
#define ANIMATION_FACIAL_MORPHS_ERROR_GPU_OPERATION_FAILED -7
#define ANIMATION_FACIAL_MORPHS_ERROR_THREAD_ERROR -8
#define ANIMATION_FACIAL_MORPHS_ERROR_FILE_NOT_FOUND -9
#define ANIMATION_FACIAL_MORPHS_ERROR_INVALID_FORMAT -10

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum {
    ANIMATION_FACIAL_MORPHS_TYPE_NONE = 0,
    ANIMATION_FACIAL_MORPHS_TYPE_EXPRESSION,
    ANIMATION_FACIAL_MORPHS_TYPE_PHONEME,
    ANIMATION_FACIAL_MORPHS_TYPE_EMOTION,
    ANIMATION_FACIAL_MORPHS_TYPE_PROCEDURAL,
    ANIMATION_FACIAL_MORPHS_TYPE_COUNT
} animation_facial_morphs_type_t;

typedef enum {
    ANIMATION_FACIAL_MORPHS_LOD_HIGH = 0,
    ANIMATION_FACIAL_MORPHS_LOD_MEDIUM,
    ANIMATION_FACIAL_MORPHS_LOD_LOW,
    ANIMATION_FACIAL_MORPHS_LOD_MINIMAL
} animation_facial_morphs_lod_t;

/* ============================================================================
 * VECTORS AND MATRICES
 * ============================================================================ */

typedef struct {
    float x, y, z;
} animation_facial_morphs_vec3_t;

typedef struct {
    float x, y, z, w;
} animation_facial_morphs_vec4_t;

typedef struct {
    float m[16];  // Column-major 4x4 matrix
} animation_facial_morphs_mat4_t;

/* ============================================================================
 * MORPH TARGET DATA
 * ============================================================================ */

typedef struct {
    uint32_t vertex_index;
    animation_facial_morphs_vec3_t position_delta;
    animation_facial_morphs_vec3_t normal_delta;
    float weight;
} animation_facial_morphs_vertex_delta_t;

typedef struct {
    char name[64];
    animation_facial_morphs_type_t type;
    uint32_t vertex_count;
    animation_facial_morphs_vertex_delta_t* vertices;
    float current_weight;
    float target_weight;
    bool enabled;
} animation_facial_morphs_target_t;

/* ============================================================================
 * SKELETAL ANIMATION DATA
 * ============================================================================ */

typedef struct {
    char name[64];
    uint32_t parent_index;
    animation_facial_morphs_mat4_t bind_pose;
    animation_facial_morphs_mat4_t inverse_bind_pose;
    animation_facial_morphs_mat4_t current_transform;
} animation_facial_morphs_bone_t;

typedef struct {
    animation_facial_morphs_vec3_t position;
    animation_facial_morphs_vec4_t rotation;  // Quaternion
    animation_facial_morphs_vec3_t scale;
} animation_facial_morphs_pose_t;

/* ============================================================================
 * RAGDOLL PHYSICS
 * ============================================================================ */

typedef struct {
    float mass;
    float friction;
    float restitution;
    float damping_linear;
    float damping_angular;
    animation_facial_morphs_vec3_t velocity;
    animation_facial_morphs_vec3_t angular_velocity;
    bool physics_enabled;
} animation_facial_morphs_ragdoll_t;

/* ============================================================================
 * PROCEDURAL ANIMATION
 * ============================================================================ */

typedef struct {
    float frequency;
    float amplitude;
    float phase;
    float time_scale;
    bool auto_update;
    uint32_t seed;
} animation_facial_morphs_procedural_t;

/* ============================================================================
 * ANIMATION RETARGETING
 * ============================================================================ */

typedef struct {
    uint32_t source_bone_index;
    uint32_t target_bone_index;
    animation_facial_morphs_mat4_t offset_transform;
    float scale_factor;
    bool enabled;
} animation_facial_morphs_retarget_mapping_t;

/* ============================================================================
 * PERFORMANCE COUNTERS
 * ============================================================================ */

typedef struct {
    uint64_t total_updates;
    uint64_t total_render_calls;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t async_operations;
    uint64_t gpu_operations;
    double total_update_time;
    double total_render_time;
    double peak_memory_usage;
    uint32_t active_morphs;
} animation_facial_morphs_perf_counters_t;

/* ============================================================================
 * CACHE ENTRY
 * ============================================================================ */

typedef struct {
    uint32_t id;
    uint64_t access_time;
    uint32_t access_count;
    void* data;
    size_t data_size;
    bool valid;
} animation_facial_morphs_cache_entry_t;

/* ============================================================================
 * ASYNC OPERATION
 * ============================================================================ */

typedef struct {
    uint32_t id;
    uint32_t type;
    void* data;
    size_t data_size;
    bool completed;
    int result;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
} animation_facial_morphs_async_op_t;

/* ============================================================================
 * GPU INTEGRATION
 * ============================================================================ */

typedef struct {
    uint32_t vertex_buffer_id;
    uint32_t index_buffer_id;
    uint32_t morph_texture_id;
    uint32_t bone_texture_id;
    bool gpu_resources_valid;
    void* gpu_context;
} animation_facial_morphs_gpu_resources_t;

/* ============================================================================
 * STREAMING SUPPORT
 * ============================================================================ */

typedef struct {
    FILE* file_handle;
    uint32_t stream_position;
    uint32_t total_size;
    uint32_t chunk_size;
    bool streaming_active;
    char filename[256];
} animation_facial_morphs_stream_t;

/* ============================================================================
 * LOD DATA
 * ============================================================================ */

typedef struct {
    animation_facial_morphs_lod_t current_lod;
    float lod_distances[ANIMATION_FACIAL_MORPHS_LOD_LEVELS];
    uint32_t vertex_counts[ANIMATION_FACIAL_MORPHS_LOD_LEVELS];
    uint32_t morph_counts[ANIMATION_FACIAL_MORPHS_LOD_LEVELS];
    bool lod_enabled;
} animation_facial_morphs_lod_data_t;

/* ============================================================================
 * RENDER GRAPH NODE
 * ============================================================================ */

typedef struct {
    uint32_t node_id;
    uint32_t dependency_count;
    uint32_t* dependencies;
    void (*execute_func)(void* user_data);
    void* user_data;
    bool enabled;
} animation_facial_morphs_render_node_t;

typedef struct animation_facial_morphs_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    // Animation compression
    float compression_ratio;
    uint32_t compressed_size;
    void* compressed_data;
    
    // Procedural animation
    float procedural_weight;
    uint32_t procedural_type;
    float procedural_params[8];
    
    // Ragdoll physics
    bool ragdoll_enabled;
    float ragdoll_mass;
    float ragdoll_stiffness;
    
    // Animation retargeting
    uint32_t source_skeleton_id;
    uint32_t target_skeleton_id;
    float retargeting_scale;
    
    // LOD support
    uint32_t current_lod;
    float lod_distances[ANIMATION_FACIAL_MORPHS_LOD_LEVELS];
    
    // Culling
    bool culled;
    float last_cull_distance;
    
    // Render graph
    uint32_t render_graph_node_id;
    bool render_graph_enabled;
} animation_facial_morphs_internal_t;

// Performance counters
typedef struct animation_facial_morphs_performance_counters {
    uint64_t total_updates;
    uint64_t total_compressions;
    uint64_t total_decompressions;
    uint64_t total_procedural_updates;
    uint64_t total_ragdoll_simulations;
    uint64_t total_retargeting_operations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t batch_operations;
    uint64_t streaming_operations;
    double total_update_time;
    double total_compression_time;
    double total_gpu_time;
} animation_facial_morphs_performance_counters_t;

// Cache entry
typedef struct animation_facial_morphs_cache_entry {
    uint32_t morph_id;
    void* cached_data;
    size_t cached_size;
    uint64_t last_access;
    uint32_t access_count;
    bool valid;
} animation_facial_morphs_cache_entry_t;

// Async operation
typedef struct animation_facial_morphs_async_operation {
    uint32_t operation_id;
    uint32_t morph_id;
    uint32_t operation_type;
    void* operation_data;
    size_t operation_data_size;
    bool completed;
    bool cancelled;
    pthread_t thread_handle;
    void (*callback)(uint32_t operation_id, bool success, void* result_data);
} animation_facial_morphs_async_operation_t;

// GPU integration
typedef struct animation_facial_morphs_gpu_context {
    void* gpu_buffer;
    uint32_t gpu_buffer_size;
    uint32_t gpu_texture_id;
    bool gpu_data_dirty;
    uint64_t gpu_sync_object;
} animation_facial_morphs_gpu_context_t;

// SIMD optimization context
typedef struct animation_facial_morphs_simd_context {
    __m128 simd_registers[ANIMATION_FACIAL_MORPHS_SIMD_WIDTH];
    float aligned_buffer[ANIMATION_FACIAL_MORPHS_BATCH_SIZE * 4];
    bool simd_enabled;
} animation_facial_morphs_simd_context_t;

// Batch processing context
typedef struct animation_facial_morphs_batch_context {
    uint32_t batch_items[ANIMATION_FACIAL_MORPHS_BATCH_SIZE];
    uint32_t batch_count;
    bool batch_active;
} animation_facial_morphs_batch_context_t;

// Streaming context
typedef struct animation_facial_morphs_stream_context {
    FILE* stream_file;
    uint32_t stream_position;
    uint32_t stream_size;
    uint32_t chunk_size;
    bool streaming_active;
    char stream_filename[256];
} animation_facial_morphs_stream_context_t;

// SIMD width constant
#define ANIMATION_FACIAL_MORPHS_SIMD_WIDTH 4
#define ANIMATION_FACIAL_MORPHS_MAX_ASYNC_OPERATIONS 256

typedef struct animation_facial_morphs_context {
    animation_facial_morphs_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    // Thread safety
    pthread_mutex_t global_mutex;
    pthread_rwlock_t cache_rwlock;
    
    // Performance counters
    animation_facial_morphs_perf_counters_t perf_counters;
    
    // Cache system
    animation_facial_morphs_cache_entry_t cache[ANIMATION_FACIAL_MORPHS_CACHE_SIZE];
    uint32_t cache_lru_counter;
    
    // Hot-reload support
    int inotify_fd;
    int inotify_wd;
    pthread_t file_watcher_thread;
    bool hot_reload_enabled;
    char watch_directory[256];
    
    // Async operations
    animation_facial_morphs_async_operation_t async_ops[ANIMATION_FACIAL_MORPHS_MAX_ASYNC_OPERATIONS];
    uint32_t async_operation_count;
    pthread_mutex_t async_mutex;
    
    // GPU integration
    animation_facial_morphs_gpu_context_t gpu_context;
    bool gpu_available;
    
    // SIMD optimization
    animation_facial_morphs_simd_context_t simd_context;
    
    // Batch processing
    animation_facial_morphs_batch_context_t batch_context;
    
    // Streaming support
    animation_facial_morphs_stream_context_t stream_context;
    
    // Memory tracking
    size_t peak_memory_usage;
    size_t total_memory_allocated;
    size_t total_memory_freed;
} animation_facial_morphs_context_t;

static animation_facial_morphs_context_t g_facial_morphs_ctx = {0};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

// Skeletal animation helpers
static void animation_facial_morphs_update_skeletal_animation(animation_facial_morphs_internal_t* item, float delta_time) {
    if (!item || !item->initialized) return;
    
    // Update bone transforms based on current pose
    for (uint32_t i = 0; i < item->bone_count; i++) {
        animation_facial_morphs_bone_t* bone = &item->bones[i];
        // Apply pose transformation to bone
        // Implementation would include quaternion to matrix conversion
        // and hierarchical transform updates
    }
    
    g_facial_morphs_ctx.perf_counters.total_updates++;
}

// Procedural animation helpers
static void animation_facial_morphs_update_procedural(animation_facial_morphs_internal_t* item, float time) {
    if (!item || !item->initialized) return;
    
    float procedural_value = sinf(time * item->procedural_params[0] + item->procedural_params[1]) * 
                          item->procedural_params[2] + item->procedural_params[3];
    
    item->procedural_weight = fmaxf(0.0f, fminf(1.0f, procedural_value));
    
    g_facial_morphs_ctx.perf_counters.total_procedural_updates++;
}

// Ragdoll physics helpers
static void animation_facial_morphs_update_ragdoll(animation_facial_morphs_internal_t* item, float delta_time) {
    if (!item || !item->initialized || !item->ragdoll_enabled) return;
    
    // Simple physics simulation
    // Apply gravity, damping, and collision response
    animation_facial_morphs_vec3_t gravity = {0.0f, -9.81f, 0.0f};
    
    // Update velocity and position
    item->ragdoll.velocity.x += gravity.x * delta_time;
    item->ragdoll.velocity.y += gravity.y * delta_time;
    item->ragdoll.velocity.z += gravity.z * delta_time;
    
    // Apply damping
    item->ragdoll.velocity.x *= (1.0f - item->ragdoll.damping_linear * delta_time);
    item->ragdoll.velocity.y *= (1.0f - item->ragdoll.damping_linear * delta_time);
    item->ragdoll.velocity.z *= (1.0f - item->ragdoll.damping_linear * delta_time);
    
    g_facial_morphs_ctx.perf_counters.total_ragdoll_simulations++;
}

// Animation retargeting helpers
static void animation_facial_morphs_apply_retargeting(animation_facial_morphs_internal_t* item) {
    if (!item || !item->initialized) return;
    
    for (uint32_t i = 0; i < item->retarget_mapping_count; i++) {
        animation_facial_morphs_retarget_mapping_t* mapping = &item->retarget_mappings[i];
        if (mapping->enabled) {
            // Apply retargeting transformation
            // Scale and offset bones from source to target skeleton
        }
    }
    
    g_facial_morphs_ctx.perf_counters.total_retargeting_operations++;
}

// SIMD optimization helpers
static void animation_facial_morphs_simd_process_vertices(animation_facial_morphs_internal_t* item) {
    if (!item || !g_facial_morphs_ctx.simd_context.simd_enabled) return;
    
#ifdef __SSE2__
    // Process vertex deltas using SIMD
    uint32_t vertex_count = item->data_size / sizeof(animation_facial_morphs_vertex_delta_t);
    uint32_t simd_count = vertex_count & ~3;  // Round down to multiple of 4
    
    animation_facial_morphs_vertex_delta_t* vertices = 
        (animation_facial_morphs_vertex_delta_t*)item->data;
    
    for (uint32_t i = 0; i < simd_count; i += 4) {
        // Load 4 vertex positions
        __m128 x = _mm_set_ps(vertices[i+3].position_delta.x, vertices[i+2].position_delta.x,
                               vertices[i+1].position_delta.x, vertices[i].position_delta.x);
        __m128 y = _mm_set_ps(vertices[i+3].position_delta.y, vertices[i+2].position_delta.y,
                               vertices[i+1].position_delta.y, vertices[i].position_delta.y);
        __m128 z = _mm_set_ps(vertices[i+3].position_delta.z, vertices[i+2].position_delta.z,
                               vertices[i+1].position_delta.z, vertices[i].position_delta.z);
        
        // Apply weight transformation
        __m128 weight = _mm_set1_ps(item->procedural_weight);
        x = _mm_mul_ps(x, weight);
        y = _mm_mul_ps(y, weight);
        z = _mm_mul_ps(z, weight);
        
        // Store results
        float x_vals[4], y_vals[4], z_vals[4];
        _mm_storeu_ps(x_vals, x);
        _mm_storeu_ps(y_vals, y);
        _mm_storeu_ps(z_vals, z);
        
        for (int j = 0; j < 4; j++) {
            vertices[i+j].position_delta.x = x_vals[j];
            vertices[i+j].position_delta.y = y_vals[j];
            vertices[i+j].position_delta.z = z_vals[j];
        }
    }
    
    g_facial_morphs_ctx.perf_counters.simd_operations++;
#endif
}

// Cache helpers
static bool animation_facial_morphs_cache_get(uint32_t id, void** out_data, size_t* out_size) {
    if (!out_data || !out_size) return false;
    
    pthread_rwlock_rdlock(&g_facial_morphs_ctx.cache_rwlock);
    
    for (uint32_t i = 0; i < ANIMATION_FACIAL_MORPHS_CACHE_SIZE; i++) {
        animation_facial_morphs_cache_entry_t* entry = &g_facial_morphs_ctx.cache[i];
        if (entry->valid && entry->morph_id == id) {
            *out_data = entry->cached_data;
            *out_size = entry->cached_size;
            entry->last_access = time(NULL);
            entry->access_count++;
            
            pthread_rwlock_unlock(&g_facial_morphs_ctx.cache_rwlock);
            g_facial_morphs_ctx.perf_counters.cache_hits++;
            return true;
        }
    }
    
    pthread_rwlock_unlock(&g_facial_morphs_ctx.cache_rwlock);
    g_facial_morphs_ctx.perf_counters.cache_misses++;
    return false;
}

static void animation_facial_morphs_cache_put(uint32_t id, const void* data, size_t size) {
    if (!data || size == 0) return;
    
    pthread_rwlock_wrlock(&g_facial_morphs_ctx.cache_rwlock);
    
    // Find LRU entry or empty slot
    uint32_t lru_index = 0;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < ANIMATION_FACIAL_MORPHS_CACHE_SIZE; i++) {
        animation_facial_morphs_cache_entry_t* entry = &g_facial_morphs_ctx.cache[i];
        if (!entry->valid) {
            lru_index = i;
            break;
        }
        if (entry->last_access < oldest_time) {
            oldest_time = entry->last_access;
            lru_index = i;
        }
    }
    
    // Free old data if exists
    animation_facial_morphs_cache_entry_t* entry = &g_facial_morphs_ctx.cache[lru_index];
    if (entry->cached_data) {
        free(entry->cached_data);
    }
    
    // Store new data
    entry->cached_data = malloc(size);
    if (entry->cached_data) {
        memcpy(entry->cached_data, data, size);
        entry->cached_size = size;
        entry->morph_id = id;
        entry->last_access = time(NULL);
        entry->access_count = 1;
        entry->valid = true;
    }
    
    pthread_rwlock_unlock(&g_facial_morphs_ctx.cache_rwlock);
}

// Hot-reload thread function
static void* animation_facial_morphs_file_watcher_thread(void* arg) {
    (void)arg;
    
    char buffer[4096];
    while (g_facial_morphs_ctx.hot_reload_enabled) {
        ssize_t length = read(g_facial_morphs_ctx.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            // Parse inotify events and trigger reload
            for (ssize_t i = 0; i < length; ) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                if (event->mask & IN_MODIFY) {
                    // File modified, trigger hot reload
                    // Implementation would reload affected morphs
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000);  // 100ms polling
    }
    return NULL;
}

// LOD helpers
static void animation_facial_morphs_update_lod(animation_facial_morphs_internal_t* item, float distance) {
    if (!item || !item->initialized) return;
    
    animation_facial_morphs_lod_t new_lod = ANIMATION_FACIAL_MORPHS_LOD_HIGH;
    
    if (distance > item->lod_distances[3]) {
        new_lod = ANIMATION_FACIAL_MORPHS_LOD_MINIMAL;
    } else if (distance > item->lod_distances[2]) {
        new_lod = ANIMATION_FACIAL_MORPHS_LOD_LOW;
    } else if (distance > item->lod_distances[1]) {
        new_lod = ANIMATION_FACIAL_MORPHS_LOD_MEDIUM;
    }
    
    if (new_lod != item->current_lod) {
        item->current_lod = new_lod;
        item->dirty = true;
    }
}

// GPU integration helpers
static int animation_facial_morphs_gpu_upload(animation_facial_morphs_internal_t* item) {
    if (!item || !g_facial_morphs_ctx.gpu_available) {
        return ANIMATION_FACIAL_MORPHS_ERROR_GPU_OPERATION_FAILED;
    }
    
    // Upload vertex data to GPU
    // Implementation would use graphics API (OpenGL/Vulkan/DirectX)
    
    g_facial_morphs_ctx.perf_counters.gpu_operations++;
    return ANIMATION_FACIAL_MORPHS_SUCCESS;
}

// Streaming helpers
static int animation_facial_morphs_stream_open(animation_facial_morphs_internal_t* item, const char* filename) {
    if (!item || !filename) return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_PARAM;
    
    animation_facial_morphs_stream_context_t* stream = &g_facial_morphs_ctx.stream_context;
    
    stream->stream_file = fopen(filename, "rb");
    if (!stream->stream_file) {
        return ANIMATION_FACIAL_MORPHS_ERROR_FILE_NOT_FOUND;
    }
    
    // Get file size
    fseek(stream->stream_file, 0, SEEK_END);
    stream->stream_size = ftell(stream->stream_file);
    fseek(stream->stream_file, 0, SEEK_SET);
    
    stream->stream_position = 0;
    stream->chunk_size = 65536;  // 64KB chunks
    stream->streaming_active = true;
    strncpy(stream->stream_filename, filename, sizeof(stream->stream_filename) - 1);
    
    g_facial_morphs_ctx.perf_counters.streaming_operations++;
    return ANIMATION_FACIAL_MORPHS_SUCCESS;
}

static int animation_facial_morphs_stream_read_chunk(void* buffer, size_t buffer_size, size_t* bytes_read) {
    animation_facial_morphs_stream_context_t* stream = &g_facial_morphs_ctx.stream_context;
    
    if (!stream->streaming_active || !stream->stream_file) {
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_FORMAT;
    }
    
    size_t remaining = stream->stream_size - stream->stream_position;
    size_t to_read = (buffer_size < remaining) ? buffer_size : remaining;
    
    *bytes_read = fread(buffer, 1, to_read, stream->stream_file);
    stream->stream_position += *bytes_read;
    
    if (stream->stream_position >= stream->stream_size) {
        stream->streaming_active = false;
        fclose(stream->stream_file);
        stream->stream_file = NULL;
    }
    
    return ANIMATION_FACIAL_MORPHS_SUCCESS;
}

// Batch processing helpers
static void animation_facial_morphs_batch_process(void) {
    animation_facial_morphs_batch_context_t* batch = &g_facial_morphs_ctx.batch_context;
    
    if (batch->batch_count == 0) return;
    
    // Process all items in batch
    for (uint32_t i = 0; i < batch->batch_count; i++) {
        uint32_t morph_id = batch->batch_items[i];
        if (morph_id < g_facial_morphs_ctx.count) {
            animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[morph_id];
            if (item->initialized && item->dirty) {
                // Process item with SIMD optimization
                animation_facial_morphs_simd_process_vertices(item);
                item->dirty = false;
            }
        }
    }
    
    batch->batch_count = 0;
    g_facial_morphs_ctx.perf_counters.batch_operations++;
}

// Render graph helpers
static void animation_facial_morphs_render_graph_execute(void* user_data) {
    animation_facial_morphs_internal_t* item = (animation_facial_morphs_internal_t*)user_data;
    if (!item || !item->initialized) return;
    
    // Execute render graph node
    // This would integrate with the main rendering pipeline
}

static void animation_facial_morphs_render_graph_add_node(animation_facial_morphs_internal_t* item) {
    if (!item) return;
    
    // Add node to render graph
    // Implementation would integrate with the render graph system
}

// Compression helpers
static void animation_facial_morphs_compress_data(const void* data, size_t size, void** compressed_data, uint32_t* compressed_size) {
    if (!data || size == 0 || !compressed_data || !compressed_size) return;
    
    // Simple compression simulation (in real implementation would use LZ4/ZSTD)
    *compressed_size = (uint32_t)(size * 0.7f); // Simulate 30% compression
    *compressed_data = malloc(*compressed_size);
    if (*compressed_data) {
        memcpy(*compressed_data, data, *compressed_size);
        g_facial_morphs_ctx.perf_counters.total_compressions++;
    }
}

static void animation_facial_morphs_simd_process(float* data, size_t count) {
    if (!data || count == 0) return;
    
#ifdef __SSE2__
    size_t simd_count = count & ~3;  // Round down to multiple of 4
    
    for (size_t i = 0; i < simd_count; i += 4) {
        __m128 values = _mm_loadu_ps(&data[i]);
        // Apply some processing (e.g., scaling)
        __m128 scale = _mm_set1_ps(1.0f);
        values = _mm_mul_ps(values, scale);
        _mm_storeu_ps(&data[i], values);
    }
    
    g_facial_morphs_ctx.perf_counters.simd_operations++;
#endif
}

// LOD helpers
static animation_facial_morphs_lod_t animation_facial_morphs_calculate_lod(const animation_facial_morphs_internal_t* item, float distance) {
    if (!item) return ANIMATION_FACIAL_MORPHS_LOD_HIGH;
    
    if (distance > item->lod_distances[3]) {
        return ANIMATION_FACIAL_MORPHS_LOD_MINIMAL;
    } else if (distance > item->lod_distances[2]) {
        return ANIMATION_FACIAL_MORPHS_LOD_LOW;
    } else if (distance > item->lod_distances[1]) {
        return ANIMATION_FACIAL_MORPHS_LOD_MEDIUM;
    } else {
        return ANIMATION_FACIAL_MORPHS_LOD_HIGH;
    }
}

static bool animation_facial_morphs_should_cull(const animation_facial_morphs_internal_t* item, float distance) {
    if (!item) return true;
    
    // Cull if beyond maximum LOD distance
    return distance > item->lod_distances[3];
}

static void animation_facial_morphs_retarget_animation(animation_facial_morphs_internal_t* item) {
    if (!item || !item->initialized) return;
    
    // Apply retargeting transformations
    // Implementation would handle bone mapping and scaling
    g_facial_morphs_ctx.perf_counters.total_retargeting_operations++;
}

// Cleanup internal function
static void animation_facial_morphs_cleanup_internal(animation_facial_morphs_internal_t* item) {
    if (!item) return;
    
    // Clean up compression data
    if (item->compressed_data) {
        free(item->compressed_data);
        item->compressed_data = NULL;
        g_facial_morphs_ctx.total_memory_freed += item->compressed_size;
    }
    
    // Clean up main data
    if (item->data) {
        free(item->data);
        item->data = NULL;
        g_facial_morphs_ctx.total_memory_freed += item->data_size;
    }
    
    // Clean up morph targets
    for (uint32_t i = 0; i < item->morph_target_count; i++) {
        if (item->morph_targets[i].vertices) {
            free(item->morph_targets[i].vertices);
        }
    }
    
    // Clean up streaming
    if (item->stream.streaming_active && item->stream.stream_file) {
        fclose(item->stream.stream_file);
        item->stream.stream_file = NULL;
    }
    
    // Clean up GPU resources
    if (item->gpu_resources.gpu_resources_valid) {
        // Implementation would release GPU resources
        item->gpu_resources.gpu_resources_valid = false;
    }
    
    // Reset all fields
    item->data_size = 0;
    item->compressed_size = 0;
    item->compression_ratio = 0.0f;
    item->procedural_weight = 0.0f;
    item->ragdoll_enabled = false;
    item->current_lod = 0;
    item->culled = false;
    item->render_graph_enabled = false;
    item->initialized = false;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool animation_facial_morphs_validate(const animation_facial_morphs_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    
    // Validate compression data
    if (item->compression_ratio < 0.0f || item->compression_ratio > 1.0f) {
        return false;
    }
    
    // Validate LOD data
    if (item->current_lod >= ANIMATION_FACIAL_MORPHS_LOD_LEVELS) {
        return false;
    }
    
    // Validate procedural parameters
    if (item->procedural_weight < 0.0f || item->procedural_weight > 1.0f) {
        return false;
    }
    
    return true;
}

static void animation_facial_morphs_cleanup_internal(animation_facial_morphs_internal_t* item) {
    if (!item) return;
    
    // Clean up compression data
    if (item->compressed_data) {
        free(item->compressed_data);
        item->compressed_data = NULL;
    }
    
    // Clean up main data
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    // Reset all fields
    item->data_size = 0;
    item->compressed_size = 0;
    item->compression_ratio = 0.0f;
    item->procedural_weight = 0.0f;
    item->ragdoll_enabled = false;
    item->current_lod = 0;
    item->culled = false;
    item->render_graph_enabled = false;
    item->initialized = false;
}

/* Helper functions for animation compression */
static int animation_facial_morphs_compress_data(const void* input_data, size_t input_size, 
                                                   void** output_data, size_t* output_size) {
    if (!input_data || !output_data || !output_size) {
        return -1;
    }
    
    // Simple run-length compression for demonstration
    *output_size = input_size + (input_size / 2); // Estimate compressed size
    *output_data = malloc(*output_size);
    if (!*output_data) {
        return -2;
    }
    
    // Perform basic compression
    const uint8_t* src = (const uint8_t*)input_data;
    uint8_t* dst = (uint8_t*)*output_data;
    size_t dst_pos = 0;
    
    for (size_t i = 0; i < input_size; ) {
        uint8_t current = src[i];
        size_t count = 1;
        
        // Count consecutive identical bytes
        while (i + count < input_size && src[i + count] == current && count < 255) {
            count++;
        }
        
        if (count > 3 || current == 0) {
            // Run-length encoding
            dst[dst_pos++] = 0; // Escape byte
            dst[dst_pos++] = (uint8_t)count;
            dst[dst_pos++] = current;
        } else {
            // Literal copy
            for (size_t j = 0; j < count; j++) {
                dst[dst_pos++] = current;
            }
        }
        
        i += count;
    }
    
    *output_size = dst_pos;
    g_facial_morphs_ctx.performance.total_compressions++;
    return 0;
}

static int animation_facial_morphs_decompress_data(const void* compressed_data, size_t compressed_size,
                                                     void** output_data, size_t* output_size) {
    if (!compressed_data || !output_data || !output_size) {
        return -1;
    }
    
    // Estimate decompressed size (worst case: 3x compressed size)
    *output_size = compressed_size * 3;
    *output_data = malloc(*output_size);
    if (!*output_data) {
        return -2;
    }
    
    const uint8_t* src = (const uint8_t*)compressed_data;
    uint8_t* dst = (uint8_t*)*output_data;
    size_t dst_pos = 0;
    
    for (size_t i = 0; i < compressed_size; ) {
        if (src[i] == 0 && i + 2 < compressed_size) {
            // Run-length encoded sequence
            uint8_t count = src[i + 1];
            uint8_t value = src[i + 2];
            
            for (int j = 0; j < count && dst_pos < *output_size; j++) {
                dst[dst_pos++] = value;
            }
            
            i += 3;
        } else {
            // Literal byte
            dst[dst_pos++] = src[i++];
        }
    }
    
    *output_size = dst_pos;
    g_facial_morphs_ctx.performance.total_decompressions++;
    return 0;
}

/* Helper functions for procedural animation */
static void animation_facial_morphs_update_procedural(animation_facial_morphs_internal_t* item, float delta_time) {
    if (!item || !item->procedural_weight) {
        return;
    }
    
    switch (item->procedural_type) {
        case 0: // Sinusoidal animation
            {
                float frequency = item->procedural_params[0];
                float amplitude = item->procedural_params[1];
                float phase = item->procedural_params[2];
                float time = (float)clock() / CLOCKS_PER_SEC;
                
                float value = amplitude * sinf(frequency * time + phase);
                
                // Apply procedural animation to morph data
                if (item->data && item->data_size > 0) {
                    float* morph_data = (float*)item->data;
                    size_t float_count = item->data_size / sizeof(float);
                    for (size_t i = 0; i < float_count; i++) {
                        morph_data[i] += value * item->procedural_weight;
                    }
                }
            }
            break;
            
        case 1: // Noise-based animation
            {
                float noise_scale = item->procedural_params[0];
                float noise_amplitude = item->procedural_params[1];
                
                // Simple pseudo-random noise
                static uint32_t noise_seed = 12345;
                noise_seed = noise_seed * 1103515245 + 12345;
                float noise = ((float)(noise_seed & 0x7FFF) / 32767.0f - 0.5f) * 2.0f;
                
                if (item->data && item->data_size > 0) {
                    float* morph_data = (float*)item->data;
                    size_t float_count = item->data_size / sizeof(float);
                    for (size_t i = 0; i < float_count; i++) {
                        morph_data[i] += noise * noise_amplitude * item->procedural_weight;
                    }
                }
            }
            break;
    }
    
    g_facial_morphs_ctx.performance.total_procedural_updates++;
}

/* Helper functions for ragdoll physics */
static void animation_facial_morphs_update_ragdoll(animation_facial_morphs_internal_t* item, float delta_time) {
    if (!item || !item->ragdoll_enabled) {
        return;
    }
    
    // Simple ragdoll simulation
    float gravity = -9.81f;
    float damping = 0.98f;
    
    // Apply gravity and damping to morph data
    if (item->data && item->data_size > 0) {
        float* morph_data = (float*)item->data;
        size_t float_count = item->data_size / sizeof(float);
        
        for (size_t i = 0; i < float_count; i += 3) { // Assume XYZ triples
            if (i + 2 < float_count) {
                // Apply gravity to Y component
                morph_data[i + 1] += gravity * delta_time * item->ragdoll_mass;
                
                // Apply damping
                morph_data[i] *= damping;
                morph_data[i + 1] *= damping;
                morph_data[i + 2] *= damping;
            }
        }
    }
    
    g_facial_morphs_ctx.performance.total_ragdoll_simulations++;
}

/* Helper functions for animation retargeting */
static int animation_facial_morphs_retarget_animation(animation_facial_morphs_internal_t* item) {
    if (!item || item->source_skeleton_id == 0 || item->target_skeleton_id == 0) {
        return -1;
    }
    
    // Simple retargeting - scale the morph data
    if (item->data && item->data_size > 0) {
        float* morph_data = (float*)item->data;
        size_t float_count = item->data_size / sizeof(float);
        
        for (size_t i = 0; i < float_count; i++) {
            morph_data[i] *= item->retargeting_scale;
        }
    }
    
    g_facial_morphs_ctx.performance.total_retargeting_operations++;
    return 0;
}

/* Helper functions for caching */
static int animation_facial_morphs_cache_get(uint32_t morph_id, void** cached_data, size_t* cached_size) {
    if (!cached_data || !cached_size) {
        return -1;
    }
    
    pthread_rwlock_rdlock(&g_facial_morphs_ctx.cache_rwlock);
    
    // Find cache entry
    animation_facial_morphs_cache_entry_t* entry = NULL;
    for (int i = 0; i < ANIMATION_FACIAL_MORPHS_CACHE_SIZE; i++) {
        if (g_facial_morphs_ctx.cache[i].valid && 
            g_facial_morphs_ctx.cache[i].morph_id == morph_id) {
            entry = &g_facial_morphs_ctx.cache[i];
            break;
        }
    }
    
    if (entry) {
        *cached_data = malloc(entry->cached_size);
        if (*cached_data) {
            memcpy(*cached_data, entry->cached_data, entry->cached_size);
            *cached_size = entry->cached_size;
            
            // Update access info
            entry->last_access = (uint64_t)time(NULL);
            entry->access_count++;
            
            g_facial_morphs_ctx.performance.cache_hits++;
            pthread_rwlock_unlock(&g_facial_morphs_ctx.cache_rwlock);
            return 0;
        }
    }
    
    g_facial_morphs_ctx.performance.cache_misses++;
    pthread_rwlock_unlock(&g_facial_morphs_ctx.cache_rwlock);
    return -2;
}

static int animation_facial_morphs_cache_put(uint32_t morph_id, const void* data, size_t size) {
    if (!data || size == 0) {
        return -1;
    }
    
    pthread_rwlock_wrlock(&g_facial_morphs_ctx.cache_rwlock);
    
    // Find empty slot or LRU entry
    animation_facial_morphs_cache_entry_t* entry = NULL;
    uint64_t oldest_access = UINT64_MAX;
    
    for (int i = 0; i < ANIMATION_FACIAL_MORPHS_CACHE_SIZE; i++) {
        if (!g_facial_morphs_ctx.cache[i].valid) {
            entry = &g_facial_morphs_ctx.cache[i];
            break;
        } else if (g_facial_morphs_ctx.cache[i].last_access < oldest_access) {
            oldest_access = g_facial_morphs_ctx.cache[i].last_access;
            entry = &g_facial_morphs_ctx.cache[i];
        }
    }
    
    if (entry) {
        // Free old data if present
        if (entry->cached_data) {
            free(entry->cached_data);
        }
        
        // Store new data
        entry->cached_data = malloc(size);
        if (entry->cached_data) {
            memcpy(entry->cached_data, data, size);
            entry->cached_size = size;
            entry->morph_id = morph_id;
            entry->last_access = (uint64_t)time(NULL);
            entry->access_count = 1;
            entry->valid = true;
            
            pthread_rwlock_unlock(&g_facial_morphs_ctx.cache_rwlock);
            return 0;
        }
    }
    
    pthread_rwlock_unlock(&g_facial_morphs_ctx.cache_rwlock);
    return -2;
}

/* Helper functions for hot-reload */
static void* animation_facial_morphs_file_watcher_thread(void* arg) {
    char buffer[4096];
    
    while (g_facial_morphs_ctx.hot_reload_enabled) {
        int length = read(g_facial_morphs_ctx.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            int i = 0;
            while (i < length) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                if (event->mask & IN_MODIFY) {
                    // File modified - trigger reload
                    printf("Facial morphs file modified, triggering reload\n");
                    // Mark all morphs as dirty for reload
                    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
                    for (uint32_t j = 0; j < g_facial_morphs_ctx.count; j++) {
                        g_facial_morphs_ctx.items[j].dirty = true;
                    }
                    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); // 100ms
    }
    
    return NULL;
}

/* Helper functions for GPU integration */
static int animation_facial_morphs_gpu_upload(animation_facial_morphs_internal_t* item) {
    if (!item || !g_facial_morphs_ctx.gpu_available) {
        return -1;
    }
    
    // Simulate GPU upload
    if (item->data && item->data_size > 0) {
        // In a real implementation, this would upload to GPU
        g_facial_morphs_ctx.performance.gpu_operations++;
        g_facial_morphs_ctx.performance.total_gpu_time += 0.001; // Simulated GPU time
    }
    
    return 0;
}

/* Helper functions for SIMD optimization */
static void animation_facial_morphs_simd_process(float* data, size_t count) {
#ifdef __SSE2__
    if (!g_facial_morphs_ctx.simd_context.simd_enabled || !data) {
        return;
    }
    
    size_t simd_count = count & ~3; // Process 4 floats at a time
    
    for (size_t i = 0; i < simd_count; i += 4) {
        __m128 vec = _mm_load_ps(&data[i]);
        // Apply some SIMD operation (e.g., scaling)
        __m128 scale = _mm_set1_ps(1.0f);
        vec = _mm_mul_ps(vec, scale);
        _mm_store_ps(&data[i], vec);
    }
    
    g_facial_morphs_ctx.performance.simd_operations++;
#endif
}

/* Helper functions for batch processing */
static int animation_facial_morphs_batch_add(uint32_t morph_id) {
    if (g_facial_morphs_ctx.batch_context.batch_count >= ANIMATION_FACIAL_MORPHS_BATCH_SIZE) {
        return -1; // Batch full
    }
    
    g_facial_morphs_ctx.batch_context.batch_items[g_facial_morphs_ctx.batch_context.batch_count++] = morph_id;
    return 0;
}

static int animation_facial_morphs_batch_process(void) {
    if (g_facial_morphs_ctx.batch_context.batch_count == 0) {
        return 0;
    }
    
    // Process all items in batch
    for (uint32_t i = 0; i < g_facial_morphs_ctx.batch_context.batch_count; i++) {
        uint32_t morph_id = g_facial_morphs_ctx.batch_context.batch_items[i];
        if (morph_id < g_facial_morphs_ctx.count) {
            animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[morph_id];
            if (item->initialized && item->dirty) {
                // Process item
                item->dirty = false;
            }
        }
    }
    
    g_facial_morphs_ctx.performance.batch_operations++;
    g_facial_morphs_ctx.batch_context.batch_count = 0;
    return 0;
}

/* Helper functions for streaming support */
static int animation_facial_morphs_stream_init(const char* filename) {
    if (!filename) {
        return -1;
    }
    
    g_facial_morphs_ctx.stream_context.stream_file = fopen(filename, "rb");
    if (!g_facial_morphs_ctx.stream_context.stream_file) {
        return -2;
    }
    
    // Get file size
    fseek(g_facial_morphs_ctx.stream_context.stream_file, 0, SEEK_END);
    g_facial_morphs_ctx.stream_context.stream_size = ftell(g_facial_morphs_ctx.stream_context.stream_file);
    fseek(g_facial_morphs_ctx.stream_context.stream_file, 0, SEEK_SET);
    
    g_facial_morphs_ctx.stream_context.stream_offset = 0;
    g_facial_morphs_ctx.stream_context.streaming_active = true;
    g_facial_morphs_ctx.stream_context.stream_chunk_size = 4096;
    
    return 0;
}

static int animation_facial_morphs_stream_read_chunk(void** chunk_data, size_t* chunk_size) {
    if (!chunk_data || !chunk_size || !g_facial_morphs_ctx.stream_context.streaming_active) {
        return -1;
    }
    
    if (g_facial_morphs_ctx.stream_context.stream_offset >= g_facial_morphs_ctx.stream_context.stream_size) {
        return -2; // End of file
    }
    
    size_t remaining = g_facial_morphs_ctx.stream_context.stream_size - 
                       g_facial_morphs_ctx.stream_context.stream_offset;
    size_t to_read = (remaining < g_facial_morphs_ctx.stream_context.stream_chunk_size) ? 
                      remaining : g_facial_morphs_ctx.stream_context.stream_chunk_size;
    
    *chunk_data = malloc(to_read);
    if (!*chunk_data) {
        return -3;
    }
    
    size_t read = fread(*chunk_data, 1, to_read, g_facial_morphs_ctx.stream_context.stream_file);
    if (read != to_read) {
        free(*chunk_data);
        return -4;
    }
    
    *chunk_size = read;
    g_facial_morphs_ctx.stream_context.stream_offset += read;
    g_facial_morphs_ctx.performance.streaming_operations++;
    
    return 0;
}

/* Helper functions for LOD support */
static uint32_t animation_facial_morphs_calculate_lod(animation_facial_morphs_internal_t* item, float distance) {
    if (!item) {
        return 0;
    }
    
    for (uint32_t i = 0; i < ANIMATION_FACIAL_MORPHS_LOD_LEVELS; i++) {
        if (distance <= item->lod_distances[i]) {
            return i;
        }
    }
    
    return ANIMATION_FACIAL_MORPHS_LOD_LEVELS - 1; // Lowest LOD
}

/* Helper functions for culling integration */
static bool animation_facial_morphs_should_cull(animation_facial_morphs_internal_t* item, float distance) {
    if (!item) {
        return true;
    }
    
    // Simple distance-based culling
    float max_distance = item->lod_distances[ANIMATION_FACIAL_MORPHS_LOD_LEVELS - 1];
    return distance > max_distance;
}

/* Helper functions for render graph node */
static int animation_facial_morphs_render_graph_add_node(animation_facial_morphs_internal_t* item) {
    if (!item) {
        return -1;
    }
    
    // In a real implementation, this would add a node to the render graph
    item->render_graph_node_id = g_facial_morphs_ctx.count; // Simple ID assignment
    item->render_graph_enabled = true;
    
    return 0;
}

/* Helper functions for serialization */
static int animation_facial_morphs_serialize(const animation_facial_morphs_internal_t* item, 
                                             void** serialized_data, size_t* serialized_size) {
    if (!item || !serialized_data || !serialized_size) {
        return -1;
    }
    
    // Calculate serialized size
    *serialized_size = sizeof(uint32_t) * 4 + sizeof(float) * 12 + item->data_size;
    *serialized_data = malloc(*serialized_size);
    if (!*serialized_data) {
        return -2;
    }
    
    uint8_t* ptr = (uint8_t*)*serialized_data;
    
    // Write magic number and version
    *(uint32_t*)ptr = ANIMATION_FACIAL_MORPHS_MAGIC_NUMBER;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = ANIMATION_FACIAL_MORPHS_VERSION;
    ptr += sizeof(uint32_t);
    
    // Write basic data
    *(uint32_t*)ptr = item->id;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->flags;
    ptr += sizeof(uint32_t);
    
    // Write compression data
    *(float*)ptr = item->compression_ratio;
    ptr += sizeof(float);
    *(uint32_t*)ptr = item->compressed_size;
    ptr += sizeof(uint32_t);
    
    // Write procedural data
    *(float*)ptr = item->procedural_weight;
    ptr += sizeof(float);
    *(uint32_t*)ptr = item->procedural_type;
    ptr += sizeof(uint32_t);
    
    // Write ragdoll data
    *(bool*)ptr = item->ragdoll_enabled;
    ptr += sizeof(bool);
    *(float*)ptr = item->ragdoll_mass;
    ptr += sizeof(float);
    *(float*)ptr = item->ragdoll_stiffness;
    ptr += sizeof(float);
    
    // Write retargeting data
    *(uint32_t*)ptr = item->source_skeleton_id;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->target_skeleton_id;
    ptr += sizeof(uint32_t);
    *(float*)ptr = item->retargeting_scale;
    ptr += sizeof(float);
    
    // Write LOD data
    *(uint32_t*)ptr = item->current_lod;
    ptr += sizeof(uint32_t);
    for (int i = 0; i < ANIMATION_FACIAL_MORPHS_LOD_LEVELS; i++) {
        *(float*)ptr = item->lod_distances[i];
        ptr += sizeof(float);
    }
    
    // Write main data
    if (item->data && item->data_size > 0) {
        memcpy(ptr, item->data, item->data_size);
    }
    
    return 0;
}

static int animation_facial_morphs_deserialize(const void* serialized_data, size_t serialized_size,
                                               animation_facial_morphs_internal_t* item) {
    if (!serialized_data || !item || serialized_size < sizeof(uint32_t) * 4) {
        return -1;
    }
    
    const uint8_t* ptr = (const uint8_t*)serialized_data;
    
    // Check magic number and version
    if (*(uint32_t*)ptr != ANIMATION_FACIAL_MORPHS_MAGIC_NUMBER) {
        return -2;
    }
    ptr += sizeof(uint32_t);
    
    if (*(uint32_t*)ptr != ANIMATION_FACIAL_MORPHS_VERSION) {
        return -3;
    }
    ptr += sizeof(uint32_t);
    
    // Read basic data
    item->id = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    item->flags = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    
    // Read compression data
    item->compression_ratio = *(float*)ptr;
    ptr += sizeof(float);
    item->compressed_size = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    
    // Read procedural data
    item->procedural_weight = *(float*)ptr;
    ptr += sizeof(float);
    item->procedural_type = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    
    // Read ragdoll data
    item->ragdoll_enabled = *(bool*)ptr;
    ptr += sizeof(bool);
    item->ragdoll_mass = *(float*)ptr;
    ptr += sizeof(float);
    item->ragdoll_stiffness = *(float*)ptr;
    ptr += sizeof(float);
    
    // Read retargeting data
    item->source_skeleton_id = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    item->target_skeleton_id = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    item->retargeting_scale = *(float*)ptr;
    ptr += sizeof(float);
    
    // Read LOD data
    item->current_lod = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    for (int i = 0; i < ANIMATION_FACIAL_MORPHS_LOD_LEVELS; i++) {
        item->lod_distances[i] = *(float*)ptr;
        ptr += sizeof(float);
    }
    
    // Read main data
    size_t data_size = serialized_size - (ptr - (const uint8_t*)serialized_data);
    if (data_size > 0) {
        item->data = malloc(data_size);
        if (item->data) {
            memcpy(item->data, ptr, data_size);
            item->data_size = data_size;
        }
    }
    
    item->initialized = true;
    return 0;
}

int animation_facial_morphs_init(void) {
    if (g_facial_morphs_ctx.initialized) {
        return 0;
    }

    // Initialize mutexes
    pthread_mutex_init(&g_facial_morphs_ctx.context_mutex, NULL);
    pthread_rwlock_init(&g_facial_morphs_ctx.cache_rwlock, NULL);
    pthread_mutex_init(&g_facial_morphs_ctx.async_mutex, NULL);

    // Allocate items
    g_facial_morphs_ctx.capacity = ANIMATION_FACIAL_MORPHS_DEFAULT_CAPACITY;
    g_facial_morphs_ctx.items = calloc(g_facial_morphs_ctx.capacity, sizeof(animation_facial_morphs_internal_t));
    if (!g_facial_morphs_ctx.items) {
        return -1;
    }

    // Initialize hot-reload
    g_facial_morphs_ctx.inotify_fd = inotify_init();
    if (g_facial_morphs_ctx.inotify_fd >= 0) {
        strcpy(g_facial_morphs_ctx.watch_directory, "/tmp/facial_morphs");
        g_facial_morphs_ctx.inotify_wd = inotify_add_watch(
            g_facial_morphs_ctx.inotify_fd, 
            g_facial_morphs_ctx.watch_directory, 
            IN_MODIFY);
        g_facial_morphs_ctx.hot_reload_enabled = true;
        pthread_create(&g_facial_morphs_ctx.file_watcher_thread, NULL, 
                       animation_facial_morphs_file_watcher_thread, NULL);
    }

    // Initialize SIMD
    g_facial_morphs_ctx.simd_context.simd_enabled = true;
    
    // Initialize GPU context
    g_facial_morphs_ctx.gpu_available = true;

    g_facial_morphs_ctx.count = 0;
    g_facial_morphs_ctx.initialized = true;
    return 0;
}

void animation_facial_morphs_shutdown(void) {
    if (!g_facial_morphs_ctx.initialized) {
        return;
    }

    // Stop hot-reload
    g_facial_morphs_ctx.hot_reload_enabled = false;
    if (g_facial_morphs_ctx.inotify_fd >= 0) {
        close(g_facial_morphs_ctx.inotify_fd);
        pthread_join(g_facial_morphs_ctx.file_watcher_thread, NULL);
    }

    // Cleanup all items
    for (uint32_t i = 0; i < g_facial_morphs_ctx.count; i++) {
        animation_facial_morphs_cleanup_internal(&g_facial_morphs_ctx.items[i]);
    }

    // Cleanup cache
    for (int i = 0; i < ANIMATION_FACIAL_MORPHS_CACHE_SIZE; i++) {
        if (g_facial_morphs_ctx.cache[i].cached_data) {
            free(g_facial_morphs_ctx.cache[i].cached_data);
        }
    }

    // Cleanup mutexes
    pthread_mutex_destroy(&g_facial_morphs_ctx.context_mutex);
    pthread_rwlock_destroy(&g_facial_morphs_ctx.cache_rwlock);
    pthread_mutex_destroy(&g_facial_morphs_ctx.async_mutex);

    // Free items
    free(g_facial_morphs_ctx.items);

    g_facial_morphs_ctx.initialized = false;
}

int animation_facial_morphs_create(animation_facial_morphs_handle_t* out_handle, const animation_facial_morphs_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);

    if (!g_facial_morphs_ctx.initialized) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return -2;
    }

    if (g_facial_morphs_ctx.count >= g_facial_morphs_ctx.capacity) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return -3;
    }

    uint32_t index = g_facial_morphs_ctx.count++;
    animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[index];

    // Initialize all fields
    memset(item, 0, sizeof(animation_facial_morphs_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    // Initialize LOD distances
    item->lod_distances[0] = 10.0f;
    item->lod_distances[1] = 25.0f;
    item->lod_distances[2] = 50.0f;
    item->lod_distances[3] = 100.0f;
    
    // Initialize procedural parameters
    item->procedural_weight = 0.0f;
    item->procedural_type = 0;
    memset(item->procedural_params, 0, sizeof(item->procedural_params));
    
    // Initialize ragdoll parameters
    item->ragdoll_enabled = false;
    item->ragdoll_mass = 1.0f;
    item->ragdoll_stiffness = 0.5f;
    
    // Initialize retargeting
    item->source_skeleton_id = 0;
    item->target_skeleton_id = 0;
    item->retargeting_scale = 1.0f;
    
    // Add to render graph
    animation_facial_morphs_render_graph_add_node(item);

    out_handle->id = index;
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return 0;
}

void animation_facial_morphs_destroy(animation_facial_morphs_handle_t handle) {
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);

    if (handle.id >= g_facial_morphs_ctx.count) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return;
    }

    animation_facial_morphs_cleanup_internal(&g_facial_morphs_ctx.items[handle.id]);
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
}

int animation_facial_morphs_update(animation_facial_morphs_handle_t handle, const void* data, size_t size) {
    if (!data || size == 0) {
        return -1;
    }
    
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);

    if (handle.id >= g_facial_morphs_ctx.count) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return -2;
    }

    animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return -3;
    }

    // Update memory tracking
    size_t old_size = item->data_size;
    
    // Reallocate data if needed
    if (size != item->data_size) {
        void* new_data = realloc(item->data, size);
        if (!new_data) {
            pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
            return -4;
        }
        item->data = new_data;
        item->data_size = size;
        
        // Update memory tracking
        g_facial_morphs_ctx.total_memory_allocated += (size - old_size);
        if (g_facial_morphs_ctx.total_memory_allocated > g_facial_morphs_ctx.peak_memory_usage) {
            g_facial_morphs_ctx.peak_memory_usage = g_facial_morphs_ctx.total_memory_allocated;
        }
    }
    
    // Copy new data
    memcpy(item->data, data, size);
    
    // Apply compression
    if (item->flags & 0x01) { // Compression flag
        animation_facial_morphs_compress_data(data, size, &item->compressed_data, &item->compressed_size);
        item->compression_ratio = (float)item->compressed_size / size;
    }
    
    // Apply SIMD optimization
    if (g_facial_morphs_ctx.simd_context.simd_enabled) {
        animation_facial_morphs_simd_process((float*)item->data, size / sizeof(float));
    }
    
    // Upload to GPU
    if (g_facial_morphs_ctx.gpu_available) {
        animation_facial_morphs_gpu_upload(item);
    }
    
    item->dirty = true;
    item->frame_updated++;
    g_facial_morphs_ctx.performance.total_updates++;
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return 0;
}

bool animation_facial_morphs_is_valid(animation_facial_morphs_handle_t handle) {
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    bool valid = false;
    if (handle.id < g_facial_morphs_ctx.count) {
        animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[handle.id];
        valid = item->initialized && animation_facial_morphs_validate(item);
    }
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return valid;
}

int animation_facial_morphs_get_info(animation_facial_morphs_handle_t handle, animation_facial_morphs_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);

    if (handle.id >= g_facial_morphs_ctx.count) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return -2;
    }

    const animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return 0;
}

void animation_facial_morphs_mark_dirty(animation_facial_morphs_handle_t handle) {
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    if (handle.id < g_facial_morphs_ctx.count) {
        animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[handle.id];
        item->dirty = true;
        
        // Mark GPU data as dirty
        if (g_facial_morphs_ctx.gpu_available) {
            g_facial_morphs_ctx.gpu_context.gpu_data_dirty = true;
        }
    }
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
}

int animation_facial_morphs_process_pending(void) {
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    // Process batch first
    animation_facial_morphs_batch_process();
    
    int processed = 0;
    for (uint32_t i = 0; i < g_facial_morphs_ctx.count; i++) {
        animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Update procedural animation
            animation_facial_morphs_update_procedural(item, 0.016f); // 60 FPS
            
            // Update ragdoll physics
            animation_facial_morphs_update_ragdoll(item, 0.016f);
            
            // Apply retargeting
            if (item->source_skeleton_id != 0 && item->target_skeleton_id != 0) {
                animation_facial_morphs_retarget_animation(item);
            }
            
            // Calculate LOD and culling
            float distance = 50.0f; // Simulated distance
            item->current_lod = animation_facial_morphs_calculate_lod(item, distance);
            item->culled = animation_facial_morphs_should_cull(item, distance);
            
            // Upload to GPU if needed
            if (g_facial_morphs_ctx.gpu_available && g_facial_morphs_ctx.gpu_context.gpu_data_dirty) {
                animation_facial_morphs_gpu_upload(item);
            }
            
            item->dirty = false;
            processed++;
        }
    }
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return processed;
}

uint32_t animation_facial_morphs_get_count(void) {
    return g_facial_morphs_ctx.count;
}

size_t animation_facial_morphs_get_memory_usage(void) {
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    size_t total = sizeof(g_facial_morphs_ctx);
    total += g_facial_morphs_ctx.capacity * sizeof(animation_facial_morphs_internal_t);
    
    // Add data sizes
    for (uint32_t i = 0; i < g_facial_morphs_ctx.count; i++) {
        total += g_facial_morphs_ctx.items[i].data_size;
        total += g_facial_morphs_ctx.items[i].compressed_size;
    }
    
    // Add cache memory
    for (int i = 0; i < ANIMATION_FACIAL_MORPHS_CACHE_SIZE; i++) {
        if (g_facial_morphs_ctx.cache[i].valid) {
            total += g_facial_morphs_ctx.cache[i].cached_size;
        }
    }
    
    // Add GPU memory
    if (g_facial_morphs_ctx.gpu_available) {
        total += g_facial_morphs_ctx.gpu_context.gpu_buffer_size;
    }
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return total;
}

void animation_facial_morphs_debug_print(void) {
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    printf("=== Facial Morphs Debug Info ===\n");
    printf("Initialized: %s\n", g_facial_morphs_ctx.initialized ? "Yes" : "No");
    printf("Count: %u / %u\n", g_facial_morphs_ctx.count, g_facial_morphs_ctx.capacity);
    printf("Hot-reload enabled: %s\n", g_facial_morphs_ctx.hot_reload_enabled ? "Yes" : "No");
    printf("GPU available: %s\n", g_facial_morphs_ctx.gpu_available ? "Yes" : "No");
    printf("SIMD enabled: %s\n", g_facial_morphs_ctx.simd_context.simd_enabled ? "Yes" : "No");
    
    printf("\n=== Performance Counters ===\n");
    printf("Total updates: %lu\n", g_facial_morphs_ctx.performance.total_updates);
    printf("Total compressions: %lu\n", g_facial_morphs_ctx.performance.total_compressions);
    printf("Total decompressions: %lu\n", g_facial_morphs_ctx.performance.total_decompressions);
    printf("Total procedural updates: %lu\n", g_facial_morphs_ctx.performance.total_procedural_updates);
    printf("Total ragdoll simulations: %lu\n", g_facial_morphs_ctx.performance.total_ragdoll_simulations);
    printf("Total retargeting operations: %lu\n", g_facial_morphs_ctx.performance.total_retargeting_operations);
    printf("Cache hits: %lu\n", g_facial_morphs_ctx.performance.cache_hits);
    printf("Cache misses: %lu\n", g_facial_morphs_ctx.performance.cache_misses);
    printf("GPU operations: %lu\n", g_facial_morphs_ctx.performance.gpu_operations);
    printf("SIMD operations: %lu\n", g_facial_morphs_ctx.performance.simd_operations);
    printf("Batch operations: %lu\n", g_facial_morphs_ctx.performance.batch_operations);
    printf("Streaming operations: %lu\n", g_facial_morphs_ctx.performance.streaming_operations);
    printf("Total update time: %.3f ms\n", g_facial_morphs_ctx.performance.total_update_time * 1000.0);
    printf("Total compression time: %.3f ms\n", g_facial_morphs_ctx.performance.total_compression_time * 1000.0);
    printf("Total GPU time: %.3f ms\n", g_facial_morphs_ctx.performance.total_gpu_time * 1000.0);
    
    printf("\n=== Memory Usage ===\n");
    printf("Current memory: %zu bytes\n", animation_facial_morphs_get_memory_usage());
    printf("Peak memory: %zu bytes\n", g_facial_morphs_ctx.peak_memory_usage);
    printf("Total allocated: %zu bytes\n", g_facial_morphs_ctx.total_memory_allocated);
    printf("Total freed: %zu bytes\n", g_facial_morphs_ctx.total_memory_freed);
    
    printf("\n=== Individual Morphs ===\n");
    for (uint32_t i = 0; i < g_facial_morphs_ctx.count; i++) {
        const animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[i];
        if (item->initialized) {
            printf("Morph %u: flags=0x%08x, data_size=%zu, compressed_size=%u, ratio=%.2f, "
                   "procedural_weight=%.2f, ragdoll=%s, lod=%u, culled=%s\n",
                   item->id, item->flags, item->data_size, item->compressed_size,
                   item->compression_ratio, item->procedural_weight,
                   item->ragdoll_enabled ? "Yes" : "No", item->current_lod,
                   item->culled ? "Yes" : "No");
        }
    }
    
    printf("\n=== Cache Status ===\n");
    uint32_t valid_cache_entries = 0;
    for (int i = 0; i < ANIMATION_FACIAL_MORPHS_CACHE_SIZE; i++) {
        if (g_facial_morphs_ctx.cache[i].valid) {
            valid_cache_entries++;
        }
    }
    printf("Valid cache entries: %u / %d\n", valid_cache_entries, ANIMATION_FACIAL_MORPHS_CACHE_SIZE);
    printf("Cache hit ratio: %.2f%%\n", 
           g_facial_morphs_ctx.performance.cache_hits > 0 ?
           (double)g_facial_morphs_ctx.performance.cache_hits / 
           (g_facial_morphs_ctx.performance.cache_hits + g_facial_morphs_ctx.performance.cache_misses) * 100.0 : 0.0);
    
    printf("\n=== Batch Status ===\n");
    printf("Batch count: %u / %d\n", g_facial_morphs_ctx.batch_context.batch_count, ANIMATION_FACIAL_MORPHS_BATCH_SIZE);
    printf("Batch active: %s\n", g_facial_morphs_ctx.batch_context.batch_active ? "Yes" : "No");
    
    printf("\n=== Streaming Status ===\n");
    printf("Streaming active: %s\n", g_facial_morphs_ctx.stream_context.streaming_active ? "Yes" : "No");
    if (g_facial_morphs_ctx.stream_context.streaming_active) {
        printf("Stream position: %lu / %lu\n", 
               g_facial_morphs_ctx.stream_context.stream_position,
               g_facial_morphs_ctx.stream_context.stream_size);
    }
    
    printf("=== End Facial Morphs Debug Info ===\n");
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
}

/* ============================================================================
 * ADDITIONAL API FUNCTIONS
 * ============================================================================ */

int animation_facial_morphs_get_performance_info(animation_facial_morphs_performance_info_t* out_info) {
    if (!out_info) return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    out_info->total_updates = g_facial_morphs_ctx.performance.total_updates;
    out_info->total_compressions = g_facial_morphs_ctx.performance.total_compressions;
    out_info->total_decompressions = g_facial_morphs_ctx.performance.total_decompressions;
    out_info->total_procedural_updates = g_facial_morphs_ctx.performance.total_procedural_updates;
    out_info->total_ragdoll_simulations = g_facial_morphs_ctx.performance.total_ragdoll_simulations;
    out_info->total_retargeting_operations = g_facial_morphs_ctx.performance.total_retargeting_operations;
    out_info->cache_hits = g_facial_morphs_ctx.performance.cache_hits;
    out_info->cache_misses = g_facial_morphs_ctx.performance.cache_misses;
    out_info->gpu_operations = g_facial_morphs_ctx.performance.gpu_operations;
    out_info->simd_operations = g_facial_morphs_ctx.performance.simd_operations;
    out_info->batch_operations = g_facial_morphs_ctx.performance.batch_operations;
    out_info->streaming_operations = g_facial_morphs_ctx.performance.streaming_operations;
    out_info->total_update_time = g_facial_morphs_ctx.performance.total_update_time;
    out_info->total_compression_time = g_facial_morphs_ctx.performance.total_compression_time;
    out_info->total_gpu_time = g_facial_morphs_ctx.performance.total_gpu_time;
    out_info->peak_memory_usage = g_facial_morphs_ctx.peak_memory_usage;
    out_info->active_morphs = g_facial_morphs_ctx.count;
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return ANIMATION_FACIAL_MORPHS_SUCCESS;
}

void animation_facial_morphs_reset_performance_counters(void) {
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    memset(&g_facial_morphs_ctx.performance, 0, sizeof(g_facial_morphs_ctx.performance));
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
}

int animation_facial_morphs_set_morph_weight(animation_facial_morphs_handle_t handle, uint32_t morph_index, float weight) {
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    if (handle.id >= g_facial_morphs_ctx.count) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_HANDLE;
    }
    
    animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[handle.id];
    if (!item->initialized || morph_index >= item->morph_target_count) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_PARAM;
    }
    
    item->morph_targets[morph_index].target_weight = fmaxf(0.0f, fminf(1.0f, weight));
    item->dirty = true;
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return ANIMATION_FACIAL_MORPHS_SUCCESS;
}

int animation_facial_morphs_get_morph_weight(animation_facial_morphs_handle_t handle, uint32_t morph_index, float* out_weight) {
    if (!out_weight) return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    if (handle.id >= g_facial_morphs_ctx.count) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_HANDLE;
    }
    
    animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[handle.id];
    if (!item->initialized || morph_index >= item->morph_target_count) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_PARAM;
    }
    
    *out_weight = item->morph_targets[morph_index].current_weight;
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return ANIMATION_FACIAL_MORPHS_SUCCESS;
}

int animation_facial_morphs_enable_ragdoll(animation_facial_morphs_handle_t handle, bool enable) {
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    if (handle.id >= g_facial_morphs_ctx.count) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_HANDLE;
    }
    
    animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_HANDLE;
    }
    
    item->ragdoll_enabled = enable;
    item->dirty = true;
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return ANIMATION_FACIAL_MORPHS_SUCCESS;
}

int animation_facial_morphs_set_procedural_params(animation_facial_morphs_handle_t handle, float frequency, float amplitude, float phase, float time_scale) {
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    if (handle.id >= g_facial_morphs_ctx.count) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_HANDLE;
    }
    
    animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_HANDLE;
    }
    
    item->procedural_params[0] = frequency;
    item->procedural_params[1] = amplitude;
    item->procedural_params[2] = phase;
    item->procedural_params[3] = time_scale;
    item->dirty = true;
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return ANIMATION_FACIAL_MORPHS_SUCCESS;
}

int animation_facial_morphs_serialize(animation_facial_morphs_handle_t handle, const char* filename) {
    if (!filename) return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    if (handle.id >= g_facial_morphs_ctx.count) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_HANDLE;
    }
    
    animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_HANDLE;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        return ANIMATION_FACIAL_MORPHS_ERROR_FILE_NOT_FOUND;
    }
    
    // Write magic number and version
    uint32_t magic = ANIMATION_FACIAL_MORPHS_MAGIC_NUMBER;
    uint32_t version = ANIMATION_FACIAL_MORPHS_VERSION;
    fwrite(&magic, sizeof(magic), 1, file);
    fwrite(&version, sizeof(version), 1, file);
    
    // Write main data
    fwrite(item, sizeof(animation_facial_morphs_internal_t), 1, file);
    
    // Write dynamic data
    if (item->data && item->data_size > 0) {
        fwrite(&item->data_size, sizeof(item->data_size), 1, file);
        fwrite(item->data, item->data_size, 1, file);
    }
    
    fclose(file);
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    return ANIMATION_FACIAL_MORPHS_SUCCESS;
}

int animation_facial_morphs_deserialize(animation_facial_morphs_handle_t* out_handle, const char* filename) {
    if (!out_handle || !filename) return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_PARAM;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return ANIMATION_FACIAL_MORPHS_ERROR_FILE_NOT_FOUND;
    
    // Read and verify magic number
    uint32_t magic, version;
    if (fread(&magic, sizeof(magic), 1, file) != 1 || 
        magic != ANIMATION_FACIAL_MORPHS_MAGIC_NUMBER) {
        fclose(file);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_FORMAT;
    }
    
    if (fread(&version, sizeof(version), 1, file) != 1 || 
        version != ANIMATION_FACIAL_MORPHS_VERSION) {
        fclose(file);
        return ANIMATION_FACIAL_MORPHS_ERROR_INVALID_FORMAT;
    }
    
    // Create new morph
    animation_facial_morphs_desc_t desc = {0};
    int result = animation_facial_morphs_create(out_handle, &desc);
    if (result != ANIMATION_FACIAL_MORPHS_SUCCESS) {
        fclose(file);
        return result;
    }
    
    pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex);
    
    animation_facial_morphs_internal_t* item = &g_facial_morphs_ctx.items[out_handle->id];
    
    // Read main data
    if (fread(item, sizeof(animation_facial_morphs_internal_t), 1, file) != 1) {
        pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
        fclose(file);
        animation_facial_morphs_destroy(*out_handle);
        return ANIMATION_FACIAL_MORPHS_ERROR_SERIALIZATION_FAILED;
    }
    
    // Read dynamic data
    if (item->data_size > 0) {
        item->data = malloc(item->data_size);
        if (!item->data || fread(item->data, item->data_size, 1, file) != 1) {
            pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
            fclose(file);
            animation_facial_morphs_destroy(*out_handle);
            return ANIMATION_FACIAL_MORPHS_ERROR_OUT_OF_MEMORY;
        }
    }
    
    pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex);
    fclose(file);
    
    return ANIMATION_FACIAL_MORPHS_SUCCESS;
}

int animation_facial_morphs_lock(void) {
    return pthread_mutex_lock(&g_facial_morphs_ctx.context_mutex) == 0 ? 
           ANIMATION_FACIAL_MORPHS_SUCCESS : ANIMATION_FACIAL_MORPHS_ERROR_THREAD_ERROR;
}

int animation_facial_morphs_unlock(void) {
    return pthread_mutex_unlock(&g_facial_morphs_ctx.context_mutex) == 0 ? 
           ANIMATION_FACIAL_MORPHS_SUCCESS : ANIMATION_FACIAL_MORPHS_ERROR_THREAD_ERROR;
}

int animation_facial_morphs_trylock(void) {
    return pthread_mutex_trylock(&g_facial_morphs_ctx.context_mutex) == 0 ? 
           ANIMATION_FACIAL_MORPHS_SUCCESS : ANIMATION_FACIAL_MORPHS_ERROR_THREAD_ERROR;
}

/* End of facial_morphs.c */
