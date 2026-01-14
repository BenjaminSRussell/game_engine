// Memory Utilities Implementation
// Centralized memory allocation utilities to reduce code duplication

#include "memory_utils.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Global memory statistics
static MemoryStats g_memory_stats = {0};
static bool g_initialized = false;

// Memory pool implementation
struct MemoryPool {
    void *memory;
    size_t block_size;
    u32 block_count;
    u32 *free_list;
    u32 free_count;
    u32 allocated_count;
    bool *used_blocks;
};

// Stack allocator implementation
struct StackAllocator {
    void *memory;
    size_t capacity;
    size_t used;
    size_t top;
};

// Initialize memory utilities
void memory_utils_init(void) {
    if (g_initialized) return;
    
    memset(&g_memory_stats, 0, sizeof(MemoryStats));
    g_memory_stats.tracking_enabled = true;
    g_initialized = true;
    
    LOG_INFO("Memory utilities initialized");
}

// Shutdown memory utilities
void memory_utils_shutdown(void) {
    if (!g_initialized) return;
    
    if (g_memory_stats.tracking_enabled) {
        memory_utils_print_stats();
        
        if (g_memory_stats.allocation_count != g_memory_stats.free_count) {
            LOG_WARN("Memory leak detected: %d allocations not freed", 
                     g_memory_stats.allocation_count - g_memory_stats.free_count);
        }
    }
    
    g_initialized = false;
    LOG_INFO("Memory utilities shutdown");
}

// Safe malloc with zero initialization
void *memory_utils_malloc_zeroed(size_t size, const char *type_name) {
    void *ptr = malloc(size);
    if (!ptr) {
        LOG_ERROR("Failed to allocate %zu bytes for %s", size, type_name);
        return NULL;
    }
    
    memset(ptr, 0, size);
    
    if (g_memory_stats.tracking_enabled) {
        g_memory_stats.total_allocated += size;
        g_memory_stats.allocation_count++;
        
        if (g_memory_stats.total_allocated - g_memory_stats.total_freed > g_memory_stats.peak_usage) {
            g_memory_stats.peak_usage = g_memory_stats.total_allocated - g_memory_stats.total_freed;
        }
    }
    
    LOG_DEBUG("Allocated %zu bytes for %s (total: %zu)", size, type_name, g_memory_stats.total_allocated);
    return ptr;
}

// Safe calloc with type tracking
void *memory_utils_calloc(size_t count, size_t size, const char *type_name) {
    void *ptr = calloc(count, size);
    if (!ptr) {
        LOG_ERROR("Failed to allocate %zu x %zu bytes for %s", count, size, type_name);
        return NULL;
    }
    
    if (g_memory_stats.tracking_enabled) {
        size_t total_size = count * size;
        g_memory_stats.total_allocated += total_size;
        g_memory_stats.allocation_count++;
        
        if (g_memory_stats.total_allocated - g_memory_stats.total_freed > g_memory_stats.peak_usage) {
            g_memory_stats.peak_usage = g_memory_stats.total_allocated - g_memory_stats.total_freed;
        }
    }
    
    LOG_DEBUG("Allocated %zu x %zu bytes for %s (total: %zu)", count, size, type_name, g_memory_stats.total_allocated);
    return ptr;
}

// Safe realloc with tracking
void *memory_utils_realloc(void *ptr, size_t new_size, const char *type_name) {
    if (!ptr) {
        return memory_utils_malloc_zeroed(new_size, type_name);
    }
    
    void *new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        LOG_ERROR("Failed to reallocate to %zu bytes for %s", new_size, type_name);
        return NULL;
    }
    
    LOG_DEBUG("Reallocated to %zu bytes for %s", new_size, type_name);
    return new_ptr;
}

// Safe free with tracking
void memory_utils_free(void *ptr, const char *type_name) {
    if (!ptr) return;
    
    free(ptr);
    
    if (g_memory_stats.tracking_enabled) {
        g_memory_stats.free_count++;
        // Note: We don't track exact size freed since we don't store it per allocation
        // This is a limitation of the simple tracking approach
    }
    
    LOG_DEBUG("Freed memory for %s", type_name);
}

// Duplicate string with tracking
char *memory_utils_strdup(const char *str, const char *context) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char *dup = (char*)memory_utils_malloc_zeroed(len, context);
    if (dup) {
        strcpy(dup, str);
    }
    
    return dup;
}

// Memory copy with bounds checking
bool memory_utils_safe_copy(void *dest, size_t dest_size, const void *src, size_t src_size) {
    if (!dest || !src) return false;
    
    size_t copy_size = (dest_size < src_size) ? dest_size : src_size;
    memcpy(dest, src, copy_size);
    
    return copy_size == src_size;
}

// Memory set with bounds checking
bool memory_utils_safe_set(void *dest, size_t dest_size, int value, size_t count) {
    if (!dest) return false;
    
    size_t set_size = (dest_size < count) ? dest_size : count;
    memset(dest, value, set_size);
    
    return set_size == count;
}

// Memory comparison with bounds checking
int memory_utils_safe_compare(const void *mem1, size_t size1, const void *mem2, size_t size2) {
    if (!mem1 || !mem2) return -1;
    
    size_t compare_size = (size1 < size2) ? size1 : size2;
    return memcmp(mem1, mem2, compare_size);
}

// Get memory statistics
MemoryStats memory_utils_get_stats(void) {
    return g_memory_stats;
}

// Reset memory statistics
void memory_utils_reset_stats(void) {
    memset(&g_memory_stats, 0, sizeof(MemoryStats));
    g_memory_stats.tracking_enabled = true;
    
    LOG_INFO("Memory statistics reset");
}

// Enable/disable memory tracking
void memory_utils_set_tracking(bool enabled) {
    g_memory_stats.tracking_enabled = enabled;
    LOG_INFO("Memory tracking %s", enabled ? "enabled" : "disabled");
}

// Check for memory leaks
bool memory_utils_check_leaks(void) {
    if (!g_memory_stats.tracking_enabled) return false;
    
    return g_memory_stats.allocation_count != g_memory_stats.free_count;
}

// Print memory statistics
void memory_utils_print_stats(void) {
    if (!g_memory_stats.tracking_enabled) {
        LOG_INFO("Memory tracking is disabled");
        return;
    }
    
    LOG_INFO("=== Memory Statistics ===");
    LOG_INFO("Total allocated: %zu bytes", g_memory_stats.total_allocated);
    LOG_INFO("Total freed: %zu bytes", g_memory_stats.total_freed);
    LOG_INFO("Current usage: %zu bytes", g_memory_stats.total_allocated - g_memory_stats.total_freed);
    LOG_INFO("Peak usage: %zu bytes", g_memory_stats.peak_usage);
    LOG_INFO("Allocations: %u", g_memory_stats.allocation_count);
    LOG_INFO("Frees: %u", g_memory_stats.free_count);
    LOG_INFO("Leaked allocations: %u", g_memory_stats.allocation_count - g_memory_stats.free_count);
    LOG_INFO("========================");
}

// Create memory pool
MemoryPool *memory_pool_create(size_t block_size, u32 block_count) {
    MemoryPool *pool = (MemoryPool*)memory_utils_malloc_zeroed(sizeof(MemoryPool), "MemoryPool");
    if (!pool) return NULL;
    
    pool->block_size = block_size;
    pool->block_count = block_count;
    
    // Allocate memory for all blocks
    pool->memory = memory_utils_malloc_zeroed(block_size * block_count, "MemoryPool::memory");
    if (!pool->memory) {
        memory_utils_free(pool, "MemoryPool");
        return NULL;
    }
    
    // Allocate free list
    pool->free_list = (u32*)memory_utils_malloc_zeroed(sizeof(u32) * block_count, "MemoryPool::free_list");
    if (!pool->free_list) {
        memory_utils_free(pool->memory, "MemoryPool::memory");
        memory_utils_free(pool, "MemoryPool");
        return NULL;
    }
    
    // Allocate used blocks array
    pool->used_blocks = (bool*)memory_utils_malloc_zeroed(sizeof(bool) * block_count, "MemoryPool::used_blocks");
    if (!pool->used_blocks) {
        memory_utils_free(pool->free_list, "MemoryPool::free_list");
        memory_utils_free(pool->memory, "MemoryPool::memory");
        memory_utils_free(pool, "MemoryPool");
        return NULL;
    }
    
    // Initialize free list
    for (u32 i = 0; i < block_count; i++) {
        pool->free_list[i] = i;
    }
    
    pool->free_count = block_count;
    pool->allocated_count = 0;
    
    LOG_DEBUG("Memory pool created: %zu x %u blocks", block_size, block_count);
    return pool;
}

// Destroy memory pool
void memory_pool_destroy(MemoryPool *pool) {
    if (!pool) return;
    
    memory_utils_free(pool->used_blocks, "MemoryPool::used_blocks");
    memory_utils_free(pool->free_list, "MemoryPool::free_list");
    memory_utils_free(pool->memory, "MemoryPool::memory");
    memory_utils_free(pool, "MemoryPool");
    
    LOG_DEBUG("Memory pool destroyed");
}

// Allocate from memory pool
void *memory_pool_alloc(MemoryPool *pool) {
    if (!pool || pool->free_count == 0) return NULL;
    
    u32 block_index = pool->free_list[pool->free_count - 1];
    pool->free_count--;
    pool->allocated_count++;
    pool->used_blocks[block_index] = true;
    
    void *block = (void*)((char*)pool->memory + block_index * pool->block_size);
    return block;
}

// Allocate zeroed from memory pool
void *memory_pool_alloc_zeroed(MemoryPool *pool) {
    void *block = memory_pool_alloc(pool);
    if (block) {
        memset(block, 0, pool->block_size);
    }
    return block;
}

// Reset memory pool
void memory_pool_reset(MemoryPool *pool) {
    if (!pool) return;
    
    // Reset free list
    for (u32 i = 0; i < pool->block_count; i++) {
        pool->free_list[i] = i;
        pool->used_blocks[i] = false;
    }
    
    pool->free_count = pool->block_count;
    pool->allocated_count = 0;
    
    LOG_DEBUG("Memory pool reset");
}

// Get memory pool statistics
void memory_pool_get_stats(MemoryPool *pool, u32 *used_blocks, u32 *free_blocks, u32 *total_blocks) {
    if (!pool) return;
    
    if (used_blocks) *used_blocks = pool->allocated_count;
    if (free_blocks) *free_blocks = pool->free_count;
    if (total_blocks) *total_blocks = pool->block_count;
}

// Create stack allocator
StackAllocator *stack_allocator_create(size_t capacity) {
    StackAllocator *allocator = (StackAllocator*)memory_utils_malloc_zeroed(sizeof(StackAllocator), "StackAllocator");
    if (!allocator) return NULL;
    
    allocator->memory = memory_utils_malloc_zeroed(capacity, "StackAllocator::memory");
    if (!allocator->memory) {
        memory_utils_free(allocator, "StackAllocator");
        return NULL;
    }
    
    allocator->capacity = capacity;
    allocator->used = 0;
    allocator->top = 0;
    
    LOG_DEBUG("Stack allocator created: %zu bytes", capacity);
    return allocator;
}

// Destroy stack allocator
void stack_allocator_destroy(StackAllocator *allocator) {
    if (!allocator) return;
    
    memory_utils_free(allocator->memory, "StackAllocator::memory");
    memory_utils_free(allocator, "StackAllocator");
    
    LOG_DEBUG("Stack allocator destroyed");
}

// Allocate from stack allocator
void *stack_allocator_alloc(StackAllocator *allocator, size_t size) {
    if (!allocator || allocator->used + size > allocator->capacity) return NULL;
    
    void *ptr = (void*)((char*)allocator->memory + allocator->used);
    allocator->used += size;
    allocator->top = allocator->used;
    
    return ptr;
}

// Allocate aligned from stack allocator
void *stack_allocator_alloc_aligned(StackAllocator *allocator, size_t size, size_t alignment) {
    if (!allocator) return NULL;
    
    // Calculate aligned offset
    size_t current = allocator->used;
    size_t aligned = (current + alignment - 1) & ~(alignment - 1);
    
    if (aligned + size > allocator->capacity) return NULL;
    
    void *ptr = (void*)((char*)allocator->memory + aligned);
    allocator->used = aligned + size;
    allocator->top = allocator->used;
    
    return ptr;
}

// Reset stack allocator
void stack_allocator_reset(StackAllocator *allocator) {
    if (!allocator) return;
    
    allocator->used = 0;
    allocator->top = 0;
    
    LOG_DEBUG("Stack allocator reset");
}

// Get current marker for rollback
size_t stack_allocator_get_marker(StackAllocator *allocator) {
    return allocator ? allocator->top : 0;
}

// Rollback to marker
void stack_allocator_rollback(StackAllocator *allocator, size_t marker) {
    if (!allocator || marker > allocator->used) return;
    
    allocator->used = marker;
    allocator->top = marker;
}

// Get stack allocator statistics
void stack_allocator_get_stats(StackAllocator *allocator, size_t *used, size_t *capacity) {
    if (!allocator) return;
    
    if (used) *used = allocator->used;
    if (capacity) *capacity = allocator->capacity;
}
