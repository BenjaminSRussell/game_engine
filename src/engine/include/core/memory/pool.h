// include/memory/pool.h
//
// Purpose: Defines various specialized memory allocation strategies designed for
// performance-critical scenarios. This header provides an API for a `MemoryPool`
// (for fixed-size object allocation), an `Arena` allocator (for bulk, temporary
// allocations), and a `StackAllocator` (for frame-based memory management).
// These allocators aim to reduce overhead and fragmentation compared to general-purpose
// heap allocators.
//
// Public APIs:
// - `MemoryPool`: Structure for a fixed-size block allocator.
// - `memory_pool_init`, `memory_pool_free`: Lifecycle for a memory pool, expecting
//   pre-allocated memory.
// - `memory_pool_alloc`, `memory_pool_dealloc`: O(1) allocation and deallocation
//   of fixed-size blocks from the pool.
// - `memory_pool_contains`: Checks if a pointer belongs to the pool.
// - `Arena`: Structure for an arena allocator.
// - `arena_init`: Initializes an arena with a pre-allocated memory region.
// - `arena_alloc`: Allocates memory linearly from the arena.
// - `arena_reset`: Resets the arena's allocation pointer to the beginning (does not free memory).
// - `StackAllocator`: Structure for a stack-based allocator.
// - `stack_allocator_init`: Initializes a stack allocator with a pre-allocated memory region.
// - `stack_alloc`: Allocates memory from the stack.
// - `stack_free`: Frees memory from the top of the stack (must be last allocation).
// - `stack_reset`: Resets the stack allocator's pointer to the beginning.
//
// Ownership: These allocators operate on pre-allocated `memory` regions provided by the user.
// The allocators themselves manage the sub-allocations within these regions. Users are
// responsible for managing the lifetime of the main `memory` block passed during initialization.
//
// Invariants:
// - `block_size` for `MemoryPool` must be consistent for all allocations.
// - `Arena` and `StackAllocator` expect their `memory` buffer to be valid throughout their lifetime.
// - `alignment` parameters must be powers of two.
// - `stack_free` must be called on the most recent allocation to maintain stack integrity.
// - `arena_reset` and `stack_reset` do not release memory to the OS, only mark it as available for reuse.
//
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H


#include "engine/include/common.h"

// Memory pool allocator - zero-allocation hot paths
typedef struct {
    void *memory;
    u32 block_size;
    u32 block_count;
    u32 *free_list;        // Stack of free block indices
    u32 free_count;
    u32 free_capacity;
} MemoryPool;

// Initialize memory pool
void memory_pool_init(MemoryPool *pool, u32 block_size, u32 block_count);
void memory_pool_free(MemoryPool *pool);

// Allocate/deallocate from pool (O(1) operations)
void *memory_pool_alloc(MemoryPool *pool);
void memory_pool_dealloc(MemoryPool *pool, void *ptr);

// Check if pointer belongs to pool
bool memory_pool_contains(MemoryPool *pool, void *ptr);

// Arena allocator - for temporary allocations
typedef struct {
    void *memory;
    u32 size;
    u32 offset;
    u32 alignment;
} Arena;

void arena_init(Arena *arena, void *memory, u32 size, u32 alignment);
void *arena_alloc(Arena *arena, u32 size);
void arena_reset(Arena *arena); // Reset to beginning (doesn't free memory)

// Note: StackAllocator forward declared in memory.h - see allocator_stack.h for full definition
// Removed duplicate typedef to avoid conflicts

#endif // MEMORY_POOL_H

