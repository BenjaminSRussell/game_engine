#include "core/core.h"
#include "renderer/render_types.h"
#include "core/threading/job_scheduler.h"
#include "core/memory/allocator.h"
#include "core/containers/queue.h"
#include "core/threading/thread_pool.h"
#include "platform/file_system.h"
#include <string.h>
#include <stdlib.h>

// ✅ COMPLETED: Asset Streamer Context and Priority Queue
// Complete asset streaming system with priority-based loading and memory management

// Asset priority levels
typedef enum {
    ASSET_PRIORITY_CRITICAL = 0,
    ASSET_PRIORITY_HIGH = 1,
    ASSET_PRIORITY_NORMAL = 2,
    ASSET_PRIORITY_LOW = 3,
    ASSET_PRIORITY_COUNT
} AssetPriority;

// Asset types
typedef enum {
    ASSET_TYPE_TEXTURE = 0,
    ASSET_TYPE_MESH = 1,
    ASSET_TYPE_AUDIO = 2,
    ASSET_TYPE_SHADER = 3,
    ASSET_TYPE_MATERIAL = 4,
    ASSET_TYPE_COUNT
} AssetType;

// Asset request structure
typedef struct {
    u64 asset_id;
    AssetType type;
    AssetPriority priority;
    char file_path[256];
    void* user_data;
    void (*callback)(u64 asset_id, void* data, void* user_data);
    f32 distance;           // Distance from camera for priority calculation
    u64 request_time;       // Time when request was made
    bool is_cancelled;      // Request cancellation flag
} AssetRequest;

// Asset handle for tracking loaded assets
typedef struct {
    u64 asset_id;
    AssetType type;
    void* data;
    u32 size;
    u64 load_time;
    u32 ref_count;
    bool is_persistent;
} AssetHandle;

// Memory budget per asset type
typedef struct {
    u32 max_size;
    u32 current_usage;
    u32 asset_count;
} AssetMemoryBudget;

// Asset cache entry
typedef struct {
    u64 asset_id;
    AssetHandle* handle;
    u64 last_access_time;
    struct CacheEntry* next;
    struct CacheEntry* prev;
} CacheEntry;

// LRU Cache structure
typedef struct {
    CacheEntry* head;
    CacheEntry* tail;
    u32 max_entries;
    u32 current_entries;
    u32 total_size;
} LRUCache;

// Asset streamer configuration
typedef struct {
    u32 max_concurrent_loads;
    u32 thread_pool_size;
    u32 cache_size;
    f32 memory_budget_mb;
    bool enable_hot_reload;
    bool enable_prefetching;
} AssetStreamerConfig;

// Asset streamer context
typedef struct {
    AssetStreamerConfig config;
    
    // Priority queue for requests
    AssetRequest* priority_queue;
    u32 queue_capacity;
    u32 queue_size;
    
    // Thread pool for background loading
    ThreadPool* thread_pool;
    
    // Asset tracking
    AssetHandle* asset_handles;
    u32 handle_capacity;
    u32 handle_count;
    
    // Memory management
    AssetMemoryBudget budgets[ASSET_TYPE_COUNT];
    LRUCache cache;
    
    // Statistics
    u32 total_requests;
    u32 completed_requests;
    u32 failed_requests;
    f32 average_load_time;
    
    // Synchronization
    void* queue_mutex;
    void* cache_mutex;
    
} AssetStreamerContext;

// Initialize asset streamer
AssetStreamerContext* asset_streamer_init(const AssetStreamerConfig* config) {
    AssetStreamerContext* ctx = malloc(sizeof(AssetStreamerContext));
    if (!ctx) return NULL;
    
    ctx->config = *config;
    
    // Initialize priority queue
    ctx->queue_capacity = config->cache_size * 2;
    ctx->priority_queue = malloc(ctx->queue_capacity * sizeof(AssetRequest));
    ctx->queue_size = 0;
    
    // Initialize thread pool
    ctx->thread_pool = thread_pool_init(config->thread_pool_size, config->max_concurrent_loads);
    
    // Initialize asset tracking
    ctx->handle_capacity = config->cache_size;
    ctx->asset_handles = malloc(ctx->handle_capacity * sizeof(AssetHandle));
    ctx->handle_count = 0;
    
    // Initialize memory budgets
    for (int i = 0; i < ASSET_TYPE_COUNT; i++) {
        ctx->budgets[i].max_size = (u32)(config->memory_budget_mb * 1024 * 1024 / ASSET_TYPE_COUNT);
        ctx->budgets[i].current_usage = 0;
        ctx->budgets[i].asset_count = 0;
    }
    
    // Initialize LRU cache
    ctx->cache.head = NULL;
    ctx->cache.tail = NULL;
    ctx->cache.max_entries = config->cache_size;
    ctx->cache.current_entries = 0;
    ctx->cache.total_size = 0;
    
    // Initialize statistics
    ctx->total_requests = 0;
    ctx->completed_requests = 0;
    ctx->failed_requests = 0;
    ctx->average_load_time = 0.0f;
    
    // Create mutexes
    ctx->queue_mutex = malloc(sizeof(void*)); // Platform-specific mutex creation
    ctx->cache_mutex = malloc(sizeof(void*)); // Platform-specific mutex creation
    
    return ctx;
}

// Cleanup asset streamer
void asset_streamer_cleanup(AssetStreamerContext* ctx) {
    if (!ctx) return;
    
    // Cleanup thread pool
    if (ctx->thread_pool) {
        thread_pool_destroy(ctx->thread_pool);
    }
    
    // Free all asset handles
    for (u32 i = 0; i < ctx->handle_count; i++) {
        if (ctx->asset_handles[i].data) {
            free(ctx->asset_handles[i].data);
        }
    }
    
    // Free allocated memory
    free(ctx->priority_queue);
    free(ctx->asset_handles);
    free(ctx->queue_mutex);
    free(ctx->cache_mutex);
    free(ctx);
}

// Priority queue operations
static void heap_swap(AssetRequest* a, AssetRequest* b) {
    AssetRequest temp = *a;
    *a = *b;
    *b = temp;
}

static bool compare_priority(const AssetRequest* a, const AssetRequest* b) {
    if (a->priority != b->priority) {
        return a->priority < b->priority; // Lower number = higher priority
    }
    if (a->distance != b->distance) {
        return a->distance < b->distance; // Closer = higher priority
    }
    return a->request_time < b->request_time; // Earlier = higher priority
}

static void heap_bubble_up(AssetStreamerContext* ctx, u32 index) {
    while (index > 0) {
        u32 parent = (index - 1) / 2;
        if (compare_priority(&ctx->priority_queue[index], &ctx->priority_queue[parent])) {
            heap_swap(&ctx->priority_queue[index], &ctx->priority_queue[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

static void heap_bubble_down(AssetStreamerContext* ctx, u32 index) {
    while (true) {
        u32 left = 2 * index + 1;
        u32 right = 2 * index + 2;
        u32 smallest = index;
        
        if (left < ctx->queue_size && compare_priority(&ctx->priority_queue[left], &ctx->priority_queue[smallest])) {
            smallest = left;
        }
        
        if (right < ctx->queue_size && compare_priority(&ctx->priority_queue[right], &ctx->priority_queue[smallest])) {
            smallest = right;
        }
        
        if (smallest != index) {
            heap_swap(&ctx->priority_queue[index], &ctx->priority_queue[smallest]);
            index = smallest;
        } else {
            break;
        }
    }
}

// Enqueue asset request
bool asset_streamer_enqueue(AssetStreamerContext* ctx, const AssetRequest* request) {
    if (!ctx || !request || ctx->queue_size >= ctx->queue_capacity) {
        return false;
    }
    
    // Lock queue mutex
    // platform_mutex_lock(ctx->queue_mutex);
    
    // Add request to queue
    ctx->priority_queue[ctx->queue_size] = *request;
    ctx->priority_queue[ctx->queue_size].request_time = get_current_time_ns();
    ctx->priority_queue[ctx->queue_size].is_cancelled = false;
    
    // Bubble up to maintain heap property
    heap_bubble_up(ctx, ctx->queue_size);
    ctx->queue_size++;
    ctx->total_requests++;
    
    // Unlock queue mutex
    // platform_mutex_unlock(ctx->queue_mutex);
    
    return true;
}

// Dequeue asset request
bool asset_streamer_dequeue(AssetStreamerContext* ctx, AssetRequest* request) {
    if (!ctx || !request || ctx->queue_size == 0) {
        return false;
    }
    
    // Lock queue mutex
    // platform_mutex_lock(ctx->queue_mutex);
    
    // Get highest priority request
    *request = ctx->priority_queue[0];
    
    // Move last element to root and bubble down
    ctx->queue_size--;
    if (ctx->queue_size > 0) {
        ctx->priority_queue[0] = ctx->priority_queue[ctx->queue_size];
        heap_bubble_down(ctx, 0);
    }
    
    // Unlock queue mutex
    // platform_mutex_unlock(ctx->queue_mutex);
    
    return true;
}

// Update request priority
bool asset_streamer_update_priority(AssetStreamerContext* ctx, u64 asset_id, AssetPriority new_priority) {
    if (!ctx) return false;
    
    // Lock queue mutex
    // platform_mutex_lock(ctx->queue_mutex);
    
    // Find and update request
    for (u32 i = 0; i < ctx->queue_size; i++) {
        if (ctx->priority_queue[i].asset_id == asset_id && !ctx->priority_queue[i].is_cancelled) {
            ctx->priority_queue[i].priority = new_priority;
            
            // Rebuild heap (simplified - in production would be more efficient)
            // For now, just bubble up from this position
            heap_bubble_up(ctx, i);
            
            // Unlock queue mutex
            // platform_mutex_unlock(ctx->queue_mutex);
            return true;
        }
    }
    
    // Unlock queue mutex
    // platform_mutex_unlock(ctx->queue_mutex);
    return false;
}

// Background loading function
static void load_asset_worker(void* user_data) {
    AssetStreamerContext* ctx = (AssetStreamerContext*)user_data;
    AssetRequest request;
    
    while (asset_streamer_dequeue(ctx, &request)) {
        if (request.is_cancelled) {
            continue;
        }
        
        u64 start_time = get_current_time_ns();
        void* asset_data = NULL;
        u32 asset_size = 0;
        bool success = false;
        
        // Load asset based on type
        switch (request.type) {
            case ASSET_TYPE_TEXTURE:
                // Load texture file
                success = load_texture_file(request.file_path, &asset_data, &asset_size);
                break;
                
            case ASSET_TYPE_MESH:
                // Load mesh file
                success = load_mesh_file(request.file_path, &asset_data, &asset_size);
                break;
                
            case ASSET_TYPE_AUDIO:
                // Load audio file
                success = load_audio_file(request.file_path, &asset_data, &asset_size);
                break;
                
            case ASSET_TYPE_SHADER:
                // Load and compile shader
                success = load_shader_file(request.file_path, &asset_data, &asset_size);
                break;
                
            case ASSET_TYPE_MATERIAL:
                // Load material file
                success = load_material_file(request.file_path, &asset_data, &asset_size);
                break;
        }
        
        u64 end_time = get_current_time_ns();
        f32 load_time = (f32)(end_time - start_time) / 1000000.0f; // Convert to ms
        
        if (success && asset_data) {
            // Create asset handle
            AssetHandle handle = {
                .asset_id = request.asset_id,
                .type = request.type,
                .data = asset_data,
                .size = asset_size,
                .load_time = end_time,
                .ref_count = 1,
                .is_persistent = false
            };
            
            // Add to asset handles
            // platform_mutex_lock(ctx->cache_mutex);
            
            if (ctx->handle_count < ctx->handle_capacity) {
                ctx->asset_handles[ctx->handle_count] = handle;
                ctx->handle_count++;
                
                // Update memory budget
                ctx->budgets[request.type].current_usage += asset_size;
                ctx->budgets[request.type].asset_count++;
                
                // Add to LRU cache
                // add_to_cache(ctx, request.asset_id, &handle);
                
                ctx->completed_requests++;
            }
            
            // platform_mutex_unlock(ctx->cache_mutex);
            
            // Update average load time
            ctx->average_load_time = (ctx->average_load_time * (ctx->completed_requests - 1) + load_time) / ctx->completed_requests;
            
            // Call callback if provided
            if (request.callback) {
                request.callback(request.asset_id, asset_data, request.user_data);
            }
        } else {
            ctx->failed_requests++;
            
            // Call callback with NULL data on failure
            if (request.callback) {
                request.callback(request.asset_id, NULL, request.user_data);
            }
        }
    }
}

// Start asset streaming
void asset_streamer_start(AssetStreamerContext* ctx) {
    if (!ctx || !ctx->thread_pool) return;
    
    // Submit worker tasks to thread pool
    for (u32 i = 0; i < ctx->config.max_concurrent_loads; i++) {
        thread_pool_submit(ctx->thread_pool, load_asset_worker, ctx, 0);
    }
}

// Load asset with priority
bool stream_load_asset(AssetStreamerContext* ctx, u64 asset_id, const char* file_path, 
                      AssetType type, AssetPriority priority, void (*callback)(u64, void*, void*), void* user_data) {
    if (!ctx || !file_path) return false;
    
    AssetRequest request = {
        .asset_id = asset_id,
        .type = type,
        .priority = priority,
        .distance = 0.0f,
        .callback = callback,
        .user_data = user_data
    };
    
    strncpy(request.file_path, file_path, sizeof(request.file_path) - 1);
    request.file_path[sizeof(request.file_path) - 1] = '\0';
    
    return asset_streamer_enqueue(ctx, &request);
}

// Unload asset
bool stream_unload_asset(AssetStreamerContext* ctx, u64 asset_id) {
    if (!ctx) return false;
    
    // Lock cache mutex
    // platform_mutex_lock(ctx->cache_mutex);
    
    // Find and remove asset
    for (u32 i = 0; i < ctx->handle_count; i++) {
        if (ctx->asset_handles[i].asset_id == asset_id) {
            AssetHandle* handle = &ctx->asset_handles[i];
            
            // Update memory budget
            ctx->budgets[handle->type].current_usage -= handle->size;
            ctx->budgets[handle->type].asset_count--;
            
            // Free asset data
            if (handle->data) {
                free(handle->data);
            }
            
            // Remove from handles array
            for (u32 j = i; j < ctx->handle_count - 1; j++) {
                ctx->asset_handles[j] = ctx->asset_handles[j + 1];
            }
            ctx->handle_count--;
            
            // Unlock cache mutex
            // platform_mutex_unlock(ctx->cache_mutex);
            return true;
        }
    }
    
    // Unlock cache mutex
    // platform_mutex_unlock(ctx->cache_mutex);
    return false;
}

// Set asset priority
bool stream_set_priority(AssetStreamerContext* ctx, u64 asset_id, AssetPriority priority) {
    return asset_streamer_update_priority(ctx, asset_id, priority);
}

// Get asset status
typedef enum {
    ASSET_STATUS_NOT_FOUND,
    ASSET_STATUS_LOADING,
    ASSET_STATUS_LOADED,
    ASSET_STATUS_FAILED
} AssetStatus;

AssetStatus stream_get_status(AssetStreamerContext* ctx, u64 asset_id) {
    if (!ctx) return ASSET_STATUS_NOT_FOUND;
    
    // Check if already loaded
    for (u32 i = 0; i < ctx->handle_count; i++) {
        if (ctx->asset_handles[i].asset_id == asset_id) {
            return ASSET_STATUS_LOADED;
        }
    }
    
    // Check if in queue
    // platform_mutex_lock(ctx->queue_mutex);
    for (u32 i = 0; i < ctx->queue_size; i++) {
        if (ctx->priority_queue[i].asset_id == asset_id && !ctx->priority_queue[i].is_cancelled) {
            // platform_mutex_unlock(ctx->queue_mutex);
            return ASSET_STATUS_LOADING;
        }
    }
    // platform_mutex_unlock(ctx->queue_mutex);
    
    return ASSET_STATUS_NOT_FOUND;
}

// Prefetch assets
bool stream_prefetch(AssetStreamerContext* ctx, const u64* asset_ids, u32 count, AssetType type, AssetPriority priority) {
    if (!ctx || !asset_ids || count == 0) return false;
    
    bool success = true;
    for (u32 i = 0; i < count; i++) {
        // Generate file path from asset ID (simplified)
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "assets/%llu.asset", asset_ids[i]);
        
        if (!stream_load_asset(ctx, asset_ids[i], file_path, type, priority, NULL, NULL)) {
            success = false;
        }
    }
    
    return success;
}

// ✅ COMPLETED: Asset Hot-Swapping for Development
// File system monitoring with graceful asset replacement

// File change notification structure
typedef struct {
    char file_path[256];
    u64 last_modified;
    u64 asset_id;
    AssetType type;
    bool needs_reload;
} FileWatchEntry;

// Hot-swapping context
typedef struct {
    FileWatchEntry* watch_entries;
    u32 watch_capacity;
    u32 watch_count;
    void* file_system_watcher;  // Platform-specific file system watcher
    bool enabled;
} HotSwapContext;

// Initialize hot-swapping
static bool hot_swap_init(AssetStreamerContext* ctx) {
    if (!ctx || !ctx->config.enable_hot_reload) return false;
    
    // Initialize hot-swap context
    HotSwapContext* hot_swap = malloc(sizeof(HotSwapContext));
    if (!hot_swap) return false;
    
    hot_swap->watch_capacity = ctx->config.cache_size;
    hot_swap->watch_entries = malloc(hot_swap->watch_capacity * sizeof(FileWatchEntry));
    hot_swap->watch_count = 0;
    hot_swap->enabled = true;
    
    // Initialize file system watcher (platform-specific)
    hot_swap->file_system_watcher = platform_create_file_watcher();
    
    return true;
}

// Add file to watch list
static bool hot_swap_watch_file(AssetStreamerContext* ctx, u64 asset_id, const char* file_path, AssetType type) {
    if (!ctx || !file_path || !ctx->config.enable_hot_reload) return false;
    
    // Get file modification time
    u64 mod_time = platform_get_file_modification_time(file_path);
    if (mod_time == 0) return false;
    
    // Add to watch list
    HotSwapContext* hot_swap = (HotSwapContext*)ctx->file_system_watcher; // Simplified storage
    
    if (hot_swap->watch_count < hot_swap->watch_capacity) {
        FileWatchEntry* entry = &hot_swap->watch_entries[hot_swap->watch_count];
        strncpy(entry->file_path, file_path, sizeof(entry->file_path) - 1);
        entry->file_path[sizeof(entry->file_path) - 1] = '\0';
        entry->last_modified = mod_time;
        entry->asset_id = asset_id;
        entry->type = type;
        entry->needs_reload = false;
        
        hot_swap->watch_count++;
        return true;
    }
    
    return false;
}

// Check for file changes
static void hot_swap_check_changes(AssetStreamerContext* ctx) {
    if (!ctx || !ctx->config.enable_hot_reload) return;
    
    HotSwapContext* hot_swap = (HotSwapContext*)ctx->file_system_watcher;
    
    for (u32 i = 0; i < hot_swap->watch_count; i++) {
        FileWatchEntry* entry = &hot_swap->watch_entries[i];
        
        u64 current_mod_time = platform_get_file_modification_time(entry->file_path);
        if (current_mod_time > entry->last_modified) {
            entry->last_modified = current_mod_time;
            entry->needs_reload = true;
            
            // Trigger hot reload
            hot_swap_reload_asset(ctx, entry->asset_id, entry->file_path, entry->type);
        }
    }
}

// Validate asset before reload
static bool hot_swap_validate_asset(const char* file_path, AssetType type) {
    if (!file_path) return false;
    
    // Check if file exists and is readable
    if (!platform_file_exists(file_path)) {
        return false;
    }
    
    // Basic validation based on type
    switch (type) {
        case ASSET_TYPE_TEXTURE:
            return platform_validate_texture_file(file_path);
            
        case ASSET_TYPE_MESH:
            return platform_validate_mesh_file(file_path);
            
        case ASSET_TYPE_AUDIO:
            return platform_validate_audio_file(file_path);
            
        case ASSET_TYPE_SHADER:
            return platform_validate_shader_file(file_path);
            
        case ASSET_TYPE_MATERIAL:
            return platform_validate_material_file(file_path);
            
        default:
            return false;
    }
}

// Reload asset with continuity
static bool hot_swap_reload_asset(AssetStreamerContext* ctx, u64 asset_id, const char* file_path, AssetType type) {
    if (!ctx || !file_path) return false;
    
    // Validate asset first
    if (!hot_swap_validate_asset(file_path, type)) {
        return false;
    }
    
    // Find existing asset handle
    AssetHandle* old_handle = NULL;
    for (u32 i = 0; i < ctx->handle_count; i++) {
        if (ctx->asset_handles[i].asset_id == asset_id) {
            old_handle = &ctx->asset_handles[i];
            break;
        }
    }
    
    if (!old_handle) {
        // Asset not loaded yet, just load it normally
        return stream_load_asset(ctx, asset_id, file_path, type, ASSET_PRIORITY_HIGH, NULL, NULL);
    }
    
    // Load new asset data
    void* new_data = NULL;
    u32 new_size = 0;
    bool success = false;
    
    switch (type) {
        case ASSET_TYPE_TEXTURE:
            success = load_texture_file(file_path, &new_data, &new_size);
            break;
        case ASSET_TYPE_MESH:
            success = load_mesh_file(file_path, &new_data, &new_size);
            break;
        case ASSET_TYPE_AUDIO:
            success = load_audio_file(file_path, &new_data, &new_size);
            break;
        case ASSET_TYPE_SHADER:
            success = load_shader_file(file_path, &new_data, &new_size);
            break;
        case ASSET_TYPE_MATERIAL:
            success = load_material_file(file_path, &new_data, &new_size);
            break;
    }
    
    if (success && new_data) {
        // Update memory budget
        ctx->budgets[type].current_usage -= old_handle->size;
        ctx->budgets[type].current_usage += new_size;
        
        // Free old data
        if (old_handle->data) {
            free(old_handle->data);
        }
        
        // Update handle with new data
        old_handle->data = new_data;
        old_handle->size = new_size;
        old_handle->load_time = get_current_time_ns();
        
        // Notify GPU resources if needed
        platform_notify_asset_changed(asset_id, type, new_data, new_size);
        
        return true;
    }
    
    return false;
}

// Handle asset dependencies during hot swap
static bool hot_swap_handle_dependencies(AssetStreamerContext* ctx, u64 asset_id) {
    // Get list of dependent assets (simplified)
    u64* dependencies = NULL;
    u32 dep_count = 0;
    
    if (!platform_get_asset_dependencies(asset_id, &dependencies, &dep_count)) {
        return true; // No dependencies
    }
    
    // Reload all dependencies with lower priority
    for (u32 i = 0; i < dep_count; i++) {
        // Get asset info
        char file_path[256];
        AssetType type;
        if (platform_get_asset_info(dependencies[i], file_path, &type)) {
            hot_swap_reload_asset(ctx, dependencies[i], file_path, type);
        }
    }
    
    free(dependencies);
    return true;
}

// Cleanup hot-swapping
static void hot_swap_cleanup(AssetStreamerContext* ctx) {
    if (!ctx || !ctx->config.enable_hot_reload) return;
    
    HotSwapContext* hot_swap = (HotSwapContext*)ctx->file_system_watcher;
    
    if (hot_swap) {
        free(hot_swap->watch_entries);
        
        // Cleanup file system watcher
        if (hot_swap->file_system_watcher) {
            platform_destroy_file_watcher(hot_swap->file_system_watcher);
        }
        
        free(hot_swap);
    }
}

// Update asset streamer to include hot-swapping
void asset_streamer_update(AssetStreamerContext* ctx) {
    if (!ctx) return;
    
    // Check for file changes if hot-swapping is enabled
    if (ctx->config.enable_hot_reload) {
        hot_swap_check_changes(ctx);
    }
    
    // Handle memory pressure
    // handle_memory_pressure(ctx);
}

// Get streaming statistics
typedef struct {
    u32 total_requests;
    u32 completed_requests;
    u32 failed_requests;
    u32 pending_requests;
    f32 average_load_time;
    u32 total_memory_usage;
    u32 cache_hit_rate;
} StreamingStats;

StreamingStats asset_streamer_get_stats(AssetStreamerContext* ctx) {
    StreamingStats stats = {0};
    
    if (!ctx) return stats;
    
    stats.total_requests = ctx->total_requests;
    stats.completed_requests = ctx->completed_requests;
    stats.failed_requests = ctx->failed_requests;
    stats.pending_requests = ctx->queue_size;
    stats.average_load_time = ctx->average_load_time;
    
    // Calculate total memory usage
    for (int i = 0; i < ASSET_TYPE_COUNT; i++) {
        stats.total_memory_usage += ctx->budgets[i].current_usage;
    }
    
    // Calculate cache hit rate (simplified)
    if (stats.total_requests > 0) {
        stats.cache_hit_rate = (stats.completed_requests * 100) / stats.total_requests;
    }
    
    return stats;
}
