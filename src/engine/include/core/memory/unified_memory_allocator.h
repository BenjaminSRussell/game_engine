#pragma once

#include "../math/types.h"
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

// ============================================================================
// UNIFIED MEMORY ALLOCATOR - CONSOLIDATING ALL MEMORY SYSTEMS
// ============================================================================

// Memory allocation strategies (consolidated from all allocator types)
typedef enum {
    MEMORY_STRATEGY_DEFAULT = 0,    // Use default malloc/free
    MEMORY_STRATEGY_LINEAR,         // Linear allocator - fast, no free
    MEMORY_STRATEGY_STACK,          // Stack allocator - LIFO
    MEMORY_STRATEGY_POOL,           // Pool allocator - fixed-size blocks
    MEMORY_STRATEGY_BUDDY,          // Buddy allocator - power-of-2 blocks
    MEMORY_STRATEGY_ARENA,          // Arena allocator - multi-threaded
    MEMORY_STRATEGY_TRACKED,        // Tracked allocations with debugging
    MEMORY_STRATEGY_GPU,            // GPU memory management
    MEMORY_STRATEGY_ALIGNED,        // Aligned allocator for SIMD
    MEMORY_STRATEGY_COUNT
} MemoryStrategy;

// Memory tagging system (consolidated from all systems)
typedef enum {
    MEMORY_TAG_DEFAULT = 0,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_AUDIO,
    MEMORY_TAG_PHYSICS,
    MEMORY_TAG_AI,
    MEMORY_TAG_ANIMATION,
    MEMORY_TAG_TERRAIN,
    MEMORY_TAG_ASSETS,
    MEMORY_TAG_NETWORK,
    MEMORY_TAG_UI,
    MEMORY_TAG_TOOLS,
    MEMORY_TAG_TEMP,
    MEMORY_TAG_PERSISTENT,
    MEMORY_TAG_GPU,
    MEMORY_TAG_COUNT
} MemoryTag;

// Memory allocation flags (consolidated)
typedef enum {
    MEMORY_FLAG_NONE = 0,
    MEMORY_FLAG_ZERO = 1 << 0,      // Zero-initialize memory
    MEMORY_FLAG_ALIGN = 1 << 1,     // Align to cache line
    MEMORY_FLAG_GUARD = 1 << 2,     // Add guard pages for corruption detection
    MEMORY_FLAG_TRACK = 1 << 3,     // Track allocation for debugging
    MEMORY_FLAG_TEMP = 1 << 4,      // Temporary allocation (auto-cleanup)
    MEMORY_FLAG_THREAD_LOCAL = 1 << 5, // Thread-local allocation
    MEMORY_FLAG_COUNT
} MemoryFlags;

// Memory allocation metadata
typedef struct {
    void* ptr;
    size_t size;
    size_t actual_size;             // Including headers and alignment
    const char* file;
    int line;
    const char* function;
    MemoryStrategy strategy;
    MemoryFlags flags;
    u64 timestamp;
    u32 allocation_id;
    struct AllocationMetadata* next;
    struct AllocationMetadata* prev;
} AllocationMetadata;

// Memory pool configuration
typedef struct {
    size_t block_size;
    u32 block_count;
    u32 initial_blocks;
    bool auto_expand;
    size_t max_size;
    MemoryStrategy backing_strategy;
} MemoryPoolConfig;

// Memory pool structure
typedef struct MemoryPool {
    void* buffer;
    size_t buffer_size;
    size_t block_size;
    u32 total_blocks;
    u32 free_blocks;
    u32* free_list;
    u32 free_head;
    pthread_mutex_t mutex;
    MemoryPoolConfig config;
    struct MemoryPool* next;
    bool initialized;
} MemoryPool;

// Stack allocator structure
typedef struct StackAllocator {
    void* buffer;
    size_t buffer_size;
    size_t current_offset;
    size_t peak_offset;
    pthread_mutex_t mutex;
    void* markers[64];              // Stack markers for rollback
    u32 marker_count;
    bool initialized;
} StackAllocator;

// Arena allocator structure
typedef struct ArenaAllocator {
    void** blocks;
    size_t* block_sizes;
    u32 block_count;
    u32 current_block;
    size_t current_offset;
    size_t total_allocated;
    pthread_mutex_t mutex;
    bool initialized;
} ArenaAllocator;

// Memory statistics
typedef struct {
    u64 total_allocations;
    u64 total_deallocations;
    u64 current_allocations;
    size_t total_allocated;
    size_t peak_allocated;
    size_t fragmentation_ratio;
    u64 allocation_failures;
    u64 guard_page_hits;
    u64 double_frees_detected;
    u64 use_after_free_detected;
    size_t pool_usage[16];          // Per-pool usage statistics
    size_t stack_usage;             // Stack allocator usage
    size_t arena_usage;             // Arena allocator usage
} MemoryStats;

// Memory limits and policies
typedef struct {
    size_t global_limit;            // Global memory limit
    size_t per_pool_limit;          // Per-pool memory limit
    size_t per_stack_limit;         // Per-stack memory limit
    size_t per_arena_limit;         // Per-arena memory limit
    u32 max_allocations;            // Maximum number of allocations
    bool enable_guard_pages;        // Enable guard page protection
    bool enable_canaries;           // Enable canary values
    bool enable_stack_traces;       // Enable call stack tracking
    u32 stack_trace_depth;         // Maximum stack trace depth
    bool enable_leak_detection;     // Enable automatic leak detection
    bool enable_fragmentation_check; // Enable fragmentation monitoring
} MemoryPolicy;

// Memory allocator context
typedef struct {
    // Allocation tracking
    AllocationMetadata* allocations;
    pthread_mutex_t allocations_mutex;
    
    // Memory pools
    MemoryPool* pools;
    pthread_mutex_t pools_mutex;
    
    // Stack allocators
    StackAllocator* stacks;
    pthread_mutex_t stacks_mutex;
    
    // Arena allocators
    ArenaAllocator* arenas;
    pthread_mutex_t arenas_mutex;
    
    // Configuration and statistics
    MemoryPolicy policy;
    MemoryStats stats;
    pthread_mutex_t stats_mutex;
    
    // Global state
    bool initialized;
    u32 next_allocation_id;
    pthread_mutex_t global_mutex;
} UnifiedMemoryAllocator;

// ============================================================================
// UNIFIED MEMORY ALLOCATOR API
// ============================================================================

// Lifecycle
bool unified_memory_init(const MemoryPolicy* policy);
void unified_memory_shutdown(void);
void unified_memory_reset(void);

// Core allocation functions
void* unified_memory_alloc(size_t size, MemoryStrategy strategy, MemoryFlags flags, 
                        const char* file, int line, const char* function);
void* unified_memory_realloc(void* ptr, size_t new_size, MemoryFlags flags,
                           const char* file, int line, const char* function);
void unified_memory_free(void* ptr, const char* file, int line, const char* function);

// Convenience functions
#define UNIFIED_ALLOC(size, strategy, flags) \
    unified_memory_alloc(size, strategy, flags, __FILE__, __LINE__, __func__)
#define UNIFIED_FREE(ptr) \
    unified_memory_free(ptr, __FILE__, __LINE__, __func__)
#define UNIFIED_REALLOC(ptr, size, flags) \
    unified_memory_realloc(ptr, size, flags, __FILE__, __LINE__, __func__)

// Strategy-specific functions
void* unified_memory_pool_alloc(MemoryPool* pool, size_t size, MemoryFlags flags);
void unified_memory_pool_free(MemoryPool* pool, void* ptr);
MemoryPool* unified_memory_pool_create(const MemoryPoolConfig* config);
void unified_memory_pool_destroy(MemoryPool* pool);

void* unified_memory_stack_alloc(StackAllocator* stack, size_t size, MemoryFlags flags);
void unified_memory_stack_reset(StackAllocator* stack);
void* unified_memory_stack_marker(StackAllocator* stack);
void unified_memory_stack_rollback(StackAllocator* stack, void* marker);
StackAllocator* unified_memory_stack_create(size_t size);
void unified_memory_stack_destroy(StackAllocator* stack);

void* unified_memory_arena_alloc(ArenaAllocator* arena, size_t size, MemoryFlags flags);
void unified_memory_arena_reset(ArenaAllocator* arena);
ArenaAllocator* unified_memory_arena_create(size_t initial_block_size);
void unified_memory_arena_destroy(ArenaAllocator* arena);

// Memory management functions
void unified_memory_gc(void);                    // Garbage collection
void unified_memory_compact(void);                // Memory compaction
void unified_memory_defragment(void);              // Defragmentation
void unified_memory_check_leaks(void);             // Leak detection
void unified_memory_validate(void);                // Validate memory integrity
void unified_memory_dump_allocations(void);        // Dump all allocations

// Statistics and monitoring
MemoryStats unified_memory_get_stats(void);
void unified_memory_reset_stats(void);
void unified_memory_print_stats(void);
void unified_memory_print_allocation_info(void* ptr);

// Configuration
void unified_memory_set_policy(const MemoryPolicy* policy);
MemoryPolicy unified_memory_get_policy(void);
void unified_memory_set_global_limit(size_t limit);
void unified_memory_enable_leak_detection(bool enable);
void unified_memory_enable_guard_pages(bool enable);

// Debugging and analysis
void unified_memory_dump_stack_trace(void* ptr);
void unified_memory_check_corruption(void);
void unified_memory_analyze_fragments(void);
void unified_memory_profile_allocations(void);
void unified_memory_benchmark(void);

// Hot-spot detection
typedef struct {
    const char* file;
    int line;
    const char* function;
    u64 allocation_count;
    size_t total_size;
    f64 average_size;
} AllocationHotSpot;

AllocationHotSpot* unified_memory_get_hot_spots(u32* count);
void unified_memory_free_hot_spots(AllocationHotSpot* hot_spots);

// Memory regions for profiling
void unified_memory_region_begin(const char* name);
void unified_memory_region_end(const char* name);
void unified_memory_print_regions(void);

// ============================================================================
// BACKWARDS COMPATIBILITY
// ============================================================================

// Compatibility with existing memory.h
#define memory_alloc(size) unified_memory_alloc(size, MEMORY_STRATEGY_TRACKED, MEMORY_FLAG_TRACK, __FILE__, __LINE__, __func__)
#define memory_calloc(count, size) unified_memory_alloc((count) * (size), MEMORY_STRATEGY_TRACKED, MEMORY_FLAG_ZERO | MEMORY_FLAG_TRACK, __FILE__, __LINE__, __func__)
#define memory_realloc(ptr, size) unified_memory_realloc(ptr, size, MEMORY_FLAG_TRACK, __FILE__, __LINE__, __func__)
#define memory_free(ptr) unified_memory_free(ptr, __FILE__, __LINE__, __func__)

#define MALLOC(size) memory_alloc(size)
#define CALLOC(count, size) memory_calloc(count, size)
#define REALLOC(ptr, size) memory_realloc(ptr, size)
#define FREE(ptr) memory_free(ptr)

// Compatibility with memory_pool.h
#define object_pool_create(block_size, count) unified_memory_pool_create(&(MemoryPoolConfig){ .block_size = block_size, .block_count = count })
#define object_pool_destroy(pool) unified_memory_pool_destroy(pool)
#define object_pool_alloc(pool, size) unified_memory_pool_alloc(pool, size, MEMORY_FLAG_NONE)
#define object_pool_free(pool, ptr) unified_memory_pool_free(pool, ptr)

// Compatibility with stack allocator
#define stack_allocator_create(size) unified_memory_stack_create(size)
#define stack_allocator_destroy(stack) unified_memory_stack_destroy(stack)
#define stack_allocator_alloc(stack, size) unified_memory_stack_alloc(stack, size, MEMORY_FLAG_NONE)
#define stack_allocator_reset(stack) unified_memory_stack_reset(stack)
