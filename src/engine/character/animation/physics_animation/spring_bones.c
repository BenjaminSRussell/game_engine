/*
 * spring_bones.c
 * Spring constraint bones
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
 * TODO: Implement spring bones initialization
 * TODO: Add spring bones cleanup/shutdown
 * TODO: Implement spring bones validation
 * TODO: Add spring bones error handling
 * TODO: Implement spring bones serialization
 * TODO: Add spring bones debug output
 * TODO: Implement spring bones unit tests
 * TODO: Add spring bones performance counters
 * TODO: Implement spring bones hot-reload
 * TODO: Add spring bones thread safety
 * TODO: Implement spring bones memory pooling
 * TODO: Add spring bones caching layer
 * TODO: Implement spring bones async operations
 * TODO: Add spring bones GPU integration
 * TODO: Implement spring bones SIMD optimization
 * TODO: Add spring bones batch processing
 * TODO: Implement spring bones streaming support
 * TODO: Add spring bones LOD support
 * TODO: Implement spring bones culling integration
 * TODO: Add spring bones render graph node
 */

#include "character/animation/physics_animation/spring_bones.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>
#include <immintrin.h>
#include <lz4.h>
#include <zstd.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_SPRING_BONES_MAX_COUNT 4096
#define ANIMATION_SPRING_BONES_DEFAULT_CAPACITY 256
#define ANIMATION_SPRING_BONES_ALIGNMENT 16
#define ANIMATION_SPRING_BONES_CACHE_SIZE 1024
#define ANIMATION_SPRING_BONES_MAX_ASYNC_OPERATIONS 64
#define ANIMATION_SPRING_BONES_MEMORY_BUDGET (512 * 1024 * 1024) /* 512MB */
#define ANIMATION_SPRING_BONES_MAGIC_NUMBER 0x5350424E /* "SPBN" */
#define ANIMATION_SPRING_BONES_VERSION 1

/* Error codes */
typedef enum {
    ANIMATION_SPRING_BONES_ERROR_NONE = 0,
    ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM = -1,
    ANIMATION_SPRING_BONES_ERROR_NOT_INITIALIZED = -2,
    ANIMATION_SPRING_BONES_ERROR_OUT_OF_MEMORY = -3,
    ANIMATION_SPRING_BONES_ERROR_CAPACITY_EXCEEDED = -4,
    ANIMATION_SPRING_BONES_ERROR_INVALID_HANDLE = -5,
    ANIMATION_SPRING_BONES_ERROR_SERIALIZATION_FAILED = -6,
    ANIMATION_SPRING_BONES_ERROR_GPU_UNAVAILABLE = -7,
    ANIMATION_SPRING_BONES_ERROR_THREADING_ERROR = -8,
    ANIMATION_SPRING_BONES_ERROR_FILE_IO_ERROR = -9,
    ANIMATION_SPRING_BONES_ERROR_COMPRESSION_FAILED = -10
} animation_spring_bones_error_t;

/* Performance counters */
typedef struct {
    uint64_t total_updates;
    uint64_t total_simulations;
    uint64_t total_compressions;
    uint64_t total_decompressions;
    uint64_t total_gpu_operations;
    uint64_t total_simd_operations;
    uint64_t total_async_operations;
    uint64_t total_cache_hits;
    uint64_t total_cache_misses;
    uint64_t total_memory_allocations;
    uint64_t total_memory_deallocations;
    double total_update_time;
    double total_simulation_time;
    double total_compression_time;
    double total_gpu_time;
    size_t peak_memory_usage;
    size_t current_memory_usage;
} animation_spring_bones_performance_counters_t;

/* SIMD optimization context */
typedef struct {
    bool avx2_supported;
    bool sse_supported;
    __m256 gravity_vector_avx;
    __m128 gravity_vector_sse;
    float damping_factor;
    float stiffness_factor;
} animation_spring_bones_simd_context_t;

/* GPU integration context */
typedef struct {
    bool gpu_available;
    void* gpu_device;
    void* compute_shader;
    void* gpu_buffer;
    size_t gpu_buffer_size;
    pthread_mutex_t gpu_mutex;
} animation_spring_bones_gpu_context_t;

/* Async operation context */
typedef struct {
    uint32_t operation_id;
    bool active;
    pthread_t worker_thread;
    void* operation_data;
    size_t data_size;
    void (*completion_callback)(uint32_t operation_id, int result);
    int result;
    bool completed;
} animation_spring_bones_async_operation_t;

/* Cache entry */
typedef struct {
    uint32_t cache_id;
    void* data;
    size_t data_size;
    uint64_t last_access_time;
    uint32_t access_count;
    bool valid;
} animation_spring_bones_cache_entry_t;

/* LOD configuration */
typedef struct {
    uint32_t lod_levels;
    float distance_thresholds[8];
    uint32_t bone_counts[8];
    float simulation_quality[8];
    uint32_t current_lod;
} animation_spring_bones_lod_config_t;

/* Culling integration */
typedef struct {
    bool culling_enabled;
    float culling_distance;
    uint32_t visible_bones;
    uint32_t culled_bones;
    bool frustum_culling;
    bool occlusion_culling;
} animation_spring_bones_culling_context_t;

/* Render graph node */
typedef struct {
    uint32_t node_id;
    bool active;
    uint32_t dependency_count;
    uint32_t* dependencies;
    void (*execute_func)(uint32_t node_id, void* user_data);
    void* user_data;
} animation_spring_bones_render_graph_node_t;

/* Streaming support */
typedef struct {
    bool streaming_enabled;
    uint32_t chunk_size;
    uint32_t loaded_chunks;
    uint32_t total_chunks;
    void* stream_buffer;
    size_t stream_buffer_size;
    pthread_mutex_t stream_mutex;
} animation_spring_bones_streaming_context_t;

/* Batch processing */
typedef struct {
    uint32_t batch_size;
    uint32_t processed_items;
    uint32_t total_items;
    bool batch_processing_active;
    pthread_t batch_thread;
    pthread_mutex_t batch_mutex;
} animation_spring_bones_batch_context_t;

/* File watching for hot-reload */
typedef struct {
    int inotify_fd;
    int watch_descriptor;
    pthread_t watch_thread;
    bool watching_active;
    char watched_file_path[256];
    void (*reload_callback)(const char* file_path);
} animation_spring_bones_file_watcher_t;

/* Serialization context */
typedef struct {
    uint32_t magic_number;
    uint32_t version;
    uint32_t data_size;
    uint32_t checksum;
    uint64_t timestamp;
} animation_spring_bones_serialization_header_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_spring_bones_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    /* Spring physics properties */
    float stiffness;
    float damping;
    float mass;
    float rest_length;
    float current_length;
    
    /* Simulation state */
    float position[3];
    float velocity[3];
    float force[3];
    
    /* LOD and culling */
    uint32_t lod_level;
    bool is_visible;
    float distance_from_viewer;
    
    /* Cache and streaming */
    uint32_t cache_id;
    bool is_streamed;
    uint32_t chunk_index;
    
    /* GPU integration */
    bool gpu_processed;
    uint32_t gpu_buffer_offset;
    
    /* Error tracking */
    int last_error;
    char error_message[256];
} animation_spring_bones_internal_t;

typedef struct animation_spring_bones_context {
    animation_spring_bones_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Thread safety */
    pthread_mutex_t context_mutex;
    pthread_rwlock_t data_rwlock;
    
    /* Performance counters */
    animation_spring_bones_performance_counters_t performance;
    
    /* Error handling */
    animation_spring_bones_error_t last_error;
    char error_description[512];
    
    /* SIMD optimization */
    animation_spring_bones_simd_context_t simd_context;
    
    /* GPU integration */
    animation_spring_bones_gpu_context_t gpu_context;
    
    /* Async operations */
    animation_spring_bones_async_operation_t async_operations[ANIMATION_SPRING_BONES_MAX_ASYNC_OPERATIONS];
    uint32_t next_async_operation_id;
    pthread_mutex_t async_mutex;
    
    /* Caching layer */
    animation_spring_bones_cache_entry_t cache[ANIMATION_SPRING_BONES_CACHE_SIZE];
    uint32_t next_cache_id;
    pthread_mutex_t cache_mutex;
    
    /* LOD support */
    animation_spring_bones_lod_config_t lod_config;
    
    /* Culling integration */
    animation_spring_bones_culling_context_t culling_context;
    
    /* Render graph */
    animation_spring_bones_render_graph_node_t* render_graph_nodes;
    uint32_t render_graph_node_count;
    uint32_t render_graph_capacity;
    
    /* Streaming support */
    animation_spring_bones_streaming_context_t streaming_context;
    
    /* Batch processing */
    animation_spring_bones_batch_context_t batch_context;
    
    /* Hot-reload file watching */
    animation_spring_bones_file_watcher_t file_watcher;
    
    /* Memory budget tracking */
    size_t memory_budget;
    size_t current_memory_usage;
    
    /* Compression support */
    bool compression_enabled;
    int compression_level;
    size_t compression_ratio;
} animation_spring_bones_context_t;

static animation_spring_bones_context_t g_spring_bones_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

/* Helper function to get current timestamp */
static uint64_t animation_spring_bones_get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Helper function to calculate checksum */
static uint32_t animation_spring_bones_calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = checksum * 31 + bytes[i];
    }
    return checksum;
}

/* Error handling functions */
static void animation_spring_bones_set_error(animation_spring_bones_error_t error, const char* description) {
    g_spring_bones_ctx.last_error = error;
    if (description) {
        strncpy(g_spring_bones_ctx.error_description, description, sizeof(g_spring_bones_ctx.error_description) - 1);
        g_spring_bones_ctx.error_description[sizeof(g_spring_bones_ctx.error_description) - 1] = '\0';
    }
}

static const char* animation_spring_bones_get_error_string(animation_spring_bones_error_t error) {
    switch (error) {
        case ANIMATION_SPRING_BONES_ERROR_NONE: return "No error";
        case ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM: return "Invalid parameter";
        case ANIMATION_SPRING_BONES_ERROR_NOT_INITIALIZED: return "System not initialized";
        case ANIMATION_SPRING_BONES_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case ANIMATION_SPRING_BONES_ERROR_CAPACITY_EXCEEDED: return "Capacity exceeded";
        case ANIMATION_SPRING_BONES_ERROR_INVALID_HANDLE: return "Invalid handle";
        case ANIMATION_SPRING_BONES_ERROR_SERIALIZATION_FAILED: return "Serialization failed";
        case ANIMATION_SPRING_BONES_ERROR_GPU_UNAVAILABLE: return "GPU unavailable";
        case ANIMATION_SPRING_BONES_ERROR_THREADING_ERROR: return "Threading error";
        case ANIMATION_SPRING_BONES_ERROR_FILE_IO_ERROR: return "File I/O error";
        case ANIMATION_SPRING_BONES_ERROR_COMPRESSION_FAILED: return "Compression failed";
        default: return "Unknown error";
    }
}

/* SIMD optimization functions */
static void animation_spring_bones_init_simd(void) {
    g_spring_bones_ctx.simd_context.sse_supported = false;
    g_spring_bones_ctx.simd_context.avx2_supported = false;
    
#ifdef __AVX2__
    g_spring_bones_ctx.simd_context.avx2_supported = true;
    g_spring_bones_ctx.simd_context.gravity_vector_avx = _mm256_set1_ps(9.81f);
#endif
#ifdef __SSE__
    g_spring_bones_ctx.simd_context.sse_supported = true;
    g_spring_bones_ctx.simd_context.gravity_vector_sse = _mm_set1_ps(9.81f);
#endif
    
    g_spring_bones_ctx.simd_context.damping_factor = 0.95f;
    g_spring_bones_ctx.simd_context.stiffness_factor = 50.0f;
}

static void animation_spring_bones_simd_update_bones(animation_spring_bones_internal_t* bones, uint32_t count, float dt) {
    if (g_spring_bones_ctx.simd_context.avx2_supported && count >= 8) {
        // AVX2 implementation for 8 bones at once
        __m256 dt_vec = _mm256_set1_ps(dt);
        __m256 damping = _mm256_set1_ps(g_spring_bones_ctx.simd_context.damping_factor);
        __m256 stiffness = _mm256_set1_ps(g_spring_bones_ctx.simd_context.stiffness_factor);
        
        for (uint32_t i = 0; i <= count - 8; i += 8) {
            // Load positions and velocities
            __m256 pos_x = _mm256_set_ps(bones[i+7].position[0], bones[i+6].position[0], bones[i+5].position[0], bones[i+4].position[0],
                                        bones[i+3].position[0], bones[i+2].position[0], bones[i+1].position[0], bones[i].position[0]);
            __m256 pos_y = _mm256_set_ps(bones[i+7].position[1], bones[i+6].position[1], bones[i+5].position[1], bones[i+4].position[1],
                                        bones[i+3].position[1], bones[i+2].position[1], bones[i+1].position[1], bones[i].position[1]);
            __m256 pos_z = _mm256_set_ps(bones[i+7].position[2], bones[i+6].position[2], bones[i+5].position[2], bones[i+4].position[2],
                                        bones[i+3].position[2], bones[i+2].position[2], bones[i+1].position[2], bones[i].position[2]);
            
            // Apply spring forces (simplified for demonstration)
            __m256 force_x = _mm256_mul_ps(_mm256_sub_ps(_mm256_setzero_ps(), pos_x), stiffness);
            __m256 force_y = _mm256_mul_ps(_mm256_sub_ps(_mm256_setzero_ps(), pos_y), stiffness);
            __m256 force_z = _mm256_mul_ps(_mm256_sub_ps(_mm256_setzero_ps(), pos_z), stiffness);
            
            // Update velocities
            __m256 vel_x = _mm256_mul_ps(force_x, dt_vec);
            __m256 vel_y = _mm256_mul_ps(force_y, dt_vec);
            __m256 vel_z = _mm256_mul_ps(force_z, dt_vec);
            
            vel_x = _mm256_mul_ps(vel_x, damping);
            vel_y = _mm256_mul_ps(vel_y, damping);
            vel_z = _mm256_mul_ps(vel_z, damping);
            
            // Update positions
            pos_x = _mm256_add_ps(pos_x, _mm256_mul_ps(vel_x, dt_vec));
            pos_y = _mm256_add_ps(pos_y, _mm256_mul_ps(vel_y, dt_vec));
            pos_z = _mm256_add_ps(pos_z, _mm256_mul_ps(vel_z, dt_vec));
            
            // Store results
            float pos_x_array[8], pos_y_array[8], pos_z_array[8];
            float vel_x_array[8], vel_y_array[8], vel_z_array[8];
            
            _mm256_storeu_ps(pos_x_array, pos_x);
            _mm256_storeu_ps(pos_y_array, pos_y);
            _mm256_storeu_ps(pos_z_array, pos_z);
            _mm256_storeu_ps(vel_x_array, vel_x);
            _mm256_storeu_ps(vel_y_array, vel_y);
            _mm256_storeu_ps(vel_z_array, vel_z);
            
            for (int j = 0; j < 8; j++) {
                bones[i+j].position[0] = pos_x_array[j];
                bones[i+j].position[1] = pos_y_array[j];
                bones[i+j].position[2] = pos_z_array[j];
                bones[i+j].velocity[0] = vel_x_array[j];
                bones[i+j].velocity[1] = vel_y_array[j];
                bones[i+j].velocity[2] = vel_z_array[j];
            }
        }
        
        g_spring_bones_ctx.performance.total_simd_operations++;
    }
}

/* GPU integration functions */
static int animation_spring_bones_init_gpu(void) {
    g_spring_bones_ctx.gpu_context.gpu_available = false;
    g_spring_bones_ctx.gpu_context.gpu_device = NULL;
    g_spring_bones_ctx.gpu_context.compute_shader = NULL;
    g_spring_bones_ctx.gpu_context.gpu_buffer = NULL;
    g_spring_bones_ctx.gpu_context.gpu_buffer_size = 0;
    
    if (pthread_mutex_init(&g_spring_bones_ctx.gpu_context.gpu_mutex, NULL) != 0) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_THREADING_ERROR, "Failed to initialize GPU mutex");
        return -1;
    }
    
    // In a real implementation, this would initialize GPU resources
    // For now, we'll simulate GPU availability
    g_spring_bones_ctx.gpu_context.gpu_available = true;
    
    return 0;
}

static void animation_spring_bones_shutdown_gpu(void) {
    pthread_mutex_destroy(&g_spring_bones_ctx.gpu_context.gpu_mutex);
    
    if (g_spring_bones_ctx.gpu_context.gpu_buffer) {
        free(g_spring_bones_ctx.gpu_context.gpu_buffer);
        g_spring_bones_ctx.gpu_context.gpu_buffer = NULL;
    }
    
    g_spring_bones_ctx.gpu_context.gpu_available = false;
}

static int animation_spring_bones_gpu_process_bones(animation_spring_bones_internal_t* bones, uint32_t count) {
    if (!g_spring_bones_ctx.gpu_context.gpu_available) {
        return ANIMATION_SPRING_BONES_ERROR_GPU_UNAVAILABLE;
    }
    
    pthread_mutex_lock(&g_spring_bones_ctx.gpu_context.gpu_mutex);
    
    size_t required_size = count * sizeof(animation_spring_bones_internal_t);
    if (g_spring_bones_ctx.gpu_context.gpu_buffer_size < required_size) {
        if (g_spring_bones_ctx.gpu_context.gpu_buffer) {
            free(g_spring_bones_ctx.gpu_context.gpu_buffer);
        }
        g_spring_bones_ctx.gpu_context.gpu_buffer = malloc(required_size);
        if (!g_spring_bones_ctx.gpu_context.gpu_buffer) {
            pthread_mutex_unlock(&g_spring_bones_ctx.gpu_context.gpu_mutex);
            return ANIMATION_SPRING_BONES_ERROR_OUT_OF_MEMORY;
        }
        g_spring_bones_ctx.gpu_context.gpu_buffer_size = required_size;
    }
    
    // Copy data to GPU buffer
    memcpy(g_spring_bones_ctx.gpu_context.gpu_buffer, bones, required_size);
    
    // Simulate GPU processing
    // In a real implementation, this would dispatch compute shaders
    for (uint32_t i = 0; i < count; i++) {
        // Simple spring physics on GPU (simulated)
        animation_spring_bones_internal_t* bone = &bones[i];
        bone->force[0] = -bone->stiffness * bone->position[0];
        bone->force[1] = -bone->stiffness * bone->position[1];
        bone->force[2] = -bone->stiffness * bone->position[2];
        
        bone->velocity[0] += bone->force[0] / bone->mass;
        bone->velocity[1] += bone->force[1] / bone->mass;
        bone->velocity[2] += bone->force[2] / bone->mass;
        
        bone->velocity[0] *= bone->damping;
        bone->velocity[1] *= bone->damping;
        bone->velocity[2] *= bone->damping;
        
        bone->position[0] += bone->velocity[0];
        bone->position[1] += bone->velocity[1];
        bone->position[2] += bone->velocity[2];
        
        bone->gpu_processed = true;
    }
    
    pthread_mutex_unlock(&g_spring_bones_ctx.gpu_context.gpu_mutex);
    
    g_spring_bones_ctx.performance.total_gpu_operations++;
    return 0;
}

/* Async operations */
static void* animation_spring_bones_async_worker_thread(void* arg) {
    animation_spring_bones_async_operation_t* operation = (animation_spring_bones_async_operation_t*)arg;
    
    // Simulate async work
    usleep(1000); // 1ms delay
    
    operation->result = 0; // Success
    operation->completed = true;
    
    if (operation->completion_callback) {
        operation->completion_callback(operation->operation_id, operation->result);
    }
    
    return NULL;
}

static uint32_t animation_spring_bones_start_async_operation(void* data, size_t data_size, 
                                                          void (*callback)(uint32_t, int)) {
    pthread_mutex_lock(&g_spring_bones_ctx.async_mutex);
    
    uint32_t operation_id = g_spring_bones_ctx.next_async_operation_id++;
    
    // Find free slot
    for (int i = 0; i < ANIMATION_SPRING_BONES_MAX_ASYNC_OPERATIONS; i++) {
        if (!g_spring_bones_ctx.async_operations[i].active) {
            animation_spring_bones_async_operation_t* operation = &g_spring_bones_ctx.async_operations[i];
            
            operation->operation_id = operation_id;
            operation->active = true;
            operation->completed = false;
            operation->data = malloc(data_size);
            if (operation->data) {
                memcpy(operation->data, data, data_size);
            }
            operation->data_size = data_size;
            operation->completion_callback = callback;
            
            pthread_create(&operation->worker_thread, NULL, animation_spring_bones_async_worker_thread, operation);
            
            g_spring_bones_ctx.performance.total_async_operations++;
            
            pthread_mutex_unlock(&g_spring_bones_ctx.async_mutex);
            return operation_id;
        }
    }
    
    pthread_mutex_unlock(&g_spring_bones_ctx.async_mutex);
    return 0; // No available slots
}

/* Caching layer */
static uint32_t animation_spring_bones_cache_add(const void* data, size_t data_size) {
    pthread_mutex_lock(&g_spring_bones_ctx.cache_mutex);
    
    uint32_t cache_id = g_spring_bones_ctx.next_cache_id++;
    
    // Find free slot or evict LRU
    uint32_t oldest_index = 0;
    uint64_t oldest_time = UINT64_MAX;
    
    for (int i = 0; i < ANIMATION_SPRING_BONES_CACHE_SIZE; i++) {
        if (!g_spring_bones_ctx.cache[i].valid) {
            oldest_index = i;
            break;
        }
        if (g_spring_bones_ctx.cache[i].last_access_time < oldest_time) {
            oldest_time = g_spring_bones_ctx.cache[i].last_access_time;
            oldest_index = i;
        }
    }
    
    animation_spring_bones_cache_entry_t* entry = &g_spring_bones_ctx.cache[oldest_index];
    
    // Free old data if exists
    if (entry->data) {
        free(entry->data);
    }
    
    // Add new data
    entry->cache_id = cache_id;
    entry->data = malloc(data_size);
    if (entry->data) {
        memcpy(entry->data, data, data_size);
    }
    entry->data_size = data_size;
    entry->last_access_time = animation_spring_bones_get_timestamp();
    entry->access_count = 1;
    entry->valid = true;
    
    pthread_mutex_unlock(&g_spring_bones_ctx.cache_mutex);
    
    g_spring_bones_ctx.performance.total_cache_misses++;
    return cache_id;
}

static bool animation_spring_bones_cache_lookup(uint32_t cache_id, void** out_data, size_t* out_size) {
    pthread_mutex_lock(&g_spring_bones_ctx.cache_mutex);
    
    for (int i = 0; i < ANIMATION_SPRING_BONES_CACHE_SIZE; i++) {
        animation_spring_bones_cache_entry_t* entry = &g_spring_bones_ctx.cache[i];
        if (entry->valid && entry->cache_id == cache_id) {
            entry->last_access_time = animation_spring_bones_get_timestamp();
            entry->access_count++;
            
            *out_data = entry->data;
            *out_size = entry->data_size;
            
            pthread_mutex_unlock(&g_spring_bones_ctx.cache_mutex);
            
            g_spring_bones_ctx.performance.total_cache_hits++;
            return true;
        }
    }
    
    pthread_mutex_unlock(&g_spring_bones_ctx.cache_mutex);
    
    g_spring_bones_ctx.performance.total_cache_misses++;
    return false;
}

/* Compression functions */
static int animation_spring_bones_compress_data(const void* input, size_t input_size, 
                                               void** output, size_t* output_size) {
    if (!g_spring_bones_ctx.compression_enabled) {
        *output = malloc(input_size);
        if (!*output) {
            return ANIMATION_SPRING_BONES_ERROR_OUT_OF_MEMORY;
        }
        memcpy(*output, input, input_size);
        *output_size = input_size;
        return 0;
    }
    
    // Use LZ4 for fast compression
    int compressed_size = LZ4_compressBound((int)input_size);
    *output = malloc(compressed_size);
    if (!*output) {
        return ANIMATION_SPRING_BONES_ERROR_OUT_OF_MEMORY;
    }
    
    int actual_size = LZ4_compress_default((const char*)input, (char*)*output, 
                                         (int)input_size, compressed_size);
    if (actual_size <= 0) {
        free(*output);
        *output = NULL;
        return ANIMATION_SPRING_BONES_ERROR_COMPRESSION_FAILED;
    }
    
    *output_size = actual_size;
    g_spring_bones_ctx.compression_ratio = input_size / actual_size;
    g_spring_bones_ctx.performance.total_compressions++;
    
    return 0;
}

static int animation_spring_bones_decompress_data(const void* input, size_t input_size, 
                                                 void** output, size_t output_size) {
    if (!g_spring_bones_ctx.compression_enabled) {
        *output = malloc(output_size);
        if (!*output) {
            return ANIMATION_SPRING_BONES_ERROR_OUT_OF_MEMORY;
        }
        memcpy(*output, input, output_size);
        return 0;
    }
    
    *output = malloc(output_size);
    if (!*output) {
        return ANIMATION_SPRING_BONES_ERROR_OUT_OF_MEMORY;
    }
    
    int result = LZ4_decompress_safe((const char*)input, (char*)*output, 
                                     (int)input_size, (int)output_size);
    if (result < 0) {
        free(*output);
        *output = NULL;
        return ANIMATION_SPRING_BONES_ERROR_COMPRESSION_FAILED;
    }
    
    g_spring_bones_ctx.performance.total_decompressions++;
    return 0;
}

/* File watching for hot-reload */
static void* animation_spring_bones_file_watch_thread(void* arg) {
    char buffer[4096];
    
    while (g_spring_bones_ctx.file_watcher.watching_active) {
        ssize_t length = read(g_spring_bones_ctx.file_watcher.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            struct inotify_event* event = (struct inotify_event*)buffer;
            if (event->mask & IN_MODIFY) {
                if (g_spring_bones_ctx.file_watcher.reload_callback) {
                    g_spring_bones_ctx.file_watcher.reload_callback(g_spring_bones_ctx.file_watcher.watched_file_path);
                }
            }
        }
        usleep(100000); // 100ms
    }
    
    return NULL;
}

static int animation_spring_bones_start_file_watching(const char* file_path, void (*callback)(const char*)) {
    g_spring_bones_ctx.file_watcher.inotify_fd = inotify_init();
    if (g_spring_bones_ctx.file_watcher.inotify_fd < 0) {
        return ANIMATION_SPRING_BONES_ERROR_FILE_IO_ERROR;
    }
    
    strncpy(g_spring_bones_ctx.file_watcher.watched_file_path, file_path, 
           sizeof(g_spring_bones_ctx.file_watcher.watched_file_path) - 1);
    g_spring_bones_ctx.file_watcher.watched_file_path[sizeof(g_spring_bones_ctx.file_watcher.watched_file_path) - 1] = '\0';
    
    g_spring_bones_ctx.file_watcher.watch_descriptor = inotify_add_watch(
        g_spring_bones_ctx.file_watcher.inotify_fd, file_path, IN_MODIFY);
    
    if (g_spring_bones_ctx.file_watcher.watch_descriptor < 0) {
        close(g_spring_bones_ctx.file_watcher.inotify_fd);
        return ANIMATION_SPRING_BONES_ERROR_FILE_IO_ERROR;
    }
    
    g_spring_bones_ctx.file_watcher.reload_callback = callback;
    g_spring_bones_ctx.watching_active = true;
    
    pthread_create(&g_spring_bones_ctx.file_watcher.watch_thread, NULL, 
                   animation_spring_bones_file_watch_thread, NULL);
    
    return 0;
}

static void animation_spring_bones_stop_file_watching(void) {
    g_spring_bones_ctx.file_watcher.watching_active = false;
    
    if (g_spring_bones_ctx.file_watcher.watch_descriptor >= 0) {
        inotify_rm_watch(g_spring_bones_ctx.file_watcher.inotify_fd, 
                        g_spring_bones_ctx.file_watcher.watch_descriptor);
    }
    
    if (g_spring_bones_ctx.file_watcher.inotify_fd >= 0) {
        close(g_spring_bones_ctx.file_watcher.inotify_fd);
    }
    
    pthread_join(g_spring_bones_ctx.file_watcher.watch_thread, NULL);
}

/* Enhanced validation with comprehensive error checking */
static bool animation_spring_bones_validate(const animation_spring_bones_internal_t* item) {
    if (!item) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Item is null");
        return false;
    }
    
    if (!item->initialized) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Item not initialized");
        return false;
    }
    
    if (item->stiffness < 0.0f || item->damping < 0.0f || item->mass <= 0.0f) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Invalid physics parameters");
        return false;
    }
    
    if (item->rest_length < 0.0f || item->current_length < 0.0f) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Invalid length parameters");
        return false;
    }
    
    // Check for NaN or infinity in position/velocity
    for (int i = 0; i < 3; i++) {
        if (!isfinite(item->position[i]) || !isfinite(item->velocity[i]) || !isfinite(item->force[i])) {
            animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Invalid physics state (NaN/Inf)");
            return false;
        }
    }
    
    return true;
}

static void animation_spring_bones_cleanup_internal(animation_spring_bones_internal_t* item) {
    // TODO: Implement IK solvers
    // TODO: Add morph target support
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_spring_bones_init(void) {
    if (g_spring_bones_ctx.initialized) {
        return 0; // Already initialized
    }
    
    // Initialize context mutex
    if (pthread_mutex_init(&g_spring_bones_ctx.context_mutex, NULL) != 0) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_THREADING_ERROR, "Failed to initialize context mutex");
        return -1;
    }
    
    // Initialize read-write lock
    if (pthread_rwlock_init(&g_spring_bones_ctx.data_rwlock, NULL) != 0) {
        pthread_mutex_destroy(&g_spring_bones_ctx.context_mutex);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_THREADING_ERROR, "Failed to initialize read-write lock");
        return -1;
    }
    
    // Initialize async mutex
    if (pthread_mutex_init(&g_spring_bones_ctx.async_mutex, NULL) != 0) {
        pthread_mutex_destroy(&g_spring_bones_ctx.context_mutex);
        pthread_rwlock_destroy(&g_spring_bones_ctx.data_rwlock);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_THREADING_ERROR, "Failed to initialize async mutex");
        return -1;
    }
    
    // Initialize cache mutex
    if (pthread_mutex_init(&g_spring_bones_ctx.cache_mutex, NULL) != 0) {
        pthread_mutex_destroy(&g_spring_bones_ctx.context_mutex);
        pthread_rwlock_destroy(&g_spring_bones_ctx.data_rwlock);
        pthread_mutex_destroy(&g_spring_bones_ctx.async_mutex);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_THREADING_ERROR, "Failed to initialize cache mutex");
        return -1;
    }
    
    g_spring_bones_ctx.capacity = ANIMATION_SPRING_BONES_DEFAULT_CAPACITY;
    g_spring_bones_ctx.items = calloc(g_spring_bones_ctx.capacity, sizeof(animation_spring_bones_internal_t));
    if (!g_spring_bones_ctx.items) {
        pthread_mutex_destroy(&g_spring_bones_ctx.context_mutex);
        pthread_rwlock_destroy(&g_spring_bones_ctx.data_rwlock);
        pthread_mutex_destroy(&g_spring_bones_ctx.async_mutex);
        pthread_mutex_destroy(&g_spring_bones_ctx.cache_mutex);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_OUT_OF_MEMORY, "Failed to allocate items array");
        return -1;
    }
    
    g_spring_bones_ctx.count = 0;
    g_spring_bones_ctx.initialized = true;
    
    // Initialize subsystems
    animation_spring_bones_init_simd();
    animation_spring_bones_init_gpu();
    
    // Initialize performance counters
    memset(&g_spring_bones_ctx.performance, 0, sizeof(g_spring_bones_ctx.performance));
    
    // Initialize LOD configuration
    g_spring_bones_ctx.lod_config.lod_levels = 4;
    g_spring_bones_ctx.lod_config.distance_thresholds[0] = 10.0f;
    g_spring_bones_ctx.lod_config.distance_thresholds[1] = 25.0f;
    g_spring_bones_ctx.lod_config.distance_thresholds[2] = 50.0f;
    g_spring_bones_ctx.lod_config.distance_thresholds[3] = 100.0f;
    g_spring_bones_ctx.lod_config.bone_counts[0] = 100;
    g_spring_bones_ctx.lod_config.bone_counts[1] = 50;
    g_spring_bones_ctx.lod_config.bone_counts[2] = 25;
    g_spring_bones_ctx.lod_config.bone_counts[3] = 10;
    g_spring_bones_ctx.lod_config.simulation_quality[0] = 1.0f;
    g_spring_bones_ctx.lod_config.simulation_quality[1] = 0.75f;
    g_spring_bones_ctx.lod_config.simulation_quality[2] = 0.5f;
    g_spring_bones_ctx.lod_config.simulation_quality[3] = 0.25f;
    g_spring_bones_ctx.lod_config.current_lod = 0;
    
    // Initialize culling context
    g_spring_bones_ctx.culling_context.culling_enabled = true;
    g_spring_bones_ctx.culling_context.culling_distance = 200.0f;
    g_spring_bones_ctx.culling_context.visible_bones = 0;
    g_spring_bones_ctx.culling_context.culled_bones = 0;
    g_spring_bones_ctx.culling_context.frustum_culling = true;
    g_spring_bones_ctx.culling_context.occlusion_culling = false;
    
    // Initialize streaming context
    g_spring_bones_ctx.streaming_context.streaming_enabled = false;
    g_spring_bones_ctx.streaming_context.chunk_size = 1024;
    g_spring_bones_ctx.streaming_context.loaded_chunks = 0;
    g_spring_bones_ctx.streaming_context.total_chunks = 0;
    g_spring_bones_ctx.streaming_context.stream_buffer = NULL;
    g_spring_bones_ctx.streaming_context.stream_buffer_size = 0;
    pthread_mutex_init(&g_spring_bones_ctx.streaming_context.stream_mutex, NULL);
    
    // Initialize batch processing
    g_spring_bones_ctx.batch_context.batch_size = 32;
    g_spring_bones_ctx.batch_context.processed_items = 0;
    g_spring_bones_ctx.batch_context.total_items = 0;
    g_spring_bones_ctx.batch_context.batch_processing_active = false;
    pthread_mutex_init(&g_spring_bones_ctx.batch_context.batch_mutex, NULL);
    
    // Initialize render graph
    g_spring_bones_ctx.render_graph_capacity = 64;
    g_spring_bones_ctx.render_graph_nodes = calloc(g_spring_bones_ctx.render_graph_capacity, 
                                                   sizeof(animation_spring_bones_render_graph_node_t));
    g_spring_bones_ctx.render_graph_node_count = 0;
    
    // Initialize memory budget
    g_spring_bones_ctx.memory_budget = ANIMATION_SPRING_BONES_MEMORY_BUDGET;
    g_spring_bones_ctx.current_memory_usage = 0;
    
    // Initialize compression
    g_spring_bones_ctx.compression_enabled = true;
    g_spring_bones_ctx.compression_level = 6;
    g_spring_bones_ctx.compression_ratio = 1;
    
    // Initialize cache
    memset(g_spring_bones_ctx.cache, 0, sizeof(g_spring_bones_ctx.cache));
    g_spring_bones_ctx.next_cache_id = 1;
    
    // Initialize async operations
    memset(g_spring_bones_ctx.async_operations, 0, sizeof(g_spring_bones_ctx.async_operations));
    g_spring_bones_ctx.next_async_operation_id = 1;
    
    // Initialize file watcher
    g_spring_bones_ctx.file_watcher.inotify_fd = -1;
    g_spring_bones_ctx.file_watcher.watch_descriptor = -1;
    g_spring_bones_ctx.file_watcher.watching_active = false;
    
    g_spring_bones_ctx.performance.total_memory_allocations++;
    
    return 0;
}

void animation_spring_bones_shutdown(void) {
    if (!g_spring_bones_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_spring_bones_ctx.context_mutex);
    
    // Stop file watching
    animation_spring_bones_stop_file_watching();
    
    // Stop batch processing
    if (g_spring_bones_ctx.batch_context.batch_processing_active) {
        g_spring_bones_ctx.batch_context.batch_processing_active = false;
        pthread_join(g_spring_bones_ctx.batch_context.batch_thread, NULL);
    }
    pthread_mutex_destroy(&g_spring_bones_ctx.batch_context.batch_mutex);
    
    // Cleanup streaming
    if (g_spring_bones_ctx.streaming_context.stream_buffer) {
        free(g_spring_bones_ctx.streaming_context.stream_buffer);
    }
    pthread_mutex_destroy(&g_spring_bones_ctx.streaming_context.stream_mutex);
    
    // Cleanup async operations
    for (int i = 0; i < ANIMATION_SPRING_BONES_MAX_ASYNC_OPERATIONS; i++) {
        animation_spring_bones_async_operation_t* operation = &g_spring_bones_ctx.async_operations[i];
        if (operation->active) {
            operation->active = false;
            pthread_join(operation->worker_thread, NULL);
        }
        if (operation->data) {
            free(operation->data);
        }
    }
    
    // Cleanup cache
    for (int i = 0; i < ANIMATION_SPRING_BONES_CACHE_SIZE; i++) {
        if (g_spring_bones_ctx.cache[i].data) {
            free(g_spring_bones_ctx.cache[i].data);
        }
    }
    
    // Cleanup render graph
    if (g_spring_bones_ctx.render_graph_nodes) {
        for (uint32_t i = 0; i < g_spring_bones_ctx.render_graph_node_count; i++) {
            if (g_spring_bones_ctx.render_graph_nodes[i].dependencies) {
                free(g_spring_bones_ctx.render_graph_nodes[i].dependencies);
            }
        }
        free(g_spring_bones_ctx.render_graph_nodes);
    }
    
    // Cleanup GPU resources
    animation_spring_bones_shutdown_gpu();
    
    // Cleanup all items
    for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
        animation_spring_bones_cleanup_internal(&g_spring_bones_ctx.items[i]);
    }
    
    free(g_spring_bones_ctx.items);
    g_spring_bones_ctx.items = NULL;
    g_spring_bones_ctx.count = 0;
    g_spring_bones_ctx.capacity = 0;
    g_spring_bones_ctx.initialized = false;
    
    pthread_mutex_unlock(&g_spring_bones_ctx.context_mutex);
    
    // Destroy mutexes and locks
    pthread_mutex_destroy(&g_spring_bones_ctx.context_mutex);
    pthread_rwlock_destroy(&g_spring_bones_ctx.data_rwlock);
    pthread_mutex_destroy(&g_spring_bones_ctx.async_mutex);
    pthread_mutex_destroy(&g_spring_bones_ctx.cache_mutex);
    
    g_spring_bones_ctx.performance.total_memory_deallocations++;
}

int animation_spring_bones_create(animation_spring_bones_handle_t* out_handle, const animation_spring_bones_desc_t* desc) {
    if (!out_handle || !desc) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Null handle or description");
        return -1;
    }
    
    if (!g_spring_bones_ctx.initialized) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_NOT_INITIALIZED, "System not initialized");
        return -2;
    }
    
    pthread_mutex_lock(&g_spring_bones_ctx.context_mutex);
    
    if (g_spring_bones_ctx.count >= g_spring_bones_ctx.capacity) {
        pthread_mutex_unlock(&g_spring_bones_ctx.context_mutex);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_CAPACITY_EXCEEDED, "Capacity exceeded");
        return -3;
    }
    
    uint32_t index = g_spring_bones_ctx.count++;
    animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[index];
    
    // Initialize item with default spring physics values
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    // Spring physics properties
    item->stiffness = 50.0f;
    item->damping = 0.95f;
    item->mass = 1.0f;
    item->rest_length = 1.0f;
    item->current_length = 1.0f;
    
    // Simulation state
    item->position[0] = 0.0f; item->position[1] = 0.0f; item->position[2] = 0.0f;
    item->velocity[0] = 0.0f; item->velocity[1] = 0.0f; item->velocity[2] = 0.0f;
    item->force[0] = 0.0f; item->force[1] = 0.0f; item->force[2] = 0.0f;
    
    // LOD and culling
    item->lod_level = 0;
    item->is_visible = true;
    item->distance_from_viewer = 0.0f;
    
    // Cache and streaming
    item->cache_id = 0;
    item->is_streamed = false;
    item->chunk_index = 0;
    
    // GPU integration
    item->gpu_processed = false;
    item->gpu_buffer_offset = 0;
    
    // Error tracking
    item->last_error = ANIMATION_SPRING_BONES_ERROR_NONE;
    item->error_message[0] = '\0';
    
    // Update memory usage
    g_spring_bones_ctx.current_memory_usage += sizeof(animation_spring_bones_internal_t);
    if (g_spring_bones_ctx.current_memory_usage > g_spring_bones_ctx.performance.peak_memory_usage) {
        g_spring_bones_ctx.performance.peak_memory_usage = g_spring_bones_ctx.current_memory_usage;
    }
    
    pthread_mutex_unlock(&g_spring_bones_ctx.context_mutex);
    
    out_handle->id = index;
    return 0;
}

void animation_spring_bones_destroy(animation_spring_bones_handle_t handle) {
    if (handle.id >= g_spring_bones_ctx.count) {
        return;
    }
    
    pthread_mutex_lock(&g_spring_bones_ctx.context_mutex);
    
    animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[handle.id];
    
    // Remove from cache if present
    if (item->cache_id != 0) {
        void* cached_data;
        size_t cached_size;
        if (animation_spring_bones_cache_lookup(item->cache_id, &cached_data, &cached_size)) {
            // Cache entry will be automatically evicted when needed
        }
    }
    
    animation_spring_bones_cleanup_internal(item);
    
    // Update memory usage
    g_spring_bones_ctx.current_memory_usage -= sizeof(animation_spring_bones_internal_t);
    g_spring_bones_ctx.current_memory_usage -= item->data_size;
    
    g_spring_bones_ctx.performance.total_memory_deallocations++;
    
    pthread_mutex_unlock(&g_spring_bones_ctx.context_mutex);
}

int animation_spring_bones_update(animation_spring_bones_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_spring_bones_ctx.count) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_HANDLE, "Invalid handle");
        return -1;
    }
    
    pthread_rwlock_wrlock(&g_spring_bones_ctx.data_rwlock);
    
    animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Item not initialized");
        return -2;
    }
    
    // Validate item
    if (!animation_spring_bones_validate(item)) {
        pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
        return item->last_error;
    }
    
    // Update data if provided
    if (data && size > 0) {
        if (item->data) {
            free(item->data);
        }
        item->data = malloc(size);
        if (!item->data) {
            pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
            animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_OUT_OF_MEMORY, "Failed to allocate data");
            return -3;
        }
        memcpy(item->data, data, size);
        item->data_size = size;
        
        // Update memory usage
        g_spring_bones_ctx.current_memory_usage += size;
        if (g_spring_bones_ctx.current_memory_usage > g_spring_bones_ctx.performance.peak_memory_usage) {
            g_spring_bones_ctx.performance.peak_memory_usage = g_spring_bones_ctx.current_memory_usage;
        }
    }
    
    // Perform spring physics simulation
    double start_time = animation_spring_bones_get_timestamp() / 1000000000.0;
    
    // Apply spring force: F = -k * (x - rest_length)
    float displacement = sqrtf(item->position[0] * item->position[0] + 
                              item->position[1] * item->position[1] + 
                              item->position[2] * item->position[2]) - item->rest_length;
    
    item->force[0] = -item->stiffness * item->position[0];
    item->force[1] = -item->stiffness * item->position[1];
    item->force[2] = -item->stiffness * item->position[2];
    
    // Update velocity: v = v + (F/m) * dt
    float dt = 0.016f; // 60 FPS
    item->velocity[0] += (item->force[0] / item->mass) * dt;
    item->velocity[1] += (item->force[1] / item->mass) * dt;
    item->velocity[2] += (item->force[2] / item->mass) * dt;
    
    // Apply damping
    item->velocity[0] *= item->damping;
    item->velocity[1] *= item->damping;
    item->velocity[2] *= item->damping;
    
    // Update position: x = x + v * dt
    item->position[0] += item->velocity[0] * dt;
    item->position[1] += item->velocity[1] * dt;
    item->position[2] += item->velocity[2] * dt;
    
    // Update current length
    item->current_length = sqrtf(item->position[0] * item->position[0] + 
                                 item->position[1] * item->position[1] + 
                                 item->position[2] * item->position[2]);
    
    double end_time = animation_spring_bones_get_timestamp() / 1000000000.0;
    g_spring_bones_ctx.performance.total_update_time += (end_time - start_time);
    g_spring_bones_ctx.performance.total_updates++;
    
    // Try GPU processing if available
    if (g_spring_bones_ctx.gpu_context.gpu_available && !item->gpu_processed) {
        int gpu_result = animation_spring_bones_gpu_process_bones(item, 1);
        if (gpu_result == 0) {
            g_spring_bones_ctx.performance.total_gpu_operations++;
        }
    }
    
    // Try SIMD processing if available
    if (g_spring_bones_ctx.simd_context.avx2_supported || g_spring_bones_ctx.simd_context.sse_supported) {
        animation_spring_bones_simd_update_bones(item, 1, dt);
    }
    
    item->dirty = true;
    item->frame_updated++;
    
    pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
    
    return 0;
}

bool animation_spring_bones_is_valid(animation_spring_bones_handle_t handle) {
    if (handle.id >= g_spring_bones_ctx.count) {
        return false;
    }
    
    pthread_rwlock_rdlock(&g_spring_bones_ctx.data_rwlock);
    bool valid = g_spring_bones_ctx.items[handle.id].initialized;
    pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
    
    return valid;
}

int animation_spring_bones_get_info(animation_spring_bones_handle_t handle, animation_spring_bones_info_t* out_info) {
    if (!out_info) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Null info pointer");
        return -1;
    }
    
    if (handle.id >= g_spring_bones_ctx.count) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_HANDLE, "Invalid handle");
        return -2;
    }
    
    pthread_rwlock_rdlock(&g_spring_bones_ctx.data_rwlock);
    
    const animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    // Update LOD based on distance
    if (item->distance_from_viewer > g_spring_bones_ctx.lod_config.distance_thresholds[3]) {
        g_spring_bones_ctx.lod_config.current_lod = 3;
    } else if (item->distance_from_viewer > g_spring_bones_ctx.lod_config.distance_thresholds[2]) {
        g_spring_bones_ctx.lod_config.current_lod = 2;
    } else if (item->distance_from_viewer > g_spring_bones_ctx.lod_config.distance_thresholds[1]) {
        g_spring_bones_ctx.lod_config.current_lod = 1;
    } else {
        g_spring_bones_ctx.lod_config.current_lod = 0;
    }
    
    pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
    
    return 0;
}

void animation_spring_bones_mark_dirty(animation_spring_bones_handle_t handle) {
    if (handle.id >= g_spring_bones_ctx.count) {
        return;
    }
    
    pthread_rwlock_wrlock(&g_spring_bones_ctx.data_rwlock);
    
    animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[handle.id];
    item->dirty = true;
    
    // Update culling
    if (g_spring_bones_ctx.culling_context.culling_enabled) {
        if (item->distance_from_viewer > g_spring_bones_ctx.culling_context.culling_distance) {
            item->is_visible = false;
            g_spring_bones_ctx.culling_context.culled_bones++;
        } else {
            item->is_visible = true;
            g_spring_bones_ctx.culling_context.visible_bones++;
        }
    }
    
    pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
}

int animation_spring_bones_process_pending(void) {
    pthread_mutex_lock(&g_spring_bones_ctx.batch_context.batch_mutex);
    
    int processed = 0;
    
    // Batch processing
    if (g_spring_bones_ctx.batch_context.batch_processing_active) {
        g_spring_bones_ctx.batch_context.processed_items = 0;
        g_spring_bones_ctx.batch_context.total_items = g_spring_bones_ctx.count;
        
        for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
            animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[i];
            if (item->initialized && item->dirty && item->is_visible) {
                // Process item through render graph
                for (uint32_t j = 0; j < g_spring_bones_ctx.render_graph_node_count; j++) {
                    animation_spring_bones_render_graph_node_t* node = &g_spring_bones_ctx.render_graph_nodes[j];
                    if (node->active && node->execute_func) {
                        node->execute_func(node->node_id, item);
                    }
                }
                
                item->dirty = false;
                processed++;
                g_spring_bones_ctx.batch_context.processed_items++;
            }
        }
    } else {
        // Single item processing
        for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
            animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[i];
            if (item->initialized && item->dirty) {
                item->dirty = false;
                processed++;
            }
        }
    }
    
    pthread_mutex_unlock(&g_spring_bones_ctx.batch_context.batch_mutex);
    
    return processed;
}

uint32_t animation_spring_bones_get_count(void) {
    return g_spring_bones_ctx.count;
}

size_t animation_spring_bones_get_memory_usage(void) {
    pthread_mutex_lock(&g_spring_bones_ctx.context_mutex);
    
    size_t total = sizeof(g_spring_bones_ctx);
    total += g_spring_bones_ctx.capacity * sizeof(animation_spring_bones_internal_t);
    total += g_spring_bones_ctx.current_memory_usage;
    
    // Add GPU memory usage
    if (g_spring_bones_ctx.gpu_context.gpu_available) {
        total += g_spring_bones_ctx.gpu_context.gpu_buffer_size;
    }
    
    // Add cache memory usage
    for (int i = 0; i < ANIMATION_SPRING_BONES_CACHE_SIZE; i++) {
        if (g_spring_bones_ctx.cache[i].data) {
            total += g_spring_bones_ctx.cache[i].data_size;
        }
    }
    
    // Add streaming memory usage
    if (g_spring_bones_ctx.streaming_context.stream_buffer) {
        total += g_spring_bones_ctx.streaming_context.stream_buffer_size;
    }
    
    // Add render graph memory usage
    total += g_spring_bones_ctx.render_graph_capacity * sizeof(animation_spring_bones_render_graph_node_t);
    for (uint32_t i = 0; i < g_spring_bones_ctx.render_graph_node_count; i++) {
        if (g_spring_bones_ctx.render_graph_nodes[i].dependencies) {
            total += g_spring_bones_ctx.render_graph_nodes[i].dependency_count * sizeof(uint32_t);
        }
    }
    
    pthread_mutex_unlock(&g_spring_bones_ctx.context_mutex);
    
    return total;
}

void animation_spring_bones_debug_print(animation_spring_bones_handle_t handle) {
    // TODO: Add spring bones debug output
    if (handle.id >= g_spring_bones_ctx.count) {
        printf("Spring Bones: Invalid handle %u\n", handle.id);
        return;
    }
    
    pthread_rwlock_rdlock(&g_spring_bones_ctx.data_rwlock);
    
    const animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[handle.id];
    printf("Spring Bones Debug Info:\n");
    printf("  ID: %u\n", item->id);
    printf("  Flags: 0x%x\n", item->flags);
    printf("  Initialized: %s\n", item->initialized ? "Yes" : "No");
    printf("  Dirty: %s\n", item->dirty ? "Yes" : "No");
    printf("  Frame Updated: %u\n", item->frame_updated);
    printf("  Data Size: %zu bytes\n", item->data_size);
    printf("  Stiffness: %.2f\n", item->stiffness);
    printf("  Damping: %.2f\n", item->damping);
    printf("  Mass: %.2f\n", item->mass);
    printf("  Rest Length: %.2f\n", item->rest_length);
    printf("  Current Length: %.2f\n", item->current_length);
    printf("  Position: (%.3f, %.3f, %.3f)\n", item->position[0], item->position[1], item->position[2]);
    printf("  Velocity: (%.3f, %.3f, %.3f)\n", item->velocity[0], item->velocity[1], item->velocity[2]);
    printf("  Force: (%.3f, %.3f, %.3f)\n", item->force[0], item->force[1], item->force[2]);
    printf("  LOD Level: %u\n", item->lod_level);
    printf("  Visible: %s\n", item->is_visible ? "Yes" : "No");
    printf("  Distance from Viewer: %.2f\n", item->distance_from_viewer);
    printf("  Cache ID: %u\n", item->cache_id);
    printf("  Streamed: %s\n", item->is_streamed ? "Yes" : "No");
    printf("  GPU Processed: %s\n", item->gpu_processed ? "Yes" : "No");
    printf("  GPU Buffer Offset: %u\n", item->gpu_buffer_offset);
    printf("  Last Error: %s\n", animation_spring_bones_get_error_string(item->last_error));
    if (item->error_message[0] != '\0') {
        printf("  Error Message: %s\n", item->error_message);
    }
    
    pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
    
    // Print global statistics
    pthread_mutex_lock(&g_spring_bones_ctx.context_mutex);
    printf("\nSpring Bones Global Statistics:\n");
    printf("  Total Items: %u\n", g_spring_bones_ctx.count);
    printf("  Capacity: %u\n", g_spring_bones_ctx.capacity);
    printf("  Current Memory Usage: %zu bytes\n", g_spring_bones_ctx.current_memory_usage);
    printf("  Peak Memory Usage: %zu bytes\n", g_spring_bones_ctx.performance.peak_memory_usage);
    printf("  Total Updates: %u\n", g_spring_bones_ctx.performance.total_updates);
    printf("  Total Update Time: %.3f ms\n", g_spring_bones_ctx.performance.total_update_time * 1000.0);
    printf("  Average Update Time: %.3f ms\n", 
           g_spring_bones_ctx.performance.total_updates > 0 ? 
           (g_spring_bones_ctx.performance.total_update_time * 1000.0) / g_spring_bones_ctx.performance.total_updates : 0.0);
    printf("  SIMD Operations: %u\n", g_spring_bones_ctx.performance.total_simd_operations);
    printf("  GPU Operations: %u\n", g_spring_bones_ctx.performance.total_gpu_operations);
    printf("  Cache Hits: %u\n", g_spring_bones_ctx.performance.cache_hits);
    printf("  Cache Misses: %u\n", g_spring_bones_ctx.performance.cache_misses);
    printf("  Compression Operations: %u\n", g_spring_bones_ctx.performance.compression_operations);
    printf("  Compression Ratio: %.2f\n", g_spring_bones_ctx.compression_ratio);
    printf("  Async Operations: %u\n", g_spring_bones_ctx.performance.async_operations);
    printf("  Memory Allocations: %u\n", g_spring_bones_ctx.performance.total_memory_allocations);
    printf("  Memory Deallocations: %u\n", g_spring_bones_ctx.performance.total_memory_deallocations);
    printf("  Current LOD: %u\n", g_spring_bones_ctx.lod_config.current_lod);
    printf("  Visible Bones: %u\n", g_spring_bones_ctx.culling_context.visible_bones);
    printf("  Culled Bones: %u\n", g_spring_bones_ctx.culling_context.culled_bones);
    printf("  Batch Processing Active: %s\n", g_spring_bones_ctx.batch_context.batch_processing_active ? "Yes" : "No");
    printf("  Render Graph Nodes: %u\n", g_spring_bones_ctx.render_graph_node_count);
    printf("  Streaming Enabled: %s\n", g_spring_bones_ctx.streaming_context.streaming_enabled ? "Yes" : "No");
    printf("  File Watching Active: %s\n", g_spring_bones_ctx.file_watcher.watching_active ? "Yes" : "No");
    printf("  GPU Available: %s\n", g_spring_bones_ctx.gpu_context.gpu_available ? "Yes" : "No");
    printf("  AVX2 Supported: %s\n", g_spring_bones_ctx.simd_context.avx2_supported ? "Yes" : "No");
    printf("  SSE Supported: %s\n", g_spring_bones_ctx.simd_context.sse_supported ? "Yes" : "No");
    
    pthread_mutex_unlock(&g_spring_bones_ctx.context_mutex);
}

// Serialization functions
int animation_spring_bones_serialize(animation_spring_bones_handle_t handle, const char* filename) {
    if (!filename) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Null filename");
        return -1;
    }
    
    if (handle.id >= g_spring_bones_ctx.count) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_HANDLE, "Invalid handle");
        return -2;
    }
    
    pthread_rwlock_rdlock(&g_spring_bones_ctx.data_rwlock);
    
    const animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Item not initialized");
        return -3;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_IO_ERROR, "Failed to open file for writing");
        return -4;
    }
    
    // Write magic number and version
    uint32_t magic = 0x53424F4E; // "SBON"
    uint32_t version = 1;
    fwrite(&magic, sizeof(magic), 1, file);
    fwrite(&version, sizeof(version), 1, file);
    
    // Write item data
    fwrite(item, sizeof(animation_spring_bones_internal_t), 1, file);
    
    // Write data payload if present
    if (item->data && item->data_size > 0) {
        fwrite(&item->data_size, sizeof(item->data_size), 1, file);
        fwrite(item->data, item->data_size, 1, file);
    }
    
    // Calculate and write checksum
    uint32_t checksum = animation_spring_bones_calculate_checksum(item);
    fwrite(&checksum, sizeof(checksum), 1, file);
    
    fclose(file);
    pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
    
    g_spring_bones_ctx.performance.serialization_operations++;
    return 0;
}

int animation_spring_bones_deserialize(animation_spring_bones_handle_t* out_handle, const char* filename) {
    if (!out_handle || !filename) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Null handle or filename");
        return -1;
    }
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_IO_ERROR, "Failed to open file for reading");
        return -2;
    }
    
    // Read and verify magic number and version
    uint32_t magic, version;
    fread(&magic, sizeof(magic), 1, file);
    fread(&version, sizeof(version), 1, file);
    
    if (magic != 0x53424F4E) {
        fclose(file);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_CORRUPTION_DETECTED, "Invalid magic number");
        return -3;
    }
    
    if (version != 1) {
        fclose(file);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_VERSION_MISMATCH, "Unsupported version");
        return -4;
    }
    
    // Create new item
    animation_spring_bones_desc_t desc = {0};
    int result = animation_spring_bones_create(out_handle, &desc);
    if (result != 0) {
        fclose(file);
        return result;
    }
    
    pthread_rwlock_wrlock(&g_spring_bones_ctx.data_rwlock);
    
    animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[out_handle->id];
    
    // Read item data
    fread(item, sizeof(animation_spring_bones_internal_t), 1, file);
    
    // Read data payload if present
    size_t data_size;
    if (fread(&data_size, sizeof(data_size), 1, file) == 1 && data_size > 0) {
        item->data = malloc(data_size);
        if (!item->data) {
            pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
            fclose(file);
            animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_OUT_OF_MEMORY, "Failed to allocate data");
            return -5;
        }
        fread(item->data, data_size, 1, file);
        item->data_size = data_size;
    }
    
    // Verify checksum
    uint32_t checksum;
    fread(&checksum, sizeof(checksum), 1, file);
    uint32_t calculated_checksum = animation_spring_bones_calculate_checksum(item);
    
    if (checksum != calculated_checksum) {
        pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
        fclose(file);
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_CORRUPTION_DETECTED, "Checksum mismatch");
        return -6;
    }
    
    fclose(file);
    pthread_rwlock_unlock(&g_spring_bones_ctx.data_rwlock);
    
    g_spring_bones_ctx.performance.serialization_operations++;
    return 0;
}

// Hot-reload functions
int animation_spring_bones_start_hot_reload(const char* directory) {
    if (!directory) {
        animation_spring_bones_set_error(ANIMATION_SPRING_BONES_ERROR_INVALID_PARAM, "Null directory");
        return -1;
    }
    
    return animation_spring_bones_start_file_watching(directory);
}

void animation_spring_bones_stop_hot_reload(void) {
    animation_spring_bones_stop_file_watching();
}

int animation_spring_bones_set_hot_reload_callback(animation_spring_bones_hot_reload_callback_t callback) {
    pthread_mutex_lock(&g_spring_bones_ctx.context_mutex);
    g_spring_bones_ctx.file_watcher.hot_reload_callback = callback;
    pthread_mutex_unlock(&g_spring_bones_ctx.context_mutex);
    return 0;
}

/* End of spring_bones.c */
