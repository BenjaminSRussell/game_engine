#ifndef ALLOCATOR_POOL_H
#define ALLOCATOR_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct PoolAllocator PoolAllocator;

/**
 * Create a new pool allocator for fixed-size elements
 * @param element_size Size of each element in bytes
 * @param initial_capacity Initial number of elements
 * @param expandable Whether pool can expand when full
 * @return Pointer to allocator or NULL on failure
 */
PoolAllocator* pool_allocator_create(size_t element_size, size_t initial_capacity, bool expandable);

/**
 * Destroy a pool allocator and free all memory
 * @param pool Pointer to allocator
 */
void pool_allocator_destroy(PoolAllocator *pool);

/**
 * Allocate an element from the pool
 * @param pool Pointer to allocator
 * @return Pointer to element or NULL if pool is full
 */
void* pool_allocator_allocate(PoolAllocator *pool);

/**
 * Deallocate an element back to the pool
 * @param pool Pointer to allocator
 * @param element Pointer to element to deallocate
 */
void pool_allocator_deallocate(PoolAllocator *pool, void *element);

/**
 * Allocate multiple elements at once
 * @param pool Pointer to allocator
 * @param count Number of elements to allocate
 * @return Array of pointers to elements or NULL on failure
 */
void** pool_allocator_allocate_batch(PoolAllocator *pool, size_t count);

/**
 * Deallocate multiple elements at once
 * @param pool Pointer to allocator
 * @param elements Array of pointers to elements
 * @param count Number of elements to deallocate
 */
void pool_allocator_deallocate_batch(PoolAllocator *pool, void **elements, size_t count);

/**
 * Get pool statistics
 * @param pool Pointer to allocator
 * @param element_size Output for element size (can be NULL)
 * @param total_capacity Output for total capacity (can be NULL)
 * @param used_count Output for used count (can be NULL)
 * @param peak_usage Output for peak usage (can be NULL)
 * @param allocation_count Output for total allocation count (can be NULL)
 */
void pool_allocator_get_stats(PoolAllocator *pool, size_t *element_size, uint32_t *total_capacity, 
                              uint32_t *used_count, uint32_t *peak_usage, uint32_t *allocation_count);

/**
 * Check if pool is full
 * @param pool Pointer to allocator
 * @return True if pool is full and cannot expand
 */
bool pool_allocator_is_full(PoolAllocator *pool);

/**
 * Get pool utilization as percentage
 * @param pool Pointer to allocator
 * @return Utilization percentage (0.0 to 1.0)
 */
float pool_allocator_get_utilization(PoolAllocator *pool);

/**
 * Set zero initialization on allocation
 * @param pool Pointer to allocator
 * @param zero_on_alloc Whether to zero-initialize allocated elements
 */
void pool_allocator_set_zero_on_alloc(PoolAllocator *pool, bool zero_on_alloc);

#ifdef __cplusplus
}
#endif

#endif // ALLOCATOR_POOL_H
