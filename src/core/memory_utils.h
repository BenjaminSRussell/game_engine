// Memory Utilities
// Centralized memory allocation utilities to reduce code duplication

#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include "engine/include/core/types.h"
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory allocation macros with built-in error checking
#define MALLOC_ZERO(type) memory_utils_malloc_zeroed(sizeof(type), #type)
#define MALLOC_ZERO_ARRAY(type, count) memory_utils_malloc_zeroed(sizeof(type) * (count), #type "[]")
#define CALLOC_STRUCT(type) ((type*)memory_utils_calloc(1, sizeof(type), #type))
#define CALLOC_ARRAY(type, count) ((type*)memory_utils_calloc((count), sizeof(type), #type "[]"))

// Memory allocation statistics
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    u32 allocation_count;
    u32 free_count;
    u32 peak_usage;
    bool tracking_enabled;
} MemoryStats;

// Initialize memory utilities
void memory_utils_init(void);

// Shutdown memory utilities
void memory_utils_shutdown(void);

// Safe malloc with zero initialization
void *memory_utils_malloc_zeroed(size_t size, const char *type_name);

// Safe calloc with type tracking
void *memory_utils_calloc(size_t count, size_t size, const char *type_name);

// Safe realloc with tracking
void *memory_utils_realloc(void *ptr, size_t new_size, const char *type_name);

// Safe free with tracking
void memory_utils_free(void *ptr, const char *type_name);

// Duplicate string with tracking
char *memory_utils_strdup(const char *str, const char *context);

// Memory copy with bounds checking
bool memory_utils_safe_copy(void *dest, size_t dest_size, const void *src, size_t src_size);

// Memory set with bounds checking
bool memory_utils_safe_set(void *dest, size_t dest_size, int value, size_t count);

// Memory comparison with bounds checking
int memory_utils_safe_compare(const void *mem1, size_t size1, const void *mem2, size_t size2);

// Get memory statistics
MemoryStats memory_utils_get_stats(void);

// Reset memory statistics
void memory_utils_reset_stats(void);

// Enable/disable memory tracking
void memory_utils_set_tracking(bool enabled);

// Check for memory leaks
bool memory_utils_check_leaks(void);

// Print memory statistics
void memory_utils_print_stats(void);

// Memory pool for frequent allocations
typedef struct MemoryPool MemoryPool;

// Create memory pool
MemoryPool *memory_pool_create(size_t block_size, u32 block_count);

// Destroy memory pool
void memory_pool_destroy(MemoryPool *pool);

// Allocate from memory pool
void *memory_pool_alloc(MemoryPool *pool);

// Allocate zeroed from memory pool
void *memory_pool_alloc_zeroed(MemoryPool *pool);

// Reset memory pool (free all allocations)
void memory_pool_reset(MemoryPool *pool);

// Get memory pool statistics
void memory_pool_get_stats(MemoryPool *pool, u32 *used_blocks, u32 *free_blocks, u32 *total_blocks);

// Stack allocator for temporary allocations
typedef struct StackAllocator StackAllocator;

// Create stack allocator
StackAllocator *stack_allocator_create(size_t capacity);

// Destroy stack allocator
void stack_allocator_destroy(StackAllocator *allocator);

// Allocate from stack allocator
void *stack_allocator_alloc(StackAllocator *allocator, size_t size);

// Allocate aligned from stack allocator
void *stack_allocator_alloc_aligned(StackAllocator *allocator, size_t size, size_t alignment);

// Reset stack allocator (free all allocations)
void stack_allocator_reset(StackAllocator *allocator);

// Get current marker for rollback
size_t stack_allocator_get_marker(StackAllocator *allocator);

// Rollback to marker
void stack_allocator_rollback(StackAllocator *allocator, size_t marker);

// Get stack allocator statistics
void stack_allocator_get_stats(StackAllocator *allocator, size_t *used, size_t *capacity);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_UTILS_H
