// src/engine/core/memory/consolidated_memory_allocator.c
//
// Purpose: Consolidated memory allocator implementation that merges all existing
// allocator systems into a single, unified, high-performance memory management system

#include "core/memory/unified_memory_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

// ============================================================================
// GLOBAL STATE AND CONFIGURATION
// ============================================================================

// Global memory manager instance
static struct {
    bool initialized;
    MemoryPolicy policy;
    
    // Consolidated allocators
    Allocator* default_allocator;
    Allocator* temp_allocator;
    Allocator* persistent_allocator;
    Allocator* asset_allocator;
    Allocator* gpu_allocator;
    
    // Thread-local storage
    pthread_key_t thread_local_key;
    
    // Global statistics
    MemoryStats global_stats;
    pthread_mutex_t global_mutex;
    
    // Allocation tracking
    AllocationMetadata* allocation_table;
    size_t allocation_table_size;
    size_t allocation_count;
    pthread_mutex_t tracking_mutex;
    
    // Memory pools for different strategies
    struct {
        Allocator* linear_pools[16]; // Different sizes
        Allocator* stack_pools[8];   // Different stack sizes
        Allocator* pool_pools[32];   // Different block sizes
        Allocator* buddy_pools[4];   // Different total sizes
    } strategy_pools;
    
    // Profiling and debugging
    bool profiling_enabled;
    u64 profile_start_time;
    size_t total_allocations;
    size_t total_frees;
    size_t peak_memory_usage;
    
} g_memory_manager = {0};

// Memory configuration constants
#define MEMORY_ALIGNMENT 16
#define GUARD_PAGE_SIZE 4096
#define CANARY_VALUE 0xDEADBEEFCAFEBABEULL
#define MAX_ALLOCATION_TABLE_SIZE 65536
#define MEMORY_POOL_SIZES 16
#define STACK_POOL_SIZES 8
#define BLOCK_POOL_SIZES 32
#define BUDDY_POOL_SIZES 4

// Default pool sizes (bytes)
static const size_t g_linear_pool_sizes[MEMORY_POOL_SIZES] = {
    1024,        // 1KB
    4096,        // 4KB
    16384,       // 16KB
    65536,       // 64KB
    262144,      // 256KB
    1048576,     // 1MB
    4194304,     // 4MB
    16777216,    // 16MB
    67108864,    // 64MB
    268435456,   // 256MB
    536870912,   // 512MB
    1073741824,  // 1GB
    2147483648,  // 2GB
    4294967296,  // 4GB
    8589934592   // 8GB
};

static const size_t g_stack_pool_sizes[STACK_POOL_SIZES] = {
    4096,        // 4KB
    16384,       // 16KB
    65536,       // 64KB
    262144,      // 256KB
    1048576,     // 1MB
    4194304,     // 4MB
    16777216,    // 16MB
    67108864     // 64MB
};

static const size_t g_block_pool_sizes[BLOCK_POOL_SIZES] = {
    16, 32, 48, 64, 80, 96, 112, 128,
    144, 160, 176, 192, 208, 224, 240, 256,
    512, 768, 1024, 1536, 2048, 3072, 4096,
    6144, 8192, 12288, 16384, 24576, 32768, 49152
};

static const size_t g_buddy_pool_sizes[BUDDY_POOL_SIZES] = {
    1048576,      // 1MB
    16777216,     // 16MB
    268435456,    // 256MB
    1073741824    // 1GB
};

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

static size_t hash_pointer(void* ptr) {
    return ((uintptr_t)ptr >> 3) % MAX_ALLOCATION_TABLE_SIZE;
}

static void add_allocation_to_table(AllocationMetadata* metadata) {
    size_t index = hash_pointer(metadata->ptr);
    metadata->next = g_memory_manager.allocation_table[index];
    g_memory_manager.allocation_table[index] = metadata;
    g_memory_manager.allocation_count++;
}

static void remove_allocation_from_table(AllocationMetadata* metadata) {
    size_t index = hash_pointer(metadata->ptr);
    AllocationMetadata** current = &g_memory_manager.allocation_table[index];
    
    while (*current) {
        if (*current == metadata) {
            *current = metadata->next;
            g_memory_manager.allocation_count--;
            break;
        }
        current = &(*current)->next;
    }
}

static AllocationMetadata* find_allocation_in_table(void* ptr) {
    size_t index = hash_pointer(ptr);
    AllocationMetadata* current = g_memory_manager.allocation_table[index];
    
    while (current) {
        if (current->ptr == ptr) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void* allocate_with_guard_pages(size_t size) {
    size_t total_size = size + 2 * GUARD_PAGE_SIZE;
    void* base = mmap(NULL, total_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (base == MAP_FAILED) {
        return NULL;
    }
    
    // Protect bottom guard page
    mprotect(base, GUARD_PAGE_SIZE, PROT_NONE);
    
    // Protect top guard page
    void* protected_memory = (u8*)base + GUARD_PAGE_SIZE;
    mprotect((u8*)protected_memory + size, GUARD_PAGE_SIZE, PROT_NONE);
    
    // Make the actual memory accessible
    mprotect(protected_memory, size, PROT_READ | PROT_WRITE);
    
    return protected_memory;
}

static void free_with_guard_pages(void* ptr, size_t size) {
    void* base = (u8*)ptr - GUARD_PAGE_SIZE;
    size_t total_size = size + 2 * GUARD_PAGE_SIZE;
    munmap(base, total_size);
}

static void setup_canary(void* ptr, size_t size) {
    u64* canary_start = (u64*)((u8*)ptr - sizeof(u64));
    u64* canary_end = (u64*)((u8*)ptr + size);
    
    *canary_start = CANARY_VALUE;
    *canary_end = CANARY_VALUE;
}

static bool check_canary(void* ptr, size_t size) {
    u64* canary_start = (u64*)((u8*)ptr - sizeof(u64));
    u64* canary_end = (u64*)((u8*)ptr + size);
    
    return *canary_start == CANARY_VALUE && *canary_end == CANARY_VALUE;
}

static void capture_stack_trace(AllocationMetadata* metadata) {
    metadata->stack_frames = backtrace(metadata->stack_trace, 16);
}

static void update_global_stats(size_t size, bool allocated) {
    pthread_mutex_lock(&g_memory_manager.global_mutex);
    
    if (allocated) {
        g_memory_manager.global_stats.total_allocated += size;
        g_memory_manager.global_stats.current_usage += size;
        g_memory_manager.global_stats.allocation_count++;
        g_memory_manager.total_allocations++;
        
        if (g_memory_manager.global_stats.current_usage > g_memory_manager.peak_memory_usage) {
            g_memory_manager.peak_memory_usage = g_memory_manager.global_stats.current_usage;
        }
    } else {
        g_memory_manager.global_stats.total_freed += size;
        g_memory_manager.global_stats.current_usage -= size;
        g_memory_manager.global_stats.free_count++;
        g_memory_manager.total_frees++;
    }
    
    pthread_mutex_unlock(&g_memory_manager.global_mutex);
}

static Allocator* select_best_allocator(size_t size, MemoryStrategy strategy, MemoryTag tag) {
    // Strategy-based selection
    switch (strategy) {
        case MEMORY_STRATEGY_LINEAR:
            // Select best linear pool based on size
            for (int i = 0; i < MEMORY_POOL_SIZES; i++) {
                if (size <= g_linear_pool_sizes[i] && g_memory_manager.strategy_pools.linear_pools[i]) {
                    return g_memory_manager.strategy_pools.linear_pools[i];
                }
            }
            break;
            
        case MEMORY_STRATEGY_STACK:
            // Select best stack pool based on size
            for (int i = 0; i < STACK_POOL_SIZES; i++) {
                if (size <= g_stack_pool_sizes[i] && g_memory_manager.strategy_pools.stack_pools[i]) {
                    return g_memory_manager.strategy_pools.stack_pools[i];
                }
            }
            break;
            
        case MEMORY_STRATEGY_POOL:
            // Select best block pool based on size
            for (int i = 0; i < BLOCK_POOL_SIZES; i++) {
                if (size <= g_block_pool_sizes[i] && g_memory_manager.strategy_pools.pool_pools[i]) {
                    return g_memory_manager.strategy_pools.pool_pools[i];
                }
            }
            break;
            
        case MEMORY_STRATEGY_BUDDY:
            // Select best buddy pool based on size
            for (int i = 0; i < BUDDY_POOL_SIZES; i++) {
                if (size <= g_buddy_pool_sizes[i] && g_memory_manager.strategy_pools.buddy_pools[i]) {
                    return g_memory_manager.strategy_pools.buddy_pools[i];
                }
            }
            break;
            
        case MEMORY_STRATEGY_GPU:
            return g_memory_manager.gpu_allocator;
            
        default:
            return g_memory_manager.default_allocator;
    }
    
    return g_memory_manager.default_allocator;
}

// ============================================================================
// CONSOLIDATED ALLOCATOR IMPLEMENTATIONS
// ============================================================================

// Linear allocator implementation
static void* linear_alloc(Allocator* allocator, size_t size, size_t alignment) {
    LinearAllocator* linear = (LinearAllocator*)allocator->impl;
    
    // Align the offset
    size_t aligned_offset = memory_align_up(linear->offset, alignment);
    size_t total_size = aligned_offset + size;
    
    if (total_size > linear->size) {
        // Try to allocate a new buffer
        if (linear->current_buffer + 1 < linear->buffer_count) {
            linear->current_buffer++;
            linear->offset = 0;
            aligned_offset = 0;
            total_size = size;
        } else {
            return NULL; // Out of memory
        }
    }
    
    void* ptr = (u8*)linear->buffers[linear->current_buffer] + aligned_offset;
    linear->offset = total_size;
    
    if (linear->offset > linear->peak_usage) {
        linear->peak_usage = linear->offset;
    }
    
    return ptr;
}

static void linear_reset(Allocator* allocator) {
    LinearAllocator* linear = (LinearAllocator*)allocator->impl;
    linear->offset = 0;
    linear->current_buffer = 0;
}

// Stack allocator implementation
static void* stack_alloc(Allocator* allocator, size_t size, size_t alignment) {
    StackAllocator* stack = (StackAllocator*)allocator->impl;
    
    // Align the offset
    size_t aligned_offset = memory_align_up(stack->offset, alignment);
    size_t total_size = aligned_offset + size;
    
    if (total_size > stack->size) {
        return NULL; // Out of memory
    }
    
    void* ptr = (u8*)stack->base + aligned_offset;
    stack->offset = total_size;
    
    if (stack->offset > stack->peak_usage) {
        stack->peak_usage = stack->offset;
    }
    
    return ptr;
}

static void stack_free(Allocator* allocator, void* ptr) {
    StackAllocator* stack = (StackAllocator*)allocator->impl;
    
    // Find the marker for this allocation
    for (int i = stack->stack_top - 1; i >= 0; i--) {
        if (stack->stack[i] <= ptr) {
            stack->offset = (uintptr_t)stack->stack[i] - (uintptr_t)stack->base;
            stack->stack_top = i;
            break;
        }
    }
}

// Pool allocator implementation
static void* pool_alloc(Allocator* allocator, size_t size, size_t alignment) {
    PoolAllocator* pool = (PoolAllocator*)allocator->impl;
    
    if (size > pool->block_size) {
        return NULL; // Too large for this pool
    }
    
    if (pool->free_count == 0) {
        return NULL; // Pool exhausted
    }
    
    void* block = pool->free_list[pool->free_count - 1];
    pool->free_list[pool->free_count - 1] = NULL;
    pool->free_count--;
    pool->allocated_count++;
    
    if (pool->allocated_count > pool->peak_allocated) {
        pool->peak_allocated = pool->allocated_count;
    }
    
    return block;
}

static void pool_free(Allocator* allocator, void* ptr) {
    PoolAllocator* pool = (PoolAllocator*)allocator->impl;
    
    if (pool->free_count >= pool->block_count) {
        return; // Pool full (shouldn't happen)
    }
    
    pool->free_list[pool->free_count] = ptr;
    pool->free_count++;
    pool->allocated_count--;
}

// ============================================================================
// UNIFIED MEMORY MANAGER API
// ============================================================================

bool unified_memory_init(const MemoryPolicy* policy) {
    if (g_memory_manager.initialized) {
        return true; // Already initialized
    }
    
    // Initialize mutexes
    if (pthread_mutex_init(&g_memory_manager.global_mutex, NULL) != 0) return false;
    if (pthread_mutex_init(&g_memory_manager.tracking_mutex, NULL) != 0) return false;
    
    // Set policy
    if (policy) {
        g_memory_manager.policy = *policy;
    } else {
        // Default policy
        g_memory_manager.policy = (MemoryPolicy){
            .default_arena_size = 64 * 1024 * 1024, // 64MB
            .max_total_memory = SIZE_MAX,
            .enable_leak_detection = true,
            .enable_guard_pages = false,
            .enable_thread_local = true,
            .enable_profiling = false,
            .alignment = MEMORY_ALIGNMENT
        };
    }
    
    // Initialize allocation table
    g_memory_manager.allocation_table_size = MAX_ALLOCATION_TABLE_SIZE;
    g_memory_manager.allocation_table = calloc(MAX_ALLOCATION_TABLE_SIZE, sizeof(AllocationMetadata*));
    if (!g_memory_manager.allocation_table) {
        return false;
    }
    
    // Create thread-local storage key
    if (pthread_key_create(&g_memory_manager.thread_local_key, NULL) != 0) {
        free(g_memory_manager.allocation_table);
        return false;
    }
    
    // Initialize strategy pools
    for (int i = 0; i < MEMORY_POOL_SIZES; i++) {
        char name[64];
        snprintf(name, sizeof(name), "linear_pool_%d", i);
        g_memory_manager.strategy_pools.linear_pools[i] = 
            linear_allocator_create(g_linear_pool_sizes[i], name);
    }
    
    for (int i = 0; i < STACK_POOL_SIZES; i++) {
        char name[64];
        snprintf(name, sizeof(name), "stack_pool_%d", i);
        g_memory_manager.strategy_pools.stack_pools[i] = 
            stack_allocator_create(g_stack_pool_sizes[i], name);
    }
    
    for (int i = 0; i < BLOCK_POOL_SIZES; i++) {
        char name[64];
        snprintf(name, sizeof(name), "pool_%d", i);
        g_memory_manager.strategy_pools.pool_pools[i] = 
            pool_allocator_create(g_block_pool_sizes[i], 1024, name);
    }
    
    for (int i = 0; i < BUDDY_POOL_SIZES; i++) {
        char name[64];
        snprintf(name, sizeof(name), "buddy_pool_%d", i);
        g_memory_manager.strategy_pools.buddy_pools[i] = 
            buddy_allocator_create(g_buddy_pool_sizes[i], 4096, name);
    }
    
    // Create main allocators
    g_memory_manager.default_allocator = tracked_allocator_create(NULL, "default");
    g_memory_manager.temp_allocator = arena_allocator_create(
        g_memory_manager.policy.default_arena_size, "temp");
    g_memory_manager.persistent_allocator = arena_allocator_create(
        g_memory_manager.policy.default_arena_size * 2, "persistent");
    g_memory_manager.asset_allocator = pool_allocator_create(1024, 10000, "asset");
    g_memory_manager.gpu_allocator = gpu_allocator_create(
        256 * 1024 * 1024, 64 * 1024 * 1024, 0, "gpu");
    
    // Initialize statistics
    memset(&g_memory_manager.global_stats, 0, sizeof(MemoryStats));
    g_memory_manager.profiling_enabled = g_memory_manager.policy.enable_profiling;
    g_memory_manager.profile_start_time = time(NULL);
    
    g_memory_manager.initialized = true;
    return true;
}

void unified_memory_shutdown(void) {
    if (!g_memory_manager.initialized) {
        return;
    }
    
    // Check for leaks if enabled
    if (g_memory_manager.policy.enable_leak_detection) {
        unified_memory_dump_leaks();
    }
    
    // Destroy all strategy pools
    for (int i = 0; i < MEMORY_POOL_SIZES; i++) {
        if (g_memory_manager.strategy_pools.linear_pools[i]) {
            g_memory_manager.strategy_pools.linear_pools[i]->vtable->destroy(
                g_memory_manager.strategy_pools.linear_pools[i]);
        }
    }
    
    for (int i = 0; i < STACK_POOL_SIZES; i++) {
        if (g_memory_manager.strategy_pools.stack_pools[i]) {
            g_memory_manager.strategy_pools.stack_pools[i]->vtable->destroy(
                g_memory_manager.strategy_pools.stack_pools[i]);
        }
    }
    
    for (int i = 0; i < BLOCK_POOL_SIZES; i++) {
        if (g_memory_manager.strategy_pools.pool_pools[i]) {
            g_memory_manager.strategy_pools.pool_pools[i]->vtable->destroy(
                g_memory_manager.strategy_pools.pool_pools[i]);
        }
    }
    
    for (int i = 0; i < BUDDY_POOL_SIZES; i++) {
        if (g_memory_manager.strategy_pools.buddy_pools[i]) {
            g_memory_manager.strategy_pools.buddy_pools[i]->vtable->destroy(
                g_memory_manager.strategy_pools.buddy_pools[i]);
        }
    }
    
    // Destroy main allocators
    if (g_memory_manager.default_allocator) {
        g_memory_manager.default_allocator->vtable->destroy(g_memory_manager.default_allocator);
    }
    if (g_memory_manager.temp_allocator) {
        g_memory_manager.temp_allocator->vtable->destroy(g_memory_manager.temp_allocator);
    }
    if (g_memory_manager.persistent_allocator) {
        g_memory_manager.persistent_allocator->vtable->destroy(g_memory_manager.persistent_allocator);
    }
    if (g_memory_manager.asset_allocator) {
        g_memory_manager.asset_allocator->vtable->destroy(g_memory_manager.asset_allocator);
    }
    if (g_memory_manager.gpu_allocator) {
        g_memory_manager.gpu_allocator->vtable->destroy(g_memory_manager.gpu_allocator);
    }
    
    // Clean up allocation table
    free(g_memory_manager.allocation_table);
    
    // Destroy thread-local storage
    pthread_key_delete(g_memory_manager.thread_local_key);
    
    // Destroy mutexes
    pthread_mutex_destroy(&g_memory_manager.global_mutex);
    pthread_mutex_destroy(&g_memory_manager.tracking_mutex);
    
    g_memory_manager.initialized = false;
}

void* unified_alloc(size_t size) {
    return unified_alloc_tagged(size, MEMORY_TAG_DEFAULT);
}

void* unified_alloc_aligned(size_t size, size_t alignment) {
    return unified_alloc_tagged_aligned(size, alignment, MEMORY_TAG_DEFAULT);
}

void* unified_alloc_tagged(size_t size, MemoryTag tag) {
    return unified_alloc_tagged_aligned(size, MEMORY_ALIGNMENT, tag);
}

void* unified_alloc_tagged_aligned(size_t size, size_t alignment, MemoryTag tag) {
    if (!g_memory_manager.initialized) {
        return NULL;
    }
    
    // Select appropriate allocator
    Allocator* allocator = select_best_allocator(size, MEMORY_STRATEGY_DEFAULT, tag);
    
    // Perform allocation
    void* ptr = allocator->vtable->alloc(allocator, size, alignment);
    if (!ptr) {
        return NULL;
    }
    
    // Track allocation if debugging is enabled
    if (g_memory_manager.policy.enable_leak_detection) {
        pthread_mutex_lock(&g_memory_manager.tracking_mutex);
        
        AllocationMetadata* metadata = malloc(sizeof(AllocationMetadata));
        if (metadata) {
            metadata->ptr = ptr;
            metadata->size = size;
            metadata->alignment = alignment;
            metadata->tag = tag;
            metadata->timestamp = time(NULL);
            metadata->file = NULL;
            metadata->line = 0;
            metadata->function = NULL;
            
            if (g_memory_manager.policy.enable_guard_pages) {
                // Reallocate with guard pages
                void* guarded_ptr = allocate_with_guard_pages(size);
                if (guarded_ptr) {
                    memcpy(guarded_ptr, ptr, size);
                    allocator->vtable->free(allocator, ptr);
                    ptr = guarded_ptr;
                    metadata->ptr = ptr;
                }
            }
            
            capture_stack_trace(metadata);
            add_allocation_to_table(metadata);
        }
        
        pthread_mutex_unlock(&g_memory_manager.tracking_mutex);
    }
    
    // Update statistics
    update_global_stats(size, true);
    
    return ptr;
}

void unified_free(void* ptr) {
    if (!ptr || !g_memory_manager.initialized) {
        return;
    }
    
    // Find allocation metadata
    AllocationMetadata* metadata = NULL;
    if (g_memory_manager.policy.enable_leak_detection) {
        pthread_mutex_lock(&g_memory_manager.tracking_mutex);
        metadata = find_allocation_in_table(ptr);
        if (metadata) {
            remove_allocation_from_table(metadata);
        }
        pthread_mutex_unlock(&g_memory_manager.tracking_mutex);
    }
    
    size_t size = metadata ? metadata->size : 0;
    
    // Check canaries if enabled
    if (metadata && g_memory_manager.policy.enable_guard_pages) {
        if (!check_canary(ptr, size)) {
            fprintf(stderr, "Memory corruption detected for pointer %p\\n", ptr);
            abort();
        }
        free_with_guard_pages(ptr, size);
    } else {
        // Find the allocator that owns this pointer
        // This is a simplified approach - in practice, we'd need better tracking
        if (g_memory_manager.default_allocator) {
            g_memory_manager.default_allocator->vtable->free(g_memory_manager.default_allocator, ptr);
        }
    }
    
    // Clean up metadata
    if (metadata) {
        free(metadata);
    }
    
    // Update statistics
    update_global_stats(size, false);
}

void unified_memory_get_stats(MemoryStats* stats) {
    if (!stats || !g_memory_manager.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_memory_manager.global_mutex);
    *stats = g_memory_manager.global_stats;
    
    // Add strategy pool statistics
    for (int i = 0; i < MEMORY_POOL_SIZES; i++) {
        if (g_memory_manager.strategy_pools.linear_pools[i]) {
            MemoryStats pool_stats;
            g_memory_manager.strategy_pools.linear_pools[i]->vtable->get_stats(
                g_memory_manager.strategy_pools.linear_pools[i], &pool_stats);
            stats->total_allocated += pool_stats.total_allocated;
            stats->current_usage += pool_stats.current_usage;
        }
    }
    
    pthread_mutex_unlock(&g_memory_manager.global_mutex);
}

void unified_memory_dump_leaks(void) {
    if (!g_memory_manager.initialized || !g_memory_manager.policy.enable_leak_detection) {
        return;
    }
    
    pthread_mutex_lock(&g_memory_manager.tracking_mutex);
    
    printf("\\n=== MEMORY LEAK REPORT ===\\n");
    printf("Total allocations: %zu\\n", g_memory_manager.total_allocations);
    printf("Total frees: %zu\\n", g_memory_manager.total_frees);
    printf("Leaked allocations: %zu\\n", g_memory_manager.total_allocations - g_memory_manager.total_frees);
    printf("Peak memory usage: %zu bytes\\n", g_memory_manager.peak_memory_usage);
    
    // Report individual leaks
    for (size_t i = 0; i < g_memory_manager.allocation_table_size; i++) {
        AllocationMetadata* metadata = g_memory_manager.allocation_table[i];
        while (metadata) {
            printf("LEAK: %p (%zu bytes) at %s:%d in %s()\\n",
                   metadata->ptr, metadata->size,
                   metadata->file ? metadata->file : "unknown",
                   metadata->line,
                   metadata->function ? metadata->function : "unknown");
            
            if (metadata->stack_frames > 0) {
                printf("Stack trace:\\n");
                char** strings = backtrace_symbols(metadata->stack_trace, metadata->stack_frames);
                for (int j = 0; j < metadata->stack_frames; j++) {
                    printf("  %s\\n", strings[j]);
                }
                free(strings);
            }
            
            metadata = metadata->next;
        }
    }
    
    printf("========================\\n\\n");
    
    pthread_mutex_unlock(&g_memory_manager.tracking_mutex);
}

// ============================================================================
// CONVENIENCE MACROS IMPLEMENTATION
// ============================================================================

Allocator* unified_memory_get_default(void) {
    return g_memory_manager.initialized ? g_memory_manager.default_allocator : NULL;
}

Allocator* unified_memory_get_temp(void) {
    return g_memory_manager.initialized ? g_memory_manager.temp_allocator : NULL;
}

Allocator* unified_memory_get_persistent(void) {
    return g_memory_manager.initialized ? g_memory_manager.persistent_allocator : NULL;
}

Allocator* unified_memory_get_asset(void) {
    return g_memory_manager.initialized ? g_memory_manager.asset_allocator : NULL;
}

Allocator* unified_memory_get_gpu(void) {
    return g_memory_manager.initialized ? g_memory_manager.gpu_allocator : NULL;
}

Allocator* unified_memory_get_thread_local(void) {
    if (!g_memory_manager.initialized) {
        return NULL;
    }
    
    Allocator* allocator = pthread_getspecific(g_memory_manager.thread_local_key);
    if (!allocator) {
        // Create thread-local allocator
        allocator = arena_allocator_create(g_memory_manager.policy.default_arena_size / 4, "thread_local");
        pthread_setspecific(g_memory_manager.thread_local_key, allocator);
    }
    
    return allocator;
}
