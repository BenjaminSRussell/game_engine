/**
 * @file physics_pipeline_optimizer.h
 * @brief Physics pipeline performance optimization
 *
 * Provides optimization strategies and performance improvements for the
 * physics pipeline including multi-threading, memory management, and
 * algorithmic optimizations.
 */

#ifndef PHYSICS_PIPELINE_OPTIMIZER_H
#define PHYSICS_PIPELINE_OPTIMIZER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct PhysicsPipelineOptimizer PhysicsPipelineOptimizer;
typedef struct OptimizationProfile OptimizationProfile;

// ========================================
// Optimization Strategies
// ========================================

typedef enum OptimizationStrategy {
    OPT_NONE,                    // No optimization
    OPT_MEMORY_POOLING,          // Memory pooling for allocations
    OPT_SIMD_ACCELERATION,       // SIMD vectorization
    OPT_MULTI_THREADING,         // Multi-threaded execution
    OPT_SPATIAL_PARTITIONING,     // Spatial partitioning
    OPT_LEVEL_OF_DETAIL,         // Level of detail systems
    OPT_CACHING,                 // Result caching
    OPT_BATCHING,                 // Batch processing
    OPT_ADAPTIVE_ALGORITHMS       // Adaptive algorithm selection
} OptimizationStrategy;

// ========================================
// Performance Metrics
// ========================================

typedef struct PerformanceMetrics {
    // Timing metrics
    float broadphase_time_ms;     // Broadphase collision time
    float narrowphase_time_ms;    // Narrowphase collision time
    float solver_time_ms;         // Constraint solver time
    float total_time_ms;          // Total physics time
    
    // Memory metrics
    size_t memory_usage_bytes;    // Current memory usage
    size_t peak_memory_usage;      // Peak memory usage
    size_t allocation_count;       // Number of allocations
    size_t deallocation_count;     // Number of deallocations
    
    // Object metrics
    int active_objects;           // Number of active physics objects
    int sleeping_objects;         // Number of sleeping objects
    int collision_pairs;          // Number of collision pairs
    int constraint_count;          // Number of constraints
    
    // Cache metrics
    float cache_hit_rate;         // Cache hit rate (0-1)
    int cache_size;               // Cache size
    int cache_evictions;          // Number of cache evictions
    
    // Threading metrics
    int thread_count;             // Number of worker threads
    float thread_utilization;     // Thread utilization (0-1)
    float load_balance_factor;     // Load balance factor (0-1)
    
} PerformanceMetrics;

// ========================================
// Optimization Profile
// ========================================

struct OptimizationProfile {
    // Enabled strategies
    bool enable_memory_pooling;
    bool enable_simd_acceleration;
    bool enable_multi_threading;
    bool enable_spatial_partitioning;
    bool enable_level_of_detail;
    bool enable_caching;
    bool enable_batching;
    bool enable_adaptive_algorithms;
    
    // Memory optimization
    size_t pool_size_bytes;        // Memory pool size
    int pool_block_count;          // Number of pool blocks
    float fragmentation_threshold;  // Fragmentation threshold
    
    // Threading optimization
    int worker_thread_count;       // Number of worker threads
    int batch_size;                // Batch size for processing
    float load_balance_threshold;  // Load balance threshold
    
    // Cache optimization
    int cache_size;                // Cache size
    float cache_eviction_threshold; // Cache eviction threshold
    int cache_ttl_ms;              // Cache time-to-live
    
    // Level of detail
    int lod_levels;                // Number of LOD levels
    float lod_distance_threshold;  // LOD distance threshold
    float lod_update_frequency;    // LOD update frequency
    
    // Adaptive algorithms
    float performance_threshold;    // Performance threshold for adaptation
    int adaptation_interval_ms;     // Adaptation interval
    
};

// ========================================
// Memory Pool Implementation
// ========================================

typedef struct MemoryPool {
    void *memory;                  // Pool memory block
    size_t size;                   // Pool size
    size_t block_size;             // Block size
    int block_count;                // Number of blocks
    uint32_t *free_list;           // Free list of blocks
    int free_count;                 // Number of free blocks
    int allocated_count;            // Number of allocated blocks
    size_t peak_usage;              // Peak memory usage
} MemoryPool;

// ========================================
// SIMD Optimization
// ========================================

typedef struct SIMDConfig {
    bool use_sse2;                  // Use SSE2 instructions
    bool use_sse3;                  // Use SSE3 instructions
    bool use_sse4_1;                // Use SSE4.1 instructions
    bool use_avx;                   // Use AVX instructions
    bool use_avx2;                  // Use AVX2 instructions
    bool use_fma;                   // Use FMA instructions
    int vector_size;                // Vector size (4, 8, 16)
} SIMDConfig;

// ========================================
// Multi-threading Support
// ========================================

typedef struct ThreadPool {
    int thread_count;               // Number of threads
    void *threads;                  // Thread handles
    volatile bool running;         // Thread running flag
    void *work_queue;               // Work queue
    void *completion_queue;         // Completion queue
    int active_workers;             // Number of active workers
} ThreadPool;

typedef struct WorkItem {
    void (*function)(void *data);   // Work function
    void *data;                     // Work data
    int priority;                   // Work priority
    uint64_t id;                    // Work item ID
} WorkItem;

// ========================================
// Spatial Partitioning
// ========================================

typedef struct SpatialGrid {
    float cell_size;                // Grid cell size
    int grid_width, grid_height, grid_depth; // Grid dimensions
    void **cells;                   // Grid cells
    int cell_capacity;              // Capacity per cell
    int total_objects;              // Total objects
} SpatialGrid;

// ========================================
// Cache System
// ========================================

typedef struct CacheEntry {
    uint64_t key;                  // Cache key
    void *data;                     // Cached data
    size_t data_size;               // Data size
    uint64_t access_time;           // Last access time
    uint64_t creation_time;         // Creation time
    int access_count;               // Access count
    bool valid;                     // Entry is valid
} CacheEntry;

typedef struct PhysicsCache {
    CacheEntry *entries;            // Cache entries
    int capacity;                   // Cache capacity
    int size;                       // Current size
    int hit_count;                  // Number of hits
    int miss_count;                 // Number of misses
    uint64_t current_time;          // Current time
    int ttl_ms;                     // Time-to-live in milliseconds
} PhysicsCache;

// ========================================
// Level of Detail System
// ========================================

typedef struct LODLevel {
    int level;                      // LOD level (0 = highest)
    float distance_threshold;        // Distance threshold
    float reduction_factor;          // Geometry reduction factor
    float physics_scale;             // Physics simulation scale
    int max_vertices;               // Maximum vertices
    int max_constraints;            // Maximum constraints
} LODLevel;

typedef struct LODManager {
    LODLevel *levels;               // LOD levels
    int level_count;                // Number of LOD levels
    float update_frequency;         // Update frequency
    uint64_t last_update;           // Last update time
    int active_objects;             // Number of active objects
} LODManager;

// ========================================
// Pipeline Optimizer Structure
// ========================================

struct PhysicsPipelineOptimizer {
    // Configuration
    OptimizationProfile profile;
    
    // Memory optimization
    MemoryPool *memory_pools[8];    // Memory pools for different sizes
    int pool_count;                 // Number of memory pools
    
    // SIMD optimization
    SIMDConfig simd_config;
    bool simd_available;             // SIMD instructions available
    
    // Multi-threading
    ThreadPool thread_pool;          // Worker thread pool
    bool multi_threading_enabled;    // Multi-threading enabled
    
    // Spatial partitioning
    SpatialGrid *spatial_grid;       // Spatial partitioning grid
    bool spatial_partitioning_enabled; // Spatial partitioning enabled
    
    // Cache system
    PhysicsCache *collision_cache;   // Collision result cache
    PhysicsCache *solver_cache;      // Solver result cache
    bool caching_enabled;            // Caching enabled
    
    // Level of detail
    LODManager *lod_manager;         // LOD manager
    bool lod_enabled;                 // LOD enabled
    
    // Performance monitoring
    PerformanceMetrics metrics;       // Current metrics
    PerformanceMetrics baseline;       // Baseline metrics
    uint64_t last_optimization;      // Last optimization time
    
    // Adaptive optimization
    bool adaptive_enabled;           // Adaptive optimization enabled
    int adaptation_counter;          // Adaptation counter
    float performance_score;          // Current performance score
    
};

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create physics pipeline optimizer
 * @param profile Optimization profile (NULL for default)
 * @return Optimizer instance or NULL on failure
 */
PhysicsPipelineOptimizer* physics_optimizer_create(const OptimizationProfile *profile);

/**
 * Destroy physics pipeline optimizer
 * @param optimizer Optimizer instance
 */
void physics_optimizer_destroy(PhysicsPipelineOptimizer *optimizer);

// ========================================
// Configuration Management
// ========================================

/**
 * Get default optimization profile
 * @return Default profile
 */
OptimizationProfile physics_optimizer_get_default_profile(void);

/**
 * Set optimization profile
 * @param optimizer Optimizer instance
 * @param profile New profile
 */
void physics_optimizer_set_profile(PhysicsPipelineOptimizer *optimizer, const OptimizationProfile *profile);

/**
 * Get current optimization profile
 * @param optimizer Optimizer instance
 * @return Current profile
 */
OptimizationProfile physics_optimizer_get_profile(PhysicsPipelineOptimizer *optimizer);

/**
 * Enable/disable optimization strategies
 * @param optimizer Optimizer instance
 * @param strategies Array of strategies to enable
 * @param strategy_count Number of strategies
 */
void physics_optimizer_enable_strategies(PhysicsPipelineOptimizer *optimizer, 
                                         const OptimizationStrategy *strategies, int strategy_count);

// ========================================
// Memory Optimization
// ========================================

/**
 * Allocate memory from optimized pool
 * @param optimizer Optimizer instance
 * @param size Size to allocate
 * @return Allocated memory or NULL
 */
void* physics_optimizer_allocate(PhysicsPipelineOptimizer *optimizer, size_t size);

/**
 * Free memory to optimized pool
 * @param optimizer Optimizer instance
 * @param ptr Memory to free
 */
void physics_optimizer_free(PhysicsPipelineOptimizer *optimizer, void *ptr);

/**
 * Get memory usage statistics
 * @param optimizer Optimizer instance
 * @param total_usage Total memory usage
 * @param pool_usage Pool-specific usage
 */
void physics_optimizer_get_memory_stats(PhysicsPipelineOptimizer *optimizer, 
                                       size_t *total_usage, size_t *pool_usage);

// ========================================
// SIMD Optimization
// ========================================

/**
 * Check SIMD availability
 * @param optimizer Optimizer instance
 * @return True if SIMD is available
 */
bool physics_optimizer_has_simd(PhysicsPipelineOptimizer *optimizer);

/**
 * Get SIMD configuration
 * @param optimizer Optimizer instance
 * @return SIMD configuration
 */
SIMDConfig physics_optimizer_get_simd_config(PhysicsPipelineOptimizer *optimizer);

/**
 * Enable SIMD optimizations
 * @param optimizer Optimizer instance
 * @param config SIMD configuration
 */
void physics_optimizer_enable_simd(PhysicsPipelineOptimizer *optimizer, const SIMDConfig *config);

// ========================================
// Multi-threading Optimization
// ========================================

/**
 * Initialize worker threads
 * @param optimizer Optimizer instance
 * @param thread_count Number of worker threads
 * @return True if successful
 */
bool physics_optimizer_init_threads(PhysicsPipelineOptimizer *optimizer, int thread_count);

/**
 * Submit work to thread pool
 * @param optimizer Optimizer instance
 * @param work Work item to submit
 * @return True if work submitted successfully
 */
bool physics_optimizer_submit_work(PhysicsPipelineOptimizer *optimizer, const WorkItem *work);

/**
 * Wait for all work to complete
 * @param optimizer Optimizer instance
 * @param timeout_ms Timeout in milliseconds
 * @return True if all work completed
 */
bool physics_optimizer_wait_for_completion(PhysicsPipelineOptimizer *optimizer, int timeout_ms);

/**
 * Get thread utilization statistics
 * @param optimizer Optimizer instance
 * @param utilization Thread utilization (0-1)
 * @param active_workers Number of active workers
 */
void physics_optimizer_get_thread_stats(PhysicsPipelineOptimizer *optimizer, 
                                       float *utilization, int *active_workers);

// ========================================
// Spatial Partitioning
// ========================================

/**
 * Initialize spatial partitioning
 * @param optimizer Optimizer instance
 * @param cell_size Grid cell size
 * @param bounds World bounds
 * @return True if successful
 */
bool physics_optimizer_init_spatial_partitioning(PhysicsPipelineOptimizer *optimizer, 
                                                  float cell_size, const float *bounds);

/**
 * Update spatial partitioning
 * @param optimizer Optimizer instance
 * @param objects Array of objects
 * @param object_count Number of objects
 */
void physics_optimizer_update_spatial_partitioning(PhysicsPipelineOptimizer *optimizer,
                                                   void **objects, int object_count);

/**
 * Query spatial partitioning for nearby objects
 * @param optimizer Optimizer instance
 * @param position Query position
 * @param radius Query radius
 * @param results Output results
 * @param max_results Maximum results
 * @return Number of results found
 */
int physics_optimizer_query_spatial_partitioning(PhysicsPipelineOptimizer *optimizer,
                                                  const float *position, float radius,
                                                  void **results, int max_results);

// ========================================
// Cache Optimization
// ========================================

/**
 * Initialize cache systems
 * @param optimizer Optimizer instance
 * @param collision_cache_size Collision cache size
 * @param solver_cache_size Solver cache size
 * @return True if successful
 */
bool physics_optimizer_init_caches(PhysicsPipelineOptimizer *optimizer, 
                                   int collision_cache_size, int solver_cache_size);

/**
 * Get cached collision result
 * @param optimizer Optimizer instance
 * @param key Cache key
 * @param result Output result
 * @return True if cache hit
 */
bool physics_optimizer_get_collision_cache(PhysicsPipelineOptimizer *optimizer, 
                                            uint64_t key, void **result);

/**
 * Store collision result in cache
 * @param optimizer Optimizer instance
 * @param key Cache key
 * @param result Result to store
 * @param size Result size
 * @return True if stored successfully
 */
bool physics_optimizer_store_collision_cache(PhysicsPipelineOptimizer *optimizer, 
                                               uint64_t key, const void *result, size_t size);

/**
 * Clear caches
 * @param optimizer Optimizer instance
 */
void physics_optimizer_clear_caches(PhysicsPipelineOptimizer *optimizer);

/**
 * Get cache statistics
 * @param optimizer Optimizer instance
 * @param hit_rate Cache hit rate (0-1)
 * @param cache_size Current cache size
 */
void physics_optimizer_get_cache_stats(PhysicsPipelineOptimizer *optimizer, 
                                      float *hit_rate, int *cache_size);

// ========================================
// Level of Detail Optimization
// ========================================

/**
 * Initialize LOD system
 * @param optimizer Optimizer instance
 * @param levels LOD levels
 * @param level_count Number of levels
 * @return True if successful
 */
bool physics_optimizer_init_lod(PhysicsPipelineOptimizer *optimizer, 
                                const LODLevel *levels, int level_count);

/**
 * Update LOD for objects
 * @param optimizer Optimizer instance
 * @param camera_position Camera position
 * @param objects Array of objects
 * @param object_count Number of objects
 */
void physics_optimizer_update_lod(PhysicsPipelineOptimizer *optimizer,
                                  const float *camera_position,
                                  void **objects, int object_count);

/**
 * Get LOD level for object
 * @param optimizer Optimizer instance
 * @param object Object to query
 * @param camera_position Camera position
 * @return LOD level
 */
int physics_optimizer_get_lod_level(PhysicsPipelineOptimizer *optimizer,
                                   const void *object, const float *camera_position);

// ========================================
// Performance Monitoring
// ========================================

/**
 * Update performance metrics
 * @param optimizer Optimizer instance
 */
void physics_optimizer_update_metrics(PhysicsPipelineOptimizer *optimizer);

/**
 * Get current performance metrics
 * @param optimizer Optimizer instance
 * @return Performance metrics
 */
PerformanceMetrics physics_optimizer_get_metrics(PhysicsPipelineOptimizer *optimizer);

/**
 * Get performance improvement metrics
 * @param optimizer Optimizer instance
 * @param improvement Percentage improvement
 * @param speedup Speedup factor
 */
void physics_optimizer_get_improvement(PhysicsPipelineOptimizer *optimizer, 
                                        float *improvement, float *speedup);

/**
 * Generate performance report
 * @param optimizer Optimizer instance
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 */
void physics_optimizer_generate_report(PhysicsPipelineOptimizer *optimizer, 
                                      char *buffer, int buffer_size);

// ========================================
// Adaptive Optimization
// ========================================

/**
 * Enable adaptive optimization
 * @param optimizer Optimizer instance
 * @param enabled Enable adaptive optimization
 */
void physics_optimizer_enable_adaptive(PhysicsPipelineOptimizer *optimizer, bool enabled);

/**
 * Run adaptive optimization pass
 * @param optimizer Optimizer instance
 * @return True if optimization applied
 */
bool physics_optimizer_run_adaptive_pass(PhysicsPipelineOptimizer *optimizer);

/**
 * Get optimization recommendations
 * @param optimizer Optimizer instance
 * @param recommendations Output recommendations
 * @param max_recommendations Maximum recommendations
 * @return Number of recommendations
 */
int physics_optimizer_get_recommendations(PhysicsPipelineOptimizer *optimizer,
                                           char **recommendations, int max_recommendations);

// ========================================
// Utility Functions
// ========================================

/**
 * Reset optimizer state
 * @param optimizer Optimizer instance
 */
void physics_optimizer_reset(PhysicsPipelineOptimizer *optimizer);

/**
 * Validate optimizer state
 * @param optimizer Optimizer instance
 * @return True if state is valid
 */
bool physics_optimizer_validate(PhysicsPipelineOptimizer *optimizer);

/**
 * Save optimizer configuration
 * @param optimizer Optimizer instance
 * @param filename Configuration file
 * @return True if saved successfully
 */
bool physics_optimizer_save_config(PhysicsPipelineOptimizer *optimizer, const char *filename);

/**
 * Load optimizer configuration
 * @param optimizer Optimizer instance
 * @param filename Configuration file
 * @return True if loaded successfully
 */
bool physics_optimizer_load_config(PhysicsPipelineOptimizer *optimizer, const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_PIPELINE_OPTIMIZER_H */
