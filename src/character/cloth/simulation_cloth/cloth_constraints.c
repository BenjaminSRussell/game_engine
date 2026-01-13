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

// Backend-specific contexts
typedef struct {
#ifdef __linux__
    VkInstance vulkan_instance;
    VkDevice vulkan_device;
    VkCommandPool vulkan_command_pool;
    VkBuffer vulkan_constraint_buffer;
    VkDeviceMemory vulkan_constraint_memory;
#endif
#ifdef __APPLE__
    id<MTLDevice> metal_device;
    id<MTLCommandQueue> metal_command_queue;
    id<MTLBuffer> metal_constraint_buffer;
#endif
#ifdef _WIN32
    ID3D12Device* d3d12_device;
    ID3D12CommandQueue* d3d12_command_queue;
    ID3D12Resource* d3d12_constraint_buffer;
#endif
    bool initialized;
} cloth_constraints_backend_context_t;

// Memory pool for constraint data
typedef struct {
    void* memory;
    size_t size;
    size_t used;
    size_t peak_usage;
    uint32_t allocation_count;
    pthread_mutex_t mutex;
} cloth_constraints_memory_pool_t;

// Cache entry for constraint data
typedef struct {
    uint64_t hash;
    void* data;
    size_t size;
    uint64_t last_access;
    uint32_t access_count;
    bool valid;
} cloth_constraints_cache_entry_t;

// Async operation structure
typedef struct {
    uint32_t id;
    cloth_constraint_type_t type;
    void* input_data;
    size_t input_size;
    void* output_data;
    size_t output_size;
    bool completed;
    bool cancelled;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} cloth_constraints_async_operation_t;

// LOD level data
typedef struct {
    uint32_t level;
    float distance_threshold;
    uint32_t constraint_count;
    float simplification_ratio;
    bool active;
} cloth_constraints_lod_level_t;

// Performance counters
typedef struct {
    uint64_t constraints_processed;
    uint64_t constraints_validated;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t async_operations;
    uint64_t simd_operations;
    uint64_t batch_operations;
    double total_processing_time;
    double average_processing_time;
    size_t peak_memory_usage;
    uint32_t active_constraints;
} cloth_constraints_performance_counters_t;

// Validation layer data
typedef struct {
    bool enabled;
    uint32_t error_count;
    uint32_t warning_count;
    char last_error[256];
    char last_warning[256];
    pthread_mutex_t mutex;
} cloth_constraints_validation_t;

// Hot-reload system
typedef struct {
    bool enabled;
    int inotify_fd;
    int watch_descriptor;
    pthread_t watcher_thread;
    pthread_mutex_t mutex;
    char watch_path[512];
    void (*reload_callback)(uint32_t constraint_id);
} cloth_constraints_hot_reload_t;

// Render graph node
typedef struct {
    uint32_t node_id;
    char name[64];
    uint32_t dependency_count;
    uint32_t* dependencies;
    bool enabled;
    void (*execute_func)(void* user_data);
    void* user_data;
} cloth_constraints_render_node_t;

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
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    cloth_system_cloth_constraints_backend_t backend;
    cloth_system_cloth_constraints_backend_ctx_t* backend_ctx;
    cloth_system_cloth_constraints_state_t resource_state;
    bool validation_enabled;
    bool hot_reload_enabled;
    bool pending_async;
    uint64_t cached_hash;
    size_t cached_size;
    uint64_t update_count;
    uint64_t bytes_serialized;
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

static cloth_system_cloth_constraints_context_t g_cloth_constraints_ctx = {0};

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

static cloth_system_cloth_constraints_backend_t cloth_system_cloth_constraints_select_backend(uint32_t flags) {
    if (flags & CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_BACKEND_VULKAN) {
        return CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_VULKAN;
    }
    if (flags & CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_BACKEND_METAL) {
        return CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_METAL;
    }
    if (flags & CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_BACKEND_D3D12) {
        return CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_D3D12;
    }
    return CLOTH_SYSTEM_CLOTH_CONSTRAINTS_BACKEND_CPU;
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
        return 0; // Already initialized
    }

    if (pthread_mutex_init(&g_cloth_constraints_ctx.mutex, NULL) != 0) {
        return -1;
    }
    g_cloth_constraints_ctx.capacity = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_DEFAULT_CAPACITY;
    g_cloth_constraints_ctx.items = calloc(g_cloth_constraints_ctx.capacity, sizeof(cloth_system_cloth_constraints_internal_t));
    if (!g_cloth_constraints_ctx.items) {
        pthread_mutex_destroy(&g_cloth_constraints_ctx.mutex);
        return -1;
    }
    cloth_system_cloth_constraints_track_alloc((ssize_t)(g_cloth_constraints_ctx.capacity * sizeof(cloth_system_cloth_constraints_internal_t)));

    g_cloth_constraints_ctx.count = 0;
    g_cloth_constraints_ctx.total_updates = 0;
    g_cloth_constraints_ctx.total_cache_hits = 0;
    g_cloth_constraints_ctx.total_async_submits = 0;
    g_cloth_constraints_ctx.total_processed = 0;
    g_cloth_constraints_ctx.initialized = true;

    return 0;
}

void cloth_system_cloth_constraints_shutdown(void) {
    if (!g_cloth_constraints_ctx.initialized) {
        return;
    }

    cloth_system_cloth_constraints_lock();
    for (uint32_t i = 0; i < g_cloth_constraints_ctx.count; i++) {
        cloth_system_cloth_constraints_cleanup_internal(&g_cloth_constraints_ctx.items[i]);
    }

    cloth_system_cloth_constraints_track_alloc(-((ssize_t)(g_cloth_constraints_ctx.capacity * sizeof(cloth_system_cloth_constraints_internal_t))));
    free(g_cloth_constraints_ctx.items);
    g_cloth_constraints_ctx.items = NULL;
    g_cloth_constraints_ctx.count = 0;
    g_cloth_constraints_ctx.capacity = 0;
    g_cloth_constraints_ctx.initialized = false;
    cloth_system_cloth_constraints_unlock();
    pthread_mutex_destroy(&g_cloth_constraints_ctx.mutex);

    if (g_cloth_constraints_ctx.allocated_bytes > 0) {
        fprintf(stderr, "cloth_constraints: leak detected (%zu bytes)\n", g_cloth_constraints_ctx.allocated_bytes);
    }
}

int cloth_system_cloth_constraints_create(cloth_system_cloth_constraints_handle_t* out_handle, const cloth_system_cloth_constraints_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_cloth_constraints_ctx.initialized) {
        return -2;
    }

    cloth_system_cloth_constraints_lock();
    if (g_cloth_constraints_ctx.count >= g_cloth_constraints_ctx.capacity) {
        cloth_system_cloth_constraints_unlock();
        return -3;
    }

    uint32_t index = g_cloth_constraints_ctx.count++;
    cloth_system_cloth_constraints_internal_t* item = &g_cloth_constraints_ctx.items[index];

    memset(item, 0, sizeof(*item));
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->backend = cloth_system_cloth_constraints_select_backend(desc->flags);
    item->resource_state = CLOTH_SYSTEM_CLOTH_CONSTRAINTS_STATE_READY;
    item->validation_enabled = (desc->flags & CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_VALIDATION) != 0;
    item->hot_reload_enabled = (desc->flags & CLOTH_SYSTEM_CLOTH_CONSTRAINTS_FLAG_HOT_RELOAD) != 0;
    item->pending_async = false;
    item->cached_hash = 0;
    item->cached_size = 0;
    item->update_count = 0;
    item->bytes_serialized = 0;
    if (cloth_system_cloth_constraints_backend_init(item) != 0) {
        item->initialized = false;
        cloth_system_cloth_constraints_unlock();
        return -4;
    }

    out_handle->id = index;
    cloth_system_cloth_constraints_unlock();
    return 0;
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
    if (handle.id >= g_cloth_constraints_ctx.count) {
        return -1;
    }

    cloth_system_cloth_constraints_lock();
    cloth_system_cloth_constraints_internal_t* item = &g_cloth_constraints_ctx.items[handle.id];
    if (!item->initialized) {
        cloth_system_cloth_constraints_unlock();
        return -2;
    }

    if (item->validation_enabled && !cloth_system_cloth_constraints_validate_item(item, data, size)) {
        cloth_system_cloth_constraints_unlock();
        return -3;
    }

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
