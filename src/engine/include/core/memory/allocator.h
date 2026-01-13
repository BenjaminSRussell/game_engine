#pragma once

#include <common.h>
#include <stddef.h>
#include <stdint.h>

/**
 * =================================================================================================
 *                           GENERIC ALLOCATOR INTERFACE
 * =================================================================================================
 *
 * Purpose: Defines a unified interface for memory allocation across different allocator types
 * (linear, pool, stack, aligned, etc.). Provides a polymorphic allocator that can use any
 * of the specialized allocators.
 */

/* ===== ALLOCATOR TYPES ===== */

typedef enum {
    ALLOCATOR_LINEAR,
    ALLOCATOR_POOL,
    ALLOCATOR_STACK,
    ALLOCATOR_ALIGNED,
    ALLOCATOR_DEFAULT,
} AllocatorType;

/**
 * Generic allocator structure that can wrap any concrete allocator implementation
 */
typedef struct {
    AllocatorType type;
    void *context;  // Points to specific allocator implementation (LinearAllocator*, PoolAllocator*, etc.)

    // Function pointers for allocator operations
    void* (*allocate)(void *context, size_t size, size_t alignment);
    void (*deallocate)(void *context, void *ptr);
    void (*reset)(void *context);
    void (*destroy)(void *context);
    size_t (*get_allocated)(void *context);
    size_t (*get_total)(void *context);
} Allocator;

/* ===== ALLOCATOR API ===== */

/**
 * Create a generic allocator from a concrete allocator type
 */
Allocator allocator_create(AllocatorType type, void *impl);

/**
 * Allocate memory with given size and alignment
 */
void* allocator_allocate(Allocator *alloc, size_t size, size_t alignment);

/**
 * Allocate memory with default alignment (typically 16 bytes)
 */
void* allocator_allocate_default(Allocator *alloc, size_t size);

/**
 * Deallocate previously allocated memory
 */
void allocator_deallocate(Allocator *alloc, void *ptr);

/**
 * Reset allocator to initial state (for linear/stack allocators)
 */
void allocator_reset(Allocator *alloc);

/**
 * Destroy allocator and free all resources
 */
void allocator_destroy(Allocator *alloc);

/**
 * Get currently allocated memory
 */
size_t allocator_get_allocated(Allocator *alloc);

/**
 * Get total available/allocated memory
 */
size_t allocator_get_total(Allocator *alloc);

/* ===== CONVENIENCE MACROS ===== */

#define ALLOCATE(alloc, type, count) \
    (type *)allocator_allocate((alloc), sizeof(type) * (count), _Alignof(type))

#define ALLOCATE_SINGLE(alloc, type) \
    (type *)allocator_allocate((alloc), sizeof(type), _Alignof(type))
