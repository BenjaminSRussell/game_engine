#ifndef MEMORY_MACROS_H
#define MEMORY_MACROS_H

#include "core/memory/unified_memory_allocator.h"

// ============================================================================
// CONVENIENCE ALLOCATION MACROS
// ============================================================================

// Basic allocation with automatic file/line tracking
#define ALLOC(size)                                                            \
  unified_memory_alloc(size, MEMORY_STRATEGY_DEFAULT, 0, __FILE__, __LINE__,   \
                       __FUNCTION__)

// Tracked allocation (enables leak detection for this allocation)
#define ALLOC_TRACKED(size)                                                    \
  unified_memory_alloc(size, MEMORY_STRATEGY_TRACKED, MEMORY_FLAG_TRACK,       \
                       __FILE__, __LINE__, __FUNCTION__)

// Zero-initialized allocation
#define ALLOC_ZERO(size)                                                       \
  unified_memory_alloc(size, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_ZERO,        \
                       __FILE__, __LINE__, __FUNCTION__)

// Aligned allocation (64-byte alignment)
#define ALLOC_ALIGNED(size)                                                    \
  unified_memory_alloc(size, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_ALIGN,       \
                       __FILE__, __LINE__, __FUNCTION__)

// Guarded allocation (with guard pages for overflow detection)
#define ALLOC_GUARDED(size)                                                    \
  unified_memory_alloc(size, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_GUARD,       \
                       __FILE__, __LINE__, __FUNCTION__)

// Free with automatic tracking
#define FREE(ptr) unified_memory_free(ptr, __FILE__, __LINE__, __FUNCTION__)

// ============================================================================
// TYPE-SAFE ALLOCATION MACROS
// ============================================================================

// Allocate single instance of a type
#define ALLOC_TYPE(type) (type *)ALLOC(sizeof(type))

// Allocate single instance of a type (zero-initialized)
#define ALLOC_TYPE_ZERO(type) (type *)ALLOC_ZERO(sizeof(type))

// Allocate array of a type
#define ALLOC_ARRAY(type, count) (type *)ALLOC(sizeof(type) * (count))

// Allocate array of a type (zero-initialized)
#define ALLOC_ARRAY_ZERO(type, count) (type *)ALLOC_ZERO(sizeof(type) * (count))

// ============================================================================
// POOL ALLOCATION MACROS
// ============================================================================

// Allocate from a specific pool
#define POOL_ALLOC(pool, size) unified_memory_pool_alloc(pool, size, 0)

// Allocate from pool (zero-initialized)
#define POOL_ALLOC_ZERO(pool, size)                                            \
  unified_memory_pool_alloc(pool, size, MEMORY_FLAG_ZERO)

// Free to pool
#define POOL_FREE(pool, ptr) unified_memory_pool_free(pool, ptr)

// ============================================================================
// STACK ALLOCATION MACROS (for frame-based allocations)
// ============================================================================

// Allocate from stack allocator
#define STACK_ALLOC(stack, size) unified_memory_stack_alloc(stack, size, 0)

// Allocate from stack (zero-initialized)
#define STACK_ALLOC_ZERO(stack, size)                                          \
  unified_memory_stack_alloc(stack, size, MEMORY_FLAG_ZERO)

// ============================================================================
// ARENA ALLOCATION MACROS (for bulk allocations)
// ============================================================================

// Allocate from arena
#define ARENA_ALLOC(arena, size) unified_memory_arena_alloc(arena, size, 0)

// Allocate from arena (zero-initialized)
#define ARENA_ALLOC_ZERO(arena, size)                                          \
  unified_memory_arena_alloc(arena, size, MEMORY_FLAG_ZERO)

// Allocate type from arena
#define ARENA_ALLOC_TYPE(arena, type) (type *)ARENA_ALLOC(arena, sizeof(type))

// Allocate array from arena
#define ARENA_ALLOC_ARRAY(arena, type, count)                                  \
  (type *)ARENA_ALLOC(arena, sizeof(type) * (count))

#endif // MEMORY_MACROS_H
