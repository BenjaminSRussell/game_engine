#pragma once

#include "../math/types.h"
#include <stddef.h>

// ============================================================================
// UNIFIED MEMORY ALLOCATOR SYSTEM
// ============================================================================

// Memory allocation strategies
typedef enum {
    ALLOCATOR_STRATEGY_LINEAR = 0,    // Fast, no free, reset only
    ALLOCATOR_STRATEGY_STACK,          // LIFO allocation/free
    ALLOCATOR_STRATEGY_POOL,           // Fixed-size blocks
    ALLOCATOR_STRATEGY_BUDDY,          // Power-of-2 blocks, no fragmentation
    ALLOCATOR_STRATEGY_TRACKED,        // General purpose with leak detection
    ALLOCATOR_STRATEGY_COUNT
} AllocatorStrategy;

// Memory tagging system
typedef enum {
    MEMORY_TAG_RENDERER = 0,
    MEMORY_TAG_AUDIO = 1,
    MEMORY_TAG_PHYSICS = 2,
    MEMORY_TAG_AI = 3,
    MEMORY_TAG_ANIMATION = 4,
    MEMORY_TAG_TERRAIN = 5,
    MEMORY_TAG_ASSETS = 6,
    MEMORY_TAG_NETWORK = 7,
    MEMORY_TAG_UI = 8,
    MEMORY_TAG_TOOLS = 9,
    MEMORY_TAG_COUNT
} MemoryTag;

// Forward declarations
typedef struct Allocator Allocator;
typedef struct LinearAllocator LinearAllocator;
typedef struct StackAllocator StackAllocator;
typedef struct PoolAllocator PoolAllocator;
typedef struct BuddyAllocator BuddyAllocator;
typedef struct TrackedAllocator TrackedAllocator;

// Base allocator interface
typedef struct {
    void* (*alloc)(Allocator* allocator, size_t size, size_t alignment);
    void* (*realloc)(Allocator* allocator, void* ptr, size_t new_size);
    void (*free)(Allocator* allocator, void* ptr);
    void (*reset)(Allocator* allocator);
    size_t (*get_usage)(Allocator* allocator);
    size_t (*get_capacity)(Allocator* allocator);
    void (*get_stats)(Allocator* allocator, char* buffer, size_t buffer_size);
    void (*destroy)(Allocator* allocator);
} AllocatorVTable;

// Base allocator structure
struct Allocator {
    const AllocatorVTable* vtable;
    const char* name;
    MemoryTag tag;
    AllocatorStrategy strategy;
    Allocator* fallback; // Fallback allocator for out-of-memory
    void* impl;          // Implementation-specific data
    bool owns_fallback;
};

// Linear allocator - ultra fast, no individual frees
struct LinearAllocator {
    void* base;
    size_t size;
    size_t offset;
    size_t peak_usage;
    u32 buffer_count;
    u32 current_buffer;
    void** buffers; // Double-buffering for multi-threading
};

// Stack allocator - LIFO allocation/free
struct StackAllocator {
    void* base;
    size_t size;
    size_t offset;
    size_t peak_usage;
    void** stack; // Stack of allocation markers
    u32 stack_capacity;
    u32 stack_top;
};

// Pool allocator - fixed-size blocks
struct PoolAllocator {
    void* base;
    size_t block_size;
    size_t block_count;
    void** free_list;
    size_t free_count;
    size_t allocated_count;
    size_t peak_allocated;
};

// Buddy allocator - power-of-2 blocks, zero fragmentation
typedef struct BuddyBlock {
    struct BuddyBlock* left;
    struct BuddyBlock* right;
    struct BuddyBlock* parent;
    size_t size;
    u32 level;
    bool is_free;
    bool is_split;
} BuddyBlock;

struct BuddyAllocator {
    void* memory_pool;
    size_t pool_size;
    size_t min_block_size;
    size_t max_block_size;
    u32 max_level;
    BuddyBlock* root;
    BuddyBlock** free_lists; // Array of free lists for each level
    u32 total_blocks;
    u32 free_blocks;
};

// Tracked allocator - general purpose with full leak detection
typedef struct AllocationHeader {
    size_t size;
    const char* file;
    u32 line;
    MemoryTag tag;
    u64 allocation_id;
    u64 timestamp;
    struct AllocationHeader* next;
    struct AllocationHeader* prev;
    void* stack_trace[16]; // Stack trace for leak detection
    u32 stack_depth;
} AllocationHeader;

struct TrackedAllocator {
    Allocator* backing_allocator;
    AllocationHeader* allocations;
    size_t total_allocated;
    size_t peak_allocated;
    u64 allocation_count;
    u64 allocation_counter;
    bool stack_trace_enabled;
    bool advanced_leak_detection;
    size_t memory_limit;
    bool enforce_limits;
    size_t tag_usage[MEMORY_TAG_COUNT];
    size_t tag_limits[MEMORY_TAG_COUNT];
};

// Memory allocation statistics
typedef struct {
    size_t total_allocated;
    size_t peak_allocated;
    size_t capacity;
    u64 allocation_count;
    u64 free_count;
    f32 fragmentation_ratio;
    size_t largest_free_block;
    u32 free_block_count;
} AllocatorStats;

// Memory profiler snapshot
typedef struct {
    u64 timestamp;
    size_t total_usage;
    size_t tag_usage[MEMORY_TAG_COUNT];
    u32 allocation_count;
    u32 free_count;
    f32 fragmentation;
} MemorySnapshot;

typedef struct {
    MemorySnapshot* snapshots;
    u32 snapshot_count;
    u32 max_snapshots;
    u32 current_index;
    bool recording;
} MemoryProfiler;

// ============================================================================
// CREATION FUNCTIONS
// ============================================================================

// Create allocators with different strategies
Allocator* allocator_create_linear(size_t size, const char* name, MemoryTag tag);
Allocator* allocator_create_stack(size_t size, const char* name, MemoryTag tag);
Allocator* allocator_create_pool(size_t block_size, size_t block_count, const char* name, MemoryTag tag);
Allocator* allocator_create_buddy(size_t min_block_size, size_t max_block_size, const char* name, MemoryTag tag);
Allocator* allocator_create_tracked(Allocator* backing, const char* name, MemoryTag tag);

// Create custom allocator with user-provided vtable
Allocator* allocator_create_custom(const AllocatorVTable* vtable, const char* name, 
                                  MemoryTag tag, Allocator* fallback);

// ============================================================================
// ALLOCATION FUNCTIONS
// ============================================================================

// Core allocation interface
void* allocator_alloc(Allocator* allocator, size_t size);
void* allocator_alloc_aligned(Allocator* allocator, size_t size, size_t alignment);
void* allocator_realloc(Allocator* allocator, void* ptr, size_t new_size);
void allocator_free(Allocator* allocator, void* ptr);
void allocator_reset(Allocator* allocator);

// Convenience macros
#define ALLOC(allocator, size) allocator_alloc(allocator, size)
#define ALLOC_ALIGNED(allocator, size, alignment) allocator_alloc_aligned(allocator, size, alignment)
#define REALLOC(allocator, ptr, size) allocator_realloc(allocator, ptr, size)
#define FREE(allocator, ptr) allocator_free(allocator, ptr)

// Tagged allocation convenience macros
#define ALLOC_TAGGED(allocator, size, tag) \
    allocator_alloc_tagged(allocator, size, tag, __FILE__, __LINE__)
#define ALLOC_TEMP(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_TEMP)
#define ALLOC_PERSISTENT(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_PERSISTENT)
#define ALLOC_ASSET(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_ASSET)
#define ALLOC_RENDERER(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_RENDERER)
#define ALLOC_AUDIO(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_AUDIO)
#define ALLOC_PHYSICS(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_PHYSICS)
#define ALLOC_AI(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_AI)
#define ALLOC_NETWORK(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_NETWORK)
#define ALLOC_UI(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_UI)
#define ALLOC_GAMEPLAY(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_GAMEPLAY)
#define ALLOC_GEOMETRY(allocator, size) ALLOC_TAGGED(allocator, size, MEMORY_TAG_GEOMETRY)

// ============================================================================
// STATISTICS AND MONITORING
// ============================================================================

// Get allocator statistics
void allocator_get_stats(Allocator* allocator, AllocatorStats* stats);
void allocator_print_stats(Allocator* allocator);
size_t allocator_get_usage(Allocator* allocator);
size_t allocator_get_capacity(Allocator* allocator);

// Memory profiler
void memory_profiler_init(u32 max_snapshots);
void memory_profiler_shutdown(void);
void memory_profiler_start_recording(void);
void memory_profiler_stop_recording(void);
void memory_profiler_capture_snapshot(Allocator* allocator);
MemorySnapshot* memory_profiler_get_snapshots(u32* count);
void memory_profiler_export_timeline(const char* filename);

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Stack trace support
void allocator_enable_stack_traces(Allocator* allocator, bool enable);
void allocator_capture_stack_trace(void** buffer, u32 max_depth, u32* out_depth);
char** allocator_resolve_stack_trace(void** addresses, u32 depth);
void allocator_free_resolved_stack_trace(char** symbols);

// Memory limits and enforcement
void allocator_set_memory_limit(Allocator* allocator, size_t limit_bytes);
void allocator_set_enforcement(Allocator* allocator, bool enabled);
size_t allocator_get_memory_limit(Allocator* allocator);
bool allocator_is_enforcement_enabled(Allocator* allocator);
void allocator_set_tag_limit(Allocator* allocator, MemoryTag tag, size_t limit);
size_t allocator_get_tag_usage(Allocator* allocator, MemoryTag tag);

// Leak detection
void allocator_check_leaks(Allocator* allocator);
void allocator_dump_allocations(Allocator* allocator);
void allocator_enable_advanced_leak_detection(Allocator* allocator, bool enable);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// SIMD-optimized memory operations
void* simd_memcpy(void* dst, const void* src, size_t size);
void* simd_memset(void* dst, u8 value, size_t size);
int simd_memcmp(const void* ptr1, const void* ptr2, size_t size);
bool simd_is_supported(void);

// Memory alignment utilities
size_t align_up(size_t size, size_t alignment);
size_t align_down(size_t size, size_t alignment);
bool is_aligned(size_t size, size_t alignment);
void* align_pointer(void* ptr, size_t alignment);

// ============================================================================
// GLOBAL ALLOCATOR MANAGEMENT
// ============================================================================

// Global allocator instances (can be set by the application)
extern Allocator* g_default_allocator;
extern Allocator* g_temp_allocator;
extern Allocator* g_persistent_allocator;
extern Allocator* g_asset_allocator;

// Initialize global allocator system
bool allocator_system_init(void);
void allocator_system_shutdown(void);

// Set global allocators
void allocator_set_default(Allocator* allocator);
void allocator_set_temp(Allocator* allocator);
void allocator_set_persistent(Allocator* allocator);
void allocator_set_asset(Allocator* allocator);

// Global allocation convenience macros (use global allocators)
#define MALLOC(size) allocator_alloc(g_default_allocator, size)
#define MALLOC_TEMP(size) allocator_alloc(g_temp_allocator, size)
#define MALLOC_PERSISTENT(size) allocator_alloc(g_persistent_allocator, size)
#define MALLOC_ASSET(size) allocator_alloc(g_asset_allocator, size)
#define FREE(ptr) allocator_free(g_default_allocator, ptr)
#define REALLOC(ptr, size) allocator_realloc(g_default_allocator, ptr, size)

// ============================================================================
// DESTRUCTION
// ============================================================================

// Destroy allocator and free all resources
void allocator_destroy(Allocator* allocator);

// ============================================================================
// THREAD-SAFE ALLOCATORS
// ============================================================================

// Thread-local allocator for per-thread temporary allocations
Allocator* allocator_get_thread_local(void);
void allocator_set_thread_local(Allocator* allocator);

// Thread-safe wrapper for any allocator
Allocator* allocator_make_thread_safe(Allocator* wrapped);

// ============================================================================
// DEBUGGING AND VALIDATION
// ============================================================================

// Validate allocator integrity
bool allocator_validate(Allocator* allocator);

// Fill memory with patterns for debugging
void allocator_debug_fill(Allocator* allocator, u8 pattern);
void allocator_debug_check_pattern(Allocator* allocator, u8 pattern);

// Memory boundary checking
typedef enum {
    BOUNDARY_CHECK_NONE = 0,
    BOUNDARY_CHECK_CANARY,
    BOUNDARY_CHECK_GUARD_PAGES
} BoundaryCheckMode;

void allocator_set_boundary_check(Allocator* allocator, BoundaryCheckMode mode);
bool allocator_check_boundaries(Allocator* allocator);
