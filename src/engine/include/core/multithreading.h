// include/engine/core/multithreading.h
//
// Purpose: Comprehensive multi-threading system for maximum scalability
// This system provides cutting-edge parallel processing capabilities that
// leverage modern multi-core CPUs for optimal performance across all engine
// systems.
//
// Key Features:
// - Work-stealing thread pool with load balancing
// - Lock-free data structures for high performance
// - Job system with dependency management
// - Parallel ECS system execution
// - Asynchronous asset loading and processing
// - Thread-safe memory management
// - Real-time thread profiling and optimization
// - CPU affinity and NUMA awareness
//
// Performance Advantages:
// - Near-linear scaling with CPU cores
// - Minimized lock contention and synchronization
// - Efficient work distribution and load balancing
// - Automatic thread affinity optimization
// - Real-time performance monitoring and tuning
//
// Public APIs:
// - ThreadPool: Work-stealing thread pool
// - JobSystem: Dependency-based job scheduling
// - ParallelECS: Parallel ECS execution
// - AsyncLoader: Asynchronous asset loading
// - ThreadProfiler: Real-time thread analysis
//
// Ownership: ThreadPool owns all worker threads
// Invariants: Thread operations must be synchronized for data consistency
//
#ifndef MULTITHREADING_H
#define MULTITHREADING_H

#include "include/common.h"
#include "include/core/performance.h"

// ============================================================================
// THREAD MANAGEMENT
// ============================================================================

// Thread priorities
typedef enum {
  THREAD_PRIORITY_IDLE = 0,
  THREAD_PRIORITY_LOW,
  THREAD_PRIORITY_NORMAL,
  THREAD_PRIORITY_HIGH,
  THREAD_PRIORITY_CRITICAL,
  THREAD_PRIORITY_REALTIME
} ThreadPriority;

// Thread states
typedef enum {
  THREAD_STATE_IDLE = 0,
  THREAD_STATE_RUNNING,
  THREAD_STATE_WAITING,
  THREAD_STATE_TERMINATING,
  THREAD_STATE_TERMINATED
} ThreadState;

// Thread affinity
typedef struct {
  u32 cpu_mask;  // Bitmask of CPU cores
  u32 numa_node; // NUMA node ID
  bool prefer_physical_cores;
  bool avoid_hyperthreading;
} ThreadAffinity;

// Thread information
typedef struct {
  u32 thread_id;
  u32 system_thread_id;
  char *thread_name;
  ThreadPriority priority;
  ThreadState state;
  ThreadAffinity affinity;
  f64 cpu_time;
  f64 active_time;
  u64 tasks_executed;
  u64 tasks_stolen;
  f64 steal_ratio;
  void *thread_handle;
  void *worker_context;
} ThreadInfo;

// ============================================================================
// WORK-STEALING THREAD POOL
// ============================================================================

// Work queue types
typedef enum {
  WORK_QUEUE_LOCAL = 0, // Thread-local queue
  WORK_QUEUE_GLOBAL,    // Global shared queue
  WORK_QUEUE_PRIORITY,  // Priority-based queue
  WORK_QUEUE_LOCK_FREE  // Lock-free queue
} WorkQueueType;

// Job function signature
typedef void (*JobFunction)(void *job_data, u32 job_id);

// Job descriptor
typedef struct {
  u32 job_id;
  JobFunction function;
  void *job_data;
  u32 job_size;
  JobFunction *dependencies;
  u32 dependency_count;
  u32 remaining_dependencies;
  ThreadPriority priority;
  bool is_completed;
  bool is_cancelled;
  f64 submit_time;
  f64 start_time;
  f64 completion_time;
  u32 executing_thread;
} Job;

// Work queue
typedef struct {
  WorkQueueType type;
  Job *jobs;
  u32 capacity;
  u32 head;
  u32 tail;
  u32 count;
  void *mutex;
  void *condition_variable;
  bool is_lock_free;
  u64 enqueue_count;
  u64 dequeue_count;
  u64 steal_count;
} WorkQueue;

// Thread worker context
typedef struct {
  ThreadInfo *thread_info;
  WorkQueue *local_queue;
  WorkQueue *global_queue;
  WorkQueue **steal_queues;
  u32 steal_queue_count;
  Job *current_job;
  u32 jobs_processed;
  u32 jobs_stolen;
  f64 total_processing_time;
  bool should_terminate;
} ThreadWorker;

// Thread pool configuration
typedef struct {
  u32 thread_count;
  u32 max_thread_count;
  ThreadPriority default_priority;
  bool enable_work_stealing;
  bool enable_thread_affinity;
  bool enable_numa_awareness;
  u32 queue_size_per_thread;
  u32 global_queue_size;
  bool enable_job_profiling;
  bool enable_load_balancing;
} ThreadPoolConfig;

// Thread pool
typedef struct {
  ThreadInfo *threads;
  u32 thread_count;
  u32 max_thread_count;
  ThreadWorker *workers;
  WorkQueue *global_queue;
  ThreadPoolConfig config;
  u32 next_job_id;
  void *shutdown_event;
  bool is_shutdown;
  f64 total_execution_time;
  u64 total_jobs_executed;
  f64 average_job_time;
} ThreadPool;

// ============================================================================
// JOB SYSTEM WITH DEPENDENCIES
// ============================================================================

// Job dependency types
typedef enum {
  DEPENDENCY_TYPE_FINISH = 0, // Wait for job to finish
  DEPENDENCY_TYPE_START,      // Wait for job to start
  DEPENDENCY_TYPE_DATA,       // Wait for data availability
  DEPENDENCY_TYPE_CUSTOM      // Custom dependency condition
} DependencyType;

// Job dependency
typedef struct {
  u32 dependency_job_id;
  DependencyType type;
  bool (*condition_func)(u32 job_id, void *context);
  void *condition_context;
  bool is_satisfied;
} JobDependency;

// Job group
typedef struct {
  u32 group_id;
  char *group_name;
  Job *jobs;
  u32 job_count;
  u32 max_jobs;
  JobDependency *dependencies;
  u32 dependency_count;
  u32 max_dependencies;
  bool is_completed;
  u32 completed_jobs;
  void *completion_callback;
  void *callback_context;
} JobGroup;

// Job system
typedef struct {
  ThreadPool *thread_pool;
  JobGroup *groups;
  u32 group_count;
  u32 max_groups;
  Job *all_jobs;
  u32 job_count;
  u32 max_jobs;
  JobDependency *dependencies;
  u32 dependency_count;
  u32 max_dependencies;
  bool enable_dependency_tracking;
  bool enable_job_cancellation;
  u32 next_group_id;
  f64 total_job_time;
  u64 total_jobs_completed;
} JobSystem;

// ============================================================================
// PARALLEL ECS EXECUTION
// ============================================================================

// ECS system execution types
typedef enum {
  ECS_EXECUTION_SINGLE = 0, // Single-threaded
  ECS_EXECUTION_PARALLEL,   // Parallel execution
  ECS_EXECUTION_PIPELINE,   // Pipeline execution
  ECS_EXECUTION_HYBRID      // Hybrid approach
} ECSExecutionType;

// Parallel ECS system
typedef struct {
  u32 system_id;
  char *system_name;
  JobFunction system_function;
  void *system_context;
  ECSExecutionType execution_type;
  u32 *dependent_systems;
  u32 dependency_count;
  ThreadPriority priority;
  bool is_parallelizable;
  u32 chunk_size;
  u32 max_chunks;
  Job *system_job;
  JobGroup *system_group;
} ParallelECSSystem;

// Parallel ECS world
typedef struct {
  void *ecs_world;
  ParallelECSSystem *systems;
  u32 system_count;
  u32 max_systems;
  JobSystem *job_system;
  bool enable_auto_parallelization;
  bool enable_load_balancing;
  u32 default_chunk_size;
  f64 total_ecs_time;
  u64 entities_processed;
} ParallelECSWorld;

// Parallel ECS executor
typedef struct {
  ParallelECSWorld *worlds;
  u32 world_count;
  u32 max_worlds;
  JobSystem *job_system;
  ThreadPool *thread_pool;
  bool enable_cross_world_parallelism;
  f64 total_execution_time;
} ParallelECSExecutor;

// ============================================================================
// ASYNCHRONOUS ASSET LOADING
// ============================================================================

// Asset loading stages
typedef enum {
  ASSET_STAGE_DISCOVERY = 0,
  ASSET_STAGE_LOADING,
  ASSET_STAGE_PROCESSING,
  ASSET_STAGE_OPTIMIZATION,
  ASSET_STAGE_CACHING,
  ASSET_STAGE_COMPLETED
} AssetStage;

// Asset load request
typedef struct {
  u32 request_id;
  char *asset_path;
  char *asset_type;
  void *asset_data;
  u64 asset_size;
  AssetStage current_stage;
  f32 progress;
  bool is_completed;
  bool is_cancelled;
  bool has_error;
  char *error_message;
  f64 submit_time;
  f64 completion_time;
  void (*completion_callback)(void *asset_data, void *context);
  void *callback_context;
  ThreadPriority priority;
} AssetLoadRequest;

// Asset loader
typedef struct {
  ThreadPool *thread_pool;
  AssetLoadRequest *requests;
  u32 request_count;
  u32 max_requests;
  WorkQueue *loading_queue;
  WorkQueue *processing_queue;
  bool enable_streaming;
  bool enable_caching;
  u32 max_concurrent_loads;
  char *cache_directory;
  f64 total_load_time;
  u64 total_bytes_loaded;
  f64 average_load_speed;
} AsyncAssetLoader;

// ============================================================================
// THREAD-SAFE MEMORY MANAGEMENT
// ============================================================================

// Memory pool types
typedef enum {
  MEMORY_POOL_THREAD_LOCAL = 0,
  MEMORY_POOL_SHARED,
  MEMORY_POOL_NUMA_AWARE,
  MEMORY_POOL_LOCK_FREE
} MemoryPoolType;

// Thread-safe memory pool
typedef struct {
  MemoryPoolType type;
  void *memory_blocks;
  u32 block_size;
  u32 block_count;
  u32 max_blocks;
  void *free_list;
  void *allocation_mutex;
  u32 thread_id; // For thread-local pools
  u32 numa_node; // For NUMA-aware pools
  u64 allocations_count;
  u64 deallocations_count;
  u64 peak_usage;
  bool enable_statistics;
} ThreadSafeMemoryPool;

// Thread-safe allocator
typedef struct {
  ThreadSafeMemoryPool *pools;
  u32 pool_count;
  u32 max_pools;
  void *global_mutex;
  bool enable_thread_caching;
  bool enable_numa_awareness;
  u64 total_allocated;
  u64 peak_allocated;
  u64 allocation_count;
} ThreadSafeAllocator;

// ============================================================================
// THREAD PROFILING SYSTEM
// ============================================================================

// Thread performance metrics
typedef struct {
  f64 cpu_time;
  f64 active_time;
  f64 idle_time;
  f64 wait_time;
  u64 context_switches;
  u64 cache_misses;
  u64 page_faults;
  f64 memory_bandwidth;
  f32 cpu_utilization;
  f32 steal_ratio;
  u64 jobs_executed;
  u64 jobs_stolen;
  f64 average_job_time;
} ThreadMetrics;

// Thread profiler
typedef struct {
  ThreadInfo *thread;
  ThreadMetrics metrics;
  u64 sample_count;
  f64 sampling_interval;
  bool is_profiling;
  f64 profiling_start_time;
  f64 total_cpu_time;
  f64 peak_cpu_utilization;
  f64 average_steal_ratio;
} ThreadProfiler;

// Thread profiling system
typedef struct {
  ThreadProfiler *profilers;
  u32 profiler_count;
  u32 max_profilers;
  bool enable_system_profiling;
  bool enable_detailed_profiling;
  f64 sampling_interval;
  u64 total_samples;
  f64 total_profiling_time;
} ThreadProfilingSystem;

// ============================================================================
// MAIN MULTITHREADING SYSTEM
// ============================================================================

// Multithreading system configuration
typedef struct {
  u32 worker_thread_count;
  u32 max_worker_threads;
  ThreadPriority default_priority;
  bool enable_work_stealing;
  bool enable_thread_affinity;
  bool enable_numa_awareness;
  bool enable_job_profiling;
  bool enable_thread_profiling;
  bool enable_async_loading;
  u32 queue_size_per_thread;
  u32 global_queue_size;
  u32 max_concurrent_asset_loads;
  bool enable_memory_tracking;
} MultithreadingConfig;

// Main multithreading system
typedef struct {
  // Configuration
  MultithreadingConfig config;

  // Thread pool
  ThreadPool *thread_pool;

  // Job system
  JobSystem *job_system;

  // Parallel ECS
  ParallelECSExecutor *ecs_executor;

  // Async asset loading
  AsyncAssetLoader *asset_loader;

  // Memory management
  ThreadSafeAllocator *memory_allocator;

  // Thread profiling
  ThreadProfilingSystem *profiling_system;

  // Performance
  Profiler *multithreading_profiler;
  f64 total_thread_time;
  f64 job_execution_time;
  f64 ecs_execution_time;
  f64 asset_loading_time;
  f64 memory_management_time;

  // Statistics
  u64 total_jobs_executed;
  u64 total_jobs_stolen;
  u64 total_assets_loaded;
  u64 total_bytes_processed;
  f64 average_cpu_utilization;
  f64 peak_cpu_utilization;
  u32 active_thread_count;
} MultithreadingSystem;

// ============================================================================
// PUBLIC API
// ============================================================================

// Multithreading system management
MultithreadingSystem *multithreading_create(const MultithreadingConfig *config);
void multithreading_destroy(MultithreadingSystem *system);
void multithreading_update(MultithreadingSystem *system, f32 delta_time);

// Configuration
MultithreadingConfig multithreading_create_default_config(void);
MultithreadingConfig multithreading_create_high_performance_config(void);
MultithreadingConfig multithreading_create_mobile_config(void);

// ============================================================================
// THREAD POOL API
// ============================================================================

// Thread pool management
ThreadPool *thread_pool_create(const ThreadPoolConfig *config);
void thread_pool_destroy(ThreadPool *pool);
bool thread_pool_start(ThreadPool *pool);
void thread_pool_shutdown(ThreadPool *pool);

// Job submission
u32 thread_pool_submit_job(ThreadPool *pool, JobFunction function,
                           void *job_data, u32 job_size);
u32 thread_pool_submit_job_with_priority(ThreadPool *pool, JobFunction function,
                                         void *job_data, u32 job_size,
                                         ThreadPriority priority);
bool thread_pool_submit_job_async(ThreadPool *pool, JobFunction function,
                                  void *job_data, u32 job_size, u32 *job_id);

// Job management
bool thread_pool_wait_for_job(ThreadPool *pool, u32 job_id);
bool thread_pool_cancel_job(ThreadPool *pool, u32 job_id);
bool thread_pool_is_job_completed(ThreadPool *pool, u32 job_id);
f64 thread_pool_get_job_progress(ThreadPool *pool, u32 job_id);

// Thread management
ThreadInfo *thread_pool_get_thread_info(ThreadPool *pool, u32 thread_id);
u32 thread_pool_get_active_thread_count(ThreadPool *pool);
f64 thread_pool_get_cpu_utilization(ThreadPool *pool);

// ============================================================================
// JOB SYSTEM API
// ============================================================================

// Job group management
JobGroup *job_system_create_group(JobSystem *system, const char *group_name);
bool job_system_add_job_to_group(JobSystem *system, JobGroup *group,
                                 JobFunction function, void *job_data,
                                 u32 job_size);
bool job_system_add_dependency(JobSystem *system, JobGroup *group, u32 job_id,
                               u32 dependency_job_id);
bool job_system_submit_group(JobSystem *system, JobGroup *group);

// Job execution
bool job_system_wait_for_group(JobSystem *system, JobGroup *group);
bool job_system_cancel_group(JobSystem *system, JobGroup *group);
bool job_system_is_group_completed(JobSystem *system, JobGroup *group);
f32 job_system_get_group_progress(JobSystem *system, JobGroup *group);

// ============================================================================
// PARALLEL ECS API
// ============================================================================

// Parallel ECS world management
ParallelECSWorld *parallel_ecs_create_world(void *ecs_world);
void parallel_ecs_destroy_world(ParallelECSWorld *world);
bool parallel_ecs_add_system(ParallelECSWorld *world, const char *system_name,
                             JobFunction function, void *context);
bool parallel_ecs_add_system_dependency(ParallelECSWorld *world,
                                        const char *system_name,
                                        const char *dependency_name);

// ECS execution
bool parallel_ecs_execute_world(ParallelECSWorld *world);
bool parallel_ecs_execute_all_worlds(ParallelECSExecutor *executor);
void parallel_ecs_enable_auto_parallelization(ParallelECSWorld *world,
                                              bool enable);
void parallel_ecs_set_chunk_size(ParallelECSWorld *world, u32 chunk_size);

// ============================================================================
// ASYNC ASSET LOADING API
// ============================================================================

// Asset loading
u32 async_load_asset(AsyncAssetLoader *loader, const char *asset_path,
                     const char *asset_type,
                     void (*callback)(void *asset_data, void *context),
                     void *context);
bool async_cancel_asset_load(AsyncAssetLoader *loader, u32 request_id);
bool async_is_asset_loaded(AsyncAssetLoader *loader, u32 request_id);
f32 async_get_asset_load_progress(AsyncAssetLoader *loader, u32 request_id);
void *async_get_loaded_asset(AsyncAssetLoader *loader, u32 request_id);

// Asset streaming
void async_enable_streaming(AsyncAssetLoader *loader, bool enable);
void async_set_max_concurrent_loads(AsyncAssetLoader *loader, u32 max_loads);
void async_set_cache_directory(AsyncAssetLoader *loader, const char *cache_dir);

// ============================================================================
// THREAD-SAFE MEMORY API
// ============================================================================

// Memory allocation
void *thread_safe_allocate(ThreadSafeAllocator *allocator, u64 size,
                           u32 alignment);
void thread_safe_deallocate(ThreadSafeAllocator *allocator, void *ptr);
void *thread_safe_reallocate(ThreadSafeAllocator *allocator, void *ptr,
                             u64 new_size);

// Memory pool management
ThreadSafeMemoryPool *thread_safe_create_pool(ThreadSafeAllocator *allocator,
                                              MemoryPoolType type,
                                              u32 block_size, u32 block_count);
void thread_safe_destroy_pool(ThreadSafeAllocator *allocator,
                              ThreadSafeMemoryPool *pool);
void *thread_safe_pool_allocate(ThreadSafeMemoryPool *pool);
void thread_safe_pool_deallocate(ThreadSafeMemoryPool *pool, void *ptr);

// ============================================================================
// THREAD PROFILING API
// ============================================================================

// Profiling control
void thread_profiling_enable(ThreadProfilingSystem *system, bool enable);
void thread_profiling_enable_detailed(ThreadProfilingSystem *system,
                                      bool enable);
ThreadProfiler *
thread_profiling_get_thread_profiler(ThreadProfilingSystem *system,
                                     u32 thread_id);

// Performance data
ThreadMetrics thread_profiling_get_metrics(ThreadProfilingSystem *system,
                                           u32 thread_id);
f32 thread_profiling_get_cpu_utilization(ThreadProfilingSystem *system,
                                         u32 thread_id);
f64 thread_profiling_get_steal_ratio(ThreadProfilingSystem *system,
                                     u32 thread_id);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Thread configuration macros
#define MULTITHREADING_DEFAULT_CONFIG()                                        \
  (MultithreadingConfig) {                                                     \
    .worker_thread_count = 0, .max_worker_threads = 64,                        \
    .default_priority = THREAD_PRIORITY_NORMAL, .enable_work_stealing = true,  \
    .enable_thread_affinity = true, .enable_numa_awareness = false,            \
    .enable_job_profiling = true, .enable_thread_profiling = true,             \
    .enable_async_loading = true, .queue_size_per_thread = 1024,               \
    .global_queue_size = 4096, .max_concurrent_asset_loads = 8,                \
    .enable_memory_tracking = true                                             \
  }

#define MULTITHREADING_HIGH_PERFORMANCE_CONFIG()                               \
  (MultithreadingConfig) {                                                     \
    .worker_thread_count = 0, .max_worker_threads = 128,                       \
    .default_priority = THREAD_PRIORITY_NORMAL, .enable_work_stealing = true,  \
    .enable_thread_affinity = true, .enable_numa_awareness = true,             \
    .enable_job_profiling = true, .enable_thread_profiling = true,             \
    .enable_async_loading = true, .queue_size_per_thread = 2048,               \
    .global_queue_size = 8192, .max_concurrent_asset_loads = 16,               \
    .enable_memory_tracking = true                                             \
  }

#define MULTITHREADING_MOBILE_CONFIG()                                         \
  (MultithreadingConfig) {                                                     \
    .worker_thread_count = 0, .max_worker_threads = 8,                         \
    .default_priority = THREAD_PRIORITY_NORMAL, .enable_work_stealing = true,  \
    .enable_thread_affinity = false, .enable_numa_awareness = false,           \
    .enable_job_profiling = false, .enable_thread_profiling = false,           \
    .enable_async_loading = true, .queue_size_per_thread = 256,                \
    .global_queue_size = 1024, .max_concurrent_asset_loads = 2,                \
    .enable_memory_tracking = true                                             \
  }

// Job submission macros
#define SUBMIT_JOB(pool, func, data)                                           \
  thread_pool_submit_job(pool, func, data, sizeof(data))

#define SUBMIT_JOB_PRIORITY(pool, func, data, priority)                        \
  thread_pool_submit_job_with_priority(pool, func, data, sizeof(data), priority)

#define SUBMIT_JOB_ASYNC(pool, func, data, job_id)                             \
  thread_pool_submit_job_async(pool, func, data, sizeof(data), job_id)

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// NUMA optimization
void multithreading_enable_numa_optimization(MultithreadingSystem *system,
                                             bool enable);
u32 multithreading_get_numa_node_count(MultithreadingSystem *system);
bool multithreading_bind_thread_to_numa_node(MultithreadingSystem *system,
                                             u32 thread_id, u32 numa_node);

// CPU affinity
bool multithreading_set_thread_affinity(MultithreadingSystem *system,
                                        u32 thread_id, u32 cpu_mask);
bool multithreading_optimize_thread_affinity(MultithreadingSystem *system);

// Load balancing
void multithreading_enable_dynamic_load_balancing(MultithreadingSystem *system,
                                                  bool enable);
void multithreading_rebalance_workload(MultithreadingSystem *system);

// Thread debugging
void multithreading_debug_render_thread_status(MultithreadingSystem *system);
void multithreading_debug_render_job_queue_status(MultithreadingSystem *system);
void multithreading_debug_render_memory_usage(MultithreadingSystem *system);

// Performance monitoring
typedef struct {
  f64 average_cpu_utilization;
  f64 peak_cpu_utilization;
  u64 total_jobs_executed;
  f64 average_job_time;
  u64 total_jobs_stolen;
  f64 average_steal_ratio;
  u64 total_assets_loaded;
  f64 average_load_speed;
  u32 active_thread_count;
} MultithreadingPerformanceReport;

MultithreadingPerformanceReport
multithreading_get_performance_report(MultithreadingSystem *system);
void multithreading_print_performance_report(MultithreadingSystem *system);

#endif // MULTITHREADING_H
