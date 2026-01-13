/*
 * defragmenter.c
 * Memory defragmentation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement Vulkan backend
 * TODO: Implement Metal backend
 * TODO: Implement D3D12 backend
 * TODO: Add thread-safe access patterns
 * TODO: Implement proper error handling with error codes
 * TODO: Add memory tracking and leak detection
 * TODO: Implement hot-reload support
 * TODO: Add validation layer integration
 * TODO: Implement resource state tracking
 * TODO: Add GPU debugging markers
 * TODO: Implement defragmenter initialization
 * TODO: Add defragmenter cleanup/shutdown
 * TODO: Implement defragmenter validation
 * TODO: Add defragmenter error handling
 * TODO: Implement defragmenter serialization
 * TODO: Add defragmenter debug output
 * TODO: Implement defragmenter unit tests
 * TODO: Add defragmenter performance counters
 * TODO: Implement defragmenter hot-reload
 * TODO: Add defragmenter thread safety
 * TODO: Implement defragmenter memory pooling
 * TODO: Add defragmenter caching layer
 * TODO: Implement defragmenter async operations
 * TODO: Add defragmenter GPU integration
 * TODO: Implement defragmenter SIMD optimization
 * TODO: Add defragmenter batch processing
 * TODO: Implement defragmenter streaming support
 * TODO: Add defragmenter LOD support
 * TODO: Implement defragmenter culling integration
 * TODO: Add defragmenter render graph node
 */

#include "core/memory/defragmenter.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// Graphics API includes for backend implementations
#ifdef ENABLE_VULKAN
#include <vulkan/vulkan.h>
#endif

#ifdef ENABLE_METAL
#include <Metal/Metal.h>
#endif

#ifdef ENABLE_D3D12
#include <d3d12.h>
#include <dxgi.h>
#endif
#include <pthread.h>
#include <time.h>
#include <errno.h>

/* Platform-specific includes */
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#include <CoreFoundation/CoreFoundation.h>
#else
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#endif

/* Graphics API includes */
#ifdef ENABLE_VULKAN
#include <vulkan/vulkan.h>
#endif
#ifdef ENABLE_METAL
#include <Metal/Metal.h>
#endif
#ifdef ENABLE_D3D12
#include <d3d12.h>
#endif
#include <pthread.h>
#include <time.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <immintrin.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CORE_DEFRAGMENTER_MAX_COUNT 4096
#define CORE_DEFRAGMENTER_DEFAULT_CAPACITY 256
#define CORE_DEFRAGMENTER_ALIGNMENT 16
#define CORE_DEFRAGMENTER_MAX_BATCH_SIZE 64
#define CORE_DEFRAGMENTER_MEMORY_POOL_SIZE (8 * 1024 * 1024) // 8MB
#define CORE_DEFRAGMENTER_MAX_THREADS 8
#define CORE_DEFRAGMENTER_CACHE_SIZE 1024
#define CORE_DEFRAGMENTER_MEMORY_BUDGET (512 * 1024 * 1024) /* 512MB */

/* Error codes */
typedef enum {
    CORE_DEFRAGMENTER_SUCCESS = 0,
    CORE_DEFRAGMENTER_ERROR_INVALID_PARAM = -1,
    CORE_DEFRAGMENTER_ERROR_NOT_INITIALIZED = -2,
    CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY = -3,
    CORE_DEFRAGMENTER_ERROR_BACKEND_UNAVAILABLE = -4,
    CORE_DEFRAGMENTER_ERROR_THREADING_ERROR = -5,
    CORE_DEFRAGMENTER_ERROR_VALIDATION_FAILED = -6,
    CORE_DEFRAGMENTER_ERROR_SERIALIZATION_FAILED = -7,
    CORE_DEFRAGMENTER_ERROR_GPU_OPERATION_FAILED = -8
} core_defragmenter_error_t;

/* Backend types */
typedef enum {
    CORE_DEFRAGMENTER_BACKEND_NONE = 0,
    CORE_DEFRAGMENTER_BACKEND_VULKAN = 1,
    CORE_DEFRAGMENTER_BACKEND_METAL = 2,
    CORE_DEFRAGMENTER_BACKEND_D3D12 = 3
} core_defragmenter_backend_type_t;

/* Validation levels */
typedef enum {
    CORE_DEFRAGMENTER_VALIDATION_DISABLED = 0,
    CORE_DEFRAGMENTER_VALIDATION_BASIC = 1,
    CORE_DEFRAGMENTER_VALIDATION_STRICT = 2
} core_defragmenter_validation_level_t;

/* Render graph node */
typedef struct core_defragmenter_render_node {
    uint32_t node_id;
    uint32_t dependency_count;
    uint32_t* dependencies;
    void (*execute_func)(struct core_defragmenter_render_node* node);
    void* user_data;
} core_defragmenter_render_node_t;

/* Performance counters */
typedef struct core_defragmenter_perf_counters {
    uint64_t operations_processed;
    uint64_t bytes_defragmented;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t validation_errors;
    uint64_t serialization_operations;
    double total_time_ms;
    double avg_operation_time_ms;
    size_t peak_memory_usage;
    size_t current_memory_usage;
} core_defragmenter_perf_counters_t;

/* Memory tracking */
typedef struct core_defragmenter_memory_tracker {
    size_t total_allocated;
    size_t peak_allocated;
    size_t current_allocated;
    uint32_t allocation_count;
    uint32_t deallocation_count;
    uint32_t leak_count;
    void* allocations[CORE_DEFRAGMENTER_MAX_COUNT];
    size_t allocation_sizes[CORE_DEFRAGMENTER_MAX_COUNT];
} core_defragmenter_memory_tracker_t;

/* Hot-reload file watcher */
typedef struct core_defragmenter_file_watcher {
    int watch_descriptor;
    char filename[256];
    void (*callback)(const char* filename, void* user_data);
    void* user_data;
    uint64_t last_modified;
} core_defragmenter_file_watcher_t;

/* Cache entry */
typedef struct core_defragmenter_cache_entry {
    uint64_t hash;
    void* data;
    size_t size;
    uint64_t last_access;
    uint32_t access_count;
    bool valid;
} core_defragmenter_cache_entry_t;

/* Async operation */
typedef struct core_defragmenter_async_op {
    uint32_t operation_id;
    uint32_t type;
    void* data;
    size_t data_size;
    void (*callback)(struct core_defragmenter_async_op* op, int result);
    void* user_data;
    bool completed;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
} core_defragmenter_async_op_t;

/* SIMD context */
typedef struct core_defragmenter_simd_context {
    bool avx2_supported;
    bool sse4_1_supported;
    bool neon_supported;
    void (*defrag_func)(void* dst, const void* src, size_t size);
} core_defragmenter_simd_context_t;

/* LOD information */
typedef struct core_defragmenter_lod_info {
    uint32_t lod_level;
    float distance_threshold;
    uint32_t quality_factor;
    bool enabled;
} core_defragmenter_lod_info_t;

/* Culling integration */
typedef struct core_defragmenter_culling_info {
    bool culling_enabled;
    float culling_distance;
    uint32_t objects_culled;
    uint32_t objects_processed;
} core_defragmenter_culling_info_t;

/* Enhanced internal structure */
typedef struct core_defragmenter_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    /* Enhanced features */
    core_defragmenter_backend_type_t backend_type;
    core_defragmenter_lod_info_t lod_info;
    core_defragmenter_culling_info_t culling_info;
    uint64_t resource_state;
    void* gpu_resource;
    uint32_t render_graph_node_id;
    
    /* Memory tracking */
    size_t memory_allocated;
    uint64_t allocation_timestamp;
    
    /* Validation */
    uint32_t validation_errors;
    bool validation_passed;
} core_defragmenter_internal_t;

/* Enhanced context structure */
typedef struct core_defragmenter_context {
    core_defragmenter_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Thread safety */
    pthread_mutex_t global_mutex;
    pthread_rwlock_t context_lock;
    pthread_t worker_threads[CORE_DEFRAGMENTER_WORKER_THREADS];
    volatile bool shutdown_requested;
    
    /* Backend support */
    core_defragmenter_backend_type_t active_backend;
#ifdef ENABLE_VULKAN
    VkInstance vulkan_instance;
    VkDevice vulkan_device;
#endif
#ifdef ENABLE_METAL
    id<MTLDevice> metal_device;
    id<MTLCommandQueue> metal_queue;
#endif
#ifdef ENABLE_D3D12
    ID3D12Device* d3d12_device;
    ID3D12CommandQueue* d3d12_queue;
#endif
    
    /* Performance and tracking */
    core_defragmenter_perf_counters_t perf_counters;
    core_defragmenter_memory_tracker_t memory_tracker;
    
    /* Hot-reload support */
    core_defragmenter_file_watcher_t file_watchers[CORE_DEFRAGMENTER_MAX_WATCH_FILES];
    uint32_t file_watcher_count;
#ifdef __linux__
    int inotify_fd;
#endif
    
    /* Validation */
    core_defragmenter_validation_level_t validation_level;
    
    /* Caching */
    core_defragmenter_cache_entry_t cache[CORE_DEFRAGMENTER_CACHE_SIZE];
    uint32_t cache_size;
    pthread_mutex_t cache_mutex;
    
    /* Async operations */
    core_defragmenter_async_op_t async_ops[CORE_DEFRAGMENTER_MAX_COUNT];
    uint32_t async_op_count;
    pthread_mutex_t async_mutex;
    
    /* SIMD support */
    core_defragmenter_simd_context_t simd_context;
    
    /* Render graph */
    core_defragmenter_render_node_t render_nodes[CORE_DEFRAGMENTER_MAX_COUNT];
    uint32_t render_node_count;
    
} core_defragmenter_context_t;

static core_defragmenter_context_t g_defragmenter_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ============================================================================ */

static const char* core_defragmenter_error_string(core_defragmenter_error_t error);
static uint64_t core_defragmenter_get_timestamp(void);
static uint64_t core_defragmenter_calculate_hash(const void* data, size_t size);
static bool core_defragmenter_detect_simd_support(void);
static void* core_defragmenter_worker_thread(void* arg);
static void core_defragmenter_simd_defrag_avx2(void* dst, const void* src, size_t size);
static void core_defragmenter_simd_defrag_sse41(void* dst, const void* src, size_t size);
static void core_defragmenter_simd_defrag_neon(void* dst, const void* src, size_t size);
static void core_defragmenter_simd_defrag_scalar(void* dst, const void* src, size_t size);
static int core_defragmenter_init_vulkan_backend(void);
static int core_defragmenter_init_metal_backend(void);
static int core_defragmenter_init_d3d12_backend(void);
static void core_defragmenter_shutdown_vulkan_backend(void);
static void core_defragmenter_shutdown_metal_backend(void);
static void core_defragmenter_shutdown_d3d12_backend(void);
static int core_defragmenter_validate_item(const core_defragmenter_internal_t* item, core_defragmenter_validation_level_t level);
static void core_defragmenter_update_performance_counters(const core_defragmenter_internal_t* item, double operation_time_ms);
static int core_defragmenter_cache_add(uint64_t hash, const void* data, size_t size);
static bool core_defragmenter_cache_lookup(uint64_t hash, void** out_data, size_t* out_size);
static void core_defragmenter_cache_cleanup(void);
static int core_defragmenter_serialize_data(const void* data, size_t size, void** out_serialized, size_t* out_serialized_size);
static int core_defragmenter_deserialize_data(const void* serialized_data, size_t serialized_size, void** out_data, size_t* out_size);
static void core_defragmenter_file_watcher_thread(void);
static int core_defragmenter_add_file_watch(const char* filename, void (*callback)(const char*, void*), void* user_data);
static void core_defragmenter_remove_file_watch(const char* filename);
static uint32_t core_defragmenter_add_render_graph_node(const core_defragmenter_render_node_t* node);
static void core_defragmenter_remove_render_graph_node(uint32_t node_id);
static void core_defragmenter_execute_render_graph(void);
static int core_defragmenter_gpu_defragment(core_defragmenter_internal_t* item);
static void core_defragmenter_apply_lod(core_defragmenter_internal_t* item, float distance);
static bool core_defragmenter_should_cull(const core_defragmenter_internal_t* item, float distance);
static void core_defragmenter_update_culling_stats(const core_defragmenter_internal_t* item, bool culled);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static const char* core_defragmenter_error_string(core_defragmenter_error_t error) {
    switch (error) {
        case CORE_DEFRAGMENTER_SUCCESS: return "Success";
        case CORE_DEFRAGMENTER_ERROR_INVALID_PARAM: return "Invalid parameter";
        case CORE_DEFRAGMENTER_ERROR_NOT_INITIALIZED: return "Not initialized";
        case CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case CORE_DEFRAGMENTER_ERROR_BACKEND_UNAVAILABLE: return "Backend unavailable";
        case CORE_DEFRAGMENTER_ERROR_THREADING_ERROR: return "Threading error";
        case CORE_DEFRAGMENTER_ERROR_VALIDATION_FAILED: return "Validation failed";
        case CORE_DEFRAGMENTER_ERROR_SERIALIZATION_FAILED: return "Serialization failed";
        case CORE_DEFRAGMENTER_ERROR_GPU_OPERATION_FAILED: return "GPU operation failed";
        default: return "Unknown error";
    }
}

static uint64_t core_defragmenter_get_timestamp(void) {
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000000) / frequency.QuadPart;
#elif defined(__APPLE__)
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    uint64_t time = mach_absolute_time();
    return (time * timebase.numer) / timebase.denom;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
#endif
}

static uint64_t core_defragmenter_calculate_hash(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t hash = 14695981039346656037ULL;
    
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    
    return hash;
}

static bool core_defragmenter_detect_simd_support(void) {
    core_defragmenter_simd_context_t* ctx = &g_defragmenter_ctx.simd_context;
    ctx->avx2_supported = false;
    ctx->sse4_1_supported = false;
    ctx->neon_supported = false;
    
#ifdef __AVX2__
    ctx->avx2_supported = true;
    ctx->defrag_func = core_defragmenter_simd_defrag_avx2;
#elif defined(__SSE4_1__)
    ctx->sse4_1_supported = true;
    ctx->defrag_func = core_defragmenter_simd_defrag_sse41;
#elif defined(__ARM_NEON)
    ctx->neon_supported = true;
    ctx->defrag_func = core_defragmenter_simd_defrag_neon;
#else
    ctx->defrag_func = core_defragmenter_simd_defrag_scalar;
#endif
    
    return ctx->avx2_supported || ctx->sse4_1_supported || ctx->neon_supported;
}

static void core_defragmenter_simd_defrag_scalar(void* dst, const void* src, size_t size) {
    memcpy(dst, src, size);
}

#ifdef __AVX2__
static void core_defragmenter_simd_defrag_avx2(void* dst, const void* src, size_t size) {
    const __m256i* src_ptr = (const __m256i*)src;
    __m256i* dst_ptr = (__m256i*)dst;
    size_t vec_size = size / sizeof(__m256i);
    
    for (size_t i = 0; i < vec_size; i++) {
        __m256i vec = _mm256_loadu_si256(&src_ptr[i]);
        _mm256_storeu_si256(&dst_ptr[i], vec);
    }
    
    // Handle remaining bytes
    size_t remaining = size % sizeof(__m256i);
    if (remaining > 0) {
        memcpy((uint8_t*)dst + vec_size * sizeof(__m256i),
               (const uint8_t*)src + vec_size * sizeof(__m256i),
               remaining);
    }
}
#endif

#ifdef __SSE4_1__
static void core_defragmenter_simd_defrag_sse41(void* dst, const void* src, size_t size) {
    const __m128i* src_ptr = (const __m128i*)src;
    __m128i* dst_ptr = (__m128i*)dst;
    size_t vec_size = size / sizeof(__m128i);
    
    for (size_t i = 0; i < vec_size; i++) {
        __m128i vec = _mm_loadu_si128(&src_ptr[i]);
        _mm_storeu_si128(&dst_ptr[i], vec);
    }
    
    // Handle remaining bytes
    size_t remaining = size % sizeof(__m128i);
    if (remaining > 0) {
        memcpy((uint8_t*)dst + vec_size * sizeof(__m128i),
               (const uint8_t*)src + vec_size * sizeof(__m128i),
               remaining);
    }
}
#endif

#ifdef __ARM_NEON
static void core_defragmenter_simd_defrag_neon(void* dst, const void* src, size_t size) {
    const uint8x16_t* src_ptr = (const uint8x16_t*)src;
    uint8x16_t* dst_ptr = (uint8x16_t*)dst;
    size_t vec_size = size / sizeof(uint8x16_t);
    
    for (size_t i = 0; i < vec_size; i++) {
        uint8x16_t vec = vld1q_u8((const uint8_t*)&src_ptr[i]);
        vst1q_u8((uint8_t*)&dst_ptr[i], vec);
    }
    
    // Handle remaining bytes
    size_t remaining = size % sizeof(uint8x16_t);
    if (remaining > 0) {
        memcpy((uint8_t*)dst + vec_size * sizeof(uint8x16_t),
               (const uint8_t*)src + vec_size * sizeof(uint8x16_t),
               remaining);
    }
}
#endif

/* Backend initialization functions */
static int core_defragmenter_init_vulkan_backend(void) {
#ifdef ENABLE_VULKAN
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Defragmenter";
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo instance_info = {0};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    
    VkResult result = vkCreateInstance(&instance_info, NULL, &g_defragmenter_ctx.vulkan_instance);
    if (result != VK_SUCCESS) {
        return CORE_DEFRAGMENTER_ERROR_BACKEND_UNAVAILABLE;
    }
    
    // Find a suitable device and create it
    // This is a simplified implementation
    g_defragmenter_ctx.active_backend = CORE_DEFRAGMENTER_BACKEND_VULKAN;
    return CORE_DEFRAGMENTER_SUCCESS;
#else
    return CORE_DEFRAGMENTER_ERROR_BACKEND_UNAVAILABLE;
#endif
}

static int core_defragmenter_init_metal_backend(void) {
#ifdef ENABLE_METAL
    g_defragmenter_ctx.metal_device = MTLCreateSystemDefaultDevice();
    if (!g_defragmenter_ctx.metal_device) {
        return CORE_DEFRAGMENTER_ERROR_BACKEND_UNAVAILABLE;
    }
    
    g_defragmenter_ctx.metal_queue = [g_defragmenter_ctx.metal_device newCommandQueue];
    if (!g_defragmenter_ctx.metal_queue) {
        return CORE_DEFRAGMENTER_ERROR_BACKEND_UNAVAILABLE;
    }
    
    g_defragmenter_ctx.active_backend = CORE_DEFRAGMENTER_BACKEND_METAL;
    return CORE_DEFRAGMENTER_SUCCESS;
#else
    return CORE_DEFRAGMENTER_ERROR_BACKEND_UNAVAILABLE;
#endif
}

static int core_defragmenter_init_d3d12_backend(void) {
#ifdef ENABLE_D3D12
    HRESULT result = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, 
                                       IID_PPV_ARGS(&g_defragmenter_ctx.d3d12_device));
    if (FAILED(result)) {
        return CORE_DEFRAGMENTER_ERROR_BACKEND_UNAVAILABLE;
    }
    
    D3D12_COMMAND_QUEUE_DESC queue_desc = {0};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    result = g_defragmenter_ctx.d3d12_device->CreateCommandQueue(&queue_desc, 
                                                                IID_PPV_ARGS(&g_defragmenter_ctx.d3d12_queue));
    if (FAILED(result)) {
        return CORE_DEFRAGMENTER_ERROR_BACKEND_UNAVAILABLE;
    }
    
    g_defragmenter_ctx.active_backend = CORE_DEFRAGMENTER_BACKEND_D3D12;
    return CORE_DEFRAGMENTER_SUCCESS;
#else
    return CORE_DEFRAGMENTER_ERROR_BACKEND_UNAVAILABLE;
#endif
}

/* Backend shutdown functions */
static void core_defragmenter_shutdown_vulkan_backend(void) {
#ifdef ENABLE_VULKAN
    if (g_defragmenter_ctx.vulkan_device) {
        vkDestroyDevice(g_defragmenter_ctx.vulkan_device, NULL);
        g_defragmenter_ctx.vulkan_device = NULL;
    }
    if (g_defragmenter_ctx.vulkan_instance) {
        vkDestroyInstance(g_defragmenter_ctx.vulkan_instance, NULL);
        g_defragmenter_ctx.vulkan_instance = NULL;
    }
#endif
}

static void core_defragmenter_shutdown_metal_backend(void) {
#ifdef ENABLE_METAL
    if (g_defragmenter_ctx.metal_queue) {
        [g_defragmenter_ctx.metal_queue release];
        g_defragmenter_ctx.metal_queue = nil;
    }
    if (g_defragmenter_ctx.metal_device) {
        [g_defragmenter_ctx.metal_device release];
        g_defragmenter_ctx.metal_device = nil;
    }
#endif
}

static void core_defragmenter_shutdown_d3d12_backend(void) {
#ifdef ENABLE_D3D12
    if (g_defragmenter_ctx.d3d12_queue) {
        g_defragmenter_ctx.d3d12_queue->Release();
        g_defragmenter_ctx.d3d12_queue = NULL;
    }
    if (g_defragmenter_ctx.d3d12_device) {
        g_defragmenter_ctx.d3d12_device->Release();
        g_defragmenter_ctx.d3d12_device = NULL;
    }
#endif
}

/* Validation function */
static int core_defragmenter_validate_item(const core_defragmenter_internal_t* item, core_defragmenter_validation_level_t level) {
    if (!item) {
        return CORE_DEFRAGMENTER_ERROR_INVALID_PARAM;
    }
    
    if (!item->initialized) {
        return CORE_DEFRAGMENTER_ERROR_NOT_INITIALIZED;
    }
    
    if (level >= CORE_DEFRAGMENTER_VALIDATION_BASIC) {
        if (!item->data && item->data_size > 0) {
            return CORE_DEFRAGMENTER_ERROR_VALIDATION_FAILED;
        }
        
        if (item->data_size > CORE_DEFRAGMENTER_MEMORY_BUDGET) {
            return CORE_DEFRAGMENTER_ERROR_VALIDATION_FAILED;
        }
    }
    
    if (level >= CORE_DEFRAGMENTER_VALIDATION_STRICT) {
        // Validate backend-specific resources
        if (item->backend_type != CORE_DEFRAGMENTER_BACKEND_NONE && !item->gpu_resource) {
            return CORE_DEFRAGMENTER_ERROR_VALIDATION_FAILED;
        }
        
        // Validate LOD settings
        if (item->lod_info.distance_threshold < 0.0f) {
            return CORE_DEFRAGMENTER_ERROR_VALIDATION_FAILED;
        }
    }
    
    return CORE_DEFRAGMENTER_SUCCESS;
}

/* Performance counter updates */
static void core_defragmenter_update_performance_counters(const core_defragmenter_internal_t* item, double operation_time_ms) {
    core_defragmenter_perf_counters_t* counters = &g_defragmenter_ctx.perf_counters;
    
    counters->operations_processed++;
    counters->bytes_defragmented += item->data_size;
    counters->total_time_ms += operation_time_ms;
    counters->avg_operation_time_ms = counters->total_time_ms / counters->operations_processed;
    
    if (g_defragmenter_ctx.memory_tracker.current_allocated > counters->peak_memory_usage) {
        counters->peak_memory_usage = g_defragmenter_ctx.memory_tracker.current_allocated;
    }
    counters->current_memory_usage = g_defragmenter_ctx.memory_tracker.current_allocated;
}

/* Cache functions */
static int core_defragmenter_cache_add(uint64_t hash, const void* data, size_t size) {
    pthread_mutex_lock(&g_defragmenter_ctx.cache_mutex);
    
    // Find existing entry or create new one
    int index = -1;
    for (uint32_t i = 0; i < g_defragmenter_ctx.cache_size; i++) {
        if (g_defragmenter_ctx.cache[i].hash == hash) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        // Find empty slot or LRU slot
        uint64_t oldest_time = UINT64_MAX;
        for (uint32_t i = 0; i < CORE_DEFRAGMENTER_CACHE_SIZE; i++) {
            if (!g_defragmenter_ctx.cache[i].valid) {
                index = i;
                break;
            }
            if (g_defragmenter_ctx.cache[i].last_access < oldest_time) {
                oldest_time = g_defragmenter_ctx.cache[i].last_access;
                index = i;
            }
        }
    }
    
    if (index >= 0 && index < CORE_DEFRAGMENTER_CACHE_SIZE) {
        // Free existing data if any
        if (g_defragmenter_ctx.cache[index].data) {
            free(g_defragmenter_ctx.cache[index].data);
        }
        
        // Add new entry
        g_defragmenter_ctx.cache[index].data = malloc(size);
        if (g_defragmenter_ctx.cache[index].data) {
            memcpy(g_defragmenter_ctx.cache[index].data, data, size);
            g_defragmenter_ctx.cache[index].hash = hash;
            g_defragmenter_ctx.cache[index].size = size;
            g_defragmenter_ctx.cache[index].last_access = core_defragmenter_get_timestamp();
            g_defragmenter_ctx.cache[index].access_count = 1;
            g_defragmenter_ctx.cache[index].valid = true;
            
            if (g_defragmenter_ctx.cache_size <= index) {
                g_defragmenter_ctx.cache_size = index + 1;
            }
            
            pthread_mutex_unlock(&g_defragmenter_ctx.cache_mutex);
            return CORE_DEFRAGMENTER_SUCCESS;
        }
    }
    
    pthread_mutex_unlock(&g_defragmenter_ctx.cache_mutex);
    return CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY;
}

static bool core_defragmenter_cache_lookup(uint64_t hash, void** out_data, size_t* out_size) {
    pthread_mutex_lock(&g_defragmenter_ctx.cache_mutex);
    
    for (uint32_t i = 0; i < g_defragmenter_ctx.cache_size; i++) {
        if (g_defragmenter_ctx.cache[i].valid && g_defragmenter_ctx.cache[i].hash == hash) {
            g_defragmenter_ctx.cache[i].last_access = core_defragmenter_get_timestamp();
            g_defragmenter_ctx.cache[i].access_count++;
            
            *out_data = malloc(g_defragmenter_ctx.cache[i].size);
            if (*out_data) {
                memcpy(*out_data, g_defragmenter_ctx.cache[i].data, g_defragmenter_ctx.cache[i].size);
                *out_size = g_defragmenter_ctx.cache[i].size;
                
                g_defragmenter_ctx.perf_counters.cache_hits++;
                pthread_mutex_unlock(&g_defragmenter_ctx.cache_mutex);
                return true;
            }
        }
    }
    
    g_defragmenter_ctx.perf_counters.cache_misses++;
    pthread_mutex_unlock(&g_defragmenter_ctx.cache_mutex);
    return false;
}

static void core_defragmenter_cache_cleanup(void) {
    pthread_mutex_lock(&g_defragmenter_ctx.cache_mutex);
    
    for (uint32_t i = 0; i < g_defragmenter_ctx.cache_size; i++) {
        if (g_defragmenter_ctx.cache[i].data) {
            free(g_defragmenter_ctx.cache[i].data);
            g_defragmenter_ctx.cache[i].data = NULL;
        }
        g_defragmenter_ctx.cache[i].valid = false;
    }
    
    g_defragmenter_ctx.cache_size = 0;
    pthread_mutex_unlock(&g_defragmenter_ctx.cache_mutex);
}

/* Serialization functions */
static int core_defragmenter_serialize_data(const void* data, size_t size, void** out_serialized, size_t* out_serialized_size) {
    if (!data || !out_serialized || !out_serialized_size) {
        return CORE_DEFRAGMENTER_ERROR_INVALID_PARAM;
    }
    
    // Header: magic number (4) + version (4) + size (8) + checksum (4) = 20 bytes
    *out_serialized_size = 20 + size;
    *out_serialized = malloc(*out_serialized_size);
    
    if (!*out_serialized) {
        return CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY;
    }
    
    uint8_t* ptr = (uint8_t*)*out_serialized;
    
    // Write header
    uint32_t magic = 0x44454652; // "DEFR"
    uint32_t version = 1;
    uint32_t checksum = 0; // Simple checksum
    
    memcpy(ptr, &magic, 4);
    memcpy(ptr + 4, &version, 4);
    memcpy(ptr + 8, &size, 8);
    memcpy(ptr + 16, &checksum, 4);
    
    // Write data
    memcpy(ptr + 20, data, size);
    
    // Calculate and write checksum
    for (size_t i = 0; i < size; i++) {
        checksum += ((uint8_t*)data)[i];
    }
    memcpy(ptr + 16, &checksum, 4);
    
    g_defragmenter_ctx.perf_counters.serialization_operations++;
    return CORE_DEFRAGMENTER_SUCCESS;
}

static int core_defragmenter_deserialize_data(const void* serialized_data, size_t serialized_size, void** out_data, size_t* out_size) {
    if (!serialized_data || !out_data || !out_size || serialized_size < 20) {
        return CORE_DEFRAGMENTER_ERROR_INVALID_PARAM;
    }
    
    const uint8_t* ptr = (const uint8_t*)serialized_data;
    
    // Read header
    uint32_t magic, version, checksum;
    size_t data_size;
    
    memcpy(&magic, ptr, 4);
    memcpy(&version, ptr + 4, 4);
    memcpy(&data_size, ptr + 8, 8);
    memcpy(&checksum, ptr + 16, 4);
    
    // Validate header
    if (magic != 0x44454652 || version != 1 || data_size + 20 != serialized_size) {
        return CORE_DEFRAGMENTER_ERROR_SERIALIZATION_FAILED;
    }
    
    // Verify checksum
    uint32_t calculated_checksum = 0;
    for (size_t i = 0; i < data_size; i++) {
        calculated_checksum += ptr[20 + i];
    }
    
    if (calculated_checksum != checksum) {
        return CORE_DEFRAGMENTER_ERROR_SERIALIZATION_FAILED;
    }
    
    // Extract data
    *out_data = malloc(data_size);
    if (!*out_data) {
        return CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(*out_data, ptr + 20, data_size);
    *out_size = data_size;
    
    return CORE_DEFRAGMENTER_SUCCESS;
}

/* Render graph functions */
static uint32_t core_defragmenter_add_render_graph_node(const core_defragmenter_render_node_t* node) {
    if (!node || g_defragmenter_ctx.render_node_count >= CORE_DEFRAGMENTER_MAX_COUNT) {
        return UINT32_MAX;
    }
    
    uint32_t node_id = g_defragmenter_ctx.render_node_count++;
    g_defragmenter_ctx.render_nodes[node_id] = *node;
    g_defragmenter_ctx.render_nodes[node_id].node_id = node_id;
    
    return node_id;
}

static void core_defragmenter_remove_render_graph_node(uint32_t node_id) {
    if (node_id >= g_defragmenter_ctx.render_node_count) {
        return;
    }
    
    // Remove dependencies and shift remaining nodes
    for (uint32_t i = node_id; i < g_defragmenter_ctx.render_node_count - 1; i++) {
        g_defragmenter_ctx.render_nodes[i] = g_defragmenter_ctx.render_nodes[i + 1];
        g_defragmenter_ctx.render_nodes[i].node_id = i;
    }
    
    g_defragmenter_ctx.render_node_count--;
}

static void core_defragmenter_execute_render_graph(void) {
    for (uint32_t i = 0; i < g_defragmenter_ctx.render_node_count; i++) {
        core_defragmenter_render_node_t* node = &g_defragmenter_ctx.render_nodes[i];
        
        // Check dependencies (simplified)
        bool dependencies_ready = true;
        for (uint32_t j = 0; j < node->dependency_count; j++) {
            if (node->dependencies[j] >= i) {
                dependencies_ready = false;
                break;
            }
        }
        
        if (dependencies_ready && node->execute_func) {
            node->execute_func(node);
        }
    }
}

/* GPU and LOD functions */
static int core_defragmenter_gpu_defragment(core_defragmenter_internal_t* item) {
    if (!item || !item->data) {
        return CORE_DEFRAGMENTER_ERROR_INVALID_PARAM;
    }
    
    // GPU defragmentation implementation would go here
    // This is a placeholder for GPU-based memory defragmentation
    g_defragmenter_ctx.perf_counters.gpu_operations++;
    
    return CORE_DEFRAGMENTER_SUCCESS;
}

static void core_defragmenter_apply_lod(core_defragmenter_internal_t* item, float distance) {
    if (!item || !item->lod_info.enabled) {
        return;
    }
    
    // Adjust quality based on distance
    if (distance < item->lod_info.distance_threshold) {
        item->lod_info.lod_level = 3; // Ultra
    } else if (distance < item->lod_info.distance_threshold * 2.0f) {
        item->lod_info.lod_level = 2; // High
    } else if (distance < item->lod_info.distance_threshold * 4.0f) {
        item->lod_info.lod_level = 1; // Medium
    } else {
        item->lod_info.lod_level = 0; // Low
    }
}

static bool core_defragmenter_should_cull(const core_defragmenter_internal_t* item, float distance) {
    if (!item || !item->culling_info.culling_enabled) {
        return false;
    }
    
    return distance > item->culling_info.culling_distance;
}

static void core_defragmenter_update_culling_stats(const core_defragmenter_internal_t* item, bool culled) {
    if (!item) {
        return;
    }
    
    g_defragmenter_ctx.perf_counters.objects_processed++;
    if (culled) {
        g_defragmenter_ctx.perf_counters.objects_culled++;
    }
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

static bool core_defragmenter_validate(const core_defragmenter_internal_t* item) {
    // TODO-30460: Add defragmenter render graph node
    // TODO-30461: Implement Vulkan backend
    // TODO-30462: Implement Metal backend
    if (!item) return false;
    if (!item->initialized) return false;
    
    // Enhanced validation with backend support
    if (item->backend_type != CORE_DEFRAGMENTER_BACKEND_NONE) {
        switch (item->backend_type) {
            case CORE_DEFRAGMENTER_BACKEND_VULKAN:
#ifdef ENABLE_VULKAN
                if (!g_defragmenter_ctx.vulkan_instance) return false;
#endif
                break;
            case CORE_DEFRAGMENTER_BACKEND_METAL:
#ifdef ENABLE_METAL
                if (!g_defragmenter_ctx.metal_device) return false;
#endif
                break;
            case CORE_DEFRAGMENTER_BACKEND_D3D12:
#ifdef ENABLE_D3D12
                if (!g_defragmenter_ctx.d3d12_device) return false;
#endif
                break;
            default:
                break;
        }
    }
    
    return true;
}

static void core_defragmenter_cleanup_internal(core_defragmenter_internal_t* item) {
    // TODO-30463: Implement D3D12 backend
    // TODO-30464: Add thread-safe access patterns
    if (!item) return;
    
    pthread_mutex_lock(&g_defragmenter_ctx.global_mutex);
    
    if (item->data) {
        free(item->data);
        item->data = NULL;
        g_defragmenter_ctx.memory_tracker.current_allocated -= item->data_size;
        g_defragmenter_ctx.memory_tracker.deallocation_count++;
    }
    
    // Cleanup GPU resources
    if (item->gpu_resource) {
        // Backend-specific cleanup would go here
        item->gpu_resource = NULL;
    }
    
    // Remove from render graph
    if (item->render_graph_node_id != UINT32_MAX) {
        core_defragmenter_remove_render_graph_node(item->render_graph_node_id);
        item->render_graph_node_id = UINT32_MAX;
    }
    
    item->initialized = false;
    
    pthread_mutex_unlock(&g_defragmenter_ctx.global_mutex);
}

int core_defragmenter_init(void) {
    // TODO-30465: Implement proper error handling with error codes
    // TODO-30466: Add memory tracking and leak detection
    // TODO-30467: Implement hot-reload support
    // TODO-30468: Add validation layer integration
    
    if (g_defragmenter_ctx.initialized) {
        return CORE_DEFRAGMENTER_SUCCESS;
    }
    
    // Initialize mutexes
    if (pthread_mutex_init(&g_defragmenter_ctx.global_mutex, NULL) != 0 ||
        pthread_rwlock_init(&g_defragmenter_ctx.context_lock, NULL) != 0 ||
        pthread_mutex_init(&g_defragmenter_ctx.cache_mutex, NULL) != 0 ||
        pthread_mutex_init(&g_defragmenter_ctx.async_mutex, NULL) != 0) {
        return CORE_DEFRAGMENTER_ERROR_THREADING_ERROR;
    }
    
    // Initialize memory tracking
    memset(&g_defragmenter_ctx.memory_tracker, 0, sizeof(g_defragmenter_ctx.memory_tracker));
    
    // Initialize performance counters
    memset(&g_defragmenter_ctx.perf_counters, 0, sizeof(g_defragmenter_ctx.perf_counters));
    
    // Detect SIMD support
    core_defragmenter_detect_simd_support();
    
    // Initialize backends
    g_defragmenter_ctx.active_backend = CORE_DEFRAGMENTER_BACKEND_NONE;
    
    g_defragmenter_ctx.capacity = CORE_DEFRAGMENTER_DEFAULT_CAPACITY;
    g_defragmenter_ctx.items = calloc(g_defragmenter_ctx.capacity, sizeof(core_defragmenter_internal_t));
    if (!g_defragmenter_ctx.items) {
        return CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY;
    }
    
    g_defragmenter_ctx.count = 0;
    g_defragmenter_ctx.validation_level = CORE_DEFRAGMENTER_VALIDATION_BASIC;
    g_defragmenter_ctx.initialized = true;
    
    return CORE_DEFRAGMENTER_SUCCESS;
}

void core_defragmenter_shutdown(void) {
    // TODO-30469: Implement resource state tracking
    // TODO-30471: Implement defragmenter initialization
    // TODO-30472: Add defragmenter cleanup/shutdown
    
    if (!g_defragmenter_ctx.initialized) {
        return;
    }
    
    g_defragmenter_ctx.shutdown_requested = true;
    
    // Wait for worker threads to finish
    for (int i = 0; i < CORE_DEFRAGMENTER_WORKER_THREADS; i++) {
        if (g_defragmenter_ctx.worker_threads[i]) {
            pthread_join(g_defragmenter_ctx.worker_threads[i], NULL);
        }
    }
    
    // Cleanup all items
    for (uint32_t i = 0; i < g_defragmenter_ctx.count; i++) {
        core_defragmenter_cleanup_internal(&g_defragmenter_ctx.items[i]);
    }
    
    // Cleanup cache
    core_defragmenter_cache_cleanup();
    
    // Cleanup backends
    switch (g_defragmenter_ctx.active_backend) {
        case CORE_DEFRAGMENTER_BACKEND_VULKAN:
            core_defragmenter_shutdown_vulkan_backend();
            break;
        case CORE_DEFRAGMENTER_BACKEND_METAL:
            core_defragmenter_shutdown_metal_backend();
            break;
        case CORE_DEFRAGMENTER_BACKEND_D3D12:
            core_defragmenter_shutdown_d3d12_backend();
            break;
        default:
            break;
    }
    
    // Cleanup memory
    free(g_defragmenter_ctx.items);
    g_defragmenter_ctx.items = NULL;
    
    // Cleanup mutexes
    pthread_mutex_destroy(&g_defragmenter_ctx.global_mutex);
    pthread_rwlock_destroy(&g_defragmenter_ctx.context_lock);
    pthread_mutex_destroy(&g_defragmenter_ctx.cache_mutex);
    pthread_mutex_destroy(&g_defragmenter_ctx.async_mutex);
    
    // Report memory leaks
    if (g_defragmenter_ctx.memory_tracker.leak_count > 0) {
        printf("Warning: %u memory leaks detected\n", g_defragmenter_ctx.memory_tracker.leak_count);
    }
    
    g_defragmenter_ctx.count = 0;
    g_defragmenter_ctx.capacity = 0;
    g_defragmenter_ctx.initialized = false;
}

int core_defragmenter_create(core_defragmenter_handle_t* out_handle, const core_defragmenter_desc_t* desc) {
    // TODO-30473: Implement defragmenter validation
    // TODO-30474: Add defragmenter error handling
    // TODO-30475: Implement defragmenter serialization
    
    if (!out_handle || !desc) {
        return CORE_DEFRAGMENTER_ERROR_INVALID_PARAM;
    }
    
    if (!g_defragmenter_ctx.initialized) {
        return CORE_DEFRAGMENTER_ERROR_NOT_INITIALIZED;
    }
    
    pthread_rwlock_wrlock(&g_defragmenter_ctx.context_lock);
    
    if (g_defragmenter_ctx.count >= g_defragmenter_ctx.capacity) {
        pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
        return CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = g_defragmenter_ctx.count++;
    core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[index];
    
    // Initialize item
    memset(item, 0, sizeof(core_defragmenter_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->backend_type = g_defragmenter_ctx.active_backend;
    item->allocation_timestamp = core_defragmenter_get_timestamp();
    item->render_graph_node_id = UINT32_MAX;
    
    // Initialize LOD and culling info
    item->lod_info.distance_threshold = 100.0f;
    item->lod_info.lod_level = 2; // High quality by default
    item->lod_info.quality_factor = 100;
    item->lod_info.enabled = true;
    
    item->culling_info.culling_enabled = true;
    item->culling_info.culling_distance = 1000.0f;
    item->culling_info.objects_culled = 0;
    item->culling_info.objects_processed = 0;
    
    // Validate item
    int validation_result = core_defragmenter_validate_item(item, g_defragmenter_ctx.validation_level);
    if (validation_result != CORE_DEFRAGMENTER_SUCCESS) {
        g_defragmenter_ctx.count--;
        pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
        return validation_result;
    }
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    out_handle->id = index;
    
    pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
    return CORE_DEFRAGMENTER_SUCCESS;
}

void core_defragmenter_destroy(core_defragmenter_handle_t handle) {
    // TODO-30478: Add defragmenter performance counters
    // TODO-30479: Implement defragmenter hot-reload
    
    if (handle.id >= g_defragmenter_ctx.count) {
        return;
    }
    
    pthread_rwlock_wrlock(&g_defragmenter_ctx.context_lock);
    core_defragmenter_cleanup_internal(&g_defragmenter_ctx.items[handle.id]);
    pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
}

int core_defragmenter_update(core_defragmenter_handle_t handle, const void* data, size_t size) {
    // TODO-30480: Add defragmenter thread safety
    // TODO-30482: Add defragmenter caching layer
    // TODO-30483: Implement defragmenter async operations
    
    if (!data || size == 0) {
        return CORE_DEFRAGMENTER_ERROR_INVALID_PARAM;
    }
    
    if (handle.id >= g_defragmenter_ctx.count) {
        return CORE_DEFRAGMENTER_ERROR_INVALID_PARAM;
    }
    
    pthread_rwlock_wrlock(&g_defragmenter_ctx.context_lock);
    
    core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
        return CORE_DEFRAGMENTER_ERROR_NOT_INITIALIZED;
    }
    
    uint64_t start_time = core_defragmenter_get_timestamp();
    
    // Check cache first
    uint64_t hash = core_defragmenter_calculate_hash(data, size);
    void* cached_data = NULL;
    size_t cached_size = 0;
    
    if (core_defragmenter_cache_lookup(hash, &cached_data, &cached_size)) {
        // Use cached data
        if (item->data) {
            free(item->data);
        }
        item->data = cached_data;
        item->data_size = cached_size;
    } else {
        // Allocate new data
        void* new_data = malloc(size);
        if (!new_data) {
            pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
            return CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY;
        }
        
        // Use SIMD-optimized copy
        if (g_defragmenter_ctx.simd_context.defrag_func) {
            g_defragmenter_ctx.simd_context.defrag_func(new_data, data, size);
            g_defragmenter_ctx.perf_counters.simd_operations++;
        } else {
            memcpy(new_data, data, size);
        }
        
        if (item->data) {
            free(item->data);
        }
        item->data = new_data;
        item->data_size = size;
        
        // Add to cache
        core_defragmenter_cache_add(hash, data, size);
    }
    
    // TODO-30484: Add defragmenter GPU integration
    // TODO-30485: Implement defragmenter SIMD optimization
    
    item->dirty = false;
    item->frame_updated++;
    
    // Update memory tracking
    g_defragmenter_ctx.memory_tracker.current_allocated += size - item->memory_allocated;
    g_defragmenter_ctx.memory_tracker.total_allocated += size;
    g_defragmenter_ctx.memory_tracker.allocation_count++;
    item->memory_allocated = size;
    
    double operation_time = (core_defragmenter_get_timestamp() - start_time) / 1000.0;
    core_defragmenter_update_performance_counters(item, operation_time);
    
    pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
    return CORE_DEFRAGMENTER_SUCCESS;
}

bool core_defragmenter_is_valid(core_defragmenter_handle_t handle) {
    // TODO-30486: Add defragmenter batch processing
    
    if (handle.id >= g_defragmenter_ctx.count) {
        return false;
    }
    
    pthread_rwlock_rdlock(&g_defragmenter_ctx.context_lock);
    bool valid = g_defragmenter_ctx.items[handle.id].initialized;
    pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
    
    return valid;
}

int core_defragmenter_get_info(core_defragmenter_handle_t handle, core_defragmenter_info_t* out_info) {
    // TODO-30487: Implement defragmenter streaming support
    // TODO-30488: Add defragmenter LOD support
    
    if (!out_info) {
        return CORE_DEFRAGMENTER_ERROR_INVALID_PARAM;
    }
    
    if (handle.id >= g_defragmenter_ctx.count) {
        return CORE_DEFRAGMENTER_ERROR_INVALID_PARAM;
    }
    
    pthread_rwlock_rdlock(&g_defragmenter_ctx.context_lock);
    
    const core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
        return CORE_DEFRAGMENTER_ERROR_NOT_INITIALIZED;
    }
    
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
    return CORE_DEFRAGMENTER_SUCCESS;
}

void core_defragmenter_mark_dirty(core_defragmenter_handle_t handle) {
    // TODO-30489: Implement defragmenter culling integration
    
    if (handle.id < g_defragmenter_ctx.count) {
        pthread_rwlock_wrlock(&g_defragmenter_ctx.context_lock);
        g_defragmenter_ctx.items[handle.id].dirty = true;
        pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
    }
}

int core_defragmenter_process_pending(void) {
    // TODO-30460: Add defragmenter render graph node
    // TODO-30486: Add defragmenter batch processing
    
    pthread_rwlock_wrlock(&g_defragmenter_ctx.context_lock);
    
    int processed = 0;
    for (uint32_t i = 0; i < g_defragmenter_ctx.count; i++) {
        core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[i];
        if (item->initialized && item->dirty) {
            
            // Apply LOD based on distance (simplified)
            float distance = 50.0f; // Would be calculated from camera position
            core_defragmenter_apply_lod(item, distance);
            
            // Check if should cull
            bool should_cull = core_defragmenter_should_cull(item, distance);
            core_defragmenter_update_culling_stats(item, should_cull);
            
            if (!should_cull) {
                // Process item
                if (item->backend_type != CORE_DEFRAGMENTER_BACKEND_NONE) {
                    core_defragmenter_gpu_defragment(item);
                }
                
                processed++;
            }
            
            item->dirty = false;
        }
    }
    
    // Execute render graph
    core_defragmenter_execute_render_graph();
    
    pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
    return processed;
}

uint32_t core_defragmenter_get_count(void) {
    return g_defragmenter_ctx.count;
}

size_t core_defragmenter_get_memory_usage(void) {
    pthread_rwlock_rdlock(&g_defragmenter_ctx.context_lock);
    size_t total = sizeof(g_defragmenter_ctx);
    total += g_defragmenter_ctx.capacity * sizeof(core_defragmenter_internal_t);
    total += g_defragmenter_ctx.memory_tracker.current_allocated;
    pthread_rwlock_unlock(&g_defragmenter_ctx.context_lock);
    return total;
}

void core_defragmenter_debug_print(void) {
    printf("=== Defragmenter Debug Info ===\n");
    printf("Initialized: %s\n", g_defragmenter_ctx.initialized ? "Yes" : "No");
    printf("Count: %u/%u\n", g_defragmenter_ctx.count, g_defragmenter_ctx.capacity);
    printf("Backend: %d\n", g_defragmenter_ctx.active_backend);
    printf("Operations: %llu\n", (unsigned long long)g_defragmenter_ctx.perf_counters.operations_processed);
    printf("Memory Usage: %zu bytes\n", core_defragmenter_get_memory_usage());
    printf("Cache Hits: %llu, Misses: %llu\n", 
           (unsigned long long)g_defragmenter_ctx.perf_counters.cache_hits,
           (unsigned long long)g_defragmenter_ctx.perf_counters.cache_misses);
    printf("============================\n");
}

/* End of defragmenter.c */
    pthread_mutex_t mutex;
} core_defragmenter_internal_t;

typedef struct core_defragmenter_context {
    core_defragmenter_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Thread safety */
    pthread_mutex_t global_mutex;
    pthread_rwlock_t context_lock;
    
    /* Memory tracking */
    core_defragmenter_memory_tracker_t memory_tracker;
    
    /* Performance counters */
    core_defragmenter_performance_counters_t performance_counters;
    
    /* Cache layer */
    core_defragmenter_cache_entry_t cache[CORE_DEFRAGMENTER_CACHE_SIZE];
    uint32_t cache_usage;
    pthread_mutex_t cache_mutex;
    
    /* Async operations */
    core_defragmenter_async_operation_t async_ops[64];
    uint32_t async_count;
    pthread_t worker_threads[CORE_DEFRAGMENTER_WORKER_THREADS];
    pthread_mutex_t async_mutex;
    pthread_cond_t async_cond;
    bool async_shutdown;
    
    /* GPU integration */
    core_defragmenter_gpu_context_t gpu_context;
    
    /* SIMD optimization */
    core_defragmenter_simd_context_t simd_context;
    
    /* Batch processing */
    core_defragmenter_batch_context_t batch_context;
    
    /* Streaming support */
    core_defragmenter_streaming_context_t streaming_context;
    
    /* LOD support */
    core_defragmenter_lod_context_t lod_context;
    
    /* Culling integration */
    core_defragmenter_culling_context_t culling_context;
    
    /* Render graph */
    core_defragmenter_render_graph_node_t render_graph_nodes[128];
    uint32_t render_graph_node_count;
    
    /* Hot-reload file watching */
    core_defragmenter_file_watcher_t file_watcher;
    
    /* Validation layer */
    core_defragmenter_validation_layer_t validation_layer;
    
} core_defragmenter_context_t;

static core_defragmenter_context_t g_defragmenter_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool core_defragmenter_validate(const core_defragmenter_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->state == CORE_DEFRAGMENTER_STATE_ERROR) return false;
    return true;
}

static void core_defragmenter_cleanup_internal(core_defragmenter_internal_t* item) {
    if (!item) return;
    
    pthread_mutex_lock(&item->mutex);
    
    if (item->data) {
        free(item->data);
        item->data = NULL;
        g_defragmenter_ctx.memory_tracker.current_usage -= item->data_size;
        g_defragmenter_ctx.memory_tracker.allocation_count--;
    }
    
    item->initialized = false;
    item->state = CORE_DEFRAGMENTER_STATE_UNINITIALIZED;
    
    pthread_mutex_unlock(&item->mutex);
}

/* Helper function to get current timestamp */
static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

/* Helper function for error string conversion */
static const char* core_defragmenter_error_string(int error_code) {
    switch (error_code) {
        case CORE_DEFRAGMENTER_SUCCESS: return "Success";
        case CORE_DEFRAGMENTER_ERROR_INVALID_PARAM: return "Invalid parameter";
        case CORE_DEFRAGMENTER_ERROR_NOT_INITIALIZED: return "Not initialized";
        case CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case CORE_DEFRAGMENTER_ERROR_CAPACITY_EXCEEDED: return "Capacity exceeded";
        case CORE_DEFRAGMENTER_ERROR_INVALID_HANDLE: return "Invalid handle";
        case CORE_DEFRAGMENTER_ERROR_THREADING: return "Threading error";
        case CORE_DEFRAGMENTER_ERROR_FILE_IO: return "File I/O error";
        case CORE_DEFRAGMENTER_ERROR_VALIDATION: return "Validation error";
        case CORE_DEFRAGMENTER_ERROR_GPU_INTEGRATION: return "GPU integration error";
        case CORE_DEFRAGMENTER_ERROR_ASYNC_OPERATION: return "Async operation error";
        default: return "Unknown error";
    }
}

/* SIMD detection */
static void detect_simd_capabilities(void) {
    g_defragmenter_ctx.simd_context.sse2_available = false;
    g_defragmenter_ctx.simd_context.avx_available = false;
    g_defragmenter_ctx.simd_context.avx2_available = false;
    g_defragmenter_ctx.simd_context.avx512_available = false;
    
    // CPUID detection would go here for x86
    // For now, assume basic SSE2 availability
#ifdef __SSE2__
    g_defragmenter_ctx.simd_context.sse2_available = true;
    g_defragmenter_ctx.simd_context.simd_flags |= CORE_DEFRAGMENTER_SIMD_SSE2;
#endif
#ifdef __AVX__
    g_defragmenter_ctx.simd_context.avx_available = true;
    g_defragmenter_ctx.simd_context.simd_flags |= CORE_DEFRAGMENTER_SIMD_AVX;
#endif
#ifdef __AVX2__
    g_defragmenter_ctx.simd_context.avx2_available = true;
    g_defragmenter_ctx.simd_context.simd_flags |= CORE_DEFRAGMENTER_SIMD_AVX2;
#endif
}

/* File watcher thread function */
static void* file_watcher_thread(void* arg) {
    (void)arg;
    
    char buffer[4096];
    while (g_defragmenter_ctx.file_watcher.watcher_running) {
        ssize_t length = read(g_defragmenter_ctx.file_watcher.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            size_t i = 0;
            while (i < (size_t)length) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                if (event->mask & IN_MODIFY) {
                    for (uint32_t j = 0; j < g_defragmenter_ctx.file_watcher.watch_count; j++) {
                        if (g_defragmenter_ctx.file_watcher.watch_descriptors[j] == event->wd) {
                            if (g_defragmenter_ctx.file_watcher.reload_callback) {
                                g_defragmenter_ctx.file_watcher.reload_callback(
                                    g_defragmenter_ctx.file_watcher.watched_files[j]);
                            }
                            break;
                        }
                    }
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); // 100ms sleep
    }
    return NULL;
}

/* Worker thread for async operations */
static void* worker_thread(void* arg) {
    (void)arg;
    
    while (!g_defragmenter_ctx.async_shutdown) {
        pthread_mutex_lock(&g_defragmenter_ctx.async_mutex);
        
        // Find pending operation
        core_defragmenter_async_operation_t* op = NULL;
        for (uint32_t i = 0; i < g_defragmenter_ctx.async_count; i++) {
            if (g_defragmenter_ctx.async_ops[i].status == CORE_DEFRAGMENTER_ASYNC_PENDING) {
                op = &g_defragmenter_ctx.async_ops[i];
                op->status = CORE_DEFRAGMENTER_ASYNC_PROCESSING;
                break;
            }
        }
        
        if (!op) {
            pthread_cond_wait(&g_defragmenter_ctx.async_cond, &g_defragmenter_ctx.async_mutex);
            pthread_mutex_unlock(&g_defragmenter_ctx.async_mutex);
            continue;
        }
        
        pthread_mutex_unlock(&g_defragmenter_ctx.async_mutex);
        
        // Process operation
        uint64_t start_time = get_timestamp_ms();
        int result = CORE_DEFRAGMENTER_SUCCESS;
        
        // Simulate processing
        usleep(1000); // 1ms processing time
        
        op->processing_time = (get_timestamp_ms() - start_time) / 1000.0;
        op->status = (result == CORE_DEFRAGMENTER_SUCCESS) ? 
                     CORE_DEFRAGMENTER_ASYNC_COMPLETED : CORE_DEFRAGMENTER_ASYNC_FAILED;
        
        if (op->callback) {
            op->callback(op->id, result, op->user_data);
        }
        
        g_defragmenter_ctx.performance_counters.async_operations++;
    }
    return NULL;
}

/* Cache management */
static int cache_lookup(uint32_t id, void** out_data, size_t* out_size) {
    pthread_mutex_lock(&g_defragmenter_ctx.cache_mutex);
    
    for (uint32_t i = 0; i < CORE_DEFRAGMENTER_CACHE_SIZE; i++) {
        if (g_defragmenter_ctx.cache[i].valid && 
            g_defragmenter_ctx.cache[i].id == id) {
            g_defragmenter_ctx.cache[i].last_access = get_timestamp_ms();
            g_defragmenter_ctx.cache[i].access_count++;
            *out_data = g_defragmenter_ctx.cache[i].data;
            *out_size = g_defragmenter_ctx.cache[i].size;
            
            g_defragmenter_ctx.performance_counters.cache_hits++;
            pthread_mutex_unlock(&g_defragmenter_ctx.cache_mutex);
            return CORE_DEFRAGMENTER_SUCCESS;
        }
    }
    
    g_defragmenter_ctx.performance_counters.cache_misses++;
    pthread_mutex_unlock(&g_defragmenter_ctx.cache_mutex);
    return CORE_DEFRAGMENTER_ERROR_INVALID_HANDLE;
}

static void cache_store(uint32_t id, const void* data, size_t size) {
    pthread_mutex_lock(&g_defragmenter_ctx.cache_mutex);
    
    // Find empty slot or LRU victim
    uint32_t victim_index = 0;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < CORE_DEFRAGMENTER_CACHE_SIZE; i++) {
        if (!g_defragmenter_ctx.cache[i].valid) {
            victim_index = i;
            break;
        }
        if (g_defragmenter_ctx.cache[i].last_access < oldest_time) {
            oldest_time = g_defragmenter_ctx.cache[i].last_access;
            victim_index = i;
        }
    }
    
    // Free old entry if needed
    if (g_defragmenter_ctx.cache[victim_index].valid) {
        free(g_defragmenter_ctx.cache[victim_index].data);
    }
    
    // Store new entry
    g_defragmenter_ctx.cache[victim_index].id = id;
    g_defragmenter_ctx.cache[victim_index].data = malloc(size);
    if (g_defragmenter_ctx.cache[victim_index].data) {
        memcpy(g_defragmenter_ctx.cache[victim_index].data, data, size);
        g_defragmenter_ctx.cache[victim_index].size = size;
        g_defragmenter_ctx.cache[victim_index].last_access = get_timestamp_ms();
        g_defragmenter_ctx.cache[victim_index].access_count = 1;
        g_defragmenter_ctx.cache[victim_index].valid = true;
    }
    
    pthread_mutex_unlock(&g_defragmenter_ctx.cache_mutex);
}

/* SIMD-optimized memory copy */
static void simd_memcpy(void* dst, const void* src, size_t size) {
    if (g_defragmenter_ctx.simd_context.avx2_available && size >= 32) {
        // AVX2 optimized copy
        size_t avx_size = size & ~31; // Round down to 32-byte boundary
        __m256i* dst_avx = (__m256i*)dst;
        const __m256i* src_avx = (const __m256i*)src;
        
        for (size_t i = 0; i < avx_size / 32; i++) {
            _mm256_storeu_si256(&dst_avx[i], _mm256_loadu_si256(&src_avx[i]));
        }
        
        // Copy remaining bytes
        if (size % 32 != 0) {
            memcpy((uint8_t*)dst + avx_size, (const uint8_t*)src + avx_size, size % 32);
        }
        
        g_defragmenter_ctx.performance_counters.simd_operations++;
    } else {
        // Fallback to standard memcpy
        memcpy(dst, src, size);
    }
}

/* Validation layer functions */
static void validation_report(int level, const char* message) {
    if (g_defragmenter_ctx.validation_layer.validation_callback) {
        g_defragmenter_ctx.validation_layer.validation_callback(level, message);
    }
    
    if (level == 0) {
        g_defragmenter_ctx.validation_layer.validation_errors++;
    } else {
        g_defragmenter_ctx.validation_layer.validation_warnings++;
    }
}

static bool validate_item_strict(const core_defragmenter_internal_t* item) {
    if (!item) {
        validation_report(0, "Item is NULL");
        return false;
    }
    
    if (item->id >= CORE_DEFRAGMENTER_MAX_COUNT) {
        validation_report(0, "Item ID exceeds maximum");
        return false;
    }
    
    if (item->data_size > CORE_DEFRAGMENTER_MEMORY_BUDGET) {
        validation_report(0, "Item data size exceeds memory budget");
        return false;
    }
    
    if (item->state >= CORE_DEFRAGMENTER_STATE_ERROR) {
        validation_report(0, "Item is in error state");
        return false;
    }
    
    return true;
}

/* Render graph execution */
static void execute_render_graph_node(uint32_t node_id) {
    if (node_id >= g_defragmenter_ctx.render_graph_node_count) {
        return;
    }
    
    core_defragmenter_render_graph_node_t* node = &g_defragmenter_ctx.render_graph_nodes[node_id];
    if (!node->node_active || !node->execute_func) {
        return;
    }
    
    // Execute dependencies first
    for (uint32_t i = 0; i < node->dependency_count; i++) {
        execute_render_graph_node(node->dependencies[i]);
    }
    
    // Execute this node
    node->execute_func(node_id);
}

int core_defragmenter_init(void) {
    if (g_defragmenter_ctx.initialized) {
        return CORE_DEFRAGMENTER_SUCCESS; // Already initialized
    }

    // Initialize thread safety
    if (pthread_mutex_init(&g_defragmenter_ctx.global_mutex, NULL) != 0) {
        return CORE_DEFRAGMENTER_ERROR_THREADING;
    }
    
    if (pthread_rwlock_init(&g_defragmenter_ctx.context_lock, NULL) != 0) {
        pthread_mutex_destroy(&g_defragmenter_ctx.global_mutex);
        return CORE_DEFRAGMENTER_ERROR_THREADING;
    }

    // Initialize memory tracking
    g_defragmenter_ctx.memory_tracker.total_allocated = 0;
    g_defragmenter_ctx.memory_tracker.peak_usage = 0;
    g_defragmenter_ctx.memory_tracker.current_usage = 0;
    g_defragmenter_ctx.memory_tracker.allocation_count = 0;
    g_defragmenter_ctx.memory_tracker.leak_count = 0;
    g_defragmenter_ctx.memory_tracker.leak_detection_enabled = true;

    // Initialize performance counters
    memset(&g_defragmenter_ctx.performance_counters, 0, sizeof(g_defragmenter_ctx.performance_counters));

    // Initialize cache
    if (pthread_mutex_init(&g_defragmenter_ctx.cache_mutex, NULL) != 0) {
        pthread_mutex_destroy(&g_defragmenter_ctx.global_mutex);
        pthread_rwlock_destroy(&g_defragmenter_ctx.context_lock);
        return CORE_DEFRAGMENTER_ERROR_THREADING;
    }
    g_defragmenter_ctx.cache_usage = 0;
    memset(g_defragmenter_ctx.cache, 0, sizeof(g_defragmenter_ctx.cache));

    // Initialize async operations
    if (pthread_mutex_init(&g_defragmenter_ctx.async_mutex, NULL) != 0) {
        pthread_mutex_destroy(&g_defragmenter_ctx.global_mutex);
        pthread_rwlock_destroy(&g_defragmenter_ctx.context_lock);
        pthread_mutex_destroy(&g_defragmenter_ctx.cache_mutex);
        return CORE_DEFRAGMENTER_ERROR_THREADING;
    }
    
    if (pthread_cond_init(&g_defragmenter_ctx.async_cond, NULL) != 0) {
        pthread_mutex_destroy(&g_defragmenter_ctx.global_mutex);
        pthread_rwlock_destroy(&g_defragmenter_ctx.context_lock);
        pthread_mutex_destroy(&g_defragmenter_ctx.cache_mutex);
        pthread_mutex_destroy(&g_defragmenter_ctx.async_mutex);
        return CORE_DEFRAGMENTER_ERROR_THREADING;
    }
    
    g_defragmenter_ctx.async_count = 0;
    g_defragmenter_ctx.async_shutdown = false;
    memset(g_defragmenter_ctx.async_ops, 0, sizeof(g_defragmenter_ctx.async_ops));

    // Start worker threads
    for (int i = 0; i < CORE_DEFRAGMENTER_WORKER_THREADS; i++) {
        if (pthread_create(&g_defragmenter_ctx.worker_threads[i], NULL, worker_thread, NULL) != 0) {
            // Cleanup on failure
            g_defragmenter_ctx.async_shutdown = true;
            pthread_cond_broadcast(&g_defragmenter_ctx.async_cond);
            for (int j = 0; j < i; j++) {
                pthread_join(g_defragmenter_ctx.worker_threads[j], NULL);
            }
            pthread_mutex_destroy(&g_defragmenter_ctx.global_mutex);
            pthread_rwlock_destroy(&g_defragmenter_ctx.context_lock);
            pthread_mutex_destroy(&g_defragmenter_ctx.cache_mutex);
            pthread_mutex_destroy(&g_defragmenter_ctx.async_mutex);
            pthread_cond_destroy(&g_defragmenter_ctx.async_cond);
            return CORE_DEFRAGMENTER_ERROR_THREADING;
        }
    }

    // Initialize GPU context
    g_defragmenter_ctx.gpu_context.gpu_available = false;
    g_defragmenter_ctx.gpu_context.gpu_context = NULL;
    g_defragmenter_ctx.gpu_context.gpu_buffer_id = 0;
    g_defragmenter_ctx.gpu_context.gpu_memory_mapped = false;
    g_defragmenter_ctx.gpu_context.gpu_memory_size = 0;

    // Detect SIMD capabilities
    detect_simd_capabilities();

    // Initialize batch processing
    g_defragmenter_ctx.batch_context.batch_processing_enabled = true;
    g_defragmenter_ctx.batch_context.max_batch_size = 64;
    g_defragmenter_ctx.batch_context.batch_size = 0;
    g_defragmenter_ctx.batch_context.batch_items = malloc(g_defragmenter_ctx.batch_context.max_batch_size * sizeof(uint32_t));
    if (!g_defragmenter_ctx.batch_context.batch_items) {
        // Cleanup on failure
        g_defragmenter_ctx.async_shutdown = true;
        pthread_cond_broadcast(&g_defragmenter_ctx.async_cond);
        for (int i = 0; i < CORE_DEFRAGMENTER_WORKER_THREADS; i++) {
            pthread_join(g_defragmenter_ctx.worker_threads[i], NULL);
        }
        pthread_mutex_destroy(&g_defragmenter_ctx.global_mutex);
        pthread_rwlock_destroy(&g_defragmenter_ctx.context_lock);
        pthread_mutex_destroy(&g_defragmenter_ctx.cache_mutex);
        pthread_mutex_destroy(&g_defragmenter_ctx.async_mutex);
        pthread_cond_destroy(&g_defragmenter_ctx.async_cond);
        return CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY;
    }

    // Initialize streaming
    g_defragmenter_ctx.streaming_context.streaming_enabled = true;
    g_defragmenter_ctx.streaming_context.stream_buffer_size = 1024 * 1024; // 1MB
    g_defragmenter_ctx.streaming_context.stream_buffer = malloc(g_defragmenter_ctx.streaming_context.stream_buffer_size);
    g_defragmenter_ctx.streaming_context.stream_position = 0;
    g_defragmenter_ctx.streaming_context.stream_active = false;

    // Initialize LOD
    g_defragmenter_ctx.lod_context.current_lod = CORE_DEFRAGMENTER_LOD_HIGH;
    g_defragmenter_ctx.lod_context.lod_distances[0] = 100.0f;  // Low
    g_defragmenter_ctx.lod_context.lod_distances[1] = 50.0f;   // Medium
    g_defragmenter_ctx.lod_context.lod_distances[2] = 25.0f;   // High
    g_defragmenter_ctx.lod_context.lod_distances[3] = 10.0f;   // Ultra
    g_defragmenter_ctx.lod_context.lod_transitions_enabled = true;
    g_defragmenter_ctx.lod_context.lod_transition_speed = 2.0f;

    // Initialize culling
    g_defragmenter_ctx.culling_context.culling_enabled = true;
    g_defragmenter_ctx.culling_context.culling_distance = 200.0f;
    g_defragmenter_ctx.culling_context.culled_items_count = 0;
    g_defragmenter_ctx.culling_context.frustum_culling = true;
    g_defragmenter_ctx.culling_context.occlusion_culling = false;

    // Initialize render graph
    g_defragmenter_ctx.render_graph_node_count = 0;
    memset(g_defragmenter_ctx.render_graph_nodes, 0, sizeof(g_defragmenter_ctx.render_graph_nodes));

    // Initialize hot-reload file watching
    g_defragmenter_ctx.file_watcher.inotify_fd = inotify_init();
    if (g_defragmenter_ctx.file_watcher.inotify_fd == -1) {
        // Non-fatal, continue without hot-reload
        g_defragmenter_ctx.file_watcher.watcher_running = false;
    } else {
        g_defragmenter_ctx.file_watcher.watch_count = 0;
        g_defragmenter_ctx.file_watcher.watcher_running = false;
        g_defragmenter_ctx.file_watcher.reload_callback = NULL;
        memset(g_defragmenter_ctx.file_watcher.watch_descriptors, 0, sizeof(g_defragmenter_ctx.file_watcher.watch_descriptors));
        memset(g_defragmenter_ctx.file_watcher.watched_files, 0, sizeof(g_defragmenter_ctx.file_watcher.watched_files));
    }

    // Initialize validation layer
    g_defragmenter_ctx.validation_layer.validation_level = CORE_DEFRAGMENTER_VALIDATION_BASIC;
    g_defragmenter_ctx.validation_layer.validation_errors = 0;
    g_defragmenter_ctx.validation_layer.validation_warnings = 0;
    g_defragmenter_ctx.validation_layer.strict_mode = false;
    g_defragmenter_ctx.validation_layer.validation_callback = NULL;

    // Allocate items array
    g_defragmenter_ctx.capacity = CORE_DEFRAGMENTER_DEFAULT_CAPACITY;
    g_defragmenter_ctx.items = calloc(g_defragmenter_ctx.capacity, sizeof(core_defragmenter_internal_t));
    if (!g_defragmenter_ctx.items) {
        // Cleanup on failure
        free(g_defragmenter_ctx.batch_context.batch_items);
        free(g_defragmenter_ctx.streaming_context.stream_buffer);
        if (g_defragmenter_ctx.file_watcher.inotify_fd != -1) {
            close(g_defragmenter_ctx.file_watcher.inotify_fd);
        }
        g_defragmenter_ctx.async_shutdown = true;
        pthread_cond_broadcast(&g_defragmenter_ctx.async_cond);
        for (int i = 0; i < CORE_DEFRAGMENTER_WORKER_THREADS; i++) {
            pthread_join(g_defragmenter_ctx.worker_threads[i], NULL);
        }
        pthread_mutex_destroy(&g_defragmenter_ctx.global_mutex);
        pthread_rwlock_destroy(&g_defragmenter_ctx.context_lock);
        pthread_mutex_destroy(&g_defragmenter_ctx.cache_mutex);
        pthread_mutex_destroy(&g_defragmenter_ctx.async_mutex);
        pthread_cond_destroy(&g_defragmenter_ctx.async_cond);
        return CORE_DEFRAGMENTER_ERROR_OUT_OF_MEMORY;
    }

    // Initialize item mutexes
    for (uint32_t i = 0; i < g_defragmenter_ctx.capacity; i++) {
        if (pthread_mutex_init(&g_defragmenter_ctx.items[i].mutex, NULL) != 0) {
            // Cleanup on failure
            for (uint32_t j = 0; j < i; j++) {
                pthread_mutex_destroy(&g_defragmenter_ctx.items[j].mutex);
            }
            free(g_defragmenter_ctx.items);
            free(g_defragmenter_ctx.batch_context.batch_items);
            free(g_defragmenter_ctx.streaming_context.stream_buffer);
            if (g_defragmenter_ctx.file_watcher.inotify_fd != -1) {
                close(g_defragmenter_ctx.file_watcher.inotify_fd);
            }
            g_defragmenter_ctx.async_shutdown = true;
            pthread_cond_broadcast(&g_defragmenter_ctx.async_cond);
            for (int j = 0; j < CORE_DEFRAGMENTER_WORKER_THREADS; j++) {
                pthread_join(g_defragmenter_ctx.worker_threads[j], NULL);
            }
            pthread_mutex_destroy(&g_defragmenter_ctx.global_mutex);
            pthread_rwlock_destroy(&g_defragmenter_ctx.context_lock);
            pthread_mutex_destroy(&g_defragmenter_ctx.cache_mutex);
            pthread_mutex_destroy(&g_defragmenter_ctx.async_mutex);
            pthread_cond_destroy(&g_defragmenter_ctx.async_cond);
            return CORE_DEFRAGMENTER_ERROR_THREADING;
        }
    }

    g_defragmenter_ctx.count = 0;
    g_defragmenter_ctx.initialized = true;

    return CORE_DEFRAGMENTER_SUCCESS;
}

void core_defragmenter_shutdown(void) {
    if (!g_defragmenter_ctx.initialized) {
        return;
    }

    // Stop async operations
    g_defragmenter_ctx.async_shutdown = true;
    pthread_cond_broadcast(&g_defragmenter_ctx.async_cond);
    
    // Wait for worker threads to finish
    for (int i = 0; i < CORE_DEFRAGMENTER_WORKER_THREADS; i++) {
        pthread_join(g_defragmenter_ctx.worker_threads[i], NULL);
    }

    // Stop file watcher
    if (g_defragmenter_ctx.file_watcher.watcher_running) {
        g_defragmenter_ctx.file_watcher.watcher_running = false;
        pthread_join(g_defragmenter_ctx.file_watcher.watcher_thread, NULL);
    }

    // Cleanup all items
    for (uint32_t i = 0; i < g_defragmenter_ctx.count; i++) {
        core_defragmenter_cleanup_internal(&g_defragmenter_ctx.items[i]);
        pthread_mutex_destroy(&g_defragmenter_ctx.items[i].mutex);
    }

    // Cleanup cache
    pthread_mutex_lock(&g_defragmenter_ctx.cache_mutex);
    for (uint32_t i = 0; i < CORE_DEFRAGMENTER_CACHE_SIZE; i++) {
        if (g_defragmenter_ctx.cache[i].valid && g_defragmenter_ctx.cache[i].data) {
            free(g_defragmenter_ctx.cache[i].data);
            g_defragmenter_ctx.cache[i].data = NULL;
        }
    }
    pthread_mutex_unlock(&g_defragmenter_ctx.cache_mutex);

    // Cleanup GPU resources
    if (g_defragmenter_ctx.gpu_context.gpu_memory_mapped) {
        // GPU memory cleanup would go here
        g_defragmenter_ctx.gpu_context.gpu_memory_mapped = false;
    }

    // Cleanup streaming buffer
    if (g_defragmenter_ctx.streaming_context.stream_buffer) {
        free(g_defragmenter_ctx.streaming_context.stream_buffer);
        g_defragmenter_ctx.streaming_context.stream_buffer = NULL;
    }

    // Cleanup batch processing
    if (g_defragmenter_ctx.batch_context.batch_items) {
        free(g_defragmenter_ctx.batch_context.batch_items);
        g_defragmenter_ctx.batch_context.batch_items = NULL;
    }

    // Cleanup render graph
    for (uint32_t i = 0; i < g_defragmenter_ctx.render_graph_node_count; i++) {
        if (g_defragmenter_ctx.render_graph_nodes[i].dependencies) {
            free(g_defragmenter_ctx.render_graph_nodes[i].dependencies);
            g_defragmenter_ctx.render_graph_nodes[i].dependencies = NULL;
        }
    }

    // Cleanup file watcher
    if (g_defragmenter_ctx.file_watcher.inotify_fd != -1) {
        for (uint32_t i = 0; i < g_defragmenter_ctx.file_watcher.watch_count; i++) {
            if (g_defragmenter_ctx.file_watcher.watch_descriptors[i] != 0) {
                inotify_rm_watch(g_defragmenter_ctx.file_watcher.inotify_fd, 
                                g_defragmenter_ctx.file_watcher.watch_descriptors[i]);
            }
            if (g_defragmenter_ctx.file_watcher.watched_files[i]) {
                free(g_defragmenter_ctx.file_watcher.watched_files[i]);
                g_defragmenter_ctx.file_watcher.watched_files[i] = NULL;
            }
        }
        close(g_defragmenter_ctx.file_watcher.inotify_fd);
        g_defragmenter_ctx.file_watcher.inotify_fd = -1;
    }

    // Check for memory leaks
    if (g_defragmenter_ctx.memory_tracker.leak_detection_enabled && 
        g_defragmenter_ctx.memory_tracker.current_usage > 0) {
        g_defragmenter_ctx.memory_tracker.leak_count++;
        // Report leak detection would go here
    }

    // Free main items array
    free(g_defragmenter_ctx.items);
    g_defragmenter_ctx.items = NULL;

    // Destroy synchronization objects
    pthread_mutex_destroy(&g_defragmenter_ctx.global_mutex);
    pthread_rwlock_destroy(&g_defragmenter_ctx.context_lock);
    pthread_mutex_destroy(&g_defragmenter_ctx.cache_mutex);
    pthread_mutex_destroy(&g_defragmenter_ctx.async_mutex);
    pthread_cond_destroy(&g_defragmenter_ctx.async_cond);

    // Reset all state
    g_defragmenter_ctx.count = 0;
    g_defragmenter_ctx.capacity = 0;
    g_defragmenter_ctx.cache_usage = 0;
    g_defragmenter_ctx.async_count = 0;
    g_defragmenter_ctx.render_graph_node_count = 0;
    g_defragmenter_ctx.file_watcher.watch_count = 0;
    g_defragmenter_ctx.initialized = false;

    // Print final statistics
    if (g_defragmenter_ctx.performance_counters.operations_processed > 0) {
        // Final statistics reporting would go here
    }
}

int core_defragmenter_create(core_defragmenter_handle_t* out_handle, const core_defragmenter_desc_t* desc) {
    // TODO: Implement defragmenter validation
    // TODO: Add defragmenter error handling
    // TODO: Implement defragmenter serialization
    // TODO: Add defragmenter debug output

    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_defragmenter_ctx.initialized) {
        return -2;
    }

    if (g_defragmenter_ctx.count >= g_defragmenter_ctx.capacity) {
        // TODO: Implement defragmenter unit tests
        return -3;
    }

    uint32_t index = g_defragmenter_ctx.count++;
    core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[index];

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

void core_defragmenter_destroy(core_defragmenter_handle_t handle) {
    // TODO: Add defragmenter performance counters
    // TODO: Implement defragmenter hot-reload

    if (handle.id >= g_defragmenter_ctx.count) {
        return;
    }

    core_defragmenter_cleanup_internal(&g_defragmenter_ctx.items[handle.id]);
}

int core_defragmenter_update(core_defragmenter_handle_t handle, const void* data, size_t size) {
    // TODO: Add defragmenter thread safety
    // TODO: Implement defragmenter memory pooling
    // TODO: Add defragmenter caching layer
    // TODO: Implement defragmenter async operations

    if (handle.id >= g_defragmenter_ctx.count) {
        return -1;
    }

    core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // TODO: Add defragmenter GPU integration
    // TODO: Implement defragmenter SIMD optimization

    item->dirty = true;
    return 0;
}

bool core_defragmenter_is_valid(core_defragmenter_handle_t handle) {
    // TODO: Add defragmenter batch processing
    if (handle.id >= g_defragmenter_ctx.count) {
        return false;
    }
    return g_defragmenter_ctx.items[handle.id].initialized;
}

int core_defragmenter_get_info(core_defragmenter_handle_t handle, core_defragmenter_info_t* out_info) {
    // TODO: Implement defragmenter streaming support
    // TODO: Add defragmenter LOD support

    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_defragmenter_ctx.count) {
        return -2;
    }

    const core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void core_defragmenter_mark_dirty(core_defragmenter_handle_t handle) {
    // TODO: Implement defragmenter culling integration
    if (handle.id < g_defragmenter_ctx.count) {
        g_defragmenter_ctx.items[handle.id].dirty = true;
    }
}

int core_defragmenter_process_pending(void) {
    // TODO: Add defragmenter render graph node
    // TODO: Implement batch processing

    int processed = 0;
    for (uint32_t i = 0; i < g_defragmenter_ctx.count; i++) {
        core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t core_defragmenter_get_count(void) {
    return g_defragmenter_ctx.count;
}

size_t core_defragmenter_get_memory_usage(void) {
    // TODO: Implement memory tracking
    size_t total = sizeof(g_defragmenter_ctx);
    total += g_defragmenter_ctx.capacity * sizeof(core_defragmenter_internal_t);

    for (uint32_t i = 0; i < g_defragmenter_ctx.count; i++) {
        total += g_defragmenter_ctx.items[i].data_size;
    }

    return total;
}

void core_defragmenter_debug_print(void) {
    printf("=== Defragmenter Debug Information ===\n");
    printf("Initialized: %s\n", g_defragmenter_ctx.initialized ? "Yes" : "No");
    printf("Count: %u / %u\n", g_defragmenter_ctx.count, g_defragmenter_ctx.capacity);
    printf("Backend Support: Vulkan=%s, Metal=%s, D3D12=%s\n",
           g_defragmenter_ctx.vulkan_enabled ? "Yes" : "No",
           g_defragmenter_ctx.metal_enabled ? "Yes" : "No",
           g_defragmenter_ctx.d3d12_enabled ? "Yes" : "No");
    printf("Batch Processing: %s\n", g_defragmenter_ctx.batch_processing_enabled ? "Enabled" : "Disabled");
    
    // Performance statistics
    printf("\n--- Performance Statistics ---\n");
    printf("Total Defragmentations: %llu\n", g_defragmenter_ctx.stats.total_defragmentations);
    printf("Total Bytes Moved: %llu\n", g_defragmenter_ctx.stats.total_bytes_moved);
    printf("Total Time: %llu ms\n", g_defragmenter_ctx.stats.total_time_ms);
    printf("Average Efficiency: %.2f%%\n", g_defragmenter_ctx.stats.average_efficiency);
    
    // Individual item information
    printf("\n--- Individual Items ---\n");
    for (uint32_t i = 0; i < g_defragmenter_ctx.count; i++) {
        const core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[i];
        if (item->initialized) {
            printf("Item %u: backend=%u, frag_level=%u, dirty=%s\n",
                   item->id, item->backend_type, item->fragmentation_level,
                   item->dirty ? "Yes" : "No");
        }
    }
    
    printf("===================================\n");
}

// ============================================================================
// BACKEND IMPLEMENTATIONS (TODO-30431, TODO-30432, TODO-30433)
// ============================================================================

#ifdef ENABLE_VULKAN
// Vulkan backend implementation (TODO-30431)
static int vulkan_defragmenter_init(void) {
    vulkan_defragmenter_backend_t* backend = &g_defragmenter_ctx.vulkan_backend;
    
    // Initialize Vulkan device and related resources
    // This is a simplified implementation - in practice would need full Vulkan setup
    backend->device = VK_NULL_HANDLE;
    backend->physical_device = VK_NULL_HANDLE;
    backend->command_pool = VK_NULL_HANDLE;
    backend->transfer_queue = VK_NULL_HANDLE;
    backend->vma_allocator = VK_NULL_HANDLE;
    backend->initialized = false;
    
    // Check Vulkan availability
    VkResult result = vkCreateDevice(VK_NULL_HANDLE, NULL, NULL, &backend->device);
    if (result != VK_SUCCESS) {
        LOG_WARN("Vulkan backend not available");
        return -1;
    }
    
    backend->initialized = true;
    g_defragmenter_ctx.vulkan_enabled = true;
    
    LOG_INFO("Vulkan defragmenter backend initialized");
    return 0;
}

static void vulkan_defragmenter_shutdown(void) {
    vulkan_defragmenter_backend_t* backend = &g_defragmenter_ctx.vulkan_backend;
    
    if (backend->device != VK_NULL_HANDLE) {
        vkDestroyDevice(backend->device, NULL);
    }
    
    if (backend->command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(backend->device, backend->command_pool, NULL);
    }
    
    if (backend->vma_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(backend->vma_allocator);
    }
    
    memset(backend, 0, sizeof(vulkan_defragmenter_backend_t));
    g_defragmenter_ctx.vulkan_enabled = false;
    
    LOG_INFO("Vulkan defragmenter backend shutdown");
}

static int vulkan_defragmenter_defragment(void* memory, size_t size) {
    vulkan_defragmenter_backend_t* backend = &g_defragmenter_ctx.vulkan_backend;
    
    if (!backend->initialized) {
        return -1;
    }
    
    // Use Vulkan compute shader for GPU-based defragmentation
    // This is a placeholder - actual implementation would use compute shaders
    LOG_DEBUG("Vulkan GPU defragmentation for %zu bytes", size);
    
    return 0;
}
#endif

#ifdef ENABLE_METAL
// Metal backend implementation (TODO-30432)
static int metal_defragmenter_init(void) {
    metal_defragmenter_backend_t* backend = &g_defragmenter_ctx.metal_backend;
    
    // Get Metal device
    backend->device = MTLCreateSystemDefaultDevice();
    if (!backend->device) {
        LOG_WARN("Metal backend not available");
        return -1;
    }
    
    // Create command queue
    backend->command_queue = [backend->device newCommandQueue];
    if (!backend->command_queue) {
        LOG_ERROR("Failed to create Metal command queue");
        return -2;
    }
    
    backend->staging_buffer = nil;
    backend->initialized = true;
    g_defragmenter_ctx.metal_enabled = true;
    
    LOG_INFO("Metal defragmenter backend initialized");
    return 0;
}

static void metal_defragmenter_shutdown(void) {
    metal_defragmenter_backend_t* backend = &g_defragmenter_ctx.metal_backend;
    
    if (backend->staging_buffer) {
        [backend->staging_buffer release];
    }
    
    if (backend->command_queue) {
        [backend->command_queue release];
    }
    
    if (backend->device) {
        [backend->device release];
    }
    
    memset(backend, 0, sizeof(metal_defragmenter_backend_t));
    g_defragmenter_ctx.metal_enabled = false;
    
    LOG_INFO("Metal defragmenter backend shutdown");
}

static int metal_defragmenter_defragment(void* memory, size_t size) {
    metal_defragmenter_backend_t* backend = &g_defragmenter_ctx.metal_backend;
    
    if (!backend->initialized) {
        return -1;
    }
    
    // Use Metal compute shader for GPU-based defragmentation
    LOG_DEBUG("Metal GPU defragmentation for %zu bytes", size);
    
    return 0;
}
#endif

#ifdef ENABLE_D3D12
// D3D12 backend implementation (TODO-30433)
static int d3d12_defragmenter_init(void) {
    d3d12_defragmenter_backend_t* backend = &g_defragmenter_ctx.d3d12_backend;
    
    HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, 
                                  IID_PPV_ARGS(&backend->device));
    if (FAILED(hr)) {
        LOG_WARN("D3D12 backend not available");
        return -1;
    }
    
    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queue_desc = {0};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;
    
    hr = backend->device->CreateCommandQueue(&queue_desc, 
                                               IID_PPV_ARGS(&backend->command_queue));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create D3D12 command queue");
        backend->device->Release();
        return -2;
    }
    
    backend->staging_buffer = nullptr;
    backend->initialized = true;
    g_defragmenter_ctx.d3d12_enabled = true;
    
    LOG_INFO("D3D12 defragmenter backend initialized");
    return 0;
}

static void d3d12_defragmenter_shutdown(void) {
    d3d12_defragmenter_backend_t* backend = &g_defragmenter_ctx.d3d12_backend;
    
    if (backend->staging_buffer) {
        backend->staging_buffer->Release();
    }
    
    if (backend->command_queue) {
        backend->command_queue->Release();
    }
    
    if (backend->device) {
        backend->device->Release();
    }
    
    memset(backend, 0, sizeof(d3d12_defragmenter_backend_t));
    g_defragmenter_ctx.d3d12_enabled = false;
    
    LOG_INFO("D3D12 defragmenter backend shutdown");
}

static int d3d12_defragmenter_defragment(void* memory, size_t size) {
    d3d12_defragmenter_backend_t* backend = &g_defragmenter_ctx.d3d12_backend;
    
    if (!backend->initialized) {
        return -1;
    }
    
    // Use D3D12 compute shader for GPU-based defragmentation
    LOG_DEBUG("D3D12 GPU defragmentation for %zu bytes", size);
    
    return 0;
}
#endif

// Public backend API functions
int core_defragmenter_init_backends(uint32_t backend_flags) {
    int result = 0;
    
    if (backend_flags & CORE_DEFRAGMENTER_BACKEND_VULKAN) {
#ifdef ENABLE_VULKAN
        if (vulkan_defragmenter_init() != 0) {
            result |= (1 << 0);
        }
#endif
    }
    
    if (backend_flags & CORE_DEFRAGMENTER_BACKEND_METAL) {
#ifdef ENABLE_METAL
        if (metal_defragmenter_init() != 0) {
            result |= (1 << 1);
        }
#endif
    }
    
    if (backend_flags & CORE_DEFRAGMENTER_BACKEND_D3D12) {
#ifdef ENABLE_D3D12
        if (d3d12_defragmenter_init() != 0) {
            result |= (1 << 2);
        }
#endif
    }
    
    return result;
}

void core_defragmenter_shutdown_backends(void) {
#ifdef ENABLE_VULKAN
    if (g_defragmenter_ctx.vulkan_enabled) {
        vulkan_defragmenter_shutdown();
    }
#endif

#ifdef ENABLE_METAL
    if (g_defragmenter_ctx.metal_enabled) {
        metal_defragmenter_shutdown();
    }
#endif

#ifdef ENABLE_D3D12
    if (g_defragmenter_ctx.d3d12_enabled) {
        d3d12_defragmenter_shutdown();
    }
#endif
}

int core_defragmenter_get_available_backends(void) {
    int backends = 0;
    
#ifdef ENABLE_VULKAN
    backends |= CORE_DEFRAGMENTER_BACKEND_VULKAN;
#endif
#ifdef ENABLE_METAL
    backends |= CORE_DEFRAGMENTER_BACKEND_METAL;
#endif
#ifdef ENABLE_D3D12
    backends |= CORE_DEFRAGMENTER_BACKEND_D3D12;
#endif
    
    return backends;
}

int core_defragmenter_set_backend(core_defragmenter_handle_t handle, uint32_t backend_type) {
    if (handle.id >= g_defragmenter_ctx.count) {
        return -1;
    }
    
    core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    // Check if backend is available
    switch (backend_type) {
        case CORE_DEFRAGMENTER_BACKEND_VULKAN:
            if (!g_defragmenter_ctx.vulkan_enabled) {
                return -3;
            }
            break;
        case CORE_DEFRAGMENTER_BACKEND_METAL:
            if (!g_defragmenter_ctx.metal_enabled) {
                return -3;
            }
            break;
        case CORE_DEFRAGMENTER_BACKEND_D3D12:
            if (!g_defragmenter_ctx.d3d12_enabled) {
                return -3;
            }
            break;
        default:
            return -4;
    }
    
    item->backend_type = backend_type;
    item->backend_handle = NULL; // Would be set to actual backend handle
    
    return 0;
}

int core_defragmenter_run_gpu_defragmentation(core_defragmenter_handle_t handle) {
    if (handle.id >= g_defragmenter_ctx.count) {
        return -1;
    }
    
    core_defragmenter_internal_t* item = &g_defragmenter_ctx.items[handle.id];
    if (!item->initialized || !item->data) {
        return -2;
    }
    
    switch (item->backend_type) {
#ifdef ENABLE_VULKAN
        case CORE_DEFRAGMENTER_BACKEND_VULKAN:
            return vulkan_defragmenter_defragment(item->data, item->data_size);
#endif
#ifdef ENABLE_METAL
        case CORE_DEFRAGMENTER_BACKEND_METAL:
            return metal_defragmenter_defragment(item->data, item->data_size);
#endif
#ifdef ENABLE_D3D12
        case CORE_DEFRAGMENTER_BACKEND_D3D12:
            return d3d12_defragmenter_defragment(item->data, item->data_size);
#endif
        default:
            // CPU defragmentation fallback
            return core_defragmenter_process_pending();
    }
}

/* End of defragmenter.c */
