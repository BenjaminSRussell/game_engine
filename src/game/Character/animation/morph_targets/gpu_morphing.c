/*
 * gpu_morphing.c
 * GPU morph target blending
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
 * TODO: Implement gpu morphing initialization
 * TODO: Add gpu morphing cleanup/shutdown
 * TODO: Implement gpu morphing validation
 * TODO: Add gpu morphing error handling
 * TODO: Implement gpu morphing serialization
 * TODO: Add gpu morphing debug output
 * TODO: Implement gpu morphing unit tests
 * TODO: Add gpu morphing performance counters
 * TODO: Implement gpu morphing hot-reload
 * TODO: Add gpu morphing thread safety
 * TODO: Implement gpu morphing memory pooling
 * TODO: Add gpu morphing caching layer
 * TODO: Implement gpu morphing async operations
 * TODO: Add gpu morphing GPU integration
 * TODO: Implement gpu morphing SIMD optimization
 * TODO: Add gpu morphing batch processing
 * TODO: Implement gpu morphing streaming support
 * TODO: Add gpu morphing LOD support
 * TODO: Implement gpu morphing culling integration
 * TODO: Add gpu morphing render graph node
 */

#include "character/animation/morph_targets/gpu_morphing.h"
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
#include <sys/time.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_GPU_MORPHING_MAX_COUNT 4096
#define ANIMATION_GPU_MORPHING_DEFAULT_CAPACITY 256
#define ANIMATION_GPU_MORPHING_ALIGNMENT 16
#define ANIMATION_GPU_MORPHING_CACHE_SIZE 1024
#define ANIMATION_GPU_MORPHING_MAX_ASYNC_OPERATIONS 64
#define ANIMATION_GPU_MORPHING_MEMORY_POOL_SIZE (32 * 1024 * 1024) // 32MB
#define ANIMATION_GPU_MORPHING_MAGIC_NUMBER 0x47504D50 // 'GPMP'
#define ANIMATION_GPU_MORPHING_SERIALIZATION_VERSION 1
#define ANIMATION_GPU_MORPHING_SIMD_WIDTH 16
#define ANIMATION_GPU_MORPHING_BATCH_SIZE 64
#define ANIMATION_GPU_MORPHING_LOD_LEVELS 4
#define ANIMATION_GPU_MORPHING_GPU_BUFFER_SIZE (16 * 1024 * 1024) // 16MB

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_gpu_morphing_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    // SIMD optimization data
    __m128* simd_data;
    size_t simd_count;
    
    // Batch processing data
    uint32_t batch_id;
    bool batch_processed;
    
    // Streaming support
    uint32_t streaming_offset;
    uint32_t streaming_size;
    bool streaming_active;
    
    // LOD support
    uint32_t current_lod;
    float lod_distance;
    uint32_t lod_levels[ANIMATION_GPU_MORPHING_MAX_LOD_LEVELS];
    
    // Culling integration
    bool culled;
    float bounding_sphere_radius;
    
    // Render graph node
    uint32_t render_graph_node_id;
    bool render_graph_active;
} animation_gpu_morphing_internal_t;

// SIMD optimization context
typedef struct animation_gpu_morphing_simd_context {
    __m128* workspace;
    size_t workspace_size;
    bool simd_enabled;
    uint32_t simd_width;
} animation_gpu_morphing_simd_context_t;

// Batch processing context
typedef struct animation_gpu_morphing_batch_context {
    animation_gpu_morphing_handle_t items[ANIMATION_GPU_MORPHING_MAX_BATCH_SIZE];
    uint32_t batch_count;
    pthread_mutex_t batch_mutex;
    bool batch_processing_enabled;
} animation_gpu_morphing_batch_context_t;

// Streaming context
typedef struct animation_gpu_morphing_streaming_context {
    uint8_t* streaming_buffer;
    size_t streaming_buffer_size;
    uint32_t streaming_chunks_loaded;
    pthread_mutex_t streaming_mutex;
    bool streaming_enabled;
} animation_gpu_morphing_streaming_context_t;

// LOD context
typedef struct animation_gpu_morphing_lod_context {
    float lod_thresholds[ANIMATION_GPU_MORPHING_MAX_LOD_LEVELS];
    uint32_t lod_count;
    bool lod_enabled;
    float current_camera_distance;
} animation_gpu_morphing_lod_context_t;

// Culling context
typedef struct animation_gpu_morphing_culling_context {
    uint32_t culled_count;
    uint32_t visible_count;
    bool culling_enabled;
    pthread_mutex_t culling_mutex;
} animation_gpu_morphing_culling_context_t;

// Render graph node context
typedef struct animation_gpu_morphing_render_graph_context {
    uint32_t node_count;
    uint32_t active_nodes;
    bool render_graph_enabled;
    pthread_mutex_t render_graph_mutex;
} animation_gpu_morphing_render_graph_context_t;

// Performance counters
typedef struct animation_gpu_morphing_performance_counters {
    uint64_t total_operations;
    uint64_t simd_operations;
    uint64_t batch_operations;
    uint64_t streaming_operations;
    uint64_t lod_transitions;
    uint64_t culling_tests;
    uint64_t render_graph_executions;
    double total_time;
    double simd_time;
    double batch_time;
    double streaming_time;
} animation_gpu_morphing_performance_counters_t;

typedef struct animation_gpu_morphing_context {
    animation_gpu_morphing_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    // Advanced subsystems
    animation_gpu_morphing_simd_context_t simd_context;
    animation_gpu_morphing_batch_context_t batch_context;
    animation_gpu_morphing_streaming_context_t streaming_context;
    animation_gpu_morphing_lod_context_t lod_context;
    animation_gpu_morphing_culling_context_t culling_context;
    animation_gpu_morphing_render_graph_context_t render_graph_context;
    animation_gpu_morphing_performance_counters_t performance_counters;
    
    // Thread safety
    pthread_mutex_t global_mutex;
    
    // Memory tracking
    size_t peak_memory_usage;
    size_t total_memory_allocated;
    
} animation_gpu_morphing_context_t;

static animation_gpu_morphing_context_t g_gpu_morphing_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool animation_gpu_morphing_validate(const animation_gpu_morphing_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data && item->data_size > 0) return false;
    return true;
}

static void animation_gpu_morphing_cleanup_internal(animation_gpu_morphing_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    if (item->simd_data) {
        _mm_free(item->simd_data);
        item->simd_data = NULL;
    }
    item->initialized = false;
}

static double animation_gpu_morphing_get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

static void animation_gpu_morphing_init_simd_context(animation_gpu_morphing_simd_context_t* ctx) {
    ctx->workspace_size = 4096;
    ctx->workspace = _mm_malloc(ctx->workspace_size * sizeof(__m128), 16);
    ctx->simd_enabled = (ctx->workspace != NULL);
    ctx->simd_width = sizeof(__m128) / sizeof(float);
}

static void animation_gpu_morphing_cleanup_simd_context(animation_gpu_morphing_simd_context_t* ctx) {
    if (ctx->workspace) {
        _mm_free(ctx->workspace);
        ctx->workspace = NULL;
    }
    ctx->simd_enabled = false;
}

static void animation_gpu_morphing_init_batch_context(animation_gpu_morphing_batch_context_t* ctx) {
    pthread_mutex_init(&ctx->batch_mutex, NULL);
    ctx->batch_count = 0;
    ctx->batch_processing_enabled = true;
}

static void animation_gpu_morphing_cleanup_batch_context(animation_gpu_morphing_batch_context_t* ctx) {
    pthread_mutex_destroy(&ctx->batch_mutex);
}

static void animation_gpu_morphing_init_streaming_context(animation_gpu_morphing_streaming_context_t* ctx) {
    ctx->streaming_buffer_size = ANIMATION_GPU_MORPHING_STREAMING_CHUNK_SIZE * 16;
    ctx->streaming_buffer = malloc(ctx->streaming_buffer_size);
    ctx->streaming_chunks_loaded = 0;
    pthread_mutex_init(&ctx->streaming_mutex, NULL);
    ctx->streaming_enabled = (ctx->streaming_buffer != NULL);
}

static void animation_gpu_morphing_cleanup_streaming_context(animation_gpu_morphing_streaming_context_t* ctx) {
    if (ctx->streaming_buffer) {
        free(ctx->streaming_buffer);
        ctx->streaming_buffer = NULL;
    }
    pthread_mutex_destroy(&ctx->streaming_mutex);
}

static void animation_gpu_morphing_init_lod_context(animation_gpu_morphing_lod_context_t* ctx) {
    ctx->lod_count = 4;
    ctx->lod_thresholds[0] = 5.0f;
    ctx->lod_thresholds[1] = 15.0f;
    ctx->lod_thresholds[2] = 30.0f;
    ctx->lod_thresholds[3] = 50.0f;
    ctx->lod_enabled = true;
    ctx->current_camera_distance = 0.0f;
}

static void animation_gpu_morphing_init_culling_context(animation_gpu_morphing_culling_context_t* ctx) {
    pthread_mutex_init(&ctx->culling_mutex, NULL);
    ctx->culled_count = 0;
    ctx->visible_count = 0;
    ctx->culling_enabled = true;
}

static void animation_gpu_morphing_cleanup_culling_context(animation_gpu_morphing_culling_context_t* ctx) {
    pthread_mutex_destroy(&ctx->culling_mutex);
}

static void animation_gpu_morphing_init_render_graph_context(animation_gpu_morphing_render_graph_context_t* ctx) {
    pthread_mutex_init(&ctx->render_graph_mutex, NULL);
    ctx->node_count = 0;
    ctx->active_nodes = 0;
    ctx->render_graph_enabled = true;
}

static void animation_gpu_morphing_cleanup_render_graph_context(animation_gpu_morphing_render_graph_context_t* ctx) {
    pthread_mutex_destroy(&ctx->render_graph_mutex);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_gpu_morphing_init(void) {
    if (g_gpu_morphing_ctx.initialized) {
        return 0; // Already initialized
    }

    // Initialize main context
    g_gpu_morphing_ctx.capacity = ANIMATION_GPU_MORPHING_DEFAULT_CAPACITY;
    g_gpu_morphing_ctx.items = calloc(g_gpu_morphing_ctx.capacity, sizeof(animation_gpu_morphing_internal_t));
    if (!g_gpu_morphing_ctx.items) {
        return -1;
    }

    // Initialize thread safety
    if (pthread_mutex_init(&g_gpu_morphing_ctx.global_mutex, NULL) != 0) {
        free(g_gpu_morphing_ctx.items);
        return -2;
    }

    // Initialize subsystems
    animation_gpu_morphing_init_simd_context(&g_gpu_morphing_ctx.simd_context);
    animation_gpu_morphing_init_batch_context(&g_gpu_morphing_ctx.batch_context);
    animation_gpu_morphing_init_streaming_context(&g_gpu_morphing_ctx.streaming_context);
    animation_gpu_morphing_init_lod_context(&g_gpu_morphing_ctx.lod_context);
    animation_gpu_morphing_init_culling_context(&g_gpu_morphing_ctx.culling_context);
    animation_gpu_morphing_init_render_graph_context(&g_gpu_morphing_ctx.render_graph_context);

    // Initialize performance counters
    memset(&g_gpu_morphing_ctx.performance_counters, 0, sizeof(animation_gpu_morphing_performance_counters_t));

    g_gpu_morphing_ctx.count = 0;
    g_gpu_morphing_ctx.peak_memory_usage = 0;
    g_gpu_morphing_ctx.total_memory_allocated = 0;
    g_gpu_morphing_ctx.initialized = true;

    return 0;
}

void animation_gpu_morphing_shutdown(void) {
    if (!g_gpu_morphing_ctx.initialized) {
        return;
    }

    // Cleanup all items
    for (uint32_t i = 0; i < g_gpu_morphing_ctx.count; i++) {
        animation_gpu_morphing_cleanup_internal(&g_gpu_morphing_ctx.items[i]);
    }

    // Cleanup subsystems
    animation_gpu_morphing_cleanup_simd_context(&g_gpu_morphing_ctx.simd_context);
    animation_gpu_morphing_cleanup_batch_context(&g_gpu_morphing_ctx.batch_context);
    animation_gpu_morphing_cleanup_streaming_context(&g_gpu_morphing_ctx.streaming_context);
    animation_gpu_morphing_cleanup_culling_context(&g_gpu_morphing_ctx.culling_context);
    animation_gpu_morphing_cleanup_render_graph_context(&g_gpu_morphing_ctx.render_graph_context);

    // Cleanup thread safety
    pthread_mutex_destroy(&g_gpu_morphing_ctx.global_mutex);

    // Cleanup main context
    free(g_gpu_morphing_ctx.items);
    g_gpu_morphing_ctx.items = NULL;
    g_gpu_morphing_ctx.count = 0;
    g_gpu_morphing_ctx.capacity = 0;
    g_gpu_morphing_ctx.initialized = false;
}

int animation_gpu_morphing_create(animation_gpu_morphing_handle_t* out_handle, const animation_gpu_morphing_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    pthread_mutex_lock(&g_gpu_morphing_ctx.global_mutex);

    if (!g_gpu_morphing_ctx.initialized) {
        pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
        return -2;
    }

    if (g_gpu_morphing_ctx.count >= g_gpu_morphing_ctx.capacity) {
        pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
        return -3;
    }

    uint32_t index = g_gpu_morphing_ctx.count++;
    animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[index];

    // Initialize basic fields
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    // Initialize SIMD data
    item->simd_data = NULL;
    item->simd_count = 0;

    // Initialize batch processing
    item->batch_id = 0;
    item->batch_processed = false;

    // Initialize streaming
    item->streaming_offset = 0;
    item->streaming_size = 0;
    item->streaming_active = false;

    // Initialize LOD
    item->current_lod = 0;
    item->lod_distance = 0.0f;
    memset(item->lod_levels, 0, sizeof(item->lod_levels));

    // Initialize culling
    item->culled = false;
    item->bounding_sphere_radius = 1.0f;

    // Initialize render graph
    item->render_graph_node_id = 0;
    item->render_graph_active = false;

    // Update memory tracking
    g_gpu_morphing_ctx.total_memory_allocated += sizeof(animation_gpu_morphing_internal_t);
    if (g_gpu_morphing_ctx.total_memory_allocated > g_gpu_morphing_ctx.peak_memory_usage) {
        g_gpu_morphing_ctx.peak_memory_usage = g_gpu_morphing_ctx.total_memory_allocated;
    }

    out_handle->id = index;
    pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
    return 0;
}

void animation_gpu_morphing_destroy(animation_gpu_morphing_handle_t handle) {
    pthread_mutex_lock(&g_gpu_morphing_ctx.global_mutex);

    if (handle.id >= g_gpu_morphing_ctx.count) {
        pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
        return;
    }

    animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[handle.id];
    
    // Update memory tracking
    g_gpu_morphing_ctx.total_memory_allocated -= item->data_size;
    
    animation_gpu_morphing_cleanup_internal(item);
    pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
}

int animation_gpu_morphing_update(animation_gpu_morphing_handle_t handle, const void* data, size_t size) {
    pthread_mutex_lock(&g_gpu_morphing_ctx.global_mutex);

    if (handle.id >= g_gpu_morphing_ctx.count) {
        pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
        return -1;
    }

    animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
        return -2;
    }

    double start_time = animation_gpu_morphing_get_time();

    // Update memory tracking
    g_gpu_morphing_ctx.total_memory_allocated -= item->data_size;
    if (item->data) {
        free(item->data);
    }

    // Allocate and copy new data
    item->data = malloc(size);
    if (!item->data) {
        pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
        return -3;
    }
    memcpy(item->data, data, size);
    item->data_size = size;
    g_gpu_morphing_ctx.total_memory_allocated += size;
    
    if (g_gpu_morphing_ctx.total_memory_allocated > g_gpu_morphing_ctx.peak_memory_usage) {
        g_gpu_morphing_ctx.peak_memory_usage = g_gpu_morphing_ctx.total_memory_allocated;
    }

    // SIMD optimization
    if (g_gpu_morphing_ctx.simd_context.simd_enabled && size >= 16) {
        size_t simd_size = size / sizeof(__m128);
        if (item->simd_data) {
            _mm_free(item->simd_data);
        }
        item->simd_data = _mm_malloc(simd_size * sizeof(__m128), 16);
        if (item->simd_data) {
            memcpy(item->simd_data, data, simd_size * sizeof(__m128));
            item->simd_count = simd_size;
            g_gpu_morphing_ctx.performance_counters.simd_operations++;
        }
    }

    // GPU integration (placeholder for actual GPU buffer update)
    // This would typically involve updating GPU vertex buffers
    // with the new morph target data

    // Update streaming if active
    if (item->streaming_active) {
        pthread_mutex_lock(&g_gpu_morphing_ctx.streaming_context.streaming_mutex);
        // Update streaming buffer with new data
        if (item->streaming_offset + size <= g_gpu_morphing_ctx.streaming_context.streaming_buffer_size) {
            memcpy(g_gpu_morphing_ctx.streaming_context.streaming_buffer + item->streaming_offset, 
                   data, size);
            g_gpu_morphing_ctx.performance_counters.streaming_operations++;
        }
        pthread_mutex_unlock(&g_gpu_morphing_ctx.streaming_context.streaming_mutex);
    }

    // Update LOD based on distance (simplified)
    if (g_gpu_morphing_ctx.lod_context.lod_enabled) {
        float distance = item->lod_distance;
        uint32_t new_lod = 0;
        for (uint32_t i = 0; i < g_gpu_morphing_ctx.lod_context.lod_count; i++) {
            if (distance > g_gpu_morphing_ctx.lod_context.lod_thresholds[i]) {
                new_lod = i + 1;
            }
        }
        if (new_lod != item->current_lod) {
            item->current_lod = new_lod;
            g_gpu_morphing_ctx.performance_counters.lod_transitions++;
        }
    }

    // Culling test
    if (g_gpu_morphing_ctx.culling_context.culling_enabled) {
        pthread_mutex_lock(&g_gpu_morphing_ctx.culling_context.culling_mutex);
        // Simple distance-based culling test
        bool was_culled = item->culled;
        item->culled = (item->lod_distance > 100.0f); // Simple culling threshold
        
        if (was_culled != item->culled) {
            if (item->culled) {
                g_gpu_morphing_ctx.culling_context.culled_count++;
                g_gpu_morphing_ctx.culling_context.visible_count--;
            } else {
                g_gpu_morphing_ctx.culling_context.culled_count--;
                g_gpu_morphing_ctx.culling_context.visible_count++;
            }
        }
        g_gpu_morphing_ctx.performance_counters.culling_tests++;
        pthread_mutex_unlock(&g_gpu_morphing_ctx.culling_context.culling_mutex);
    }

    // Render graph update
    if (item->render_graph_active) {
        pthread_mutex_lock(&g_gpu_morphing_ctx.render_graph_context.render_graph_mutex);
        // Mark render graph node as needing update
        g_gpu_morphing_ctx.performance_counters.render_graph_executions++;
        pthread_mutex_unlock(&g_gpu_morphing_ctx.render_graph_context.render_graph_mutex);
    }

    item->dirty = true;
    item->frame_updated++;
    
    double end_time = animation_gpu_morphing_get_time();
    g_gpu_morphing_ctx.performance_counters.total_operations++;
    g_gpu_morphing_ctx.performance_counters.total_time += (end_time - start_time);

    pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
    return 0;
}

bool animation_gpu_morphing_is_valid(animation_gpu_morphing_handle_t handle) {
    pthread_mutex_lock(&g_gpu_morphing_ctx.global_mutex);
    
    bool valid = false;
    if (handle.id < g_gpu_morphing_ctx.count) {
        valid = g_gpu_morphing_ctx.items[handle.id].initialized;
    }
    
    pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
    return valid;
}

int animation_gpu_morphing_get_info(animation_gpu_morphing_handle_t handle, animation_gpu_morphing_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    pthread_mutex_lock(&g_gpu_morphing_ctx.global_mutex);

    if (handle.id >= g_gpu_morphing_ctx.count) {
        pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
        return -2;
    }

    const animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
    return 0;
}

void animation_gpu_morphing_mark_dirty(animation_gpu_morphing_handle_t handle) {
    pthread_mutex_lock(&g_gpu_morphing_ctx.global_mutex);
    
    if (handle.id < g_gpu_morphing_ctx.count) {
        animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[handle.id];
        item->dirty = true;
        
        // Update culling if enabled
        if (g_gpu_morphing_ctx.culling_context.culling_enabled) {
            pthread_mutex_lock(&g_gpu_morphing_ctx.culling_context.culling_mutex);
            // Re-evaluate culling status
            bool was_culled = item->culled;
            item->culled = (item->lod_distance > 100.0f);
            
            if (was_culled != item->culled) {
                if (item->culled) {
                    g_gpu_morphing_ctx.culling_context.culled_count++;
                    g_gpu_morphing_ctx.culling_context.visible_count--;
                } else {
                    g_gpu_morphing_ctx.culling_context.culled_count--;
                    g_gpu_morphing_ctx.culling_context.visible_count++;
                }
            }
            pthread_mutex_unlock(&g_gpu_morphing_ctx.culling_context.culling_mutex);
        }
    }
    
    pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
}

int animation_gpu_morphing_process_pending(void) {
    pthread_mutex_lock(&g_gpu_morphing_ctx.global_mutex);
    
    int processed = 0;
    double start_time = animation_gpu_morphing_get_time();
    
    // Batch processing
    if (g_gpu_morphing_ctx.batch_context.batch_processing_enabled) {
        pthread_mutex_lock(&g_gpu_morphing_ctx.batch_context.batch_mutex);
        
        // Collect dirty items for batch processing
        uint32_t batch_items[ANIMATION_GPU_MORPHING_MAX_BATCH_SIZE];
        uint32_t batch_count = 0;
        
        for (uint32_t i = 0; i < g_gpu_morphing_ctx.count && batch_count < ANIMATION_GPU_MORPHING_MAX_BATCH_SIZE; i++) {
            animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[i];
            if (item->initialized && item->dirty && !item->culled) {
                batch_items[batch_count++] = i;
            }
        }
        
        // Process batch
        if (batch_count > 0) {
            // SIMD batch processing
            if (g_gpu_morphing_ctx.simd_context.simd_enabled) {
                for (uint32_t i = 0; i < batch_count; i++) {
                    animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[batch_items[i]];
                    if (item->simd_data && item->simd_count > 0) {
                        // Process SIMD data
                        for (size_t j = 0; j < item->simd_count; j++) {
                            // Example SIMD operation: apply morph weights
                            __m128 morph_data = item->simd_data[j];
                            __m128 weight = _mm_set1_ps(1.0f); // Example weight
                            item->simd_data[j] = _mm_mul_ps(morph_data, weight);
                        }
                        g_gpu_morphing_ctx.performance_counters.simd_operations++;
                    }
                }
            }
            
            // Mark items as processed
            for (uint32_t i = 0; i < batch_count; i++) {
                animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[batch_items[i]];
                item->dirty = false;
                item->batch_processed = true;
                item->batch_id = g_gpu_morphing_ctx.performance_counters.batch_operations;
                processed++;
            }
            
            g_gpu_morphing_ctx.performance_counters.batch_operations++;
        }
        
        pthread_mutex_unlock(&g_gpu_morphing_ctx.batch_context.batch_mutex);
    } else {
        // Individual processing
        for (uint32_t i = 0; i < g_gpu_morphing_ctx.count; i++) {
            animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[i];
            if (item->initialized && item->dirty && !item->culled) {
                // Process item
                item->dirty = false;
                processed++;
            }
        }
    }
    
    // Render graph processing
    if (g_gpu_morphing_ctx.render_graph_context.render_graph_enabled) {
        pthread_mutex_lock(&g_gpu_morphing_ctx.render_graph_context.render_graph_mutex);
        
        uint32_t active_nodes = 0;
        for (uint32_t i = 0; i < g_gpu_morphing_ctx.count; i++) {
            animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[i];
            if (item->render_graph_active && !item->culled) {
                active_nodes++;
            }
        }
        g_gpu_morphing_ctx.render_graph_context.active_nodes = active_nodes;
        g_gpu_morphing_ctx.performance_counters.render_graph_executions++;
        
        pthread_mutex_unlock(&g_gpu_morphing_ctx.render_graph_context.render_graph_mutex);
    }
    
    double end_time = animation_gpu_morphing_get_time();
    g_gpu_morphing_ctx.performance_counters.batch_time += (end_time - start_time);
    
    pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
    return processed;
}

uint32_t animation_gpu_morphing_get_count(void) {
    return g_gpu_morphing_ctx.count;
}

size_t animation_gpu_morphing_get_memory_usage(void) {
    pthread_mutex_lock(&g_gpu_morphing_ctx.global_mutex);
    
    size_t total = sizeof(g_gpu_morphing_ctx);
    total += g_gpu_morphing_ctx.capacity * sizeof(animation_gpu_morphing_internal_t);
    
    // Add SIMD memory usage
    if (g_gpu_morphing_ctx.simd_context.workspace) {
        total += g_gpu_morphing_ctx.simd_context.workspace_size * sizeof(__m128);
    }
    
    // Add streaming memory usage
    if (g_gpu_morphing_ctx.streaming_context.streaming_buffer) {
        total += g_gpu_morphing_ctx.streaming_context.streaming_buffer_size;
    }
    
    // Add per-item memory
    for (uint32_t i = 0; i < g_gpu_morphing_ctx.count; i++) {
        const animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[i];
        total += item->data_size;
        if (item->simd_data) {
            total += item->simd_count * sizeof(__m128);
        }
    }
    
    pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
    return total;
}

void animation_gpu_morphing_debug_print(void) {
    pthread_mutex_lock(&g_gpu_morphing_ctx.global_mutex);
    
    printf("=== GPU Morphing System Debug Info ===\n");
    printf("Initialized: %s\n", g_gpu_morphing_ctx.initialized ? "Yes" : "No");
    printf("Count: %u / %u\n", g_gpu_morphing_ctx.count, g_gpu_morphing_ctx.capacity);
    printf("Peak Memory Usage: %zu bytes\n", g_gpu_morphing_ctx.peak_memory_usage);
    printf("Total Memory Allocated: %zu bytes\n", g_gpu_morphing_ctx.total_memory_allocated);
    
    // SIMD context info
    printf("\n--- SIMD Context ---\n");
    printf("SIMD Enabled: %s\n", g_gpu_morphing_ctx.simd_context.simd_enabled ? "Yes" : "No");
    printf("SIMD Width: %u\n", g_gpu_morphing_ctx.simd_context.simd_width);
    printf("Workspace Size: %zu\n", g_gpu_morphing_ctx.simd_context.workspace_size);
    
    // Batch context info
    printf("\n--- Batch Context ---\n");
    printf("Batch Processing Enabled: %s\n", g_gpu_morphing_ctx.batch_context.batch_processing_enabled ? "Yes" : "No");
    printf("Current Batch Count: %u\n", g_gpu_morphing_ctx.batch_context.batch_count);
    
    // Streaming context info
    printf("\n--- Streaming Context ---\n");
    printf("Streaming Enabled: %s\n", g_gpu_morphing_ctx.streaming_context.streaming_enabled ? "Yes" : "No");
    printf("Streaming Buffer Size: %zu bytes\n", g_gpu_morphing_ctx.streaming_context.streaming_buffer_size);
    printf("Chunks Loaded: %u\n", g_gpu_morphing_ctx.streaming_context.streaming_chunks_loaded);
    
    // LOD context info
    printf("\n--- LOD Context ---\n");
    printf("LOD Enabled: %s\n", g_gpu_morphing_ctx.lod_context.lod_enabled ? "Yes" : "No");
    printf("LOD Count: %u\n", g_gpu_morphing_ctx.lod_context.lod_count);
    printf("Camera Distance: %.2f\n", g_gpu_morphing_ctx.lod_context.current_camera_distance);
    for (uint32_t i = 0; i < g_gpu_morphing_ctx.lod_context.lod_count; i++) {
        printf("  LOD %u Threshold: %.2f\n", i, g_gpu_morphing_ctx.lod_context.lod_thresholds[i]);
    }
    
    // Culling context info
    printf("\n--- Culling Context ---\n");
    printf("Culling Enabled: %s\n", g_gpu_morphing_ctx.culling_context.culling_enabled ? "Yes" : "No");
    printf("Culled Count: %u\n", g_gpu_morphing_ctx.culling_context.culled_count);
    printf("Visible Count: %u\n", g_gpu_morphing_ctx.culling_context.visible_count);
    
    // Render graph context info
    printf("\n--- Render Graph Context ---\n");
    printf("Render Graph Enabled: %s\n", g_gpu_morphing_ctx.render_graph_context.render_graph_enabled ? "Yes" : "No");
    printf("Node Count: %u\n", g_gpu_morphing_ctx.render_graph_context.node_count);
    printf("Active Nodes: %u\n", g_gpu_morphing_ctx.render_graph_context.active_nodes);
    
    // Performance counters
    printf("\n--- Performance Counters ---\n");
    printf("Total Operations: %lu\n", g_gpu_morphing_ctx.performance_counters.total_operations);
    printf("SIMD Operations: %lu\n", g_gpu_morphing_ctx.performance_counters.simd_operations);
    printf("Batch Operations: %lu\n", g_gpu_morphing_ctx.performance_counters.batch_operations);
    printf("Streaming Operations: %lu\n", g_gpu_morphing_ctx.performance_counters.streaming_operations);
    printf("LOD Transitions: %lu\n", g_gpu_morphing_ctx.performance_counters.lod_transitions);
    printf("Culling Tests: %lu\n", g_gpu_morphing_ctx.performance_counters.culling_tests);
    printf("Render Graph Executions: %lu\n", g_gpu_morphing_ctx.performance_counters.render_graph_executions);
    printf("Total Time: %.6f seconds\n", g_gpu_morphing_ctx.performance_counters.total_time);
    printf("SIMD Time: %.6f seconds\n", g_gpu_morphing_ctx.performance_counters.simd_time);
    printf("Batch Time: %.6f seconds\n", g_gpu_morphing_ctx.performance_counters.batch_time);
    printf("Streaming Time: %.6f seconds\n", g_gpu_morphing_ctx.performance_counters.streaming_time);
    
    // Per-item info
    printf("\n--- Individual Items ---\n");
    for (uint32_t i = 0; i < g_gpu_morphing_ctx.count; i++) {
        const animation_gpu_morphing_internal_t* item = &g_gpu_morphing_ctx.items[i];
        printf("Item %u: ID=%u, Flags=0x%x, DataSize=%zu, Dirty=%s, LOD=%u, Distance=%.2f, Culled=%s\n",
               i, item->id, item->flags, item->data_size, item->dirty ? "Yes" : "No",
               item->current_lod, item->lod_distance, item->culled ? "Yes" : "No");
    }
    
    printf("\n=== End GPU Morphing Debug Info ===\n");
    
    pthread_mutex_unlock(&g_gpu_morphing_ctx.global_mutex);
}

/* End of gpu_morphing.c */
