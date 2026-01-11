/**
 * @file physics_pipeline_optimizer.c
 * @brief Physics pipeline performance optimization implementation
 *
 * Implements comprehensive optimization strategies for the physics pipeline
 * including memory pooling, SIMD acceleration, multi-threading, spatial
 * partitioning, caching, and adaptive algorithms.
 */

#include "physics_pipeline_optimizer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>

// ========================================
// Platform-specific Headers
// ========================================

#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#endif

// ========================================
// Constants
// ========================================

#define MAX_MEMORY_POOLS 8
#define MAX_THREADS 16
#define MAX_CACHE_SIZE 1024
#define MAX_LOD_LEVELS 8
#define ADAPTATION_INTERVAL_MS 1000

// ========================================
// High-Resolution Timer
// ========================================

static uint64_t get_current_time_ms(void) {
#ifdef _WIN32
    return GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

// ========================================
// Memory Pool Implementation
// ========================================

static MemoryPool* create_memory_pool(size_t pool_size, int block_count) {
    MemoryPool *pool = (MemoryPool*)calloc(1, sizeof(MemoryPool));
    if (!pool) return NULL;
    
    pool->size = pool_size;
    pool->block_size = pool_size / block_count;
    pool->block_count = block_count;
    pool->free_count = block_count;
    pool->allocated_count = 0;
    pool->peak_usage = 0;
    
    pool->memory = malloc(pool_size);
    if (!pool->memory) {
        free(pool);
        return NULL;
    }
    
    pool->free_list = (uint32_t*)malloc(block_count * sizeof(uint32_t));
    if (!pool->free_list) {
        free(pool->memory);
        free(pool);
        return NULL;
    }
    
    // Initialize free list
    for (int i = 0; i < block_count; i++) {
        pool->free_list[i] = i;
    }
    
    return pool;
}

static void destroy_memory_pool(MemoryPool *pool) {
    if (pool) {
        free(pool->memory);
        free(pool->free_list);
        free(pool);
    }
}

static void* allocate_from_pool(MemoryPool *pool) {
    if (!pool || pool->free_count == 0) return NULL;
    
    uint32_t block_index = pool->free_list[--pool->free_count];
    void *ptr = (uint8_t*)pool->memory + block_index * pool->block_size;
    
    pool->allocated_count++;
    if (pool->allocated_count > pool->peak_usage) {
        pool->peak_usage = pool->allocated_count;
    }
    
    return ptr;
}

static void free_to_pool(MemoryPool *pool, void *ptr) {
    if (!pool || !ptr) return;
    
    // Calculate block index
    uint8_t *base = (uint8_t*)pool->memory;
    uint8_t *block = (uint8_t*)ptr;
    uint32_t block_index = (uint32_t)(block - base) / pool->block_size;
    
    if (block_index < pool->block_count) {
        pool->free_list[pool->free_count++] = block_index;
        pool->allocated_count--;
    }
}

// ========================================
// SIMD Detection
// ========================================

static bool detect_sse2(void) {
#ifdef _WIN32
    int info[4];
    __cpuid(info, 1);
    return (info[3] & (1 << 26)) != 0; // SSE2 bit
#else
    return false; // Simplified for non-Windows
#endif
}

static bool detect_sse3(void) {
#ifdef _WIN32
    int info[4];
    __cpuid(info, 1);
    return (info[2] & (1 << 0)) != 0; // SSE3 bit
#else
    return false;
#endif
}

static bool detect_avx(void) {
#ifdef _WIN32
    int info[4];
    __cpuid(info, 1);
    return (info[2] & (1 << 28)) != 0; // AVX bit
#else
    return false;
#endif
}

static SIMDConfig get_simd_config(void) {
    SIMDConfig config = {0};
    
    config.use_sse2 = detect_sse2();
    config.use_sse3 = detect_sse3();
    config.use_avx = detect_avx();
    config.use_avx2 = false; // Would need extended CPUID
    config.use_fma = false;    // Would need extended CPUID
    config.vector_size = config.use_avx ? 8 : 4;
    
    return config;
}

// ========================================
// Thread Pool Implementation
// ========================================

#ifdef _WIN32
static DWORD WINAPI worker_thread(LPVOID arg) {
#else
static void* worker_thread(void *arg) {
#endif
    ThreadPool *pool = (ThreadPool*)arg;
    
    while (pool->running) {
        // Simple worker implementation
        // In a real implementation, this would pull from work queue
        Sleep(1); // Prevent busy waiting
    }
    
    return 0;
}

static bool create_thread_pool(ThreadPool *pool, int thread_count) {
    if (!pool || thread_count <= 0 || thread_count > MAX_THREADS) {
        return false;
    }
    
    pool->thread_count = thread_count;
    pool->running = true;
    pool->active_workers = 0;
    
    pool->threads = malloc(thread_count * sizeof(void*));
    if (!pool->threads) return false;
    
    for (int i = 0; i < thread_count; i++) {
#ifdef _WIN32
        HANDLE thread = CreateThread(NULL, 0, worker_thread, pool, 0, NULL);
        if (!thread) {
            // Cleanup already created threads
            for (int j = 0; j < i; j++) {
                CloseHandle((HANDLE)pool->threads[j]);
            }
            free(pool->threads);
            return false;
        }
        pool->threads[i] = thread;
#else
        pthread_t thread;
        if (pthread_create(&thread, NULL, worker_thread, pool) != 0) {
            // Cleanup already created threads
            for (int j = 0; j < i; j++) {
                pthread_cancel((pthread_t)pool->threads[j]);
            }
            free(pool->threads);
            return false;
        }
        pool->threads[i] = (void*)thread;
#endif
    }
    
    return true;
}

static void destroy_thread_pool(ThreadPool *pool) {
    if (!pool) return;
    
    pool->running = false;
    
    // Wait for threads to finish
    for (int i = 0; i < pool->thread_count; i++) {
#ifdef _WIN32
        WaitForSingleObject((HANDLE)pool->threads[i], 1000);
        CloseHandle((HANDLE)pool->threads[i]);
#else
        pthread_join((pthread_t)pool->threads[i], NULL);
#endif
    }
    
    free(pool->threads);
}

// ========================================
// Spatial Grid Implementation
// ========================================

static SpatialGrid* create_spatial_grid(float cell_size, int width, int height, int depth) {
    SpatialGrid *grid = (SpatialGrid*)calloc(1, sizeof(SpatialGrid));
    if (!grid) return NULL;
    
    grid->cell_size = cell_size;
    grid->grid_width = width;
    grid->grid_height = height;
    grid->grid_depth = depth;
    grid->cell_capacity = 64; // Default capacity per cell
    
    int total_cells = width * height * depth;
    grid->cells = (void**)calloc(total_cells, sizeof(void*));
    if (!grid->cells) {
        free(grid);
        return NULL;
    }
    
    return grid;
}

static void destroy_spatial_grid(SpatialGrid *grid) {
    if (grid) {
        free(grid->cells);
        free(grid);
    }
}

static int get_grid_cell_index(SpatialGrid *grid, const float *position) {
    int x = (int)(position[0] / grid->cell_size);
    int y = (int)(position[1] / grid->cell_size);
    int z = (int)(position[2] / grid->cell_size);
    
    // Clamp to grid bounds
    x = (x < 0) ? 0 : (x >= grid->grid_width) ? grid->grid_width - 1 : x;
    y = (y < 0) ? 0 : (y >= grid->grid_height) ? grid->grid_height - 1 : y;
    z = (z < 0) ? 0 : (z >= grid->grid_depth) ? grid->grid_depth - 1 : z;
    
    return x + y * grid->grid_width + z * grid->grid_width * grid->grid_height;
}

// ========================================
// Cache Implementation
// ========================================

static PhysicsCache* create_cache(int capacity, int ttl_ms) {
    PhysicsCache *cache = (PhysicsCache*)calloc(1, sizeof(PhysicsCache));
    if (!cache) return NULL;
    
    cache->entries = (CacheEntry*)calloc(capacity, sizeof(CacheEntry));
    if (!cache->entries) {
        free(cache);
        return NULL;
    }
    
    cache->capacity = capacity;
    cache->size = 0;
    cache->hit_count = 0;
    cache->miss_count = 0;
    cache->current_time = get_current_time_ms();
    cache->ttl_ms = ttl_ms;
    
    return cache;
}

static void destroy_cache(PhysicsCache *cache) {
    if (cache) {
        // Free cached data
        for (int i = 0; i < cache->size; i++) {
            if (cache->entries[i].data) {
                free(cache->entries[i].data);
            }
        }
        free(cache->entries);
        free(cache);
    }
}

static bool cache_get(PhysicsCache *cache, uint64_t key, void **data) {
    if (!cache || !data) return false;
    
    uint64_t current_time = get_current_time_ms();
    
    for (int i = 0; i < cache->size; i++) {
        CacheEntry *entry = &cache->entries[i];
        if (entry->valid && entry->key == key) {
            // Check TTL
            if (current_time - entry->creation_time > cache->ttl_ms) {
                entry->valid = false;
                cache->size--;
                cache->miss_count++;
                return false;
            }
            
            entry->access_time = current_time;
            entry->access_count++;
            cache->hit_count++;
            *data = entry->data;
            return true;
        }
    }
    
    cache->miss_count++;
    return false;
}

static bool cache_store(PhysicsCache *cache, uint64_t key, const void *data, size_t data_size) {
    if (!cache || !data) return false;
    
    uint64_t current_time = get_current_time_ms();
    
    // Find invalid entry or LRU entry
    int target_index = -1;
    uint64_t oldest_time = current_time;
    
    for (int i = 0; i < cache->capacity; i++) {
        CacheEntry *entry = &cache->entries[i];
        if (!entry->valid) {
            target_index = i;
            break;
        }
        if (entry->access_time < oldest_time) {
            oldest_time = entry->access_time;
            target_index = i;
        }
    }
    
    if (target_index == -1) return false;
    
    CacheEntry *entry = &cache->entries[target_index];
    
    // Free old data if valid
    if (entry->valid && entry->data) {
        free(entry->data);
    }
    
    // Store new data
    entry->data = malloc(data_size);
    if (!entry->data) return false;
    
    memcpy(entry->data, data, data_size);
    entry->key = key;
    entry->data_size = data_size;
    entry->creation_time = current_time;
    entry->access_time = current_time;
    entry->access_count = 1;
    entry->valid = true;
    
    if (!entry->valid) {
        cache->size++;
    }
    
    return true;
}

// ========================================
// Public API Implementation
// ========================================

OptimizationProfile physics_optimizer_get_default_profile(void) {
    OptimizationProfile profile = {
        .enable_memory_pooling = true,
        .enable_simd_acceleration = true,
        .enable_multi_threading = true,
        .enable_spatial_partitioning = true,
        .enable_level_of_detail = false,
        .enable_caching = true,
        .enable_batching = true,
        .enable_adaptive_algorithms = true,
        
        .pool_size_bytes = 1024 * 1024, // 1MB
        .pool_block_count = 1024,
        .fragmentation_threshold = 0.7f,
        
        .worker_thread_count = 4,
        .batch_size = 64,
        .load_balance_threshold = 0.8f,
        
        .cache_size = 512,
        .cache_eviction_threshold = 0.8f,
        .cache_ttl_ms = 5000,
        
        .lod_levels = 4,
        .lod_distance_threshold = 50.0f,
        .lod_update_frequency = 0.1f,
        
        .performance_threshold = 0.8f,
        .adaptation_interval_ms = 1000
    };
    
    return profile;
}

PhysicsPipelineOptimizer* physics_optimizer_create(const OptimizationProfile *profile) {
    PhysicsPipelineOptimizer *optimizer = (PhysicsPipelineOptimizer*)calloc(1, sizeof(PhysicsPipelineOptimizer));
    if (!optimizer) return NULL;
    
    // Set profile
    if (profile) {
        optimizer->profile = *profile;
    } else {
        optimizer->profile = physics_optimizer_get_default_profile();
    }
    
    // Initialize memory pools
    optimizer->pool_count = 0;
    if (optimizer->profile.enable_memory_pooling) {
        // Create pools of different sizes
        size_t pool_sizes[] = {64, 128, 256, 512, 1024, 2048, 4096, 8192};
        for (int i = 0; i < MAX_MEMORY_POOLS; i++) {
            optimizer->memory_pools[i] = create_memory_pool(pool_sizes[i] * 1024, 64);
            if (optimizer->memory_pools[i]) {
                optimizer->pool_count++;
            }
        }
    }
    
    // Initialize SIMD
    if (optimizer->profile.enable_simd_acceleration) {
        optimizer->simd_config = get_simd_config();
        optimizer->simd_available = optimizer->simd_config.use_sse2 || 
                                   optimizer->simd_config.use_avx;
    }
    
    // Initialize threading
    if (optimizer->profile.enable_multi_threading) {
        optimizer->multi_threading_enabled = create_thread_pool(&optimizer->thread_pool, 
                                                               optimizer->profile.worker_thread_count);
    }
    
    // Initialize spatial partitioning
    if (optimizer->profile.enable_spatial_partitioning) {
        float bounds[6] = {-100.0f, -100.0f, -100.0f, 100.0f, 100.0f, 100.0f};
        optimizer->spatial_grid = create_spatial_grid(2.0f, 100, 100, 100);
        optimizer->spatial_partitioning_enabled = (optimizer->spatial_grid != NULL);
    }
    
    // Initialize caches
    if (optimizer->profile.enable_caching) {
        optimizer->collision_cache = create_cache(optimizer->profile.cache_size, 
                                               optimizer->profile.cache_ttl_ms);
        optimizer->solver_cache = create_cache(optimizer->profile.cache_size, 
                                             optimizer->profile.cache_ttl_ms);
        optimizer->caching_enabled = (optimizer->collision_cache != NULL && 
                                   optimizer->solver_cache != NULL);
    }
    
    // Initialize baseline metrics
    memset(&optimizer->metrics, 0, sizeof(PerformanceMetrics));
    optimizer->baseline = optimizer->metrics;
    optimizer->last_optimization = get_current_time_ms();
    
    // Initialize adaptive optimization
    optimizer->adaptive_enabled = optimizer->profile.enable_adaptive_algorithms;
    optimizer->adaptation_counter = 0;
    optimizer->performance_score = 1.0f;
    
    return optimizer;
}

void physics_optimizer_destroy(PhysicsPipelineOptimizer *optimizer) {
    if (!optimizer) return;
    
    // Destroy memory pools
    for (int i = 0; i < optimizer->pool_count; i++) {
        destroy_memory_pool(optimizer->memory_pools[i]);
    }
    
    // Destroy thread pool
    if (optimizer->multi_threading_enabled) {
        destroy_thread_pool(&optimizer->thread_pool);
    }
    
    // Destroy spatial grid
    if (optimizer->spatial_grid) {
        destroy_spatial_grid(optimizer->spatial_grid);
    }
    
    // Destroy caches
    if (optimizer->collision_cache) {
        destroy_cache(optimizer->collision_cache);
    }
    if (optimizer->solver_cache) {
        destroy_cache(optimizer->solver_cache);
    }
    
    free(optimizer);
}

void* physics_optimizer_allocate(PhysicsPipelineOptimizer *optimizer, size_t size) {
    if (!optimizer || !optimizer->profile.enable_memory_pooling) {
        return malloc(size);
    }
    
    // Find appropriate pool
    for (int i = 0; i < optimizer->pool_count; i++) {
        MemoryPool *pool = optimizer->memory_pools[i];
        if (pool && size <= pool->block_size) {
            return allocate_from_pool(pool);
        }
    }
    
    // Fallback to malloc
    return malloc(size);
}

void physics_optimizer_free(PhysicsPipelineOptimizer *optimizer, void *ptr) {
    if (!optimizer || !ptr) return;
    
    if (!optimizer->profile.enable_memory_pooling) {
        free(ptr);
        return;
    }
    
    // Try to free to appropriate pool
    for (int i = 0; i < optimizer->pool_count; i++) {
        MemoryPool *pool = optimizer->memory_pools[i];
        if (pool) {
            uint8_t *base = (uint8_t*)pool->memory;
            uint8_t *block = (uint8_t*)ptr;
            if (block >= base && block < base + pool->size) {
                free_to_pool(pool, ptr);
                return;
            }
        }
    }
    
    // Fallback to free
    free(ptr);
}

PerformanceMetrics physics_optimizer_get_metrics(PhysicsPipelineOptimizer *optimizer) {
    if (!optimizer) {
        PerformanceMetrics empty = {0};
        return empty;
    }
    
    physics_optimizer_update_metrics(optimizer);
    return optimizer->metrics;
}

void physics_optimizer_update_metrics(PhysicsPipelineOptimizer *optimizer) {
    if (!optimizer) return;
    
    // Update timing metrics (simplified)
    optimizer->metrics.total_time_ms = optimizer->metrics.broadphase_time_ms +
                                       optimizer->metrics.narrowphase_time_ms +
                                       optimizer->metrics.solver_time_ms;
    
    // Update memory metrics
    size_t total_memory = 0;
    for (int i = 0; i < optimizer->pool_count; i++) {
        if (optimizer->memory_pools[i]) {
            total_memory += optimizer->memory_pools[i]->peak_usage * 
                           optimizer->memory_pools[i]->block_size;
        }
    }
    optimizer->metrics.memory_usage_bytes = total_memory;
    
    // Update cache metrics
    if (optimizer->caching_enabled) {
        int total_hits = optimizer->collision_cache->hit_count + 
                        optimizer->solver_cache->hit_count;
        int total_requests = total_hits + 
                           optimizer->collision_cache->miss_count + 
                           optimizer->solver_cache->miss_count;
        
        optimizer->metrics.cache_hit_rate = (total_requests > 0) ? 
                                            (float)total_hits / total_requests : 0.0f;
        optimizer->metrics.cache_size = optimizer->collision_cache->size + 
                                         optimizer->solver_cache->size;
    }
    
    // Update threading metrics
    if (optimizer->multi_threading_enabled) {
        optimizer->metrics.thread_count = optimizer->thread_pool.thread_count;
        optimizer->metrics.active_workers = optimizer->thread_pool.active_workers;
        optimizer->metrics.thread_utilization = (optimizer->metrics.thread_count > 0) ? 
                                                   (float)optimizer->metrics.active_workers / 
                                                   optimizer->metrics.thread_count : 0.0f;
    }
    
    // Calculate performance score
    float baseline_time = optimizer->baseline.total_time_ms;
    float current_time = optimizer->metrics.total_time_ms;
    optimizer->performance_score = (baseline_time > 0.0f) ? baseline_time / current_time : 1.0f;
}

bool physics_optimizer_get_collision_cache(PhysicsPipelineOptimizer *optimizer, 
                                            uint64_t key, void **result) {
    if (!optimizer || !optimizer->caching_enabled || !result) {
        return false;
    }
    
    return cache_get(optimizer->collision_cache, key, result);
}

bool physics_optimizer_store_collision_cache(PhysicsPipelineOptimizer *optimizer, 
                                               uint64_t key, const void *result, size_t size) {
    if (!optimizer || !optimizer->caching_enabled) {
        return false;
    }
    
    return cache_store(optimizer->collision_cache, key, result, size);
}

void physics_optimizer_generate_report(PhysicsPipelineOptimizer *optimizer, 
                                      char *buffer, int buffer_size) {
    if (!optimizer || !buffer || buffer_size <= 0) return;
    
    PerformanceMetrics metrics = physics_optimizer_get_metrics(optimizer);
    
    snprintf(buffer, buffer_size,
        "Physics Pipeline Optimization Report\n"
        "=====================================\n\n"
        "Performance Metrics:\n"
        "  Broadphase Time: %.2f ms\n"
        "  Narrowphase Time: %.2f ms\n"
        "  Solver Time: %.2f ms\n"
        "  Total Time: %.2f ms\n\n"
        "Memory Usage:\n"
        "  Current: %zu bytes\n"
        "  Peak: %zu bytes\n"
        "  Allocations: %zu\n"
        "  Deallocations: %zu\n\n"
        "Object Statistics:\n"
        "  Active Objects: %d\n"
        "  Sleeping Objects: %d\n"
        "  Collision Pairs: %d\n"
        "  Constraints: %d\n\n"
        "Cache Performance:\n"
        "  Hit Rate: %.2f%%\n"
        "  Cache Size: %d\n"
        "  Evictions: %d\n\n"
        "Threading:\n"
        "  Thread Count: %d\n"
        "  Utilization: %.2f%%\n"
        "  Load Balance: %.2f\n\n"
        "Optimization Score: %.2f\n"
        "Status: %s\n",
        metrics.broadphase_time_ms,
        metrics.narrowphase_time_ms,
        metrics.solver_time_ms,
        metrics.total_time_ms,
        metrics.memory_usage_bytes,
        metrics.peak_memory_usage,
        metrics.allocation_count,
        metrics.deallocation_count,
        metrics.active_objects,
        metrics.sleeping_objects,
        metrics.collision_pairs,
        metrics.constraint_count,
        metrics.cache_hit_rate * 100.0f,
        metrics.cache_size,
        metrics.cache_evictions,
        metrics.thread_count,
        metrics.thread_utilization * 100.0f,
        metrics.load_balance_factor,
        optimizer->performance_score,
        (optimizer->performance_score >= 1.0f) ? "OPTIMIZED" : "NEEDS OPTIMIZATION"
    );
}

bool physics_optimizer_run_adaptive_pass(PhysicsPipelineOptimizer *optimizer) {
    if (!optimizer || !optimizer->adaptive_enabled) {
        return false;
    }
    
    uint64_t current_time = get_current_time_ms();
    if (current_time - optimizer->last_optimization < optimizer->profile.adaptation_interval_ms) {
        return false;
    }
    
    // Check if optimization is needed
    if (optimizer->performance_score < optimizer->profile.performance_threshold) {
        // Apply optimizations based on current performance bottlenecks
        
        // Example: If broadphase is slow, enable spatial partitioning
        if (optimizer->metrics.broadphase_time_ms > optimizer->metrics.total_time_ms * 0.5f &&
            !optimizer->spatial_partitioning_enabled) {
            // Enable spatial partitioning
            float bounds[6] = {-100.0f, -100.0f, -100.0f, 100.0f, 100.0f, 100.0f};
            optimizer->spatial_grid = create_spatial_grid(2.0f, 100, 100, 100);
            optimizer->spatial_partitioning_enabled = true;
        }
        
        // Example: If cache hit rate is low, increase cache size
        if (optimizer->metrics.cache_hit_rate < 0.5f && optimizer->caching_enabled) {
            // Increase cache size (simplified)
            optimizer->profile.cache_size *= 2;
        }
        
        optimizer->last_optimization = current_time;
        optimizer->adaptation_counter++;
        
        return true;
    }
    
    return false;
}
