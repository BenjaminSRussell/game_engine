/*
 * cloth_tearing.c
 * Cloth tearing
 *
 * Part of the Cloth System subsystem
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
 * TODO: Implement cloth tearing initialization
 * TODO: Add cloth tearing cleanup/shutdown
 * TODO: Implement cloth tearing validation
 * TODO: Add cloth tearing error handling
 * TODO: Implement cloth tearing serialization
 * TODO: Add cloth tearing debug output
 * TODO: Implement cloth tearing unit tests
 * TODO: Add cloth tearing performance counters
 * TODO: Implement cloth tearing hot-reload
 * TODO: Add cloth tearing thread safety
 * TODO: Implement cloth tearing memory pooling
 * TODO: Add cloth tearing caching layer
 * TODO: Implement cloth tearing async operations
 * TODO: Add cloth tearing GPU integration
 * TODO: Implement cloth tearing SIMD optimization
 * TODO: Add cloth tearing batch processing
 * TODO: Implement cloth tearing streaming support
 * TODO: Add cloth tearing LOD support
 * TODO: Implement cloth tearing culling integration
 * TODO: Add cloth tearing render graph node
 */

#include "character/cloth/simulation_cloth/cloth_tearing.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <d3d12.h>
#endif

#ifdef __linux__
#include <sys/inotify.h>
#include <unistd.h>
#endif
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <immintrin.h>
#include <vulkan/vulkan.h>
#include <Metal/Metal.h>
#include <d3d12.h>
#include <time.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CLOTH_SYSTEM_CLOTH_TEARING_MAX_COUNT 4096
#define CLOTH_SYSTEM_CLOTH_TEARING_DEFAULT_CAPACITY 256
#define CLOTH_SYSTEM_CLOTH_TEARING_ALIGNMENT 16
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_BACKEND_VULKAN (1u << 0)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_BACKEND_METAL (1u << 1)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_BACKEND_D3D12 (1u << 2)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_THREAD_SAFE (1u << 3)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_SIMD_OPTIMIZATION (1u << 4)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_ASYNC_OPERATIONS (1u << 5)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_GPU_INTEGRATION (1u << 6)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_BATCH_PROCESSING (1u << 7)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_STREAMING_SUPPORT (1u << 8)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_LOD_SUPPORT (1u << 9)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_CULLING_INTEGRATION (1u << 10)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_RENDER_GRAPH_NODE (1u << 11)

/* Error codes */
#define CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE 0
#define CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM -1
#define CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NOT_INITIALIZED -2
#define CLOTH_SYSTEM_CLOTH_TEARING_ERROR_OUT_OF_MEMORY -3
#define CLOTH_SYSTEM_CLOTH_TEARING_ERROR_BACKEND_FAILED -4
#define CLOTH_SYSTEM_CLOTH_TEARING_ERROR_THREADING -5
#define CLOTH_SYSTEM_CLOTH_TEARING_ERROR_VALIDATION -6
#define CLOTH_SYSTEM_CLOTH_TEARING_ERROR_SERIALIZATION -7
#define CLOTH_SYSTEM_CLOTH_TEARING_ERROR_HOT_RELOAD -8
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_BACKEND_D3D12 (1u << 2)
#define CLOTH_SYSTEM_CLOTH_TEARING_CACHE_SIZE 1024
#define CLOTH_SYSTEM_CLOTH_TEARING_MAX_ASYNC_OPERATIONS 64
#define CLOTH_SYSTEM_CLOTH_TEARING_MAX_BATCH_SIZE 128
#define CLOTH_SYSTEM_CLOTH_TEARING_MAX_LOD_LEVELS 8
#define CLOTH_SYSTEM_CLOTH_TEARING_MEMORY_POOL_SIZE (16 * 1024 * 1024)
#define CLOTH_SYSTEM_CLOTH_TEARING_FLAG_BACKEND_D3D12 (1u << 2)
#define CLOTH_SYSTEM_CLOTH_TEARING_MAGIC_NUMBER 0x54454152  /* 'TEAR' */
#define CLOTH_SYSTEM_CLOTH_TEARING_VERSION 1
#define CLOTH_SYSTEM_CLOTH_TEARING_MAX_WATCHES 128
#define CLOTH_SYSTEM_CLOTH_TEARING_CACHE_SIZE 64
#define CLOTH_SYSTEM_CLOTH_TEARING_MAX_ASYNC_OPS 32

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum cloth_system_cloth_tearing_backend {
    CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_CPU = 0,
    CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_VULKAN,
    CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_METAL,
    CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_D3D12
} cloth_system_cloth_tearing_backend_t;

typedef enum cloth_system_cloth_tearing_error {
    CLOTH_SYSTEM_CLOTH_TEARING_SUCCESS = 0,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM = -1,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NOT_INITIALIZED = -2,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_OUT_OF_MEMORY = -3,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_BACKEND_INIT_FAILED = -4,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_GPU_OPERATION_FAILED = -5,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_ASYNC_OPERATION_FAILED = -6,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_CACHE_MISS = -7,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_THREAD_SAFETY_VIOLATION = -8
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_VALIDATION_FAILED = -5,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_SERIALIZATION_FAILED = -6,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_THREAD_SAFETY_VIOLATION = -7
} cloth_system_cloth_tearing_error_t;

typedef struct cloth_system_cloth_tearing_backend_ctx {
    uint32_t version;
    uint64_t last_frame;
#ifdef _WIN32
    ID3D12Device* d3d12_device;
    ID3D12CommandQueue* d3d12_queue;
#endif
    void* vulkan_instance;
    void* metal_device;
} cloth_system_cloth_tearing_backend_ctx_t;

typedef struct cloth_system_cloth_tearing_memory_stats {
    size_t total_allocated;
    size_t peak_usage;
    uint32_t allocation_count;
    uint32_t leak_count;
} cloth_system_cloth_tearing_memory_stats_t;

typedef struct cloth_system_cloth_tearing_performance_counters {
    uint64_t frames_processed;
    uint64_t tears_detected;
    uint64_t tears_processed;
    double avg_processing_time;
    uint64_t last_update_time;
} cloth_system_cloth_tearing_performance_counters_t;

typedef struct cloth_system_cloth_tearing_hot_reload {
    bool enabled;
    int file_watch_fd;
    pthread_t watch_thread;
    bool watch_thread_running;
    char watch_directory[256];
} cloth_system_cloth_tearing_hot_reload_t;

typedef struct cloth_system_cloth_tearing_cache_entry {
    uint32_t id;
    void* data;
    size_t size;
    uint64_t timestamp;
    uint32_t access_count;
    bool valid;
} cloth_system_cloth_tearing_cache_entry_t;

typedef struct cloth_system_cloth_tearing_async_operation {
    uint32_t id;
    bool active;
    pthread_t thread;
    void* input_data;
    size_t input_size;
    void* output_data;
    size_t output_size;
    int status;
    void (*callback)(void*, void*);
    void* user_data;
} cloth_system_cloth_tearing_async_operation_t;

typedef struct cloth_system_cloth_tearing_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    cloth_system_cloth_tearing_backend_t backend;
    cloth_system_cloth_tearing_backend_ctx_t* backend_ctx;
    
    /* Extended features */
    pthread_mutex_t mutex;
    bool thread_safe;
    cloth_system_cloth_tearing_cache_entry_t* cache;
    uint32_t cache_size;
    cloth_system_cloth_tearing_async_operation_t* async_ops;
    uint32_t async_ops_count;
    void* gpu_buffer;
    size_t gpu_buffer_size;
    uint32_t lod_level;
    bool culling_enabled;
    bool render_graph_node;
} cloth_system_cloth_tearing_internal_t;

typedef struct cloth_system_cloth_tearing_context {
    cloth_system_cloth_tearing_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    pthread_mutex_t mutex;
    cloth_system_cloth_tearing_memory_stats_t memory_stats;
    cloth_system_cloth_tearing_performance_counters_t perf_counters;
    cloth_system_cloth_tearing_hot_reload_t hot_reload;
    bool hot_reload_enabled;
    bool validation_enabled;
    size_t total_memory_allocated;
    size_t peak_memory_usage;
} cloth_system_cloth_tearing_context_t;

static cloth_system_cloth_tearing_context_t g_cloth_tearing_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static cloth_system_cloth_tearing_backend_t cloth_system_cloth_tearing_select_backend(uint32_t flags) {
    if (flags & CLOTH_SYSTEM_CLOTH_TEARING_FLAG_BACKEND_VULKAN) {
        return CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_VULKAN;
    }
    if (flags & CLOTH_SYSTEM_CLOTH_TEARING_FLAG_BACKEND_METAL) {
        return CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_METAL;
    }
    if (flags & CLOTH_SYSTEM_CLOTH_TEARING_FLAG_BACKEND_D3D12) {
        return CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_D3D12;
    }
    return CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_CPU;
}

static const char* cloth_system_cloth_tearing_error_string(cloth_system_cloth_tearing_error_t error) {
    switch (error) {
        case CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE: return "Success";
        case CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM: return "Invalid parameter";
        case CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NOT_INITIALIZED: return "Not initialized";
        case CLOTH_SYSTEM_CLOTH_TEARING_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case CLOTH_SYSTEM_CLOTH_TEARING_ERROR_BACKEND_FAILED: return "Backend failed";
        case CLOTH_SYSTEM_CLOTH_TEARING_ERROR_VALIDATION_FAILED: return "Validation failed";
        case CLOTH_SYSTEM_CLOTH_TEARING_ERROR_SERIALIZATION_FAILED: return "Serialization failed";
        case CLOTH_SYSTEM_CLOTH_TEARING_ERROR_THREAD_SAFETY_VIOLATION: return "Thread safety violation";
        default: return "Unknown error";
    }
}

static uint32_t cloth_system_cloth_tearing_calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ bytes[i];
    }
    return checksum;
}

static void* cloth_system_cloth_tearing_file_watch_thread(void* arg) {
    cloth_system_cloth_tearing_context_t* ctx = (cloth_system_cloth_tearing_context_t*)arg;
    if (!ctx) return NULL;
    
    char buffer[4096];
    while (ctx->hot_reload.watch_thread_running) {
        int length = read(ctx->hot_reload.file_watch_fd, buffer, sizeof(buffer));
        if (length > 0) {
            int i = 0;
            while (i < length) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                if (event->mask & IN_MODIFY) {
                    /* File modified - trigger hot reload */
                    pthread_mutex_lock(&ctx->mutex);
                    /* Mark all items as dirty for reprocessing */
                    for (uint32_t j = 0; j < ctx->count; j++) {
                        ctx->items[j].dirty = true;
                    }
                    pthread_mutex_unlock(&ctx->mutex);
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); /* 100ms */
    }
    
    return NULL;
}

static int cloth_system_cloth_tearing_init_hot_reload(cloth_system_cloth_tearing_context_t* ctx) {
    if (!ctx) return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM;
    
    ctx->hot_reload.file_watch_fd = inotify_init();
    if (ctx->hot_reload.file_watch_fd < 0) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_HOT_RELOAD;
    }
    
    /* Watch current directory for changes */
    int wd = inotify_add_watch(ctx->hot_reload.file_watch_fd, ".", 
                               IN_MODIFY | IN_CREATE | IN_DELETE);
    if (wd < 0) {
        close(ctx->hot_reload.file_watch_fd);
        ctx->hot_reload.file_watch_fd = -1;
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_HOT_RELOAD;
    }
    
    ctx->hot_reload.watch_thread_running = true;
    if (pthread_create(&ctx->hot_reload.watch_thread, NULL, 
                      cloth_system_cloth_tearing_file_watch_thread, ctx) != 0) {
        close(ctx->hot_reload.file_watch_fd);
        ctx->hot_reload.file_watch_fd = -1;
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_THREADING;
    }
    
    ctx->hot_reload.enabled = true;
    return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE;
}

static void cloth_system_cloth_tearing_shutdown_hot_reload(cloth_system_cloth_tearing_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->hot_reload.watch_thread_running) {
        ctx->hot_reload.watch_thread_running = false;
        pthread_join(ctx->hot_reload.watch_thread, NULL);
    }
    
    if (ctx->hot_reload.file_watch_fd >= 0) {
        close(ctx->hot_reload.file_watch_fd);
        ctx->hot_reload.file_watch_fd = -1;
    }
    
    ctx->hot_reload.enabled = false;
}

static void* cloth_system_cloth_tearing_async_worker_thread(void* arg) {
    cloth_system_cloth_tearing_async_operation_t* op = (cloth_system_cloth_tearing_async_operation_t*)arg;
    if (!op) return NULL;
    
    /* Simulate async processing */
    usleep(1000); /* 1ms delay */
    
    if (op->callback) {
        op->callback(op->output_data, op->user_data);
    }
    
    op->active = false;
    return NULL;
}

static int cloth_system_cloth_tearing_init_cache(cloth_system_cloth_tearing_internal_t* item) {
    if (!item) return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM;
    
    item->cache = calloc(CLOTH_SYSTEM_CLOTH_TEARING_CACHE_SIZE, sizeof(cloth_system_cloth_tearing_cache_entry_t));
    if (!item->cache) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_OUT_OF_MEMORY;
    }
    
    item->cache_size = CLOTH_SYSTEM_CLOTH_TEARING_CACHE_SIZE;
    return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE;
}

static void cloth_system_cloth_tearing_cleanup_cache(cloth_system_cloth_tearing_internal_t* item) {
    if (!item || !item->cache) return;
    
    for (uint32_t i = 0; i < item->cache_size; i++) {
        if (item->cache[i].data) {
            free(item->cache[i].data);
            item->cache[i].data = NULL;
        }
    }
    
    free(item->cache);
    item->cache = NULL;
    item->cache_size = 0;
}

static int cloth_system_cloth_tearing_init_async_ops(cloth_system_cloth_tearing_internal_t* item) {
    if (!item) return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM;
    
    item->async_ops = calloc(CLOTH_SYSTEM_CLOTH_TEARING_MAX_ASYNC_OPS, sizeof(cloth_system_cloth_tearing_async_operation_t));
    if (!item->async_ops) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_OUT_OF_MEMORY;
    }
    
    item->async_ops_count = CLOTH_SYSTEM_CLOTH_TEARING_MAX_ASYNC_OPS;
    return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE;
}

static void cloth_system_cloth_tearing_cleanup_async_ops(cloth_system_cloth_tearing_internal_t* item) {
    if (!item || !item->async_ops) return;
    
    for (uint32_t i = 0; i < item->async_ops_count; i++) {
        if (item->async_ops[i].active) {
            pthread_join(item->async_ops[i].thread, NULL);
        }
        if (item->async_ops[i].input_data) {
            free(item->async_ops[i].input_data);
        }
        if (item->async_ops[i].output_data) {
            free(item->async_ops[i].output_data);
        }
    }
    
    free(item->async_ops);
    item->async_ops = NULL;
    item->async_ops_count = 0;
}

static int cloth_system_cloth_tearing_backend_init(cloth_system_cloth_tearing_internal_t* item, cloth_system_cloth_tearing_backend_t backend) {
    if (!item) return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM;
    
    item->backend = backend;
    
    switch (backend) {
        case CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_CPU:
            item->backend_ctx = NULL;
            return CLOTH_SYSTEM_CLOTH_TEARING_SUCCESS;
            
        case CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_VULKAN: {
            item->backend_ctx = calloc(1, sizeof(cloth_system_cloth_tearing_backend_ctx_t));
            if (!item->backend_ctx) {
                return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_OUT_OF_MEMORY;
            }
            
            /* Initialize Vulkan backend */
            VkApplicationInfo app_info = {0};
            app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.pApplicationName = "Cloth Tearing System";
            app_info.apiVersion = VK_API_VERSION_1_0;
            
            VkInstanceCreateInfo create_info = {0};
            create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            create_info.pApplicationInfo = &app_info;
            
            VkResult result = vkCreateInstance(&create_info, NULL, (VkInstance*)&item->backend_ctx->vulkan_instance);
            if (result != VK_SUCCESS) {
                free(item->backend_ctx);
                item->backend_ctx = NULL;
                return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_BACKEND_FAILED;
            }
            
            item->backend_ctx->version = 1;
            item->backend_ctx->last_frame = 0;
            return CLOTH_SYSTEM_CLOTH_TEARING_SUCCESS;
        }
        item->backend_ctx->vulkan_instance = (void*)0x12345678; // Placeholder
    } else if (backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_METAL) {
        item->backend_ctx->metal_device = (void*)0x87654321; // Placeholder
    } else if (backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_D3D12) {
#ifdef _WIN32
        HRESULT result = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_0, 
                                       IID_PPV_ARGS(&item->backend_ctx->d3d12_device));
        if (FAILED(result)) {
            free(item->backend_ctx);
            item->backend_ctx = NULL;
            return -4;
        }
        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queue_desc.NodeMask = 0;
        
        result = item->backend_ctx->d3d12_device->lpVtbl->CreateCommandQueue(
            item->backend_ctx->d3d12_device, &queue_desc, 
            IID_PPV_ARGS(&item->backend_ctx->d3d12_queue));
        if (FAILED(result)) {
            item->backend_ctx->d3d12_device->lpVtbl->Release(item->backend_ctx->d3d12_device);
            free(item->backend_ctx);
            item->backend_ctx = NULL;
            return -5;
        }
#endif
    }
    
    return 0;
}

static void cloth_system_cloth_tearing_backend_shutdown(cloth_system_cloth_tearing_internal_t* item) {
    if (!item || !item->backend_ctx) return;
    
    if (item->backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_VULKAN) {
        if (item->backend_ctx->vulkan_instance) {
            // vkDestroyInstance((VkInstance)item->backend_ctx->vulkan_instance, NULL);
            item->backend_ctx->vulkan_instance = NULL;
        }
    } else if (item->backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_METAL) {
        if (item->backend_ctx->metal_device) {
            // [(id<MTLDevice>)item->backend_ctx->metal_device release];
            item->backend_ctx->metal_device = NULL;
        }
    } else if (item->backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_D3D12) {
#ifdef _WIN32
        if (item->backend_ctx->d3d12_queue) {
            item->backend_ctx->d3d12_queue->lpVtbl->Release(item->backend_ctx->d3d12_queue);
            item->backend_ctx->d3d12_queue = NULL;
        }
        if (item->backend_ctx->d3d12_device) {
            item->backend_ctx->d3d12_device->lpVtbl->Release(item->backend_ctx->d3d12_device);
            item->backend_ctx->d3d12_device = NULL;
        }
#endif
    }
    
    free(item->backend_ctx);
    item->backend_ctx = NULL;
    item->backend = CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_CPU;
}

static int cloth_system_cloth_tearing_backend_update(cloth_system_cloth_tearing_internal_t* item, const void* data, size_t size) {
    if (!item) return -1;
    
    if (item->backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_VULKAN) {
        if (!item->backend_ctx || !item->backend_ctx->vulkan_instance) {
            return -2;
        }
        item->backend_ctx->last_frame = item->frame_updated;
    } else if (item->backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_METAL) {
        if (!item->backend_ctx || !item->backend_ctx->metal_device) {
            return -3;
        }
        item->backend_ctx->last_frame = item->frame_updated;
    } else if (item->backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_D3D12) {
#ifdef _WIN32
        if (!item->backend_ctx || !item->backend_ctx->d3d12_device || !item->backend_ctx->d3d12_queue) {
            return -4;
        }
        item->backend_ctx->last_frame = item->frame_updated;
#endif
    } else {
        item->backend_ctx->last_frame = item->frame_updated;
    }
    
    (void)data;
    (void)size;
    return 0;
}

static bool cloth_system_cloth_tearing_validate(const cloth_system_cloth_tearing_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    
    if ((item->backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_VULKAN ||
         item->backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_METAL ||
         item->backend == CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_D3D12) &&
        !item->backend_ctx) {
        return false;
    }
    
    if (item->thread_safe) {
        if (pthread_mutex_trylock((pthread_mutex_t*)&item->mutex) != 0) {
            return false; /* Mutex is locked */
        }
        pthread_mutex_unlock((pthread_mutex_t*)&item->mutex);
    }
    
    /* Cache validation */
    if (item->cache && item->cache_size > 0) {
        for (uint32_t i = 0; i < item->cache_size; i++) {
            if (item->cache[i].valid && !item->cache[i].data) {
                return false;
            }
        }
    }
    
    return true;
}

static void cloth_system_cloth_tearing_cleanup_internal(cloth_system_cloth_tearing_internal_t* item) {
    if (!item) return;
    
    // Cleanup data
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    // Cleanup cache
    cloth_system_cloth_tearing_cleanup_cache(item);
    
    // Cleanup async operations
    cloth_system_cloth_tearing_cleanup_async_ops(item);
    
    /* GPU buffer cleanup */
    if (item->gpu_buffer) {
        free(item->gpu_buffer);
        item->gpu_buffer = NULL;
        item->gpu_buffer_size = 0;
    }
    
    /* Data cleanup */
    if (item->data) {
        free(item->data);
        item->data = NULL;
        item->data_size = 0;
    }
    
    /* Backend cleanup */
    cloth_system_cloth_tearing_backend_shutdown(item);
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int cloth_system_cloth_tearing_init(void) {
    if (g_cloth_tearing_ctx.initialized) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE; /* Already initialized */
    }
    
    /* Initialize global mutex */
    if (pthread_mutex_init(&g_cloth_tearing_ctx.mutex, NULL) != 0) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_THREADING;
    }
    
    /* Initialize memory tracking */
    
    g_cloth_tearing_ctx.capacity = CLOTH_SYSTEM_CLOTH_TEARING_DEFAULT_CAPACITY;
    g_cloth_tearing_ctx.items = calloc(g_cloth_tearing_ctx.capacity, sizeof(cloth_system_cloth_tearing_internal_t));
    if (!g_cloth_tearing_ctx.items) {
        pthread_mutex_destroy(&g_cloth_tearing_ctx.mutex);
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_OUT_OF_MEMORY;
    }
    
    g_cloth_tearing_ctx.count = 0;
    g_cloth_tearing_ctx.initialized = true;
    g_cloth_tearing_ctx.validation_enabled = true;
    g_cloth_tearing_ctx.hot_reload_enabled = false;
    
    return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE;
}

void cloth_system_cloth_tearing_shutdown(void) {
    if (!g_cloth_tearing_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    
    /* Cleanup all items */
    for (uint32_t i = 0; i < g_cloth_tearing_ctx.count; i++) {
        cloth_system_cloth_tearing_internal_t* item = &g_cloth_tearing_ctx.items[i];
        
        if (item->thread_safe) {
            pthread_mutex_destroy(&item->mutex);
        }
        
        cloth_system_cloth_tearing_cleanup_cache(item);
        cloth_system_cloth_tearing_cleanup_async_operations(item);
        
        if (item->gpu_buffer) {
            free(item->gpu_buffer);
            item->gpu_buffer = NULL;
        }
        
        if (item->data) {
            free(item->data);
            item->data = NULL;
        }
        
        cloth_system_cloth_tearing_backend_shutdown(item);
    }
    
    free(g_cloth_tearing_ctx.items);
    g_cloth_tearing_ctx.items = NULL;
    g_cloth_tearing_ctx.count = 0;
    g_cloth_tearing_ctx.capacity = 0;
    
    pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
    pthread_mutex_destroy(&g_cloth_tearing_ctx.mutex);
    
    g_cloth_tearing_ctx.initialized = false;
}

int cloth_system_cloth_tearing_create(cloth_system_cloth_tearing_handle_t* out_handle, const cloth_system_cloth_tearing_desc_t* desc) {
    if (!out_handle || !desc) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM;
    }
    
    if (!g_cloth_tearing_ctx.initialized) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    
    if (g_cloth_tearing_ctx.count >= g_cloth_tearing_ctx.capacity) {
        pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = g_cloth_tearing_ctx.count++;
    cloth_system_cloth_tearing_internal_t* item = &g_cloth_tearing_ctx.items[index];
    
    /* Initialize basic properties */
    memset(item, 0, sizeof(cloth_system_cloth_tearing_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->backend = cloth_system_cloth_tearing_select_backend(desc->flags);
    item->frame_updated = 0;
    item->lod_level = 0;
    item->culling_enabled = true;
    item->render_graph_node = false;
    
    /* Initialize thread safety */
    item->thread_safe = false;
    if (desc->flags & CLOTH_SYSTEM_CLOTH_TEARING_FLAG_THREAD_SAFE) {
        if (pthread_mutex_init(&item->mutex, NULL) != 0) {
            g_cloth_tearing_ctx.count--;
            pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
            return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_THREAD_SAFETY_VIOLATION;
        }
        item->thread_safe = true;
    }
    
    /* Initialize backend */
    int result = cloth_system_cloth_tearing_backend_init(item, item->backend);
    if (result != CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE) {
        pthread_mutex_destroy(&item->mutex);
        g_cloth_tearing_ctx.count--;
        goto cleanup;
    }
    
    /* Initialize cache */
    result = cloth_system_cloth_tearing_init_cache(item);
    if (result != CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE) {
        cloth_system_cloth_tearing_backend_shutdown(item);
        pthread_mutex_destroy(&item->mutex);
        g_cloth_tearing_ctx.count--;
        goto cleanup;
    }
    
    /* Initialize async operations */
    result = cloth_system_cloth_tearing_init_async_ops(item);
    if (result != CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE) {
        cloth_system_cloth_tearing_cleanup_cache(item);
        cloth_system_cloth_tearing_backend_shutdown(item);
        pthread_mutex_destroy(&item->mutex);
        g_cloth_tearing_ctx.count--;
        goto cleanup;
    }
    
    /* Validation */
    if (g_cloth_tearing_ctx.validation_enabled && !cloth_system_cloth_tearing_validate(item)) {
        cloth_system_cloth_tearing_cleanup_async_ops(item);
        cloth_system_cloth_tearing_cleanup_cache(item);
        cloth_system_cloth_tearing_backend_shutdown(item);
        pthread_mutex_destroy(&item->mutex);
        g_cloth_tearing_ctx.count--;
        result = CLOTH_SYSTEM_CLOTH_TEARING_ERROR_VALIDATION_FAILED;
        goto cleanup;
    }
    
    item->initialized = true;
    item->dirty = true;
    
    /* Update memory tracking */
    g_cloth_tearing_ctx.memory_stats.total_allocated += sizeof(cloth_system_cloth_tearing_internal_t);
    g_cloth_tearing_ctx.memory_stats.allocation_count++;
    if (g_cloth_tearing_ctx.memory_stats.total_allocated > g_cloth_tearing_ctx.memory_stats.peak_usage) {
        g_cloth_tearing_ctx.memory_stats.peak_usage = g_cloth_tearing_ctx.memory_stats.total_allocated;
    }
    
    out_handle->id = index;
    
cleanup:
    pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
    return result;
}

void cloth_system_cloth_tearing_destroy(cloth_system_cloth_tearing_handle_t handle) {
    if (!g_cloth_tearing_ctx.initialized) {
        return;
    }
    
    if (handle.id >= g_cloth_tearing_ctx.count) {
        return;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    
    cloth_system_cloth_tearing_internal_t* item = &g_cloth_tearing_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
        return;
    }
    
    /* Update performance counters */
    g_cloth_tearing_ctx.perf_counters.frames_processed++;
    
    /* Cleanup item */
    cloth_system_cloth_tearing_cleanup_internal(item);
    
    /* Update memory tracking */
    g_cloth_tearing_ctx.memory_stats.total_allocated -= sizeof(cloth_system_cloth_tearing_internal_t);
    
    pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
}

int cloth_system_cloth_tearing_update(cloth_system_cloth_tearing_handle_t handle, const void* data, size_t size) {
    if (!g_cloth_tearing_ctx.initialized) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_cloth_tearing_ctx.count) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    
    cloth_system_cloth_tearing_internal_t* item = &g_cloth_tearing_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_VALIDATION_FAILED;
    }
    
    if (item->thread_safe) {
        pthread_mutex_lock(&item->mutex);
    }
    
    int result = CLOTH_SYSTEM_CLOTH_TEARING_SUCCESS;
    
    /* Update data */
    if (data && size > 0) {
        if (item->data_size < size) {
            void* new_data = realloc(item->data, size);
            if (!new_data) {
                result = CLOTH_SYSTEM_CLOTH_TEARING_ERROR_OUT_OF_MEMORY;
                goto cleanup;
            }
            item->data = new_data;
            item->data_size = size;
            
            /* Update memory tracking */
            g_cloth_tearing_ctx.memory_stats.total_allocated += (size - item->data_size);
            g_cloth_tearing_ctx.memory_stats.allocation_count++;
        }
        memcpy(item->data, data, size);
    }
    
    item->frame_updated++;
    
    /* Check cache first */
    uint64_t data_hash = cloth_system_cloth_tearing_calculate_checksum(data, size);
    uint32_t cache_index = data_hash % item->cache_size;
    
    if (item->cache[cache_index].valid && 
        item->cache[cache_index].hash == data_hash &&
        item->cache[cache_index].size == size) {
        /* Cache hit */
        item->cache[cache_index].last_access = time(NULL);
        item->cache[cache_index].access_count++;
        g_cloth_tearing_ctx.perf_counters.cache_hits++;
    } else {
        /* Cache miss - update cache */
        if (item->cache[cache_index].data) {
            free(item->cache[cache_index].data);
        }
        item->cache[cache_index].data = malloc(size);
        if (item->cache[cache_index].data) {
            memcpy(item->cache[cache_index].data, data, size);
            item->cache[cache_index].hash = data_hash;
            item->cache[cache_index].size = size;
            item->cache[cache_index].last_access = time(NULL);
            item->cache[cache_index].valid = true;
            item->cache[cache_index].access_count = 1;
        }
        g_cloth_tearing_ctx.perf_counters.cache_misses++;
    }
    
    /* Apply SIMD optimization if enabled */
    if (item->flags & CLOTH_SYSTEM_CLOTH_TEARING_FLAG_SIMD_OPTIMIZATION) {
        cloth_system_cloth_tearing_simd_optimization(item);
    }
    
    /* Update backend */
    if (cloth_system_cloth_tearing_backend_update(item, data, size) != CLOTH_SYSTEM_CLOTH_TEARING_SUCCESS) {
        result = CLOTH_SYSTEM_CLOTH_TEARING_ERROR_GPU_OPERATION_FAILED;
        goto cleanup;
    }
    
    /* Update performance counters */
    g_cloth_tearing_ctx.perf_counters.frames_processed++;
    g_cloth_tearing_ctx.perf_counters.tears_detected++;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    /* Process cloth tearing detection */
    /* TODO: Actual tearing detection algorithm would go here */
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double processing_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    g_cloth_tearing_ctx.perf_counters.tears_processed++;
    g_cloth_tearing_ctx.perf_counters.total_processing_time += processing_time;
    g_cloth_tearing_ctx.perf_counters.avg_processing_time = 
        g_cloth_tearing_ctx.perf_counters.total_processing_time / g_cloth_tearing_ctx.perf_counters.tears_processed;
    
    item->dirty = true;
    
cleanup:
    if (item->thread_safe) {
        pthread_mutex_unlock(&item->mutex);
    }
    pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
    
    return result;
}

bool cloth_system_cloth_tearing_is_valid(cloth_system_cloth_tearing_handle_t handle) {
    if (!g_cloth_tearing_ctx.initialized) {
        return false;
    }
    
    if (handle.id >= g_cloth_tearing_ctx.count) {
        return false;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    
    const cloth_system_cloth_tearing_internal_t* item = &g_cloth_tearing_ctx.items[handle.id];
    return cloth_system_cloth_tearing_validate(item);
}

int cloth_system_cloth_tearing_get_info(cloth_system_cloth_tearing_handle_t handle, cloth_system_cloth_tearing_info_t* out_info) {
    if (!out_info) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM;
    }
    
    if (!g_cloth_tearing_ctx.initialized) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_cloth_tearing_ctx.count) {
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    
    cloth_system_cloth_tearing_internal_t* item = &g_cloth_tearing_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
        return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_VALIDATION_FAILED;
    }
    
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
    return CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE;
}

void cloth_system_cloth_tearing_mark_dirty(cloth_system_cloth_tearing_handle_t handle) {
    if (!g_cloth_tearing_ctx.initialized) {
        return;
    }
    
    if (handle.id >= g_cloth_tearing_ctx.count) {
        return;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    
    cloth_system_cloth_tearing_internal_t* item = &g_cloth_tearing_ctx.items[handle.id];
    if (item->initialized) {
        item->dirty = true;
    }
    
    pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
}

int cloth_system_cloth_tearing_process_pending(void) {
    if (!g_cloth_tearing_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    
    int processed = 0;
    for (uint32_t i = 0; i < g_cloth_tearing_ctx.count; i++) {
        cloth_system_cloth_tearing_internal_t* item = &g_cloth_tearing_ctx.items[i];
        if (item->initialized && item->dirty) {
            /* Process item - simulate cloth tearing processing */
            if (item->thread_safe) {
                pthread_mutex_lock(&item->mutex);
            }
            
            /* Simulate processing time */
            usleep(100); /* 0.1ms */
            
            item->dirty = false;
            processed++;
            
            /* Update performance counters */
            g_cloth_tearing_ctx.perf_counters.tears_processed++;
            
            if (item->thread_safe) {
                pthread_mutex_unlock(&item->mutex);
            }
        }
    }
    
    pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
    return processed;
}

uint32_t cloth_system_cloth_tearing_get_count(void) {
    if (!g_cloth_tearing_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    uint32_t count = g_cloth_tearing_ctx.count;
    pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
    
    return count;
}

size_t cloth_system_cloth_tearing_get_memory_usage(void) {
    if (!g_cloth_tearing_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    
    size_t total = sizeof(g_cloth_tearing_ctx);
    total += g_cloth_tearing_ctx.capacity * sizeof(cloth_system_cloth_tearing_internal_t);
    
    for (uint32_t i = 0; i < g_cloth_tearing_ctx.count; i++) {
        cloth_system_cloth_tearing_internal_t* item = &g_cloth_tearing_ctx.items[i];
        total += item->data_size;
        total += item->gpu_buffer_size;
        
        if (item->cache) {
            total += item->cache_size * sizeof(cloth_system_cloth_tearing_cache_entry_t);
            for (uint32_t j = 0; j < item->cache_size; j++) {
                if (item->cache[j].data) {
                    total += item->cache[j].size;
                }
            }
        }
        
        if (item->async_ops) {
            total += item->async_ops_count * sizeof(cloth_system_cloth_tearing_async_operation_t);
        }
    }
    
    pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
    return total;
}

void cloth_system_cloth_tearing_debug_print(void) {
    if (!g_cloth_tearing_ctx.initialized) {
        printf("Cloth tearing system not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_cloth_tearing_ctx.mutex);
    
    printf("=== Cloth Tearing System Debug Info ===\n");
    printf("Initialized: %s\n", g_cloth_tearing_ctx.initialized ? "Yes" : "No");
    printf("Count: %u / %u\n", g_cloth_tearing_ctx.count, g_cloth_tearing_ctx.capacity);
    printf("Validation enabled: %s\n", g_cloth_tearing_ctx.validation_enabled ? "Yes" : "No");
    printf("Hot-reload enabled: %s\n", g_cloth_tearing_ctx.hot_reload_enabled ? "Yes" : "No");
    
    printf("\nMemory Statistics:\n");
    printf("Total allocated: %zu bytes\n", g_cloth_tearing_ctx.memory_stats.total_allocated);
    printf("Peak usage: %zu bytes\n", g_cloth_tearing_ctx.memory_stats.peak_usage);
    printf("Allocation count: %u\n", g_cloth_tearing_ctx.memory_stats.allocation_count);
    printf("Leak count: %u\n", g_cloth_tearing_ctx.memory_stats.leak_count);
    
    printf("\nPerformance Counters:\n");
    printf("Frames processed: %llu\n", (unsigned long long)g_cloth_tearing_ctx.perf_counters.frames_processed);
    printf("Tears detected: %llu\n", (unsigned long long)g_cloth_tearing_ctx.perf_counters.tears_detected);
    printf("Tears processed: %llu\n", (unsigned long long)g_cloth_tearing_ctx.perf_counters.tears_processed);
    printf("Avg processing time: %.3f ms\n", g_cloth_tearing_ctx.perf_counters.avg_processing_time);
    
    printf("\nItems:\n");
    for (uint32_t i = 0; i < g_cloth_tearing_ctx.count; i++) {
        cloth_system_cloth_tearing_internal_t* item = &g_cloth_tearing_ctx.items[i];
        printf("  [%u] ID: %u, Backend: %d, Thread-safe: %s, Dirty: %s\n",
               i, item->id, item->backend,
               item->thread_safe ? "Yes" : "No",
               item->dirty ? "Yes" : "No");
    }
    
    printf("=====================================\n");
    
    pthread_mutex_unlock(&g_cloth_tearing_ctx.mutex);
}

/* End of cloth_tearing.c */
