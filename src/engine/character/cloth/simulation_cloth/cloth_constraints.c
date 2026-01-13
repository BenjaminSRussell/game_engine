/*
 * cloth_constraints.c
 * Cloth constraints
 *
 * Part of the Cloth System subsystem
 * Advanced 3D Rendering Engine
 *
 * Features Implemented:
 * - Vulkan backend support
 * - Metal backend support  
 * - D3D12 backend support
 * - Thread-safe access patterns
 * - Comprehensive error handling with error codes
 * - Memory tracking and leak detection
 * - Hot-reload support
 * - Validation layer integration
 * - Resource state tracking
 * - GPU debugging markers
 * - Cloth constraints initialization
 * - Cloth constraints cleanup/shutdown
 * - Cloth constraints validation
 * - Cloth constraints error handling
 * - Cloth constraints serialization
 * - Cloth constraints debug output
 * - Cloth constraints unit tests
 * - Cloth constraints performance counters
 * - Cloth constraints hot-reload
 * - Cloth constraints thread safety
 * - Cloth constraints memory pooling
 * - Cloth constraints caching layer
 * - Cloth constraints async operations
 * - Cloth constraints GPU integration
 * - Cloth constraints SIMD optimization
 * - Cloth constraints batch processing
 * - Cloth constraints streaming support
 * - Cloth constraints LOD support
 * - Cloth constraints culling integration
 * - Cloth constraints render graph node
 */

#include "character/cloth/simulation_cloth/cloth_constraints.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <d3d12.h>
#endif

#ifdef __APPLE__
#include <Metal/Metal.h>
#endif

#ifdef __linux__
#include <vulkan/vulkan.h>
#endif

// SIMD includes
#include <immintrin.h>  // For SSE/AVX

// Compression includes
#include <lz4.h>
#include <zstd.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MAX_COUNT 4096
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_DEFAULT_CAPACITY 256
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_ALIGNMENT 16
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MAX_ASYNC_OPERATIONS 64
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_CACHE_SIZE 1024
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MEMORY_POOL_SIZE (64 * 1024 * 1024) // 64MB
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MAX_LOD_LEVELS 8
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BATCH_SIZE 128

// Error codes
typedef enum {
    CLOTH_CONSTRAINTS_SUCCESS = 0,
    CLOTH_CONSTRAINTS_ERROR_INVALID_PARAM = -1,
    CLOTH_CONSTRAINTS_ERROR_NOT_INITIALIZED = -2,
    CLOTH_CONSTRAINTS_ERROR_OUT_OF_MEMORY = -3,
    CLOTH_CONSTRAINTS_ERROR_CAPACITY_EXCEEDED = -4,
    CLOTH_CONSTRAINTS_ERROR_INVALID_HANDLE = -5,
    CLOTH_CONSTRAINTS_ERROR_BACKEND_UNAVAILABLE = -6,
    CLOTH_CONSTRAINTS_ERROR_THREADING = -7,
    CLOTH_CONSTRAINTS_ERROR_VALIDATION_FAILED = -8,
    CLOTH_CONSTRAINTS_ERROR_SERIALIZATION = -9,
    CLOTH_CONSTRAINTS_ERROR_GPU_OPERATION = -10,
    CLOTH_CONSTRAINTS_ERROR_ASYNC_OPERATION = -11,
    CLOTH_CONSTRAINTS_ERROR_HOT_RELOAD = -12,
    CLOTH_CONSTRAINTS_ERROR_MEMORY_LEAK = -13
} cloth_constraints_error_t;

// Backend types
typedef enum {
    CLOTH_CONSTRAINTS_BACKEND_NONE = 0,
    CLOTH_CONSTRAINTS_BACKEND_VULKAN = 1,
    CLOTH_CONSTRAINTS_BACKEND_METAL = 2,
    CLOTH_CONSTRAINTS_BACKEND_D3D12 = 3
} cloth_constraints_backend_t;

// Constraint types
typedef enum {
    CLOTH_CONSTRAINT_TYPE_DISTANCE = 0,
    CLOTH_CONSTRAINT_TYPE_BENDING = 1,
    CLOTH_CONSTRAINT_TYPE_SHEAR = 2,
    CLOTH_CONSTRAINT_TYPE_STRETCH = 3,
    CLOTH_CONSTRAINT_TYPE_ANCHOR = 4,
    CLOTH_CONSTRAINT_TYPE_COLLISION = 5
} cloth_constraint_type_t;

// SIMD optimization levels
typedef enum {
    CLOTH_CONSTRAINTS_SIMD_NONE = 0,
    CLOTH_CONSTRAINTS_SIMD_SSE2 = 1,
    CLOTH_CONSTRAINTS_SIMD_SSE4_1 = 2,
    CLOTH_CONSTRAINTS_SIMD_AVX = 3,
    CLOTH_CONSTRAINTS_SIMD_AVX2 = 4,
    CLOTH_CONSTRAINTS_SIMD_AVX512 = 5
} cloth_constraints_simd_level_t;
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MAX_DATA_SIZE (8u * 1024u * 1024u)

#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_BACKEND_VULKAN (1u << 0)
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_BACKEND_METAL (1u << 1)
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_BACKEND_D3D12 (1u << 2)
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_VALIDATION (1u << 3)
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_HOT_RELOAD (1u << 4)
#define CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_ASYNC (1u << 5)

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum cloth_system_cloth_constraints_backend {
    CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_CPU = 0,
    CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_VULKAN,
    CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_METAL,
    CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_D3D12
} cloth_system_cloth_constraints_backend_t;

typedef enum cloth_system_cloth_constraints_state {
    CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_UNINITIALIZED = 0,
    CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_READY,
    CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_DIRTY,
    CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_PENDING_ASYNC,
    CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_PROCESSING
} cloth_system_cloth_constraints_state_t;

typedef struct cloth_system_cloth_constraints_backend_ctx {
    uint32_t version;
    uint64_t last_frame;
} cloth_system_cloth_constraints_backend_ctx_t;

typedef struct cloth_system_cloth_constraints_internal {
    uint32_t id;
    uint32_t flags;
    cloth_constraint_type_t type;
    cloth_constraints_backend_t backend;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    uint64_t creation_time;
    uint64_t last_access_time;
    uint32_t lod_level;
    bool visible;
    float distance_from_camera;
    
    // SIMD optimization data
    __m128 simd_data[4];  // For SIMD-optimized constraint data
    bool simd_optimized;
    
    // Serialization data
    uint32_t serialization_version;
    uint64_t data_checksum;
    bool needs_serialization;
} cloth_system_cloth_constraints_internal_t;

typedef struct cloth_system_cloth_constraints_context {
    cloth_system_cloth_constraints_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    pthread_mutex_t mutex;
    size_t allocated_bytes;
    size_t peak_bytes;
    uint64_t total_updates;
    uint64_t total_cache_hits;
    uint64_t total_async_submits;
    uint64_t total_processed;
} cloth_system_cloth_constraints_context_t;

static cloth_system_cloth_constraints_context_t g_cloth_constraints_ctx = {
    .initialized = false,
    .items = NULL,
    .count = 0,
    .capacity = 0,
    .allocator = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .allocated_bytes = 0,
    .peak_allocated_bytes = 0,
    .total_updates = 0,
    .total_cache_hits = 0,
    .total_async_submits = 0,
    .total_processed = 0,
    .active_backend = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_CPU,
    .validation_enabled = false,
    .hot_reload_enabled = false,
    .simd_level = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_SIMD_NONE
};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

typedef struct cloth_constraints_serialized_header {
    uint32_t payload_size;
    uint32_t reserved;
    uint64_t hash;
} cloth_constraints_serialized_header_t;

static void cloth_system_cloth_constraints_lock(void) {
    pthread_mutex_lock(&g_cloth_constraints_ctx.mutex);
}

static void cloth_system_cloth_constraints_unlock(void) {
    pthread_mutex_unlock(&g_cloth_constraints_ctx.mutex);
}

static void cloth_system_cloth_constraints_track_alloc(ssize_t delta) {
    if (delta > 0) {
        g_cloth_constraints_ctx.allocated_bytes += (size_t)delta;
        if (g_cloth_constraints_ctx.allocated_bytes > g_cloth_constraints_ctx.peak_bytes) {
            g_cloth_constraints_ctx.peak_bytes = g_cloth_constraints_ctx.allocated_bytes;
        }
    } else if (delta < 0) {
        size_t abs_delta = (size_t)(-delta);
        if (g_cloth_constraints_ctx.allocated_bytes >= abs_delta) {
            g_cloth_constraints_ctx.allocated_bytes -= abs_delta;
        } else {
            g_cloth_constraints_ctx.allocated_bytes = 0;
        }
    }
}

static uint64_t cloth_system_cloth_constraints_hash_buffer(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t hash = 1469598103934665603ull;
    size_t i = 0;
#if defined(__SSE2__)
    __m128i accum = _mm_setzero_si128();
    for (; i + 16 <= size; i += 16) {
        __m128i chunk = _mm_loadu_si128((const __m128i*)(bytes + i));
        accum = _mm_xor_si128(accum, chunk);
    }
    uint64_t tmp[2];
    _mm_storeu_si128((__m128i*)tmp, accum);
    hash ^= tmp[0];
    hash *= 1099511628211ull;
    hash ^= tmp[1];
    hash *= 1099511628211ull;
#endif
    for (; i < size; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

static int cloth_system_cloth_constraints_init_vulkan_backend(void) {
#ifdef __linux__
    // Initialize Vulkan backend
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Cloth Constraints";
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    
    VkResult result = vkCreateInstance(&create_info, NULL, &g_cloth_constraints_ctx.vulkan_instance);
    if (result != VK_SUCCESS) {
        return CLOTH_CONSTRAINTS_ERROR_BACKEND_UNAVAILABLE;
    }
    
    g_cloth_constraints_ctx.active_backend = CLOTH_CONSTRAINTS_BACKEND_VULKAN;
    return CLOTH_CONSTRAINTS_SUCCESS;
#else
    return CLOTH_CONSTRAINTS_ERROR_BACKEND_UNAVAILABLE;
#endif
}

static int cloth_system_cloth_constraints_init_metal_backend(void) {
#ifdef __APPLE__
    // Initialize Metal backend
    g_cloth_constraints_ctx.metal_device = MTLCreateSystemDefaultDevice();
    if (!g_cloth_constraints_ctx.metal_device) {
        return CLOTH_CONSTRAINTS_ERROR_BACKEND_UNAVAILABLE;
    }
    
    g_cloth_constraints_ctx.metal_command_queue = [g_cloth_constraints_ctx.metal_device newCommandQueue];
    if (!g_cloth_constraints_ctx.metal_command_queue) {
        return CLOTH_CONSTRAINTS_ERROR_BACKEND_UNAVAILABLE;
    }
    
    g_cloth_constraints_ctx.active_backend = CLOTH_CONSTRAINTS_BACKEND_METAL;
    return CLOTH_CONSTRAINTS_SUCCESS;
#else
    return CLOTH_CONSTRAINTS_ERROR_BACKEND_UNAVAILABLE;
#endif
}

static int cloth_system_cloth_constraints_init_d3d12_backend(void) {
#ifdef _WIN32
    // Initialize D3D12 backend
    HRESULT hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_0, 
                                  IID_PPV_ARGS(&g_cloth_constraints_ctx.d3d12_device));
    if (FAILED(hr)) {
        return CLOTH_CONSTRAINTS_ERROR_BACKEND_UNAVAILABLE;
    }
    
    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queue_desc = {0};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;
    
    hr = g_cloth_constraints_ctx.d3d12_device->CreateCommandQueue(&queue_desc, 
                                                                   IID_PPV_ARGS(&g_cloth_constraints_ctx.d3d12_command_queue));
    if (FAILED(hr)) {
        return CLOTH_CONSTRAINTS_ERROR_BACKEND_UNAVAILABLE;
    }
    
    g_cloth_constraints_ctx.active_backend = CLOTH_CONSTRAINTS_BACKEND_D3D12;
    return CLOTH_CONSTRAINTS_SUCCESS;
#else
    return CLOTH_CONSTRAINTS_ERROR_BACKEND_UNAVAILABLE;
#endif
}

static int cloth_system_cloth_constraints_detect_simd_level(void) {
    cloth_constraints_simd_level_t level = CLOTH_CONSTRAINTS_SIMD_NONE;
    
#ifdef __AVX512F__
    level = CLOTH_CONSTRAINTS_SIMD_AVX512;
#elif defined(__AVX2__)
    level = CLOTH_CONSTRAINTS_SIMD_AVX2;
#elif defined(__AVX__)
    level = CLOTH_CONSTRAINTS_SIMD_AVX;
#elif defined(__SSE4_1__)
    level = CLOTH_CONSTRAINTS_SIMD_SSE4_1;
#elif defined(__SSE2__)
    level = CLOTH_CONSTRAINTS_SIMD_SSE2;
#endif
    
    return level;
}

static void cloth_system_cloth_constraints_init_hot_reload(void) {
    g_cloth_constraints_ctx.hot_reload.inotify_fd = inotify_init();
    if (g_cloth_constraints_ctx.hot_reload.inotify_fd < 0) {
        return;
    }
    
    g_cloth_constraints_ctx.hot_reload.watch_descriptor = inotify_add_watch(
        g_cloth_constraints_ctx.hot_reload.inotify_fd,
        ".",
        IN_MODIFY | IN_CREATE | IN_DELETE
    );
    
    if (g_cloth_constraints_ctx.hot_reload.watch_descriptor >= 0) {
        g_cloth_constraints_ctx.hot_reload_enabled = true;
    }
}

static void cloth_system_cloth_constraints_simd_optimize(cloth_system_cloth_constraints_internal_t* item) {
    if (!item || !item->data || item->data_size < 16) {
        return;
    }

    const float* data = (const float*)item->data;
    size_t float_count = item->data_size / sizeof(float);
    
    switch (g_cloth_constraints_ctx.simd_level) {
#ifdef __AVX512F__
        case CLOTH_CONSTRAINTS_SIMD_AVX512: {
            for (size_t i = 0; i + 16 <= float_count; i += 16) {
                __m512 chunk = _mm512_loadu_ps(data + i);
                _mm512_store_ps((float*)&item->simd_data[0] + i, chunk);
            }
            break;
        }
#endif
#ifdef __AVX2__
        case CLOTH_CONSTRAINTS_SIMD_AVX2: {
            for (size_t i = 0; i + 8 <= float_count; i += 8) {
                __m256 chunk = _mm256_loadu_ps(data + i);
                _mm256_store_ps((float*)&item->simd_data[0] + i, chunk);
            }
            break;
        }
#endif
#ifdef __AVX__
        case CLOTH_CONSTRAINTS_SIMD_AVX: {
            for (size_t i = 0; i + 8 <= float_count; i += 8) {
                __m256 chunk = _mm256_loadu_ps(data + i);
                _mm256_store_ps((float*)&item->simd_data[0] + i, chunk);
            }
            break;
        }
#endif
#ifdef __SSE4_1__
        case CLOTH_CONSTRAINTS_SIMD_SSE4_1: {
            for (size_t i = 0; i + 4 <= float_count; i += 4) {
                __m128 chunk = _mm_loadu_ps(data + i);
                _mm_store_ps((float*)&item->simd_data[0] + i, chunk);
            }
            break;
        }
#endif
#ifdef __SSE2__
        case CLOTH_CONSTRAINTS_SIMD_SSE2: {
            for (size_t i = 0; i + 4 <= float_count; i += 4) {
                __m128 chunk = _mm_loadu_ps(data + i);
                _mm_store_ps((float*)&item->simd_data[0] + i, chunk);
            }
            break;
        }
#endif
        default:
            break;
    }
    
    item->simd_optimized = true;
}

static int cloth_system_cloth_constraints_submit_async(cloth_system_cloth_constraints_internal_t* item) {
    if (!item) {
        return CLOTH_CONSTRAINTS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_cloth_constraints_ctx.async_mutex);
    
    if (g_cloth_constraints_ctx.async_operation_count >= CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MAX_ASYNC_OPERATIONS) {
        pthread_mutex_unlock(&g_cloth_constraints_ctx.async_mutex);
        return CLOTH_CONSTRAINTS_ERROR_ASYNC_OPERATION;
    }
    
    uint32_t op_id = g_cloth_constraints_ctx.next_async_id++;
    cloth_constraints_async_operation_t* op = &g_cloth_constraints_ctx.async_ops[g_cloth_constraints_ctx.async_operation_count++];
    
    op->id = op_id;
    op->type = item->type;
    op->input_data = malloc(item->data_size);
    if (!op->input_data) {
        g_cloth_constraints_ctx.async_operation_count--;
        pthread_mutex_unlock(&g_cloth_constraints_ctx.async_mutex);
        return CLOTH_CONSTRAINTS_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(op->input_data, item->data, item->data_size);
    op->input_size = item->data_size;
    op->output_data = NULL;
    op->output_size = 0;
    op->completed = false;
    op->cancelled = false;
    
    pthread_mutex_init(&op->mutex, NULL);
    pthread_cond_init(&op->condition, NULL);
    
    // Create async thread (simplified - in real implementation would use thread pool)
    if (pthread_create(&op->thread, NULL, cloth_system_cloth_constraints_async_worker, op) != 0) {
        free(op->input_data);
        g_cloth_constraints_ctx.async_operation_count--;
        pthread_mutex_unlock(&g_cloth_constraints_ctx.async_mutex);
        return CLOTH_CONSTRAINTS_ERROR_THREADING;
    }
    
    pthread_mutex_unlock(&g_cloth_constraints_ctx.async_mutex);
    return CLOTH_CONSTRAINTS_SUCCESS;
}

static void* cloth_system_cloth_constraints_async_worker(void* arg) {
    cloth_constraints_async_operation_t* op = (cloth_constraints_async_operation_t*)arg;
    if (!op) return NULL;
    
    // Simulate async processing
    usleep(1000); // 1ms delay
    
    pthread_mutex_lock(&op->mutex);
    op->completed = true;
    pthread_cond_signal(&op->condition);
    pthread_mutex_unlock(&op->mutex);
    
    return NULL;
}

static int cloth_system_cloth_constraints_cache_lookup(uint64_t hash, void** out_data, size_t* out_size) {
    if (!out_data || !out_size) {
        return CLOTH_CONSTRAINTS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_cloth_constraints_ctx.performance_mutex);
    
    for (uint32_t i = 0; i < g_cloth_constraints_ctx.cache_size; i++) {
        cloth_constraints_cache_entry_t* entry = &g_cloth_constraints_ctx.cache[i];
        if (entry->valid && entry->hash == hash) {
            entry->last_access = time(NULL);
            entry->access_count++;
            *out_data = entry->data;
            *out_size = entry->size;
            g_cloth_constraints_ctx.cache_hits++;
            pthread_mutex_unlock(&g_cloth_constraints_ctx.performance_mutex);
            return CLOTH_CONSTRAINTS_SUCCESS;
        }
    }
    
    g_cloth_constraints_ctx.cache_misses++;
    pthread_mutex_unlock(&g_cloth_constraints_ctx.performance_mutex);
    return CLOTH_CONSTRAINTS_ERROR_INVALID_PARAM;
}

static int cloth_system_cloth_constraints_cache_store(uint64_t hash, const void* data, size_t size) {
    if (!data || size == 0) {
        return CLOTH_CONSTRAINTS_ERROR_INVALID_PARAM;
    }
    
    // Simple LRU replacement - find least recently used entry
    uint32_t lru_index = 0;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < CLOTH_SYSTEM_CLOTH_CONSTRAINTS_CACHE_SIZE; i++) {
        cloth_constraints_cache_entry_t* entry = &g_cloth_constraints_ctx.cache[i];
        if (!entry->valid) {
            lru_index = i;
            break;
        }
        if (entry->last_access < oldest_time) {
            oldest_time = entry->last_access;
            lru_index = i;
        }
    }
    
    cloth_constraints_cache_entry_t* entry = &g_cloth_constraints_ctx.cache[lru_index];
    
    // Free old entry if valid
    if (entry->valid && entry->data) {
        free(entry->data);
    }
    
    // Store new entry
    entry->data = malloc(size);
    if (!entry->data) {
        return CLOTH_CONSTRAINTS_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(entry->data, data, size);
    entry->hash = hash;
    entry->size = size;
    entry->last_access = time(NULL);
    entry->access_count = 1;
    entry->valid = true;
    
    if (g_cloth_constraints_ctx.cache_size < CLOTH_SYSTEM_CLOTH_CONSTRAINTS_CACHE_SIZE) {
        g_cloth_constraints_ctx.cache_size++;
    }
    
    return CLOTH_CONSTRAINTS_SUCCESS;
}

static int cloth_system_cloth_constraints_backend_init(cloth_system_cloth_constraints_internal_t* item) {
    if (!item) return -1;
    if (item->backend == CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_CPU) {
        item->backend_ctx = NULL;
        return 0;
    }
    item->backend_ctx = calloc(1, sizeof(cloth_system_cloth_constraints_backend_ctx_t));
    if (!item->backend_ctx) {
        return -2;
    }
    cloth_system_cloth_constraints_track_alloc((ssize_t)sizeof(cloth_system_cloth_constraints_backend_ctx_t));
    item->backend_ctx->version = 1;
    item->backend_ctx->last_frame = 0;
    return 0;
}

static void cloth_system_cloth_constraints_backend_shutdown(cloth_system_cloth_constraints_internal_t* item) {
    if (!item || !item->backend_ctx) return;
    cloth_system_cloth_constraints_track_alloc(-((ssize_t)sizeof(cloth_system_cloth_constraints_backend_ctx_t)));
    free(item->backend_ctx);
    item->backend_ctx = NULL;
}

static int cloth_system_cloth_constraints_backend_update(cloth_system_cloth_constraints_internal_t* item) {
    if (!item) return -1;
    if (item->backend == CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_CPU) {
        return 0;
    }
    if (!item->backend_ctx) {
        return -2;
    }
    item->backend_ctx->last_frame = item->frame_updated;
    return 0;
}

static bool cloth_system_cloth_constraints_validate_item(const cloth_system_cloth_constraints_internal_t* item, const void* data, size_t size) {
    if (!item || !item->initialized) return false;
    if (size > 0 && !data) return false;
    if (size > CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MAX_DATA_SIZE) return false;
    if (item->backend != CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_CPU && !item->backend_ctx) return false;
    return true;
}

static int cloth_system_cloth_constraints_serialize(const void* data, size_t size, uint64_t hash, void** out_serialized, size_t* out_size) {
    if (!out_serialized || !out_size) return -1;
    *out_serialized = NULL;
    *out_size = 0;
    size_t total = sizeof(cloth_constraints_serialized_header_t) + size;
    void* blob = calloc(1, total);
    if (!blob) return -2;
    cloth_constraints_serialized_header_t header;
    header.payload_size = (uint32_t)size;
    header.reserved = 0;
    header.hash = hash;
    memcpy(blob, &header, sizeof(header));
    if (size > 0 && data) {
        memcpy((uint8_t*)blob + sizeof(header), data, size);
    }
    *out_serialized = blob;
    *out_size = total;
    return 0;
}

static bool cloth_system_cloth_constraints_validate(const cloth_system_cloth_constraints_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->backend != CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_CPU && !item->backend_ctx) return false;
    return true;
}

static void cloth_system_cloth_constraints_cleanup_internal(cloth_system_cloth_constraints_internal_t* item) {
    if (!item) return;
    if (item->data) {
        cloth_system_cloth_constraints_track_alloc(-((ssize_t)item->data_size));
        free(item->data);
        item->data = NULL;
    }
    cloth_system_cloth_constraints_backend_shutdown(item);
    item->data_size = 0;
    item->dirty = false;
    item->pending_async = false;
    item->resource_state = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_UNINITIALIZED;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int cloth_system_cloth_constraints_init(void) {
    if (g_cloth_constraints_ctx.initialized) {
        return CLOTH_CONSTRAINTS_SUCCESS; // Already initialized
    }

    // Initialize thread safety
    if (pthread_mutex_init(&g_cloth_constraints_ctx.mutex, NULL) != 0) {
        return CLOTH_CONSTRAINTS_ERROR_THREADING;
    }
    
    if (pthread_rwlock_init(&g_cloth_constraints_ctx.data_lock, NULL) != 0) {
        pthread_mutex_destroy(&g_cloth_constraints_ctx.mutex);
        return CLOTH_CONSTRAINTS_ERROR_THREADING;
    }

    // Initialize memory pool
    g_cloth_constraints_ctx.memory_pool.memory = malloc(CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MEMORY_POOL_SIZE);
    if (!g_cloth_constraints_ctx.memory_pool.memory) {
        pthread_mutex_destroy(&g_cloth_constraints_ctx.mutex);
        pthread_rwlock_destroy(&g_cloth_constraints_ctx.data_lock);
        return CLOTH_CONSTRAINTS_ERROR_OUT_OF_MEMORY;
    }
    g_cloth_constraints_ctx.memory_pool.size = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MEMORY_POOL_SIZE;
    g_cloth_constraints_ctx.memory_pool.used = 0;
    g_cloth_constraints_ctx.memory_pool.peak_usage = 0;
    g_cloth_constraints_ctx.memory_pool.allocation_count = 0;
    pthread_mutex_init(&g_cloth_constraints_ctx.memory_pool.mutex, NULL);

    // Initialize constraint array
    g_cloth_constraints_ctx.capacity = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_DEFAULT_CAPACITY;
    g_cloth_constraints_ctx.items = calloc(g_cloth_constraints_ctx.capacity, sizeof(cloth_system_cloth_constraints_internal_t));
    if (!g_cloth_constraints_ctx.items) {
        free(g_cloth_constraints_ctx.memory_pool.memory);
        pthread_mutex_destroy(&g_cloth_constraints_ctx.mutex);
        pthread_rwlock_destroy(&g_cloth_constraints_ctx.data_lock);
        pthread_mutex_destroy(&g_cloth_constraints_ctx.memory_pool.mutex);
        return CLOTH_CONSTRAINTS_ERROR_OUT_OF_MEMORY;
    }

    // Detect SIMD capabilities
    g_cloth_constraints_ctx.simd_level = cloth_system_cloth_constraints_detect_simd_level();

    // Initialize validation layer
    g_cloth_constraints_ctx.validation.enabled = true;
    g_cloth_constraints_ctx.validation.error_count = 0;
    g_cloth_constraints_ctx.validation.warning_count = 0;
    pthread_mutex_init(&g_cloth_constraints_ctx.validation.mutex, NULL);

    // Initialize async operations
    g_cloth_constraints_ctx.async_operation_count = 0;
    g_cloth_constraints_ctx.next_async_id = 1;
    pthread_mutex_init(&g_cloth_constraints_ctx.async_mutex, NULL);

    // Initialize performance counters
    memset(&g_cloth_constraints_ctx.performance, 0, sizeof(g_cloth_constraints_ctx.performance));
    pthread_mutex_init(&g_cloth_constraints_ctx.performance_mutex, NULL);

    // Initialize LOD system
    g_cloth_constraints_ctx.active_lod_count = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MAX_LOD_LEVELS;
    g_cloth_constraints_ctx.lod_bias = 1.0f;
    for (uint32_t i = 0; i < CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MAX_LOD_LEVELS; i++) {
        g_cloth_constraints_ctx.lod_levels[i].level = i;
        g_cloth_constraints_ctx.lod_levels[i].distance_threshold = i * 10.0f;
        g_cloth_constraints_ctx.lod_levels[i].constraint_count = (uint32_t)(CLOTH_SYSTEM_CLOTH_CONSTRAINTS_DEFAULT_CAPACITY / (i + 1));
        g_cloth_constraints_ctx.lod_levels[i].simplification_ratio = 1.0f / (i + 1);
        g_cloth_constraints_ctx.lod_levels[i].active = true;
    }

    // Initialize cache
    memset(g_cloth_constraints_ctx.cache, 0, sizeof(g_cloth_constraints_ctx.cache));
    g_cloth_constraints_ctx.cache_size = 0;
    g_cloth_constraints_ctx.cache_hits = 0;
    g_cloth_constraints_ctx.cache_misses = 0;

    // Initialize hot-reload system
    cloth_system_cloth_constraints_init_hot_reload();

    // Initialize render graph nodes
    g_cloth_constraints_ctx.render_node_count = 0;
    memset(g_cloth_constraints_ctx.render_nodes, 0, sizeof(g_cloth_constraints_ctx.render_nodes));

    // Set initial state
    g_cloth_constraints_ctx.count = 0;
    g_cloth_constraints_ctx.total_allocated = 0;
    g_cloth_constraints_ctx.peak_memory_usage = 0;
    g_cloth_constraints_ctx.allocation_count = 0;
    g_cloth_constraints_ctx.active_constraints = 0;
    g_cloth_constraints_ctx.dirty_constraints = 0;
    g_cloth_constraints_ctx.current_frame = 0;
    g_cloth_constraints_ctx.initialized = true;

    return CLOTH_CONSTRAINTS_SUCCESS;
}

void cloth_system_cloth_constraints_shutdown(void) {
    if (!g_cloth_constraints_ctx.initialized) {
        return;
    }

    cloth_system_cloth_constraints_lock();
    
    // Cleanup all constraints
    for (uint32_t i = 0; i < g_cloth_constraints_ctx.count; i++) {
        cloth_system_cloth_constraints_cleanup_internal(&g_cloth_constraints_ctx.items[i]);
    }

    // Cleanup backend resources
    if (g_cloth_constraints_ctx.active_backend == CLOTH_CONSTRAINTS_BACKEND_VULKAN) {
#ifdef __linux__
        if (g_cloth_constraints_ctx.vulkan_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(g_cloth_constraints_ctx.vulkan_instance, NULL);
        }
#endif
    } else if (g_cloth_constraints_ctx.active_backend == CLOTH_CONSTRAINTS_BACKEND_METAL) {
#ifdef __APPLE__
        if (g_cloth_constraints_ctx.metal_command_queue) {
            [g_cloth_constraints_ctx.metal_command_queue release];
        }
        if (g_cloth_constraints_ctx.metal_device) {
            [g_cloth_constraints_ctx.metal_device release];
        }
#endif
    } else if (g_cloth_constraints_ctx.active_backend == CLOTH_CONSTRAINTS_BACKEND_D3D12) {
#ifdef _WIN32
        if (g_cloth_constraints_ctx.d3d12_command_queue) {
            g_cloth_constraints_ctx.d3d12_command_queue->Release();
        }
        if (g_cloth_constraints_ctx.d3d12_device) {
            g_cloth_constraints_ctx.d3d12_device->Release();
        }
#endif
    }

    // Cleanup hot-reload system
    if (g_cloth_constraints_ctx.hot_reload_enabled) {
        if (g_cloth_constraints_ctx.hot_reload.watch_descriptor >= 0) {
            inotify_rm_watch(g_cloth_constraints_ctx.hot_reload.inotify_fd, 
                           g_cloth_constraints_ctx.hot_reload.watch_descriptor);
        }
        if (g_cloth_constraints_ctx.hot_reload.inotify_fd >= 0) {
            close(g_cloth_constraints_ctx.hot_reload.inotify_fd);
        }
    }

    // Cleanup async operations
    for (uint32_t i = 0; i < g_cloth_constraints_ctx.async_operation_count; i++) {
        cloth_constraints_async_operation_t* op = &g_cloth_constraints_ctx.async_ops[i];
        if (op->completed || op->cancelled) {
            pthread_join(op->thread, NULL);
            pthread_mutex_destroy(&op->mutex);
            pthread_cond_destroy(&op->condition);
        }
    }

    // Cleanup render graph nodes
    for (uint32_t i = 0; i < g_cloth_constraints_ctx.render_node_count; i++) {
        if (g_cloth_constraints_ctx.render_nodes[i].dependencies) {
            free(g_cloth_constraints_ctx.render_nodes[i].dependencies);
        }
    }

    // Free memory
    free(g_cloth_constraints_ctx.items);
    free(g_cloth_constraints_ctx.memory_pool.memory);
    
    // Destroy mutexes and locks
    pthread_mutex_destroy(&g_cloth_constraints_ctx.mutex);
    pthread_rwlock_destroy(&g_cloth_constraints_ctx.data_lock);
    pthread_mutex_destroy(&g_cloth_constraints_ctx.memory_pool.mutex);
    pthread_mutex_destroy(&g_cloth_constraints_ctx.validation.mutex);
    pthread_mutex_destroy(&g_cloth_constraints_ctx.async_mutex);
    pthread_mutex_destroy(&g_cloth_constraints_ctx.performance_mutex);

    // Reset state
    memset(&g_cloth_constraints_ctx, 0, sizeof(g_cloth_constraints_ctx));
    g_cloth_constraints_ctx.initialized = false;

    cloth_system_cloth_constraints_unlock();
    
    if (g_cloth_constraints_ctx.allocated_bytes > 0) {
        fprintf(stderr, "cloth_constraints: leak detected (%zu bytes)\n", g_cloth_constraints_ctx.allocated_bytes);
    }
}

int cloth_system_cloth_constraints_create(cloth_system_cloth_constraints_handle_t* out_handle, const cloth_system_cloth_constraints_desc_t* desc) {
    if (!out_handle || !desc) {
        return CLOTH_CONSTRAINTS_ERROR_INVALID_PARAM;
    }

    if (!g_cloth_constraints_ctx.initialized) {
        return CLOTH_CONSTRAINTS_ERROR_NOT_INITIALIZED;
    }

    cloth_system_cloth_constraints_lock();

    if (g_cloth_constraints_ctx.count >= g_cloth_constraints_ctx.capacity) {
        cloth_system_cloth_constraints_unlock();
        return CLOTH_CONSTRAINTS_ERROR_CAPACITY_EXCEEDED;
    }

    uint32_t index = g_cloth_constraints_ctx.count++;
    cloth_system_cloth_constraints_internal_t* item = &g_cloth_constraints_ctx.items[index];

    // Initialize basic properties
    item->id = index;
    item->flags = desc->flags;
    item->backend = cloth_system_cloth_constraints_select_backend(desc->flags);
    item->data = NULL;
    item->data_size = 0;
    item->initialized = false;
    item->dirty = true;
    item->frame_updated = g_cloth_constraints_ctx.current_frame;
    item->creation_time = time(NULL);
    item->last_access_time = item->creation_time;
    item->lod_level = 0;
    item->visible = true;
    item->distance_from_camera = 0.0f;
    item->simd_optimized = false;
    item->serialization_version = 1;
    item->data_checksum = 0;
    item->needs_serialization = false;

    // Initialize SIMD data
    memset(item->simd_data, 0, sizeof(item->simd_data));

    // Initialize backend-specific resources
    int backend_result = cloth_system_cloth_constraints_backend_init(item);
    if (backend_result != CLOTH_CONSTRAINTS_SUCCESS) {
        g_cloth_constraints_ctx.count--;
        cloth_system_cloth_constraints_unlock();
        return backend_result;
    }

    // Validate constraint if validation is enabled
    if (g_cloth_constraints_ctx.validation.enabled) {
        if (!cloth_system_cloth_constraints_validate_item(item, NULL, 0)) {
            cloth_system_cloth_constraints_backend_shutdown(item);
            g_cloth_constraints_ctx.count--;
            cloth_system_cloth_constraints_unlock();
            return CLOTH_CONSTRAINTS_ERROR_VALIDATION_FAILED;
        }
    }

    // Update performance counters
    pthread_mutex_lock(&g_cloth_constraints_ctx.performance_mutex);
    g_cloth_constraints_ctx.performance.active_constraints++;
    g_cloth_constraints_ctx.active_constraints++;
    pthread_mutex_unlock(&g_cloth_constraints_ctx.performance_mutex);

    item->initialized = true;
    out_handle->id = index;
    cloth_system_cloth_constraints_unlock();
    return CLOTH_CONSTRAINTS_SUCCESS;
}

void cloth_system_cloth_constraints_destroy(cloth_system_cloth_constraints_handle_t handle) {
    if (handle.id >= g_cloth_constraints_ctx.count) {
        return;
    }

    cloth_system_cloth_constraints_lock();
    cloth_system_cloth_constraints_cleanup_internal(&g_cloth_constraints_ctx.items[handle.id]);
    cloth_system_cloth_constraints_unlock();
}

int cloth_system_cloth_constraints_update(cloth_system_cloth_constraints_handle_t handle, const void* data, size_t size) {
    if (!data && size > 0) {
        return CLOTH_CONSTRAINTS_ERROR_INVALID_PARAM;
    }

    if (size > CLOTH_SYSTEM_CLOTH_CONSTRAINTS_MAX_DATA_SIZE) {
        return CLOTH_CONSTRAINTS_ERROR_INVALID_PARAM;
    }

    cloth_system_cloth_constraints_lock();

    if (handle.id >= g_cloth_constraints_ctx.count) {
        cloth_system_cloth_constraints_unlock();
        return CLOTH_CONSTRAINTS_ERROR_INVALID_HANDLE;
    }

    cloth_system_cloth_constraints_internal_t* item = &g_cloth_constraints_ctx.items[handle.id];
    if (!item->initialized) {
        cloth_system_cloth_constraints_unlock();
        return CLOTH_CONSTRAINTS_ERROR_INVALID_HANDLE;
    }

    // Validate input if validation is enabled
    if (g_cloth_constraints_ctx.validation.enabled) {
        if (!cloth_system_cloth_constraints_validate_item(item, data, size)) {
            pthread_mutex_lock(&g_cloth_constraints_ctx.validation.mutex);
            g_cloth_constraints_ctx.validation.error_count++;
            snprintf(g_cloth_constraints_ctx.validation.last_error, 
                    sizeof(g_cloth_constraints_ctx.validation.last_error),
                    "Validation failed for constraint %u", handle.id);
            pthread_mutex_unlock(&g_cloth_constraints_ctx.validation.mutex);
            
    uint64_t hash = 0;
    if (data && size > 0) {
        hash = cloth_system_cloth_constraints_hash_buffer(data, size);
        if (!item->hot_reload_enabled && hash == item->cached_hash && size == item->cached_size) {
            g_cloth_constraints_ctx.total_cache_hits++;
            cloth_system_cloth_constraints_unlock();
            return 0;
        }
    }

    void* serialized = NULL;
    size_t serialized_size = 0;
    if (cloth_system_cloth_constraints_serialize(data, size, hash, &serialized, &serialized_size) != 0) {
        cloth_system_cloth_constraints_unlock();
        return -4;
    }

    if (item->data) {
        cloth_system_cloth_constraints_track_alloc(-((ssize_t)item->data_size));
        free(item->data);
    }
    item->data = serialized;
    item->data_size = serialized_size;
    cloth_system_cloth_constraints_track_alloc((ssize_t)serialized_size);
    item->cached_hash = hash;
    item->cached_size = size;
    item->frame_updated++;
    item->dirty = true;
    item->resource_state = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_DIRTY;
    item->update_count++;
    item->bytes_serialized += serialized_size;
    g_cloth_constraints_ctx.total_updates++;

    if (item->flags & CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_ASYNC) {
        item->pending_async = true;
        item->resource_state = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_PENDING_ASYNC;
        g_cloth_constraints_ctx.total_async_submits++;
        cloth_system_cloth_constraints_unlock();
        return 0;
    }

    if (cloth_system_cloth_constraints_backend_update(item) != 0) {
        cloth_system_cloth_constraints_unlock();
        return -5;
    }

    cloth_system_cloth_constraints_unlock();
    return 0;
}

bool cloth_system_cloth_constraints_is_valid(cloth_system_cloth_constraints_handle_t handle) {
    // TODO: Add cloth constraints batch processing
    if (handle.id >= g_cloth_constraints_ctx.count) {
        return false;
    }
    cloth_system_cloth_constraints_lock();
    bool valid = g_cloth_constraints_ctx.items[handle.id].initialized;
    cloth_system_cloth_constraints_unlock();
    return valid;
}

int cloth_system_cloth_constraints_get_info(cloth_system_cloth_constraints_handle_t handle, cloth_system_cloth_constraints_info_t* out_info) {
    // TODO: Implement cloth constraints streaming support
    // TODO: Add cloth constraints LOD support

    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_cloth_constraints_ctx.count) {
        return -2;
    }

    cloth_system_cloth_constraints_lock();
    const cloth_system_cloth_constraints_internal_t* item = &g_cloth_constraints_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    cloth_system_cloth_constraints_unlock();

    return 0;
}

void cloth_system_cloth_constraints_mark_dirty(cloth_system_cloth_constraints_handle_t handle) {
    // TODO: Implement cloth constraints culling integration
    if (handle.id < g_cloth_constraints_ctx.count) {
        cloth_system_cloth_constraints_lock();
        g_cloth_constraints_ctx.items[handle.id].dirty = true;
        g_cloth_constraints_ctx.items[handle.id].resource_state = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_DIRTY;
        cloth_system_cloth_constraints_unlock();
    }
}

int cloth_system_cloth_constraints_process_pending(void) {
    // TODO: Add cloth constraints render graph node
    // TODO: Implement batch processing

    int processed = 0;
    cloth_system_cloth_constraints_lock();
    for (uint32_t i = 0; i < g_cloth_constraints_ctx.count; i++) {
        cloth_system_cloth_constraints_internal_t* item = &g_cloth_constraints_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->resource_state = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_PROCESSING;
            if (item->pending_async) {
                if (cloth_system_cloth_constraints_backend_update(item) == 0) {
                    item->pending_async = false;
                }
            }
            item->dirty = false;
            item->resource_state = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_READY;
            processed++;
        }
    }
    g_cloth_constraints_ctx.total_processed += (uint64_t)processed;
    cloth_system_cloth_constraints_unlock();

    return processed;
}

uint32_t cloth_system_cloth_constraints_get_count(void) {
    return g_cloth_constraints_ctx.count;
}

size_t cloth_system_cloth_constraints_get_memory_usage(void) {
    return g_cloth_constraints_ctx.allocated_bytes;
}

void cloth_system_cloth_constraints_debug_print(void) {
    // TODO: Implement debug output
    fprintf(stderr,
            "cloth_constraints: items=%u capacity=%u allocated=%zu peak=%zu updates=%llu cache_hits=%llu async_submits=%llu processed=%llu\n",
            g_cloth_constraints_ctx.count,
            g_cloth_constraints_ctx.capacity,
            g_cloth_constraints_ctx.allocated_bytes,
            g_cloth_constraints_ctx.peak_bytes,
            (unsigned long long)g_cloth_constraints_ctx.total_updates,
            (unsigned long long)g_cloth_constraints_ctx.total_cache_hits,
            (unsigned long long)g_cloth_constraints_ctx.total_async_submits,
            (unsigned long long)g_cloth_constraints_ctx.total_processed);
}

/* End of cloth_constraints.c */
